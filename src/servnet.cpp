#include <cctype>

#include "commont.h"
#include "server.h"
#include "admshell.h"
#include "leetnet/server.h"
#include "leetnet/rudp.h"	// get_self_IP
#include "leetnet/sleep.h"	// sleep util
#include "network.h"
#include "servnet.h"
#include "nassert.h"

//delay for the server contacting the master server, in seconds
// it is good if this delay is set to a minute or so, since this will
// filter out people opening and closing servers frequently
#define	DELAY_TO_REPORT_SERVER	10.0

//master job struct
class masterjob_c {
public:

	char		request[512];

	bool		html_end;

	char		lebuf[65536];		//lebuf for collecting response
	int			n;	// lebuf length

	int			code;
	int			cid;

	//return values of the callback
	bool		retry;

	masterjob_c() {
		lebuf[0]=0;
		html_end = false;
		request[0]=0;
	}
};

ServerNetworking::ServerNetworking(gameserver_c* hostp, ServerWorld& w) : host(hostp), world(w) {
	hostname[0] = 0;
	server = 0;
	#ifdef SEND_FRAMEOFFSET
	frameSentTime = 0;	// no meaning
	#endif
	pthread_mutex_init(&fslavesock_mutex, 0);
	pthread_mutex_init(&mjob_mutex, 0);
}

ServerNetworking::~ServerNetworking() {
	pthread_mutex_destroy(&fslavesock_mutex);
	pthread_mutex_destroy(&mjob_mutex);
	if (server) {
		delete server;
		server = 0;
	}
}

void ServerNetworking::upload_next_file_chunk(int i) {

	int CHUNKSIZE = 128;		// the max chunk size in bytes

	//actual size sent
	int chunksize = fileTransfer[i].fsize - fileTransfer[i].dp;		//attempt to send remaining...
	if (chunksize > CHUNKSIZE)							//...but there is the maximum
		chunksize = CHUNKSIZE;

	//check if will be last
	NLubyte islast = 0;	//default:no
	if (fileTransfer[i].dp + chunksize == fileTransfer[i].fsize) //maybe yes?
		islast = 1;		//yes.

	//send
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, data_file_download);		//28 = next file chunk 4 u....
	writeByte(lebuf, count, islast);
	writeLong(lebuf, count, fileTransfer[i].dp );
	writeShort(lebuf, count, ((NLushort)chunksize) );
	writeBlock(lebuf, count, &(fileTransfer[i].data[ fileTransfer[i].dp ] ), chunksize);
	server->send_message(i, lebuf, count);

	//save old dp for the ack
	fileTransfer[i].old_dp = fileTransfer[i].dp;

	//inc dp
	fileTransfer[i].dp += chunksize;
}

//load file from disk. puts file of type/name into the given buffer, returns filesize
int ServerNetworking::get_download_file(char *lebuf, char *ftype, char *fname) {

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

void ServerNetworking::send_me_packet(int pid) {
	int count = 0;
	char lebuf[1024];
	writeByte(lebuf, count, data_first_packet);
	writeByte(lebuf, count, ((NLubyte)pid) );					// who am I
	writeByte(lebuf, count, ((NLubyte)host->current_map_nr()));	// current map
	writeByte(lebuf, count, ((NLubyte)world.teams[0].score()));	// team 0 current score
	writeByte(lebuf, count, ((NLubyte)world.teams[1].score()));	// team 1 current score
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
void ServerNetworking::send_player_name_update(int cid, int pid) {

	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, data_name_update);
	writeByte(lebuf, count, pid);		// what player id
	writeStr(lebuf, count, world.player[pid].name);
	server->send_message(cid, lebuf, count);
}

//broadcast new player name
void ServerNetworking::broadcast_player_name(int pid) {
	for (int i=0;i<maxplayers;i++)
		if (world.player[i].used)
			send_player_name_update(world.player[i].cid, pid);

	//update the ADMIN SHELL
	if (shellssock) {
		char lebuf[256]; int count = 0;
		writeLong(lebuf, count, STA_PLAYER_NAME_UPDATE);
		writeLong(lebuf, count, world.player[pid].cid);
		writeStr(lebuf, count, world.player[pid].name);
		nlWrite(shellssock, lebuf, count);
	}
}

//send a player crap update to a client
void ServerNetworking::send_player_crap_update(int cid, int pid) {
	const ClientData& clid = host->getClientData(cid);

	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, data_crap_update);

	// --- RECALC CRAP ---
	//reg_status char:
	if (clid.token_have) {
		if (clid.token_valid)
			world.player[pid].reg_status = '*';
		else
			world.player[pid].reg_status = '?';
	}
	else
		world.player[pid].reg_status = ' ';

	writeByte(lebuf, count, ((NLubyte)pid));
	writeByte(lebuf, count, ((NLubyte)world.player[pid].reg_status));					//regstatus
	writeLong(lebuf, count, ((NLulong)clid.rank));		//ranking#
	writeLong(lebuf, count, ((NLulong)clid.score));		//score POS
	writeLong(lebuf, count, ((NLulong)clid.neg_score));		//score NEG v0.4.8
	writeLong(lebuf, count, ((NLulong)max_world_rank));		//MAX WORLD ranking#
	writeLong(lebuf, count, ((NLulong)max_world_score));		//MAX WORLD score

	//LOG5("CRAPZ SENT TO CID %i of PID %i %c r:%i s:%i\n", cid, pid, world.player[pid].reg_status
	//	,client[world.player[pid].cid].rank
	//	,client[world.player[pid].cid].score);

	server->send_message(cid, lebuf, count);
}

//v0.4.5: broadcast player crap
void ServerNetworking::broadcast_player_crap(int pid) {

	for (int i=0;i<maxplayers;i++)
		if (world.player[i].used)
			send_player_crap_update(world.player[i].cid, pid);
}

// messages to update moved players (players/clients with new clients/players)
void ServerNetworking::move_update_player(int a) {
	if (world.player[a].used) {
		ctop[ world.player[a].cid ] = a;

		broadcast_player_name(a);
		send_me_packet(a);

		//v0.4.4 : tentativa de conserto
		//broadcast frags update
		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, data_frags_update);
		writeByte(lebuf, count, a);		// what player id
		writeLong(lebuf, count, world.player[a].frags);
		server->broadcast_message(lebuf, count);

		//v0.4.5 : atualiza registration char / score / rank
		broadcast_player_crap( a );

		//name (NEEDED? FIXME - ja tem la em cima!)
		//broadcast_player_name( a );

		//message
		bprintf("@I%s moved to %s team", world.player[a].name.c_str(), teamname[a/TSIZE]);
	}
}

//broadcast a sample
void ServerNetworking::broadcast_sample(int code) {

	char lebuf[64]; int count = 0;
	writeByte(lebuf, count, data_sound);
	writeByte(lebuf, count, (NLubyte)code);		// the sample code
	server->broadcast_message(lebuf, count);
}

//play a sample to a player's screen audience
void ServerNetworking::broadcast_screen_sample(int p, int code) {
	char lebuf[64]; int count = 0;
	writeByte(lebuf, count, data_sound);
	writeByte(lebuf, count, (NLubyte)code);		// the sample code
	broadcast_screen_message(world.player[p].roomx, world.player[p].roomy, (char*)lebuf, count);
}

//send current flag status (cid == -1 : broadcast)
void ServerNetworking::ctf_net_flag_status(int cid, int team) {
	//just resetting server state -- no update needed
	if (!server)
		return;

	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, data_flag_update);

	writeByte(lebuf, count, static_cast<NLubyte>(team));	//what team

	// how many flags
	writeByte(lebuf, count, static_cast<NLubyte>(world.teams[team].flags().size()));

	for (vector<Flag>::const_iterator fi = world.teams[team].flags().begin(); fi != world.teams[team].flags().end(); ++fi)
		if (fi->carried())	{ 			//carried?
			writeByte(lebuf, count, 1);	//TRUE
			//new flag carrier
			writeByte(lebuf, count, static_cast<NLubyte>(fi->carrier()));	//player who took it
		}
		else {
			writeByte(lebuf, count, 0);	//FALSE
			//new flag position
			writeByte(lebuf, count, static_cast<NLubyte>(fi->position().px));
			writeByte(lebuf, count, static_cast<NLubyte>(fi->position().py));
			writeShort(lebuf, count, static_cast<NLshort>(fi->position().x));
			writeShort(lebuf, count, static_cast<NLshort>(fi->position().y));
		}

	if (cid == -1)
		server->broadcast_message(lebuf, count);
	else
		server->send_message(cid, lebuf, count);
}

//update team scores
void ServerNetworking::ctf_update_teamscore(int t) {
	char lebuf[64]; int count = 0;
	writeByte(lebuf, count, data_score_update);
	writeByte(lebuf, count, static_cast<NLubyte>(t));		// the team
	writeByte(lebuf, count, static_cast<NLubyte>(world.teams[t].score()));	//the score
	server->broadcast_message(lebuf, count);
}

void ServerNetworking::broadcast_capture(const ServerPlayer& player) const {
	char lebuf[64];
	int count = 0;
	writeByte(lebuf, count, data_capture);
	writeByte(lebuf, count, static_cast<NLubyte>(player.id));
	server->broadcast_message(lebuf, count);
}

void ServerNetworking::broadcast_flag_take(const ServerPlayer& player) const {
	char lebuf[64];
	int count = 0;
	writeByte(lebuf, count, data_flag_take);
	writeByte(lebuf, count, static_cast<NLubyte>(player.id));
	server->broadcast_message(lebuf, count);
}

void ServerNetworking::broadcast_flag_return(const ServerPlayer& player) const {
	char lebuf[64];
	int count = 0;
	writeByte(lebuf, count, data_flag_return);
	writeByte(lebuf, count, static_cast<NLubyte>(player.id));
	server->broadcast_message(lebuf, count);
}

// player dropped the flag on purpose
void ServerNetworking::broadcast_flag_drop(const ServerPlayer& player) const {
	char lebuf[64];
	int count = 0;
	writeByte(lebuf, count, data_flag_drop);
	writeByte(lebuf, count, static_cast<NLubyte>(player.id));
	server->broadcast_message(lebuf, count);
}

void ServerNetworking::broadcast_kill(const ServerPlayer& attacker, const ServerPlayer& target, bool deathbringer, bool flag) const {
	char lebuf[64];
	int count = 0;
	writeByte(lebuf, count, data_kill);
	// first byte, deatbringer bit and attacker id
	NLubyte att_db = attacker.id;
	if (deathbringer)
		att_db |= 0x80;
	// second byte, flag bit and target id
	NLubyte tar_flag = target.id;
	if (flag)
		tar_flag |= 0x80;
	writeByte(lebuf, count, att_db);
	writeByte(lebuf, count, tar_flag);
	server->broadcast_message(lebuf, count);
}

void ServerNetworking::broadcast_suicide(const ServerPlayer& player, bool flag) const {
	char lebuf[64];
	int count = 0;
	writeByte(lebuf, count, data_suicide);
	NLubyte id_flag = player.id;
	if (flag)
		id_flag |= 0x80;
	writeByte(lebuf, count, id_flag);
	server->broadcast_message(lebuf, count);
}

void ServerNetworking::broadcast_spawn(const ServerPlayer& player) const {
	char lebuf[64];
	int count = 0;
	writeByte(lebuf, count, data_spawn);
	writeByte(lebuf, count, static_cast<NLubyte>(player.id));
	server->broadcast_message(lebuf, count);
}

void ServerNetworking::send_movements_and_shots(const ServerPlayer& player) const {
	char lebuf[64];
	int count = 0;
	writeByte(lebuf, count, data_movements_shots);
	for (int i = 0; i < maxplayers; i++)
		if (world.player[i].used) {
			writeLong(lebuf, count, static_cast<NLlong>(world.player[i].stats().movement()));
			writeShort(lebuf, count, static_cast<NLshort>(world.player[i].stats().shots()));
			writeShort(lebuf, count, static_cast<NLshort>(world.player[i].stats().hits()));
			writeShort(lebuf, count, static_cast<NLshort>(world.player[i].stats().shots_taken()));
		}
	server->send_message(player.cid, lebuf, count);
}

void ServerNetworking::send_stats(const ServerPlayer& player) const {
	char lebuf[256];
	int count = 0;
	writeByte(lebuf, count, data_stats);
	for (int i = 0; i < maxplayers; i++)
		if (world.player[i].used) {
			writeByte(lebuf, count, static_cast<NLubyte>(world.player[i].stats().kills()));
			writeByte(lebuf, count, static_cast<NLshort>(world.player[i].stats().deaths()));
			writeByte(lebuf, count, static_cast<NLubyte>(world.player[i].stats().cons_kills()));
			writeByte(lebuf, count, static_cast<NLubyte>(world.player[i].stats().current_cons_kills()));
			writeByte(lebuf, count, static_cast<NLshort>(world.player[i].stats().cons_deaths()));
			writeByte(lebuf, count, static_cast<NLubyte>(world.player[i].stats().current_cons_deaths()));
			writeByte(lebuf, count, static_cast<NLshort>(world.player[i].stats().suicides()));
			writeByte(lebuf, count, static_cast<NLubyte>(world.player[i].stats().captures()));
			writeByte(lebuf, count, static_cast<NLubyte>(world.player[i].stats().flags_taken()));
			writeByte(lebuf, count, static_cast<NLubyte>(world.player[i].stats().flags_dropped()));
			writeByte(lebuf, count, static_cast<NLubyte>(world.player[i].stats().flags_returned()));
			writeByte(lebuf, count, static_cast<NLubyte>(world.player[i].stats().carriers_killed()));
		}
	server->send_message(player.cid, lebuf, count);
}

void ServerNetworking::send_map_info(const ServerPlayer& player) {
	int count = 0;
	char lebuf[256];
	writeByte(lebuf, count, data_map_list);
	const vector<gameserver_c::MapInfo>::const_iterator mi = host->maplist().begin() + player.current_map_list_item;
	writeStr(lebuf, count, mi->title);
	writeStr(lebuf, count, mi->author);
	writeByte(lebuf, count, static_cast<NLchar>(mi->width));
	writeByte(lebuf, count, static_cast<NLchar>(mi->height));
	writeByte(lebuf, count, static_cast<NLchar>(mi->votes));
	server->send_message(player.cid, lebuf, count);
}

void ServerNetworking::broadcast_map_votes_update() {
	// check changed votes
	vector<pair<NLchar, NLchar> > votes;	// map number and votes
	NLchar i = 0;
	for (vector<gameserver_c::MapInfo>::iterator mi = host->maplist().begin(); mi != host->maplist().end(); ++mi, ++i)
		if (mi->votes_changed) {
			votes.push_back(pair<NLchar, NLchar>(i, mi->votes));
			mi->votes_changed = false;
		}

	// build packet
	int count = 0;
	char lebuf[256];
	writeByte(lebuf, count, data_map_votes_update);
	writeByte(lebuf, count, static_cast<NLchar>(votes.size()));
	for (vector<pair<NLchar, NLchar> >::const_iterator vi = votes.begin(); vi != votes.end(); ++vi) {
		writeByte(lebuf, count, vi->first);
		writeByte(lebuf, count, vi->second);
	}

	// send packet
	if (!votes.empty())
		for (int i = 0; i < maxplayers; i++)
			if (world.player[i].used)
				server->send_message(world.player[i].cid, lebuf, count);
}

//send map time left
void ServerNetworking::send_map_time(int cid) {
	if (!world.isTimeLimit())
		return;
	const NLulong time_left = max(0, world.getTimeLeft()) / 10;
	char lebuf[64]; int count = 0;
	writeByte(lebuf, count, data_map_time);
	writeLong(lebuf, count, time_left);
	if (cid == -1)
		server->broadcast_message(lebuf, count);
	else
		server->send_message(cid, lebuf, count);
}

//enqueue a job to the master server to update a client's delta score
void ServerNetworking::client_report_status(int id) {
	ClientData& clid = host->getClientData(id);

	if (!clid.token_have || !clid.token_valid)
		return;
	if (clid.delta_score == 0 && clid.neg_delta_score == 0)
		return;

	//submit-- create job
	masterjob_c *job = new masterjob_c();
	job->cid = id;
	job->code = 2;		// probably a code for mastejob thread

	//V0.4.8: envia POS e NEG deltascore
	sprintf(job->request, "GET /servlet/fcecin.tk1/index.html?%s&dscp=%i&dscn=%i&name=%s&token=%s\n\n", TK1_VERSION_STRING, clid.delta_score, clid.neg_delta_score, world.player[ ctop [id] ].name.c_str(), clid.token);

	//LOG2("== MJOB for REPORT STATUS : %i '''%s'''\n", id, job->request);

	pthread_t mjob_thread;
	MasterjobThreadData* mjtd = new MasterjobThreadData;
	mjtd->host = this;
	mjtd->job = job;
	pthread_mutex_lock ( &mjob_mutex );
	mjob_count++;
	pthread_mutex_unlock ( &mjob_mutex );
	pthread_create (&mjob_thread, 0, thread_masterjob_f, mjtd);	//#fix: make detached since it will never be joined

	//assume new score computed
	//clid.score += clid.delta_score;
	// NOT! new score will come...

	//reset the delta
	clid.delta_score = 0;
	clid.neg_delta_score = 0;
	clid.fdp = 0.0;
	clid.fdn = 0.0;
}

//broadcast team message
void ServerNetworking::broadcast_team_message(int team, char *text) {

	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, data_text_message);
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
void ServerNetworking::broadcast_screen_message(int px, int py, char *lebuf, int count) {
	for (int j=0;j<maxplayers;j++)
	if (world.player[j].used)
	if (world.player[j].roomx == px)
	if (world.player[j].roomy == py)
		server->send_message(world.player[j].cid, lebuf, count); //send the message
}

// V0.4.9 : broadcast message with varargs
void ServerNetworking::bprintf(const char *fs, ...) {
	//vsprintf...
	va_list argptr;
	char msg[16384];
	va_start(argptr, fs);
	vsprintf(msg, fs, argptr);
	va_end (argptr);

	//broadcast it
	broadcast_message(msg);
}
void ServerNetworking::plprintf(int pid, const char* fmt, ...) {	// bprintf for a single player
	char buf[16385];
	buf[0] = data_text_message;	// message type
	va_list argptr;
	va_start(argptr, fmt);
	vsprintf(buf+1, fmt, argptr);
	va_end(argptr);
	server->send_message(world.player[pid].cid, buf, strlen(buf)+1);
}

//send a single message player-printf
void ServerNetworking::player_message(int pid, const char *text) {
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, data_text_message);
	writeString(lebuf, count, text);
	if (world.player[pid].used)
		server->send_message(world.player[pid].cid, lebuf, count);
}

//broadcast message to all
void ServerNetworking::broadcast_message(const char *text) {
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, data_text_message);
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

//send map change message to a player
void ServerNetworking::send_map_change_message(int pid, int reason, const char* mapname) {

	char lebuf[256];
	int count = 0;
	writeByte(lebuf, count, data_map_change);

	writeByte(lebuf, count, 2);   // file map format
	writeShort(lebuf, count, world.map.crc);
	writeString(lebuf, count, mapname);
	writeByte(lebuf, count, static_cast<NLchar>(host->current_map_nr()));
	server->send_message(world.player[pid].cid, lebuf, count);

	//VERY IMPORTANT: flags the player as "awaiting map load" - client must confirm map to proceed
	world.player[pid].awaiting_client_ready = true;

	//send a show gameover plaque message, if that is the case
	if (reason != NEXTMAP_NONE) {
		count = 0;
		writeByte(lebuf, count, data_gameover_show);
		writeByte(lebuf, count, ((NLubyte)reason));		//capture limit plaque or vote exit plaque
		if ((reason == NEXTMAP_CAPTURE_LIMIT) || (reason == NEXTMAP_VOTE_EXIT)) {
			//informacoes para mostrar apos o jogo (time vencedor, most valuable player, etc.)
			writeByte(lebuf, count, static_cast<NLubyte>(world.teams[0].score()));	//RED team final score
			writeByte(lebuf, count, static_cast<NLubyte>(world.teams[1].score()));	//BLUE team final score
		}
		server->send_message(world.player[pid].cid, lebuf, count);
	}
}

bool ServerNetworking::start() {
	ping_send_client = 0;

	for (int i=0; i<256; ++i)
		ctop[i]=-1;
	player_count = 0;

	max_world_score = max_world_rank = 0;

	ping_send_client = 0;
	for (int i = 0; i < MAX_PLAYERS; ++i)
		fileTransfer[i].reset();

	//reset fslavesocks
	for (int ss=0;ss<MAX_PLAYERS;ss++)
		fslavesock[ss] = NL_INVALID;			//inicializa
	file_threads_quit = false;	//not yet

	// start server
	server = new_server_c();

	server->setHelloCallback(sfunc_client_hello);
	server->setConnectedCallback(sfunc_client_connected);
	server->setDisconnectedCallback(sfunc_client_disconnected);
	server->setDataCallback(sfunc_client_data);
	server->setLagStatusCallback(sfunc_client_lag_status);
	server->setPingResultCallback(sfunc_client_ping_result);

	server->setCallbackCustomPointer(this);

	server->set_client_timeout(5, 10);

	if (!server->start(port))
		return false;

	//v0.4.4 reset master jobs count
	mjob_count = 0;
	mjob_exit = false;				//flag for all pending master jobs to quit now
	mjob_fastretry = false;		//flag for all pending master jobs to stop waiting and retry immediately

	//v0.4.2 : calc TCP PORT

	NLboolean ok;

	//start TCP thread for talking with master server
	if (!privateserver) {
		master_talk_time = get_time();
		msock = NL_INVALID;		//not opened
		master_talk_time = get_time() + DELAY_TO_REPORT_SERVER;	//give it a break
		master_never_talked = true;		//not talked yet
		pthread_create(&mthread, 0, thread_mastertalker_f, this);
	}

	//start website thread
	websock = NL_INVALID;		//not opened
	pthread_create(&webthread, 0, thread_website_f, this);
	website_exiting_ok = false;

	//shell socket
	//v0.4.2 : new port
	int tcp_shell_port = port - 500;

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
	pthread_create(&shellmthread, 0, thread_shellmaster_f, this);
	pthread_create(&shellsthread, 0, thread_shellslave_f, this);
	return true;
}

//reload hostname
void ServerNetworking::reload_hostname() {
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
		strcpy(hostname, "Anonymous host");

	//update serverinfo
	update_serverinfo();
}

//update serverinfo
void ServerNetworking::update_serverinfo() {

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

int ServerNetworking::client_connected(int id) {

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
		host->refresh_team_score_modifiers();
		targ = TSIZE * host->getLessScoredTeam();
	}

	//alloc new player : scans only slots of the team (targ...targ + 7)
	int myself = -1;

	for (i=targ;i<(targ+TSIZE);i++) {
		if (!world.player[i].used) {
			// add player to players_present
			//players_present = players_present | (1 << i);

			// init player
			int cid;
			cid=id;
			ctop[cid]=i;

			world.player[i].clear(true, i, cid, "");

			myself = i;

			//reset keypresses
			world.player[i].controls = ClientControls();

			//check pickup creation
			world.check_pickup_creation(false);

			break;
		}
	}

	send_map_change_message(myself, NEXTMAP_NONE, host->getCurrentMapFile().c_str());

	// internal error can be detected if no player is free (used == false)
	//0.4.7: normal behavior (bots...)
	if (myself == -1) {
		//LOG("ERROR: BAD BAD BAD INTERNAL GAMESERVER ERROR myself == -1 !!! client_connected()...\n");
		return myself;	//-1...
	}

	//CONNECT OK: another one...
	player_count++;

	// spawn player
	world.player[myself].respawn_to_base = true;
	world.respawnPlayer(myself);

	// se o player_count ficou == 2, reseta partida
	//
	if (player_count == 2)
		host->ctf_game_restart();

	//char lelix[222];
	//sprintf(lelix, "PCOUNT = %i BCOUNT = %i\n", player_count, bot_count);
	//network.broadcast_message(lelix);

	char lebuf[256]; int count;

	host->resetPlayer(id);

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

	// - all other player's names
	// - all other player's frags

	for (i=0;i<maxplayers;i++) {
		if (!world.player[i].used)
			continue;
		if (i == myself)
			continue;

		send_player_name_update(id, i);

		//frags update
		count = 0;
		writeByte(lebuf, count, data_frags_update);
		writeByte(lebuf, count, i);		// what player id
		writeLong(lebuf, count, world.player[i].frags);
		server->send_message(id, lebuf, count);

		send_player_crap_update(id, i);
	}

	const vector<string>& welcome_message = host->getWelcomeMessage();
	for (vector<string>::const_iterator line=welcome_message.begin(); line!=welcome_message.end(); line++)
		world.player[myself].add_to_queue(*line);

	//check for team changes
	host->check_team_changes();

	//update serverinfo
	update_serverinfo();

	//send map time left if there is a time limit
	send_map_time(id);

	//send stats
	send_stats(world.player[myself]);

	//the first map info to be sent
	world.player[myself].current_map_list_item = 0;

	//ok!
	return myself;
}

//client disconnected (callback function)
//
// mesmas observacoes para client_connect. adicionalmente:
//
void ServerNetworking::client_disconnected(int id) {
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

	bprintf("@I%s left the game with %i frags", world.player[pid].name.c_str(), world.player[pid].frags);
	broadcast_sample(SAMPLE_LEFTGAME);

	//report the latest player achievements to the master server
	client_report_status(id);

	fileTransfer[id].reset();
	host->game_remove_player(pid);

	host->check_team_changes();
	host->check_map_exit();

	//update serverinfo
	update_serverinfo();
}

//client ping result
void ServerNetworking::ping_result(int client_id, int ping_time) {
	if (ctop[client_id]==-1)
		return;

	//save result
	world.player[ ctop[client_id] ].ping = ping_time;
}

void ServerNetworking::newPlayer(int pid) {
	bprintf("@I%s entered the game", world.player[pid].name.c_str());
	broadcast_sample(SAMPLE_ENTERGAME);
	if (shellssock) {
		char lebuf[256]; int count = 0;
		writeLong(lebuf, count, STA_PLAYER_IP);
		writeLong(lebuf, count, world.player[pid].cid);
		char addrBuf[50];
		NLaddress addr=get_client_address(world.player[pid].cid);
		nlSetAddrPort(&addr, 0);
		nlAddrToString(&addr, addrBuf);
		writeString(lebuf, count, addrBuf);
		nlWrite(shellssock, lebuf, count);
	}
}

void ServerNetworking::forwardSayadminMessage(int cid, const char* message) {
	if (!shellssock)
		return;
	char lebuf[256];
	int count=0;
	writeLong(lebuf, count, STA_ADMIN_MESSAGE);
	writeLong(lebuf, count, cid);
	writeString(lebuf, count, message);
	nlWrite(shellssock, lebuf, count);
}

//process incoming client data (callback function)
void ServerNetworking::incoming_client_data(int id, char *data, int length) {
	(void)length;
	if (ctop[id]==-1)
		return;

	//player id
	int pid = ctop[id];

	//1. process client's frame data
	int count = 0;

	NLubyte clFrame;
	readByte(data, count, clFrame);
	ServerPlayer& pl = world.player[pid];
	#ifdef WATCH_CONNECTION
	if (pl.lastClientFrame != clFrame) {
		if (static_cast<NLubyte>(pl.lastClientFrame - clFrame) < 128)
			plprintf(pid, "@WC>S packet order: prev %d this %d", pl.lastClientFrame, clFrame);
		else if (static_cast<NLubyte>(pl.lastClientFrame + 1) != clFrame)
			plprintf(pid, "@WC>S packet lost : prev %d this %d", pl.lastClientFrame, clFrame);
	}
	#endif
	if (static_cast<NLubyte>(pl.lastClientFrame - clFrame) >= 128) {	// this frame is very likely newer than the previous one
		pl.lastClientFrame = clFrame;
		#ifdef SEND_FRAMEOFFSET
		pl.frameOffset = 10. * (get_time() - frameSentTime);
		#endif

		NLubyte ccb;
		readByte(data, count, ccb);
		pl.controls.fromNetwork(ccb, false);

		const ClientControls& ctrl = pl.controls;
		//if not strafing, update direction
		if (!ctrl.isStrafe()) {
			// left
			if (ctrl.isLeft() && !ctrl.isRight()) {
				if (ctrl.isUp() && !ctrl.isDown())	// + up
					pl.gundir = 5;
				else if (!ctrl.isUp() && ctrl.isDown()) // + down
					pl.gundir = 3;
				else if (!ctrl.isUp() && !ctrl.isDown()) // !up !down
					pl.gundir = 4;
			}
			// right
			else if (!ctrl.isLeft() && ctrl.isRight()) {
				if (ctrl.isUp() && !ctrl.isDown())	// + up
					pl.gundir = 7;
				else if (!ctrl.isUp() && ctrl.isDown()) // + down
					pl.gundir = 1;
				else if (!ctrl.isUp() && !ctrl.isDown()) // !up !down
					pl.gundir = 0;
			}
			//!right !left
			else if (!ctrl.isLeft() && !ctrl.isRight()) {
				if (ctrl.isUp() && !ctrl.isDown())	// + up
					pl.gundir = 6;
				else if (!ctrl.isUp() && ctrl.isDown()) // + down
					pl.gundir = 2;
			}
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
			LOG1("MESSAGE FROM CLIENT, CODE=%i\n", code);
			if (code == data_name_update) {
				string name;
				readStr(msg, count, name);
				host->nameChange(id, pid, name);
			}
			else if (code == data_text_message) {
				host->chat(id, pid, msg+1);
			}
			//+attack
			else if (code == data_fire_on) {
				world.player[pid].attack = true;
			}
			//SUICIDE!!
			else if (code == data_suicide) {
				world.suicide(pid);
			}
			//-attack
			else if (code == data_fire_off) {
				world.player[pid].attack = false;
			}
			// want changeteams on
			else if (code == data_change_team_on) {
				if (!world.player[pid].want_change_teams) {
					world.player[pid].want_change_teams = true;
					//network.broadcast_message("@I%s player '%s' wants to change teams", teamname[pid/TSIZE], world.player[pid].name.c_str());
					host->check_team_changes();
					pid = ctop[id];
				}
			}
			// want changeteams off
			else if (code == data_change_team_off) {
				if (world.player[pid].want_change_teams) {
					world.player[pid].want_change_teams = false;
					world.player[pid].team_change_pending = false; //so pra garantir
					//network.broadcast_message("@I%s player '%s' don't want to change teams", teamname[pid/TSIZE], world.player[pid].name.c_str());
				}
			}
			// "client ready" message
			else if (code == data_client_ready) {
				//client is ready to play now
				world.player[pid].awaiting_client_ready = false;
			}
			// want change map
			else if (code == data_map_exit_on) {
				if (world.player[pid].want_map_exit == false) {
					world.player[pid].want_map_exit = true;
					host->check_map_exit();
				}
			}
			// dont' want change map
			else if (code == data_map_exit_off) {
				if (world.player[pid].want_map_exit == true) {
					world.player[pid].want_map_exit = false;
					host->check_map_exit();
				}
			}
			// v0.4.4: UDP DOWNLOAD of files - request
			else if (code == data_file_request) {
				char ftype[64];
				char fname[256];
				readString(msg, count, ftype);
				readString(msg, count, fname);
				if (fileTransfer[id].serving_udp_file) {
					// FIXME: ERROR: this client is already downloading a file
					LOG1("ERROR: CLIENT %i ALREADY DOWNLOADING A FILE!\n", id);
				}
				else {
					//alloc to download
					fileTransfer[id].serving_udp_file = true;
					char buffy[65536];		//buffy is our friend buffer
					int fsize = get_download_file((char *)buffy, ftype, fname);
					//error: can't read file FIXME: DISCONNECT THE CLIENT
					if (fsize == -1) {
						LOG("ERROR: CAN'T READ THE FILE CLIENT IS ASKING FOR. FIXME: MUST DISCONNECT HIM...\n");
					}
					else {
						fileTransfer[id].data = new char[fsize];	//allocated to fit!
						memcpy(fileTransfer[id].data, buffy, fsize);	//copy from buffy to the client's buffer
						fileTransfer[id].dp = 0;	//RESET FILE POINTER: important
						fileTransfer[id].fsize = fsize;
						//send a chunk
						upload_next_file_chunk(id);
					}
				}
			}
			// v0.4.4: UDP DOWNLOAD of files - the ack
			else if (code == data_file_ack) {
				//check expected ack
				NLulong ackpos;
				readLong(msg, count, ackpos);
				if (fileTransfer[id].old_dp == ackpos) {

					//check upload successful
					if (fileTransfer[id].dp >= fileTransfer[id].fsize) {
						//no more data, this was the last ack. close stuff
						fileTransfer[id].reset();	//reset the download data structs
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
			else if (code == data_registration_token) {
				string tok;
				readStr(msg, count, tok);
				if (host->changeRegistration(id, tok)) {
					//v0.4.5 : atualiza registration char / score / rank
					broadcast_player_crap( ctop[id] );

					// ENQUEUE TOKEN VALIDATION IN A QUEUE THAT TALKS TO THE MASTER SERVERS

					//create job
					masterjob_c *job = new masterjob_c();
					job->cid = id;
					job->code = 1;
					sprintf(job->request, "GET /servlet/fcecin.tk1/index.html?%s&chktk&name=%s&token=%s\n\n", TK1_VERSION_STRING, world.player[ ctop [id] ].name.c_str(), tok.c_str());

					//LOG2("== MJOB CREATED : %i '''%s'''\n", id, job->request);

					pthread_t mjob_thread;
					MasterjobThreadData* mjtd = new MasterjobThreadData;
					mjtd->host = this;
					mjtd->job = job;
					pthread_mutex_lock ( &mjob_mutex );
					mjob_count++;
					pthread_mutex_unlock ( &mjob_mutex );
					pthread_create (&mjob_thread, 0, thread_masterjob_f, mjtd);	//#fix: make detached since it will never be joined
				}
			}
			// drop flag
			else if (code == data_drop_flag) {
				world.player[pid].drop_key = true;
				world.player[pid].dropped_flag = true;
				world.dropFlagIfAny(pid, true);
			}
			// stop dropping flag
			else if (code == data_stop_drop_flag)
				world.player[pid].drop_key = false;
			// map vote
			else if (code == data_map_vote) {
				NLbyte vote;
				readByte(msg, count, vote);
				if (world.player[pid].mapVote != vote) {
					if (world.player[pid].mapVote >= 0 && world.player[pid].mapVote < static_cast<int>(host->maplist().size()))
						host->maplist()[world.player[pid].mapVote].votes_changed = true;
					if (vote >= 0 && vote < static_cast<int>(host->maplist().size()))
						host->maplist()[vote].votes_changed = true;
					world.player[pid].mapVote = vote;
					host->check_map_exit();
				}
			}
			else {
				//ERROR: unknown message from client
				LOG3("ERROR: UNKNOWN MESSAGE FROM CLIENT %i CODE=%i LENGTH=%i\n", id, code, msglen);
			}
		}
	} while (msg != 0);
}

void ServerNetworking::disconnect_client(int cid, int timeout) {
	server->disconnect_client(cid, timeout);
}

void ServerNetworking::sendEndGameover() {
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, data_gameover_hide);
	server->broadcast_message(lebuf, count);
}

//simulate and broadcast frame
void ServerNetworking::broadcast_frame(bool gameRunning) {

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
	static int tviter[2] = { 0, 0 };
	static int helmiter = 0;		// HELM ITERATOR : manda todo mundo!
	bool tview[2][MAX_PLAYERS / 2];	//[time][inimigo# visto? 1-8]
	NLushort tview_bits[2];			//enemy view SHORT (bitfield for the 8 enemies of each team(0,1))

	//HELM PATCH: the "helm view" bytes for both teams - if somebody has helm, he will se
	//  helmview[] for his team
	NLushort helmview[2];

	//atualiza HELM ITERATOR - para em um player valido ou entao qualquer um
	int runaway = maxplayers + 1;
	do {
		helmiter++;
		if (helmiter > maxplayers - 1)
			helmiter = 0;
		if (world.player[helmiter].used && !world.player[helmiter].item_helm() || world.player[helmiter].flag())
			break;
	} while (runaway-- > 0);

	//atualiza tview E HELMVIEW
	for (int t = 0; t < 2; t++) {
		tview_bits[t] = 0;
		helmview[t] = 0;		//default zero

		for (int i = 0; i < maxplayers; i++)			// p/ cada inimigo desse time
			if (i / TSIZE == 1 - t && world.player[i].used) {
				// ---- helmview -----
				// mostra se NAO TEM HELM ou SE TA COM FLAG
				if (!world.player[i].item_helm() || world.player[i].flag())
					helmview[t] += static_cast<NLushort>(1 << (i % TSIZE));

				// ---- tview -----
				tview[t][i % TSIZE] = false;		// invisible

				for (int j = 0; j < maxplayers; j++)
					if (j / TSIZE == t && world.player[j].used)
						if (world.player[j].roomx == world.player[i].roomx && world.player[j].roomy == world.player[i].roomy)
							if (world.player[i].visibility > 0 || world.player[i].flag()) { // visible
								tview[t][i % TSIZE] = true;
								tview_bits[t] |= (1 << (i % TSIZE));	// set visibility bit
								break;
							}
		}

		//avanca tviter do time p/ escolher alguem
		int runaway = maxplayers + 3;
		do {
			//avanca proximo candidato a envio
			tviter[t]++;
			if (tviter[t] >= maxplayers)
				tviter[t] = 0;

			//testa se o candidato se aplica ao visor minimap do time
			//testa apenas used players
			if (world.player[tviter[t]].used) {
				// same team, OK
				if (tviter[t] / TSIZE == t)
					break;
				// enemy team, check if visible
				if (tviter[t] / TSIZE == 1 - t && tview[t][tviter[t] % TSIZE])
					break;
			}
		} while (runaway-- > 0);
	}

	// ==================================================================
	//   BUILD AND SEND EVERY DAMN PACKET
	// ==================================================================
	for (int i=0; i<maxplayers; i++) {
		if (!world.player[i].used)
			continue;

		//rewrite past common data
		lecount = count;

		NLubyte clFrame = world.player[i].lastClientFrame;
		writeByte(lebuf, lecount, clFrame);
		#ifdef SEND_FRAMEOFFSET
		NLubyte fo = static_cast<NLubyte>( bound<float>(world.player[i].frameOffset, 0., .999) * 256. );
		writeByte(lebuf, lecount, fo);
		#endif

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

		bool skip_frame = world.player[i].awaiting_client_ready || !gameRunning;

		if (skip_frame) xtra += 4;		// VERY IMPORTANT
		writeByte(lebuf, lecount, xtra);

		// ****** VERY IMPORTANT ******
		// send almost empty frame if client not ready (leave bandwidth for data transfer) OR IF
		// server showing gameover plaque
		if (!skip_frame) {
			// NEW: 0.3.9 : send before players_onscreen 2 bytes with the screen of self
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

			for (int j = 0; j < maxplayers; j++) {
				// player j exists, in same room, visible or in same team or has a flag
				if ((players_present & (1 << j)) != 0 &&
						world.player[j].roomx == world.player[i].roomx && world.player[j].roomy == world.player[i].roomy &&
						(world.player[j].visibility > 0 || i / TSIZE == j / TSIZE || world.player[j].flag())) {
					//add to players_onscreen
					players_onscreen += (1 << j);

					const ServerPlayer& h = world.player[j];

					//V0.3.9: took out screen from here

					//V0.3.9 : transmissao x,y de 4 bytes para 3
					NLubyte xy;
					NLushort hx,hy;
					hx = (NLushort)h.lx;
					hy = (NLushort)h.ly;

					xy = (NLubyte) (hx & 255);
					writeByte(lebuf, lecount, xy);		//first 8 bits x
					xy = (NLubyte) (hy & 255);
					writeByte(lebuf, lecount, xy);		//first 8 bits y
					//256+512+1024+2048 = 3840    last 4 bits mask
					xy = (NLubyte) ( ((hx & 0xF00) >> 8) + ((hy & 0xF00) >> 4) ); //x: bit 8-11 to 0-3  y: bit 8-11 to 4-7
					writeByte(lebuf, lecount, xy);   //last 4 bits x + last 4 bits y

					//sho = ((NLshort)h.x);
					//writeShort(lebuf, lecount, sho);	//x
					//sho = ((NLshort)h.y);
					//writeShort(lebuf, lecount, sho);	//y

					//speed em bytes - xinelao mesmo
					NLbyte sxy;
					sxy = ((NLbyte)(h.sx * 2));
					writeByte(lebuf, lecount, sxy);
					sxy = ((NLbyte)(h.sy * 2));
					writeByte(lebuf, lecount, sxy);

					//sho = ((NLshort)(h.sx * 100));
					//writeShort(lebuf, lecount, sho );	//sx  30.283482345634... = 30283 = 30.283(depois)
					//sho = ((NLshort)(h.sy * 100));
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

					NLubyte ccb;
					if (world.player[j].health > 0)	// if dead player, don't send keys
						ccb = world.player[j].controls.toNetwork(true);
					else
						ccb = 0;
					ccb |= h.gundir << 5;
					writeByte(lebuf, lecount, ccb);

					writeByte(lebuf, lecount, (NLubyte)world.player[j].visibility);
				}
			}

			//update players_onscreen (it's before the players on screen data (above))
			writeLong(lebuf, p_on_count, players_onscreen);

			//ELMO: visao alem do alcance!!
			NLubyte who;
			if (world.player[i].item_helm()) {
				//team "viewed enemies" do meu time (i/TSIZE)
				//writeByte(lebuf, lecount, 255);		// todos!!!
				//FIX: helm nao enxerga todo mundo nao
				//team "viewed enemies" do meu time (i/TSIZE)
				//writeByte(lebuf, lecount, tview_bits[i/TSIZE]);
				//FIX: helm tambem nao eh no viewed enemies. o helm de um time é 255 (todos)
				//     menos quem tem helm
				writeShort(lebuf, lecount, helmview[i/TSIZE] | tview_bits[i/TSIZE]);

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

		//send server map list if not sent yet
		if (world.player[i].current_map_list_item < host->maplist().size()) {
			send_map_info(world.player[i]);
			++world.player[i].current_map_list_item;
		}
	}

	// map votes update
	if (world.frame % 10 == 0)
		broadcast_map_votes_update();

	// stats update
	if (world.frame / MAX_PLAYERS % 5 == 0) {
		const int pid = world.frame % MAX_PLAYERS;
		if (world.player[pid].used)
			send_movements_and_shots(world.player[pid]);
	}

	// PING: v0.4.1
	// envia um ping a cada frame, faz revezamento entre todos os players
	ping_send_client++;		//next player
	if (ping_send_client >= maxplayers)
		ping_send_client = 0;
	if (world.player[ping_send_client].used) // valid player?
		server->ping_client(world.player[ping_send_client].cid); //ping

	#ifdef SEND_FRAMEOFFSET
	frameSentTime = get_time();
	#endif
}

double ServerNetworking::getTraffic() {
	return ( server->get_socket_stat(NL_AVE_BYTES_RECEIVED) + server->get_socket_stat(NL_AVE_BYTES_SENT) ) / 1024.;
}

//a master job response is obtained: parse it
void ServerNetworking::master_job_response(masterjob_c *j) {
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

					ClientData& clid = host->getClientData(j->cid);

					//OK!
					char lebuf[128]; int count = 0;
					writeByte(lebuf, count, data_registration_response);
					writeByte(lebuf, count, 1);		// OK!
					server->send_message(j->cid, lebuf, count);

					//set this player as being recorded as of now
					clid.token_valid = true;		//validated his token

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
					clid.score = atoi(pb);
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
					clid.neg_score = atoi(pb);
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
					clid.rank = atoi(pb);
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
					writeByte(lebuf, count, data_registration_response);
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

					ClientData& clid = host->getClientData(j->cid);

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
					clid.score = atoi(pb);
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
					clid.neg_score = atoi(pb);
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
					clid.rank = atoi(pb);
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
void ServerNetworking::run_masterjob_thread(masterjob_c* job) {
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
bool ServerNetworking::check_private_IP(char *address) {
	int i1, i2;
	int n=sscanf(address, "%d.%d.", &i1, &i2);
	nAssert(n==2);
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
void ServerNetworking::run_mastertalker_thread() {

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

void ServerNetworking::run_website_thread() {
	LOG("run_website_thread()\n");

	string address;
	if (force_ip)
		address = force_ip_name;
	else {
		//don't force: resolve
		NLaddress myadr;
		get_self_IP(&myadr);
		char addr[NL_MAX_STRING_LENGTH];
		nlAddrToString(&myadr, addr);
		address = addr;
	}

	const string web_settings("website.txt");
	// load web script location
	ifstream in(web_settings.c_str());
	if (!in) {
		LOG1("Website thread: Can not open %s. Quit.\n", web_settings.c_str());
		website_exiting_ok = true;
		return;
	}
	string site_name, site_ip, site_script, site_auth;
	if (!getline(in, site_name) || !getline(in, site_ip) || !getline(in, site_script)) {
		LOG1("Website thread: No valid format in %s. Quit.\n", web_settings.c_str());
		website_exiting_ok = true;
		return;
	}
	getline(in, site_auth);
	in.close();
	if (site_script.empty() || (site_name.empty() && site_ip.empty())) {
		LOG1("Website thread: No script or site location in %s. Quit.\n", web_settings.c_str());
		website_exiting_ok = true;
		return;
	}

	NLaddress website_address;
	double website_talk_time = 0.0;
	bool first_connection = true;

	do {
		if (get_time() > website_talk_time) {
			website_talk_time = get_time() + 2 * 60.0;		// 2 minutes
			LOG("Website thread: Start sending information to server website.\n");
			nlEnable(NL_BLOCKING_IO);
			websock = nlOpen(0, NL_RELIABLE);
			nlDisable(NL_BLOCKING_IO);
			if (websock == NL_INVALID) {
				LOG("Website thread: Server can't open socket to connect to server website!\n");
				continue;
			}
			if (!nlGetAddrFromName(site_name.c_str(), &website_address)) {
				const NLchar* const reason = nlGetSystemErrorStr(nlGetSystemError());
				LOG2("Website thread: Can't get IP address for %s! Reason: %s\n", site_name.c_str(), reason);
			}
			int web_port = nlGetPortFromAddr(&website_address);
			if (!web_port) {
				web_port = 80;
				nlSetAddrPort(&website_address, web_port);
			}
			if (!website_address.valid || nlConnect(websock, &website_address) == NL_FALSE) {		// connect
				const NLchar* const reason = nlGetSystemErrorStr(nlGetSystemError());
				LOG3("Website thread: Server can't connect to %s:%d! Reason: %s\n", site_name.c_str(), web_port, reason);
				nlStringToAddr(site_ip.c_str(), &website_address);
				web_port = nlGetPortFromAddr(&website_address);
				if (!web_port) {
					web_port = 80;
					nlSetAddrPort(&website_address, web_port);
				}
				if (nlConnect(websock, &website_address) == NL_FALSE) {	// connect to IP address
					const NLchar* const reason = nlGetSystemErrorStr(nlGetSystemError());
					LOG2("Website thread: Server can't connect to %s! Reason: %s\n", site_ip.c_str(), reason);
					nlClose(websock);
					websock = NL_INVALID;
					continue;
				}
				else {	// save new name for web server
					NLchar new_name[NL_MAX_STRING_LENGTH];
					nlGetNameFromAddr(&website_address, new_name);
					if (new_name && site_name != new_name) {
						ofstream out(web_settings.c_str());
						out << new_name << '\n' << site_ip << '\n' << site_script << '\n' << site_auth << '\n';
						out.close();
						LOG2("Website thread: Saved new name (%s) for IP address %s.\n", new_name, site_ip.c_str());
					}
				}
			}
			else {	// save new IP address for web server
				NLchar new_address[NL_MAX_STRING_LENGTH];
				nlAddrToString(&website_address, new_address);
				if (site_ip != new_address) {
					ofstream out(web_settings.c_str());
					out << site_name << '\n' << new_address << '\n' << site_script << '\n' << site_auth << '\n';
					out.close();
					LOG2("Website thread: Saved new IP address (%s) for %s.\n", new_address, site_name.c_str());
				}
			}

			// build and send data
			map<string, string> parameters = website_parameters(address);
			if (first_connection) {		// send maplist
				parameters["maplist"] = website_maplist();
				first_connection = false;
			}
			const string data = build_http_data(parameters);
			NLint result = post_http_data(site_script, data, site_auth);
			LOG("Website thread: Sent information to server website:\n");
			LOG1("\t%s", data.c_str());
			LOG1("\tResult: %i\n", result);
			if (result == -1)
				website_talk_time = get_time() + 15.0;		// 15 seconds

			// save response to a file
			ofstream out("web.log");
			save_http_response(out);
			out.close();

			//close socket
			nlClose(websock);
			websock = NL_INVALID;
		}

		//no: sleep a bit
		MS_SLEEP(500);
	} while (!file_threads_quit);

	LOG("Website thread: time to say goodbye\n");

	//qutting: close the socket
	if (websock != NL_INVALID) {
		LOG("Website thread: bye 1\n");
		nlClose(websock);
		websock = NL_INVALID;
	}

	//quitting: send server shutdown message to web script
	//open socket
	//nlDisable(NL_BLOCKING_IO);			//nonblocking socket, let's make this simple...
	nlEnable(NL_BLOCKING_IO);
	websock = nlOpen(0, NL_RELIABLE);
	nlDisable(NL_BLOCKING_IO);

	if (websock == NL_INVALID) {
		LOG("Website thread: (Quite) Server can't open socket to connect to server website!\n");
		website_exiting_ok = true;
		return;
	}

	//connect
	if (nlConnect(websock, &website_address) == NL_FALSE) {		//connect
		LOG1("Website thread: (Quit) Server can't connect to %s!\n", site_name.c_str());
		nlClose(websock);
		websock = NL_INVALID;
		website_exiting_ok = true;
		return;
	}

	// send quit message
	const string quit = "quit=1\r\n";
	NLint result = post_http_data(site_script, quit, site_auth);
	LOG("Website thread: Sent information to server website:\n");
	LOG1("\t%s", quit.c_str());
	LOG1("\tResult: %i\n", result);

	// save response to a file
	ofstream out("web.log");
	save_http_response(out);
	out.close();

	//close socket
	nlClose(websock);
	websock = NL_INVALID;

	website_exiting_ok = true;
}

map<string, string> ServerNetworking::website_parameters(const string& address) const {
	map<string, string> parameters;
	parameters["name"] = hostname;
	parameters["ip"] = address;
	ostringstream p;
	p << port;
	parameters["port"] = p.str();
	ostringstream pc;
	pc << player_count;
	parameters["players"] = pc.str();
	ostringstream mpc;
	mpc << maxplayers;
	parameters["max_players"] = mpc.str();
	parameters["version"] = GAME_VERSION;
	ostringstream upt;
	upt << world.frame / 10;
	parameters["uptime"] = upt.str();
	parameters["map"] = host->current_map().title;
	parameters["mapfile"] = host->getCurrentMapFile();
	parameters["info"] = "Test server";
	return parameters;
}

string ServerNetworking::website_maplist() const {
    ostringstream maps;
    for (vector<gameserver_c::MapInfo>::const_iterator m = host->maplist().begin(); m != host->maplist().end(); m++) {
        if (m != host->maplist().begin())
            maps << '\n';
        maps << m->title;
    }
    return maps.str();
}

string ServerNetworking::build_http_data(const map<string, string>& parameters) const {
	// URL encode parameter values
	ostringstream param_line;
	for (map<string, string>::const_iterator i = parameters.begin(); i != parameters.end(); i++) {
		for (string::const_iterator s = i->first.begin(); s != i->first.end(); s++)
			url_encode(*s, param_line);
		param_line << '=';
		for (string::const_iterator s = i->second.begin(); s != i->second.end(); s++)
			url_encode(*s, param_line);
		param_line << '&';
	}
	param_line << "\r\n";
	return param_line.str();
}

NLint ServerNetworking::post_http_data(const string& script, string parameters, const string& auth) const {
	char lebuf[65536]; int count = 0;
	ostringstream data;
	const string password = base64_encode(auth);
	parameters += "passwd=";
	parameters += password;
	data << "POST " << script << " HTTP/1.0\r\n";
	data << "User-Agent: Outgun " << GAME_VERSION << "\r\n";
	data << "Authorization: Basic " << password << "\r\n";
	data << "Content-Type: application/x-www-form-urlencoded\r\n";
	data << "Content-Length: " << parameters.length() << "\r\n\r\n";
	writeStr(lebuf, count, data.str()); count--;
	writeStr(lebuf, count, parameters); count--;
	return nlWrite(websock, lebuf, count);
}

void ServerNetworking::save_http_response(ostream& out) const {
	const double timeout = get_time() + 60.0;
	const int buffer_size = 511;
	char lebuf[buffer_size + 1];
	NLint result;
	do {
		// read
		result = nlRead(websock, lebuf, buffer_size);
		lebuf[result] = '\0';
		// save
		out << lebuf;
	} while (result == buffer_size && get_time() <= timeout && !file_threads_quit);
}

void ServerNetworking::url_encode(char c, ostream& out) const {
	if (is_url_safe(c))	// send safe characters as they are
		out << c;
	else if (c == ' ')	// spaces to + characters
		out << '+';
	else				// encode unsafe characters to %xx
		out << '%' << hex << setw(2) << setfill('0') << static_cast<int>(static_cast<unsigned char>(c));
}

bool ServerNetworking::is_url_safe(char c) const {
	if (c >= 'a' && c <= 'z')
		return true;
	else if (c >= 'A' && c <= 'Z')
		return true;
	else if (c >= '0' && c <= '9')
		return true;
	const string safe_characters = "$-_.+!*'(),";
	return safe_characters.find(c) != string::npos;
}

string ServerNetworking::base64_encode(const string& data) const {
	const string conversion_table("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
	const char padding = '=';
	string result;
	// Convert data to 6-bit sequences. Take characters for every sequence
	// from the conversion table.
	for (string::const_iterator s = data.begin(); s != data.end(); s++) {
		// first encoded byte
		char value = (*s >> 2) & 0x3F;
		result += conversion_table[value];
		// second encoded byte
		value = (*s << 4) & 0x3F;
		s++;
		if (s != data.end())
			value |= (*s >> 4) & 0x0F;
		result += conversion_table[value];
		// third encoded byte
		if (s != data.end()) {
			value = (*s << 2) & 0x3F;
			s++;
			if (s != data.end())
				value |= (*s >> 6) & 0x03;
			result += conversion_table[value];
		}
		else
			result += padding;
		// fourth encoded byte
		if (s != data.end()) {
			value = *s & 0x3F;
			result += conversion_table[value];
		}
		else
			result += padding;
	}
	return result;
}

//read a string from a blocking TCP stream, one char at a time
bool ServerNetworking::read_string_from_TCP(NLsocket sock, char *buf) {
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
void ServerNetworking::run_shellmaster_thread() {

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
				writeStr(lebuf, count, world.player[i].name);
				writeLong(lebuf, count, STA_PLAYER_IP);
				writeLong(lebuf, count, world.player[i].cid);
				char addrBuf[50];
				NLaddress addr = get_client_address(world.player[i].cid);
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
void ServerNetworking::run_shellslave_thread() {
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
					host->mutePlayer(pid, arg);
				break;
			case ATS_KICK_PLAYER:
				result = nlRead(shellssock, rbuf, 4);
				rcount = 0; readLong(rbuf, rcount, clid); pid = ctop[clid];
				if (result == 4 && pid != -1)
					host->kickPlayer(pid);
				break;
			#ifdef SV_NAME_AUTHORIZATION
			case ATS_BAN_PLAYER:
				result = nlRead(shellssock, rbuf, 4);
				rcount = 0; readLong(rbuf, rcount, clid); pid = ctop[clid];
				if (result == 4 && pid != -1)
					host->banPlayer(pid);
				break;
			#endif
			case ATS_RESET_SETTINGS:
				host->reset_settings(true);
				break;
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

void ServerNetworking::stop() {
	//submit all pending player reports
	for (int i=0; i<maxplayers; i++)
		if (world.player[i].used)
			client_report_status(world.player[i].cid);

	//v0.4.4 : flag master job threads to start trying to resolve themselves quickly
	mjob_fastretry = true;
	double mjmaxtime = get_time() + 30.0;		//timeout : 30 seconds

	server_status_string("Shutdown: Net Server");

	if (server)
		server->stop(3);
	else
		throw 8384;

	//reset client_c struct (closes files...)
	for (int i=0; i<MAX_PLAYERS; i++)
		fileTransfer[i].reset();

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

	//thread for website interface
	for (int waitcount = 0; !website_exiting_ok; waitcount++) {
		if (waitcount > 30) {		// 30 * 100ms = 3 seconds
			LOG("Tired of waiting for website thread...\n");
			//kill the socket
			server_status_string("Shutdown: Website Socket");
			nlClose(websock);		//close AGAIN (it's a different one)
			websock = NL_INVALID;
			break;
		}
		MS_SLEEP(100);
	}

	//thread for TCP connection that server uses to register it's IP on the master-server
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

void ServerNetworking::sendWeaponPower(int pid) {
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, data_weapon_change);
	writeByte(lebuf, count, ((NLubyte)world.player[pid].weapon) );
	server->send_message(world.player[pid].cid, lebuf, count);
}

void ServerNetworking::sendRocketMessage(int shots, int gundir, NLubyte* sid, int team, bool power,
												int px, int py, int x, int y) {	// sid = shot-id; array of NLubyte[shots]
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, data_rocket_fire);
	//NLubyte  powerdir;		//bits 0..4 = power bits 5..8=dir
	//powerdir = NLubyte(shots) | NLubyte(gundir<<4);
	//writeByte(lebuf, count, powerdir);	// power and dir
	writeByte(lebuf, count, shots);			// power and dir
	writeByte(lebuf, count, gundir);		// power and dir
	for (int i=0; i<shots; i++)
		writeByte(lebuf, count, sid[i]);	// rocket-object id (needed because client-side rockets can be deleted by the server)
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

void ServerNetworking::sendRocketDeletion(NLulong plymask, int rid, NLshort hitx, NLshort hity, int targ) {
	//assembly rocket delete message
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, data_rocket_delete);
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

void ServerNetworking::sendDeathbringer(int pid, const ServerPlayer& ply) {
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, data_deathbringer);
	writeByte(lebuf, count, ((NLubyte)pid));	//team/target player
	writeLong(lebuf, count, world.frame);		//frame # of the bringer shot (message can be delayed)
	writeByte(lebuf, count, ((NLubyte)ply.roomx));
	writeByte(lebuf, count, ((NLubyte)ply.roomy));
	writeShort(lebuf, count, ((NLushort)ply.lx));
	writeShort(lebuf, count, ((NLushort)ply.ly));

	server->broadcast_message(lebuf, count);
}

void ServerNetworking::sendPickupVisible(int pid, int pup_id, const pickup_c& it) {
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, data_pup_visible);
	writeByte(lebuf, count, (NLubyte)pup_id);	//what item
	writeByte(lebuf, count, (NLubyte)it.kind);	//kind
	writeByte(lebuf, count, (NLubyte)it.px);		//screen
	writeByte(lebuf, count, (NLubyte)it.py);
	writeShort(lebuf, count, (NLushort)it.x);	//pos in screen
	writeShort(lebuf, count, (NLushort)it.y);
	server->send_message(world.player[pid].cid, lebuf, count);
}

void ServerNetworking::sendPupTime(int pid, NLubyte pupType, double timeLeft) {
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, data_pup_timer);
	writeByte(lebuf, count, pupType);
	writeShort(lebuf, count, (NLushort)timeLeft);
	server->send_message(world.player[pid].cid, lebuf, count);
}

void ServerNetworking::sendFragUpdate(int pid, NLulong frags) {
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, data_frags_update);
	writeByte(lebuf, count, pid);		// what player id
	writeLong(lebuf, count, frags);
	server->broadcast_message(lebuf, count);
}

NLaddress ServerNetworking::get_client_address(int cid) const {
	return server->get_client_address(cid);
}

void ServerNetworking::sfunc_client_hello(void* customp, int client_id, char* data, int length, ServerHelloResult* res) {
	((ServerNetworking*)customp)->clientHello(client_id, data, length, res);
}

void ServerNetworking::clientHello(int client_id, char* data, int length, ServerHelloResult* res) {
	(void)length;	//#fix
	res->customDataLength = 0;

	//LOG1("hello client %i!\n", arg->client_id);

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

	readString(data, count, stri);	//read gamestring

	if (strcmp(stri, GAME_STRING)) {
		LOG2("GAME STRINGS DONT MATCH: %s and %s\n", GAME_STRING, stri);
		res->accepted = false;		// not accepted

		sprintf(temp, "Different game: '%s'", stri);
		writeString(res->customData, res->customDataLength, temp);
	}
	else {
		readString(data, count, stri);	//read protocol string

		if (strcmp(stri, GAME_PROTOCOL)) {
			LOG2("GAME PROTOCOL STRINGS DONT MATCH: %s and %s\n", GAME_PROTOCOL, stri);
			res->accepted = false;

			sprintf(temp, "Protocol mismatch: server: %s, client: %s", GAME_PROTOCOL, stri);
			writeString(res->customData, res->customDataLength, temp);
		}
		else if (player_count >= maxplayers) {		//server full!
			LOG("....unfortunatelly the server is FULL! hello rejected\n");
			res->accepted = false;

			sprintf(temp, "Server is full. (%i players)", player_count);
			writeString(res->customData, res->customDataLength, temp);
		}
		#ifdef SV_NAME_AUTHORIZATION
		else if (host->isBanned(client_id)) {
			res->accepted = false;
			writeString(res->customData, res->customDataLength, "You are banned");
		}
		#endif
		else {
			res->accepted = true;
			writeByte(res->customData, res->customDataLength, ((NLubyte)maxplayers));
			writeString(res->customData, res->customDataLength, hostname);
		}
	}
}

void ServerNetworking::sfunc_client_connected(void* customp, int client_id) {
	((ServerNetworking*)customp)->client_connected(client_id);
}

void ServerNetworking::sfunc_client_disconnected(void* customp, int client_id) {
	((ServerNetworking*)customp)->client_disconnected(client_id);
}

void ServerNetworking::sfunc_client_lag_status(void* customp, int client_id, int status) {
	(void)(customp && client_id && status);
}

void ServerNetworking::sfunc_client_data(void* customp, int client_id, char* data, int length) {
	((ServerNetworking*)customp)->incoming_client_data(client_id, data, length);
}

void ServerNetworking::sfunc_client_ping_result(void* customp, int client_id, int pingtime) {
	((ServerNetworking*)customp)->ping_result(client_id, pingtime);
}

//============================================================
//  TCP server admin shell interaction thread
//============================================================

//thread for server shell connections
void* ServerNetworking::thread_shellmaster_f(void* arg) {
	((ServerNetworking*)arg)->run_shellmaster_thread();
	return 0;
}

void* ServerNetworking::thread_shellslave_f(void* arg) {
	((ServerNetworking*)arg)->run_shellslave_thread();
	return 0;
}

//============================================================
//  TCP master server (web servlet) interaction thread -- LIST SERVER
//============================================================

//thread for master server interaction
void* ServerNetworking::thread_mastertalker_f(void* arg) {
	((ServerNetworking*)arg)->run_mastertalker_thread();
	return 0;
}

//============================================================
//  a single independent job to the master server
//============================================================

void* ServerNetworking::thread_masterjob_f(void* arg) {
	MasterjobThreadData* p = static_cast<MasterjobThreadData*>(arg);
	p->host->run_masterjob_thread(p->job);
	delete p;
	return 0;
}


//============================================================
//  server website interaction thread
//============================================================

void* ServerNetworking::thread_website_f(void* arg) {
	((ServerNetworking*)arg)->run_website_thread();
	return 0;
}

