#include "commont.h"
#include "server.h"

//************************************************************
//  server stuff
//************************************************************

//delay for the server contacting the master server, in seconds
// it is good if this delay is set to a minute or so, since this will
// filter out people opening and closing servers frequently
#define	DELAY_TO_REPORT_SERVER	10.0

#define SERVER_DEFAULT_PLAYER_NAME "**DEFAULT**"

// client count
int	player_count;

// server callbacks
int sfunc_client_hello(runes_t *arg);
int sfunc_client_connected(runes_t *arg);
int sfunc_client_disconnected(runes_t *arg);
int sfunc_client_lag_status(runes_t *arg);
int sfunc_client_data(runes_t *arg);
int sfunc_client_ping_result(runes_t *arg);

//threads for tcp file transfer
void *thread_filemaster_f(void *arg);
void *thread_fileslave_f(void *arg);

//thread for master server interaction (LIST SERVER)
void *thread_mastertalker_f(void *arg);

//thread for server shell connections
void *thread_shellmaster_f(void *arg);
void *thread_shellslave_f(void *arg);

//master job (ACCOUNT/STATS SERVER)
void *thread_masterjob_f(void *arg);

gameserver_c *gameserver;

gameserver_c::MapInfo::MapInfo() : votes(0) { }

bool gameserver_c::MapInfo::load(string mapName) {
	Map map;
	bool ok = load_map(SERVER_MAPS_DIR, mapName, &map);
	if (!ok)
		return false;
	file = mapName;
	title = map.title;
	width = map.w;
	height = map.h;
	votes = 0;
	return true;
}

gameserver_c::gameserver_c() {
	server = 0;
	hostname[0]=0;	//hostname
	//memset(&world, 0, sizeof(frame_t));		//the current frame (game world simulation state)
	frame = 0;		// current frame count
	next_vote_announce_frame = 0;
	last_vote_announce_votes = last_vote_announce_needed = 0;

	server_kbps_traffic = 0;		//total server traffic in kbytes/sec
	ping_send_counter = 0;		// ping send counter
	ping_send_client = 0;

	for (int i=0; i<256; ++i)
		ctop[i]=-1;

	pthread_mutex_init(&fslavesock_mutex, 0);

	pthread_mutex_init(&mjob_mutex, 0);
}

gameserver_c::~gameserver_c() {

	pthread_mutex_destroy(&fslavesock_mutex);

	pthread_mutex_destroy(&mjob_mutex);

	LOG("GAMESERVER_C() DESTRUCTOR");
	if (server) {
		delete server;
		server = 0;
	}
}

void gameserver_c::mutePlayer(int pid, int mode) {	// 0 = unmute, 1 = normal, 2 = mute silently (do not inform the player)
	if (mode==0 && player[pid].muted!=2)
		plprintf(pid, "@WYou have been unmuted (you can send messages again)");
	else if (mode == 1)
		plprintf(pid, "@WYou have been muted (you can't send messages)");
	for (int i=0; i<MAX_PLAYERS; ++i)
		if (player[i].used && i!=pid) {
			if (mode == 0)
				plprintf(i, "@IThe admin has unmuted %s", player[pid].name);
			else
				plprintf(i, "@IThe admin has muted %s", player[pid].name);
		}
	player[pid].muted = mode;
}
void gameserver_c::kickPlayer(int pid, bool ban) {
	player[pid].delayedMessages.clear();
	if (ban)
		plprintf(pid, "@WYou are now BANNED from this server! Have a nice life...");
	else {
		plprintf(pid, "@WYou are being kicked from this server!");
		plprintf(pid, "@WWarning: you can get permanently banned for behaving badly!");
	}
	for (int i=0; i<MAX_PLAYERS; ++i)
		if (player[i].used && i!=pid)
			plprintf(i, "@IThe admin has %s %s (disconnect in 10 seconds)", ban?"banned":"kicked", player[pid].name);
	player[pid].kickTimer = 10*10;
}

#ifdef SV_NAME_AUTHORIZATION
void gameserver_c::banPlayer(int pid) {
	authorizations.ban(server->get_client_address(player[pid].cid));
	authorizations.save();
	kickPlayer(pid, true);
}
#endif

//v0.4.4 choose a kind from all chances
int gameserver_c::choose_powerup_kind() {

	int max = pup_chance_shield + pup_chance_turbo + pup_chance_shadow + pup_chance_power
						+ pup_chance_weapon + pup_chance_megahealth + pup_chance_deathbringer;

	int chance = 1 + rand() % max;		//1..100 por exemplo se max = 100

	chance -= pup_chance_shield;
	if (chance <= 0) return 1;
	chance -= pup_chance_turbo;
	if (chance <= 0) return 2;
	chance -= pup_chance_shadow;
	if (chance <= 0) return 3;
	chance -= pup_chance_power;
	if (chance <= 0) return 4;
	chance -= pup_chance_weapon;
	if (chance <= 0) return 5;
	chance -= pup_chance_megahealth;
	if (chance <= 0) return 6;
	//chance -= pup_chance_deathbringer;
	return 7;
}

// ---- FILE DOWNLOADING TO CLIENTS VIA UDP -------

//upload next chunk of a file to a client_c
void gameserver_c::upload_next_file_chunk(int i) {

	int CHUNKSIZE = 128;		// the max chunk size in bytes

	//actual size sent
	int chunksize = client[i].fsize - client[i].dp;		//attempt to send remaining...
	if (chunksize > CHUNKSIZE)							//...but there is the maximum
		chunksize = CHUNKSIZE;

	//check if will be last
	NLubyte islast = 0;	//default:no
	if (client[i].dp + chunksize == client[i].fsize) //maybe yes?
		islast = 1;		//yes.

	//send
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, 28);		//28 = next file chunk 4 u....
	writeByte(lebuf, count, islast);
	writeLong(lebuf, count, client[i].dp );
	writeShort(lebuf, count, ((NLushort)chunksize) );
	writeBlock(lebuf, count, &(client[i].data[ client[i].dp ] ), chunksize);
	server->send_message(i, lebuf, count);

	//save old dp for the ack
	client[i].old_dp = client[i].dp;

	//inc dp
	client[i].dp += chunksize;
}

//---- FILE DOWNLOADING TO CLIENTS VIA TCP --------

//load file from disk. puts file of type/name into the given buffer, returns filesize
int gameserver_c::get_download_file(char *lebuf, char *ftype, char *fname) {

	//map file type
	if (!strcmp(ftype, "map")) {
		if (strpbrk(fname, "./:\\")!=NULL) {
			LOG1("*!*!*!* ILLEGAL FILE DOWNLOAD ATTEMPT: MAP \"%s\"\n", fname);
			return -1;	//#should also kick their butt for that
		}

		char lebuffer[1024];
		char dest[WHERE_PATH_SIZE];

		// MAPDIR + / + MAPNAME + .TXT
		strcpy(lebuffer, SERVER_MAPS_DIR);
		put_backslash(lebuffer);
		strcat(lebuffer, fname);
		strcat(lebuffer, ".txt");

		//append all that to the root dir of the game
		append_filename(dest, wheregamedir, lebuffer, WHERE_PATH_SIZE);
		FILE *fmap = fopen(dest, "rb");
		if (fmap) {
			int amount = fread(lebuf, 1, 65536, fmap);
			fclose(fmap);
			LOG1("UPLOADING MAP \"%s\" (SV)\n", fname);
			return amount;	//size read!
		}
		else {
			LOG1("FAILED MAP DOWNLOAD ATTEMPT \"%s\" (SV)\n", fname);
			return -1;	//can't read!
		}
	}

	// don't know type!
	return -1;
}

//run a file master thread
void gameserver_c::run_filemaster_thread(void *) {

	while (1) {
		//accept one connection
		nlEnable(NL_BLOCKING_IO);
		NLsocket pidaosock = nlAcceptConnection(filesock);
		nlDisable(NL_BLOCKING_IO);

		//valid socket?
		if (pidaosock != NL_INVALID) {

			// FIXME: deny connections from client IPs that are not connected to the server already!

			//create thread for serving this client
			// & update list
			int i;
			pthread_mutex_lock( &fslavesock_mutex );

			for (i=0;i<MAX_PLAYERS;i++)
				if (fslavesock[i] == NL_INVALID) {
					pthread_create(&(fslavethr[i]), 0, thread_fileslave_f, (void *)i);	//keep thread so we can join
					fslavesock[i] = pidaosock;		//keep socket so it can be closed
					break;
				}

			pthread_mutex_unlock( &fslavesock_mutex );
		}
		else {
			if (file_threads_quit) {		//quitting!
				LOG("FILEMASTER THREAD IS QUITTING\n");
				nlClose(filesock);
				return;
			}
		}

		//sleep a bit
		MS_SLEEP(500);
	}
}

//run a file slave thread
void gameserver_c::run_fileslave_thread(void *arg) {

	//NLsocket sock = (NLsocket)arg;

	int k = (int)arg;

	pthread_mutex_lock( &fslavesock_mutex );
	NLsocket sock = fslavesock[k];
	pthread_mutex_unlock( &fslavesock_mutex );

	char lebuf[65536]; int count = 0;

	char ftype[256];
	char fname[256];

	while (1) {

		//read what client wants
		NLint result = nlRead(sock, lebuf, 1);
		if (file_threads_quit) break;
		//FIXME: check result
		if (result == 0) {
			MS_SLEEP(100);	//this should never be happening
			continue;
		}
		if (result != 1) {
			//not read a byte, NL_INVALID!
			//FIXME: client misbehaved, should also disconnect him
			LOG("ERROR: CLIENT MISBEHAVED HIS FILE SOCKET, QUITTING THREAD!");
			break;
		}

		NLubyte req;
		count = 0;
		readByte(lebuf, count, req);

		//what you want?
		if (req == 1) {		// gimme file!
			LOG("FILE SLAVE THREAD: CLIENT WANTS FILE!\n");

			int i;	//index
			bool bad_error;

			//READ ONE STRING ON LEBUF
			i = 0;
			bad_error = false;
			do {
				result = nlRead(sock, &(lebuf[i]), 1);
				if (file_threads_quit) break;
				//FIXME: check result
				if (result == 0) {
					MS_SLEEP(100);	//this should never be happening
					continue;
				}
				if (result != 1) {
					//not read a byte, NL_INVALID!
					//FIXME: client misbehaved, should also disconnect him
					bad_error = true;
					break;
				}

				if (lebuf[i] == 0)
					break;
				i++;
			} while (1);
			if (file_threads_quit) break;

			//can't break from inside the loop
			if (bad_error) {
				LOG("ERROR: CLIENT MISBEHAVED HIS FILE SOCKET, QUITTING THREAD! (2)");
				break;
			}

			//copy to ftype
			strcpy(ftype, lebuf);

			LOG1("FILE SLAVE THREAD: TYPE IS '%s'!\n", ftype);

			//READ ONE STRING ON LEBUF
			i = 0;
			bad_error = false;
			do {
				result = nlRead(sock, &(lebuf[i]), 1);
				if (file_threads_quit) break;
				//FIXME: check result
				if (result == 0) {
					MS_SLEEP(100);	//this should never be happening
					continue;
				}
				if (result != 1) {
					//not read a byte, NL_INVALID!
					//FIXME: client misbehaved, should also disconnect him
					bad_error = true;
					break;
				}
				if (lebuf[i] == 0)
					break;
				i++;
			} while (1);
			if (file_threads_quit) break;

			//can't break from inside the loop
			if (bad_error) {
				LOG("ERROR: CLIENT MISBEHAVED HIS FILE SOCKET, QUITTING THREAD! (3)");
				break;
			}

			//copy to fname
			strcpy(fname, lebuf);

			LOG1("FILE SLAVE THREAD: NAME IS '%s'!\n", fname);

			//load file from disk. puts file of type/name into the given buffer, returns filesize
			int filesize = get_download_file((char *)lebuf, ftype, fname);
			if (file_threads_quit) break;

			// FIXME: write 4 or something to signal THAT THE FILE WAS NOT FOUND then quit
			if (filesize == -1) {
				//FIXME: also client misbehaved -- asked for bad file -- disconnect him!
				LOG("ERROR: CLIENT MISBEHAVED ASKING FOR BAD FILE, QUITTING THREAD!");
				break;
			}

			//write 2 (sending file baby!)
			char	leheader[256];
			count = 0;
			writeByte(leheader, count, 2);	//ok baby, sending!
			writeShort(leheader, count, 0);		//FIXME: send file CRC
			writeLong(leheader, count, (NLulong)filesize);	// file size
			result = nlWrite(sock, leheader, count);
			if (file_threads_quit) break;
			//FIXME: check result
			if (result != count) {
				//not read a byte, NL_INVALID!
				//FIXME: client misbehaved, should also disconnect him
				LOG("ERROR: CLIENT MISBEHAVED HIS FILE SOCKET, QUITTING THREAD! (4)");
				break;
			}

			LOG1("FILE SLAVE THREAD: FILESIZE UPLOADED IS %i\n", filesize);

			//send file to client as a big chunk)
			// FIXME: make this send in smaller pieces
			result = nlWrite(sock, lebuf, filesize);
			if (file_threads_quit) break;
			// FIXME: check result
			if (result != filesize) {
				//not read a byte, NL_INVALID!
				//FIXME: client misbehaved, should also disconnect him
				LOG("ERROR: CLIENT MISBEHAVED HIS FILE SOCKET, QUITTING THREAD! (5)");
				break;
			}

			LOG1("FILE SLAVE THREAD: RESULT OF UPLOAD IS %i\n", result);
		}
		else if (req == 3) {	// bye!
			LOG("FILE SLAVE THREAD GIVING BYE...\n");
			//ok..
			//nlClose(sock);
			break;
		}
	}

	//thread is no more - will quit by itself, no need to notify
	nlClose(sock);
	LOG("FILESLAVE THREAD IS QUITTING...");
	pthread_mutex_lock( &fslavesock_mutex );
	fslavethr[k] = (pthread_t)-1;		//"invalid"?
	fslavesock[k] = NL_INVALID;
	pthread_mutex_unlock( &fslavesock_mutex );
	LOG("QUIT!\n");
}

//---- CTF/GAME ------------------

//also called the "first packet"
// na verdade eh um packet de proposito geral. manda sempre que:
//  - fisica mudar
//  - "myself" de um client mudar
void gameserver_c::send_me_packet(int pid) {
	int count = 0;
	char lebuf[1024];
	writeByte(lebuf, count, 3); // "3" = first-packet information
	writeByte(lebuf, count, ((NLubyte)pid) );		// who am I
	writeByte(lebuf, count, ((NLubyte)world.flag[0].score) );		//team 0 current score
	writeByte(lebuf, count, ((NLubyte)world.flag[1].score) );		//team 1 current score
	//server physics parameters
	writeFloat(lebuf, count, ((NLfloat)svp_fric) );
	writeFloat(lebuf, count, ((NLfloat)svp_accel) );
	writeFloat(lebuf, count, ((NLfloat)svp_maxspeed) );
	writeFloat(lebuf, count, ((NLfloat)svp_fric_run) );
	writeFloat(lebuf, count, ((NLfloat)svp_accel_run) );
	writeFloat(lebuf, count, ((NLfloat)svp_maxspeed_run) );
	writeFloat(lebuf, count, ((NLfloat)svp_fric_turbo) );
	writeFloat(lebuf, count, ((NLfloat)svp_accel_turbo) );
	writeFloat(lebuf, count, ((NLfloat)svp_maxspeed_turbo) );
	writeFloat(lebuf, count, ((NLfloat)svp_fric_turborun) );
	writeFloat(lebuf, count, ((NLfloat)svp_accel_turborun) );
	writeFloat(lebuf, count, ((NLfloat)svp_maxspeed_turborun) );
	writeFloat(lebuf, count, ((NLfloat)svp_flag_penalty) );
	server->send_message(player[pid].cid, lebuf, count);
}

//send a player name update to a client
void gameserver_c::send_player_name_update(int cid, int pid) {

	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, 1);	// "1" = player name update
	writeByte(lebuf, count, pid);		// what player id
	player[pid].name[15]=0;		// force trunc name at 15 chars (paranoia)
	writeString(lebuf, count, player[pid].name);
	server->send_message(cid, lebuf, count);
}

//broadcast new player name
void gameserver_c::broadcast_player_name(int pid) {

	for (int i=0;i<maxplayers;i++)
	if (player[i].used)
		send_player_name_update(player[i].cid, pid);

	//server->broadcast_message(lebuf, count);
}

//send a player crap update to a client
void gameserver_c::send_player_crap_update(int cid, int pid) {

	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, 32);	// "32" = player CRAP update

	// --- RECALC CRAP ---
	//reg_status char:
	if (client[ player[pid].cid ].token_have) {
		if (client[ player[pid].cid ].token_valid)
			player[pid].reg_status = '*';
		else
			player[pid].reg_status = '?';
	}
	else
		player[pid].reg_status = ' ';

	writeByte(lebuf, count, ((NLubyte)pid));
	writeByte(lebuf, count, ((NLubyte)player[pid].reg_status));					//regstatus
	writeLong(lebuf, count, ((NLulong)client[player[pid].cid].rank));		//ranking#
	writeLong(lebuf, count, ((NLulong)client[player[pid].cid].score));		//score POS
	writeLong(lebuf, count, ((NLulong)client[player[pid].cid].neg_score));		//score NEG v0.4.8
	writeLong(lebuf, count, ((NLulong)max_world_rank));		//MAX WORLD ranking#
	writeLong(lebuf, count, ((NLulong)max_world_score));		//MAX WORLD score

	//LOG5("CRAPZ SENT TO CID %i of PID %i %c r:%i s:%i\n", cid, pid, player[pid].reg_status
	//	,client[player[pid].cid].rank
	//	,client[player[pid].cid].score);

	server->send_message(cid, lebuf, count);
}

//v0.4.5: broadcast player crap
void gameserver_c::broadcast_player_crap(int pid) {

	for (int i=0;i<maxplayers;i++)
	if (player[i].used)
		send_player_crap_update(player[i].cid, pid);
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
	if (!player[pid].used) return;
	if (!player[pid].want_change_teams) return;
	if (get_time() < player[pid].team_change_time) {
		player[pid].team_change_pending = true;	//vai continuar tentando
		return;	//v0.3.3 : intervalos minimos para troca de times
	}

	//count players in each team
	int tc[2];tc[0]=0;tc[1]=0;
	for (int i=0;i<maxplayers;i++)
		if (player[i].used)
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
			if (!player[i].used)		// player vago
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
		if (player[i].used)	// um player
		if (i/TSIZE != pid/TSIZE)		// do outro time
		if (player[i].want_change_teams)		// que quer trocar de time
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
				if (!player[i].used)		// player vago
				{
					move_player(pid, i);	// move pid to free slot
					break;
				}
			}
			/*
			for (int i=0;i<MAX_PLAYERS;i++)
			if (!player[i].used)		// player vago
			if (i/TSIZE != pid/TSIZE)			// no time oposto
			{
				move_player(pid, i);	// move pid to free slot
				break;
			}
			*/
		}
	}
}

// messages to update moved players (players/clients with new clients/players)
//
void gameserver_c::move_update_player(int a) {

	if (player[a].used) {
		broadcast_player_name(a);

			send_me_packet(a);
			/*
			count = 0;
			writeByte(lebuf, count, 3); // "3" = first-packet information
			writeByte(lebuf, count, ((NLubyte)a) );		// who am I
			writeByte(lebuf, count, ((NLubyte)world.flag[0].score) );		//team 0 current score
			writeByte(lebuf, count, ((NLubyte)world.flag[1].score) );		//team 1 current score
			server->send_message(player[a].cid, lebuf, count);
			*/

			//v0.4.4 : tentativa de conserto
			//broadcast frags update
			char lebuf[256]; int count = 0;
			writeByte(lebuf, count, 4);	//"4" = frags update
			writeByte(lebuf, count, a);		// what player id
			writeLong(lebuf, count, player[a].frags);
			server->broadcast_message(lebuf, count);

			//v0.4.5 : atualiza registration char / score / rank
			broadcast_player_crap( a );

			//name (NEEDED? FIXME - ja tem la em cima!)
			//broadcast_player_name( a );

		//message
		bprintf("@I%s moved to %s team", player[a].name, teamname[a/TSIZE]);
	}
}

//move player - move player (f rom) to empty position (t o)
//
void gameserver_c::move_player(int f, int t) {

	//broadcast sound
	broadcast_sample(SAMPLE_CHANGETEAM);

	//UGLY HACK
	if (!check[t]) {
		check[t] = 1;
		checount--;
	}

	ctf_drop_flag_if_any(f);

	//copy to t
	player[t] = player[f];

	//copy hero
	world.hero[t] = world.hero[f];

	//remove f
	game_remove_player(f);

	//update ctop
	ctop[ player[t].cid ] = t;

	//I really dont want to change teams no more..
	player[t].want_change_teams = false;
	player[t].team_change_time = get_time() + 10.0;		//10 secs interval

	//kill t
	if (player[t].health > 0)
		game_reset_player(t);

	//update t
	move_update_player(t);
}

//swap players - both are valid players
void gameserver_c::swap_players(int a, int b) {
	broadcast_sample(SAMPLE_CHANGETEAM);

	if (player[a].health > 0)
		game_reset_player(a);
	if (player[b].health > 0)
		game_reset_player(b);

	swap(player[a], player[b]);

	//swap client id's
	ctop[ player[a].cid ] = a;
	ctop[ player[b].cid ] = b;

	//either don't want to change teams anymore
	player[a].want_change_teams = false;
	player[a].team_change_time = get_time() + 10.0;		//10 secs interval
	player[b].want_change_teams = false;
	player[b].team_change_time = get_time() + 10.0;		//10 secs interval

	//send updates
	move_update_player(a);
	move_update_player(b);
}

//broadcast a sample
void gameserver_c::broadcast_sample(int code) {

	char lebuf[64]; int count = 0;
	writeByte(lebuf, count, 14);		// sample play
	writeByte(lebuf, count, (NLubyte)code);		// the sample code
	server->broadcast_message(lebuf, count);
}

//play a sample to a player's screen audience
void gameserver_c::broadcast_screen_sample(int p, int code) {
	char lebuf[64]; int count = 0;
	writeByte(lebuf, count, 14);		// sample play
	writeByte(lebuf, count, (NLubyte)code);		// the sample code
	broadcast_screen_message(player[p].x, player[p].y, (char*)lebuf, count);
}

//send current flag status (cid == -1 : broadcast)
void gameserver_c::ctf_net_flag_status(int cid, int team) {

	//just resetting server state -- no update needed
	if (!server) return;

	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, 6);	// 6 = flag status update

	NLubyte te = (NLubyte)team;
	writeByte(lebuf, count, te);	//what team

	if (world.flag[team].carried)	{ //carried?

		writeByte(lebuf, count, 1);	//TRUE

		//new flag carrier
		NLubyte thecarrier = ((NLubyte)world.flag[team].carrier);
		writeByte(lebuf, count, thecarrier);	//player who took it
	}
	else {
		NLubyte	p; NLshort sh;

		writeByte(lebuf, count, 0);	//FALSE

		//new flag position
		p = (NLubyte)world.flag[team].pos.px;		//px
		writeByte(lebuf, count, p);

		p = (NLubyte)world.flag[team].pos.py;		//py
		writeByte(lebuf, count, p);

		sh = (NLshort)world.flag[team].pos.x;		//x  FIXED v0.5.0
		writeShort(lebuf, count, sh);

		sh = (NLshort)world.flag[team].pos.y;		//y  FIXED v0.5.0
		writeShort(lebuf, count, sh);
	}

	if (cid == -1)
		server->broadcast_message(lebuf, count);
	else
		server->send_message(cid, lebuf, count);
}

//return flag to base position
void gameserver_c::ctf_return_flag(int team) {

	world.flag[team].carried = false;			// not carried anymore
	world.flag[team].pos = map.tinfo[team].flag;		// return to original position
	world.flag[team].atbase = true;		// yes, at base

	ctf_net_flag_status(-1, team);	// broadcast new status
}

//drop flag on ground
void gameserver_c::ctf_drop_flag(int team, int px, int py, int x, int y) {

	world.flag[team].carried = false;		// not carried
	world.flag[team].pos.px = px;		// dropped somewhere
	world.flag[team].pos.py = py;
	world.flag[team].pos.x = x;
	world.flag[team].pos.y = y;
	world.flag[team].atbase = false;		// not at base, team must touch to return (or it can be stolen)

	ctf_net_flag_status(-1, team);	// broadcast new status
}

//steal flag
void gameserver_c::ctf_steal_flag(int team, int carrier) {

	world.flag[team].carried = true;		// carried
	world.flag[team].carrier = carrier;	// who stole it
	world.flag[team].atbase = false;		// not at base (not needed / paranoia)

	ctf_net_flag_status(-1, team); // broadcast new status
}

//update team scores
void gameserver_c::ctf_update_teamscore(int t) {

	if (world.flag[t].score == capture_limit) {

		//change map!
		server_next_map(NEXTMAP_CAPTURE_LIMIT);	// ignore return value

		//maximum score reached -- restart game (reposiciona jogadores no novo mapa)
		ctf_game_restart();

		return;
	}

	char lebuf[64]; int count = 0;
	writeByte(lebuf, count, 9);		// CTF teamscore update
	writeByte(lebuf, count, ((NLubyte)t));		// the team
	writeByte(lebuf, count, ((NLubyte)world.flag[t].score));	//the score
	server->broadcast_message(lebuf, count);
}

//respawn player. killed==true when player was killed, if joining game or it's
//		a fresh game start, it's set to false
//  if killed==true, use team spawn points and world spawn points
//	if killed==false, use team spawn points only
//
void gameserver_c::game_respawn_player(int pid) {

	// not time to respawn anymore
	player[pid].respawn_time = -1;

	//the player's team
	int t = pid/TSIZE;

	spoint_t pos;
	if (map.tinfo[t].spawn.empty())
		player[pid].respawn_to_base = false;
	else if (player[pid].respawn_to_base) {
		//choose a team spawn point
		if (++map.tinfo[t].lastspawn >= map.tinfo[t].spawn.size())
			map.tinfo[t].lastspawn = 0;
		pos = map.tinfo[t].spawn[ map.tinfo[t].lastspawn ];	// the point
	}

	//if was killed or map spawn point places player over a wall
	if (!player[pid].respawn_to_base || map.fall_on_wall(pos.px, pos.py, pos.x-20, pos.y-SV_SHIFTY-20, pos.x+20, pos.y-SV_SHIFTY+20)) {
		// generate a random spot for respawn:
		// - unnocupied screen
		// - away from walls

		//calculate room touch matrix
		vector<bool> roompop;
		roompop.resize(map.w*map.h, false);
		for (int i=0;i<maxplayers;i++)
			if (player[i].used && player[i].x >= 0 && player[i].y >= 0 && player[i].x < map.w && player[i].y < map.h)
				roompop[player[i].y * map.w + player[i].x] = true;

		int runaway = 400;
		do {
			//find screen
			int ridx;
			do {
				ridx = rand() % (map.w*map.h);
			} while ((runaway-- > 200) && (roompop[ridx] == true));	//keep trying until unnocupied (==false)
			pos.px = ridx%map.w;
			pos.py = ridx/map.w;

			//find a suitable coordinate -- middle square
			pos.x = plw / 8 + rand() % (3 * plw / 4);
			pos.y = plh / 8 + rand() % (3 * plh / 4) +SV_SHIFTY;

			//do a check for walls, maybe retrying another screen if hits a wall
			if (!map.fall_on_wall(pos.px, pos.py, pos.x-20, pos.y-SV_SHIFTY-20, pos.x+20, pos.y-SV_SHIFTY+20))
				break;	//success!

			//fall on wall true, keep trying...

		} while (runaway-- > 0);

		if (runaway <= 0)
			broadcast_message("PLAYER SPAWN RUNAWAY");
	}

	//put player there
	//LOG("SPAWN %i %i  %i %i\n", pos.px, pos.py, pos.x, pos.y);
	player[pid].x = pos.px;	//screen
	player[pid].y = pos.py;
	world.hero[pid].x = pos.x;	//screen position
	world.hero[pid].y = pos.y;

	//reset speeds / z
	world.hero[pid].sx = 0;
	world.hero[pid].sy = 0;

	//reset player attributes
	player[pid].health = 100;
	player[pid].energy = 100;
	player[pid].megabonus = 0;  //balaca megahealth

	player[pid].weapon = 0;		//default weapon

	//notify player weapon power change
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, 18);		//player power change
	writeByte(lebuf, count, ((NLubyte)player[pid].weapon) );
	server->send_message(player[pid].cid, lebuf, count);

	player[pid].item_shield = false;			// no items
	player[pid].item_quad = false;
	player[pid].item_speed = false;
	player[pid].item_helm = 0;
	player[pid].item_deathbringer = false;//
	player[pid].deathbringer_end = 0;		//not hit by deathbringer yet

	player[pid].respawn_to_base = false;

	player[pid].last_spawn_time = (int)get_time();
	player[pid].dead = false;

	//for all effects, player screen changed
	game_player_screen_change(pid);

	//FIXME: add teleport visual effect and sound
}

//delete a rocket
void gameserver_c::game_delete_rocket(int r, NLshort hitx, NLshort hity, int targ) {

	rocket_c *rock = &(world.rock[r]);

	//assembly rocket delete message
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, 8);		// 8 = rocket deletion
	NLubyte byt = (NLubyte)r;
	writeByte(lebuf, count, byt);		// rocket-object id
	byt = (NLubyte)targ;
	writeByte(lebuf, count, byt);		// player-target. if 255, no player in particular was hit

	//byt = (NLubyte)framesleft;
	//writeByte(lebuf, count, byt);		// 10-msecs' left to the client to animate
	writeShort(lebuf, count, hitx);		// HIT X,Y OF ROCKET
	writeShort(lebuf, count, hity);

	//send message to players that received the rocket
	for (int p=0;p<maxplayers;p++)
	if (player[p].used)								//still valid player? (nao custa checar..)
	if (rock->vislist & (1 << p))			//verifica se o bit de "conhece o rocket" ta ligado
	{
		// send the message to this player
		server->send_message(player[p].cid, lebuf, count);

		//LOG2("...sent to pl=%i rock=%i\n", p, byt);
	}

	//server-side invalidate
	rock->owner = -1;
}

//make damn rocket v0.4.7 remendo chute brabo pra tentar consertar 1 bug
void gameserver_c::make_damn_rocket(int i, int playernum, int px, int py, int x, int y, double deg, int xdelta) {
	//alloc
	rocket_c *rock = &(world.rock[i]);
	rock->owner = playernum;
	rock->team = playernum/TSIZE;
	rock->px = px;
	rock->py = py;
	rock->x = x;
	rock->y = y;
	rock->deg = deg;	//direcao em RADIANOS
	rock->hit_time = 0;
	//speed nos eixos: constante depende da direcao
	rock->sx = cos(rock->deg) * (ROCKET_SPEED);
	rock->sy = sin(rock->deg) * (ROCKET_SPEED);

	//deslocamento a 90graus
	rock->x += xdelta * cos(deg + PI/2);
	rock->y += xdelta * sin(deg + PI/2);

	//REMENDAO: avanca 0,5 frame  (5 vezes 1 decimo da velo (/2)
	rock->x += rock->sx * 5.0 / 10.0;
	rock->y += rock->sy * 5.0 / 10.0;
}

//shoot rocket to a certain direction
//deg: em radianos
//retorno: id do rocket alocado
//XDELTA: deslocamento positivo para a direita ou negativo para a esquerda
NLubyte gameserver_c::game_do_shoot_rocket(int playernum, int px, int py, int x, int y, double deg, int xdelta) {

	for (NLubyte i=0;i<MAX_ROCKETS;i++)
		if (world.rock[i].owner == -1) { //unused
			make_damn_rocket(i,playernum,px,py,x,y,deg,xdelta);
			return i;
		}

	//whoops!
	LOG("WHOOPS!\n");
	int wtf = rand() % MAX_ROCKETS;
	make_damn_rocket(wtf,playernum,px,py,x,y,deg,xdelta);
	return (NLubyte)wtf;
}

//versao 0.1.2
void gameserver_c::game_shoot_rocket(int playernum, int shots, int px, int py, int x, int y, int gundir) {

	player[playernum].total_shots++;

	//ids alocados pra shots
	NLubyte		sid[16];

	// center degree
	double cdeg = gundir * PIOIT;

	//allocate a new rocket server-side for each shot
	// shots = qual arma (1-9 tiros!)
	switch (shots) {
	case 1:
		sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, 0);
		break;
	case 2:
		sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, - SHOT_DELTAX);
		sid[1] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, + SHOT_DELTAX);
		break;
	case 3:
		//V0.4.8 : NEW TRIPLE SHOT!
		sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, 0);
		sid[1] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, - SHOT_DELTAX * 2);
		sid[2] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, + SHOT_DELTAX * 2);
		//sid[1] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT, 0);
		//sid[2] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT, 0);
		break;
	case 4:
		sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, - SHOT_DELTAX);
		sid[1] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, + SHOT_DELTAX);
		sid[2] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT, 0);
		sid[3] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT, 0);
		break;
	case 5:
		sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, 0);
		sid[1] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, - SHOT_DELTAX * 2);
		sid[2] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, + SHOT_DELTAX * 2);
		sid[3] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT * 2, 0);
		sid[4] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT * 2, 0);
		break;
	case 6:
		sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, - SHOT_DELTAX);
		sid[1] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, + SHOT_DELTAX);
		sid[2] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT, 0);
		sid[3] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT, 0);
		sid[4] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT * 2, 0);
		sid[5] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT * 2, 0);
		break;
	case 7:
		sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, 0);
		sid[1] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, - SHOT_DELTAX * 2);
		sid[2] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, + SHOT_DELTAX * 2);
		sid[3] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT * 2, 0);
		sid[4] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT * 2, 0);
		sid[5] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT * 3, 0);
		sid[6] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT * 3, 0);
		break;
	case 8:
		sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, - SHOT_DELTAX);
		sid[1] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, + SHOT_DELTAX);
		sid[2] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT, 0);
		sid[3] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT, 0);
		sid[4] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT * 2, 0);
		sid[5] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT * 2, 0);
		sid[6] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT * 3, 0);
		sid[7] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT * 3, 0);
		break;
	case 9:
		sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, 0);
		sid[1] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, - SHOT_DELTAX * 2);
		sid[2] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, + SHOT_DELTAX * 2);
		sid[3] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT, 0);
		sid[4] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT, 0);
		sid[5] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT * 2, 0);
		sid[6] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT * 2, 0);
		sid[7] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT * 3, 0);
		sid[8] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT * 3, 0);
		break;
	}

	//assembly multi-rocket message
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, 7);		// 7 = MULTI rocket fire
	NLubyte  powerdir;		//bits 0..4 = power bits 5..8=dir
	powerdir = (NLubyte)shots;			// shots

	//powerdir += (NLubyte)(gundir * 16);	//16,32,64,128
	// AWW FUCK IT
	int fuck = powerdir;
	fuck += gundir * 16;
	powerdir = (NLubyte)fuck;

	//writeByte(lebuf, count, powerdir);		// power and dir
	writeByte(lebuf, count, shots);		// power and dir
	writeByte(lebuf, count, gundir);		// power and dir
	for (int i=0;i<shots;i++) //MULTI ROCKETS!
		writeByte(lebuf, count, sid[i]);		// rocket-object id (needed because client-side rockets can be deleted by the server)
	writeLong(lebuf, count, this->frame);	// time of shot of the rocket: current (last simulated) frame
	writeByte(lebuf, count, (NLubyte)playernum);	// owner of all rockets
	writeByte(lebuf, count, (NLubyte)px);	//coord
	writeByte(lebuf, count, (NLubyte)py);
	writeShort(lebuf, count, (NLshort)x);
	writeShort(lebuf, count, (NLshort)y);

	//send to all people, build people-that-know DOUBLE WORD (32bits == 32players max)
	//send message to players on the same screen
	NLulong  vislist = 0;
	for (int p=0;p<maxplayers;p++)
	if (player[p].used)
	if (player[p].x == px)
	if (player[p].y == py) {
		vislist += (1 << p);	// mark as sent
		server->send_message(player[p].cid, lebuf, count);	// send the message to this player
	}

	//mark all created rockets with the vislist
	for (int k=0;k<shots;k++)
		world.rock[ sid[k] ].vislist = vislist;
}

//ctf player drops flag if carrying any
bool gameserver_c::ctf_drop_flag_if_any(int pid) {

	int enemyteam = 1 - (pid/TSIZE);

	//if is carrier of enemy flag, drop it, extra frag for fragging carrier
	if (world.flag[enemyteam].carried)		// attacker team's flag carried
	if (world.flag[enemyteam].carrier == pid) {	//...by the target

		//message
		bprintf("@I%s LOST THE %s FLAG!", player[pid].name, teamname[enemyteam]);

		//sound broadcast
		broadcast_sample(SAMPLE_CTF_LOST);

		//drop the flag
		ctf_drop_flag(enemyteam, player[pid].x, player[pid].y, (int)world.hero[pid].x, (int)world.hero[pid].y);

		player[pid].total_flags_dropped++;

		return true;
	}

	return false;
}


//refresh team ratings
void gameserver_c::refresh_team_score_modifiers() {

	double raw[2];
	raw[0]=0.0;
	raw[1]=0.0;

	//somatorio raw ratings
	for (int p=0;p<maxplayers;p++)
	if (player[p].used) {
		// use "1.0" rating for anybody with less than 100 positive points
		if (client[player[p].cid].score < MINIMUM_POSITIVE_SCORE_FOR_RANKING)
			raw[p/TSIZE] += DEFAULT_PLAYER_RATE;		// default player rate
		else
			raw[p/TSIZE] += ( ((double)client[player[p].cid].score) + 1.0) / ( ((double)client[player[p].cid].neg_score) + 1.0);
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
	player[p].frags += amount;

	//v0.4.4 -- add score to the player's score accumulator
	//v0.4.7: DO NOT add score if map is not valid for scoring
	if (map.valid_for_scoring)
	if (player_count >= 2) { //v0.4.7.1 : skip the scoring if only one player present

		//refresh team ratings
		refresh_team_score_modifiers();

		int cid = player[p].cid;

		double parcela = ((double)amount) * team_smul[p/TSIZE];

		//add normalizado
		client[cid].fdp += parcela;

		//refresh "inteiro version"
		client[cid].delta_score = (int)(client[cid].fdp);

		//DEBUGz
		//char lix[256];
		//sprintf(lix, "%s scores +%.4f for %.4f +delta", player[p].name, parcela, client[cid].fdp);
		//broadcast_message(lix);

		//client[cid].delta_score += amount;		//just add the frags for now
	}
}

//score! NEG FRAG (v0.4.8)
void gameserver_c::score_neg(int p, int amount) {

	//add regular frags amount
	//player[p].frags += amount;

	//v0.4.4 -- add score to the player's score accumulator
	//v0.4.7: DO NOT add score if map is not valid for scoring
	if (map.valid_for_scoring)
	if (player_count >= 2) { //v0.4.7.1 : skip the scoring if only one player present

		//refresh team ratings
		refresh_team_score_modifiers();

		int cid = player[p].cid;

		double parcela = ((double)amount);		// NAO multiplica....

		//add normalizado
		client[cid].fdn += parcela;

		//refresh "inteiro version"
		client[cid].neg_delta_score = (int)(client[cid].fdn);

		//DEBUGz
		//char lix[256];
		//sprintf(lix, "%s scores -%.4f for %.4f -delta", player[p].name, parcela, client[cid].fdn);
		//broadcast_message(lix);

		//client[cid].neg_delta_score += amount;		//just add the frags for now V0.4.8: NEG SCORE!
	}
}

//enqueue a job to the master server to update a client's delta score
void gameserver_c::client_report_status(int id) {

	if (client[id].token_have)			// told token
	if (client[id].token_valid)			// validated token
	if ((client[id].delta_score != 0) || (client[id].neg_delta_score != 0)) { //if zero, NOP

		//submit-- create job
		masterjob_c *job = new masterjob_c();
		job->cid = id;
		job->code = 2;

		//V0.4.8: envia POS e NEG deltascore
		sprintf(job->request, "GET /servlet/fcecin.tk1/index.html?%s&dscp=%i&dscn=%i&name=%s&token=%s\n\n", TK1_VERSION_STRING, client[id].delta_score, client[id].neg_delta_score, player[ ctop [id] ].name, client[id].token);

		//LOG2("== MJOB for REPORT STATUS : %i '''%s'''\n", id, job->request);

		pthread_t mjob_thread;
		pthread_create (&mjob_thread, 0, thread_masterjob_f, (void *)job);

		//assume new score computed
		//client[id].score += client[id].delta_score;
		// NOT! new score will come...

		//reset the delta
		client[id].delta_score = 0;
		client[id].neg_delta_score = 0;
		client[id].fdp = 0.0;
		client[id].fdn = 0.0;
	}
}

void gameserver_c::game_reset_player(int target, float time_penalty) {	// take the player out of the game
	player[target].health = 0;

	player[target].item_helm = 0;
	player[target].item_quad = false;
	player[target].item_speed = false;
	// deathbringer is not removed until respawn because the flag is needed

	//stop all speed
	world.hero[target].sx = 0;
	world.hero[target].sy = 0;

	ctf_drop_flag_if_any(target);
	player[target].respawn_time = get_time() + respawn_time + time_penalty;
	if (!player[target].dead) {
		player[target].lifetime += (int)get_time() - player[target].last_spawn_time;
		player[target].dead = true;
	}
}

void gameserver_c::game_kill_player(int target, bool time_penalty) {	// kill the player in the usual way with score penalties and deathbringer effect
	score_neg(target, 1);	// score neg points because of death
	if (ctf_drop_flag_if_any(target))
		score_neg(target, 1);	// score neg points because of losing the flag
	player[target].total_deaths++;
	if (++player[target].current_consecutive_deaths > player[target].most_consecutive_deaths)
		player[target].most_consecutive_deaths = player[target].current_consecutive_deaths;
	player[target].current_consecutive_kills = 0;

	if (player[target].item_deathbringer) {
		//record time to simulate the deathbringer explosion
		player[target].item_deathbringer_time = frame;

		//deathbringer message
		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 26);	//26==deathbringer
		writeByte(lebuf, count, ((NLubyte)target));	//team/target player
		writeLong(lebuf, count, frame);		//frame # of the bringer shot (message can be delayed)
		//LIE: x,y can be taken from the player since it's rock-dead on the bringer's position
		//v0.4.6: somehow the graphical effect is playing on the wrong screen of the clients
		//  trying sending screen x,y and player x y
		writeByte(lebuf, count, ((NLubyte)player[target].x));
		writeByte(lebuf, count, ((NLubyte)player[target].y));
		writeShort(lebuf, count, ((NLushort)world.hero[target].x));
		writeShort(lebuf, count, ((NLushort)world.hero[target].y));

		server->broadcast_message(lebuf, count);
	}

	game_reset_player(target, (player[target].item_deathbringer || time_penalty)?waiting_time_deathbringer:0);
}

void gameserver_c::game_damage_player(int target, int attacker, int damage, bool deathbringer) {	// inflict normal or deathbringer damage on target
	//HELM powerup: show player
	if (player[target].item_helm > 0)
		player[target].item_helm = 255;

	if (player[target].item_shield) {
		player[target].energy -= damage;
		if (player[target].energy <= 0) {
			player[target].energy = 0;
			player[target].item_shield = false;
			if (!deathbringer)
				broadcast_screen_sample(target, SAMPLE_SHIELD_LOST);
		}
		else if (!deathbringer)
			broadcast_screen_sample(target, SAMPLE_SHIELD_DAMAGE);
	}
	//else do the regular body damage
	else {
		player[target].health -= damage;
		//freeze target's gun
		player[target].next_shoot_time = get_time() + 1.0;
	}
	if (player[target].health > 0)
		return;

	score_frag(attacker, 1);	//frag to attacker for the kill
	player[attacker].total_kills++;
	if (++player[attacker].current_consecutive_kills > player[attacker].most_consecutive_kills)
		player[attacker].most_consecutive_kills = player[attacker].current_consecutive_kills;
	player[attacker].current_consecutive_deaths = 0;

	int tateam = target/TSIZE;
	int atteam = attacker/TSIZE;

	//check if the enemy flag is carried in this screen(target's) by somebody that is not me
	if (world.flag[tateam].carried) {
		int p = world.flag[tateam].carrier;
		if (player[p].used && p!=attacker && player[p].x==player[target].x && player[p].y == player[target].y) {
			bprintf("@I%s DEFENDS THE %s CARRIER", player[attacker].name, teamname[atteam]);
			score_frag(attacker, 1);
		}
	}
	if (!world.flag[atteam].carried && world.flag[atteam].pos.px==player[target].x && world.flag[atteam].pos.py==player[target].y) {
		bprintf("@I%s DEFENDS THE %s FLAG", player[attacker].name, teamname[atteam]);
		score_frag(attacker, 1);
	}
	if (world.flag[atteam].carried && world.flag[atteam].carrier==target) {
		score_frag(attacker, 1);	// extra frag for fragging a carrier
		player[attacker].total_flag_carriers_killed++;
	}

	if (deathbringer) {
		if (player[attacker].used)
			bprintf("@I%s was choked by %s", player[target].name, player[attacker].name);
		broadcast_screen_sample(target, SAMPLE_DIEDEATHBRINGER);
	}
	else
		bprintf("@I%s was nailed by %s", player[target].name, player[attacker].name);

	//update the ADMIN SHELL
	if (shellssock) {
		char lebuf[256]; int count; NLint result;
		count = 0;
		writeLong(lebuf, count, STA_PLAYER_KILLS);
		writeLong(lebuf, count, player[attacker].cid);
		result = nlWrite(shellssock, lebuf, count);
		count = 0;
		writeLong(lebuf, count, STA_PLAYER_DIES);
		writeLong(lebuf, count, player[target].cid);
		result = nlWrite(shellssock, lebuf, count);
	}

	game_kill_player(target, false);
}

//remove player from the game
void gameserver_c::game_remove_player(int pid) {
	//remove all shots from this player
	for (int r=0; r<MAX_ROCKETS; r++)
		if (world.rock[r].owner == pid)
			game_delete_rocket(r, 0, 0, 255);

	ctf_drop_flag_if_any(pid);

	//erase player
	player[pid].delayedMessages.clear();
	ctop[player[pid].cid] = -1;
	player[pid].used = false;
}

//restart ctf game
void gameserver_c::ctf_game_restart() {

	int i,cid;

	//submit all pending reports
	for (i=0;i<maxplayers;i++)
	if (player[i].used)
	{
		cid = player[i].cid;
		client_report_status(cid);
	}

	//final score
	char lix[256];
	sprintf(lix, "@ICTF GAME RESTARTED - FINAL SCORE:   %i RED x %i BLUE !", world.flag[0].score, world.flag[1].score);
	broadcast_message(lix);

	//tell players the capture and time limit
	if (time_limit == 0)
		sprintf(lix, "@ICAPTURE %i FLAGS TO WIN THE GAME", capture_limit);
	else
		sprintf(lix, "@ICAPTURE %i FLAGS TO WIN THE GAME - TIME LIMIT IS %lu MINUTES", capture_limit, time_limit / 10 / 60);
	broadcast_message(lix);

	//sound
	broadcast_sample(SAMPLE_CTF_GAMEOVER);

	// zero teamscores
	world.flag[0].score = 0;
	world.flag[1].score = 0;
	ctf_update_teamscore(0);
	ctf_update_teamscore(1);

	// return all flags
	ctf_return_flag(0);
	ctf_return_flag(1);

	// reset map start time
	map_start_time = frame;

	// zero all player frags and kill them
	for (i=0;i<maxplayers;i++)
	if (player[i].used)
	{
		//kill - to respawn
		player[i].respawn_to_base = true;
		game_reset_player(i);
		//zero score
		player[i].frags = 0;
	}

	//zero all rockets
	for (i=0;i<MAX_ROCKETS;i++)
		world.rock[i].owner = -1;

	// remove and regenerate powerups
	for (i=0;i<MAX_PICKUPS;i++)
		world.item[i].kind = 0;
	check_pickup_creation(true);

	//update the ADMIN SHELL
	if (shellssock) {
		char lebuf[256]; int count = 0;
		writeLong(lebuf, count, STA_GAME_OVER);
		nlWrite(shellssock, lebuf, count);
	}
}

//respawn a powerup
// put in a screen where there are NO players and NO other powerups
void gameserver_c::respawn_pickup(int p) {

	//char lixox[200];
	//broadcast_message("pickup respawned %s %s\n", itoa(p, lixox, 10), "hoo!");

	//nullify
	world.item[p].kind = 0;

	//find a screen with no players and no other powerups
	int px, py, itemx, itemy, i;
	for (int runaway=300;; --runaway) {
		bool hit = false;
		px = rand() % map.w;
		py = rand() % map.h;

		//check for players if not tried a 100 times yet

		//check players
		if (runaway>200)
			for (i=0; i<maxplayers; i++)
				if (player[i].used && player[i].x==px && player[i].y==py) {
					hit = true;
					break;
				}
		if (hit)
			continue;

		//check for items if not tried 200 times yet

		//check items if no players found
		if (runaway>100)
			for (i=0;i<MAX_PICKUPS;i++)
				if (world.item[i].kind!=0 && world.item[i].px==px && world.item[i].py==py) {
					hit = true;
					break;
				}
		if (hit)
			continue;

		//find a suitable coordinate -- middle square
		itemx = plw / 8 + rand() % (3 * plw / 4);
		itemy = plh / 8 + rand() % (3 * plh / 4);

		//do a check for walls, maybe retrying another screen if hits a wall
		hit = map.fall_on_wall(px, py, itemx - 20, itemy - 20, itemx + 20, itemy + 20);
		if (!hit)
			break;
		if (--runaway < 0) {
			broadcast_message("ITEM SPAWN RUNAWAY");
			return;
		}
	}
	//choose a powerup kind
	//v0.4.4 : roulette kind
	int kind = choose_powerup_kind(); //1 + (rand() % NUMBER_OF_POWERUP_KINDS);  //  % x   = x different items

	//v0.4.0 chance to set deathbringer to something else
	//if (kind == 7)
	//if (rand() % 100 <= 50)
	//kind = 1 + (rand() % NUMBER_OF_POWERUP_KINDS);  //  % x   = x different items

	//alloc powerup
	world.item[p].kind = (NLubyte)kind;

	//world.item[p].respawning = false;
	world.item[p].px = px;
	world.item[p].py = py;
	world.item[p].x = itemx;	//copy from randomized position
	world.item[p].y = itemy;
	//screen-change message of players in the screen the powerup arrived
	//fixes "invisible powerup" problem, I hope
	for (i=0;i<maxplayers;i++)
	if (player[i].used)		//valid
	if (player[i].x == px)	//on the screen of the item
	if (player[i].y == py)
	{
		pickup_c *it = &(world.item[p]);
		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 15);		//"item update"
		writeByte(lebuf, count, (NLubyte)p);	//what item
		writeByte(lebuf, count, (NLubyte)it->kind);	//kind
		writeByte(lebuf, count, (NLubyte)it->px);		//screen
		writeByte(lebuf, count, (NLubyte)it->py);
		writeShort(lebuf, count, (NLushort)it->x);	//pos in screen
		writeShort(lebuf, count, (NLushort)it->y);
		server->send_message(player[i].cid, lebuf, count);
	}
}

int gameserver_c::pups_by_percent(int percentage) const {
	int result = (map.w*map.h*percentage+50) / 100;	// +50 to round properly
	if (result==0 && percentage>0)
		return 1;
	if (result>MAX_PICKUPS)
		return MAX_PICKUPS;
	return result;
}

// verifica powerups unused por jogadores presentes
void gameserver_c::check_pickup_creation(bool instant) {
	int i, pc, ic;

	//count number of players
	pc = 0;
	for (i=0;i<maxplayers;i++)
		if (player[i].used)
			pc++;

	//count number of items
	// TEST SERVER FUK : change "if" to :    if (player[i].used)
	ic = 0;
	for (i=0;i<MAX_PICKUPS;i++)
		if (world.item[i].kind != 0)	//0=unused 255=respawning 1..6(?)=spawned/kind
			ic++;

	int real_min = pups_min_percentage?pups_by_percent(pups_min):pups_min;
	int real_max = pups_max_percentage?pups_by_percent(pups_max):pups_max;
	if (pc > real_min)
		real_min = pc;
	if (real_min > real_max)
		real_min = real_max;
	if (ic >= real_min)
		return;
	//while number of players > number of pickups: create a pickup and ic++
	for (i=0; i<MAX_PICKUPS; i++)
		if (world.item[i].kind == 0) {
			world.item[i].kind = 255;
			if (instant)
				respawn_pickup(i);
			else
				world.item[i].respawn_time = get_time() + pups_respawn_time;
			if (++ic>=real_min)
				break;
		}
}

// player i touches a pickup p!
void gameserver_c::game_touch_pickup(int p, int pk) {

	pickup_c *it = &world.item[pk];

	//send "item removed" message to all players on the current screen
	//
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, 16);		//"item removed"
	writeByte(lebuf, count, (NLubyte)pk);	//what item id
	//server->send_message(player[j].cid, lebuf, count);
	broadcast_screen_message(it->px, it->py, lebuf, count);

	//player picked it! make fx
	//shield
	if (it->kind == 1) {

		player[p].item_shield = true;

		//increase health to minimum of 100
		if (player[p].health < 100)
			player[p].health = 100;		//full health

		//increase energy +100
		if (player[p].energy < 200) {
			player[p].energy += 100;
			if (player[p].energy > 200)
				player[p].energy = 200;
		}

		broadcast_screen_sample(p, SAMPLE_SHIELD_PICKUP);
	}
	//boots
	else if (it->kind == 2) {

		double itemTime=player[p].item_speed_time-get_time();
		if (!player[p].item_speed || itemTime<0)
			itemTime = 0;
		itemTime += pup_add_time;
		if (itemTime > pup_max_time)
			itemTime = pup_max_time;

		player[p].item_speed = true;
		player[p].item_speed_time = get_time() + itemTime;

		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 17);		//powerup time indicator
		writeByte(lebuf, count, it->kind);		//what kind
		writeShort(lebuf, count, (NLushort)itemTime);		//time
		server->send_message(player[p].cid, lebuf, count);

		broadcast_screen_sample(p, SAMPLE_BOOTS_ON);
	}
	//helm
	else if (it->kind == 3) {

		double itemTime=player[p].item_helm_time-get_time();
		if (!player[p].item_helm || itemTime<0)
			itemTime = 0;
		itemTime += pup_add_time;
		if (itemTime > pup_max_time)
			itemTime = pup_max_time;

		player[p].item_helm = 1;		//invis maximo de inicio
		player[p].item_helm_time = get_time() + itemTime;

		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 17);		//powerup time indicator
		writeByte(lebuf, count, it->kind);		//what kind
		writeShort(lebuf, count, (NLushort)itemTime);		//time
		server->send_message(player[p].cid, lebuf, count);

		broadcast_screen_sample(p, SAMPLE_HELM_ON);
	}
	//quad
	else if (it->kind == 4) {

		double itemTime=player[p].item_quad_time-get_time();
		if (!player[p].item_quad || itemTime<0)
			itemTime = 0;
		itemTime += pup_add_time;
		if (itemTime > pup_max_time)
			itemTime = pup_max_time;

		player[p].item_quad = true;
		player[p].item_quad_time = get_time() + itemTime;

		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 17);		//powerup time indicator
		writeByte(lebuf, count, it->kind);		//what kind
		writeShort(lebuf, count, (NLushort)itemTime);		//time
		server->send_message(player[p].cid, lebuf, count);

		broadcast_screen_sample(p, SAMPLE_QUAD_ON);
	}
	//weapon
	else if (it->kind == 5) {

		if (player[p].weapon < 8)	// test for max (shots=weapon+1, entao p/ shots max 9, weapon max = 8)
			player[p].weapon++;	//increase weapon power

		//increase energy +100
		if (player[p].energy < 200) {
			player[p].energy += 100;
			if (player[p].energy > 200)
				player[p].energy = 200;
		}

		//notify player weapon power change
		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 18);		//player power change
		writeByte(lebuf, count, ((NLubyte)player[p].weapon) );
		server->send_message(player[p].cid, lebuf, count);

		broadcast_screen_sample(p, SAMPLE_WEAPON_UP);
	}
	//megahealth
	else if (it->kind == 6) {

		//megabonus!
		player[p].megabonus += 160;

		//increase energy +100, upto 300
		//player[p].energy += 100;
		//if (player[p].energy > 300)
		//	player[p].energy = 300;

		//increase health +100, upto 300
		//player[p].health += 100;
		//if (player[p].health > 300)
		//	player[p].health = 300;

		broadcast_screen_sample(p, SAMPLE_MEGAHEALTH);
	}
	//deathbringer
	else if (it->kind == 7) {
		if (pup_deathbringer_switch)
			player[p].item_deathbringer = !player[p].item_deathbringer;
		else
			player[p].item_deathbringer = true;

		broadcast_screen_sample(p, SAMPLE_GETDEATHBRINGER);
	}

	// unused item
	it->kind = 0;

	// check pickup creation
	check_pickup_creation(false);
}

//game player screen changed
// --> send any pickups on screen
void gameserver_c::game_player_screen_change(int p) {

	//check for new pickups visible
	for (int i=0;i<MAX_PICKUPS;i++) {
		pickup_c *it = &world.item[i];
		if (it->kind)		// item exists
		if (it->kind != 255)		// item not respawning
		if (it->px == player[p].x) // item on screen that player is entering
		if (it->py == player[p].y) {

			#ifndef SV_NO_PUP_SWITCHING
			//broadcast_message("sending powerup update\n");

			//v0.1.2: PRIMEIRO verifica se tem mais alguem nessa tela. se nao
			//  tiver, verifica se nao seria interessante mudar o "kind" do item
			//muda WHILE item alvo eh powerup cujo time do jogador eh > 30
			bool temjog = false;
			for (int j=0;j<maxplayers;j++)
			if (j != p)
			if (player[j].used)	//valido
			if (player[j].x == player[p].x)	// mesma tela
			if (player[j].y == player[p].y) {
				temjog = true;
				break;
			}

			int original = it->kind;

			if (!temjog) {
				bool non_satisfactory;
				do {
					non_satisfactory = false;

					if ((it->kind == 1) && (player[p].health >= 80) && (player[p].energy >= 30) && (player[p].item_shield))//hide if just using as extra battery or not seriously injured
						non_satisfactory = true;
					else if ((it->kind == 2) && (player[p].item_speed) && (player[p].item_speed_time - get_time() > 40.0))
						non_satisfactory = true;
					else if ((it->kind == 3) && (player[p].item_helm) && (player[p].item_helm_time - get_time() > 40.0))
						non_satisfactory = true;
					else if ((it->kind == 4) && (player[p].item_quad) && (player[p].item_quad_time - get_time() > 40.0))
						non_satisfactory = true;
					else if ((it->kind == 6) && (player[p].health + (rand() % 70) >= 300))//if 300 non-satisf. but if >200, less chance of seeing another one
						non_satisfactory = true;
					else if ((it->kind == 7) && (player[p].item_deathbringer))
						non_satisfactory = true;

					//re-choose item type
					if (non_satisfactory)
						it->kind = (NLubyte)choose_powerup_kind();

				} while (non_satisfactory);

				//if loop choosed "weapon" powerup (item 5) but you are at maximum, then keep the original choice
				if ((it->kind == 5) && (player[p].weapon >= 8))
					it->kind = (NLubyte)original;
			}
			#endif	// SV_NO_PUP_SWITCHING

			//send a "item on the screen" message
			char lebuf[256]; int count = 0;
			writeByte(lebuf, count, 15);		//"item update"
			writeByte(lebuf, count, (NLubyte)i);	//what item
			writeByte(lebuf, count, (NLubyte)it->kind);	//kind
			writeByte(lebuf, count, (NLubyte)it->px);		//screen
			writeByte(lebuf, count, (NLubyte)it->py);
			writeShort(lebuf, count, (NLushort)it->x);	//pos in screen
			writeShort(lebuf, count, (NLushort)it->y);
			server->send_message(player[p].cid, lebuf, count);
		}
	}
}


//broadcast team message
void gameserver_c::broadcast_team_message(int team, char *text) {

	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, 2);
	writeString(lebuf, count, text);

	// send message only to teammates
	for (int i=0;i<maxplayers;i++)
	if (player[i].used)
	if ((i/TSIZE) == team) {
		server->send_message(player[i].cid, lebuf, count);
	}

	//send to the admin shell
	if (shellssock) {
		count = 0;
		writeLong(lebuf, count, STA_GAME_TEXT);
		writeString(lebuf, count, text);
		nlWrite(shellssock, lebuf, count);
	}
}

//broadcast message to all players in one screen
void gameserver_c::broadcast_screen_message(int px, int py, char *lebuf, int count) {

	for (int j=0;j<maxplayers;j++)
	if (player[j].used)
	if (player[j].x == px)
	if (player[j].y == py)
		server->send_message(player[j].cid, lebuf, count); //send the message
}

// V0.4.9 : broadcast message with varargs
void gameserver_c::bprintf(const char *fs, ...) {
	//vsprintf...
	va_list argptr;
	char msg[16384];
	va_start(argptr, fs);
	vsprintf(msg, fs, argptr);
	va_end (argptr);

	//broadcast it
	broadcast_message(msg);
}
void gameserver_c::plprintf(int pid, const char* fmt, ...) {	// bprintf for a single player
	char buf[16385];
	buf[0]=2;	// server text
	va_list argptr;
	va_start(argptr, fmt);
	vsprintf(buf+1, fmt, argptr);
	va_end(argptr);
	server->send_message(player[pid].cid, buf, strlen(buf)+1);
}

//send a single message player-printf
void gameserver_c::player_message(int pid, const char *text) {
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, 2);
	writeString(lebuf, count, text);
	if (player[pid].used)
		server->send_message(player[pid].cid, lebuf, count);
}

//broadcast message to all
void gameserver_c::broadcast_message(const char *text) {
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, 2);
	writeString(lebuf, count, text);
	for (int i=0;i<maxplayers;i++)
		if (player[i].used)
			server->send_message(player[i].cid, lebuf, count);
	//send to the admin shell
	if (shellssock) {
		count = 0;
		writeLong(lebuf, count, STA_GAME_TEXT);
		writeString(lebuf, count, text);
		nlWrite(shellssock, lebuf, count);
	}
}

// ---- GAME MOD -------

void gameserver_c::load_game_mod() {

	char dest[WHERE_PATH_SIZE];
	append_filename(dest, wheregamedir, "gamemod.txt", WHERE_PATH_SIZE);
	FILE *fmod = fopen(dest, "r");
	if (fmod) {

		char s[1024];

		bool command = true;
		int cmd = 0;

		LOG1("loading game mod from '%s'...\n", dest);

		while (1) {
			//char* fgets(char* s, int n, FILE* stream);
			//Copies characters from (input) stream stream to s,
			// stopping when n-1 characters copied, newline copied, end-of-file reached or error occurs.
			//If no error, s is NUL-terminated. Returns NULL on end-of-file or error, s otherwise.

			if (fgets((char*)s, 1024, fmod) == 0)
				break;

			s[ strlen(s) - 1] = 0;	//erase \n

			//LOG1("modline '%s'\n", s);

			if (s[0] == '\0')	//skip blank
				continue;

			if (s[0] == ';') // skip comment
				continue;

			if (command) {
				if (!strcmp(s, "friction")) cmd = 1;
				else if (!strcmp(s, "accel")) cmd = 2;
				else if (!strcmp(s, "maxspeed")) cmd = 3;
				else if (!strcmp(s, "friction_run")) cmd = 4;
				else if (!strcmp(s, "accel_run")) cmd = 5;
				else if (!strcmp(s, "maxspeed_run")) cmd = 6;
				else if (!strcmp(s, "friction_turbo")) cmd = 7;
				else if (!strcmp(s, "accel_turbo")) cmd = 8;
				else if (!strcmp(s, "maxspeed_turbo")) cmd = 9;
				else if (!strcmp(s, "friction_turborun")) cmd = 10;
				else if (!strcmp(s, "accel_turborun")) cmd = 11;
				else if (!strcmp(s, "maxspeed_turborun")) cmd = 12;
				else if (!strcmp(s, "flag_penalty")) cmd = 13;
				else if (!strcmp(s, "map")) cmd = 14;
				else if (!strcmp(s, "pups_min")) cmd = 15;
				else if (!strcmp(s, "pups_max")) cmd = 28;
				else if (!strcmp(s, "pups_respawn_time")) cmd = 16;
				else if (!strcmp(s, "pup_chance_shield")) cmd = 17;
				else if (!strcmp(s, "pup_chance_turbo")) cmd = 18;
				else if (!strcmp(s, "pup_chance_shadow")) cmd = 19;
				else if (!strcmp(s, "pup_chance_power")) cmd = 20;
				else if (!strcmp(s, "pup_chance_weapon")) cmd = 21;
				else if (!strcmp(s, "pup_chance_megahealth")) cmd = 22;
				else if (!strcmp(s, "pup_chance_deathbringer")) cmd = 23;
				else if (!strcmp(s, "time_limit")) cmd = 24;
				else if (!strcmp(s, "capture_limit")) cmd = 25;
				else if (!strcmp(s, "welcome_message")) cmd = 26;
				else if (!strcmp(s, "info_message")) cmd = 27;
				// 28 = pups_max
				else if (!strcmp(s, "pup_add_time")) cmd = 29;
				else if (!strcmp(s, "pup_max_time")) cmd = 30;
				else if (!strcmp(s, "pup_deathbringer_switch")) cmd = 31;
				else if (!strcmp(s, "random_maprot")) cmd = 32;
				else if (!strcmp(s, "vote_block_time")) cmd = 33;
				else if (!strcmp(s, "respawn_time")) cmd = 34;
				else if (!strcmp(s, "waiting_time_deathbringer")) cmd = 35;
				else if (!strcmp(s, "pup_shadow_invisibility")) cmd = 36;
				else if (!strcmp(s, "sayadmin_enabled")) cmd = 37;
				else if (!strcmp(s, "sayadmin_comment")) cmd = 38;
				else {
					LOG1("*** Bad command in gamemod: %s\n", s);
					cmd = 0;
				}

				//LOG1("is command %i\n", cmd);
			}
			else {
				double val = 1.0;
				int ival = 1;

				sscanf(s, "%lf", &val);
				sscanf(s, "%i", &ival);

				//LOG3("set cmd %i value to %f from '%s'\n", cmd, val, s);
				//LOG3("set cmd %i value to %i from '%s'\n", cmd, ival, s);

				if (cmd == 1) {
					svp_fric = val;
				}
				else if (cmd == 2) {
					svp_accel = val;
				}
				else if (cmd == 3) {
					svp_maxspeed = val;
				}
				else if (cmd == 4) {
					svp_fric_run = val;
				}
				else if (cmd == 5) {
					svp_accel_run = val;
				}
				else if (cmd == 6) {
					svp_maxspeed_run = val;
				}
				else if (cmd == 7) {
					svp_fric_turbo = val;
				}
				else if (cmd == 8) {
					svp_accel_turbo = val;
				}
				else if (cmd == 9) {
					svp_maxspeed_turbo = val;
				}
				else if (cmd == 10) {
					svp_fric_turborun = val;
				}
				else if (cmd == 11) {
					svp_accel_turborun = val;
				}
				else if (cmd == 12) {
					svp_maxspeed_turborun = val;
				}
				else if (cmd == 13) {
					svp_flag_penalty = val;
				}
				else if (cmd == 14) {
					MapInfo mi;
					if (mi.load(s)) {
						maprot.push_back(mi);
						LOG1("Added '%s' to map rotation\n", s);
					}
					else
						LOG1("Can't add '%s' to map rotation\n", s);
				}
				else if (cmd == 15) {
					if (strchr(s, '%')) {
						if (ival >= 0) {
							pups_min = ival;
							pups_min_percentage = true;
						}
						else LOG1("Can't set pups_min to %d%%\n", ival);
					}
					else if (ival >= 0 && ival <= MAX_PICKUPS) {
						pups_min = ival;
						pups_min_percentage = false;
					}
					else LOG1("Can't set pups_min to %d\n", ival);
				}
				else if (cmd == 28) {
					if (strchr(s, '%')) {
						if (ival >= 0) {
							pups_max = ival;
							pups_max_percentage = true;
						}
						else LOG1("Can't set pups_max to %d%%\n", ival);
					}
					else if (ival>=0 && ival<=MAX_PICKUPS) {
						pups_max = ival;
						pups_max_percentage = false;
					}
					else LOG1("Can't set pups_max to %d\n", ival);
				}
				else if (cmd == 16) {
					if (ival >= 0 && ival <= 60)
						pups_respawn_time = ival;
					else LOG1("Can't set pups_respawn_time to %d\n", ival);
				}
				else if (cmd == 17) {
					pup_chance_shield = ival;
				}
				else if (cmd == 18) {
					pup_chance_turbo = ival;
				}
				else if (cmd == 19) {
					pup_chance_shadow = ival;
				}
				else if (cmd == 20) {
					pup_chance_power = ival;
				}
				else if (cmd == 21) {
					pup_chance_weapon = ival;
				}
				else if (cmd == 22) {
					pup_chance_megahealth = ival;
				}
				else if (cmd == 23) {
					pup_chance_deathbringer = ival;
				}
				else if (cmd == 24) {
					if (ival >= 0)
						time_limit = 60 * 10 * ival; // convert minutes to frames
				}
				else if (cmd == 25) {
					if (ival > 0)
						capture_limit = ival;
				}
				else if (cmd == 26) {
					welcome_message.push_back(s);
				}
				else if (cmd == 27) {
					info_message.push_back(s);
				}
				else if (cmd == 29) {
					if (ival > 0 && ival<1000)
						pup_add_time = ival;
					else LOG1("Can't set pup_add_time to %d\n", ival);
				}
				else if (cmd == 30) {
					if (ival > 0 && ival<1000)
						pup_max_time = ival;
					else LOG1("Can't set pup_max_time to %d\n", ival);
				}
				else if (cmd == 31) {
					if (ival == 0 || ival == 1)
						pup_deathbringer_switch = ival==1?true:false;
					else LOG1("Can't set pup_deathbringer_switch to %d\n", ival);
				}
				else if (cmd == 32) {
					if (ival == 0 || ival == 1)
						random_maprot = ival==1?true:false;
					else LOG1("Can't set random_maprot to %d\n", ival);
				}
				else if (cmd == 33) {
					if (ival >= 0)
						vote_block_time = 60*10*ival;	// minutes to frames
				}
				else if (cmd == 34) {
					if (val >= 0)
						respawn_time = val;
				}
				else if (cmd == 35) {
					if (val >= 0)
						waiting_time_deathbringer = val;
				}
				else if (cmd == 36) {
					if (ival == 0 || ival == 1)
						shadow_minimum = ival==1?1:SV_SHADOW_MINIMUM_NORMAL;
					else LOG1("Can't set pup_shadow_invisibility to %d\n", ival);
				}
				else if (cmd == 37) {
					if (ival == 0 || ival == 1)
						sayadmin_enabled = ival==1?true:false;
					else LOG1("Can't set sayadmin_enabled to %d\n", ival);
				}
				else if (cmd == 38) {
					sayadmin_comment = s;
				}
			}

			//parameter
			command = !command;
		}

		LOG("END OF MOD FILE.\n");

		fclose(fmod);
	}
}

//load a map from the rotation list
bool gameserver_c::load_rotation_map(int pos) {
	bool ok = load_map(SERVER_MAPS_DIR, maprot[pos].file, &map);
	if (!ok)
		return false;
	LOG2("load_rotation_map() maprot[%i] = '%s'\n", pos, maprot[pos].file.c_str());
	return true;
}

//send map change message to a player
//reason: NEXTMAP_CAPTURE_LIMIT ou NEXTMAP_VOTE_EXIT
void gameserver_c::send_map_change_message(int pid, int reason, const char* mapname) {

	char lebuf[256];
	int count = 0;
	writeByte(lebuf, count, 20);	// 20 = map change

	writeByte(lebuf, count, 2);		// 2 = custom map message
	writeShort(lebuf, count, map.crc);
	writeString(lebuf, count, mapname);
	server->send_message(player[pid].cid, lebuf, count);

	//VERY IMPORTANT: flags the player as "awaiting map load" - client must confirm map to proceed
	player[pid].awaiting_client_ready = true;

	//send a show gameover plaque message, if that is the case
	if (reason != NEXTMAP_NONE) {

		count = 0;
		writeByte(lebuf, count, 24);	// 24 = gameover plaque
		writeByte(lebuf, count, ((NLubyte)reason));		//capture limit plaque or vote exit plaque
		if ((reason == NEXTMAP_CAPTURE_LIMIT) || (reason == NEXTMAP_VOTE_EXIT)) {
			//informacoes para mostrar apos o jogo (time vencedor, most valuable player, etc.)

			writeByte(lebuf, count, (NLubyte)world.flag[0].score);	//RED team final score
			writeByte(lebuf, count, (NLubyte)world.flag[1].score);	//BLUE team final score
		}
		server->send_message(player[pid].cid, lebuf, count);

		//important: server is showing gameover plaque. nobody should move or receive world frames
		gameover = true;
		gameover_time = get_time() + 5.0;		//5 secods timeout for gameover plaque
	}
}

bool gameserver_c::server_next_map(int reason) {

	//(re)load hostname
	reload_hostname();

	assert(!maprot.empty());

	#ifdef SV_CONSOLE
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
		if (winners.size()>1)
			winners.erase(find(winners.begin(), winners.end(), currmap));
		currmap=winners[rand()%winners.size()];
	}
	// clear votes for the current map
	for (int p=0; p<maxplayers; ++p) {
		player[p].want_map_exit=false;
		if (player[p].mapVote==currmap)
			player[p].mapVote=-1;
	}
	maprot[currmap].votes=0;
	#else
	if (++currmap >= (int)maprot.size()) // next map on rotation
		currmap = 0;
	for (int p=0; p<maxplayers; ++p)
		player[p].want_map_exit=false;
	#endif
	last_vote_announce_votes = last_vote_announce_needed = 0;
	next_vote_announce_frame = 0;	// let a new announcement be made as soon as someone votes

	if (!load_rotation_map(currmap))
		return false;

	// notify all players
	for (int i=0;i<maxplayers;i++)
		if (player[i].used)
			send_map_change_message(i, reason, maprot[currmap].file.c_str());

	char lix[256];
	sprintf(lix, "Server changed map to: %s (%i of %i)", maprot[currmap].file.c_str(), currmap+1, maprot.size());
	broadcast_message(lix);

	// reset map start time
	map_start_time = frame;
	return true;
}

//check map exit by vote
void gameserver_c::check_map_exit() {
	int num_for = 0, num_against = 0;
	for (int i=0; i<maxplayers; i++)
		if (player[i].used) {
			if (player[i].want_map_exit)
				num_for++;
			else
				num_against++;
		}

	#ifdef SV_CONSOLE
	// this could be done elsewhere, but this function is called whenever votes change
	for (int m=0; m<(int)maprot.size(); ++m)
		maprot[m].votes=0;
	for (int p=0; p<maxplayers; ++p)
		if (player[p].used && player[p].mapVote!=-1)
			++maprot[player[p].mapVote].votes;
	#endif

	if ((map_start_time+vote_block_time<frame && num_for>num_against) || (num_against==0 && num_for)) {
		server_next_map(NEXTMAP_VOTE_EXIT);	// ignore return value
		ctf_game_restart();
	}
}

//----- THE REST  ----------------

bool gameserver_c::reset_settings() {
	set_default_physics();

	//default configuration
	//DEFAULT POWERUP CONFIG
	pup_add_time = 60;
	pup_max_time = 180;

	pups_min = 6;
	pups_min_percentage = false;
	pups_max = MAX_PICKUPS;
	pups_max_percentage = false;
	pups_respawn_time = 25;

	pup_chance_shield = 16;
	pup_chance_turbo = 14;
	pup_chance_shadow = 14;
	pup_chance_power = 14;
	pup_chance_weapon = 18;
	pup_chance_megahealth = 13;
	pup_chance_deathbringer = 11;

	pup_deathbringer_switch = true;
	shadow_minimum = SV_SHADOW_MINIMUM_NORMAL;

	respawn_time = 2.0;
	waiting_time_deathbringer = 4.0;

	// default time and capture limits
	time_limit = 0;	// no time limit
	capture_limit = 8;

	vote_block_time = 0;	// no limit

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
	if (maprot.size() == 0) {
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

	if (maprot.size() == 0) {
		LOG("No maps for rotation\n");
		return false;
	}

	if (random_maprot)
		random_shuffle(maprot.begin(), maprot.end());

	return true;
}

//start server
bool gameserver_c::start(int target_maxplayers) {

	#ifdef SV_NAME_AUTHORIZATION
	authorizations.load();
	#endif

	int i;

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
	for (i=0;i<MAX_PLAYERS;i++)
		client[i].reset();

	//not showing gameover plaque to clients
	gameover = false;

	ping_send_counter = 0;
	ping_send_client = 0;

	//reset fslavesocks
	for (int ss=0;ss<MAX_PLAYERS;ss++)
		fslavesock[ss] = NL_INVALID;			//inicializa
	file_threads_quit = false;	//not yet

	// reset players
	//players_present = 0;
	player_count = 0;
	for (i=0;i<MAX_PLAYERS;i++) {
		player[i].used = false;
		player[i].id = i;
		player[i].name[0]=0;
	}

	if (!reset_settings())
		return false;

	if (!load_rotation_map(currmap))
		return false;

	// start server
	server = new_server_c();

	server->set_callback(SFUNC_CLIENT_HELLO, sfunc_client_hello);
	server->set_callback(SFUNC_CLIENT_CONNECTED, sfunc_client_connected);
	server->set_callback(SFUNC_CLIENT_DISCONNECTED, sfunc_client_disconnected);
	server->set_callback(SFUNC_CLIENT_LAG_STATUS, sfunc_client_lag_status);
	server->set_callback(SFUNC_CLIENT_DATA, sfunc_client_data);
	server->set_callback(SFUNC_CLIENT_PING_RESULT, sfunc_client_ping_result);
	server->set_client_timeout(5, 10);

	if (!server->start(port))
		return false;

	//v0.4.4 reset master jobs count
	mjob_count = 0;
	mjob_exit = false;				//flag for all pending master jobs to quit now
	mjob_fastretry = false;		//flag for all pending master jobs to stop waiting and retry immediately

	//v0.4.2 : calc TCP PORT

	NLboolean ok;

	//v0.4.4 : DO NOT open tcp download port if (-notcp) enabled
	//
	if (!no_tcp_download) {

		int tcp_port = 24999 - (port - 25000);

		//open socket for accepting connections to TCP file download requests
		//nlEnable(NL_BLOCKING_IO);
		filesock = nlOpen((NLushort)tcp_port, NL_RELIABLE);		//v0.4.2 : custom TCP port
		//nlDisable(NL_BLOCKING_IO);
		if (filesock == NL_INVALID) {
			LOG1("CAN'T OPEN THE TCP SOCKET ON PORT %i\n", tcp_port);
			return false; //oh no
		}
		ok = nlListen(filesock);
		if (!ok) {
			LOG("CAN'T SET SOCKET TO LISTEN MODE\n");
			return false; //oh no
		}

		//start TCP file server master thread
		pthread_create(&server_filemaster_thread, 0, thread_filemaster_f, (void *)0);
	}

	//start TCP thread for talking with master server
	if (!privateserver) {
		master_talk_time = get_time();
		msock = NL_INVALID;		//not opened
		master_talk_time = get_time() + DELAY_TO_REPORT_SERVER;	//give it a break
		master_never_talked = true;		//not talked yet
		pthread_create(&mthread, 0, thread_mastertalker_f, (void *)0);
	}

	//shell socket
	//v0.4.2 : new port
	int tcp_shell_port = 24500 + (port - 25000);

	shellmsock = nlOpen((NLushort)tcp_shell_port, NL_RELIABLE);
	if (shellmsock == NL_INVALID) {
		LOG1("CAN'T OPEN THE SHELL SOCKET ON PORT %i\n", tcp_shell_port);
		return false; //oh no
	}
	ok = nlListen(shellmsock);
	if (!ok) {
		LOG("CAN'T SET SHELL SOCKET TO LISTEN MODE\n");
		return false; //oh no
	}
	shellssock = NL_INVALID;

	//start TCP shell master and slave threads
	pthread_create(&shellmthread, 0, thread_shellmaster_f, (void *)0);
	pthread_create(&shellsthread, 0, thread_shellslave_f, (void *)0);

	// reset game
	ctf_game_restart();

	//default serverinfo with reload hostname
	reload_hostname();

	return true;
}

//reload hostname
void gameserver_c::reload_hostname() {
	char dest[WHERE_PATH_SIZE];
	append_filename(dest, wheregamedir, "hostname.txt", WHERE_PATH_SIZE);
	FILE *cfg = fopen(dest, "r");
	if (cfg) {
		fgets(hostname, 256, cfg);
		hostname[ strlen(hostname) - 1 ] = 0;  //replace newline with \0
		LOG1("HOSTNAME IS = '%s'\n", hostname);

		//v0.4.7: ad name (FIXME MAKEIT)
		//fgets(hostadname, 256, cfg);
		//hostadname[ strlen(hostadname) - 1 ] = 0;  //replace newline with \0
		//LOG1("HOST AD NAME IS = '%s'\n", hostadname);

		fclose(cfg);
	}
	else
		strcpy(hostname, "ANNONYMOUS HOST");

	//update serverinfo
	update_serverinfo();
}

//update serverinfo
void gameserver_c::update_serverinfo() {

	//v0.4.8 UGLY FIX : count all players again, check for discrepancy
	int pc = 0;
	for (int i=0;i<maxplayers;i++)
		if (player[i].used == true)
			pc++;
	if (pc != player_count) { //debug
		LOG2("** update_serverinfo() BUG FOUND: PC=%i player_count=%i !\n", pc, player_count);
	}
	//force player_count
	player_count = pc;

	char sinfo[1024];
	sprintf(sinfo, "%2i %7s/%s", player_count, GAME_VERSION, hostname);
	server->set_server_info(sinfo);
}

int gameserver_c::client_connected(int id) {

	//2TEAM: check wich team to put player
	int t1, t2, targ;
	t1 = 0;		//red team count
	t2 = 0;		//blue team count
	int i;
	for (i=0;i<maxplayers;i++)
		if (player[i].used) {
			if (i/TSIZE == 0)
				t1++;
			else
				t2++;
		}

	//put on red team, blue team, or randomize if same # of players in both teams
	if (t1 < t2)
		targ = 0;
	else if (t1 > t2)
		targ = TSIZE;
	else {
		//refresh team modifiers
		refresh_team_score_modifiers();

		//this means team 0 is LACKING
		if (team_smul[0] > team_smul[1])
			targ = 0;
		//this means team 1 is LACKING
		else if (team_smul[1] > team_smul[0])
			targ = 8;
		// 0 or 8 -- choose a random one
		else
			targ = (rand()%2) * TSIZE;
	}

	//alloc new player : scans only slots of the team (targ...targ + 7)
	int myself = -1;

	for (i=targ;i<(targ+TSIZE);i++)
	{
		if (!player[i].used)
		{
			// add player to players_present
			//players_present = players_present | (1 << i);

			// init player
			int cid;
			cid=id;
			ctop[cid]=i;

			player[i].clear(true, i, cid, SERVER_DEFAULT_PLAYER_NAME);

			myself = i;

			// spawn player
			player[i].respawn_to_base = true;
			game_respawn_player(i);

			//reset keypresses
			world.hero[i].l = 0;
			world.hero[i].r = 0;
			world.hero[i].u = 0;
			world.hero[i].d = 0;

			//check pickup creation
			check_pickup_creation(false);

			break;
		}
	}

	// internal error can be detected if no player is free (used == false)
	//0.4.7: normal behavior (bots...)
	if (myself == -1) {
		//LOG("ERROR: BAD BAD BAD INTERNAL GAMESERVER ERROR myself == -1 !!! client_connected()...\n");
		return myself;	//-1...
	}

	//CONNECT OK: another one...
	player_count++;

	// se o player_count ficou == 2, reseta partida
	//
	if (player_count == 2)
		ctf_game_restart();

	//char lelix[222];
	//sprintf(lelix, "PCOUNT = %i BCOUNT = %i\n", player_count, bot_count);
	//broadcast_message(lelix);

	char lebuf[256]; int count;

	//**** V0.4.4 : init oneclient_c ****
	client[id].reset();
	client[id].token_have = false;		// no token

	//first update the ADMIN SHELL
	if (shellssock) {
		count = 0;
		writeLong(lebuf, count, STA_PLAYER_CONNECTED);
		writeLong(lebuf, count, player[myself].cid);
		nlWrite(shellssock, lebuf, count);
	}

	//update the player with world information
	//	- who is he (player #)

	send_me_packet(myself);

	// - world ctf flags information
	ctf_net_flag_status(id, 0);
	ctf_net_flag_status(id, 1);

	// MAP NAME+CRC !!! VERY IMPORTANT
	//
	send_map_change_message(myself, NEXTMAP_NONE, maprot[currmap].file.c_str());

	// - all other player's names
	// - all other player's frags

	for (i=0;i<maxplayers;i++)
		if (player[i].used)
		if (i != myself) {

			//player name update
			send_player_name_update(id, i);
			//count = 0;
			//writeByte(lebuf, count, 1);	// "1" = player name update
			//writeByte(lebuf, count, i);		// what player id
			//player[i].name[15]=0;		// force trunc name at 15 chars (paranoia)
			//writeString(lebuf, count, player[i].name);
			//server->send_message(id, lebuf, count);

			//frags update
			count = 0;
			writeByte(lebuf, count, 4);	//"4" = frags update
			writeByte(lebuf, count, i);		// what player id
			writeLong(lebuf, count, player[i].frags);
			server->send_message(id, lebuf, count);

			//crap update
			send_player_crap_update(id, i);
		}

	for (vector<string>::const_iterator line=welcome_message.begin(); line!=welcome_message.end(); line++)
		player[myself].add_to_queue(*line);

	//check for team changes
	//
	check_team_changes();

	//update serverinfo
	update_serverinfo();

	//ok!
	//
	return myself;
}

//client disconnected (callback function)
//
// mesmas observacoes para client_connect. adicionalmente:
//
void gameserver_c::client_disconnected(int id) {
	if (ctop[id]==-1)
		return;

	int pid;

	//less one...
	player_count--;

	//what player
	pid = ctop[id];

	//first update the ADMIN SHELL
	if (shellssock) {
		char lebuf[256]; int count;
		count = 0;
		writeLong(lebuf, count, STA_PLAYER_DISCONNECTED);
		writeLong(lebuf, count, player[pid].cid);
		nlWrite(shellssock, lebuf, count);
	}

	//broadcast a textual message "Player BLABLA left the game"
	bprintf("@I%s left the game with %i frags", player[pid].name, player[pid].frags);

	//sound
	broadcast_sample(SAMPLE_LEFTGAME);

	//report the latest player achievements to the master server
	client_report_status(id);

	//clear oneclient_c
	client[id].reset();

	//remove player from the game
	game_remove_player(pid);

	check_team_changes();
	check_map_exit();

	//update serverinfo
	update_serverinfo();
}

//client ping result
void gameserver_c::ping_result(int client_id, int ping_time) {
	if (ctop[client_id]==-1)
		return;

	//save result
	player[ ctop[client_id] ].ping = ping_time;
}

//process incoming client data (callback function)
void gameserver_c::incoming_client_data(int id, char *data, int length) {
	(void)length;
	if (ctop[id]==-1)
		return;

	//player id
	int pid = ctop[id];

	//for talk msgs
	char talkmsg[256];

	//1. process client's frame data
	NLubyte keys;
	int count = 0;
	readByte(data, count, keys);
	//update the player's direction keys (accel.vectrs) (and other keys too if needed later)
	hero_t *h;
	h = &(world.hero[pid]);

	h->l = (keys & 1) != 0;
	h->r = (keys & 2) != 0;
	h->u = (keys & 4) != 0;
	h->d = (keys & 8) != 0;
	h->strafe = (keys & 16) != 0;
	h->run = (keys & 32) != 0;

	//if not strafing, update direction
	if (!h->strafe) {
		// left
		if ((h->l) && (!h->r)) {
			if ((h->u) && (!h->d))	// + up
				h->gundir = 5;
			else if ((!h->u) && (h->d)) // + down
				h->gundir = 3;
			else if ((!h->u) && (!h->d)) // !up !down
				h->gundir = 4;
		}
		// right
		else if ((!h->l) && (h->r)) {
			if ((h->u) && (!h->d))	// + up
				h->gundir = 7;
			else if ((!h->u) && (h->d)) // + down
				h->gundir = 1;
			else if ((!h->u) && (!h->d)) // !up !down
				h->gundir = 0;
		}
		//!right !left
		else if ((!h->l) && (!h->r)) {
			if ((h->u) && (!h->d))	// + up
				h->gundir = 6;
			else if ((!h->u) && (h->d)) // + down
				h->gundir = 2;
		}
	}

	//2. process messages
	char *msg;
	int msglen;
	do {
		msg = server->receive_message(id, &msglen);
		if (msg != 0) {
			// process a client's message
			//
			int count = 0;
			NLubyte code;
			readByte(msg, count, code);
			if (code == 1) {

				//read the name
				char tempname[256];
				readString(msg, count, tempname); //name update request

				//v0.4.9: IF SAME NAME: ignore it. if not, keep going
				if (strcmp(tempname, player[pid].name))
				//name change flooding protection
				if (get_time() >= player[pid].waitnametime) {

					//FLUSH PENDING REPORTS TO MASTER IF token_have/token_valid !!!
					client_report_status(id);

					//name changed -- this means that the player is NOT REGISTERED
					//  anymore for recording statistics
					client[id].token_have = false;

					//need to broadcast the player's crap to remove eventual '*' and stuff
					broadcast_player_crap( pid );

					//check if it's the first name information from client. then it
					// must have just entered the game
					bool entered_game = !strcmp(SERVER_DEFAULT_PLAYER_NAME, player[pid].name);

					// log
					//LOG3("client %i player %i '%s' renamed to", id, pid, player[pid].name);

					//readString(msg, count, player[pid].name); //name update request
					strcpy(player[pid].name, "(invalid name)");
					if (strpbrk(tempname, "%@")!=NULL)
						player[pid].add_to_queue("@WSorry, this server doesn't accept % or @ in a name");
					else if (strspnp(tempname, " ")==NULL)
						player[pid].add_to_queue("@WPlease enter a name");
					else {
						#ifdef SV_NAME_AUTHORIZATION
						int nid=authorizations.identifyName(tempname);
						if (nid==-1 || authorizations.authorize(nid, server->get_client_address(id)))
							strcpy(player[pid].name, tempname);
						else {
							player[pid].queue_printf("@WThe name %s is reserved on this server.", authorizations.getName(nid).c_str());
							player[pid].queue_printf("@ITo authorize, type /auth %s,pass where pass is your password.", authorizations.getName(nid).c_str());
						}
						#else
						strcpy(player[pid].name, tempname);
						#endif
					}

					//LOG1("'%s'\n", player[pid].name);

					//send entered-game message
					if (entered_game) {
						bprintf("@I%s entered the game", player[pid].name);
						//sound
						broadcast_sample(SAMPLE_ENTERGAME);
					}
					// next time allowed to change name
					player[pid].waitnametime = get_time() + 1.0;

					//broadcast the new player's name
					broadcast_player_name(pid);

					//update the ADMIN SHELL
					if (shellssock) {
						char lebuf[256]; int count = 0;
						writeLong(lebuf, count, STA_PLAYER_NAME_UPDATE);
						writeLong(lebuf, count, player[pid].cid);
						writeString(lebuf, count, player[pid].name);
						if (entered_game) {
							writeLong(lebuf, count, STA_PLAYER_IP);
							writeLong(lebuf, count, player[pid].cid);
							char addrBuf[50];
							NLaddress addr=server->get_client_address(player[pid].cid);
							nlSetAddrPort(&addr, 0);
							nlAddrToString(&addr, addrBuf);
							writeString(lebuf, count, addrBuf);
						}
						nlWrite(shellssock, lebuf, count);
					}
				}
			}
			//chat!
			else if (code == 2) {
				const char* sbuf=msg+1;
				#ifdef SV_CONSOLE
				// handle 'console' commands
				if (player[pid].delayedMessages.size()>2) {
					player[pid].delayedMessages.clear();
					plprintf(pid, "@I(rest of message cancelled)");
				}
				player[pid].reset_message_queue_timing();
				if (sbuf[0]=='/') {
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
//							                          123456789*123456789*123456789*123456789*123456789*123456789*123456789*123456789*
						player[pid].queue_printf("@TConsole commands available on this server:");
						player[pid].queue_printf("/help       this screen");
						if (!info_message.empty())
							player[pid].queue_printf("/info       information about this server");
						player[pid].queue_printf("/config     current server configuration");
						player[pid].queue_printf("/stats      see your stats");
						player[pid].queue_printf("/mapinfo n  information about map n (default: current map)");
						player[pid].queue_printf("/votemap n  vote for the next map to be n (default: list maps and votes)");
						player[pid].queue_printf("/time       check server uptime, current map time and time left on the map");
						if (sayadmin_enabled) {
							ostringstream ostr;
							ostr << "/sayadmin   send a message to the server admin";
							if (sayadmin_comment.length())
								ostr << " (" << sayadmin_comment << ')';
							player[pid].add_to_queue(ostr.str());
						}
					}
					else if (!strcmp(cbuf, "info") && !info_message.empty()) {
						for (vector<string>::const_iterator line=info_message.begin(); line!=info_message.end(); line++)
							player[pid].add_to_queue(*line);
						player[pid].add_to_queue("type /config to see current server settings");
					}
					else if (!strcmp(cbuf, "config")) {
						player[pid].queue_printf("@TCurrent server settings:");
						player[pid].queue_printf("- Flag capture limit: %i", capture_limit);
						if (time_limit == 0)
							player[pid].queue_printf("- No map time limit.");
						else
							player[pid].queue_printf("- Map time limit: %i min", time_limit / 10 / 60);
						if (pup_max_time > pup_add_time)
							player[pid].queue_printf("- Power-ups add %d seconds to what's left, with a maximum of %d seconds", pup_add_time, pup_max_time);
						else
							player[pid].queue_printf("- Power-up time is %d seconds", pup_max_time);
						if (pup_deathbringer_switch)
							player[pid].queue_printf("- Picking up a second deathbringer power-up cancels the effect");
						if (shadow_minimum == 1)
							player[pid].queue_printf("- A player using the shadow power-up gets totally invisible");
						ostringstream pupstr;
						pupstr << "- Base number of power-ups is " << pups_min; if (pups_min_percentage) pupstr << '%';
						pupstr << " and upper limit " << pups_max; if (pups_max_percentage) pupstr << '%';
						if (pups_min_percentage || pups_max_percentage)
							pupstr << " (% of map size)";
						player[pid].add_to_queue(pupstr.str());
						#ifdef SV_SERVER_PHYSICS
						player[pid].queue_printf("The physics model is different (looks funny with a standard 0.5.0 client)");
						#endif
					}
					else if (!strcmp(cbuf, "sayadmin") && sayadmin_enabled) {
						if (strspnp(pCommand, " ")!=NULL) {
							FILE* logp=fopen("sayadmin.log", "at+");
							time_t tt=time(0);
							struct tm* tmb=localtime(&tt);
							fprintf(logp, "%d-%02d-%02d %02d:%02d:%02d  %s: %s\n", tmb->tm_year+1900, tmb->tm_mon+1, tmb->tm_mday,
									tmb->tm_hour, tmb->tm_min, tmb->tm_sec, player[pid].name, pCommand);
							fclose(logp);
							if (shellssock) {
								char lebuf[256];
								int count=0;
								writeLong(lebuf, count, STA_ADMIN_MESSAGE);
								writeLong(lebuf, count, player[pid].cid);
								writeString(lebuf, count, pCommand);
								nlWrite(shellssock, lebuf, count);
							}
							player[pid].queue_printf("@IYour message has been logged. Thank you for your feedback!");
						}
						else
							player[pid].queue_printf("@IFor example to send \"Hello!\", type /sayadmin Hello!");
					}
					else if (!strcmp(cbuf, "map") || !strcmp(cbuf, "mapinfo")) {
						if (*pCommand!='\0') {
							int mid=atoi(pCommand)-1;
							if (mid>=0 && mid<(int)maprot.size() && pCommand[strspn(pCommand, "0123456789")]=='\0') {
								player[pid].queue_printf("@IMap %d is %s", mid+1, maprot[mid].title.c_str());
								player[pid].queue_printf("@I%s.txt, size %dx%d", maprot[mid].file.c_str(), maprot[mid].width, maprot[mid].height);
							}
							else
								player[pid].queue_printf("@WValid map id's are 1 to %d", maprot.size());
						}
						else {
							player[pid].queue_printf("@IThis map is %s", maprot[currmap].title.c_str());
							player[pid].queue_printf("@I%s.txt, size %dx%d", maprot[currmap].file.c_str(), maprot[currmap].width, maprot[currmap].height);
						}
						player[pid].queue_printf("@IType /votemap to see a list of all maps");
					}
					else if (!strcmp(cbuf, "votemap")) {
						string status;
						bool err=false;
						if (*pCommand!='\0') {
							int mid=atoi(pCommand)-1;
							if (mid>=-1 && mid<(int)maprot.size() && pCommand[strspn(pCommand, "0123456789")]=='\0') {
								if (player[pid].mapVote==mid)
									status="no changes";
								else {
									if (player[pid].mapVote==-1)
										status="vote added";
									else if (mid==-1)
										status="vote removed";
									else
										status="vote updated";
									player[pid].mapVote=mid;
									check_map_exit();
								}
								if (!player[pid].want_map_exit)
									player[pid].queue_printf("@TPress F4 to actually vote for a mapchange");
							}
							else {
								player[pid].queue_printf("@W\"%s\" is not a valid map id (1 to %d)", pCommand, maprot.size());
								err=true;
							}
						}
						else
							player[pid].queue_printf("@TFor example to vote for map 1, type /votemap 1");
						if (!err) {
							if (status.length())
								player[pid].queue_printf("@T(%s) Maps on this server: ID, votes, description", status.c_str());
							else
								player[pid].queue_printf("@TMaps on this server: ID, votes, description");
							// 26 chars usable for entry, to fit three on a line
							char buf[200]; int bufi=0;
							int rows=(maprot.size()+2)/3;
							for (int row=0; row<rows; ++row) {
								for (int col=0; col<3; ++col) {
									int mid=col*rows+row;
									if (mid>=(int)maprot.size())
										continue;
									sprintf(buf+bufi, "%2d %2d %-18s", mid+1, maprot[mid].votes, maprot[mid].title.c_str());
									if (strlen(buf+bufi)>24)
										strcpy(buf+bufi+23, ".. ");
									else
										strcpy(buf+bufi+24, "  ");
									bufi+=26;
								}
								player[pid].queue_printf("%s", buf);
								bufi=0;
							}
						}
					}
					else if (!strcmp(cbuf, "time")) {
						// server uptime
						unsigned long uptime = frame/10/60;	// minutes
						int days = uptime / 60 / 24;
						ostringstream server_time;
						server_time << "@IThe server has been up for ";
						if (days > 0)
							server_time << ' ' << days << " day" << (days > 1 ? "s " : " ");
						server_time << uptime / 60 % 24 << ':' << setfill('0') << setw(2) << uptime % 60;
						if (days == 0)
							server_time << " hours";
						server_time << '.';
						player[pid].add_to_queue(server_time.str());
						// map time
						int seconds = (frame - map_start_time) / 10;
						int remaining_seconds = (time_limit / 10 - seconds);
						ostringstream map_time;
						map_time << "@IMap time: " << seconds / 60 << ':' << setfill('0') << setw(2) << seconds % 60 << '.';
						if (time_limit == 0)
							map_time << " There is no time limit.";
						else {
							// time limit not very useful when only one player
							int players = 0;
							for (int i = 0; i < maxplayers; i++)
								if (player[i].used)
									players++;
							if (players == 1)
								map_time << " No time limit at the moment as you are the only player.";
							else if (remaining_seconds < 0) // if time is out and game continues, it must be sudden death
								map_time << " Sudden death.";
							else {
								map_time << " Time left: " << remaining_seconds / 60 << ':';
								map_time << setfill('0') << setw(2) << remaining_seconds % 60 << '.';
							}
						}
						player[pid].add_to_queue(map_time.str());
					}
					else if (!strcmp(cbuf, "stats")) {
						int playing_time = (int)get_time() - player[pid].start_time;  // seconds
						int lifetime = player[pid].lifetime;
						if (!player[pid].dead)
							lifetime += (int)get_time() - player[pid].last_spawn_time;
						player[pid].queue_printf("@TYour stats: %d captures, %d kills, %d deaths, %d suicides",
							player[pid].total_captures,
							player[pid].total_kills,
							player[pid].total_deaths,
							player[pid].total_suicides);
						player[pid].queue_printf(" Enemy flags: %d taken, %d dropped",
							player[pid].total_flags_taken,
							player[pid].total_flags_dropped);
						player[pid].queue_printf(" Own flags: %d returned, %d carriers killed",
							player[pid].total_flags_returned,
							player[pid].total_flag_carriers_killed);
						player[pid].queue_printf(" Consecutive kills: %d (%d), deaths: %d (%d)",
							player[pid].most_consecutive_kills,
							player[pid].current_consecutive_kills,
							player[pid].most_consecutive_deaths,
							player[pid].current_consecutive_deaths);
						int accuracy = 0;
						if (player[pid].total_shots > 0)
							accuracy = int((100. * player[pid].total_hits) / player[pid].total_shots + 0.5);
						player[pid].queue_printf(" Shots: %d shot, accuracy %d%%, %d taken",
							player[pid].total_shots,
							accuracy,
							player[pid].total_shots_taken);
						player[pid].queue_printf(" Distance travelled: %.0lf units, average speed %.2lf units/s.",
							player[pid].total_movement/30.,	// make the unit player diameter <-> divide by 30.
							player[pid].total_movement/30./double(lifetime));
						int av_lifetime = lifetime / (player[pid].total_deaths + 1);
						player[pid].queue_printf(" You have played %d min. Total lifetime %d:%02d. Average lifetime %d:%02d.",
							playing_time / 60,
							lifetime / 60,
							lifetime % 60,
							av_lifetime / 60,
							av_lifetime % 60);
						// Add more stats: flag carrying time, etc.
					}
					#ifdef SV_NAME_AUTHORIZATION
					else if (!strcmp(cbuf, "auth")) {
						string nameUpr;
						for (; *pCommand && *pCommand!=','; ++pCommand)
							nameUpr+=toupper(*pCommand);
						if (*pCommand==',') {
							string pwd(pCommand+1);
							if (authorizations.addIP(nameUpr, pwd, server->get_client_address(id))) {
								authorizations.save();
								player[pid].queue_printf("@WOK: authorized your IP address to use %s", nameUpr.c_str());
								player[pid].queue_printf("@WYou may change your name now");
							}
							else
								player[pid].queue_printf("@WAuthorization failed");
						}
						else
							player[pid].queue_printf("@WInvalid auth command");
					}
					#endif
					else
						player[pid].queue_printf("@WUnknown command %s. Type /help for a list.", cbuf);
				}
				else
				#endif
				if (strspnp(sbuf, " ")!=NULL) {	// ignore messages that are all spaces
					//talk flood protection
					player[pid].talk_temp += player[pid].talk_hotness;
					player[pid].talk_hotness += 3.0;
					if (player[pid].talk_temp > 18.0)
						player[pid].talk_temp = 18.0;
					if (player[pid].talk_hotness > 6.0)
						player[pid].talk_hotness = 6.0;

					if (player[pid].talk_temp > 10.0) {

						//esquenta o cara
						player[pid].talk_temp = 18.0;

						char elbuf[200]; int elcnt = 0;
						writeByte(elbuf, elcnt, 2);
						writeString(elbuf, elcnt, "@WToo much talk. Chill...");
						server->send_message(player[pid].cid, elbuf, elcnt);
					}
					else if (player[pid].muted==1) {
						plprintf(pid, "@WYou are muted. You can't send messages.");
					}
					else {
						// check for team message:
						if (msg[1] == '.') {
							sprintf(talkmsg, "@T%s: %s", player[pid].name, sbuf+1);
							if (player[pid].muted==2)
								player_message(pid, talkmsg);
							else
								broadcast_team_message(pid/TSIZE, talkmsg);
						}
						//regular msg
						else {
							sprintf(talkmsg, "%s: %s", player[pid].name, sbuf);
							if (player[pid].muted==2)
								player_message(pid, talkmsg);
							else
								broadcast_message(talkmsg);
						}
						// log
						//LOG4("client %i player %i name %s says: '%s'\n", id, pid, player[pid].name, &(msg[1]));
					}
				}
			}
			//+attack
			else if (code == 5) {

				//if attack was false, it's a fire event (fire false-->true)
				bool fire_event = (player[pid].attack == false);

				player[pid].attack = true;

				if (fire_event) { // firing code moved to simulate/broadcast frame (player[id].attack is tested there)
				}

			}
			//SUICIDE!!
			else if (code == 10) {
				//only if alive still
				if (player[pid].health > 0) {
					game_kill_player(pid, true);
					player[pid].frags--;                        
					player[pid].total_suicides++;
				}
			}
			//-attack
			else if (code == 11) {

				//if attack was true, it's a fire event (fire false-->true)
				bool unfire_event = (player[pid].attack == true);

				player[pid].attack = false;

				if (unfire_event) {
				}
			}
			// want changeteams on
			else if (code == 12) {
				if (!player[pid].want_change_teams) {
					//set
					player[pid].want_change_teams = true;
					//show message
					//broadcast_message("@I%s player '%s' wants to change teams", teamname[pid/TSIZE], player[pid].name);
					//check for team changes
					check_team_changes();
				}
			}
			// want changeteams off
			else if (code == 13) {
				// show message if command had effect
				if (player[pid].want_change_teams) {
					player[pid].want_change_teams = false;
					player[pid].team_change_pending = false; //so pra garantir
					//broadcast_message("@I%s player '%s' don't want to change teams", teamname[pid/TSIZE], player[pid].name);
				}
			}
			// "client ready" message
			else if (code == 21) {
				//client is ready to play now
				player[pid].awaiting_client_ready = false;
			}
			// want change teams
			else if (code == 22) {
				if (player[pid].want_map_exit == false) {
					player[pid].want_map_exit = true;
					check_map_exit();		//check map exit
				}
			}
			// dont' want change teams
			else if (code == 23) {
				if (player[pid].want_map_exit == true) {
					player[pid].want_map_exit = false;
					check_map_exit();		//check map exit
				}
			}
			// v0.4.4: UDP DOWNLOAD of files - request
			else if (code == 27) {
				char ftype[64];
				char fname[256];
				readString(msg, count, ftype);
				readString(msg, count, fname);
				if (client[id].serving_udp_file) {
					// FIXME: ERROR: this client is already downloading a file
					LOG1("ERROR: CLIENT %i ALREADY DOWNLOADING A FILE!\n", id);
				}
				else {
					//alloc to download
					client[id].serving_udp_file = true;
					char buffy[65536];		//buffy is our friend buffer
					int fsize = get_download_file((char *)buffy, ftype, fname);
					//error: can't read file FIXME: DISCONNECT THE CLIENT
					if (fsize == -1) {
						LOG("ERROR: CAN'T READ THE FILE CLIENT IS ASKING FOR. FIXME: MUST DISCONNECT HIM...\n");
					}
					else {
						client[id].data = new char[fsize];	//allocated to fit!
						memcpy(client[id].data, buffy, fsize);	//copy from buffy to the client's buffer
						client[id].dp = 0;	//RESET FILE POINTER: important
						client[id].fsize = fsize;
						//send a chunk
						upload_next_file_chunk(id);
					}
				}
			}
			// v0.4.4: UDP DOWNLOAD of files - the ack
			else if (code == 29) {

				//check expected ack
				NLulong ackpos;
				readLong(msg, count, ackpos);
				if (client[id].old_dp == ackpos) {

					//check upload successful
					if (client[id].dp >= client[id].fsize) {
						//no more data, this was the last ack. close stuff
						client[id].download_reset();	//reset the download data structs
										//the client will carry on from here
					}
					else {
						//send next
						upload_next_file_chunk(id);
					}
				}
				else {
					//unexpected ack pos. should never happen and if it does ,just discard...
				}
			}
			// v0.4.4 : client is telling his newest token
			else if (code == 30) {

				//get the token from the client
				readString(msg, count, client[id].token);	//read the token
				int intoken = atoi(client[id].token); //atoi it

				//se TOKEN SET + TOKEN VALID + TOKEN IDENTICO : ignora
				if ((client[id].token_have) && (client[id].token_valid) && (intoken == client[id].intoken)) {
#ifdef DEBUG_RANKING
					bprintf("%s %i %i token identico.", player[ctop[id]].name, client[id].intoken, intoken);
#endif
				}
				//senao, faz a rotina de dispatch old results + set new invalid token to be validated
				else {

#ifdef DEBUG_RANKING
					//debug message
					if ((client[id].token_have) && (client[id].token_valid))
						bprintf("%s %i %i changes registration, submitting results...", player[ctop[id]].name, client[id].intoken, intoken);
					else if ((client[id].token_have) && (!client[id].token_valid))
						bprintf("%s %i %i changes registration ('?')", player[ctop[id]].name, client[id].intoken, intoken);
					else if (!client[id].token_have)
						bprintf("%s %i %i sends registration.", player[ctop[id]].name, client[id].intoken, intoken);
#endif

					// v0.4.9 FIX : IF HAD previous token have/valid, then FLUSH his stats
					client_report_status(id);

					// new token
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

					//v0.4.5 : atualiza registration char / score / rank
					broadcast_player_crap( ctop[id] );

					// ENQUEUE TOKEN VALIDATION IN A QUEUE THAT TALKS TO THE MASTER SERVERS

					//create job
					masterjob_c *job = new masterjob_c();
					job->cid = id;
					job->code = 1;
					sprintf(job->request, "GET /servlet/fcecin.tk1/index.html?%s&chktk&name=%s&token=%s\n\n", TK1_VERSION_STRING, player[ ctop [id] ].name, client[id].token);

					//LOG2("== MJOB CREATED : %i '''%s'''\n", id, job->request);

					pthread_t mjob_thread;
					pthread_create (&mjob_thread, 0, thread_masterjob_f, (void *)job);
				}
			}
			//V0.4.9: DEBUGGING the TAB ranking stuff
			else if (code == 33) {
					player_message( ctop[id], "Refreshed your crap...");
					//broadcast his crap
					broadcast_player_crap( ctop[id] );
			}
			// drop flag
			else if (code == 34) {
				player[pid].dropped_flag = true;
				ctf_drop_flag_if_any(pid);
			}
			else {
				//ERROR: unknown message from client
				LOG3("ERROR: UNKNOWN MESSAGE FROM CLIENT %i CODE=%i LENGTH=%i\n", id, code, msglen);
			}
		}
	} while (msg != 0);
}

//team t's flag touched by player #i?
bool gameserver_c::check_flag_touch(int px, int py, int x, int y, int t) {
	if (world.flag[t].carried) return false;	//carried can't touch
	if (world.flag[t].pos.px != px) return false;	//screen x mismatch
	if (world.flag[t].pos.py != py) return false;	//screen y mismatch

	int fx = world.flag[t].pos.x;
	int fy = world.flag[t].pos.y;

	if (fx > x - 30)
	if (fx < x + 30)
	if (fy > y - 30)
	if (fy < y + 30)
		return true;	//touch

	return false;
}

//run a physics frame simulation step for a player
void gameserver_c::run_server_player_physics(int i, frame_t *src, frame_t *dest) {	//player id, frame source, frame dest
	if (dest != src)
		dest->hero[i] = src->hero[i];
	hero_t* hd = &dest->hero[i];

	if (hd->tx<0 || hd->ty<0 || hd->tx>=map.w || hd->ty>=map.h) return;	//#fix: remove this and track why these are given sometimes
	const Room& room = map.room[hd->tx][hd->ty];

	bool carryFlag = src->flag[1-(i/TSIZE)].carried && src->flag[1-(i/TSIZE)].carrier == i;
	bool deathbringerAffected = player[i].deathbringer_end >= get_time();

	float startx = hd->x, starty = hd->y;

	#ifdef SV_SERVER_PHYSICS
		NR_applyPhysics(hd, room, 1., player[i].item_speed, carryFlag, deathbringerAffected);
	#else
		applyDefaultPhysics(hd, room, 1., player[i].item_speed, carryFlag, deathbringerAffected);
	#endif

	float xd = hd->x - startx;
	float yd = hd->y - starty;
	player[i].total_movement += sqrt( xd*xd + yd*yd );

	//check room change x
	if (int(hd->x) == plw) {
		hd->x = 1;
		if (++hd->tx >= map.w)
			hd->tx = 0;
	}
	else if (int(hd->x) == 0) {
		hd->x = plw - 1;
		if (--hd->tx < 0)
			hd->tx = map.w - 1;
	}

	//check room change y
	if (int(hd->y)-SV_SHIFTY == plh) {
		hd->y = 1 +SV_SHIFTY;
		if (++hd->ty >= map.h)
			hd->ty = 0;
	}
	else if (int(hd->y)-SV_SHIFTY == 0) {
		hd->y = plh - 1 +SV_SHIFTY;
		if (--hd->ty < 0)
			hd->ty = map.h - 1;
	}
}

//simulate and broadcast frame
void gameserver_c::simulate_and_broadcast_frame() {

	int i;

	//hack
	static unsigned long ticker = 0;
	ticker++;

	// (-1) check powerup respawn
	//
	double thetime = get_time();
	for (i=0;i<MAX_PICKUPS;i++)
	if (world.item[i].kind == 255)					//valid & respawning
	//if (world.item[i].respawning)		//respawning
	if (thetime > world.item[i].respawn_time)
		respawn_pickup(i);

	// (0) do stuff for every player
	//
	if (!gameover)		//skip if game over
	for (i=0;i<maxplayers;i++)
	if (player[i].used) {

		//dec talk flood protect counter
		player[i].talk_temp -= 0.1;
		if (player[i].talk_temp < 0.0)
			player[i].talk_temp = 0.0;
		player[i].talk_hotness -= 0.1;
		if (player[i].talk_hotness < 1.0)
			player[i].talk_hotness = 1.0;

		//check frags changed
		if (player[i].oldfrags != player[i].frags) {
			//updated
			player[i].oldfrags = player[i].frags;

			//send frag update
			char lebuf[256]; int count = 0;
			writeByte(lebuf, count, 4);	//"4" = frags update
			writeByte(lebuf, count, i);		// what player id
			writeLong(lebuf, count, player[i].frags);
			server->broadcast_message(lebuf, count);
		}

		// check powerups expired
		//
		if (player[i].item_speed)
		if (get_time() > player[i].item_speed_time) {
			player[i].item_speed = false;
			broadcast_screen_sample(i, SAMPLE_BOOTS_OFF);
		}
		if (player[i].item_quad)
		if (get_time() > player[i].item_quad_time) {
			player[i].item_quad = false;
			broadcast_screen_sample(i, SAMPLE_QUAD_OFF);
		}
		if (player[i].item_helm)
		if (get_time() > player[i].item_helm_time) {
			player[i].item_helm = 0;
			broadcast_screen_sample(i, SAMPLE_HELM_OFF);
		}

		// helm alpha down
		//
		if (player[i].item_helm > 0) {
			player[i].item_helm -= 10;		//slowly fades....
			if (player[i].item_helm < shadow_minimum)	// minimum
				player[i].item_helm = shadow_minimum;
		}

		// check deathbringer effect
		//
		if (player[i].deathbringer_end > get_time()) {
			//check if still alive
			if (player[i].health > 0) {
				//has shield: do big damage to it, in order to remove the shield
				if (player[i].item_shield)
					game_damage_player(i, player[i].deathbringer_attacker, 12, true);
				else
					game_damage_player(i, player[i].deathbringer_attacker, 3, true); // 30 / s, 150 / 5 s
			}
		}

		// check for a player's deathbringer to bring death
		//
		if (player[i].health <= 0) //dead
		if (player[i].item_deathbringer)		//with deathbringer
		{
			//delta time since shoot
			double delta = (frame - player[i].item_deathbringer_time) * 0.1;
			//figure out new radius
			int rad;
			if (delta < 1.0)
				rad = (int)(delta * 100);
			else
				rad = 100 + (int)((delta - 1.0) * (delta - 1.0) * 800);

			//check enemy players onscreen that are not hit by it yet and are inside
			// the donut radius...radius-50
			for (int v=0;v<maxplayers;v++)
			if (v/TSIZE != i/TSIZE)		//enemy players only
			if (player[v].used)	//used
			if (player[v].health > 0)	//alive
			if (player[v].x == player[i].x)	// in the same screen of the deathbringer
			if (player[v].y == player[i].y)
			if (player[v].deathbringer_end < get_time())		// deathbringer fx end time -- not already hit?
			{
				//calculate player distance to the deathbringer core
				double ex = world.hero[i].x;
				double ey = world.hero[i].y - 15;
				double rx = world.hero[v].x;
				double ry = world.hero[v].y - 15;
				double dt = sqrt( (ex - rx)*(ex - rx) + (ey - ry)*(ey - ry) );

				// hit distance: if dt == rad, hit, if rad
				if ((rad <= dt + 20) && (rad >= dt - 60)) {
					player[v].item_deathbringer = false;
					broadcast_screen_sample(v, SAMPLE_HITDEATHBRINGER);
					player[v].deathbringer_attacker = i;
					// time of effect ; also freeze his gun for this same amount of time
					player[v].deathbringer_end = player[v].next_shoot_time = get_time() + 4.5 + ((double)(rand() % 1000) / 1000.0);

					// calc recoil:
					double tx = world.hero[v].x - world.hero[i].x;
					double ty = world.hero[v].y - world.hero[i].y;

					double mul = 40. / sqrt( tx*tx + ty*ty );	// set speed to 40
					world.hero[v].sx = tx * mul;
					world.hero[v].sy = ty * mul;
				}
			}
		}

		// check for player weapons fire time
		//
		if (player[i].attack)	// player holding attack button
		if (player[i].health > 0)		// check if player alive
		if (get_time() > player[i].next_shoot_time)  // check if time allowed to fire again
		{
			//gasta 7 + 2 * tiros energy, se tem energy
			int numshots = 1;
			player[i].energy -= 7;			//gasta normal
			if (player[i].energy < 0)	//se ficou menor que zero, atira 1 so
				player[i].energy = 0;
			else {
				for (int k=1;k<player[i].weapon+1;k++) {
					//try add one shot
					player[i].energy -= 1;		//v0.4.7: diminuí METADE do gasto per shot!
					if (player[i].energy < 0)
						player[i].energy = 0;
					else
						numshots++;
				}
			}

			player[i].next_shoot_time = get_time() + 0.5;		// add minimum interval (in secs)

			//show helm
			if (player[i].item_helm > 0)
				player[i].item_helm = 255;

			//v0.1.2 shoot rocket
			game_shoot_rocket(
				i,						//player
				numshots,			//quantos tiros
				player[i].x,	//px
				player[i].y,	//py
				(int)world.hero[i].x,		//x
				(int)world.hero[i].y,		//y
				world.hero[i].gundir);	//direction
		}

	}


	// (1)  simulate (calculate) the next frame
	//

	// for each ROCKET, update position
	//
	if (!gameover)		//skip if game over
	for (i=0;i<MAX_ROCKETS;i++)
	if (world.rock[i].owner != -1)	//exists
	{
		rocket_c *rock = &(world.rock[i]);

		//run ten times for better collision accuracy (UGLY UGLY UGLY HACK)
		int t;
		for (t=0;t<10;t++)
		{
			//move-se
			rock->x += rock->sx / 10.0;
			rock->y += rock->sy / 10.0;

			//out of bounds
			if ((rock->x < -20) || (rock->y < -20) || (rock->x > plw + 20) || (rock->y > plh + 20)) {
				rock->owner = -1;	//just remove it. clients will figure out the same

				//broadcast_message("SE FOI");

				//2-loop break
				t=999;break;
			}

			//wall hit - remove
			#if !defined(SV_SERVER_PHYSICS)
			if (map.fall_on_wall(rock->px, rock->py, (int)rock->x, (int)rock->y, (int)rock->x, (int)rock->y)) {
				rock->owner=-1;
				t=999;break;
			}
			#endif
			#ifdef SV_SERVER_PHYSICS
			if (map.fall_on_wall(rock->px, rock->py, (int)rock->x-2, (int)rock->y-SV_SHIFTY-2, (int)rock->x+2, (int)rock->y-SV_SHIFTY+2)) {
				rock->owner=-1;
				t=999;
				break;
			}
			#endif

			// check if a player (alive) is hit by this rocket now
			//
			//sqrt( (ex - x)*(ex - x) + (ey - y)*(ey - y) ). Acho que é isto...

			for (int p=0;p<maxplayers;p++)
			if (player[p].used)
			if (player[p].health > 0)		// alive
			if (rock->team != (p/TSIZE)) // shot is from opposing team
			if (rock->px == player[p].x) // in same screen
			if (rock->py == player[p].y)
			{
				//calculate distance rocket<->target center
				double ex = world.hero[p].x;
				double ey = world.hero[p].y - 15.0;
				double rx = rock->x;
				double ry = rock->y - 15.0;
				double dt = sqrt( (ex - rx)*(ex - rx) + (ey - ry)*(ey - ry) );

				//the number is the sum of the two balls bounding boxes radiuses (15 player + 3 rocket's)
				if (dt <= 18.0)
				{
					//record wether the player had shield, if yes, will not blink him
					bool had_shield = player[p].item_shield;

					//default damage to the target: 70
					int damage = 70;

					//v0.4.0: dano 50 se esta com o deathbringer
					if (player[rock->owner].item_deathbringer)
						damage = 50;

					if (player[rock->owner].item_quad)
						damage *= 2;

					//do damage
					game_damage_player(p, rock->owner, damage, false);

					player[rock->owner].total_hits++;
					player[p].total_shots_taken++;

					//if player not dead, push him
					if (player[p].health > 0) {
						if (((world.hero[p].sx > 0) && (rock->sx < 0)) || ((world.hero[p].sx < 0) && (rock->sx > 0)))
							world.hero[p].sx = 0;
						if (((world.hero[p].sy > 0) && (rock->sy < 0)) || ((world.hero[p].sy < 0) && (rock->sy > 0)))
							world.hero[p].sy = 0;
						world.hero[p].sx += rock->sx / 3.0;
						world.hero[p].sy += rock->sy / 3.0;
					}

					//delete shot
					//game_delete_rocket(i, t, p);
					if (had_shield)
						game_delete_rocket(i, (NLshort)rock->x, (NLshort)rock->y, 252);		//do not blink
					else
						game_delete_rocket(i, (NLshort)rock->x, (NLshort)rock->y, p);			//blink

					//2-loop break
					t=999;break;
				}
			}

		}
	}

	// for each player, update positions & speeds
	//
	hero_t  *h;

	if (!gameover)		//skip if game over
	for (i=0;i<maxplayers;i++)
	if (player[i].used) {
		h = &(world.hero[i]);

		//check if dead/respawn
		if (player[i].health <= 0) {
			if (player[i].respawn_time < get_time())
				game_respawn_player(i);		//time to respawn player
		}
		// player alive: do stuff for alive players
		else {

			// IN : copia player screen p/ hero screen
			h->tx = player[i].x;
			h->ty = player[i].y;

			// run server physics frame
			run_server_player_physics(i, &world, &world);	//player id, frame source, frame dest

			//OUT : copy screen information from hero back to player
			if ((player[i].x != h->tx) || (player[i].y != h->ty))
			{
				player[i].x = h->tx;
				player[i].y = h->ty;

				//player screen changed check
				game_player_screen_change(i);
			}


			//---------------------------------
			// player every-frame stuff
			//---------------------------------

			// check don't regen because of deathbringer
			//v0.4.0: do not regen if has deathbringer and both health/energy are at no less than 100
			bool deathbringer_penalty =
					((player[i].item_deathbringer) && (player[i].health >= 100) && (player[i].energy >= 100))			//rand() % 100 < 50
					||
					(player[i].deathbringer_end > get_time());

			// regen?
			if (!deathbringer_penalty) {

				// regenerate +1 health or +1 energy
				if (player[i].health < 100)
					player[i].health++;
				else {

					//caso energy > 100, regenera mais devagar (-33%)
					if (player[i].energy < 100)
						player[i].energy++;
					else if (player[i].energy < 200) {
						// 0.3.0: MAIS devagar
						//if (frame % 3)
						if (frame % 2)
							player[i].energy++;
					}
					//MEGA health vagarosamente...
					else if ((player[i].health < 200) && (frame %10 == 0))
						player[i].health++;
				}
			}

			//lose health & energy if running
			if (h->run) {

				if (player[i].energy <= 0) {

					//if (!player[i].item_speed)	// se ta com SPEED, faz nao hurt
					if (player[i].health > MIN_HEALTH_FOR_RUN_PENALTY) {	// se health > 30, desconta

						if (ticker % 2)
							player[i].health -= 2;	//desconta 2 (o normal)
						else
							player[i].health -= 1;	//desconta 1 (menos)

						if (player[i].health < MIN_HEALTH_FOR_RUN_PENALTY)		// garante minimo 30
							player[i].health = MIN_HEALTH_FOR_RUN_PENALTY;
					}

				} else {

					if (ticker % 2)
						player[i].energy -= 2; //desconta 2 (o normal)
					else
						player[i].energy -= 1; //desconta 1 (menos)

					if (player[i].energy == -1) { // special case

						player[i].energy++;

						if (player[i].health > MIN_HEALTH_FOR_RUN_PENALTY) {	// se health > 30, desconta

							player[i].health--;

							if (player[i].health < MIN_HEALTH_FOR_RUN_PENALTY)		// garante minimo 30
								player[i].health = MIN_HEALTH_FOR_RUN_PENALTY;
						}
					}
				}
			}

			//rot health to 100 if has deathbringer
			if ((player[i].item_deathbringer) && (player[i].health > 100) && (frame % 4 == 0))
				player[i].health--;

			//rot energy to 100 if has deathbringer
			if ((player[i].item_deathbringer) && (player[i].energy > 100) && (frame % 4 == 0))
				player[i].energy--;

			//megahealth bonus:
			if (player[i].megabonus > 0)
			if ((player[i].health == 300) && (player[i].energy == 300))
				player[i].megabonus--;	// realiza um certo "guardamento" de energia mas nao muito...
			else
			for (int mh=0;mh<5;mh++) {

					//+health
					if (player[i].megabonus > 0)
					if (player[i].health < 300) {
							player[i].health++;
							player[i].megabonus--;
					}

					//+energy
					if (player[i].megabonus > 0)
					if (player[i].energy < 300) {
							player[i].energy++;
							player[i].megabonus--;
					}
			}


			//limit health 0 .. 300
			if (player[i].health < 0)
				player[i].health = 0;
			else if (player[i].health > 300)
				player[i].health = 300;

			//limit energy 0 .. 300
			if (player[i].energy < 0)
				player[i].energy = 0;
			else if (player[i].energy > 300)
				player[i].energy = 300;

			//---------------------------------
			// check game object collisions
			//---------------------------------

			int myteam = i/TSIZE;
			int enemyteam = 1 - myteam;

			// --> ITEM PICKUP
			//
			int prad = 10;	//pickup item radius

			for (int k=0;k<MAX_PICKUPS;k++)
			if (world.item[k].kind > 0)	//valid item
			if (world.item[k].kind != 255) // not respawning
			if (world.item[k].px == player[i].x)		// player's screen
			if (world.item[k].py == player[i].y)
			//x,y == center of powerup!
			if (world.item[k].x + prad > world.hero[i].x - 20)
			if (world.item[k].x - prad < world.hero[i].x + 20)
			if (world.item[k].y + prad > world.hero[i].y - 20 - 10)
			if (world.item[k].y - prad < world.hero[i].y + 20 - 10)
			{
				//pick pickup
				game_touch_pickup(i, k);		//COOL!
			}

			// --> CTF FLAG STEAL touch other team's flag
			//
			if (!world.flag[enemyteam].carried &&	// enemy flag dropped (at base or somewhere)
				check_flag_touch(player[i].x, player[i].y, (int)h->x, (int)h->y, enemyteam))  // and I touch it
			{
				// Has player just dropped the flag or not?
				if (!player[i].dropped_flag) {
					//v0.4.7: update grab time (to detect degenerated maps) if flag was at base
					if (world.flag[enemyteam].atbase)
						world.flag[enemyteam].grab_time = get_time();

					//FLAG STOLEN!
					score_frag(i, 1);	// just add some frags

					player[i].total_flags_taken++;

					bprintf("@I%s GOT THE %s FLAG!", player[i].name, teamname[enemyteam]);

					ctf_steal_flag(enemyteam, i);  //flag stolen!

					//HELM powerup: show player
					if (player[i].item_helm > 0)
						player[i].item_helm = 255;
				}
			}
			else	// Player has removed away from the flag.
				player[i].dropped_flag = false;

			// --> CTF FLAG RETURN
			//
			if (!world.flag[myteam].carried)	// my flag dropped
			if (!world.flag[myteam].atbase)	// not at base
			if (check_flag_touch(player[i].x, player[i].y, (int)h->x, (int)h->y, myteam))  // and I touch it
			{
				//FLAG RETURNED!
				score_frag(i, 1);	// just add some frags
				player[i].total_flags_returned++;

				bprintf("@I%s RETURNED THE %s FLAG!", player[i].name, teamname[myteam]);

				ctf_return_flag(myteam);  //flag returned

				//sound broadcast
				broadcast_sample(SAMPLE_CTF_RETURN);
			}

			// --> CTF FLAG CAPTURE
			//
			if (world.flag[enemyteam].carried)		// enemy flag carried
			if (world.flag[enemyteam].carrier == i)	// by me
			if (!world.flag[myteam].carried)	// my flag dropped
			if (world.flag[myteam].atbase)	// at my base
			if (check_flag_touch(player[i].x, player[i].y, (int)h->x, (int)h->y, myteam))		// I touch my flag
			{
				//v0.4.7: detect degenerated maps
				if (map.valid_for_scoring)		//still valid?
				if (get_time() - world.flag[enemyteam].grab_time <= MINIMUM_GRAB_TO_CAPTURE_TIME) {

					//this map is bogus, ignore all scoring for it.
					map.valid_for_scoring = false;
					//tell people...
					broadcast_message("@WThis map is too small. Scoring for World Ranking disabled.");
					//zero all delta scores so far
					for (int p=0;p<MAX_PLAYERS;p++) {
						client[p].delta_score = 0;
						client[p].neg_delta_score = 0;		//V0.4.8
						client[p].fdp = 0.0;
						client[p].fdn = 0.0;		//V0.4.8
					}
				}

				//add frags to all players of the team

				// V0.4.8: PENALIZE every player of the other team

				for (int h=0;h<MAX_PLAYERS;h++)
				if (player[h].used)
				if ((h/TSIZE) == myteam)
					score_frag(h, 2);				//small two-frag bonus
				else
					score_neg(h, 1);		//v0.4.8 : small NEG POINT penalty for YOUR FLAG BEING CAPTURED

				//CHANGED 0.4.8: add +3 extra frags to the capturer (for a total of 5)
				score_frag(i, 3);

				//return enemy flag to their base
				ctf_return_flag(enemyteam);

				//message
				string one_more;
				if (world.flag[myteam].score == capture_limit - 2) // points update later
					one_more = " One more to win!";
				bprintf("@I%s CAPTURED THE %s FLAG!%s", player[i].name, teamname[enemyteam], one_more.c_str());

				//count
				player[i].total_captures++;

				//update the ADMIN SHELL
				char lebuf[256]; int count; NLint result;
				count = 0;
				writeLong(lebuf, count, STA_PLAYER_CAPTURES);
				writeLong(lebuf, count, player[i].cid);
				result = nlWrite(shellssock, lebuf, count);

				//CAPTURE (team count ++)
				world.flag[myteam].score++;
				ctf_update_teamscore(myteam);		//this function can decide to restart the game . (?)

				//sound broadcast
				broadcast_sample(SAMPLE_CTF_CAPTURE);
			}

		} // player.health > 0

	}

	// announce voting status
	if (frame >= next_vote_announce_frame) {
		int votes=0, players=0;
		for (int i=0; i<maxplayers; ++i)
			if (player[i].used) {
				++players;
				if (player[i].want_map_exit)
					++votes;
			}
		players=players/2+1;
		if (votes!=last_vote_announce_votes || (players!=last_vote_announce_needed && votes!=0)) {
			last_vote_announce_votes=votes;
			last_vote_announce_needed=players;
			next_vote_announce_frame=frame+SV_VOTE_ANNOUNCE_INTERVAL*10;
			ostringstream voteinfo;
			voteinfo << "@I*** " << votes << '/' << players << " votes for mapchange";
			if (map_start_time+vote_block_time >= frame)
				voteinfo << " (all players needed for " << (map_start_time+vote_block_time-frame+5)/10 << " more seconds)";
			broadcast_message(voteinfo.str().c_str());
		}
	}

	// player maintenance (check delayed messages etc)
	int players = 0;
	for (int i=0; i<maxplayers; ++i)
		if (player[i].used) {
			++players;
			if (player[i].kickTimer) {
				--player[i].kickTimer;
				if (player[i].kickTimer==0)
					server->disconnect_client(player[i].cid, 1);	// 1 second timeout
				else if (player[i].kickTimer%10 == 0 && player[i].kickTimer<=50)
					plprintf(i, "@WDisconnecting in %d...", player[i].kickTimer/10);
			}
			else {
				player_t::DMQueueT& dm=player[i].delayedMessages;
				while (dm.size() && --dm.begin()->first<0) {
					player_message(i, dm.begin()->second.c_str());
					dm.erase(dm.begin());
				}
			}
		}

	// check timelimit
	if (players > 1 && time_limit > 0) {
		if (time_limit >= 10*60 * 10 && frame - map_start_time == time_limit - 5*60 * 10)
			bprintf("@I*** Five minutes left in the game");
		if (time_limit >= 2*60 * 10 && frame - map_start_time == time_limit - 60 * 10)
			bprintf("@I*** One minute left in the game");
		else if (time_limit >= 60 * 10 && frame - map_start_time == time_limit - 30 * 10)
			bprintf("@I*** 30 seconds left in the game");
		else if (frame - map_start_time > time_limit) {
			bprintf("@I*** Time out - CTF game over");
			server_next_map(NEXTMAP_CAPTURE_LIMIT);	// ignore return value
			ctf_game_restart();
		}
	}

	// (2)  broadcast the frame
	//
	//		o pacote nao eh o mesmo pra todo mundo, entao nao eh broadcast
	//		m uma parte que depende do player (tipo, qual o health do cara)
	//
	// server frame format:  (protocolo v0.4.1)
	//
	//  --- PRIMEIRA PARTE : igual pra todo mundo ----
	//
	//    LONG  frame
	//		LONG  players present (bits 0..31 dizendo quais players[] sao validos)
	//
	// --- SEGUNDA PARTE : varia p/ cada cliente -----
	//
	//		BYTE xtra   (bitfield)
	//       0  health extra bit (+256)
	//       1  energy extra bit (+256)
	//       2  SKIP FRAME : no more frame data (depois desse byte)
	//       3..7   "me" (0..31)
	//    BYTE player screen(room) x
	//    BYTE player screen(room) y
	//    LONG players onscreen (bits 0..31 dizendo quais players[] estao na mesma room que eu)
	//
	//    ** E PARA CADA "PLAYER ONSCREEN", na ordem do bitfield, de 0 a 31:
	//       3 BYTES   x e y
	//       2 BYTES   sx e sy
	//       BYTE   extra (bitfield)
	//         0   player dead?
	//         1   has deathbringer?
	//         2   affected by deathbringer?
	//         3   has shield?
	//         4   has speed?
	//         5   has quad?
	//         6..7   FREE BITS
	//       BYTE   keys (aceleracao/bitfield)
	//           0   left?
	//           1   right?
	//           2   up?
	//           3   down?
	//           4   running? (SHIFT)
	//           5..7   gundir  (direcao em que esta mirando)
	//       BYTE   shadow alpha level
	//
	//    SHORT inimigos visiveis (0..15)  (bitfield)
	//    BYTE  indice "V" do jogador que eu vou ficar sabendo agora (0-31)
	//    BYTE  minimap x do player V
	//    BYTE  minimap y do player V
	//    BYTE  health base do jogador (primeiros 8 bits)
	//    BYTE  energy base do jogador (primeiros 8 bits)
	//    SHORT ping do jogador : player[frame % maxplayers].ping;
	//

	//RECALC PLAYERS PRESENT EVERY TIME
	NLulong players_present = 0;
	for (int pp=0;pp<maxplayers;pp++)
	if (player[pp].used)
		players_present += (1 << pp);


	// ============================
	//   build common data buffer
	// ============================
	char lebuf[4096];		//common frame data
	int count = 0;

	//frame
	writeLong(lebuf, count, frame);

	//players present NEW: LONG
	writeLong(lebuf, count, players_present);

	//===============================
	//  build packet for each client
	//		with custom data
	//===============================
	int lecount;	//count after "count"

	//stuff for minimap update: my team's enemy team view
	static int tviter[2] = { 0 , 0 };
	static int helmiter = 0;		// HELM ITERATOR : manda todo mundo!
	int tview[2][MAX_PLAYERS/2];		//[time][inimigo# visto? 1-8]
	NLushort	tview_bits[2];	//enemy view SHORT (bitfield for the 8 enemies of each team(0,1))

	//HELM PATCH: the "helm view" bytes for both teams - if somebody has helm, he will se
	//  helmview[] for his team
	NLushort helmview[2];

	//atualiza HELM ITERATOR - para em um player valido ou entao qualquer um
	int runaway = maxplayers + 1;
	do {
		helmiter++;
		if (helmiter > maxplayers - 1)
			helmiter = 0;
		if (player[helmiter].used)
			//fix: helm nao enxerga outros helms, a nao ser com a flag
			//ou seja: so mostra (break) se:  NAO TEM HELM   ou   TEM FLAG
			if ((player[helmiter].item_helm == 0) || ((world.flag[1-helmiter/TSIZE].carried) && (world.flag[1-helmiter/TSIZE].carrier == helmiter)))
				break;
	} while (runaway-- > 0);

	int t;

	//atualiza tview E HELMVIEW
	for (t=0;t<2;t++) {		// p/ cada time

		tview_bits[t] = 0;

		helmview[t] = 0;		//default zero

		for (int i=0;i<maxplayers;i++)			// p/ cada inimigo desse time
		if (i/TSIZE == 1-t)
		if (player[i].used)
		{
			// ---- helmview -----
			// mostra se NAO TEM HELM ou SE TA COM FLAG
			if ((player[i].item_helm == 0) || ((world.flag[1-i/TSIZE].carried) && (world.flag[1-i/TSIZE].carrier == i))) {
				//adiciona bit
				//helmview[t] += ((NLushort) (1 << (i%TSIZE)));
				// FUCKING TYPE CONVERSION HELL
				int fuck = helmview[t];
				fuck += (1 << (i%TSIZE));
				helmview[t] = (NLushort)fuck;
			}

			// ---- tview -----
			tview[t][i] = 0;		// default = nao visto

			for (int j=0;j<maxplayers;j++)			// verifica se ele esta no campo de visao (tela) de alguem do meu time
			if (j/TSIZE == t)
			if (player[j].used)
			{
				if ((player[j].x == player[i].x) && (player[j].y == player[i].y))	{

					//se o cara tem helm E NAO TEM FLAG, nao aparece!!
					if ((player[i].item_helm > 0) && ((world.flag[1-i/TSIZE].carried == false) || (world.flag[1-i/TSIZE].carrier != i))) {
						//nao visto!
					}
					else {
						//visto!
						tview[t][i%TSIZE] = 1;	//visto!
						//tview_bits[t] += ((NLushort) (1 << (i%TSIZE)));		//seta bit de "visto"
						// FUCKING TYPE CONVERSION HELL
						int fuck = tview_bits[t];
						fuck += (1 << (i%TSIZE));
						tview_bits[t] = (NLushort)fuck;
						break;
					}
				}
			}
		}

		//avanca tviter do time p/ escolher alguem
		int runaway = maxplayers + 3;
		do {
			//avanca proximo candidato a envio
			tviter[t]++;
			if (tviter[t] < 0)
				tviter[t] = 0;
			if (tviter[t] >= maxplayers)
				tviter[t] = 0;

			//testa se o candidato se aplica ao visor minimap do time
			//testa apenas used players
			if (player[tviter[t]].used) {

				//do meu time? envia, tenho q saber todos do meu time
				if (tviter[t]/TSIZE == t)
					break;

				//inimigo? so se estiver na visao do time
				if (tviter[t]/TSIZE == 1-t)
				if (tview[t][ tviter[t]%TSIZE] == 1)
					break;
			}

		} while (runaway-- > 0);
	}

	// ==================================================================
	//   BUILD AND SEND EVERY DAMN PACKET
	// ==================================================================
	for (i=0;i<maxplayers;i++)
	if (player[i].used)			// player valido (used)
	{
		//rewrite past common data
		lecount = count;

		//extra byte of information
		// BIT 0: extra health
		// BIT 1: extra energy
		// BIT 2  ( VERY IMPORTANT ) : player not ready bit (envia frame vazio!) OU server exibindo placa "game over"
		NLubyte xtra = 0;
		if (player[i].health & 256) xtra += 1;
		if (player[i].energy & 256) xtra += 2;
		// BITS 3..8 == what player id
		if (i & 1) xtra += 8;
		if (i & 2) xtra += 16;
		if (i & 4) xtra += 32;
		if (i & 8) xtra += 64;
		if (i & 16) xtra += 128;

		bool skip_frame = (player[i].awaiting_client_ready) || ((gameover) && (gameover_time > get_time()));

		if (skip_frame) xtra += 4;		// VERY IMPORTANT
		writeByte(lebuf, lecount, xtra);

		//****** VERY IMPORTANT ******
		// send almost empty frame if client not ready (leave bandwidth for data transfer) OR IF
		// server showing gameover plaque
		if (!skip_frame) {

			// NEW: 0.3.9 : send before players_onscreen 2 bytes with the screen of self
			//
			NLubyte scr;
			scr = ((NLubyte)player[i].x);
			writeByte(lebuf, lecount, scr);	//player.x (screen)
			scr = ((NLubyte)player[i].y);
			writeByte(lebuf, lecount, scr);	//player.y (screen)

			//player data field for each player ON SCREEN
			NLulong		players_onscreen = 0;

			//keep place for players_onscreen
			int p_on_count = lecount;
			writeLong(lebuf, lecount, 0);

			NLubyte keys;
			for (int j=0;j<maxplayers;j++)
			if ((players_present & (1 << j)) != 0)		//player j exists
			// j is on same screen than i (the viewer)
			// AND
			//   ((j helm != 1)  ||  (j/TSIZE == i/TSIZE))  // nao-totalmente invisivel OU e do mesmo time OU j com flag
			if (
					(player[j].x == player[i].x)
					&&
					(player[j].y == player[i].y)
					&&
					(player[j].item_helm != 1 || i/TSIZE == j/TSIZE ||
							(world.flag[1-j/TSIZE].carried && world.flag[1-j/TSIZE].carrier == j)) ) {
				//add to players_onscreen
				players_onscreen += (1 << j);

				h = &(world.hero[j]);

//					NLshort sho;

				//V0.3.9: took out screen from here

				//V0.3.9 : transmissao x,y de 4 bytes para 3
				NLubyte xy;
				NLushort hx,hy;
				hx = ((NLushort)h->x);
				hy = ((NLushort)h->y);

				xy = (NLubyte) (hx & 255);
				writeByte(lebuf, lecount, xy);		//first 8 bits x
				xy = (NLubyte) (hy & 255);
				writeByte(lebuf, lecount, xy);		//first 8 bits y
				//256+512+1024+2048 = 3840    last 4 bits mask
				xy = (NLubyte) ( ((hx & 3840) >> 8) + ((hy & 3840) >> 4) ); //x: bit 8-11 to 0-3  y: bit 8-11 to 4-7
				writeByte(lebuf, lecount, xy);   //last 4 bits x + last 4 bits y

				//sho = ((NLshort)h->x);
				//writeShort(lebuf, lecount, sho);	//x
				//sho = ((NLshort)h->y);
				//writeShort(lebuf, lecount, sho);	//y

				//speed em bytes - xinelao mesmo
				NLbyte sxy;
				sxy = ((NLbyte)(h->sx * 2));
				writeByte(lebuf, lecount, sxy);
				sxy = ((NLbyte)(h->sy * 2));
				writeByte(lebuf, lecount, sxy);

				//sho = ((NLshort)(h->sx * 100));
				//writeShort(lebuf, lecount, sho );	//sx  30.283482345634... = 30283 = 30.283(depois)
				//sho = ((NLshort)(h->sy * 100));
				//writeShort(lebuf, lecount, sho );	//sy

				/*
				EXTRA BYTE (ex- zframe)

				bit 0 : player dead
				bit 1 : has deathbringer
				bit 2 : deathbringer-affected
				*/
				NLubyte extra = 0;
				if (player[j].health <= 0) extra += 1; //deadflag
				if (player[j].item_deathbringer) extra += 2; //has deathbringer
				if (player[j].deathbringer_end > get_time()) extra += 4;		//deathbringer-affected
				// ITEMS: moved to this byte
				if (player[j].item_shield)		extra += 8;
				if (player[j].item_speed)			extra += 16;
				if (player[j].item_quad)			extra += 32;

				//write extra byte
				writeByte(lebuf, lecount, extra);

				//keys
				keys = 0;

				//set bits 0..3
				// if dead player, don't send keys
				if (player[j].health > 0) {
					if (h->l) keys += 1;
					if (h->r) keys += 2;
					if (h->u) keys += 4;
					if (h->d) keys += 8;
					if (h->run) keys += 16;		//bit 4 is "running" flag (max-speed modifier for the clientside movement extrapolator)
				}

				//set bits 5..7
				//keys = keys + ((NLubyte)(h->gundir * 32));
				// FUCK NONIMPLICIT TYPE CONVERSION AND FUCK ANSI-C
				int fuck = keys;
				fuck += h->gundir * 32;
				keys = (NLubyte)fuck;

				//write keys
				writeByte(lebuf, lecount, keys);

				//write SHIELD  BOOTS / QUAD
				//NLubyte		itemflags = 0;
				//if (player[j].item_shield)		itemflags += 1;
				//if (player[j].item_speed)			itemflags += 2;
				//if (player[j].item_quad)			itemflags += 4;
				//writeByte(lebuf, lecount, itemflags);

				//write HELM alpha level
				writeByte(lebuf, lecount, ((NLubyte)player[j].item_helm));
			}

			//update players_onscreen (it's before the players on screen data (above))
			writeLong(lebuf, p_on_count, players_onscreen);

			//ELMO: visao alem do alcance!!
			NLubyte who;
			if (player[i].item_helm > 0) {

				//team "viewed enemies" do meu time (i/TSIZE)
				//writeByte(lebuf, lecount, 255);		// todos!!!
				//FIX: helm nao enxerga todo mundo nao
				//team "viewed enemies" do meu time (i/TSIZE)
				//writeByte(lebuf, lecount, tview_bits[i/TSIZE]);
				//FIX: helm tambem nao eh no viewed enemies. o helm de um time é 255 (todos)
				//     menos quem tem helm
				writeShort(lebuf, lecount, helmview[i/TSIZE]);

				//"quem eu vou ficar sabendo no minimap agora?" -- do time
				who = (NLubyte)helmiter;
				writeByte(lebuf, lecount, who);
			}
			//sem elmo: visao normal
			else {

				//team "viewed enemies" do meu time (i/TSIZE)
				writeShort(lebuf, lecount, tview_bits[i/TSIZE]);

				//"quem eu vou ficar sabendo no minimap agora?" -- do time
				who = (NLubyte)tviter[i/TSIZE];
				writeByte(lebuf, lecount, who);
			}

			//x do cara, 0..255 (%) do mundo
			NLubyte mx = (NLubyte)(((world.hero[who].x + ((double)(player[who].x * plw))) / (map.w*plw)) * 255.0);
			writeByte(lebuf, lecount, mx);

			//y do cara, 0..255 (%) do mundo
			NLubyte my = (NLubyte)(((world.hero[who].y + ((double)(player[who].y * plh))) / (map.h*plh)) * 255.0);
			writeByte(lebuf, lecount, my);

			//send player's BASE health (first 8 bits)
			if (player[i].health < 0) player[i].health = 0;
			writeByte(lebuf, lecount, ((NLubyte)(player[i].health & 255)));

			//send player's BASE energy (first 8 bits)
			if (player[i].energy < 0) player[i].energy = 0;
			writeByte(lebuf, lecount, ((NLubyte)(player[i].energy & 255)));

			//ping of player frame# % MAXPLAYERS
			NLushort theping = (NLushort) player[frame % maxplayers].ping;
			writeShort(lebuf, lecount, theping);

		}//!player[i].awaiting_client_ready

		//send the packet
		server->send_frame(player[i].cid, lebuf, lecount);	//use client id of the player, and LEcount
	}

	// PING: v0.4.1
	// envia um ping a cada frame, faz revezamento entre todos os players
	ping_send_client++;		//next player
	if (ping_send_client >= maxplayers)
		ping_send_client = 0;
	if (player[ping_send_client].used) // valid player?
	server->ping_client(player[ping_send_client].cid); //ping
}

//run something after simulate_and_broadcast
void gameserver_c::server_think_after_broadcast() {
	int i;

	//check players with pending team changes
	for (i=0;i<maxplayers;i++)
	if (player[i].used)
	if (player[i].team_change_pending)
	if (player[i].want_change_teams)
	if (player[i].team_change_time < get_time())
		check_player_change_teams(i);

	//check end of gameover plaque
	if (gameover)
	if (gameover_time < get_time()) {
		gameover = false;
		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 25);		//end of gameover plaque
		server->broadcast_message(lebuf, count);
	}
}

//loop server
// running_flag: pointer to bool, if this bool goes to false, the loop quits.
void gameserver_c::loop(volatile bool *running_flag) {

	LOG("GAMESERVER::LOOP()\n");

	frame = 0;	//frame to generate next

	//sync with speed counter until it's time to generate one frame (== 1)
	server_speed_counter = -3;
	while (server_speed_counter < 1)
		MS_SLEEP(1);		//*** NO CPU PROBLEM HERE ***

	// no flag specified: esc quits
	bool keep_running = true;
	if (!running_flag)
		running_flag = &keep_running;

	static int fubie = 0;

	LOG("GAMESERVER::LOOP() (2)\n");

	while ( (*running_flag) == true )	{

		// generate and send frame
		simulate_and_broadcast_frame();

		//dec counter - another 100ms must pass before next send
		server_speed_counter--;

		// next frame
		frame++;

		//update dedserver wintitle
		if (fubie++ > 10) {

			server_kbps_traffic =
				server->get_socket_stat(NL_AVE_BYTES_RECEIVED) +
				server->get_socket_stat(NL_AVE_BYTES_SENT);
			server_kbps_traffic /= 1024.0;

			//update bar
			fubie = 0;
			char elbuf[128];
			sprintf(elbuf, "%i/%ip %.1fk/s v%s port:%i ESC:quit", player_count, maxplayers, server_kbps_traffic, GAME_VERSION, port);

			//V0.5.0 : -text  flag
			server_status_string(elbuf);
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

//a master job response is obtained: parse it
void gameserver_c::master_job_response(masterjob_c *j) {
	if (ctop[j->cid] == -1)	// client no longer connected
		return; 

	//LOG4("== MJOB RESPONSE : %i %i %i '''%s'''\n", j->code, j->cid, j->html_end, j->lebuf);

	int i;

	//1 -- player token check
	// RETORNO ESPERADO : @K<SCORE>#<RANKPOS>#

	bool got_a_final_response = false;		//if got a final response (@F/@E/@K)
																				//if not, will retry (e.g. getting "can't contact servlet runner"
																				//or other pages describing temporary problems in the master server)

	if (j->code == 1) {

		//parse response
		for (i=0;i<j->n;i++) {

			if (j->lebuf[i] == '@') {
				i++;

				if ((j->lebuf[i] == 'F') || (j->lebuf[i] == 'E')) {

					//FIXME: this is bad news -- deal better with it
					got_a_final_response = true;

				}
				if (j->lebuf[i] == 'K') {

					got_a_final_response = true;

					//OK!
					char lebuf[128]; int count = 0;
					writeByte(lebuf, count, 31);		//31 = registration NAME,TOKEN response
					writeByte(lebuf, count, 1);		// OK!
					server->send_message(j->cid, lebuf, count);

					//set this player as being recorded as of now
					client[j->cid].token_valid = true;		//validated his token

					//PARSE STUFF
					char pb[256];
					int  pc;
					//PARSE: current score
					pb[0]=0;
					pc=0;
					i++;
					while (j->lebuf[i] != '#') {
						pb[pc] = j->lebuf[i];
						pb[pc+1] = 0;
						pc++;
						i++;
						if (pc > 15) break;	//improbable length
					}
					client[j->cid].score = atoi(pb);
					//PARSE: current NEG score   // V0.4.8 ===
					pb[0]=0;
					pc=0;
					i++;
					while (j->lebuf[i] != '#') {
						pb[pc] = j->lebuf[i];
						pb[pc+1] = 0;
						pc++;
						i++;
						if (pc > 15) break;	//improbable length
					}
					client[j->cid].neg_score = atoi(pb);
					//LOG1("=== parsed SCORE '%s'\n", pb);
					//PARSE: current ranking pos
					pb[0]=0;
					pc=0;
					i++;
					while (j->lebuf[i] != '#') {
						pb[pc] = j->lebuf[i];
						pb[pc+1] = 0;
						pc++;
						i++;
						if (pc > 15) break;	//improbable length
					}
					client[j->cid].rank = atoi(pb);
					//LOG1("=== parsed RANKPOS '%s'\n", pb);
					//PARSE: max score
					pb[0]=0;
					pc=0;
					i++;
					while (j->lebuf[i] != '#') {
						pb[pc] = j->lebuf[i];
						pb[pc+1] = 0;
						pc++;
						i++;
						if (pc > 15) break;	//improbable length
					}
					max_world_score = atoi(pb);
					//PARSE: max rank pos
					pb[0]=0;
					pc=0;
					i++;
					while (j->lebuf[i] != '#') {
						pb[pc] = j->lebuf[i];
						pb[pc+1] = 0;
						pc++;
						i++;
						if (pc > 15) break;	//improbable length
					}
					max_world_rank = atoi(pb);

					//v0.4.5 : atualiza registration char / score / rank
					broadcast_player_crap( ctop [j->cid] );

					//done
					break;
				}
				else if (j->lebuf[i] == 'F') {
					//FAILED!
					char lebuf[128]; int count = 0;
					writeByte(lebuf, count, 31);		//31 = registration NAME,TOKEN response
					writeByte(lebuf, count, 0);		// FAILED!
					server->send_message(j->cid, lebuf, count);

					//done
					break;
				}
			}
		}
	}
	//2 -- submit a player's report
	// RETORNO ESPERADO : @K<SCORE>#<RANKPOS>#  (atualizados...)
	else if (j->code == 2) {
		//parse response
		for (i=0;i<j->n;i++) {

			if (j->lebuf[i] == '@') {
				i++;
				if ((j->lebuf[i] == 'F') || (j->lebuf[i] == 'E')) {

					//FIXME: this is bad news -- deal better with it
					got_a_final_response = true;

				}
				if (j->lebuf[i] == 'K') {

					got_a_final_response = true;

					//deltascore report: just update score/ranking

					//PARSE STUFF
					char pb[256];
					int  pc;
					//PARSE: current score
					pb[0]=0;
					pc=0;
					i++;
					while (j->lebuf[i] != '#') {
						pb[pc] = j->lebuf[i];
						pb[pc+1] = 0;
						pc++;
						i++;
						if (pc > 15) break;	//improbable length
					}
					client[j->cid].score = atoi(pb);
					//PARSE: current NEG score  v0.4.8 !! ======
					pb[0]=0;
					pc=0;
					i++;
					while (j->lebuf[i] != '#') {
						pb[pc] = j->lebuf[i];
						pb[pc+1] = 0;
						pc++;
						i++;
						if (pc > 15) break;	//improbable length
					}
					client[j->cid].neg_score = atoi(pb);
					//LOG1("=== parsed SCORE '%s'\n", pb);
					//PARSE: current ranking pos
					pb[0]=0;
					pc=0;
					i++;
					while (j->lebuf[i] != '#') {
						pb[pc] = j->lebuf[i];
						pb[pc+1] = 0;
						pc++;
						i++;
						if (pc > 15) break;	//improbable length
					}
					client[j->cid].rank = atoi(pb);
					//LOG1("=== parsed RANKPOS '%s'\n", pb);
					//PARSE: max score
					pb[0]=0;
					pc=0;
					i++;
					while (j->lebuf[i] != '#') {
						pb[pc] = j->lebuf[i];
						pb[pc+1] = 0;
						pc++;
						i++;
						if (pc > 15) break;	//improbable length
					}
					max_world_score = atoi(pb);
					//PARSE: max rank pos
					pb[0]=0;
					pc=0;
					i++;
					while (j->lebuf[i] != '#') {
						pb[pc] = j->lebuf[i];
						pb[pc+1] = 0;
						pc++;
						i++;
						if (pc > 15) break;	//improbable length
					}
					max_world_rank = atoi(pb);

					//v0.4.5 : atualiza registration char / score / rank
					broadcast_player_crap( ctop [j->cid] );

					//done
					break;
				}
				else if (j->lebuf[i] == 'F') {

					//FIXME: something went wrong, DELTA SCORE LOST !!!
					//  log this, must check reasons later
					LOG("ERROR : A PLAYER DELTA SCORE UPDATE HAS BEEN LOST! (@F returned)!\n");

					break;
				}
			}
		}
	}

	//no definitive response: should retry
	if (got_a_final_response == false)
		j->retry = true;
}

//master job -- handle a single request
void gameserver_c::run_masterjob_thread(void *arg) {

	//increment job count FIXME: this shouldn't be here really... must place it BEFORE
	//creating the thread
	pthread_mutex_lock ( &mjob_mutex );
	mjob_count++;
	pthread_mutex_unlock ( &mjob_mutex );

	//the rune
	masterjob_c *job = (masterjob_c*)arg;

	int w; //wait

	NLsocket sock = NL_INVALID;

	while (mjob_exit == false) {

		//open a nonblocking socket
		nlDisable(NL_BLOCKING_IO);
		sock = nlOpen(0, NL_RELIABLE);
		if (sock == NL_INVALID) {
			//FIXME show "cant open socket to master" error
			for (w=0;w<60*2*2;w++) { if (mjob_exit) break; if (mjob_fastretry) break; MS_SLEEP(500); }
			continue;				//again...
		}

		//connect the nonblocking way
		nlConnect(sock, &master_address);

		//build query
		char querybuf[1024]; int qcount = 0;
		writeString(querybuf, qcount, job->request);
		qcount--;	//take the zero out

		//FIXME: LOG PROGRESS

		//keep trying to write the query.
		NLint result;
		do {
			result = nlWrite(sock, querybuf, qcount);
			MS_SLEEP(50);

			//qutting?
			if (mjob_exit) break;

		} while ((result == NL_INVALID) && (nlGetError() == NL_CON_PENDING));

		//qutting?
		if (mjob_exit) continue;

		//FIXME: LOG PROGRESS

		//try to read the reply
		//parse the response (should be <HTML><BODY> etc... with "@I @I @I ... @K" on it
		int nostuffcound = 0;
		int n = 0;
		char *lebuf = &(job->lebuf[0]);
		do {

			//read
			result = nlRead(sock, &(lebuf[n]), 1);

			//quitting?
			if (mjob_exit) break;

			//no byte
			if (result == 0) {

				if (nostuffcound > 0) {
					nostuffcound++;
					//200 (4000*50/1000) seconds after it came some stuff but now without coming more stuff
					// THEN: retry in a while
					if (nostuffcound > 4000) {
						//retry
						lebuf[0] = 0;
						nlClose(sock);
						sock = NL_INVALID;
						//FIXME: LOG PROGRESS strcpy(namestatus, "NO RESPONSE. RETRYING...");
						for (w=0;w<3*2;w++) { if (mjob_exit) break; if (mjob_fastretry) break; MS_SLEEP(500); }
						break;
					}
				}

				MS_SLEEP(50);
			}

			//error occured
			if (result == NL_INVALID) {

				//if already got html_end, no error
				//if (html_end)  // *** FIXME: parsing the result?
				//break;

				//error: try again
				nlClose(sock);
				sock = NL_INVALID;
				//FIXME: LOG PROGRESS strcpy(namestatus, "ERROR. RETRYING...");
				for (w=0;w<3*2;w++) { if (mjob_exit) break; if (mjob_fastretry) break; MS_SLEEP(500); }
				break;
			}

			//received anything below 32: turn them into "+" signals...
			if (lebuf[n] < 32)
				lebuf[n] = '+';

			//check for received </HTML>
			if (n >= 6) {
				if (
					(lebuf[n-6] == '<') &&
					(lebuf[n-5] == '/') &&
					((lebuf[n-4] == 'h') || (lebuf[n-4] == 'H')) &&
					((lebuf[n-3] == 't') || (lebuf[n-3] == 'T')) &&
					((lebuf[n-2] == 'm') || (lebuf[n-2] == 'M')) &&
					((lebuf[n-1] == 'l') || (lebuf[n-1] == 'L')) &&
					(lebuf[n-0] == '>')
				)
				{
					//LOG1("CLIENT MASTER QUERY RECEIVED </HTML>! SUCCESS!! n=%i\n", n);
					job->html_end = true;
					lebuf[n+1] = 0;
					//LOG1("---- Full response START ----\n%s\n---- Full response END ----\n", lebuf);
					break;
				}
			}

			//check for received another <HTML> : reset all stuff
			if (n >= 5) {
				if (
					(lebuf[n-5] == '<') &&
					((lebuf[n-4] == 'h') || (lebuf[n-4] == 'H')) &&
					((lebuf[n-3] == 't') || (lebuf[n-3] == 'T')) &&
					((lebuf[n-2] == 'm') || (lebuf[n-2] == 'M')) &&
					((lebuf[n-1] == 'l') || (lebuf[n-1] == 'L')) &&
					(lebuf[n-0] == '>')
				)
				{
					lebuf[n+1]=0;
					//LOG1("\n** READ <HTML>, DISCARDING BUFFER '%s' **\n", lebuf);
					n = -1;
				}
			}

			//read next
			n++;

		} while (1);

		//save n
		job->n = n;

		//quitting?
		if (mjob_exit) break;

		//found it?
		if (job->html_end) {

			//FIRST THINGS FIRST: close the socket
			nlClose(sock);
			sock = NL_INVALID;

			//job completed: who to call?
			job->retry = false;
			master_job_response(job);

			//check for retry -- 2 minutes wait before doing so
			if (job->retry)
				for (w=0;w<60*2*2;w++) { if (mjob_exit) break; if (mjob_fastretry) break; MS_SLEEP(500); }
			else
				break;	// ALL DONE !!
		}
		else {
			//failed, just retry (go on with the loop)
		}

	}//WHILE(password set)

	// =====
	//CLOSE SOCKET AND DECREMENT JOB COUNT
	// =====
	if (sock != NL_INVALID)
		nlClose(sock);
	pthread_mutex_lock ( &mjob_mutex );
	mjob_count--;
	pthread_mutex_unlock ( &mjob_mutex );

	//job completed -- nuke it
	delete job;
}

//check for private IP
bool gameserver_c::check_private_IP(char *address) {
	int i1, i2;
	int n=sscanf(address, "%d.%d.", &i1, &i2);
	assert(n==2);
	if (n != 2)
		return false;
	// private IP ranges:
	// 10.0.0.0        -   10.255.255.255
	// 172.16.0.0      -   172.31.255.255
	// 192.168.0.0     -   192.168.255.255
	// 169.254.0.0     -   169.254.255.255 [used by Microsoft DHCP client]
	return (i1==10 || (i1==172 && i2>=16 && i2<=31) || (i1==192 && i2==168) || (i1==169 && i2==254));
}

//master server talker thread
void gameserver_c::run_mastertalker_thread(void *) {

	//FIXME: generate a decent password here

	LOG("run_mastertalker_thread()\n");

	//get my IP
	//V0.4.4: -ip : force IP to something else
	char address[256];
	if (force_ip) {
		strcpy(address, force_ip_name);		//force IP

		LOG1("FORCING IP TO VALUE %s\n", force_ip_name);
	}
	else {
		//don't force: resolve
		NLaddress myadr;
		get_self_IP(&myadr);
		nlAddrToString(&myadr, address);

		//strcpy(address, "192.168.1.1");  //DEBUG private ip

		//v0.4.7: check for "private class IPs":
		bool privateip = check_private_IP(address);

		//LOG2("CHECKED DEFAULT IP %s RESULT = %i\n", address, privateip);

		//private ip?
		if (privateip) {

			//don't despair! check for all IPs
			NLaddress *locals;
			NLint     locsize;
			locals = nlGetAllLocalAddr(&locsize);

			for (int z=0;z<locsize;z++) {
				nlAddrToString( &(locals[z]) , address );
				LOG1("CHECKING LOCAL : %s ... ", address);
				privateip = check_private_IP(address);
				if (!privateip)	{
					LOG("NOT PRIVATE! this is good\n");
					break;	//success! "address" now has non-private address string
				}
				else
					LOG("PRIVATE! sucks... trying next...\n");
			}

			//still??
			if (privateip) {
				LOG1("PRIVATE IP: %s, (and all others): not talking to master server...\n", address);
				msock = NL_INVALID;	//???
				master_exiting_ok = true;
				return;
			}
		}
	}

	//v0.4.2: add port
	char sport[200];
	sprintf(sport, ":%i", port);
	strcat(address, sport);

	//while not time to quit
	while (!file_threads_quit) {

		//time to update with master server?
		if (get_time() > master_talk_time) {

			//LOG("MASTER TALKER: time to talk\n");

			//schedule next
			master_talk_time = get_time() + 3 * 60.0 ;		//3 minutes

			//open socket
			nlEnable(NL_BLOCKING_IO);
			msock = nlOpen(0, NL_RELIABLE);
			nlDisable(NL_BLOCKING_IO);
			if (msock == NL_INVALID) {
				LOG("SERVER CAN'T OPEN SOCKET TO CONNECT TO MASTER SERVER!!\n");
				continue;
			}

			//LOG("MASTER TALKER: socket open\n");

			//connect
			//NLaddress masadr;
			//nlGetAddrFromName("www.mycgiserver.com", &masadr);	//www.mycgiserver.com
			//nlStringToAddr("212.69.162.53", &masadr);
			//nlSetAddrPort(&masadr, 80);													//port 80
			if (nlConnect(msock, &master_address) == NL_FALSE) {		//connect
				LOG("SERVER CAN'T CONNECT TO WWW.MYCGISERVER.COM:80 !!!\n");
				nlClose(msock);
				msock = NL_INVALID;
				continue;
			}

			//LOG("MASTER TALKER: socket connected\n");

			//built the state
			char state[1024];
			sprintf(state, "%i/%i players - %s - v%s", player_count, maxplayers, hostname, GAME_VERSION);
			for (int h=0; state[h]; h++)
				if (state[h] == ' ')	//switch spaces to plus'es
					state[h] = '+';

			//build the GET request
			char query[1024];
			sprintf(query, "GET /servlet/fcecin.m3/index.html?add=%s&pass=1111&st=%s\n\n", address, state);
			char lebuf[65536]; int count = 0;
			writeString(lebuf, count, query);
			//erase the 0
			count--;

			//chance to give up
			if (file_threads_quit)
				break;

			//now we have talked
			master_never_talked = false;

			//send it
			NLint result = nlWrite(msock, lebuf, count);
			//LOG3("WROTE TO MASTER '%s', result = %i, count = %i\n", query, result, count);
			//LOG2("MASTER TALKER: wrote to master %i,%i", result, count);

			//parse the response (should be <HTML><BODY> etc... with "@K" on it
			int n=0;
			double timeout = get_time() + 60.0;
			do {

				//read
				result = nlRead(msock, &(lebuf[n]), 1);
				if (result != 1) {
					LOG1("MASTER TALKER: ERROR r=%i\n", result);
					break;
				}

				//check for received </HTML>
				if (n > 8) {
					if (
						(lebuf[n-6] == '<') &&
						(lebuf[n-5] == '/') &&
						((lebuf[n-4] == 'h') || (lebuf[n-4] == 'H')) &&
						((lebuf[n-3] == 't') || (lebuf[n-3] == 'T')) &&
						((lebuf[n-2] == 'm') || (lebuf[n-2] == 'M')) &&
						((lebuf[n-1] == 'l') || (lebuf[n-1] == 'L')) &&
						(lebuf[n-0] == '>')
					)
					{

						//LOG("MASTER TALKER: </HTML>\n");
						lebuf[n+1] = 0;
						//LOG1("---- Full response START ----\n%s\n---- Full response END ----\n", lebuf);
						break;
					}
				}

				//read next
				n++;

				//quit if timeout
				if (get_time() > timeout)
					break;

				//quit if told to
				if (file_threads_quit)
					break;

			} while (1);

			//close socket
			nlClose(msock);
			msock = NL_INVALID;
		}

		//no: sleep a bit
		MS_SLEEP(500);
	}

	LOG("MASTER TALKER: time to say goodbye\n");

	//master is pre-exiting, no need to do the first socket closure
	master_pre_exiting_ok = true;

	//qutting: close the socket
	if (msock != NL_INVALID) {
		LOG("MASTER TALKER: bye 1\n");
		nlClose(msock);
		msock = NL_INVALID;
	}

	//never talked? then just quit
	if (master_never_talked) {
		//master exited OK!
		master_exiting_ok = true;
		return;
	}

	LOG("MASTER TALKER: bye 2\n");

	//quitting: delete my IP from master so clients won't see it
	//open socket
	nlDisable(NL_BLOCKING_IO);			//nonblocking socket, let's make this simple...
	msock = nlOpen(0, NL_RELIABLE);

	if (msock == NL_INVALID) {
		LOG("(QUIT) SERVER CAN'T OPEN SOCKET TO CONNECT TO MASTER SERVER!!\n");
		return;
	}

	//connect
	//NLaddress masadr;
	//nlGetAddrFromName("www.mycgiserver.com", &masadr);	//www.mycgiserver.com
	//nlStringToAddr("212.69.162.53", &masadr);
	//nlSetAddrPort(&masadr, 80);													//port 80
	if (nlConnect(msock, &master_address) == NL_FALSE) {		//connect
		LOG("(QUIT) SERVER CAN'T CONNECT TO WWW.MYCGISERVER.COM:80 !!!\n");
		nlClose(msock);
		msock = NL_INVALID;
		return;
	}

	//build the GET request
	char query[1024];
	sprintf(query, "GET /servlet/fcecin.m3/index.html?del=%s&pass=1111\n\n", address);
	char lebuf[65536]; int count = 0;
	writeString(lebuf, count, query);
	//erase the 0
	count--;

	//send it

	NLint result;
	double querytimeout = get_time() + 5.0;

	do {
		MS_SLEEP(50);
		result = nlWrite(msock, lebuf, count);
	} while ((result == NL_INVALID) && (get_time() < querytimeout));

	if (get_time() >= querytimeout) {

		LOG("(QUIT) query timeout\n");

		//master exited OK!
		master_exiting_ok = true;

		//close socket
		nlClose(msock);
		msock = NL_INVALID;
		return;
	}

	LOG3("(QUIT) WROTE TO MASTER '%s', result = %i, count = %i\n", query, result, count);

	//parse the response (should be <HTML><BODY> etc... with "@K" on it
	double timeout = get_time() + 5.0;
	int n=0;
	do {

		//read
		result = nlRead(msock, &(lebuf[n]), 1);
		if (result == NL_INVALID) {
			LOG1("(QUIT) ERROR READING RESPONSE result = %i\n", result);
			break;
		}
		//timeout?
		if (get_time() > timeout)
			break;
		//nothing to read
		if (result == 0) {
			MS_SLEEP(10);
			continue;
		}

		//check for received </HTML>
		if (n > 8) {
			if (
				(lebuf[n-6] == '<') &&
				(lebuf[n-5] == '/') &&
				((lebuf[n-4] == 'h') || (lebuf[n-4] == 'H')) &&
				((lebuf[n-3] == 't') || (lebuf[n-3] == 'T')) &&
				((lebuf[n-2] == 'm') || (lebuf[n-2] == 'M')) &&
				((lebuf[n-1] == 'l') || (lebuf[n-1] == 'L')) &&
				(lebuf[n-0] == '>')
			)
			{
				LOG("(QUIT) RECEIVED </HTML>! SUCCESS!!\n");
				lebuf[n+1] = 0;
				//LOG1("(QUIT) ---- Full response START ----\n%s\n(QUIT) ---- Full response END ----\n", lebuf);
				break;
			}
		}

		//read next
		n++;

	} while (1);

	//master exited OK!
	master_exiting_ok = true;

	//close socket
	nlClose(msock);
	msock = NL_INVALID;
}

//read a string from a blocking TCP stream, one char at a time
bool gameserver_c::read_string_from_TCP(NLsocket sock, char *buf) {
	for (;;) {
		NLint result = nlRead(sock, buf, 1);
		if (result != 1)	//interrupted
			return false;
		if (*buf == '\0')
			return true;
		++buf;
	}
}

//run a admin shell master thread
void gameserver_c::run_shellmaster_thread(void *) {

	LOG("\nrun_shellmaster_thread() STARTED\n");

	while (1) {
		//accept one connection
		nlEnable(NL_BLOCKING_IO);
		NLsocket pidaosock = nlAcceptConnection(shellmsock);
		nlDisable(NL_BLOCKING_IO);

		//valid socket?
		if (pidaosock != NL_INVALID) {

			LOG("\npidaosock NOT INVALID! incoming SHELL CONNECTION!\n");

			//accept connections only from localhost
			NLaddress addr, c1, c2;
			nlGetRemoteAddr(pidaosock, &addr);
			nlStringToAddr("127.0.0.1", &c1);
			get_self_IP(&c2);
			nlSetAddrPort(&addr, 0);
			nlSetAddrPort(&c1, 0);
			nlSetAddrPort(&c2, 0);

			if ((nlAddrCompare(&addr, &c1) == NL_FALSE) && (nlAddrCompare(&addr, &c2) == NL_FALSE)) {
				LOG("\nWARNING: attempt to connect a remote admin shell blocked!\n");
				nlClose(pidaosock);
				continue;
			}

			//if already connected, skip
			if (shellssock != NL_INVALID)	{ //skip
				LOG("\nWARNING: attempt to connect two simultaneous admin shells blocked!\n");
				nlClose(pidaosock);
				continue;
			}

			LOG("**** ADMIN SHELL SOCKET CONNECTED ON LOOPBACK PORT! ******\n");

			//ADMIN SHELL just connected: tell about the current situation!
			//
			char lebuf[4096]; int count = 0;
			for (int i=0;i<maxplayers;i++)
			if (player[i].used)
			{
				writeLong(lebuf, count, STA_PLAYER_CONNECTED);////1 .... player connected <int id>
				writeLong(lebuf, count, player[i].cid);
				writeLong(lebuf, count, STA_PLAYER_FRAGS);
				writeLong(lebuf, count, player[i].cid);
				writeLong(lebuf, count, player[i].frags);
				writeLong(lebuf, count, STA_PLAYER_NAME_UPDATE);
				writeLong(lebuf, count, player[i].cid);
				writeString(lebuf, count, player[i].name);
				writeLong(lebuf, count, STA_PLAYER_IP);
				writeLong(lebuf, count, player[i].cid);
				char addrBuf[50];
				NLaddress addr=server->get_client_address(player[i].cid);
				nlSetAddrPort(&addr, 0);
				nlAddrToString(&addr, addrBuf);
				writeString(lebuf, count, addrBuf);
			}
			nlWrite(pidaosock, lebuf, count);

			//keep socket so it can be closed. this assignment also will reflect
			// on the execution of run_shellslave_thread()
			shellssock = pidaosock;
		}
		else {
			if (file_threads_quit) {		//quitting!
				LOG("SHELLMASTER THREAD IS QUITTING\n");
				nlClose(shellmsock);
				return;
			}
			//else
			//LOG("(SH)");
		}

		//sleep a bit
		MS_SLEEP(500);
	}
}

//run an admin shell slave thread
void gameserver_c::run_shellslave_thread(void *) {

	LOG("run_shellslave_thread() STARTED\n");

	while (1) {

		// valid socket?
		if (shellssock != NL_INVALID) {

			//LOG("SLAVE SHELLSOCK READING MESSAGE...\n");

			//read request code
			NLint code, pid, clid, arg;
			char rbuf[256]; int rcount = 0;
			NLint result = nlRead(shellssock, rbuf, 4);
			rcount = 0; readLong(rbuf, rcount, code);

			//a zero result may be connection not ready yet (?)
			if (result == 0) {
				MS_SLEEP(10);
				continue;
			}

			LOG2("READ RESULT = %i VALUE = %i\n", result, code);

			if (result == NL_INVALID) {
				LOG4("RESULT IS NL_INVALID. errors are %i %s %i %s\n", nlGetError(), nlGetErrorStr(nlGetError()), nlGetSystemError(), nlGetSystemErrorStr(nlGetSystemError()));
				LOG("SLAVE CONNECTION RESET (1)\n");
				if (shellssock != NL_INVALID) {
					nlClose(shellssock);
					shellssock = NL_INVALID;
				}
				continue;
			}

			if (file_threads_quit) {
				LOG("SLAVE QUIT (2)\n");
				break; //quitting...
			}

			if (result != 4) {
				LOG("SLAVE CONNECTION RESET (2)\n");
				if (shellssock != NL_INVALID) {
					nlClose(shellssock);
					shellssock = NL_INVALID;
				}
				continue;
			}//error occured: end here

			bool should_quit = false;
			bool answer = false;
			char lebuf[1024];
			char chat[2048];
			char lechat[2048];
			int count = 0, delta;

			//parse it
			switch (code) {
			case ATS_NOOP:						//0= no-op
				break;
			case ATS_GET_PLAYER_FRAGS:		//1... request the frags amount of a player <int id>
				result = nlRead(shellssock, rbuf, 4);
				rcount = 0; readLong(rbuf, rcount, clid); pid = ctop[clid];
				if (result == 4) {
					if (player[pid].used) {
						answer = true; //ADMIN SHELL
						writeLong(lebuf, count, STA_PLAYER_FRAGS);
						writeLong(lebuf, count, player[pid].cid);
						writeLong(lebuf, count, player[pid].frags);
					}
				}
				break;
			case ATS_GET_PLAYER_TOTAL_TIME:		//request the frags amount of a player <int id>
				result = nlRead(shellssock, rbuf, 4);
				rcount = 0; readLong(rbuf, rcount, clid); pid = ctop[clid];
				if (result == 4) {
					if (player[pid].used) {
						answer = true;
						writeLong(lebuf, count, STA_PLAYER_TOTAL_TIME);
						writeLong(lebuf, count, player[pid].cid);
						delta = (int)(get_time() - player[pid].start_time);
						writeLong(lebuf, count, delta);
					}
				}
				break;
			case ATS_GET_PLAYER_TOTAL_KILLS:		//request the total kills amount of a player <int id>
				result = nlRead(shellssock, rbuf, 4);
				rcount = 0; readLong(rbuf, rcount, clid); pid = ctop[clid];
				if (result == 4) {
					if (player[pid].used) {
						answer = true;//ADMIN SHELL
						writeLong(lebuf, count, STA_PLAYER_TOTAL_KILLS);
						writeLong(lebuf, count, player[pid].cid);
						writeLong(lebuf, count, player[pid].total_kills);
					}
				}
				break;
			case ATS_GET_PLAYER_TOTAL_DEATHS:		//request the total deaths amount of a player <int id>
				result = nlRead(shellssock, rbuf, 4);
				rcount = 0; readLong(rbuf, rcount, clid); pid = ctop[clid];
				if (result == 4) {
					if (player[pid].used) {
						answer = true;//ADMIN SHELL
						writeLong(lebuf, count, STA_PLAYER_TOTAL_DEATHS);
						writeLong(lebuf, count, player[pid].cid);
						writeLong(lebuf, count, player[pid].total_deaths);
					}
				}
				break;
			case ATS_GET_PLAYER_TOTAL_CAPTURES:		//request the total captures amount of a player <int id>				}
				result = nlRead(shellssock, rbuf, 4);
				rcount = 0; readLong(rbuf, rcount, clid); pid = ctop[clid];
				if (result == 4) {
					if (player[pid].used) {
						answer = true;//ADMIN SHELL
						writeLong(lebuf, count, STA_PLAYER_TOTAL_CAPTURES);
						writeLong(lebuf, count, player[pid].cid);
						writeLong(lebuf, count, player[pid].total_captures);
					}
				}
				break;
			case ATS_SERVER_CHAT:									//server is saying <string chat line>
				read_string_from_TCP(shellssock, (char *)chat);
				sprintf(lechat, "ADMIN: %s", chat);
				broadcast_message(lechat);
				break;
			case ATS_GET_PINGS:
				for (int p=0; p<maxplayers; ++p)
					if (player[p].used) {
						answer=true;
						writeLong(lebuf, count, STA_PLAYER_PING);
						writeLong(lebuf, count, player[p].cid);
						writeLong(lebuf, count, player[p].ping);
					}
				break;
			case ATS_MUTE_PLAYER:
				result = nlRead(shellssock, rbuf, 8);
				rcount = 0; readLong(rbuf, rcount, clid); pid = ctop[clid];
				readLong(rbuf, rcount, arg);
				if (result == 8)
					mutePlayer(pid, arg);
				break;
			case ATS_KICK_PLAYER:
				result = nlRead(shellssock, rbuf, 4);
				rcount = 0; readLong(rbuf, rcount, clid); pid = ctop[clid];
				if (result == 4)
					kickPlayer(pid);
				break;
			#ifdef SV_NAME_AUTHORIZATION
			case ATS_BAN_PLAYER:
				result = nlRead(shellssock, rbuf, 4);
				rcount = 0; readLong(rbuf, rcount, clid); pid = ctop[clid];
				if (result == 4)
					banPlayer(pid);
				break;
			#endif
			case ATS_RESET_SETTINGS:
				{
					string currMapFile = maprot[currmap].file;
					reset_settings();
					size_t mapi;
					for (mapi=0; mapi<maprot.size(); ++mapi)
						if (maprot[mapi].file == currMapFile)
							break;
					if (mapi == maprot.size()) {	// not found
						currmap = -1;
						server_next_map(NEXTMAP_VOTE_EXIT);
					}
					break;
				}
			case ATS_QUIT:
				should_quit = true;
				break;
			}

			//quitting?
			if (should_quit) {
				LOG("SLAVE ATS_QUIT (connection reset)\n");
				if (shellssock != NL_INVALID) {
					nlClose(shellssock);
					shellssock = NL_INVALID;
				}
				continue;
			}

			//quitting...
			if (file_threads_quit) {
				LOG("SLAVE QUIT (4)\n");
				break;
			}

			//send answer to the shell
			if (answer) {
				result = nlWrite(shellssock, lebuf, count);
			}
		}
		//not valid
		else {
			//sleep a bit
			MS_SLEEP(1000);
		}

		//quitting...
		if (file_threads_quit) {
			LOG("SLAVE QUIT (5)\n");
			break;
		}
	}

	//quitting
	LOG("ADMIN SHELL SLAVE THREAD QUITTING... CLOSING SOCKET\n");
	if (shellssock != NL_INVALID) {
		nlClose(shellssock);
		shellssock = NL_INVALID;
	}
}

//stop server
void gameserver_c::stop() {

	//submit all pending player reports
	int i,cid;
	for (i=0;i<maxplayers;i++)
	if (player[i].used)
	{
		cid = player[i].cid;
		client_report_status(cid);
	}

	//v0.4.4 : flag master job threads to start trying to resolve themselves quickly
	mjob_fastretry = true;
	double mjmaxtime = get_time() + 30.0;		//timeout : 30 seconds

	server_status_string("Shutdown: Net Server");

	if (server)
		server->stop(3);
	else
		throw 8384;

	//reset client_c struct (closes files...)
	for (i=0;i<MAX_PLAYERS;i++)
		client[i].reset();

	// flag so threads will quit themselves
	master_pre_exiting_ok = false;
	master_exiting_ok = false;
	file_threads_quit = true;	//quit stuff now

	//close TCP connection with the server admin shell
	//
	server_status_string("Shutdown: MSHELL Thread");

	LOG("GAMESERVER JOINING SHELL-MASTER THREAD...\n");
	pthread_join( shellmthread, 0 );

	server_status_string("Shutdown: SSHELL Socket");

	LOG("GAMESERVER CLOSING SHELL-SLAVE SOCKET...\n");
	if (shellssock != NL_INVALID)
		nlClose( shellssock );

	server_status_string("Shutdown: SSHELL Thread");

	LOG("GAMESERVER JOINING SHELL-SLAVE THREAD...\n");
	pthread_join( shellsthread, 0 );

	//file downlaod to clients threads..
	//

	//v0.4.4 : DO NOT open tcp download port if (-notcp) enabled
	//
	if (no_tcp_download) {
		LOG("SKIPPING TCP FILE SOCKETS/THREADS (-notcp....)");
	}
	else {

		LOG("GAMESERVER CLOSING FILEMASTER'S SOCKET...\n");

		server_status_string("Shutdown: MFILE Socket");

		nlClose(filesock);
		LOG("GAMESERVER STOP JOIN FILEMASTER...\n");

		server_status_string("Shutdown: MFILE Thread");

		pthread_join( server_filemaster_thread , 0 );
		LOG("OK!\n");

		pthread_mutex_lock( &fslavesock_mutex );

		for (i=0;i<MAX_PLAYERS;i++)
			if (fslavesock[i] != NL_INVALID) {
				server_status_string("Shutdown: SFILE Socket");
				nlClose(fslavesock[i]);
				LOG2("GAMESERVER STOP JOIN FILESLAVE %i %i...", i, (int)fslavesock[i]);
				server_status_string("Shutdown: SFILE Thread");
				if (fslavethr[i] != (pthread_t)-1)
					pthread_join ( fslavethr[i] , 0 );
				LOG("OK!\n");
			}

		pthread_mutex_unlock( &fslavesock_mutex );
	}

	//thread for TCP connection that server uses to register it's IP on the master-server
	//
	if (!privateserver) {

		//MS_SLEEP(1000);	//wait a bit for the slave thread to catch up

		if (!master_pre_exiting_ok) {
			if (msock != NL_INVALID) {
				LOG("GAMESERVER CLOSING MASTER SERVER SOCKET...\n");
				server_status_string("Shutdown: MASTER Socket 1");
				nlClose(msock);
				msock = NL_INVALID;
			}
			else
				LOG("GAMESERVER CLOSING MASTER SERVER SOCKET (BUT IT HAS ALREADY BEEN DISCONNECTED AND SENDING DISCONNECT TO MASTER)...\n");
		}

		//MS_SLEEP(1000);	//wait a bit for the slave thread to catch up

		LOG("GAMESERVER JOINING MASTER SERVER THREAD...\n");

		server_status_string("Shutdown: MASTER Thread");

		int waitcount = 0;
		do {
			MS_SLEEP(100);
			if (master_exiting_ok) {	//done!
				LOG("IT EXITED...\n");
				break;
			}

			waitcount++;
			if (waitcount > 30) {		// 30 * 100ms = 3 seconds
				LOG("TIRED OF WAITING...\n");
				//kill the socket
				server_status_string("Shutdown: MASTER Socket 2");
				nlClose(msock);		//close AGAIN (it's a different one)
				msock = NL_INVALID;
				break;
			}

		} while (1);

		//wait for all master jobs to complete nicely
		while ( (mjob_count > 0) && ( get_time() < mjmaxtime )) {
			char lix[200];
			sprintf(lix, "Shutdown: MJOBS %i left", mjob_count);
			server_status_string(lix);
			MS_SLEEP(100);
		}

		//clean up jobs
		mjob_exit = true;		//MUST terminate -- abort
		while (mjob_count > 0) {
			char lix[200];
			sprintf(lix, "Shutdown: MJOBS ABORT %i left", mjob_count);
			server_status_string(lix);
			MS_SLEEP(100);
		}

		//NOW one can join with the thread without fear
		LOG("DE FACTO JOIN...\n");

		server_status_string("(Kill this window if not closing)");

		pthread_join( mthread , 0 );
	}


}

//get hostname: for hello
char* gameserver_c::get_hostname() {
	return hostname;
}

int sfunc_client_hello(runes_t *arg) {

	//FIXME: this is returned by the function! should be using one per client
	static char lebuf[128];
	lebuf[127]=0;	//paranoia

	//LOG1("hello client %i!\n", arg->client_id);
	static runes_t result;

	//check versions
	char stri[256];
	char temp[256];
	int count = 0;

	/*LOG("args bytevalues : ");
	for (int i=0;i<arg->length;i++) {
		LOG1("%i.\n", arg->data[i] );
	}
	LOG("args charvalues : ");
	for (i=0;i<arg->length;i++) {
		LOG1("%c.\n", arg->data[i] );
	}*/

	readString(arg->data, count, stri);	//read gamestring

	if (strcmp(stri, GAME_STRING)) {

		LOG2("GAME STRINGS DONT MATCH: %s and %s\n", GAME_STRING, stri);
		result.client_id = -1;		// not accepted

		sprintf(temp, "Different game: '%s'", stri);
		count = 0;
		writeString(lebuf, count, temp);

		result.data = &(lebuf[0]);			//custom deny data
		result.length = count;
		//result.data = 0;		//no custom data
		//result.length = 0;	//no custom data
	}
	else {

		readString(arg->data, count, stri);	//read protocol string

		if (strcmp(stri, GAME_PROTOCOL)) {

			LOG2("GAME PROTOCOL STRINGS DONT MATCH: %s and %s\n", GAME_PROTOCOL, stri);
			result.client_id = -1;		// not accepted
			//result.data = 0;		//no custom data
			//result.length = 0;	//no custom data

			sprintf(temp, "Protocol mismatch: server: %s, client: %s", GAME_PROTOCOL, stri);
			count = 0;
			writeString(lebuf, count, temp);

			result.data = &(lebuf[0]);			//custom deny data
			result.length = count;
		}
		else if (player_count >= maxplayers) {		//server full!

			LOG("....unfortunatelly the server is FULL! hello rejected\n");
			result.client_id = -1;		// not accepted
			//result.data = 0;		//no custom data
			//result.length = 0;	//no custom data

			sprintf(temp, "Server is full. (%i players)", player_count);
			count = 0;
			writeString(lebuf, count, temp);

			result.data = &(lebuf[0]);			//custom deny data
			result.length = count;
		}
		#ifdef SV_NAME_AUTHORIZATION
		else if (gameserver->authorizations.isBanned(gameserver->server->get_client_address(arg->client_id))) {
			result.client_id = -1;	// not accepted
			count=0;
			writeString(lebuf, count, "You are banned");
			result.data = lebuf;
			result.length = count;
		}
		#endif
		else {

			result.client_id = arg->client_id;		//this means "accept the connection"
			//custom data:
			//   BYTE MAXPLAYERS
			//   STRING hostname
			//
			int count = 0;
			writeByte(lebuf, count, ((NLubyte)maxplayers));
			writeString(lebuf, count, gameserver->get_hostname());
			if (no_tcp_download)
				writeByte(lebuf, count, 1);							//V0.4.4 NEW: server's NOTCP flag value. 0=off 1=on
			else
				writeByte(lebuf, count, 0);							//V0.4.4 NEW: server's NOTCP flag value. 0=off 1=on

			//strcpy(lebuf, gameserver->get_hostname());
			//result.data = &(lebuf[0]);
			//result.length = strlen(lebuf)+1;	//inclui o zero
			result.data = &(lebuf[0]);
			result.length = count;	//inclui o zero
		}
	}

	return (int)(&result);
}

int sfunc_client_connected(runes_t *arg) {

	//LOG1("client connected %i\n", arg->client_id);

	gameserver->client_connected(arg->client_id);

	return 0;
}

int sfunc_client_disconnected(runes_t *arg) {

	//LOG1("client disconnected %i\n", arg->client_id);

	gameserver->client_disconnected(arg->client_id);

	return 0;
}

int sfunc_client_lag_status(runes_t *) {

	//LOG2("client %i lagstatus %i\n", arg->client_id, arg->status);

	return 0;
}

int sfunc_client_data(runes_t *arg) {

	//LOG2("client %i data=%i\n", arg->client_id, arg->length);

	//process it
	gameserver->incoming_client_data(arg->client_id, arg->data, arg->length);

	return 0;
}

int sfunc_client_ping_result(runes_t *arg) {
	gameserver->ping_result(arg->client_id, arg->pingtime);

	return 0;
}

//============================================================
//  TCP server admin shell interaction thread
//============================================================

//thread for server shell connections
void *thread_shellmaster_f(void *arg) {

	gameserver->run_shellmaster_thread(arg);

	return 0;
}

void *thread_shellslave_f(void *arg) {

	gameserver->run_shellslave_thread(arg);

	return 0;
}

//============================================================
//  TCP master server (web servlet) interaction thread -- LIST SERVER
//============================================================

//thread for master server interaction
void *thread_mastertalker_f(void *arg) {

	gameserver->run_mastertalker_thread(arg);

	return 0;
}

//============================================================
//  a single independent job to the master server
//============================================================

void *thread_masterjob_f(void *arg) {

	gameserver->run_masterjob_thread(arg);

	return 0;
}

//============================================================
//  TCP file server threads
//============================================================

void *thread_filemaster_f(void *arg) {

	gameserver->run_filemaster_thread(arg);

	return 0;
}

void *thread_fileslave_f(void *arg) {

	gameserver->run_fileslave_thread(arg);

	return 0;
}

//============================================================
//  listen server thread
//============================================================

int listen_port_running;
volatile bool	listen_server_running = false;
pthread_t	listen_server_thread;

void *thread_listenserver_f(void *) {
	srand(time(0));

	//save for display
	listen_port_running = port;		//port selectr

	//(1) start the localserver
	//
	gameserver = new gameserver_c();
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
	LOG("GAMESERVER STOPPING");
	gameserver->stop();
	LOG("GAMESERVER DELETING");
	delete gameserver;
	LOG("GAMESERVER DELETED");
	gameserver = 0;

	//restore client's windowtitle
	server_status_string("Outgun client - CTRL+F12 to quit");

	return 0;
}

void listen_start() {

	if (listen_server_running) return;
	listen_server_running = true;

	LOG("listen_start()\n");

	pthread_create(&listen_server_thread, 0, thread_listenserver_f, (void *)0);
}

void listen_stop() {

	if (!listen_server_running) return;
	listen_server_running = false;

	LOG("listen_stop()\n");

	pthread_join(listen_server_thread, 0);
}


