#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "incalleg.h"
#include "commont.h"
#include "world.h"
#include "leetnet/sleep.h"	// sleep util
#include "thread.h"
#include "nassert.h"

// implements:
#include "server.h"
#include "gameserver_interface.h"

//#define DEBUG_RANKING
const int minimum_positive_score_for_ranking = 100;

using std::endl;
using std::find;
using std::ifstream;
using std::ios;
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

gameserver_c::gameserver_c(LogSet hostLogs) :
	normalLog("serverlog.txt", true),
	errorLog(normalLog, "ERROR: "),
	securityLog(normalLog, "SECURITY WARNING: ", "server_securitylog.txt", false),
	log(&normalLog, &errorLog, &securityLog),
	world(this, &network, log),
	network(this, world, log),
	authorizations(log)
{
	hostLogs("See serverlog.txt for server's log messages");
	next_vote_announce_frame = 0;
	last_vote_announce_votes = last_vote_announce_needed = 0;
	fav_colors[0].resize(16, false);
	fav_colors[1].resize(16, false);
}

gameserver_c::~gameserver_c() {
	errorMessage("Server had these errors: (see serverlog.txt)", errorLog);
}

void gameserver_c::mutePlayer(int pid, int mode, int admin) {	// 0 = unmute, 1 = normal, 2 = mute silently (do not inform the player)
	if (world.player[pid].muted == mode)
		return;
	const char* adminName = (admin == -1 ? "The admin" : world.player[admin].name.c_str());
	if (mode==0 && world.player[pid].muted!=2)
		network.plprintf(pid, msg_warning, "You have been unmuted (you can send messages again)");
	else if (mode == 1) {
		network.plprintf(pid, msg_warning, "You have been muted (you can't send messages)");
		if (admin != -1)
			network.plprintf(pid, msg_info, "The administrator responsible is %s.", adminName);
	}
	for (int i=0; i<MAX_PLAYERS; ++i)
		if (world.player[i].used && i!=pid) {
			if (mode == 0)
				network.plprintf(i, msg_info, "%s has unmuted %s", adminName, world.player[pid].name.c_str());
			else
				network.plprintf(i, msg_info, "%s has muted %s", adminName, world.player[pid].name.c_str());
		}
	world.player[pid].muted = mode;
}

void gameserver_c::kickPlayer(int pid, int admin, bool ban) {
	const char* adminName = (admin == -1 ? "The admin" : world.player[admin].name.c_str());
	if (ban)
		network.plprintf(pid, msg_warning, "You are now BANNED from this server! Have a nice life...");
	else {
		network.plprintf(pid, msg_warning, "You are being kicked from this server!");
		network.plprintf(pid, msg_warning, "Warning: you can get permanently banned for behaving badly!");
	}
	if (admin != -1)
		network.plprintf(pid, msg_info, "The administrator responsible is %s.", adminName);
	for (int i=0; i<MAX_PLAYERS; ++i)
		if (world.player[i].used && i!=pid)
			network.plprintf(i, msg_info, "%s has %s %s (disconnect in 10 seconds)", adminName, ban?"banned":"kicked", world.player[pid].name.c_str());
	world.player[pid].kickTimer = 10*10;
}

#ifdef SV_NAME_AUTHORIZATION
void gameserver_c::banPlayer(int pid, int admin) {
	authorizations.load();
	authorizations.ban(network.get_client_address(world.player[pid].cid), admin == -1);	// allow hard bans only to 'hard' admin because of the required IP tracking
	authorizations.save();
	kickPlayer(pid, admin, true);
}

bool gameserver_c::check_name_password(const string& name, const string& password) const {
	return authorizations.checkNamePassword(name, password);
}
#endif

void gameserver_c::ctf_game_restart() {
	//submit all pending reports
	for (int i = 0; i < maxplayers; i++)
		if (world.player[i].used)
			network.client_report_status(world.player[i].cid);

	char lix[256];
	sprintf(lix, "CTF GAME OVER - FINAL SCORE: RED %i - BLUE %i", world.teams[0].score(), world.teams[1].score());
	network.broadcast_message(msg_info, lix);

	if (worldConfig.balance_teams)
		balance_teams();

	if (worldConfig.getTimeLimit() == 0)
		sprintf(lix, "CAPTURE %i FLAGS TO WIN THE GAME", worldConfig.getCaptureLimit());
	else if (worldConfig.getCaptureLimit() > 0)
		sprintf(lix, "CAPTURE %i FLAGS TO WIN THE GAME - TIME LIMIT IS %lu MINUTES", worldConfig.getCaptureLimit(), worldConfig.getTimeLimit() / 10 / 60);
	else
		sprintf(lix, "TIME LIMIT IS %lu MINUTES", worldConfig.getTimeLimit() / 10 / 60);
	network.broadcast_message(msg_info, lix);

	network.broadcast_sample(SAMPLE_CTF_GAMEOVER);

	network.sendWorldReset();	// must be before world.reset() because world.reset() already sends initializations
	world.reset();

	network.ctf_update_teamscore(0);
	network.ctf_update_teamscore(1);

	network.send_map_time(-1);
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

//check if team change requests can be satisfied
void gameserver_c::check_team_changes() {
	// check players in random order
	//
	for (int i=0;i<maxplayers;i++) check[i]=0;
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

//check if a player wants to change teams and if yes, try to fullfill the wish
void gameserver_c::check_player_change_teams(int pid) {
	//valid players that want to change teams only
	if (!world.player[pid].used) return;
	if (!world.player[pid].want_change_teams) return;
	if (get_time() < world.player[pid].team_change_time) {
		world.player[pid].team_change_pending = true;	//vai continuar tentando
		return;	//v0.3.3 : intervalos minimos para troca de times
	}

	//count players in each team
	int tc[2];tc[0]=0;tc[1]=0;
	for (int i=0;i<maxplayers;i++)
		if (world.player[i].used)
			tc[i/TSIZE]++;

	//check if team changing happens: calculate delta TARGET TEAM - MY TEAM
	int teamdelta = tc[1-(pid/TSIZE)] - tc[pid/TSIZE];

	//case 0: target team with MORE players: do not move
	if (teamdelta > 0) {
	}
	//case 1: target team with 2 players less:  move player without trades
	else if (teamdelta <= -2) {
		// MOVE W/O TRADE
		for (int i=0;i<maxplayers;i++)
		if (i/TSIZE != pid/TSIZE)			// no time oposto
		{
			if (!world.player[i].used)		// player vago
			{
				move_player(pid, i);	// move pid to free slot
				break;
			}
		}
	}
	//case 2: target team with 0 player less: check for trade, else do nothing
	//case 3: target team with 1 player less: check for trade, else go anyways
	else {
		// FIND A TRADE
		bool found = false;

		for (int i=0;i<maxplayers;i++)
		if (world.player[i].used)	// um player
		if (i/TSIZE != pid/TSIZE)		// do outro time
		if (world.player[i].want_change_teams)		// que quer trocar de time
		{
			found = true;
			swap_players(pid, i);		// make trade
			break;
		}

		// IF TRADE NOT FOUND AND TEAMDELTA == 1, MOVE W/O TRADE
		if ((teamdelta == -1) && (!found)) {

			//comentando isso fora conserta o bug q eh o server entrar em loop

			for (int i=0;i<maxplayers;i++)
			if (i/TSIZE != pid/TSIZE)			// no time oposto
			{
				if (!world.player[i].used)		// player vago
				{
					move_player(pid, i);	// move pid to free slot
					break;
				}
			}
			/*
			for (int i=0;i<MAX_PLAYERS;i++)
			if (!world.player[i].used)		// player vago
			if (i/TSIZE != pid/TSIZE)			// no time oposto
			{
				move_player(pid, i);	// move pid to free slot
				break;
			}
			*/
		}
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
	game_remove_player(f);

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
				raw[p / TSIZE] += DEFAULT_PLAYER_RATE;
			else
				raw[p / TSIZE] += (client[world.player[p].cid].score + 1.0) / (client[world.player[p].cid].neg_score + 1.0);
		}

	//modifiers
	team_smul[0] = raw[1] / raw[0];
	team_smul[1] = raw[0] / raw[1];

	//ceil,floor (1/3 & 3/1)
	for (int i = 0; i < 2; i++) {
		if (team_smul[i] < 0.3333)
			team_smul[i] = 0.3333;
		if (team_smul[i] > 3.0)
			team_smul[i] = 3.0;
	}
}

//score!
void gameserver_c::score_frag(int p, int amount) {
	//add regular frags amount
	world.player[p].frags += amount;

	//v0.4.4 -- add score to the player's score accumulator
	//v0.4.7: DO NOT add score if map is not valid for scoring
	if (world.map.valid_for_scoring)
	if (network.get_player_count() >= 2) { //v0.4.7.1 : skip the scoring if only one player present

		//refresh team ratings
		refresh_team_score_modifiers();

		int cid = world.player[p].cid;

		double parcela = ((double)amount) * team_smul[p/TSIZE];

		//add normalizado
		client[cid].fdp += parcela;

		//refresh "inteiro version"
		client[cid].delta_score = (int)(client[cid].fdp);

		//DEBUGz
		//char lix[256];
		//sprintf(lix, "%s scores +%.4f for %.4f +delta", world.player[p].name.c_str(), parcela, client[cid].fdp);
		//network.broadcast_message(lix);

		//client[cid].delta_score += amount;		//just add the frags for now
	}
}

//score! NEG FRAG (v0.4.8)
void gameserver_c::score_neg(int p, int amount) {

	//add regular frags amount
	//world.player[p].frags += amount;

	//v0.4.4 -- add score to the player's score accumulator
	//v0.4.7: DO NOT add score if map is not valid for scoring
	if (world.map.valid_for_scoring)
	if (network.get_player_count() >= 2) { //v0.4.7.1 : skip the scoring if only one player present

		//refresh team ratings
		refresh_team_score_modifiers();

		int cid = world.player[p].cid;

		double parcela = ((double)amount);		// NAO multiplica....

		//add normalizado
		client[cid].fdn += parcela;

		//refresh "inteiro version"
		client[cid].neg_delta_score = (int)(client[cid].fdn);

		//DEBUGz
		//char lix[256];
		//sprintf(lix, "%s scores -%.4f for %.4f -delta", world.player[p].name.c_str(), parcela, client[cid].fdn);
		//network.broadcast_message(lix);

		//client[cid].neg_delta_score += amount;		//just add the frags for now V0.4.8: NEG SCORE!
	}
}

void gameserver_c::load_game_mod() {
	const int default_capture_limit = worldConfig.getCaptureLimit();

	string filename = wheregamedir + "gamemod.txt";
	ifstream in(filename.c_str());
	if (in) {
		bool command = true;

		log("Loading game mod: '%s'", filename.c_str());

		string line, cmd;
		while (in) {
			getline_smart(in, line);
			if (line.empty() || line[0] == ';')	//skip blank and comment
				continue;
			else if (command)
				cmd = line;
			else {	// parameter
				double val = atof(line.c_str());
				int ival = atoi(line.c_str());

				if (cmd == "friction")
					svp_fric = val;
				else if (cmd == "accel")
					svp_accel = val;
				else if (cmd == "maxspeed")
					svp_maxspeed = val;
				else if (cmd == "friction_run")
					svp_fric_run = val;
				else if (cmd == "accel_run")
					svp_accel_run = val;
				else if (cmd == "maxspeed_run")
					svp_maxspeed_run = val;
				else if (cmd == "friction_turbo")
					svp_fric_turbo = val;
				else if (cmd == "accel_turbo")
					svp_accel_turbo = val;
				else if (cmd == "maxspeed_turbo")
					svp_maxspeed_turbo = val;
				else if (cmd == "friction_turborun")
					svp_fric_turborun = val;
				else if (cmd == "accel_turborun")
					svp_accel_turborun = val;
				else if (cmd == "maxspeed_turborun")
					svp_maxspeed_turborun = val;
				else if (cmd == "flag_penalty")
					svp_flag_penalty = val;
				else if (cmd == "friendly_fire") {
					if (ival == 0 || ival == 1)
						svp_friendly_fire = ival == 1 ? true : false;
					else
						log.error("Can't set %s to %d", cmd.c_str(), ival);
				}
				else if (cmd == "friendly_deathbringer") {
					if (ival == 0 || ival == 1)
						svp_friendly_db = ival == 1 ? true : false;
					else
						log.error("Can't set %s to %d", cmd.c_str(), ival);
				}
				else if (cmd == "map") {
					MapInfo mi;
					if (mi.load(log, line)) {
						maprot.push_back(mi);
						log("Added '%s' to map rotation", line.c_str());
					}
					else {
						log.error("Can't add '%s' to map rotation", line.c_str());
					}
				}
				else if (cmd == "pups_min") {
					if (line.find('%') != string::npos) {
						if (ival >= 0) {
							pupConfig.pups_min = ival;
							pupConfig.pups_min_percentage = true;
						}
						else {
							log.error("Can't set pups_min to %d%%", ival);
						}
					}
					else if (ival >= 0 && ival <= MAX_PICKUPS) {
						pupConfig.pups_min = ival;
						pupConfig.pups_min_percentage = false;
					}
					else {
						log.error("Can't set pups_min to %d", ival);
					}
				}
				else if (cmd == "pups_max") {
					if (line.find('%') != string::npos) {
						if (ival >= 0) {
							pupConfig.pups_max = ival;
							pupConfig.pups_max_percentage = true;
						}
						else {
							log.error("Can't set pups_max to %d%%", ival);
						}
					}
					else if (ival >= 0 && ival <= MAX_PICKUPS) {
						pupConfig.pups_max = ival;
						pupConfig.pups_max_percentage = false;
					}
					else {
						log.error("Can't set pups_max to %d", ival);
					}
				}
				else if (cmd == "pups_respawn_time")
					pupConfig.pups_respawn_time = ival;
				else if (cmd == "pup_chance_shield")
					pupConfig.pup_chance_shield = ival;
				else if (cmd == "pup_chance_turbo")
					pupConfig.pup_chance_turbo = ival;
				else if (cmd == "pup_chance_shadow")
					pupConfig.pup_chance_shadow = ival;
				else if (cmd == "pup_chance_power")
					pupConfig.pup_chance_power = ival;
				else if (cmd == "pup_chance_weapon")
					pupConfig.pup_chance_weapon = ival;
				else if (cmd == "pup_chance_megahealth")
					pupConfig.pup_chance_megahealth = ival;
				else if (cmd == "pup_chance_deathbringer")
					pupConfig.pup_chance_deathbringer = ival;
				else if (cmd == "time_limit") {
					if (ival >= 0)
						worldConfig.time_limit = 60 * 10 * ival; // minutes to frames
					else
						log.error("Can't set %s to %d", cmd.c_str(), ival);
				}
				else if (cmd == "extra_time") {
					if (ival >= 0)
						worldConfig.extra_time = 60 * 10 * ival; // minutes to frames
					else
						log.error("Can't set %s to %d", cmd.c_str(), ival);
				}
				else if (cmd == "sudden_death") {
					if (ival == 0 || ival == 1)
						worldConfig.sudden_death = ival == 1 ? true : false;
					else
						log.error("Can't set %s to %d", cmd.c_str(), ival);
				}
				else if (cmd == "capture_limit") {
					if (ival >= 0)
						worldConfig.capture_limit = ival;
					else
						log.error("Can't set %s to %d", cmd.c_str(), ival);
				}
				else if (cmd == "balance_teams") {
					if (ival == 0 || ival == 1)
						worldConfig.balance_teams = ival == 1 ? true : false;
					else
						log.error("Can't set %s to %d", cmd.c_str(), ival);
				}
				else if (cmd == "welcome_message")
					welcome_message.push_back(line);
				else if (cmd == "info_message")
					info_message.push_back(line);
				else if (cmd == "server_password")
					network.set_server_password(line);
				else if (cmd == "pup_add_time") {
					if (ival > 0 && ival < 1000)
						pupConfig.pup_add_time = ival;
					else
						log.error("Can't set %s to %d", cmd.c_str(), ival);
				}
				else if (cmd == "pup_max_time") {
					if (ival > 0 && ival < 1000)
						pupConfig.pup_max_time = ival;
					else
						log.error("Can't set %s to %d", cmd.c_str(), ival);
				}
				else if (cmd == "pup_deathbringer_switch") {
					if (ival == 0 || ival == 1)
						pupConfig.pup_deathbringer_switch = ival == 1 ? true : false;
					else
						log.error("Can't set %s to %d", cmd.c_str(), ival);
				}
				else if (cmd == "pup_deathbringer_time") {
					if (val >= 1.0)
						pupConfig.pup_deathbringer_time = val;
					else
						log.error("Can't set %s to %f", cmd.c_str(), val);
				}
				else if (cmd == "pups_drop_at_death") {
					if (ival == 0 || ival == 1)
						pupConfig.pups_drop_at_death = ival == 1 ? true : false;
					else
						log.error("Can't set %s to %d", cmd.c_str(), ival);
				}
				else if (cmd == "pup_health_bonus") {
					if (ival >= 0)
						pupConfig.pup_health_bonus = ival;
					else
						log.error("Can't set %s to %d", cmd.c_str(), ival);
				}
				else if (cmd == "pup_power_damage") {
					if (val > 0.)
						pupConfig.pup_power_damage = val;
					else
						log.error("Can't set %s to %f", cmd.c_str(), val);
				}
				else if (cmd == "pup_weapon_max") {
					if (ival >= 1 && ival <= 9)
						pupConfig.pup_weapon_max = ival - 1;
					else
						log.error("Can't set %s to %d", cmd.c_str(), ival);
				}
				else if (cmd == "random_maprot") {
					if (ival == 0 || ival == 1)
						random_maprot = ival == 1 ? true : false;
					else
						log.error("Can't set %s to %d", cmd.c_str(), ival);
				}
				else if (cmd == "vote_block_time") {
					if (ival >= 0)
						vote_block_time = 60 * 10 * ival;	// minutes to frames
					else
						log.error("Can't set %s to %d", cmd.c_str(), ival);
				}
				else if (cmd == "idlekick_time") {
					if (ival >= 10 || ival == 0)
						idlekick_time = ival * 10;	// seconds to frames
					else
						log.error("Can't set %s to %d", cmd.c_str(), ival);
				}
				else if (cmd == "respawn_time") {
					if (val >= 0)
						worldConfig.respawn_time = val;
					else
						log.error("Can't set %s to %d", cmd.c_str(), ival);
				}
				else if (cmd == "waiting_time_deathbringer") {
					if (val >= 0)
						worldConfig.waiting_time_deathbringer = val;
					else
						log.error("Can't set %s to %d", cmd.c_str(), ival);
				}
				else if (cmd == "pup_shadow_invisibility") {
					if (ival == 0 || ival == 1)
						worldConfig.shadow_minimum = ival == 1 ? 0 : worldConfig.shadow_minimum_normal;
					else
						log.error("Can't set %s to %d", cmd.c_str(), ival);
				}
				else if (cmd == "rocket_damage") {
					if (ival >= 0)
						worldConfig.rocket_damage = ival;
					else
						log.error("Can't set %s to %d", cmd.c_str(), ival);
				}
				else if (cmd == "sayadmin_enabled") {
					if (ival == 0 || ival == 1)
						sayadmin_enabled = ival == 1 ? true : false;
					else
						log.error("Can't set %s to %d", cmd.c_str(), ival);
				}
				else if (cmd == "sayadmin_comment")
					sayadmin_comment = line;
				else if (cmd == "server_website")
					server_website_url = line;
				else
					log.error("*** Bad command in gamemod: %s", cmd.c_str());
			}

			// command or parameter
			command = !command;
		}

		log("END OF GAME MOD FILE.");

		// game without capture and time limit is not allowed
		if (worldConfig.getCaptureLimit() == 0 && worldConfig.getTimeLimit() == 0)
			worldConfig.capture_limit = default_capture_limit;

		in.close();
	}
	else
		log.error("Can't open game mod file gamemod.txt!");
	world.setConfig(worldConfig, pupConfig);
}

//load a map from the rotation list
bool gameserver_c::load_rotation_map(int pos) {
	bool ok = world.load_map(SERVER_MAPS_DIR, maprot[pos].file);
	if (!ok)
		return false;
	log("load_rotation_map() maprot[%i] = '%s'", pos, maprot[pos].file.c_str());
	return true;
}

bool gameserver_c::server_next_map(int reason) {
	//(re)load hostname
	network.reload_hostname();

	nAssert(!maprot.empty());

	vector<int> winners;
	int maxVotes=0;
	for (int m=0; m<(int)maprot.size(); ++m) {
		if (maprot[m].votes<maxVotes)
			continue;
		if (maprot[m].votes>maxVotes) {
			maxVotes=maprot[m].votes;
			winners.clear();
		}
		winners.push_back(m);
	}
	if (maxVotes==0)
		currmap=(currmap+1)%maprot.size();
	else {
		if (winners.size()>1) {
			vector<int>::iterator it = find(winners.begin(), winners.end(), currmap);
			if (it != winners.end())
				winners.erase(it);
		}
		currmap=winners[rand()%winners.size()];
	}
	// clear votes for the current map
	for (int p=0; p<maxplayers; ++p) {
		world.player[p].want_map_exit=false;
		if (world.player[p].mapVote==currmap)
			world.player[p].mapVote=-1;
	}
	maprot[currmap].votes = 0;
	maprot[currmap].votes_changed = true;
	last_vote_announce_votes = last_vote_announce_needed = 0;
	next_vote_announce_frame = 0;	// let a new announcement be made as soon as someone votes

	if (!load_rotation_map(currmap))
		return false;

	// notify all players
	for (int i=0;i<maxplayers;i++)
		if (world.player[i].used)
			network.send_map_change_message(i, reason, maprot[currmap].file.c_str());

	//important: server is showing gameover plaque. nobody should move or receive world frames
	gameover = true;
	gameover_time = get_time() + 5.0;		//5 secods timeout for gameover plaque

	char lix[256];
	sprintf(lix, "Server changed map to: %s (%i of %i)", maprot[currmap].title.c_str(), currmap+1, maprot.size());
	network.broadcast_message(msg_info, lix);

	return true;
}

//check map exit by vote
void gameserver_c::check_map_exit() {
	int num_for = 0, num_against = 0;
	for (int i=0; i<maxplayers; i++)
		if (world.player[i].used) {
			if (world.player[i].want_map_exit)
				num_for++;
			else
				num_against++;
		}

	// this could be done elsewhere, but this function is called whenever votes change
	for (int m=0; m<(int)maprot.size(); ++m)
		maprot[m].votes=0;
	for (int p=0; p<maxplayers; ++p)
		if (world.player[p].used && world.player[p].mapVote!=-1)
			++maprot[world.player[p].mapVote].votes;

	if ((world.getMapTime()>=vote_block_time && num_for>num_against) || (num_against==0 && num_for)) {
		server_next_map(NEXTMAP_VOTE_EXIT);	// ignore return value
		ctf_game_restart();
	}
}

//----- THE REST  ----------------

bool gameserver_c::reset_settings(bool keepMap) {
	string currMapFile;
	if (keepMap)
		currMapFile = maprot[currmap].file;

	set_default_physics();

	pupConfig.reset();
	worldConfig.reset();

	vote_block_time = 0;	// no limit
	idlekick_time = 0;		// no limit

	random_maprot = false;
	// reset server rotation list
	currmap = 0;

	sayadmin_comment.clear();
	sayadmin_enabled = false;

	welcome_message.clear();
	info_message.clear();

	server_website_url.clear();

	maprot.clear();

	// load server configuration from gamemod.txt
	load_game_mod();

	// did not specify maps, scan "maps/" folder for .txt map files
	if (maprot.empty()) {
		string searchPattern = wheregamedir + SERVER_MAPS_DIR + directory_separator + "*.txt";

		log("Scanning for maps: '%s'", searchPattern.c_str());

		al_ffblk mapffblk;	//for al_find*

		int result = al_findfirst(searchPattern.c_str(), &mapffblk, FA_ARCH|FA_RDONLY);
		while (result == 0) {
			char nameBuf[500];
			//char *replace_extension(char *dest, const char *filename, const char *ext, int size
			replace_extension(nameBuf, mapffblk.name, "", 500);
			nameBuf[strlen(nameBuf)-1] = 0;	//take last damn '.' out

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
		for (mapi=0; mapi<maprot.size(); ++mapi)
			if (maprot[mapi].file == currMapFile)
				break;
		if (mapi == maprot.size()) {	// not found
			currmap = -1;
			server_next_map(NEXTMAP_VOTE_EXIT);
		}
	}
	return true;
}

//start server
bool gameserver_c::start(int target_maxplayers) {
	#ifdef SV_NAME_AUTHORIZATION
	authorizations.load();

	// read the admins file
	ifstream is("admins.txt");
	if (is) {
		for (;;) {
			string line;
			getline_smart(is, line);
			if (!is)
				break;
			admins.push_back(line);
		}
		log("Loaded %u administrator names", admins.size());
	}
	else
		log("Couldn't load administrator names: no admins.txt");
	#endif

	//check if maxplayers is valid
	if (target_maxplayers < 2)				//menos de dois
		return false;
	if (target_maxplayers > MAX_PLAYERS)		//mais que o maximo
		return false;
	if (target_maxplayers % 2 == 1)	//numero impar de jogadores
		return false;

	//set maxplayers
	maxplayers = target_maxplayers;

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

	//default serverinfo with reload hostname
	network.reload_hostname();

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

void gameserver_c::game_remove_player(int pid) {
	fav_colors[pid / TSIZE][world.player[pid].color()] = false;
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

	//need to broadcast the player's crap to remove eventual '*' and stuff
	network.broadcast_player_crap( pid );

	//check if it's the first name information from client. then it
	// must have just entered the game
	bool entered_game = world.player[pid].name.empty();

	// Name with only whitespaces not allowed.
	if (tempname.find_first_not_of("  \t") == string::npos)
		disconnectPlayer(pid, disconnect_client_misbehavior);
	else {
		#ifdef SV_NAME_AUTHORIZATION
		int nid = authorizations.identifyName(tempname);
		if (nid == -1 || authorizations.authorize(nid, password)) {
			world.player[pid].name = tempname;
			world.player[pid].waitnametime = get_time() + 1.0;
		}
		else if (entered_game)
			disconnectPlayer(pid, disconnect_client_misbehavior);
		else
			network.sendNameAuthorizationRequest(pid);
		#else
		world.player[pid].name = tempname;
		world.player[pid].waitnametime = get_time() + 1.0;
		#endif
	}

	//send entered-game message
	if (entered_game)
		network.newPlayer(pid);

	//broadcast the new player's name
	network.broadcast_player_name(pid);
}

class PlayerMessager : public LineReceiver {
	gameserver_c& host;
	int player;
	Message_type type;

public:
	PlayerMessager(gameserver_c& server, int pid, Message_type mtype) : host(server), player(pid), type(mtype) { }
	PlayerMessager& operator()(const std::string& str) { host.sendMessage(player, type, str); return *this; }
};

void gameserver_c::chat(int pid, const char* sbuf) {
	// handle 'console' commands
	if (sbuf[0]=='/') {
		bool admin = false;
		if (world.player[pid].reg_status == '*' || authorizations.identifyName(world.player[pid].name) != -1) {
			// the player surely is who his name implies, authorized either by the tournament master or the local authorization database
			if (find(admins.begin(), admins.end(), world.player[pid].name) != admins.end())
				admin = true;
		}

		const char* pCommand=sbuf+1;
		char cbuf[30];
		int ci;
		for (ci=0;; ++ci, ++pCommand) {
			if (ci==29) {
				cbuf[29]='\0';
				break;
			}
			if (*pCommand==' ') {
				cbuf[ci]='\0';
				++pCommand;
				break;
			}
			cbuf[ci]=*pCommand;
			if (*pCommand=='\0')
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
				network.player_message(pid, msg_normal, "/ban n      ban player with ID n");
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
			pupConfig.print(pm);
		}
		else if (!strcmp(cbuf, "sayadmin") && sayadmin_enabled) {
			if (strspnp(pCommand, " ")!=NULL) {
				ofstream log("sayadmin.log", ios::out | ios::app);
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
			network.player_message(pid, msg_header, "Players on server: ID, name");
			for (int ppid = 0; ppid < MAX_PLAYERS; ) {
				char buf[100];
				int bufi = 0;
				for (int onrow = 0; onrow < 3 && ppid < MAX_PLAYERS; ++ppid)
					if (world.player[ppid].used) {
						sprintf(buf+bufi, "%2d %c%-22s", ppid, world.player[ppid].reg_status, world.player[ppid].name.c_str());
						bufi+=26;
						++onrow;
					}
				if (bufi > 0)
					network.player_message(pid, msg_normal, buf);
			}
		}
		else if ((!strcmp(cbuf, "kick") || !strcmp(cbuf, "ban") || !strcmp(cbuf, "mute")
					|| !strcmp(cbuf, "smute") || !strcmp(cbuf, "unmute")) && admin) {
			int ppid = atoi(pCommand);
			if (*pCommand != '\0' && ppid >= 0 && ppid < MAX_PLAYERS && world.player[ppid].used && pCommand[strspn(pCommand, "0123456789")]=='\0') {
				if (!strcmp(cbuf, "kick"))
					kickPlayer(ppid, pid);
				else if (!strcmp(cbuf, "ban"))
					banPlayer(ppid, pid);
				else if (!strcmp(cbuf, "mute"))
					mutePlayer(ppid, 1, pid);
				else if (!strcmp(cbuf, "smute"))
					mutePlayer(ppid, 2, pid);
				else if (!strcmp(cbuf, "unmute"))
					mutePlayer(ppid, 0, pid);
				else
					nAssert(0);
			}
			else
				network.player_message(pid, msg_warning, "No such player. Type /list for a list of IDs.");
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
	else if (strspnp(sbuf, " ")!=NULL) {	// ignore messages that are all spaces
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
		else if (world.player[pid].muted==1)
			network.plprintf(pid, msg_warning, "You are muted. You can't send messages.");
		else {
			char talkmsg[256];
			// check for team message:
			if (sbuf[0] == '.') {
				sprintf(talkmsg, "%s: %s", world.player[pid].name.c_str(), sbuf+1);
				if (world.player[pid].muted==2)
					network.player_message(pid, msg_team, talkmsg);
				else
					network.broadcast_team_message(pid/TSIZE, talkmsg);
			}
			//regular msg
			else {
				sprintf(talkmsg, "%s: %s", world.player[pid].name.c_str(), sbuf);
				if (world.player[pid].muted==2)
					network.player_message(pid, msg_normal, talkmsg);
				else
					network.broadcast_message(msg_normal, talkmsg);
			}
		}
	}
}

bool gameserver_c::changeRegistration(int id, const string& token) {
	int intoken = atoi(token.c_str()); //atoi it
	if (intoken == client[id].intoken) {
		#ifdef DEBUG_RANKING
		network.bprintf("%s %i %i token identico.", world.player[ctop[id]].name.c_str(), client[id].intoken, intoken);
		#endif
		return false;
	}
	#ifdef DEBUG_RANKING
	//debug message
	if ((client[id].token_have) && (client[id].token_valid))
		network.bprintf("%s %i %i changes registration, submitting results...", world.player[ctop[id]].name.c_str(), client[id].intoken, intoken);
	else if ((client[id].token_have) && (!client[id].token_valid))
		network.bprintf("%s %i %i changes registration ('?')", world.player[ctop[id]].name.c_str(), client[id].intoken, intoken);
	else if (!client[id].token_have)
		network.bprintf("%s %i %i sends registration.", world.player[ctop[id]].name.c_str(), client[id].intoken, intoken);
	#endif

	// v0.4.9 FIX : IF HAD previous token have/valid, then FLUSH his stats
	network.client_report_status(id);

	strcpy(client[id].token, token.c_str());
	client[id].intoken = intoken;

	// NEW (or first) REGISTRATION -- reset player report / stop reporting his old ID
	client[id].neg_delta_score = 0;
	client[id].delta_score = 0; //V0.4.8
	client[id].fdp = 0.0; //V0.4.8
	client[id].fdn = 0.0; //V0.4.8
	client[id].score = 0;
	client[id].neg_score = 0;		//V0.4.8
	client[id].rank = 0;

	client[id].token_have = true;			//token set
	client[id].token_valid = false;		//BUT not validated yet
	return true;
}

void gameserver_c::simulate_and_broadcast_frame() {
	//check end of gameover plaque
	if (gameover)
		if (gameover_time < get_time()) {
			gameover = false;
			network.sendEndGameover();
		}
	if (!gameover)
		world.simulateFrame();

	if (world.frame >= next_vote_announce_frame) {	// announce voting status
		int votes = 0;
		for (int i = 0; i < maxplayers; ++i)
			if (world.player[i].used && world.player[i].want_map_exit)
				++votes;
		int players = get_player_count() / 2 + 1;
		if (votes != last_vote_announce_votes || (players != last_vote_announce_needed && votes != 0)) {
			last_vote_announce_votes = votes;
			last_vote_announce_needed = players;
			next_vote_announce_frame = world.frame + SV_VOTE_ANNOUNCE_INTERVAL * 10;
			ostringstream voteinfo;
			voteinfo << "*** " << votes << '/' << players << " votes for mapchange";
			if (world.getMapTime() < vote_block_time)
				voteinfo << " (all players needed for " << (vote_block_time-world.getMapTime()+5)/10 << " more seconds)";
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

//loop server
// running_flag: pointer to bool, if this bool goes to false, the loop quits.
void gameserver_c::loop(volatile bool *running_flag) {

	log("GAMESERVER::LOOP()");

	world.frame = 0;	//frame to generate next

	//sync with speed counter until it's time to generate one frame (== 1)
	server_speed_counter = -3;
	while (server_speed_counter < 1)
		MS_SLEEP(1);		// *** NO CPU PROBLEM HERE ***

	// no flag specified: esc quits
	bool keep_running = true;
	if (!running_flag)
		running_flag = &keep_running;

	log("GAMESERVER::LOOP() (2)");

	while (*running_flag && !force_exit) {
		// generate and send frame
		simulate_and_broadcast_frame();

		//dec counter - another 100ms must pass before next send
		server_speed_counter--;

		// next frame
		world.frame++;

		//update dedserver wintitle
		if (world.frame % 10 == 0) {
			//update bar
			ostringstream status;
			status << network.get_player_count() << '/' << maxplayers << "p ";
			status << setprecision(1) << std::fixed << network.getTraffic() << "k/s v" << GAME_VERSION;
			status << " port:" << port;
			if (dedserver)
				status << " ESC:quit";
			server_status_string(status.str());
		}

		// executa algo para todos os players
		server_think_after_broadcast();

		// sleep while not time to send again
		while (server_speed_counter <= 0) {
			// sleep a bit
			MS_SLEEP(2);			// *** OPTIMIZE THIS ***
		}

		// quit? if no running-flag specified
		if (key[KEY_ESC])
			keep_running = false;
	}

	log("GAMESERVER::LOOP() (EXITING!)");
}

//stop server
void gameserver_c::stop() {
	network.stop();
}

void gameserver_c::clearWorldRankingDeltas() {
	for (int p=0;p<MAX_PLAYERS;p++) {
		if (!world.player[p].used)
			continue;
		int cid = world.player[p].cid;
		client[cid].delta_score = 0;
		client[cid].neg_delta_score = 0;		//V0.4.8
		client[cid].fdp = 0.0;
		client[cid].fdn = 0.0;		//V0.4.8
	}
}

GameserverInterface::GameserverInterface(LogSet hostLog) {
	host = new gameserver_c(hostLog);
}

GameserverInterface::~GameserverInterface() {
	delete host;
}

bool GameserverInterface::start(int maxplayers) {
	return host->start(maxplayers);
}

void GameserverInterface::loop(volatile bool* runFlag) {
	host->loop(runFlag);
}

void GameserverInterface::stop() {
	host->stop();
}

