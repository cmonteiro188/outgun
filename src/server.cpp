#include "commont.h"
#include "world.h"
#include "leetnet/sleep.h"	// sleep util
#include "server.h"
#include "nassert.h"

//#define DEBUG_RANKING
#define MINIMUM_POSITIVE_SCORE_FOR_RANKING 100

using std::find;
using std::ifstream;
using std::max;
using std::ostringstream;
using std::random_shuffle;
using std::setfill;
using std::setprecision;
using std::setw;
using std::string;
using std::swap;
using std::vector;

gameserver_c::gameserver_c() : world(this, &network), network(this, world) {
	next_vote_announce_frame = 0;
	last_vote_announce_votes = last_vote_announce_needed = 0;
}

gameserver_c::~gameserver_c() {
}

void gameserver_c::mutePlayer(int pid, int mode, int admin) {    // 0 = unmute, 1 = normal, 2 = mute silently (do not inform the player)
	const char* adminName = (admin == -1 ? "The admin" : world.player[admin].name.c_str());
	if (mode==0 && world.player[pid].muted!=2)
		network.plprintf(pid, "@WYou have been unmuted (you can send messages again)");
	else if (mode == 1) {
		network.plprintf(pid, "@WYou have been muted (you can't send messages)");
		if (admin != -1)
			network.plprintf(pid, "@IThe administrator responsible is %s.", adminName);
	}
	for (int i=0; i<MAX_PLAYERS; ++i)
		if (world.player[i].used && i!=pid) {
			if (mode == 0)
				network.plprintf(i, "@I%s has unmuted %s", adminName, world.player[pid].name.c_str());
			else
				network.plprintf(i, "@I%s has muted %s", adminName, world.player[pid].name.c_str());
		}
	world.player[pid].muted = mode;
}

void gameserver_c::kickPlayer(int pid, int admin, bool ban) {
	const char* adminName = (admin == -1 ? "The admin" : world.player[admin].name.c_str());
	world.player[pid].delayedMessages.clear();
	if (ban)
		network.plprintf(pid, "@WYou are now BANNED from this server! Have a nice life...");
	else {
		network.plprintf(pid, "@WYou are being kicked from this server!");
		network.plprintf(pid, "@WWarning: you can get permanently banned for behaving badly!");
	}
	if (admin != -1)
		network.plprintf(pid, "@IThe administrator responsible is %s.", adminName);
	for (int i=0; i<MAX_PLAYERS; ++i)
		if (world.player[i].used && i!=pid)
			network.plprintf(i, "@I%s has %s %s (disconnect in 10 seconds)", adminName, ban?"banned":"kicked", world.player[pid].name.c_str());
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
	int i;

	//submit all pending reports
	for (i=0;i<maxplayers;i++)
		if (world.player[i].used)
			network.client_report_status(world.player[i].cid);

	char lix[256];
	sprintf(lix, "@ICTF GAME RESTARTED - FINAL SCORE:   %i RED x %i BLUE !", world.teams[0].score(), world.teams[1].score());
	network.broadcast_message(lix);

	if (worldConfig.getTimeLimit() == 0)
		sprintf(lix, "@ICAPTURE %i FLAGS TO WIN THE GAME", worldConfig.getCaptureLimit());
	else
		sprintf(lix, "@ICAPTURE %i FLAGS TO WIN THE GAME - TIME LIMIT IS %lu MINUTES", worldConfig.getCaptureLimit(), worldConfig.getTimeLimit() / 10 / 60);
	network.broadcast_message(lix);

	network.broadcast_sample(SAMPLE_CTF_GAMEOVER);

	network.sendWorldReset();	// must be before world.reset() because world.reset() already sends initializations
	world.reset();

	network.ctf_update_teamscore(0);
	network.ctf_update_teamscore(1);

	network.send_map_time(-1);
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
//
void gameserver_c::move_player(int f, int t) {

	//broadcast sound
	network.broadcast_sample(SAMPLE_CHANGETEAM);

	//UGLY HACK
	if (!check[t]) {
		check[t] = 1;
		checount--;
	}

	world.dropFlagIfAny(f);

	//copy to t
	world.player[t] = world.player[f];

	//change rockets owner from f to t
	world.changeRocketsOwner(f, t);

	//remove f
	game_remove_player(f);

	world.player[t].id = t;

	//I really dont want to change teams no more..
	world.player[t].want_change_teams = false;
	world.player[t].team_change_time = get_time() + 10.0;		//10 secs interval

	//kill t
	if (world.player[t].health > 0)
		world.resetPlayer(t);

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

	swap(world.player[a], world.player[b]);
	world.swapRocketOwners(a, b);
	world.player[a].id = a;
	world.player[b].id = b;

	//either don't want to change teams anymore
	world.player[a].want_change_teams = false;
	world.player[a].team_change_time = get_time() + 10.0;		//10 secs interval
	world.player[b].want_change_teams = false;
	world.player[b].team_change_time = get_time() + 10.0;		//10 secs interval

	//send updates
	network.move_update_player(a);
	network.move_update_player(b);
}

//refresh team ratings
void gameserver_c::refresh_team_score_modifiers() {

	double raw[2];
	raw[0]=0.0;
	raw[1]=0.0;

	//somatorio raw ratings
	for (int p=0;p<maxplayers;p++)
		if (world.player[p].used) {
			// use "1.0" rating for anybody with less than 100 positive points
			if (client[world.player[p].cid].score < MINIMUM_POSITIVE_SCORE_FOR_RANKING)
				raw[p/TSIZE] += DEFAULT_PLAYER_RATE;
			else
				raw[p/TSIZE] += ( ((double)client[world.player[p].cid].score) + 1.0) / ( ((double)client[world.player[p].cid].neg_score) + 1.0);
		}

	//modifiers
	team_smul[0] = raw[1] / raw[0];
	team_smul[1] = raw[0] / raw[1];

	//ceil,floor (1/3 & 3/1)
	for (int i=0;i<2;i++) {
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
	char filename[WHERE_PATH_SIZE];
	append_filename(filename, wheregamedir, "gamemod.txt", WHERE_PATH_SIZE);
	ifstream in(filename);
	if (in) {
		bool command = true;

		LOG1("Loading game mod from '%s'...\n", filename);

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
						LOG1("Can't set friendly_fire to %d\n", ival);
				}
				else if (cmd == "friendly_deathbringer") {
					if (ival == 0 || ival == 1)
						svp_friendly_db = ival == 1 ? true : false;
					else
						LOG1("Can't set friendly_deathbringer to %d\n", ival);
				}
				else if (cmd == "map") {
					MapInfo mi;
					if (mi.load(line)) {
						maprot.push_back(mi);
						LOG1("Added '%s' to map rotation\n", line.c_str());
					}
					else {
						LOG1("Can't add '%s' to map rotation\n", line.c_str());
					}
				}
				else if (cmd == "pups_min") {
					if (line.find('%') != string::npos) {
						if (ival >= 0) {
							pupConfig.pups_min = ival;
							pupConfig.pups_min_percentage = true;
						}
						else {
							LOG1("Can't set pups_min to %d%%\n", ival);
						}
					}
					else if (ival >= 0 && ival <= MAX_PICKUPS) {
						pupConfig.pups_min = ival;
						pupConfig.pups_min_percentage = false;
					}
					else {
						LOG1("Can't set pups_min to %d\n", ival);
					}
				}
				else if (cmd == "pups_max") {
					if (line.find('%') != string::npos) {
						if (ival >= 0) {
							pupConfig.pups_max = ival;
							pupConfig.pups_max_percentage = true;
						}
						else {
							LOG1("Can't set pups_max to %d%%\n", ival);
						}
					}
					else if (ival >= 0 && ival <= MAX_PICKUPS) {
						pupConfig.pups_max = ival;
						pupConfig.pups_max_percentage = false;
					}
					else {
						LOG1("Can't set pups_max to %d\n", ival);
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
				}
				else if (cmd == "capture_limit") {
					if (ival > 0)
						worldConfig.capture_limit = ival;
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
						LOG1("Can't set pup_add_time to %d\n", ival);
				}
				else if (cmd == "pup_max_time") {
					if (ival > 0 && ival < 1000)
						pupConfig.pup_max_time = ival;
					else
						LOG1("Can't set pup_max_time to %d\n", ival);
				}
				else if (cmd == "pup_deathbringer_switch") {
					if (ival == 0 || ival == 1)
						pupConfig.pup_deathbringer_switch = ival == 1 ? true : false;
					else
						LOG1("Can't set pup_deathbringer_switch to %d\n", ival);
				}
				else if (cmd == "pups_drop_at_death") {
					if (ival == 0 || ival == 1)
						pupConfig.pups_drop_at_death = ival == 1 ? true : false;
					else
						LOG1("Can't set pups_drop_at_death to %d\n", ival);
				}
				else if (cmd == "random_maprot") {
					if (ival == 0 || ival == 1)
						random_maprot = ival == 1 ? true : false;
					else
						LOG1("Can't set random_maprot to %d\n", ival);
				}
				else if (cmd == "vote_block_time") {
					if (ival >= 0)
						vote_block_time = 60 * 10 * ival;	// minutes to frames
				}
				else if (cmd == "idlekick_time") {
					if (ival >= 10 || ival == 0)
						idlekick_time = ival * 10;	// seconds to frames
				}
				else if (cmd == "respawn_time") {
					if (val >= 0)
						worldConfig.respawn_time = val;
				}
				else if (cmd == "waiting_time_deathbringer") {
					if (val >= 0)
						worldConfig.waiting_time_deathbringer = val;
				}
				else if (cmd == "pup_shadow_invisibility") {
					if (ival == 0 || ival == 1)
						worldConfig.shadow_minimum = ival == 1 ? 0 : SV_SHADOW_MINIMUM_NORMAL;
					else
						LOG1("Can't set pup_shadow_invisibility to %d\n", ival);
				}
				else if (cmd == "sayadmin_enabled") {
					if (ival == 0 || ival == 1)
						sayadmin_enabled = ival == 1 ? true : false;
					else
						LOG1("Can't set sayadmin_enabled to %d\n", ival);
				}
				else if (cmd == "sayadmin_comment")
					sayadmin_comment = line;
				/*else if (cmd == "idle_kick_time")
					idle_kick_time = 60 * 10 * val;		// minutes to frames*/
				else
					LOG1("*** Bad command in gamemod: %s\n", cmd.c_str());
			}

			// command or parameter
			command = !command;
		}

		LOG("END OF GAME MOD FILE.\n");

		in.close();
	}
	else
		LOG("Can't open game mod file gamemod.txt!\n");
	world.setConfig(worldConfig, pupConfig);
}

//load a map from the rotation list
bool gameserver_c::load_rotation_map(int pos) {
	bool ok = world.load_map(SERVER_MAPS_DIR, maprot[pos].file);
	if (!ok)
		return false;
	LOG2("load_rotation_map() maprot[%i] = '%s'\n", pos, maprot[pos].file.c_str());
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
	network.broadcast_message(lix);

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
	idlekick_time = 0;	// no limit

	random_maprot = false;
	// reset server rotation list
	currmap = 0;

	sayadmin_comment = string();
	sayadmin_enabled = false;

	welcome_message.clear();
	info_message.clear();

	maprot.clear();

	// load server configuration from gamemod.txt
	load_game_mod();

	// did not specify maps, scan "maps/" folder for .txt map files
	if (maprot.empty()) {
		char mappath[256];
		strcpy(mappath, SERVER_MAPS_DIR);  // maps
		put_backslash(mappath);					// maps/
		strcat(mappath, "*.txt");				// maps/*.txt
		char nameBuf[512];
		char dest[1024];
		append_filename(dest, wheregamedir, mappath, WHERE_PATH_SIZE);	// <FULL-DIR>/maps/*.txt, I hope

		LOG1("MAP SCAN DIR IS = '%s'\n", dest);

		al_ffblk mapffblk;	//for al_find*

		int result = al_findfirst(dest, &mapffblk, FA_ARCH|FA_RDONLY);
		while (result == 0) {
			//char *replace_extension(char *dest, const char *filename, const char *ext, int size
			replace_extension(nameBuf, mapffblk.name, "", 500);
			nameBuf[strlen(nameBuf)-1] = 0;	//take last damn '.' out

			MapInfo mi;
			if (mi.load(nameBuf)) {
				maprot.push_back(mi);
				LOG1("Added '%s' to map rotation\n", nameBuf);
			}
			else
				LOG1("Can't add '%s' to map rotation\n", nameBuf);

			//try next
			result = al_findnext(&mapffblk);
		}
	}

	if (maprot.empty()) {
		LOG("No maps for rotation\n");
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
		LOG1("Loaded %u administrator names\n", admins.size());
	}
	else
		LOG("Couldn't load administrator names: no admins.txt\n");
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
	if (!network.start())
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
		return rand()%2;
}

void gameserver_c::game_remove_player(int pid) {
	client[world.player[pid].cid].reset();
	network.removePlayer(pid);
	world.removePlayer(pid);
}

void gameserver_c::nameChange(int id, int pid, const string& tempname) {
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

	world.player[pid].name = "(invalid name)";
	if (tempname.find_first_not_of(' ') == string::npos)
		world.player[pid].add_to_queue("@WPlease enter a name");
	else {
		#ifdef SV_NAME_AUTHORIZATION
		int nid=authorizations.identifyName(tempname);
		if (nid==-1 || authorizations.authorize(nid, network.get_client_address(id)))
			world.player[pid].name = tempname;
		else {
			world.player[pid].queue_printf("@WThe name %s is reserved on this server.", authorizations.getName(nid).c_str());
			world.player[pid].queue_printf("@ITo authorize, type /auth %s,pass where pass is your password.", authorizations.getName(nid).c_str());
			world.player[pid].queue_printf("@IUse /authadd instead if you want to keep your previous addresses authorized.");
		}
		#else
		world.player[pid].name = tempname;
		#endif
	}

	// next time allowed to change name
	world.player[pid].waitnametime = get_time() + 1.0;

	//send entered-game message
	if (entered_game)
		network.newPlayer(pid);

	//broadcast the new player's name
	network.broadcast_player_name(pid);
}

void gameserver_c::chat(int id, int pid, const char* sbuf) {
	// handle 'console' commands
	if (world.player[pid].delayedMessages.size()>2) {
		world.player[pid].delayedMessages.clear();
		network.plprintf(pid, "@I(rest of message cancelled)");
	}
	world.player[pid].reset_message_queue_timing();
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
			world.player[pid].queue_printf("@TConsole commands available on this server:");
			world.player[pid].queue_printf("/help       this screen");
			if (!info_message.empty())
				world.player[pid].queue_printf("/info       information about this server");
			world.player[pid].queue_printf("/config     current server configuration");
			world.player[pid].queue_printf("/stats      see your stats");
			world.player[pid].queue_printf("/mapinfo n  information about map n (default: current map)");
			world.player[pid].queue_printf("/time       check server uptime, current map time and time left on the map");
			if (sayadmin_enabled) {
				ostringstream ostr;
				ostr << "/sayadmin   send a message to the server admin";
				if (sayadmin_comment.length())
					ostr << " (" << sayadmin_comment << ')';
				world.player[pid].add_to_queue(ostr.str());
			}
			if (admin) {
				world.player[pid].queue_printf("@TAdmin commands:");
				world.player[pid].queue_printf("/list       get a list of player ID's");
				world.player[pid].queue_printf("/kick n     kick player with ID n");
				world.player[pid].queue_printf("/ban n      ban player with ID n");
				world.player[pid].queue_printf("/mute n     mute player with ID n");
				world.player[pid].queue_printf("/smute n    silently mute player with ID n (crude!)");
				world.player[pid].queue_printf("/unmute n   cancel muting of player with ID n");
				world.player[pid].queue_printf("/forcemap   restart the game and change map if you've used votemap");
			}
		}
		else if (!strcmp(cbuf, "info") && !info_message.empty()) {
			for (vector<string>::const_iterator line=info_message.begin(); line!=info_message.end(); line++)
				world.player[pid].add_to_queue(*line);
			world.player[pid].add_to_queue("type /config to see current server settings");
		}
		else if (!strcmp(cbuf, "config")) {
			world.player[pid].queue_printf("@TCurrent server settings:");
			PlayerQueueAdder pqa(world.player[pid]);
			worldConfig.print(pqa);
			pupConfig.print(pqa);
		}
		else if (!strcmp(cbuf, "sayadmin") && sayadmin_enabled) {
			if (strspnp(pCommand, " ")!=NULL) {
				FILE* logp=fopen("sayadmin.log", "at+");
				time_t tt=time(0);
				struct tm* tmb=localtime(&tt);
				fprintf(logp, "%d-%02d-%02d %02d:%02d:%02d  %s: %s\n", tmb->tm_year+1900, tmb->tm_mon+1, tmb->tm_mday,
						tmb->tm_hour, tmb->tm_min, tmb->tm_sec, world.player[pid].name.c_str(), pCommand);
				fclose(logp);
				network.forwardSayadminMessage(world.player[pid].cid, pCommand);
				world.player[pid].queue_printf("@IYour message has been logged. Thank you for your feedback!");
			}
			else
				world.player[pid].queue_printf("@IFor example to send \"Hello!\", type /sayadmin Hello!");
		}
		else if (!strcmp(cbuf, "map") || !strcmp(cbuf, "mapinfo")) {
			if (*pCommand!='\0') {
				int mid=atoi(pCommand)-1;
				if (mid>=0 && mid<(int)maprot.size() && pCommand[strspn(pCommand, "0123456789")]=='\0') {
					world.player[pid].queue_printf("@IMap %d is %s (%s)", mid+1, maprot[mid].title.c_str(), maprot[mid].author.c_str());
					world.player[pid].queue_printf("@I%s.txt, size %d×%d", maprot[mid].file.c_str(), maprot[mid].width, maprot[mid].height);
				}
				else
					world.player[pid].queue_printf("@WValid map id's are 1 to %d", maprot.size());
			}
			else {
				world.player[pid].queue_printf("@IThis map is %s (%s)", maprot[currmap].title.c_str(), maprot[currmap].author.c_str());
				world.player[pid].queue_printf("@I%s.txt, size %d×%d", maprot[currmap].file.c_str(), maprot[currmap].width, maprot[currmap].height);
			}
		}
		else if (!strcmp(cbuf, "time")) {
			PlayerQueueAdder pqa(world.player[pid]);
			world.printTimeStatus(pqa);
		}
		else if (!strcmp(cbuf, "stats")) {
			int playing_time = (int)get_time() - world.player[pid].start_time;  // seconds
			int lifetime = world.player[pid].lifetime;
			if (!world.player[pid].dead)
				lifetime += (int)get_time() - world.player[pid].last_spawn_time;
			world.player[pid].queue_printf("@TYour stats: %d captures, %d kills, %d deaths, %d suicides",
				world.player[pid].total_captures,
				world.player[pid].total_kills,
				world.player[pid].total_deaths,
				world.player[pid].total_suicides);
			world.player[pid].queue_printf(" Enemy flags: %d taken, %d dropped",
				world.player[pid].total_flags_taken,
				world.player[pid].total_flags_dropped);
			world.player[pid].queue_printf(" Own flags: %d returned, %d carriers killed",
				world.player[pid].total_flags_returned,
				world.player[pid].total_flag_carriers_killed);
			world.player[pid].queue_printf(" Consecutive kills: %d (%d), deaths: %d (%d)",
				world.player[pid].most_consecutive_kills,
				world.player[pid].current_consecutive_kills,
				world.player[pid].most_consecutive_deaths,
				world.player[pid].current_consecutive_deaths);
			int accuracy = 0;
			if (world.player[pid].total_shots > 0)
				accuracy = int((100. * world.player[pid].total_hits) / world.player[pid].total_shots + 0.5);
			world.player[pid].queue_printf(" Shots: %d shot, accuracy %d%%, %d taken",
				world.player[pid].total_shots,
				accuracy,
				world.player[pid].total_shots_taken);
			world.player[pid].queue_printf(" Distance travelled: %.0lf units, average speed %.2lf units/s.",
				world.player[pid].total_movement/(PLAYER_RADIUS*2.),	// make the unit player diameter
				world.player[pid].total_movement/(PLAYER_RADIUS*2.)/double(lifetime));
			int av_lifetime = lifetime / (world.player[pid].total_deaths + 1);
			world.player[pid].queue_printf(" You have played %d min. Total lifetime %d:%02d. Average lifetime %d:%02d.",
				playing_time / 60,
				lifetime / 60,
				lifetime % 60,
				av_lifetime / 60,
				av_lifetime % 60);
			// Add more stats: flag carrying time, etc.
		}
		#ifdef SV_NAME_AUTHORIZATION
		else if (!strcmp(cbuf, "auth") || !strcmp(cbuf, "authadd")) {
			string nameUpr;
			for (; *pCommand && *pCommand!=','; ++pCommand)
				nameUpr+=toupper(*pCommand);
			if (*pCommand==',') {
				string pwd(pCommand+1);
				authorizations.load();
				if (strcmp(cbuf, "authadd"))
					authorizations.clearIPs(nameUpr, pwd);
				if (authorizations.addIP(nameUpr, pwd, network.get_client_address(id))) {
					authorizations.save();
					ostringstream line;
					line << "@WOK: authorized your IP address ";
					if (!strcmp(cbuf, "authadd"))
						line << "also ";
					line << "to use " << nameUpr;
					world.player[pid].add_to_queue(line.str());
					world.player[pid].add_to_queue("@WYou may change your name now");
				}
				else
					world.player[pid].add_to_queue("@WAuthorization failed");
			}
			else
				world.player[pid].add_to_queue("@WInvalid auth command");
		}
		#endif
		else if (!strcmp(cbuf, "list") && admin) {
			world.player[pid].queue_printf("@TPlayers on server: ID, name");
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
					world.player[pid].add_to_queue(buf);
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
				world.player[pid].queue_printf("@WNo such player. Type /list for a list of IDs.");
		}
		else if (!strcmp(cbuf, "forcemap") && admin) {
			if (world.player[pid].mapVote != -1 && world.player[pid].mapVote != currmap) {
				network.bprintf("@I%s decided it's time for a map change", world.player[pid].name.c_str());
				maprot[world.player[pid].mapVote].votes = 99;
				server_next_map(NEXTMAP_VOTE_EXIT); // ignore return value
			}
			else
				network.bprintf("@I%s decided it's time for a restart", world.player[pid].name.c_str());
			ctf_game_restart();
		}
		else
			world.player[pid].queue_printf("@WUnknown command %s. Type /help for a list.", cbuf);
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
			network.player_message(pid, "@WToo much talk. Chill...");
		}
		else if (world.player[pid].muted==1)
			network.plprintf(pid, "@WYou are muted. You can't send messages.");
		else {
			char talkmsg[256];
			// check for team message:
			if (sbuf[0] == '.') {
				sprintf(talkmsg, "@T%s: %s", world.player[pid].name.c_str(), sbuf+1);
				if (world.player[pid].muted==2)
					network.player_message(pid, talkmsg);
				else
					network.broadcast_team_message(pid/TSIZE, talkmsg);
			}
			//regular msg
			else {
				sprintf(talkmsg, "%s: %s", world.player[pid].name.c_str(), sbuf);
				if (world.player[pid].muted==2)
					network.player_message(pid, talkmsg);
				else
					network.broadcast_message(talkmsg);
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

	// announce voting status
	if (world.frame >= next_vote_announce_frame) {
		int votes=0, players=0;
		for (int i=0; i<maxplayers; ++i)
			if (world.player[i].used) {
				++players;
				if (world.player[i].want_map_exit)
					++votes;
			}
		players=players/2+1;
		if (votes!=last_vote_announce_votes || (players!=last_vote_announce_needed && votes!=0)) {
			last_vote_announce_votes=votes;
			last_vote_announce_needed=players;
			next_vote_announce_frame=world.frame+SV_VOTE_ANNOUNCE_INTERVAL*10;
			ostringstream voteinfo;
			voteinfo << "@I*** " << votes << '/' << players << " votes for mapchange";
			if (world.getMapTime() < vote_block_time)
				voteinfo << " (all players needed for " << (vote_block_time-world.getMapTime()+5)/10 << " more seconds)";
			network.broadcast_message(voteinfo.str().c_str());
		}
		else if (world.getMapTime() == vote_block_time)
			check_map_exit();
	}
	for (int i=0; i<maxplayers; ++i)
		if (world.player[i].used) {
			if (world.player[i].kickTimer) {
				--world.player[i].kickTimer;
				if (world.player[i].kickTimer==0)
					network.disconnect_client(world.player[i].cid, 1);	// 1 second timeout
				else if (world.player[i].kickTimer%10 == 0 && world.player[i].kickTimer<=50)
					network.plprintf(i, "@WDisconnecting in %d...", world.player[i].kickTimer/10);
				continue;
			}
			if (idlekick_time != 0 && !world.player[i].attack && world.player[i].controls.idle()) {
				++world.player[i].idleFrames;
				int timeToKick = idlekick_time - world.player[i].idleFrames;
				if (timeToKick == 0)
					network.disconnect_client(world.player[i].cid, 1);
				else if ((timeToKick == 60*10 && idlekick_time >= 3*60*10) ||
						 (timeToKick == 30*10 && idlekick_time >= 3*30*10) ||
						 (timeToKick == 15*10 && idlekick_time >= 2*15*10) ||
						  timeToKick ==  5*10) {
					network.plprintf(i, "@W*** Idle kick: move or be kicked in %d seconds", timeToKick / 10);
				}
			}
			else
				world.player[i].idleFrames = 0;
			ServerPlayer::DMQueueT& dm=world.player[i].delayedMessages;
			while (dm.size() && --dm.begin()->first<0) {
				network.player_message(i, dm.begin()->second.c_str());
				dm.erase(dm.begin());
			}
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

	LOG("GAMESERVER::LOOP()\n");

	world.frame = 0;	//frame to generate next

	//sync with speed counter until it's time to generate one frame (== 1)
	server_speed_counter = -3;
	while (server_speed_counter < 1)
		MS_SLEEP(1);		// *** NO CPU PROBLEM HERE ***

	// no flag specified: esc quits
	bool keep_running = true;
	if (!running_flag)
		running_flag = &keep_running;

	LOG("GAMESERVER::LOOP() (2)\n");

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

	LOG("GAMESERVER::LOOP() (EXITING!)\n");
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

//============================================================
//  listen server thread
//============================================================

int listen_port_running;
volatile bool	listen_server_running = false;
pthread_t	listen_server_thread;

void* thread_listenserver_f(void*) {
	srand(time(0));

	//save for display
	listen_port_running = port;		//port selectr

	//(1) start the localserver
	//
	gameserver_c* gameserver = new gameserver_c();
	if (!gameserver->start(server_maxplayers)) {
		LOG("ERROR: cannot start LISTEN GAME SERVER!!!\n");
		listen_server_running = false;
		return 0;
	}

	//(2) loop the server until not quitting
	//
	gameserver->loop( &listen_server_running );

	//(3) shutdown the localserver
	//
	LOG("GAMESERVER STOPPING\n");
	gameserver->stop();
	LOG("GAMESERVER DELETING\n");
	delete gameserver;
	LOG("GAMESERVER DELETED\n");
	gameserver = 0;

	//restore client's windowtitle
	server_status_string("Outgun client - CTRL+F12 to quit");

	return 0;
}

void listen_start() {
	if (listen_server_running)
		return;
	listen_server_running = true;
	LOG("listen_start()\n");
	pthread_create(&listen_server_thread, 0, thread_listenserver_f, (void *)0);
}

void listen_stop() {
	if (!listen_server_running)
		return;
	listen_server_running = false;
	LOG("listen_stop()\n");
	pthread_join(listen_server_thread, 0);
}


