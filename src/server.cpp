#include "commont.h"
#include "world.h"
#include "server.h"

// ************************************************************
//  server stuff
// ************************************************************

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
	bool ok = map.load(SERVER_MAPS_DIR, mapName);
	if (!ok)
		return false;
	file = mapName;
	title = map.title;
	width = map.w;
	height = map.h;
	votes = 0;
	return true;
}

gameserver_c::gameserver_c() : world(this) {
	server = 0;
	hostname[0]=0;	//hostname
	world.frame = 0;		// current frame count
	next_vote_announce_frame = 0;
	last_vote_announce_votes = last_vote_announce_needed = 0;

	server_kbps_traffic = 0;		//total server traffic in kbytes/sec
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
	if (mode==0 && world.player[pid].muted!=2)
		plprintf(pid, "@WYou have been unmuted (you can send messages again)");
	else if (mode == 1)
		plprintf(pid, "@WYou have been muted (you can't send messages)");
	for (int i=0; i<MAX_PLAYERS; ++i)
		if (world.player[i].used && i!=pid) {
			if (mode == 0)
				plprintf(i, "@IThe admin has unmuted %s", world.player[pid].name);
			else
				plprintf(i, "@IThe admin has muted %s", world.player[pid].name);
		}
	world.player[pid].muted = mode;
}
void gameserver_c::kickPlayer(int pid, bool ban) {
	world.player[pid].delayedMessages.clear();
	if (ban)
		plprintf(pid, "@WYou are now BANNED from this server! Have a nice life...");
	else {
		plprintf(pid, "@WYou are being kicked from this server!");
		plprintf(pid, "@WWarning: you can get permanently banned for behaving badly!");
	}
	for (int i=0; i<MAX_PLAYERS; ++i)
		if (world.player[i].used && i!=pid)
			plprintf(i, "@IThe admin has %s %s (disconnect in 10 seconds)", ban?"banned":"kicked", world.player[pid].name);
	world.player[pid].kickTimer = 10*10;
}

#ifdef SV_NAME_AUTHORIZATION
void gameserver_c::banPlayer(int pid) {
	authorizations.ban(server->get_client_address(world.player[pid].cid));
	authorizations.save();
	kickPlayer(pid, true);
}
#endif

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
	server->send_message(world.player[pid].cid, lebuf, count);
}

//send a player name update to a client
void gameserver_c::send_player_name_update(int cid, int pid) {

	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, 1);	// "1" = player name update
	writeByte(lebuf, count, pid);		// what player id
	world.player[pid].name[15]=0;		// force trunc name at 15 chars (paranoia)
	writeString(lebuf, count, world.player[pid].name);
	server->send_message(cid, lebuf, count);
}

//broadcast new player name
void gameserver_c::broadcast_player_name(int pid) {

	for (int i=0;i<maxplayers;i++)
	if (world.player[i].used)
		send_player_name_update(world.player[i].cid, pid);

	//server->broadcast_message(lebuf, count);
}

//send a player crap update to a client
void gameserver_c::send_player_crap_update(int cid, int pid) {

	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, 32);	// "32" = player CRAP update

	// --- RECALC CRAP ---
	//reg_status char:
	if (client[ world.player[pid].cid ].token_have) {
		if (client[ world.player[pid].cid ].token_valid)
			world.player[pid].reg_status = '*';
		else
			world.player[pid].reg_status = '?';
	}
	else
		world.player[pid].reg_status = ' ';

	writeByte(lebuf, count, ((NLubyte)pid));
	writeByte(lebuf, count, ((NLubyte)world.player[pid].reg_status));					//regstatus
	writeLong(lebuf, count, ((NLulong)client[world.player[pid].cid].rank));		//ranking#
	writeLong(lebuf, count, ((NLulong)client[world.player[pid].cid].score));		//score POS
	writeLong(lebuf, count, ((NLulong)client[world.player[pid].cid].neg_score));		//score NEG v0.4.8
	writeLong(lebuf, count, ((NLulong)max_world_rank));		//MAX WORLD ranking#
	writeLong(lebuf, count, ((NLulong)max_world_score));		//MAX WORLD score

	//LOG5("CRAPZ SENT TO CID %i of PID %i %c r:%i s:%i\n", cid, pid, world.player[pid].reg_status
	//	,client[world.player[pid].cid].rank
	//	,client[world.player[pid].cid].score);

	server->send_message(cid, lebuf, count);
}

//v0.4.5: broadcast player crap
void gameserver_c::broadcast_player_crap(int pid) {

	for (int i=0;i<maxplayers;i++)
	if (world.player[i].used)
		send_player_crap_update(world.player[i].cid, pid);
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

// messages to update moved players (players/clients with new clients/players)
//
void gameserver_c::move_update_player(int a) {

	if (world.player[a].used) {
		broadcast_player_name(a);

			send_me_packet(a);
			/*
			count = 0;
			writeByte(lebuf, count, 3); // "3" = first-packet information
			writeByte(lebuf, count, ((NLubyte)a) );		// who am I
			writeByte(lebuf, count, ((NLubyte)world.flag[0].score) );		//team 0 current score
			writeByte(lebuf, count, ((NLubyte)world.flag[1].score) );		//team 1 current score
			server->send_message(world.player[a].cid, lebuf, count);
			*/

			//v0.4.4 : tentativa de conserto
			//broadcast frags update
			char lebuf[256]; int count = 0;
			writeByte(lebuf, count, 4);	//"4" = frags update
			writeByte(lebuf, count, a);		// what player id
			writeLong(lebuf, count, world.player[a].frags);
			server->broadcast_message(lebuf, count);

			//v0.4.5 : atualiza registration char / score / rank
			broadcast_player_crap( a );

			//name (NEEDED? FIXME - ja tem la em cima!)
			//broadcast_player_name( a );

		//message
		bprintf("@I%s moved to %s team", world.player[a].name, teamname[a/TSIZE]);
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

	world.dropFlagIfAny(f);

	//copy to t
	world.player[t] = world.player[f];

	//change rockets owner from f to t
	world.changeRocketsOwner(f, t);

	//remove f
	game_remove_player(f);

	//update ctop
	ctop[ world.player[t].cid ] = t;

	//I really dont want to change teams no more..
	world.player[t].want_change_teams = false;
	world.player[t].team_change_time = get_time() + 10.0;		//10 secs interval

	//kill t
	if (world.player[t].health > 0)
		world.resetPlayer(t);

	//update t
	move_update_player(t);
}

//swap players - both are valid players
void gameserver_c::swap_players(int a, int b) {
	broadcast_sample(SAMPLE_CHANGETEAM);

	if (world.player[a].health > 0)
		world.resetPlayer(a);
	if (world.player[b].health > 0)
		world.resetPlayer(b);

	swap(world.player[a], world.player[b]);
	world.swapRocketOwners(a, b);

	//swap client id's
	ctop[ world.player[a].cid ] = a;
	ctop[ world.player[b].cid ] = b;

	//either don't want to change teams anymore
	world.player[a].want_change_teams = false;
	world.player[a].team_change_time = get_time() + 10.0;		//10 secs interval
	world.player[b].want_change_teams = false;
	world.player[b].team_change_time = get_time() + 10.0;		//10 secs interval

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
	broadcast_screen_message(world.player[p].roomx, world.player[p].roomy, (char*)lebuf, count);
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

//update team scores
void gameserver_c::ctf_update_teamscore(int t) {
	char lebuf[64]; int count = 0;
	writeByte(lebuf, count, 9);		// CTF teamscore update
	writeByte(lebuf, count, ((NLubyte)t));		// the team
	writeByte(lebuf, count, ((NLubyte)world.flag[t].score));	//the score
	server->broadcast_message(lebuf, count);
}

//send map time left
void gameserver_c::send_map_time(int cid) {
	const NLulong time_left = max(0LU, worldConfig.getTimeLimit() - world.getMapTime()) / 10;
	char lebuf[64]; int count = 0;
	writeByte(lebuf, count, 35);	// 35 = map time left
	writeLong(lebuf, count, time_left);
	if (cid == -1)
		server->broadcast_message(lebuf, count);
	else
		server->send_message(cid, lebuf, count);
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
			raw[p/TSIZE] += DEFAULT_PLAYER_RATE;		// default player rate
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
	if (player_count >= 2) { //v0.4.7.1 : skip the scoring if only one player present

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
		//sprintf(lix, "%s scores +%.4f for %.4f +delta", world.player[p].name, parcela, client[cid].fdp);
		//broadcast_message(lix);

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
	if (player_count >= 2) { //v0.4.7.1 : skip the scoring if only one player present

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
		//sprintf(lix, "%s scores -%.4f for %.4f -delta", world.player[p].name, parcela, client[cid].fdn);
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
		sprintf(job->request, "GET /servlet/fcecin.tk1/index.html?%s&dscp=%i&dscn=%i&name=%s&token=%s\n\n", TK1_VERSION_STRING, client[id].delta_score, client[id].neg_delta_score, world.player[ ctop [id] ].name, client[id].token);

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

//broadcast team message
void gameserver_c::broadcast_team_message(int team, char *text) {

	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, 2);
	writeString(lebuf, count, text);

	// send message only to teammates
	for (int i=0;i<maxplayers;i++)
	if (world.player[i].used)
	if ((i/TSIZE) == team) {
		server->send_message(world.player[i].cid, lebuf, count);
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
	if (world.player[j].used)
	if (world.player[j].roomx == px)
	if (world.player[j].roomy == py)
		server->send_message(world.player[j].cid, lebuf, count); //send the message
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
	server->send_message(world.player[pid].cid, buf, strlen(buf)+1);
}

//send a single message player-printf
void gameserver_c::player_message(int pid, const char *text) {
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, 2);
	writeString(lebuf, count, text);
	if (world.player[pid].used)
		server->send_message(world.player[pid].cid, lebuf, count);
}

//broadcast message to all
void gameserver_c::broadcast_message(const char *text) {
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, 2);
	writeString(lebuf, count, text);
	for (int i=0;i<maxplayers;i++)
		if (world.player[i].used)
			server->send_message(world.player[i].cid, lebuf, count);
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
							pupConfig.pups_min = ival;
							pupConfig.pups_min_percentage = true;
						}
						else LOG1("Can't set pups_min to %d%%\n", ival);
					}
					else if (ival >= 0 && ival <= MAX_PICKUPS) {
						pupConfig.pups_min = ival;
						pupConfig.pups_min_percentage = false;
					}
					else LOG1("Can't set pups_min to %d\n", ival);
				}
				else if (cmd == 28) {
					if (strchr(s, '%')) {
						if (ival >= 0) {
							pupConfig.pups_max = ival;
							pupConfig.pups_max_percentage = true;
						}
						else LOG1("Can't set pups_max to %d%%\n", ival);
					}
					else if (ival>=0 && ival<=MAX_PICKUPS) {
						pupConfig.pups_max = ival;
						pupConfig.pups_max_percentage = false;
					}
					else LOG1("Can't set pups_max to %d\n", ival);
				}
				else if (cmd == 16) {
					if (ival >= 0 && ival <= 60)
						pupConfig.pups_respawn_time = ival;
					else LOG1("Can't set pups_respawn_time to %d\n", ival);
				}
				else if (cmd == 17) {
					pupConfig.pup_chance_shield = ival;
				}
				else if (cmd == 18) {
					pupConfig.pup_chance_turbo = ival;
				}
				else if (cmd == 19) {
					pupConfig.pup_chance_shadow = ival;
				}
				else if (cmd == 20) {
					pupConfig.pup_chance_power = ival;
				}
				else if (cmd == 21) {
					pupConfig.pup_chance_weapon = ival;
				}
				else if (cmd == 22) {
					pupConfig.pup_chance_megahealth = ival;
				}
				else if (cmd == 23) {
					pupConfig.pup_chance_deathbringer = ival;
				}
				else if (cmd == 24) {
					if (ival >= 0)
						worldConfig.time_limit = 60 * 10 * ival; // convert minutes to frames
				}
				else if (cmd == 25) {
					if (ival > 0)
						worldConfig.capture_limit = ival;
				}
				else if (cmd == 26) {
					welcome_message.push_back(s);
				}
				else if (cmd == 27) {
					info_message.push_back(s);
				}
				else if (cmd == 29) {
					if (ival > 0 && ival<1000)
						pupConfig.pup_add_time = ival;
					else LOG1("Can't set pup_add_time to %d\n", ival);
				}
				else if (cmd == 30) {
					if (ival > 0 && ival<1000)
						pupConfig.pup_max_time = ival;
					else LOG1("Can't set pup_max_time to %d\n", ival);
				}
				else if (cmd == 31) {
					if (ival == 0 || ival == 1)
						pupConfig.pup_deathbringer_switch = ival==1?true:false;
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
						worldConfig.respawn_time = val;
				}
				else if (cmd == 35) {
					if (val >= 0)
						worldConfig.waiting_time_deathbringer = val;
				}
				else if (cmd == 36) {
					if (ival == 0 || ival == 1)
						worldConfig.shadow_minimum = ival==1?1:SV_SHADOW_MINIMUM_NORMAL;
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

//send map change message to a player
//reason: NEXTMAP_CAPTURE_LIMIT ou NEXTMAP_VOTE_EXIT
void gameserver_c::send_map_change_message(int pid, int reason, const char* mapname) {

	char lebuf[256];
	int count = 0;
	writeByte(lebuf, count, 20);	// 20 = map change

	writeByte(lebuf, count, 2);		// 2 = custom map message
	writeShort(lebuf, count, world.map.crc);
	writeString(lebuf, count, mapname);
	server->send_message(world.player[pid].cid, lebuf, count);

	//VERY IMPORTANT: flags the player as "awaiting map load" - client must confirm map to proceed
	world.player[pid].awaiting_client_ready = true;

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
		server->send_message(world.player[pid].cid, lebuf, count);

		//important: server is showing gameover plaque. nobody should move or receive world frames
		gameover = true;
		gameover_time = get_time() + 5.0;		//5 secods timeout for gameover plaque
	}
}

bool gameserver_c::server_next_map(int reason) {

	//(re)load hostname
	reload_hostname();

	assert(!maprot.empty());

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
		world.player[p].want_map_exit=false;
		if (world.player[p].mapVote==currmap)
			world.player[p].mapVote=-1;
	}
	maprot[currmap].votes=0;
	last_vote_announce_votes = last_vote_announce_needed = 0;
	next_vote_announce_frame = 0;	// let a new announcement be made as soon as someone votes

	if (!load_rotation_map(currmap))
		return false;

	// notify all players
	for (int i=0;i<maxplayers;i++)
		if (world.player[i].used)
			send_map_change_message(i, reason, maprot[currmap].file.c_str());

	char lix[256];
	sprintf(lix, "Server changed map to: %s (%i of %i)", maprot[currmap].file.c_str(), currmap+1, maprot.size());
	broadcast_message(lix);

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
		world.ctf_game_restart();
	}
}

//----- THE REST  ----------------

bool gameserver_c::reset_settings() {
	set_default_physics();

	pupConfig.reset();
	worldConfig.reset();

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

	ping_send_client = 0;

	//reset fslavesocks
	for (int ss=0;ss<MAX_PLAYERS;ss++)
		fslavesock[ss] = NL_INVALID;			//inicializa
	file_threads_quit = false;	//not yet

	// reset players
	//players_present = 0;
	player_count = 0;
	for (i=0;i<MAX_PLAYERS;i++) {
		world.player[i].used = false;
		world.player[i].id = i;
		world.player[i].name[0]=0;
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
	world.ctf_game_restart();

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
		if (world.player[i].used == true)
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
		if (world.player[i].used) {
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
		if (!world.player[i].used)
		{
			// add player to players_present
			//players_present = players_present | (1 << i);

			// init player
			int cid;
			cid=id;
			ctop[cid]=i;

			world.player[i].clear(true, i, cid, SERVER_DEFAULT_PLAYER_NAME);

			myself = i;

			// spawn player
			world.player[i].respawn_to_base = true;
			world.respawnPlayer(i);

			//reset keypresses
			world.player[i].l = 0;
			world.player[i].r = 0;
			world.player[i].u = 0;
			world.player[i].d = 0;

			//check pickup creation
			world.check_pickup_creation(false);

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
		world.ctf_game_restart();

	//char lelix[222];
	//sprintf(lelix, "PCOUNT = %i BCOUNT = %i\n", player_count, bot_count);
	//broadcast_message(lelix);

	char lebuf[256]; int count;

	// **** V0.4.4 : init oneclient_c ****
	client[id].reset();
	client[id].token_have = false;		// no token

	//first update the ADMIN SHELL
	if (shellssock) {
		count = 0;
		writeLong(lebuf, count, STA_PLAYER_CONNECTED);
		writeLong(lebuf, count, world.player[myself].cid);
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
		if (world.player[i].used)
		if (i != myself) {

			//player name update
			send_player_name_update(id, i);
			//count = 0;
			//writeByte(lebuf, count, 1);	// "1" = player name update
			//writeByte(lebuf, count, i);		// what player id
			//world.player[i].name[15]=0;		// force trunc name at 15 chars (paranoia)
			//writeString(lebuf, count, world.player[i].name);
			//server->send_message(id, lebuf, count);

			//frags update
			count = 0;
			writeByte(lebuf, count, 4);	//"4" = frags update
			writeByte(lebuf, count, i);		// what player id
			writeLong(lebuf, count, world.player[i].frags);
			server->send_message(id, lebuf, count);

			//crap update
			send_player_crap_update(id, i);
		}

	for (vector<string>::const_iterator line=welcome_message.begin(); line!=welcome_message.end(); line++)
		world.player[myself].add_to_queue(*line);

	//check for team changes
	//
	check_team_changes();

	//update serverinfo
	update_serverinfo();

	//send map time left if there is a time limit
	if (worldConfig.getTimeLimit() > 0)
		send_map_time(id);

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
		writeLong(lebuf, count, world.player[pid].cid);
		nlWrite(shellssock, lebuf, count);
	}

	//broadcast a textual message "Player BLABLA left the game"
	bprintf("@I%s left the game with %i frags", world.player[pid].name, world.player[pid].frags);

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
	world.player[ ctop[client_id] ].ping = ping_time;
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
	ServerPlayer *h;
	h = &(world.player[pid]);

	h->l = (keys & 1) != 0;
	h->r = (keys & 2) != 0;
	h->u = (keys & 4) != 0;
	h->d = (keys & 8) != 0;
	bool strafe = (keys & 16) != 0;
	h->run = (keys & 32) != 0;

	//if not strafing, update direction
	if (!strafe) {
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
				if (strcmp(tempname, world.player[pid].name))
				//name change flooding protection
				if (get_time() >= world.player[pid].waitnametime) {

					//FLUSH PENDING REPORTS TO MASTER IF token_have/token_valid !!!
					client_report_status(id);

					//name changed -- this means that the player is NOT REGISTERED
					//  anymore for recording statistics
					client[id].token_have = false;

					//need to broadcast the player's crap to remove eventual '*' and stuff
					broadcast_player_crap( pid );

					//check if it's the first name information from client. then it
					// must have just entered the game
					bool entered_game = !strcmp(SERVER_DEFAULT_PLAYER_NAME, world.player[pid].name);

					// log
					//LOG3("client %i player %i '%s' renamed to", id, pid, world.player[pid].name);

					//readString(msg, count, world.player[pid].name); //name update request
					strcpy(world.player[pid].name, "(invalid name)");
					if (strpbrk(tempname, "%@")!=NULL)
						world.player[pid].add_to_queue("@WSorry, this server doesn't accept % or @ in a name");
					else if (strspnp(tempname, " ")==NULL)
						world.player[pid].add_to_queue("@WPlease enter a name");
					else {
						#ifdef SV_NAME_AUTHORIZATION
						int nid=authorizations.identifyName(tempname);
						if (nid==-1 || authorizations.authorize(nid, server->get_client_address(id)))
							strcpy(world.player[pid].name, tempname);
						else {
							world.player[pid].queue_printf("@WThe name %s is reserved on this server.", authorizations.getName(nid).c_str());
							world.player[pid].queue_printf("@ITo authorize, type /auth %s,pass where pass is your password.", authorizations.getName(nid).c_str());
						}
						#else
						strcpy(world.player[pid].name, tempname);
						#endif
					}

					//LOG1("'%s'\n", world.player[pid].name);

					//send entered-game message
					if (entered_game) {
						bprintf("@I%s entered the game", world.player[pid].name);
						//sound
						broadcast_sample(SAMPLE_ENTERGAME);
					}
					// next time allowed to change name
					world.player[pid].waitnametime = get_time() + 1.0;

					//broadcast the new player's name
					broadcast_player_name(pid);

					//update the ADMIN SHELL
					if (shellssock) {
						char lebuf[256]; int count = 0;
						writeLong(lebuf, count, STA_PLAYER_NAME_UPDATE);
						writeLong(lebuf, count, world.player[pid].cid);
						writeString(lebuf, count, world.player[pid].name);
						if (entered_game) {
							writeLong(lebuf, count, STA_PLAYER_IP);
							writeLong(lebuf, count, world.player[pid].cid);
							char addrBuf[50];
							NLaddress addr=server->get_client_address(world.player[pid].cid);
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
				// handle 'console' commands
				if (world.player[pid].delayedMessages.size()>2) {
					world.player[pid].delayedMessages.clear();
					plprintf(pid, "@I(rest of message cancelled)");
				}
				world.player[pid].reset_message_queue_timing();
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
						world.player[pid].queue_printf("@TConsole commands available on this server:");
						world.player[pid].queue_printf("/help       this screen");
						if (!info_message.empty())
							world.player[pid].queue_printf("/info       information about this server");
						world.player[pid].queue_printf("/config     current server configuration");
						world.player[pid].queue_printf("/stats      see your stats");
						world.player[pid].queue_printf("/mapinfo n  information about map n (default: current map)");
						world.player[pid].queue_printf("/votemap n  vote for the next map to be n (default: list maps and votes)");
						world.player[pid].queue_printf("/time       check server uptime, current map time and time left on the map");
						if (sayadmin_enabled) {
							ostringstream ostr;
							ostr << "/sayadmin   send a message to the server admin";
							if (sayadmin_comment.length())
								ostr << " (" << sayadmin_comment << ')';
							world.player[pid].add_to_queue(ostr.str());
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
									tmb->tm_hour, tmb->tm_min, tmb->tm_sec, world.player[pid].name, pCommand);
							fclose(logp);
							if (shellssock) {
								char lebuf[256];
								int count=0;
								writeLong(lebuf, count, STA_ADMIN_MESSAGE);
								writeLong(lebuf, count, world.player[pid].cid);
								writeString(lebuf, count, pCommand);
								nlWrite(shellssock, lebuf, count);
							}
							world.player[pid].queue_printf("@IYour message has been logged. Thank you for your feedback!");
						}
						else
							world.player[pid].queue_printf("@IFor example to send \"Hello!\", type /sayadmin Hello!");
					}
					else if (!strcmp(cbuf, "map") || !strcmp(cbuf, "mapinfo")) {
						if (*pCommand!='\0') {
							int mid=atoi(pCommand)-1;
							if (mid>=0 && mid<(int)maprot.size() && pCommand[strspn(pCommand, "0123456789")]=='\0') {
								world.player[pid].queue_printf("@IMap %d is %s", mid+1, maprot[mid].title.c_str());
								world.player[pid].queue_printf("@I%s.txt, size %dx%d", maprot[mid].file.c_str(), maprot[mid].width, maprot[mid].height);
							}
							else
								world.player[pid].queue_printf("@WValid map id's are 1 to %d", maprot.size());
						}
						else {
							world.player[pid].queue_printf("@IThis map is %s", maprot[currmap].title.c_str());
							world.player[pid].queue_printf("@I%s.txt, size %dx%d", maprot[currmap].file.c_str(), maprot[currmap].width, maprot[currmap].height);
						}
						world.player[pid].queue_printf("@IType /votemap to see a list of all maps");
					}
					else if (!strcmp(cbuf, "votemap")) {
						string status;
						bool err=false;
						if (*pCommand!='\0') {
							int mid=atoi(pCommand)-1;
							if (mid>=-1 && mid<(int)maprot.size() && pCommand[strspn(pCommand, "0123456789")]=='\0') {
								if (world.player[pid].mapVote==mid)
									status="no changes";
								else {
									if (world.player[pid].mapVote==-1)
										status="vote added";
									else if (mid==-1)
										status="vote removed";
									else
										status="vote updated";
									world.player[pid].mapVote=mid;
									check_map_exit();
								}
								if (!world.player[pid].want_map_exit)
									world.player[pid].queue_printf("@TPress F4 to actually vote for a mapchange");
							}
							else {
								world.player[pid].queue_printf("@W\"%s\" is not a valid map id (1 to %d)", pCommand, maprot.size());
								err=true;
							}
						}
						else
							world.player[pid].queue_printf("@TFor example to vote for map 1, type /votemap 1");
						if (!err) {
							if (status.length())
								world.player[pid].queue_printf("@T(%s) Maps on this server: ID, votes, description", status.c_str());
							else
								world.player[pid].queue_printf("@TMaps on this server: ID, votes, description");
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
								world.player[pid].queue_printf("%s", buf);
								bufi=0;
							}
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
							world.player[pid].total_movement/30.,	// make the unit player diameter <-> divide by 30.
							world.player[pid].total_movement/30./double(lifetime));
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
					else if (!strcmp(cbuf, "auth")) {
						string nameUpr;
						for (; *pCommand && *pCommand!=','; ++pCommand)
							nameUpr+=toupper(*pCommand);
						if (*pCommand==',') {
							string pwd(pCommand+1);
							if (authorizations.addIP(nameUpr, pwd, server->get_client_address(id))) {
								authorizations.save();
								world.player[pid].queue_printf("@WOK: authorized your IP address to use %s", nameUpr.c_str());
								world.player[pid].queue_printf("@WYou may change your name now");
							}
							else
								world.player[pid].queue_printf("@WAuthorization failed");
						}
						else
							world.player[pid].queue_printf("@WInvalid auth command");
					}
					#endif
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

						//esquenta o cara
						world.player[pid].talk_temp = 18.0;

						char elbuf[200]; int elcnt = 0;
						writeByte(elbuf, elcnt, 2);
						writeString(elbuf, elcnt, "@WToo much talk. Chill...");
						server->send_message(world.player[pid].cid, elbuf, elcnt);
					}
					else if (world.player[pid].muted==1) {
						plprintf(pid, "@WYou are muted. You can't send messages.");
					}
					else {
						// check for team message:
						if (msg[1] == '.') {
							sprintf(talkmsg, "@T%s: %s", world.player[pid].name, sbuf+1);
							if (world.player[pid].muted==2)
								player_message(pid, talkmsg);
							else
								broadcast_team_message(pid/TSIZE, talkmsg);
						}
						//regular msg
						else {
							sprintf(talkmsg, "%s: %s", world.player[pid].name, sbuf);
							if (world.player[pid].muted==2)
								player_message(pid, talkmsg);
							else
								broadcast_message(talkmsg);
						}
						// log
						//LOG4("client %i player %i name %s says: '%s'\n", id, pid, world.player[pid].name, &(msg[1]));
					}
				}
			}
			//+attack
			else if (code == 5) {
				world.player[pid].attack = true;
			}
			//SUICIDE!!
			else if (code == 10) {
				world.suicide(pid);
			}
			//-attack
			else if (code == 11) {
				world.player[pid].attack = false;
			}
			// want changeteams on
			else if (code == 12) {
				if (!world.player[pid].want_change_teams) {
					//set
					world.player[pid].want_change_teams = true;
					//show message
					//broadcast_message("@I%s player '%s' wants to change teams", teamname[pid/TSIZE], world.player[pid].name);
					//check for team changes
					check_team_changes();
				}
			}
			// want changeteams off
			else if (code == 13) {
				// show message if command had effect
				if (world.player[pid].want_change_teams) {
					world.player[pid].want_change_teams = false;
					world.player[pid].team_change_pending = false; //so pra garantir
					//broadcast_message("@I%s player '%s' don't want to change teams", teamname[pid/TSIZE], world.player[pid].name);
				}
			}
			// "client ready" message
			else if (code == 21) {
				//client is ready to play now
				world.player[pid].awaiting_client_ready = false;
			}
			// want change teams
			else if (code == 22) {
				if (world.player[pid].want_map_exit == false) {
					world.player[pid].want_map_exit = true;
					check_map_exit();		//check map exit
				}
			}
			// dont' want change teams
			else if (code == 23) {
				if (world.player[pid].want_map_exit == true) {
					world.player[pid].want_map_exit = false;
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
					bprintf("%s %i %i token identico.", world.player[ctop[id]].name, client[id].intoken, intoken);
#endif
				}
				//senao, faz a rotina de dispatch old results + set new invalid token to be validated
				else {

#ifdef DEBUG_RANKING
					//debug message
					if ((client[id].token_have) && (client[id].token_valid))
						bprintf("%s %i %i changes registration, submitting results...", world.player[ctop[id]].name, client[id].intoken, intoken);
					else if ((client[id].token_have) && (!client[id].token_valid))
						bprintf("%s %i %i changes registration ('?')", world.player[ctop[id]].name, client[id].intoken, intoken);
					else if (!client[id].token_have)
						bprintf("%s %i %i sends registration.", world.player[ctop[id]].name, client[id].intoken, intoken);
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
					sprintf(job->request, "GET /servlet/fcecin.tk1/index.html?%s&chktk&name=%s&token=%s\n\n", TK1_VERSION_STRING, world.player[ ctop [id] ].name, client[id].token);

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
				world.player[pid].dropped_flag = true;
				world.dropFlagIfAny(pid);
			}
			else {
				//ERROR: unknown message from client
				LOG3("ERROR: UNKNOWN MESSAGE FROM CLIENT %i CODE=%i LENGTH=%i\n", id, code, msglen);
			}
		}
	} while (msg != 0);
}

//simulate and broadcast frame
void gameserver_c::simulate_and_broadcast_frame() {
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
			broadcast_message(voteinfo.str().c_str());
		}
		else if (world.getMapTime() == vote_block_time)
			check_map_exit();
	}
	for (int i=0; i<maxplayers; ++i)
		if (world.player[i].used) {
			if (world.player[i].kickTimer) {
				--world.player[i].kickTimer;
				if (world.player[i].kickTimer==0)
					server->disconnect_client(world.player[i].cid, 1);	// 1 second timeout
				else if (world.player[i].kickTimer%10 == 0 && world.player[i].kickTimer<=50)
					plprintf(i, "@WDisconnecting in %d...", world.player[i].kickTimer/10);
			}
			else {
				ServerPlayer::DMQueueT& dm=world.player[i].delayedMessages;
				while (dm.size() && --dm.begin()->first<0) {
					player_message(i, dm.begin()->second.c_str());
					dm.erase(dm.begin());
				}
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
	//    SHORT ping do jogador : world.player[frame % maxplayers].ping;
	//

	//RECALC PLAYERS PRESENT EVERY TIME
	NLulong players_present = 0;
	for (int pp=0;pp<maxplayers;pp++)
	if (world.player[pp].used)
		players_present += (1 << pp);


	// ============================
	//   build common data buffer
	// ============================
	char lebuf[4096];		//common frame data
	int count = 0;

	//frame
	writeLong(lebuf, count, world.frame);

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
		if (world.player[helmiter].used)
			//fix: helm nao enxerga outros helms, a nao ser com a flag
			//ou seja: so mostra (break) se:  NAO TEM HELM   ou   TEM FLAG
			if ((world.player[helmiter].item_helm == 0) || ((world.flag[1-helmiter/TSIZE].carried) && (world.flag[1-helmiter/TSIZE].carrier == helmiter)))
				break;
	} while (runaway-- > 0);

	int t;

	//atualiza tview E HELMVIEW
	for (t=0;t<2;t++) {		// p/ cada time

		tview_bits[t] = 0;

		helmview[t] = 0;		//default zero

		for (int i=0;i<maxplayers;i++)			// p/ cada inimigo desse time
		if (i/TSIZE == 1-t)
		if (world.player[i].used)
		{
			// ---- helmview -----
			// mostra se NAO TEM HELM ou SE TA COM FLAG
			if ((world.player[i].item_helm == 0) || ((world.flag[1-i/TSIZE].carried) && (world.flag[1-i/TSIZE].carrier == i))) {
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
			if (world.player[j].used)
			{
				if ((world.player[j].roomx == world.player[i].roomx) && (world.player[j].roomy == world.player[i].roomy))	{

					//se o cara tem helm E NAO TEM FLAG, nao aparece!!
					if ((world.player[i].item_helm > 0) && ((world.flag[1-i/TSIZE].carried == false) || (world.flag[1-i/TSIZE].carrier != i))) {
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
			if (world.player[tviter[t]].used) {

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
	for (int i=0;i<maxplayers;i++)
	if (world.player[i].used)			// player valido (used)
	{
		//rewrite past common data
		lecount = count;

		//extra byte of information
		// BIT 0: extra health
		// BIT 1: extra energy
		// BIT 2  ( VERY IMPORTANT ) : player not ready bit (envia frame vazio!) OU server exibindo placa "game over"
		NLubyte xtra = 0;
		if (world.player[i].health & 256) xtra += 1;
		if (world.player[i].energy & 256) xtra += 2;
		// BITS 3..8 == what player id
		if (i & 1) xtra += 8;
		if (i & 2) xtra += 16;
		if (i & 4) xtra += 32;
		if (i & 8) xtra += 64;
		if (i & 16) xtra += 128;

		bool skip_frame = (world.player[i].awaiting_client_ready) || ((gameover) && (gameover_time > get_time()));

		if (skip_frame) xtra += 4;		// VERY IMPORTANT
		writeByte(lebuf, lecount, xtra);

		// ****** VERY IMPORTANT ******
		// send almost empty frame if client not ready (leave bandwidth for data transfer) OR IF
		// server showing gameover plaque
		if (!skip_frame) {

			// NEW: 0.3.9 : send before players_onscreen 2 bytes with the screen of self
			//
			NLubyte scr;
			scr = ((NLubyte)world.player[i].roomx);
			writeByte(lebuf, lecount, scr);	//player.x (screen)
			scr = ((NLubyte)world.player[i].roomy);
			writeByte(lebuf, lecount, scr);	//player.y (screen)

			//player data field for each player ON SCREEN
			NLulong		players_onscreen = 0;

			//keep place for players_onscreen
			int p_on_count = lecount;
			writeLong(lebuf, lecount, 0);

			for (int j=0;j<maxplayers;j++)
			if ((players_present & (1 << j)) != 0)		//player j exists
			// j is on same screen than i (the viewer)
			// AND
			//   ((j helm != 1)  ||  (j/TSIZE == i/TSIZE))  // nao-totalmente invisivel OU e do mesmo time OU j com flag
			if (
					(world.player[j].roomx == world.player[i].roomx)
					&&
					(world.player[j].roomy == world.player[i].roomy)
					&&
					(world.player[j].item_helm != 1 || i/TSIZE == j/TSIZE ||
							(world.flag[1-j/TSIZE].carried && world.flag[1-j/TSIZE].carrier == j)) ) {
				//add to players_onscreen
				players_onscreen += (1 << j);

				ServerPlayer* h = &(world.player[j]);

//					NLshort sho;

				//V0.3.9: took out screen from here

				//V0.3.9 : transmissao x,y de 4 bytes para 3
				NLubyte xy;
				NLushort hx,hy;
				hx = ((NLushort)h->lx);
				hy = ((NLushort)h->ly);

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

				// EXTRA BYTE (ex- zframe)  bit 0 : player dead  bit 1 : has deathbringer  bit 2 : deathbringer-affected
				NLubyte extra = 0;
				if (world.player[j].health <= 0) extra += 1; //deadflag
				if (world.player[j].item_deathbringer) extra += 2; //has deathbringer
				if (world.player[j].deathbringer_end > get_time()) extra += 4;		//deathbringer-affected
				// ITEMS: moved to this byte
				if (world.player[j].item_shield)		extra += 8;
				if (world.player[j].item_speed)			extra += 16;
				if (world.player[j].item_quad)			extra += 32;

				//write extra byte
				writeByte(lebuf, lecount, extra);

				NLubyte keys = h->gundir << 5;
				// if dead player, don't send keys
				if (world.player[j].health > 0) {
					if (h->l) keys |= 1;
					if (h->r) keys |= 2;
					if (h->u) keys |= 4;
					if (h->d) keys |= 8;
					if (h->run) keys |= 16;
				}
				writeByte(lebuf, lecount, keys);

				writeByte(lebuf, lecount, (NLubyte)world.player[j].item_helm);
			}

			//update players_onscreen (it's before the players on screen data (above))
			writeLong(lebuf, p_on_count, players_onscreen);

			//ELMO: visao alem do alcance!!
			NLubyte who;
			if (world.player[i].item_helm > 0) {

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
			NLubyte mx = (NLubyte)(((world.player[who].lx + ((double)(world.player[who].roomx * plw))) / (world.map.w*plw)) * 255.0);
			writeByte(lebuf, lecount, mx);

			//y do cara, 0..255 (%) do mundo
			NLubyte my = (NLubyte)(((world.player[who].ly + ((double)(world.player[who].roomy * plh))) / (world.map.h*plh)) * 255.0);
			writeByte(lebuf, lecount, my);

			//send player's BASE health (first 8 bits)
			if (world.player[i].health < 0) world.player[i].health = 0;
			writeByte(lebuf, lecount, ((NLubyte)(world.player[i].health & 255)));

			//send player's BASE energy (first 8 bits)
			if (world.player[i].energy < 0) world.player[i].energy = 0;
			writeByte(lebuf, lecount, ((NLubyte)(world.player[i].energy & 255)));

			//ping of player frame# % MAXPLAYERS
			NLushort theping = (NLushort) world.player[world.frame % maxplayers].ping;
			writeShort(lebuf, lecount, theping);

		}//!world.player[i].awaiting_client_ready
		//send the packet
		server->send_frame(world.player[i].cid, lebuf, lecount);	//use client id of the player, and LEcount
	}

	// PING: v0.4.1
	// envia um ping a cada frame, faz revezamento entre todos os players
	ping_send_client++;		//next player
	if (ping_send_client >= maxplayers)
		ping_send_client = 0;
	if (world.player[ping_send_client].used) // valid player?
		server->ping_client(world.player[ping_send_client].cid); //ping
}

//run something after simulate_and_broadcast
void gameserver_c::server_think_after_broadcast() {
	int i;

	//check players with pending team changes
	for (i=0;i<maxplayers;i++)
	if (world.player[i].used)
	if (world.player[i].team_change_pending)
	if (world.player[i].want_change_teams)
	if (world.player[i].team_change_time < get_time())
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

	while ( (*running_flag) == true )	{

		// generate and send frame
		simulate_and_broadcast_frame();

		//dec counter - another 100ms must pass before next send
		server_speed_counter--;

		// next frame
		world.frame++;

		//update dedserver wintitle
		if (world.frame % 10 == 0) {

			server_kbps_traffic =
				server->get_socket_stat(NL_AVE_BYTES_RECEIVED) +
				server->get_socket_stat(NL_AVE_BYTES_SENT);
			server_kbps_traffic /= 1024.0;

			//update bar
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
			if (world.player[i].used)
			{
				writeLong(lebuf, count, STA_PLAYER_CONNECTED);////1 .... player connected <int id>
				writeLong(lebuf, count, world.player[i].cid);
				writeLong(lebuf, count, STA_PLAYER_FRAGS);
				writeLong(lebuf, count, world.player[i].cid);
				writeLong(lebuf, count, world.player[i].frags);
				writeLong(lebuf, count, STA_PLAYER_NAME_UPDATE);
				writeLong(lebuf, count, world.player[i].cid);
				writeString(lebuf, count, world.player[i].name);
				writeLong(lebuf, count, STA_PLAYER_IP);
				writeLong(lebuf, count, world.player[i].cid);
				char addrBuf[50];
				NLaddress addr=server->get_client_address(world.player[i].cid);
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
					if (world.player[pid].used) {
						answer = true; //ADMIN SHELL
						writeLong(lebuf, count, STA_PLAYER_FRAGS);
						writeLong(lebuf, count, world.player[pid].cid);
						writeLong(lebuf, count, world.player[pid].frags);
					}
				}
				break;
			case ATS_GET_PLAYER_TOTAL_TIME:		//request the frags amount of a player <int id>
				result = nlRead(shellssock, rbuf, 4);
				rcount = 0; readLong(rbuf, rcount, clid); pid = ctop[clid];
				if (result == 4) {
					if (world.player[pid].used) {
						answer = true;
						writeLong(lebuf, count, STA_PLAYER_TOTAL_TIME);
						writeLong(lebuf, count, world.player[pid].cid);
						delta = (int)(get_time() - world.player[pid].start_time);
						writeLong(lebuf, count, delta);
					}
				}
				break;
			case ATS_GET_PLAYER_TOTAL_KILLS:		//request the total kills amount of a player <int id>
				result = nlRead(shellssock, rbuf, 4);
				rcount = 0; readLong(rbuf, rcount, clid); pid = ctop[clid];
				if (result == 4) {
					if (world.player[pid].used) {
						answer = true;//ADMIN SHELL
						writeLong(lebuf, count, STA_PLAYER_TOTAL_KILLS);
						writeLong(lebuf, count, world.player[pid].cid);
						writeLong(lebuf, count, world.player[pid].total_kills);
					}
				}
				break;
			case ATS_GET_PLAYER_TOTAL_DEATHS:		//request the total deaths amount of a player <int id>
				result = nlRead(shellssock, rbuf, 4);
				rcount = 0; readLong(rbuf, rcount, clid); pid = ctop[clid];
				if (result == 4) {
					if (world.player[pid].used) {
						answer = true;//ADMIN SHELL
						writeLong(lebuf, count, STA_PLAYER_TOTAL_DEATHS);
						writeLong(lebuf, count, world.player[pid].cid);
						writeLong(lebuf, count, world.player[pid].total_deaths);
					}
				}
				break;
			case ATS_GET_PLAYER_TOTAL_CAPTURES:		//request the total captures amount of a player <int id>				}
				result = nlRead(shellssock, rbuf, 4);
				rcount = 0; readLong(rbuf, rcount, clid); pid = ctop[clid];
				if (result == 4) {
					if (world.player[pid].used) {
						answer = true;//ADMIN SHELL
						writeLong(lebuf, count, STA_PLAYER_TOTAL_CAPTURES);
						writeLong(lebuf, count, world.player[pid].cid);
						writeLong(lebuf, count, world.player[pid].total_captures);
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
					if (world.player[p].used) {
						answer=true;
						writeLong(lebuf, count, STA_PLAYER_PING);
						writeLong(lebuf, count, world.player[p].cid);
						writeLong(lebuf, count, world.player[p].ping);
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
	if (world.player[i].used)
	{
		cid = world.player[i].cid;
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

void gameserver_c::sendWeaponPower(int pid) {
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, 18);		//player power change
	writeByte(lebuf, count, ((NLubyte)world.player[pid].weapon) );
	server->send_message(world.player[pid].cid, lebuf, count);
}

void gameserver_c::sendRocketMessage(int shots, int gundir, NLubyte* sid, int team, bool power, int px, int py, int x, int y) {	// sid = shot-id; array of NLubyte[shots]
	//assembly multi-rocket message
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, 7);		// 7 = MULTI rocket fire
	//NLubyte  powerdir;		//bits 0..4 = power bits 5..8=dir
	//powerdir = NLubyte(shots) | NLubyte(gundir<<4);
	//writeByte(lebuf, count, powerdir);		// power and dir
	writeByte(lebuf, count, shots);		// power and dir
	writeByte(lebuf, count, gundir);		// power and dir
	for (int i=0;i<shots;i++) //MULTI ROCKETS!
		writeByte(lebuf, count, sid[i]);		// rocket-object id (needed because client-side rockets can be deleted by the server)
	writeLong(lebuf, count, world.frame);	// time of shot of the rocket: current (last simulated) frame
	NLubyte shotType = (team<<1) | power;
	writeByte(lebuf, count, (NLubyte)shotType);	// owner of all rockets
	writeByte(lebuf, count, (NLubyte)px);	//coord
	writeByte(lebuf, count, (NLubyte)py);
	writeShort(lebuf, count, (NLshort)x);
	writeShort(lebuf, count, (NLshort)y);

	for (int p=0; p<maxplayers; p++)
		if (world.player[p].used && world.player[p].roomx==px && world.player[p].roomy==py)
			server->send_message(world.player[p].cid, lebuf, count);
}

void gameserver_c::sendRocketDeletion(NLulong plymask, int rid, NLshort hitx, NLshort hity, int targ) {
	//assembly rocket delete message
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, 8);		// 8 = rocket deletion
	NLubyte byt = (NLubyte)rid;
	writeByte(lebuf, count, byt);		// rocket-object id
	byt = (NLubyte)targ;
	writeByte(lebuf, count, byt);		// player-target. if 255, no player in particular was hit

	writeShort(lebuf, count, hitx);		// HIT X,Y OF ROCKET
	writeShort(lebuf, count, hity);

	//send message to players that received the rocket
	for (int p=0; p<maxplayers; p++)
		if (world.player[p].used)	//still valid player? (nao custa checar..)
			if (plymask & (1<<p))
				server->send_message(world.player[p].cid, lebuf, count);
}

void gameserver_c::sendDeathbringer(int pid, const ServerPlayer& ply) {
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, 26);	//26==deathbringer
	writeByte(lebuf, count, ((NLubyte)pid));	//team/target player
	writeLong(lebuf, count, world.frame);		//frame # of the bringer shot (message can be delayed)
	writeByte(lebuf, count, ((NLubyte)ply.roomx));
	writeByte(lebuf, count, ((NLubyte)ply.roomy));
	writeShort(lebuf, count, ((NLushort)ply.lx));
	writeShort(lebuf, count, ((NLushort)ply.ly));

	server->broadcast_message(lebuf, count);
}

void gameserver_c::sendPickupVisible(int pid, int pup_id, const pickup_c& it) {
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, 15);		//"item update"
	writeByte(lebuf, count, (NLubyte)pup_id);	//what item
	writeByte(lebuf, count, (NLubyte)it.kind);	//kind
	writeByte(lebuf, count, (NLubyte)it.px);		//screen
	writeByte(lebuf, count, (NLubyte)it.py);
	writeShort(lebuf, count, (NLushort)it.x);	//pos in screen
	writeShort(lebuf, count, (NLushort)it.y);
	server->send_message(world.player[pid].cid, lebuf, count);
}

void gameserver_c::sendPupTime(int pid, NLubyte pupType, double timeLeft) {
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, 17);		//powerup time indicator
	writeByte(lebuf, count, pupType);
	writeShort(lebuf, count, (NLushort)timeLeft);
	server->send_message(world.player[pid].cid, lebuf, count);
}

void gameserver_c::sendFragUpdate(int pid, NLulong frags) {
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, 4);	//"4" = frags update
	writeByte(lebuf, count, pid);		// what player id
	writeLong(lebuf, count, frags);
	server->broadcast_message(lebuf, count);
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

//#@

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
		else if (gameserver->isBanned(arg->client_id)) {
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


