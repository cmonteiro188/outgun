#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>	// auto_ptr
#include <sstream>
#include <string>
#include <vector>

#include "incalleg.h"
#include "commont.h"
#include "gamemod.h"
#include "leetnet/sleep.h"	// MS_SLEEP
#include "nassert.h"
#include "thread.h"
#include "world.h"

// implements:
#include "server.h"
#include "gameserver_interface.h"

const int minimum_positive_score_for_ranking = 100;
const int voteAnnounceInterval = 5;	// in seconds, how often a changing voting status will be announced

using std::endl;
using std::find;
using std::ifstream;
using std::ios;
using std::istringstream;
using std::max;
using std::ofstream;
using std::ostringstream;
using std::random_shuffle;
using std::setfill;
using std::setprecision;
using std::setw;
using std::string;
using std::swap;
using std::vector;

gameserver_c::gameserver_c(LogSet& hostLogs, const ServerExternalSettings& config) :
	normalLog(wheregamedir + "log" + directory_separator + "serverlog.txt", true),
	errorLog(normalLog, "ERROR: "),
	securityLog(normalLog, "SECURITY WARNING: ", wheregamedir + "log" + directory_separator + "server_securitylog.txt", false),
	log(&normalLog, &errorLog, &securityLog),
	world(this, &network, log),
	network(this, world, log),
	extConfig(config),
	authorizations(log)
{
	hostLogs("See serverlog.txt for server's log messages");
	setMaxPlayers(MAX_PLAYERS);
	next_vote_announce_frame = 0;
	last_vote_announce_votes = last_vote_announce_needed = 0;
	fav_colors[0].resize(16, false);
	fav_colors[1].resize(16, false);
	Thread::setCallerPriority(config.priority);
}

gameserver_c::~gameserver_c() {
	errorMessage("Server had these errors: (see serverlog.txt)", errorLog);
}

void gameserver_c::mutePlayer(int pid, int mode, int admin) {	// 0 = unmute, 1 = normal, 2 = mute silently (do not inform the player)
	if (world.player[pid].muted == mode)
		return;
	const char* adminName = (admin == -1 ? "The admin" : world.player[admin].name.c_str());
	if (mode == 0 && world.player[pid].muted != 2)
		network.plprintf(pid, msg_warning, "You have been unmuted (you can send messages again)");
	else if (mode == 1) {
		network.plprintf(pid, msg_warning, "You have been muted (you can't send messages)");
		if (admin != -1)
			network.plprintf(pid, msg_info, "The administrator responsible is %s.", adminName);
	}
	for (int i = 0; i < MAX_PLAYERS; ++i)
		if (world.player[i].used && i != pid) {
			if (mode == 0)
				network.plprintf(i, msg_info, "%s has unmuted %s", adminName, world.player[pid].name.c_str());
			else
				network.plprintf(i, msg_info, "%s has muted %s", adminName, world.player[pid].name.c_str());
		}
	world.player[pid].muted = mode;
}

void gameserver_c::kickPlayer(int pid, int admin, int minutes) {	// if minutes > 0, it's really a ban
	const char* adminName = (admin == -1 ? "The admin" : world.player[admin].name.c_str());
	if (minutes > 0)
		network.plprintf(pid, msg_warning, "You are BANNED from this server for %s!", approxTime(minutes * 60).c_str());
	else
		network.plprintf(pid, msg_warning, "You are being kicked from this server!");
	if (admin != -1)
		network.plprintf(pid, msg_info, "The administrator responsible is %s.", adminName);
	for (int i = 0; i < MAX_PLAYERS; ++i)
		if (world.player[i].used && i != pid)
			network.plprintf(i, msg_info, "%s has %s %s (disconnect in 10 seconds)", adminName, minutes > 0 ? "banned" : "kicked", world.player[pid].name.c_str());
	world.player[pid].kickTimer = 10 * 10;
}

void gameserver_c::banPlayer(int pid, int admin, int minutes) {
	authorizations.load();
	const NLaddress addr = network.get_client_address(world.player[pid].cid);
	authorizations.ban(addr, world.player[pid].name, minutes);
	authorizations.save();
	kickPlayer(pid, admin, minutes);
}

bool gameserver_c::check_name_password(const string& name, const string& password) const {
	return authorizations.checkNamePassword(name, password);
}

void gameserver_c::ctf_game_restart() {
	//submit all pending reports and update tournament participation flags
	for (int i = 0; i < maxplayers; i++)
		if (world.player[i].used) {
			network.client_report_status(world.player[i].cid);
			if (client[world.player[i].cid].current_participation != client[world.player[i].cid].next_participation) {
				client[world.player[i].cid].current_participation = client[world.player[i].cid].next_participation;
				network.broadcast_player_crap(i);
			}
		}

	ostringstream msg;
	msg << "CTF GAME OVER - FINAL SCORE: RED " << world.teams[0].score() << " - BLUE " << world.teams[1].score();
	network.broadcast_message(msg_info, msg.str());

	if (worldConfig.balanceTeams()) {
		balance_teams();
		if (worldConfig.balanceTeams() == WorldSettings::TB_balance_and_shuffle)
			shuffle_teams();
	}

	msg.str("");
	if (worldConfig.getCaptureLimit() > 0) {
		msg << "CAPTURE " << worldConfig.getCaptureLimit() << " FLAGS TO WIN THE GAME";
		if (worldConfig.getTimeLimit() > 0)
			msg << " - ";
	}
	if (worldConfig.getTimeLimit() > 0)
		msg << "TIME LIMIT IS " << worldConfig.getTimeLimit() / 10 / 60 << " MINUTES";
	network.broadcast_message(msg_info, msg.str());

	network.broadcast_sample(SAMPLE_CTF_GAMEOVER);

	network.sendWorldReset();	// must be before world.reset() because world.reset() already sends initializations
	world.reset();
}

void gameserver_c::balance_teams() {
	vector<int> team[2];
	for (int i = 0; i < maxplayers; i++)
		if (world.player[i].used)
			team[i / TSIZE].push_back(i);
	int difference = team[0].size() - team[1].size();
	const int bigger_team = (difference > 0 ? 0 : 1);
	while (difference > 1 || difference < -1) {
		const int victim = rand() % team[bigger_team].size();
		// Find a free slot in another team and move victim there.
		for (int i = (1 - bigger_team) * TSIZE; i < (2 - bigger_team) * TSIZE; i++)
			if (!world.player[i].used) {
				move_player(team[bigger_team][victim], i);
				team[1 - bigger_team].push_back(team[bigger_team][victim]);
				team[bigger_team].erase(team[bigger_team].begin() + victim);
				break;
			}
		difference = team[0].size() - team[1].size();
	}
}

void gameserver_c::shuffle_teams() {	// weird system, because players table has gaps
	vector<int> players;
	for (int i = 0; i < maxplayers; i++)
		if (world.player[i].used)
			players.push_back(i);
	vector<int> swap = players;
	random_shuffle(swap.begin(), swap.end());
	for (int i = 0, sw = 0; i < maxplayers; i++)
		if (world.player[i].used) {
			if (players[sw] / TSIZE != swap[sw] / TSIZE)
				swap_players(players[sw], swap[sw]);
			sw++;
		}
}

//check if team change requests can be satisfied
void gameserver_c::check_team_changes() {
	// check players in random order
	for (int i = 0; i < maxplayers; i++)
		check[i] = 0;
	checount = maxplayers;
	while (checount > 0) {
		int p = rand() % maxplayers;
		if (!check[p]) {
			check[p] = 1;
			checount--;
			check_player_change_teams(p);
		}
	}
}

// Check if a player wants to change teams and if yes, try to fullfill the wish.
void gameserver_c::check_player_change_teams(int pid) {
	if (!world.player[pid].used || !world.player[pid].want_change_teams)
		return;
	if (get_time() < world.player[pid].team_change_time) {
		world.player[pid].team_change_pending = true;
		return;
	}

	//count players in each team
	int tc[2] = { 0, 0 };
	for (int i = 0; i < maxplayers; i++)
		if (world.player[i].used)
			tc[i / TSIZE]++;

	//check if team changing happens: calculate delta TARGET TEAM - MY TEAM
	const int teamdelta = tc[1 - pid / TSIZE] - tc[pid / TSIZE];

	// target team with MORE players: do not move
	if (teamdelta > 0)
		return;
	// target team with 2 players less: move player without a trade
	if (teamdelta <= -2)
		for (int i = 0; i < maxplayers; i++)
			if (i / TSIZE != pid / TSIZE)
				if (!world.player[i].used) {
					move_player(pid, i);
					return;
				}
	// Find a trade.
	for (int i = 0; i < maxplayers; i++)
		if (world.player[i].used && i / TSIZE != pid / TSIZE && world.player[i].want_change_teams) {
			swap_players(pid, i);
			return;
		}

	// If the old team has one player more, move the player.
	if (teamdelta == -1)
		for (int i = 0; i < maxplayers; i++)
			if (i / TSIZE != pid / TSIZE && !world.player[i].used) {
				move_player(pid, i);
				return;
			}
}

//move player - move player (f rom) to empty position (t o)
void gameserver_c::move_player(int f, int t) {
	//broadcast sound
	network.broadcast_sample(SAMPLE_CHANGETEAM);

	//UGLY HACK
	if (!check[t]) {
		check[t] = 1;
		checount--;
	}

	world.dropFlagIfAny(f);

	fav_colors[f / TSIZE][world.player[f].color()] = false;
	world.player[f].set_color(-1);

	//copy to t
	world.player[t] = world.player[f];

	//change rockets owner from f to t
	world.changeRocketsOwner(f, t);

	//remove f
	game_remove_player(f, false);

	world.player[t].id = t;

	//I really don't want to change teams anymore.
	world.player[t].want_change_teams = false;
	world.player[t].team_change_time = get_time() + 10.0;		//10 secs interval

	//kill t
	if (world.player[t].health > 0)
		world.resetPlayer(t);

	check_fav_colors(t);

	//update t
	network.move_update_player(t);
}

//swap players - both are valid players
void gameserver_c::swap_players(int a, int b) {
	network.broadcast_sample(SAMPLE_CHANGETEAM);

	if (world.player[a].health > 0)
		world.resetPlayer(a);
	if (world.player[b].health > 0)
		world.resetPlayer(b);

	fav_colors[a / TSIZE][world.player[a].color()] = false;
	fav_colors[b / TSIZE][world.player[b].color()] = false;
	world.player[a].set_color(-1);
	world.player[b].set_color(-1);

	swap(world.player[a], world.player[b]);
	world.swapRocketOwners(a, b);
	world.player[a].id = a;
	world.player[b].id = b;

	// either don't want to change teams anymore
	world.player[a].want_change_teams = false;
	world.player[a].team_change_time = get_time() + 10.0;		//10 secs interval
	world.player[b].want_change_teams = false;
	world.player[b].team_change_time = get_time() + 10.0;		//10 secs interval

	check_fav_colors(a);
	check_fav_colors(b);

	// send updates
	network.move_update_player(a);
	network.move_update_player(b);
}

void gameserver_c::set_fav_colors(int pid, const vector<char>& colors) {
	if (world.player[pid].used)
		world.player[pid].set_fav_colors(colors);
}

void gameserver_c::check_fav_colors(int pid) {
	ServerPlayer& player = world.player[pid];
	if (!player.used)
		return;
	const int team = pid / TSIZE;
	const vector<char>& player_colors = player.fav_colors();
	// check favourite colours
	for (vector<char>::const_iterator col = player_colors.begin(); col != player_colors.end(); col++) {
		nAssert(*col < static_cast<int>(fav_colors[team].size()));
		if (player.color() == *col)
			return;
		else if (!fav_colors[team][*col]) {
			if (player.color() != -1)
				fav_colors[team][player.color()] = false;
			player.set_color(*col);
			fav_colors[team][player.color()] = true;
			return;
		}
	}
	// if no favourites free, check all colours
	for (int i = 0; i < static_cast<int>(fav_colors[team].size()); i++)
		if (!fav_colors[team][i]) {
			if (player.color() != -1)
				fav_colors[team][player.color()] = false;
			player.set_color(i);
			fav_colors[team][player.color()] = true;
			return;
		}
	nAssert(0);		// should never go here
}

void gameserver_c::sendMessage(int pid, Message_type type, const std::string& msg) {
	network.player_message(pid, type, msg);
}

//refresh team ratings
void gameserver_c::refresh_team_score_modifiers() {
	double raw[2];
	raw[0] = 0.0;
	raw[1] = 0.0;

	//somatorio raw ratings
	for (int p = 0; p < maxplayers; p++)
		if (world.player[p].used) {
			// use "1.0" rating for anybody with less than 100 positive points
			if (client[world.player[p].cid].score < minimum_positive_score_for_ranking)
				raw[p / TSIZE] += 1.0;
			else
				raw[p / TSIZE] += (client[world.player[p].cid].score + 1.0) / (client[world.player[p].cid].neg_score + 1.0);
		}

	//modifiers
	team_smul[0] = bound(raw[1] / raw[0], .3333, 3.);
	team_smul[1] = bound(raw[0] / raw[1], .3333, 3.);
}

//score!
void gameserver_c::score_frag(int pid, int amount) {
	world.player[pid].stats().add_frag(amount);

	const int cid = world.player[pid].cid;

	// add tournament scoring delta if all criteria for tournament scoring are satisfied
	if (tournament && network.get_player_count() >= 2 && client[cid].current_participation) {
		refresh_team_score_modifiers();
		client[cid].fdp += amount * team_smul[pid / TSIZE];
		client[cid].delta_score = static_cast<int>(client[cid].fdp);
	}
}

//score! NEG FRAG (v0.4.8)
void gameserver_c::score_neg(int p, int amount) {
	const int cid = world.player[p].cid;

	// add tournament scoring delta if all criteria for tournament scoring are satisfied
	if (tournament && network.get_player_count() >= 2 && client[cid].current_participation) {
		client[cid].fdn += amount;	// not affected by team modifier
		client[cid].neg_delta_score = static_cast<int>(client[cid].fdn);
	}
}

bool gameserver_c::trySetMaxplayers(int val) {
	if (val != maxplayers && network.get_player_count() != 0) {
		log.error("Can't change max_players while players are connected");
		return false;
	}
	setMaxPlayers(val);
	return true;
}

bool checkMaxplayerSetting(int val) { return (val >= 2 && val <= MAX_PLAYERS && val % 2 == 0); }	// helper for load_game_mod

void gameserver_c::load_game_mod() {
	RedirectToMemFun1<ServerNetworking, void, const std::string&> setHostname(&network, &ServerNetworking::set_hostname);
	RedirectToMemFun1<ServerNetworking, void, const std::string&> setServerPassword(&network, &ServerNetworking::set_server_password);

	RedirectToFun1<bool, int> checkMaxplayer(checkMaxplayerSetting);
	RedirectToMemFun1<gameserver_c, bool, int> tryMaxplayer(this, &gameserver_c::trySetMaxplayers);

	RedirectToMemFun1<ServerNetworking, void, const std::string&> addWebServer(&network, &ServerNetworking::add_web_server);
	RedirectToMemFun1<ServerNetworking, void, const std::string&> setWebScript(&network, &ServerNetworking::set_web_script);
	RedirectToMemFun1<ServerNetworking, void, const std::string&> setWebAuth(&network, &ServerNetworking::set_web_auth);
	RedirectToMemFun1<ServerNetworking, bool, int> setWebRefresh(&network, &ServerNetworking::set_web_refresh);

	typedef std::auto_ptr<GamemodSetting> PT;
	static PT settings[] = {
		PT(new GS_Float		("friction",				&world.physics.fric)),
		PT(new GS_Float		("drag",					&world.physics.drag)),
		PT(new GS_Float		("acceleration",			&world.physics.accel)),
		PT(new GS_Float		("run_acceleration",		&world.physics.run_mul)),
		PT(new GS_Float		("turbo_acceleration",		&world.physics.turbo_mul)),
		PT(new GS_Float		("flag_acceleration",		&world.physics.flag_mul)),
		PT(new GS_Boolean	("player_collisions",		&world.physics.player_collisions)),
		PT(new GS_Boolean	("friendly_fire",			&world.physics.friendly_fire)),
		PT(new GS_Boolean	("friendly_deathbringer",	&world.physics.friendly_db)),
		PT(new GS_Map		("map",						&maprot)),
		PT(new GS_PowerupNum("pups_min",				&pupConfig.pups_min, &pupConfig.pups_min_percentage)),
		PT(new GS_PowerupNum("pups_max",				&pupConfig.pups_max, &pupConfig.pups_max_percentage)),
		PT(new GS_Int		("pups_respawn_time",		&pupConfig.pups_respawn_time,		0)),
		PT(new GS_Int		("pup_chance_shield",		&pupConfig.pup_chance_shield,		0)),
		PT(new GS_Int		("pup_chance_turbo",		&pupConfig.pup_chance_turbo,		0)),
		PT(new GS_Int		("pup_chance_shadow",		&pupConfig.pup_chance_shadow,		0)),
		PT(new GS_Int		("pup_chance_power",		&pupConfig.pup_chance_power,		0)),
		PT(new GS_Int		("pup_chance_weapon",		&pupConfig.pup_chance_weapon,		0)),
		PT(new GS_Int		("pup_chance_megahealth",	&pupConfig.pup_chance_megahealth,	0)),
		PT(new GS_Int		("pup_chance_deathbringer",	&pupConfig.pup_chance_deathbringer,	0)),
		PT(new GS_Ulong		("time_limit",				&worldConfig.time_limit, 0, GS_Ulong::lim::max(), 60 * 10)),	// convert minutes to frames
		PT(new GS_Ulong		("extra_time",				&worldConfig.extra_time, 0, GS_Ulong::lim::max(), 60 * 10)),	// convert minutes to frames
		PT(new GS_Boolean	("sudden_death",			&worldConfig.sudden_death)),
		PT(new GS_Int		("game_end_delay",			&game_end_delay, 0)),
		PT(new GS_Int		("capture_limit",			&worldConfig.capture_limit, 0)),
		PT(new GS_Balance	("balance_teams",			&worldConfig.balance_teams)),
		PT(new GS_ForwardStr("server_name",				setHostname)),
		PT(new GS_ForwardInt("max_players",				string() + "an even integer between 2 and " + itoa(MAX_PLAYERS), checkMaxplayer, tryMaxplayer)),
		PT(new GS_AddString	("welcome_message",			&welcome_message)),
		PT(new GS_AddString	("info_message",			&info_message)),
		PT(new GS_ForwardStr("server_password",			setServerPassword)),
		PT(new GS_Int		("pup_add_time",			&pupConfig.pup_add_time, 1, 999)),
		PT(new GS_Int		("pup_max_time",			&pupConfig.pup_max_time, 1, 999)),
		PT(new GS_Boolean	("pup_deathbringer_switch",	&pupConfig.pup_deathbringer_switch)),
		PT(new GS_Float		("pup_deathbringer_time",	&pupConfig.pup_deathbringer_time, 1.)),
		PT(new GS_Boolean	("pups_drop_at_death",		&pupConfig.pups_drop_at_death)),
		PT(new GS_Int		("pup_health_bonus",		&pupConfig.pup_health_bonus, 1)),
		PT(new GS_Float		("pup_power_damage",		&pupConfig.pup_power_damage, 0.)),
		PT(new GS_Int		("pup_weapon_max",			&pupConfig.pup_weapon_max, 1, 9, 1, -1)),	// decrease by 1 to end up with the internal setting
		PT(new GS_Boolean	("random_maprot",			&random_maprot)),
		PT(new GS_Int		("vote_block_time",			&vote_block_time, 0, GS_Int::lim::max(), 60 * 10)),	// convert minutes to frames
		PT(new GS_Int		("idlekick_time",			&idlekick_time, 10, GS_Int::lim::max(), 1, 0, true)),	// special setting: allow 0 that is outside the normal range
		PT(new GS_Double	("respawn_time",			&worldConfig.respawn_time, 0.)),
		PT(new GS_Double	("waiting_time_deathbringer",	&worldConfig.waiting_time_deathbringer, 0.)),
		PT(new GS_Int		("pup_shadow_invisibility",	&worldConfig.shadow_minimum, 0, 1, -WorldSettings::shadow_minimum_normal, +WorldSettings::shadow_minimum_normal)),	// 0->smn, 1->0
		PT(new GS_Int		("rocket_damage",			&worldConfig.rocket_damage, 0)),
		PT(new GS_Boolean	("sayadmin_enabled",		&sayadmin_enabled)),
		PT(new GS_String	("sayadmin_comment",		&sayadmin_comment)),
		PT(new GS_Boolean	("tournament",				&tournament)),
		PT(new GS_Boolean	("save_stats",				&save_stats)),
		PT(new GS_String	("server_website",			&server_website_url)),
		PT(new GS_ForwardStr("web_server",				addWebServer)),
		PT(new GS_ForwardStr("web_script",				setWebScript)),
		PT(new GS_ForwardStr("web_auth",				setWebAuth)),
		PT(new GS_ForwardInt("web_refresh",				"at least 1", setWebRefresh, setWebRefresh)),
		PT(0)
	};

	const string filename = wheregamedir + "config" + directory_separator + "gamemod.txt";
	ifstream in(filename.c_str());
	if (in) {
		log("Loading game mod: '%s'", filename.c_str());
		string line;
		while (getline_skip_comments(in, line)) {
			string cmd, value;
			istringstream ist(line);
			ist >> cmd;
			ist.ignore();
			getline(ist, value);
			for (int si = 0;; ++si) {
				if (&*settings[si] == 0) {	// end of settings marker
					log.error("Unrecognized gamemod setting: '%s'", cmd.c_str());
					break;
				}
				if (settings[si]->matchCommand(cmd)) {
					settings[si]->set(log, value);	// ignore return value; the status is logged
					break;
				}
			}
		}

		if ((pupConfig.pups_min_percentage == pupConfig.pups_max_percentage && pupConfig.pups_min > pupConfig.pups_max) ||
				pupConfig.pups_max == 0)	// if they are in different units, only the value of 0 is comparable
			pupConfig.pups_min = pupConfig.pups_max;

		if (!server_website_url.empty())
			info_message.push_back(string() + "Website: " + server_website_url);

		log("Game mod file read.");
		in.close();
	}
	else
		log.error("Can't open game mod file: '%s'", filename.c_str());
	world.setConfig(worldConfig, pupConfig);
}

//load a map from the rotation list
bool gameserver_c::load_rotation_map(int pos) {
	const bool ok = world.load_map(SERVER_MAPS_DIR, maprot[pos].file);
	if (!ok)
		return false;
	log("Map number %i: '%s'", pos, maprot[pos].file.c_str());
	return true;
}

bool gameserver_c::server_next_map(int reason) {
	network.update_serverinfo();

	nAssert(!maprot.empty());

	if (save_stats)
		world.save_stats("server_stats", current_map().title);

	vector<int> winners;
	int maxVotes = 0;
	for (int m = 0; m < static_cast<int>(maprot.size()); ++m) {
		if (maprot[m].votes < maxVotes)
			continue;
		if (maprot[m].votes > maxVotes) {
			maxVotes = maprot[m].votes;
			winners.clear();
		}
		winners.push_back(m);
	}
	if (maxVotes == 0)
		currmap = (currmap + 1) % maprot.size();
	else {
		if (winners.size() > 1) {
			vector<int>::iterator it = find(winners.begin(), winners.end(), currmap);
			if (it != winners.end())
				winners.erase(it);
		}
		currmap = winners[rand() % winners.size()];
	}
	// clear votes for the current map
	for (int p = 0; p < maxplayers; ++p) {
		world.player[p].want_map_exit = false;
		if (world.player[p].mapVote == currmap)
			world.player[p].mapVote = -1;
	}
	maprot[currmap].votes = 0;
	maprot[currmap].votes_changed = true;
	last_vote_announce_votes = last_vote_announce_needed = 0;
	next_vote_announce_frame = 0;	// let a new announcement be made as soon as someone votes

	if (!load_rotation_map(currmap))
		return false;

	// notify all players
	for (int i = 0; i < maxplayers; i++)
		if (world.player[i].used)
			network.send_map_change_message(i, reason, maprot[currmap].file.c_str());

	//important: server is showing gameover plaque. nobody should move or receive world frames
	gameover = true;
	gameover_time = get_time() + game_end_delay;		// timeout for gameover plaque

	ostringstream msg;
	msg << "Server changed map to: " << maprot[currmap].title << " (" << currmap + 1 << " of " << maprot.size() << ')';
	network.broadcast_message(msg_info, msg.str());

	return true;
}

//check map exit by vote
void gameserver_c::check_map_exit() {
	int num_for = 0, num_against = 0;
	for (int i = 0; i < maxplayers; i++)
		if (world.player[i].used) {
			if (world.player[i].want_map_exit)
				num_for++;
			else
				num_against++;
		}

	// this could be done elsewhere, but this function is called whenever votes change
	for (int m = 0; m < static_cast<int>(maprot.size()); ++m)
		maprot[m].votes = 0;
	for (int p = 0; p < maxplayers; ++p)
		if (world.player[p].used && world.player[p].mapVote != -1)
			++maprot[world.player[p].mapVote].votes;

	if ((world.getMapTime() >= vote_block_time && num_for > num_against) || (num_against == 0 && num_for)) {
		server_next_map(NEXTMAP_VOTE_EXIT);	// ignore return value
		ctf_game_restart();
	}
}

//----- THE REST  ----------------

bool gameserver_c::reset_settings(bool keepMap) {
	string currMapFile;
	if (keepMap)
		currMapFile = maprot[currmap].file;

	world.physics = PhysicalSettings();	// default values
	maprot.clear();
	pupConfig.reset();
	worldConfig.reset();
	currmap = 0;

	network.set_hostname("");
	network.set_server_password("");

	game_end_delay = 5;
	random_maprot = false;
	vote_block_time = 0;	// no limit
	idlekick_time = 0;		// no limit

	welcome_message.clear();
	info_message.clear();

	sayadmin_comment.clear();
	sayadmin_enabled = false;

	server_website_url.clear();

	tournament = true;
	save_stats = false;

	network.clear_web_servers();
	network.set_web_refresh(2);

	// load server configuration from gamemod.txt
	load_game_mod();

	// did not specify maps, scan "maps/" folder for .txt map files
	if (maprot.empty()) {
		string searchPattern = wheregamedir + SERVER_MAPS_DIR + directory_separator + "*.txt";

		log("Scanning for maps: '%s'", searchPattern.c_str());

		al_ffblk mapffblk;	//for al_find*

		int result = al_findfirst(searchPattern.c_str(), &mapffblk, FA_ARCH | FA_RDONLY);
		while (result == 0) {
			char nameBuf[500];
			//char *replace_extension(char *dest, const char *filename, const char *ext, int size
			replace_extension(nameBuf, mapffblk.name, "", 500);
			nameBuf[strlen(nameBuf) - 1] = 0;	// erase last '.'

			MapInfo mi;
			if (mi.load(log, nameBuf)) {
				maprot.push_back(mi);
				log("Added '%s' to map rotation", nameBuf);
			}
			else
				log.error("Can't add '%s' to map rotation", nameBuf);

			//try next
			result = al_findnext(&mapffblk);
		}
	}

	if (maprot.empty()) {
		log.error("No maps for rotation");
		return false;
	}

	if (random_maprot)
		random_shuffle(maprot.begin(), maprot.end());

	if (keepMap) {
		size_t mapi;
		for (mapi = 0; mapi < maprot.size(); ++mapi)
			if (maprot[mapi].file == currMapFile)
				break;
		if (mapi == maprot.size()) {	// not found
			currmap = -1;
			server_next_map(NEXTMAP_VOTE_EXIT);
		}
		else
			currmap = mapi;
	}
	return true;
}

//start server
bool gameserver_c::start(int target_maxplayers) {
	authorizations.load();

	//check if maxplayers is valid
	if (target_maxplayers < 2)				//menos de dois
		return false;
	if (target_maxplayers > MAX_PLAYERS)		//mais que o maximo
		return false;
	if (target_maxplayers % 2 == 1)	//numero impar de jogadores
		return false;

	// Set maxplayers, could be reset by gamemod setting.
	setMaxPlayers(target_maxplayers);

	//reset client_c struct (closes files...)
	for (int i = 0; i < MAX_PLAYERS; i++)
		client[i].reset();

	gameover = false;

	for (int i = 0; i < MAX_PLAYERS; i++)
		world.player[i].clear(false, i, 0, "", i / TSIZE);	// 0 : fake cid

	if (!reset_settings(false))
		return false;
	if (!load_rotation_map(currmap))
		return false;
	if (!network.start())	// this must be last, because network.stop() must always be called if start() succeeds
		return false;

	// reset game
	ctf_game_restart();

 	network.update_serverinfo();

	return true;
}

int gameserver_c::getLessScoredTeam() const {
	if (team_smul[0] > team_smul[1])
		return 0;
	else if (team_smul[1] > team_smul[0])
		return 1;
	else
		return rand() % 2;
}

void gameserver_c::game_remove_player(int pid, bool removeClient) {
	fav_colors[pid / TSIZE][world.player[pid].color()] = false;
	if (removeClient)
		client[world.player[pid].cid].reset();
	network.removePlayer(pid);
	world.removePlayer(pid);
}

void gameserver_c::disconnectPlayer(int pid, Disconnect_reason reason) {
	network.disconnect_client(world.player[pid].cid, 2, reason);
}

void gameserver_c::nameChange(int id, int pid, const string& tempname, const std::string& password) {
	if (tempname == world.player[pid].name)
		return;
	//name change flooding protection
	if (get_time() < world.player[pid].waitnametime)
		return;

	//FLUSH PENDING REPORTS TO MASTER IF token_have/token_valid !!!
	network.client_report_status(id);

	//name changed -- this means that the player is NOT REGISTERED
	//  anymore for recording statistics
	client[id].token_have = false;

	//check if it's the first name information from client. then it
	// must have just entered the game
	const bool entered_game = world.player[pid].name.empty();

	if (!check_name(tempname))
		disconnectPlayer(pid, disconnect_client_misbehavior);
	else {
		if (authorizations.checkNamePassword(tempname, password)) {
			world.player[pid].name = tempname;
			world.player[pid].waitnametime = get_time() + 1.0;
		}
		else if (entered_game)
			disconnectPlayer(pid, disconnect_client_misbehavior);
		else
			network.sendNameAuthorizationRequest(pid);
	}

	if (entered_game)
		network.broadcast_new_player_notice(pid);
	network.broadcast_player_name(pid);

	// token removed; possibly authorized and/or admin
	network.broadcast_player_crap(pid);
}

class PlayerMessager : public LineReceiver {
	gameserver_c& host;
	int player;
	Message_type type;

public:
	PlayerMessager(gameserver_c& server, int pid, Message_type mtype) : host(server), player(pid), type(mtype) { }
	PlayerMessager& operator()(const std::string& str) { host.sendMessage(player, type, str); return *this; }
};

bool gameserver_c::isLocallyAuthorized(int pid) const {
	return world.player[pid].localIP || authorizations.identifyName(world.player[pid].name) != -1;	// must have authorized because otherwise couldn't use the name
}

bool gameserver_c::isAdmin(int pid) const {
	return world.player[pid].localIP || authorizations.isAdmin(world.player[pid].name);
}

void gameserver_c::chat(int pid, const char* sbuf) {
	// handle 'console' commands
	if (sbuf[0] == '/') {
		bool admin = false;
		ClientData& cld = client[world.player[pid].cid];
		if ((cld.token_have && cld.token_valid) || isLocallyAuthorized(pid)) {
			// the player surely is who his name implies, authorized either by the tournament master or the local authorization database
			if (isAdmin(pid))
				admin = true;
		}

		const char* pCommand=sbuf+1;
		char cbuf[30];
		int ci;
		for (ci = 0;; ++ci, ++pCommand) {
			if (ci == 29) {
				cbuf[29]='\0';
				break;
			}
			if (*pCommand == ' ') {
				cbuf[ci] = '\0';
				++pCommand;
				break;
			}
			cbuf[ci] = *pCommand;
			if (*pCommand == '\0')
				break;
		}
		if (!strcmp(cbuf, "help")) {
			network.player_message(pid, msg_header, "Console commands available on this server:");
			network.player_message(pid, msg_normal, "/help       this screen");
			if (!info_message.empty())
				network.player_message(pid, msg_normal, "/info       information about this server");
			network.player_message(pid, msg_normal, "/config     current server configuration");
			network.player_message(pid, msg_normal, "/mapinfo n  information about map n (default: current map)");
			network.player_message(pid, msg_normal, "/time       check server uptime, current map time and time left on the map");
			if (sayadmin_enabled) {
				ostringstream ostr;
				ostr << "/sayadmin   send a message to the server admin";
				if (sayadmin_comment.length())
					ostr << " (" << sayadmin_comment << ')';
				network.player_message(pid, msg_normal, ostr.str());
			}
			if (admin) {
				network.player_message(pid, msg_header, "Admin commands:");
				network.player_message(pid, msg_normal, "/list       get a list of player ID's");
				network.player_message(pid, msg_normal, "/kick n     kick player with ID n");
				network.player_message(pid, msg_normal, "/ban n t    ban player with ID n for t minutes (default: 60)");
				network.player_message(pid, msg_normal, "/mute n     mute player with ID n");
				network.player_message(pid, msg_normal, "/smute n    silently mute player with ID n (crude!)");
				network.player_message(pid, msg_normal, "/unmute n   cancel muting of player with ID n");
				network.player_message(pid, msg_normal, "/forcemap   restart the game and change map if you've used votemap");
			}
		}
		else if (!strcmp(cbuf, "info") && !info_message.empty()) {
			network.player_message(pid, msg_header, info_message.front());
			for (vector<string>::const_iterator line = info_message.begin() + 1; line != info_message.end(); line++)
				network.player_message(pid, msg_normal, *line);
			network.player_message(pid, msg_normal, "type /config to see current server settings");
		}
		else if (!strcmp(cbuf, "config")) {
			network.player_message(pid, msg_header, "Current server settings:");
			PlayerMessager pm(*this, pid, msg_normal);
			worldConfig.print(pm);
			world.physics.print(pm);
			pupConfig.print(pm);
		}
		else if (!strcmp(cbuf, "sayadmin") && sayadmin_enabled) {
			if (strspnp(pCommand, " ")!=NULL) {
				ofstream log((wheregamedir + "log" + directory_separator + "sayadmin.log").c_str(), ios::out | ios::app);
				log << date_and_time() << "  " << world.player[pid].name << ": " << pCommand << endl;
				network.forwardSayadminMessage(world.player[pid].cid, pCommand);
				network.player_message(pid, msg_info, "Your message has been logged. Thank you for your feedback!");
			}
			else
				network.player_message(pid, msg_info, "For example to send \"Hello!\", type /sayadmin Hello!");
		}
		else if (!strcmp(cbuf, "map") || !strcmp(cbuf, "mapinfo")) {
			if (*pCommand!='\0') {
				int mid=atoi(pCommand)-1;
				if (mid>=0 && mid<(int)maprot.size() && pCommand[strspn(pCommand, "0123456789")]=='\0') {
					network.plprintf(pid, msg_info, "Map %d is %s (%s)", mid+1, maprot[mid].title.c_str(), maprot[mid].author.c_str());
					network.plprintf(pid, msg_info, "%s.txt, size %d×%d", maprot[mid].file.c_str(), maprot[mid].width, maprot[mid].height);
				}
				else
					network.plprintf(pid, msg_warning, "Valid map id's are 1 to %d", maprot.size());
			}
			else {
				network.plprintf(pid, msg_info, "This map is %s (%s)", maprot[currmap].title.c_str(), maprot[currmap].author.c_str());
				network.plprintf(pid, msg_info, "%s.txt, size %d×%d", maprot[currmap].file.c_str(), maprot[currmap].width, maprot[currmap].height);
			}
		}
		else if (!strcmp(cbuf, "time")) {
			PlayerMessager pm(*this, pid, msg_info);
			world.printTimeStatus(pm);
		}
		else if (!strcmp(cbuf, "list") && admin) {
			network.player_message(pid, msg_header, "Players on server: ID, login flags, name");
			for (int ppid = 0; ppid < MAX_PLAYERS; ) {
				char buf[100];
				int bufi = 0;
				for (int onrow = 0; onrow < 3 && ppid < MAX_PLAYERS; ++ppid)
					if (world.player[ppid].used) {
						snprintf(buf + bufi, 26, "%2d %4s %-18s", ppid, world.player[ppid].reg_status.strFlags().c_str(), world.player[ppid].name.c_str());
						bufi += 26;
						++onrow;
					}
				if (bufi > 0)
					network.player_message(pid, msg_normal, buf);
			}
		}
		else if ((!strcmp(cbuf, "kick") || !strcmp(cbuf, "ban") || !strcmp(cbuf, "mute")
					|| !strcmp(cbuf, "smute") || !strcmp(cbuf, "unmute")) && admin) {
			istringstream command(pCommand);
			int ppid;
			int time = 0;	// used only for bans
			command >> ppid;
			bool ok = command;
			if (!strcmp(cbuf, "ban")) {
				command >> time;
				if (command.eof())
					time = 60;	// default: 1 hour
				else if (!command)
					ok = false;
			}
			if (!ok)
				network.plprintf(pid, msg_warning, "Syntax error. Expecting \"/%s ID%s\".", cbuf, !strcmp(cbuf, "ban") ? " [minutes]" : "");
			else if (ppid < 0 || ppid >= MAX_PLAYERS || !world.player[ppid].used)
				network.player_message(pid, msg_warning, "No such player. Type /list for a list of IDs.");
			else if (time <= 0 || time > 60 * 24 * 7)	// allow at most a weeks ban (a bit over 10000 minutes)
				network.player_message(pid, msg_warning, "The ban time must be more than 0 and at most 10 000 minutes (1 week)");
			else {	// syntax OK
				if (!strcmp(cbuf, "kick"))
					kickPlayer(ppid, pid);
				else if (!strcmp(cbuf, "ban"))
					banPlayer(ppid, pid, time);
				else if (!strcmp(cbuf, "mute"))
					mutePlayer(ppid, 1, pid);
				else if (!strcmp(cbuf, "smute"))
					mutePlayer(ppid, 2, pid);
				else if (!strcmp(cbuf, "unmute"))
					mutePlayer(ppid, 0, pid);
				else
					nAssert(0);
			}
		}
		else if (!strcmp(cbuf, "forcemap") && admin) {
			if (world.player[pid].mapVote != -1 && world.player[pid].mapVote != currmap) {
				network.bprintf(msg_info, "%s decided it's time for a map change", world.player[pid].name.c_str());
				maprot[world.player[pid].mapVote].votes = 99;
				server_next_map(NEXTMAP_VOTE_EXIT); // ignore return value
			}
			else
				network.bprintf(msg_info, "%s decided it's time for a restart", world.player[pid].name.c_str());
			ctf_game_restart();
		}
		else
			network.plprintf(pid, msg_warning, "Unknown command %s. Type /help for a list.", cbuf);
	}
	else if (strspnp(sbuf, " ") != NULL) {	// ignore messages that are all spaces
		//talk flood protection
		world.player[pid].talk_temp += world.player[pid].talk_hotness;
		world.player[pid].talk_hotness += 3.0;
		if (world.player[pid].talk_temp > 18.0)
			world.player[pid].talk_temp = 18.0;
		if (world.player[pid].talk_hotness > 6.0)
			world.player[pid].talk_hotness = 6.0;
		if (world.player[pid].talk_temp > 10.0) {
			world.player[pid].talk_temp = 18.0;
			network.player_message(pid, msg_warning, "Too much talk. Chill...");
		}
		else if (world.player[pid].muted == 1)
			network.plprintf(pid, msg_warning, "You are muted. You can't send messages.");
		else {
			ostringstream msg;
			msg << world.player[pid].name << ": ";
			if (sbuf[0] == '.') {	// team message
				msg << sbuf + 1;
				if (world.player[pid].muted == 2)
					network.player_message(pid, msg_team, msg.str());
				else
					network.broadcast_team_message(pid / TSIZE, msg.str());
			}
			else {					// regular message
				msg << sbuf;
				if (world.player[pid].muted == 2)
					network.player_message(pid, msg_normal, msg.str());
				else
					network.broadcast_message(msg_normal, msg.str());
			}
		}
	}
}

bool gameserver_c::changeRegistration(int id, const string& token) {
	const int intoken = atoi(token.c_str());
	if (intoken == client[id].intoken)
		return false;

	// v0.4.9 FIX : IF HAD previous token have/valid, then FLUSH his stats
	network.client_report_status(id);

	strcpy(client[id].token, token.c_str());
	client[id].intoken = intoken;

	// NEW (or first) REGISTRATION -- reset player report / stop reporting his old ID
	client[id].neg_delta_score = 0;
	client[id].delta_score = 0;
	client[id].fdp = 0.0;
	client[id].fdn = 0.0;
	client[id].score = 0;
	client[id].neg_score = 0;
	client[id].rank = 0;

	client[id].token_have = !token.empty();	//token set
	client[id].token_valid = false;	//BUT not validated yet

	network.broadcast_player_crap(network.getPid(id));

	return client[id].token_have;
}

void gameserver_c::simulate_and_broadcast_frame() {
	//check end of gameover plaque
	if (gameover)
		if (gameover_time < get_time()) {
			gameover = false;
			world.reset_time();
			network.sendEndGameover();
		}
	if (!gameover)
		world.simulateFrame();

	if (world.frame >= next_vote_announce_frame) {	// announce voting status
		int votes = 0;
		for (int i = 0; i < maxplayers; ++i)
			if (world.player[i].used && world.player[i].want_map_exit)
				++votes;
		const int players = get_player_count() / 2 + 1;
		if (votes != last_vote_announce_votes || (players != last_vote_announce_needed && votes != 0)) {
			last_vote_announce_votes = votes;
			last_vote_announce_needed = players;
			next_vote_announce_frame = world.frame + voteAnnounceInterval * 10;
			ostringstream voteinfo;
			voteinfo << "*** " << votes << '/' << players << " votes for mapchange";
			if (world.getMapTime() < vote_block_time)
				voteinfo << " (all players needed for " << (vote_block_time - world.getMapTime() + 5) / 10 << " more seconds)";
			network.broadcast_message(msg_info, voteinfo.str().c_str());
		}
		else if (world.getMapTime() == vote_block_time)
			check_map_exit();
	}
	for (int i = 0; i < maxplayers; ++i)
		if (world.player[i].used) {
			if (world.player[i].kickTimer) {
				--world.player[i].kickTimer;
				if (world.player[i].kickTimer == 0)
					disconnectPlayer(i, disconnect_kick);
				else if (world.player[i].kickTimer % 10 == 0 && world.player[i].kickTimer <= 50)
					network.plprintf(i, msg_warning, "Disconnecting in %d...", world.player[i].kickTimer / 10);
				continue;
			}
			if (idlekick_time != 0 && !world.player[i].attack && world.player[i].controls.idle() && get_player_count() > 1) {
				++world.player[i].idleFrames;
				int timeToKick = idlekick_time - world.player[i].idleFrames;
				if (timeToKick == 0)
					disconnectPlayer(i, disconnect_idlekick);
				else if ((timeToKick == 60*10 && idlekick_time >= 3*60*10) ||
						 (timeToKick == 30*10 && idlekick_time >= 3*30*10) ||
						 (timeToKick == 15*10 && idlekick_time >= 2*15*10) ||
						 (timeToKick ==  5*10 && idlekick_time >= 2* 5*10)) {
					network.plprintf(i, msg_warning, "*** Idle kick: move or be kicked in %d seconds", timeToKick / 10);
				}
			}
			else
				world.player[i].idleFrames = 0;
		}
	network.broadcast_frame(!gameover);
}

//run something after simulate_and_broadcast
void gameserver_c::server_think_after_broadcast() {
	//check players with pending team changes
	for (int i = 0; i < maxplayers; i++)
		if (world.player[i].used &&
			world.player[i].team_change_pending &&
			world.player[i].want_change_teams &&
			world.player[i].team_change_time < get_time())
				check_player_change_teams(i);
}

void gameserver_c::loop(volatile bool *quitFlag, bool quitOnEsc) {
	nAssert(quitFlag);
	log("at gameserver::loop()");

	world.frame = 0;	//frame to generate next

	//sync with speed counter until it's time to generate one frame (== 1)
	server_speed_counter = 0;
	while (server_speed_counter < 1)
		MS_SLEEP(2);

	while (!*quitFlag) {
		// generate and send frame
		simulate_and_broadcast_frame();

		//dec counter - another 100ms must pass before next send
		server_speed_counter--;

		// next frame
		world.frame++;

		//update wintitle
		if (world.frame % 10 == 0) {
			//update bar
			ostringstream status;
			const int errors = errorLog.size();
			if (errors)
				status << "ERRORS:" << errors << "  ";
			status << network.get_player_count() << '/' << maxplayers << "p ";
			status << setprecision(1) << std::fixed << network.getTraffic() << "k/s v" << GAME_VERSION;
			status << " port:" << extConfig.port;
			if (quitOnEsc)
				status << " ESC:quit";
			extConfig.statusOutput(status.str());
		}

		// executa algo para todos os players
		server_think_after_broadcast();

		// sleep while not time to send again
		while (server_speed_counter <= 0) {
			// sleep a bit
			MS_SLEEP(2);			// *** OPTIMIZE THIS ***
		}

		if (quitOnEsc && key[KEY_ESC])
			break;
	}

	log("exiting gameserver::loop()");
}

//stop server
void gameserver_c::stop() {
	network.stop();
}

GameserverInterface::GameserverInterface(LogSet& hostLog, const ServerExternalSettings& settings) {
	host = new gameserver_c(hostLog, settings);
}

GameserverInterface::~GameserverInterface() {
	delete host;
}

bool GameserverInterface::start(int maxplayers) {
	return host->start(maxplayers);
}

void GameserverInterface::loop(volatile bool *quitFlag, bool quitOnEsc) {
	host->loop(quitFlag, quitOnEsc);
}

void GameserverInterface::stop() {
	host->stop();
}

