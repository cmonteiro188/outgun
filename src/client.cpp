#include <cctype>
#include "commont.h"
#include "world.h"
#include "names.h"
#include "leetnet/client.h"
#include "leetnet/rudp.h"	// get_self_IP
#include "leetnet/sleep.h"	// sleep util
#include "client.h"
#include "network.h"
#include "nassert.h"

//#define CLIENT_PREDICTION
const float lagWanted = .5;

#ifdef NIX
void set_close_button_callback(void (*fn)()) {
	set_window_close_hook(fn);
}
#endif

gameclient_c *gameclient;	//#fix: get rid

class ClientPhysicsCallbacks : public PhysicsCallbacksBase {
	gameclient_c& c;

public:
	ClientPhysicsCallbacks(gameclient_c& c_) : c(c_) { }

	bool collideToRockets() const { return false; }
	bool gatherMovementDistance() const { return false; }
	bool allowRoomChange() const { return false; }
	void addMovementDistance(int, float) { }
	void playerScreenChange(int) { }
	void rocketHitWall(int rid, bool power, float x, float y, int roomx, int roomy) { c.rocketHitWallCallback(rid, power, x, y, roomx, roomy); }
	bool rocketHitPlayer(int, int) { return false; }
	void playerHitWall(int pid) { c.playerHitWallCallback(pid); }
	void rocketOutOfBounds(int rid) { c.rocketOutOfBoundsCallback(rid); }
	bool shouldApplyPhysicsToPlayer(int pid) { return c.shouldApplyPhysicsToPlayerCallback(pid); }
};

// client callbacks
int cfunc_connection_update(client_runes_t *arg);
int cfunc_server_data(client_runes_t *arg);

//client password-to-token retrieval thread
void *thread_clientpassword_f(void *arg);

bool gameclient_c::force_exit = false;
const size_t gameclient_c::chat_size = 32;

bool gameclient_c::start() {
	// gfx init
	if (!client_graphics.reset_video_mode())		// fatal error
		return false;

	set_close_button_callback(gameclient_c::close_button_callback);

	// open message log file
	if (message_logging)
		message_log.open("message.log", ios::app);

	//default physics parameters
	//set_default_physics();
	//LOG3("\nNORMAL   fri %.1f acc %.1f mxs %.1f\n", svp_fric, svp_accel, svp_maxspeed);
	//LOG3("RUN      fri %.1f acc %.1f mxs %.1f\n", svp_fric_run, svp_accel_run, svp_maxspeed_run);
	//LOG3("TURBO    fri %.1f acc %.1f mxs %.1f\n", svp_fric_turbo, svp_accel_turbo, svp_maxspeed_turbo);
	//LOG3("TURBORUN fri %.1f acc %.1f mxs %.1f\n", svp_fric_turborun, svp_accel_turborun, svp_maxspeed_turborun);

	//clear UDPDQ
	for (int uq=0;uq<MAX_UDPDQ;uq++) udpdq[uq] = 0;
	udpdq_ptr = -1;
	udpdq_size = 0;

	//hide helpscreen
	helpshow = false;

	totalframecount = 0;
	framecount = 0;

	clFrameSent = clFrameWorld = 0;
	frameReceiveTime = 0;

	#ifdef SEND_FRAMEOFFSET
	frameOffsetDeltaTotal = 0;
	frameOffsetDeltaNum = 0;
	#endif
	averageLag = 0;

	// default map
	//load_default_map(&map);
	map_ready = false;		// NO map change commands from server yet
	servermap[0] = 0;

	//not showing gameover plaque
	gameover_plaque = NEXTMAP_NONE;

	trying_connection = false;
	connected = false;

	client = new_client_c();
	client->set_callback(CFUNC_CONNECTION_UPDATE, cfunc_connection_update);
	client->set_callback(CFUNC_SERVER_DATA, cfunc_server_data);

	//init gamespy/adresses
	first_fav_refresh = false;
	showmaster = true;
	gi = 0;
	for (int i=0;i<MAX_GAMESPY;i++) {
		gamespy[i].address[0]=0;
		gamespy[i].refreshed = false;
		gamespy[i].invalid = false;	//don't know the status yet
		//master gamespy:
		mgamespy[i].address[0]=0;
		mgamespy[i].refreshed = false;
		mgamespy[i].invalid = false;	//don't know the status yet
	}

	//assume no password
	player_token_set = false;			//no token
	player_password_set = false;	//no password
	namestatus = "NO PASSWORD SET";
	namestatus_code = 0;

	//try to load the client's password
	char dest[WHERE_PATH_SIZE];
	int c;
	append_filename(dest, wheregamedir, "password.bin", WHERE_PATH_SIZE);
	FILE *psf = fopen(dest, "rb");
	if (psf) {

		char pas[PASSBUFFER];
		for (c=0;c<PASSBUFFER;c++) {
			int cha = fgetc(psf);
			if (cha == EOF) break;
			pas[c] = (char)cha;
		}

		//read all?
		if (c == PASSBUFFER) {
			//toggle bits
			int rot;
			for (int d=0;d<PASSBUFFER;d++) {
				rot = pas[d];
				rot = 255 - rot;
				pas[d] = (char)rot;
			}
			//get the password
			pas[8] = 0;
			editplayerpass = pas;
			//copy to editpass and simulate pressing ENTER on the name/pass screen...
			check_change_pass_command();
		}

		fclose(psf);
	}

	//try to load client configuration
	bool randomname = true; // give random name
	append_filename(dest, wheregamedir, "clconfig.txt", WHERE_PATH_SIZE);
	LOG1("dest for clconfig.txt = %s\n", dest);

	ifstream cfg(dest);
	if (cfg) {
		string dir;
		//read sound theme directory name
		if (getline_smart(cfg, dir)) {
			client_sounds.set_theme_dir(dir);
			LOG1("Sound theme directory default = %s\n", dir.c_str());
		}

		//read graphics theme directory name
		if (getline_smart(cfg, dir)) {
			client_graphics.set_theme_dir(dir);
			LOG1("Graphics theme directory default = %s\n", dir.c_str());
		}

		//read player name
		string name;
		if (getline_smart(cfg, name)) {
			randomname = false;
			playername = name;
		}

		//read addresses
		for (int i = 0; i < MAX_GAMESPY; i++) {
			string addr;
			if (getline_smart(cfg, addr)) {
				strncpy(gamespy[i].address, addr.c_str(), 127);
				strncpy(mgamespy[i].address, addr.c_str(), 127); //copy to master list too!
			}
		}
		cfg.close();
	}

	//give a random name
	if (randomname)
		playername = RandomName();

	client_sounds.search_themes();
	client_graphics.search_themes();

	//refresh master!
	get_servers_from_master();

	return true;
}

//send "client ready" message to server (when map load and/or download completes)
void gameclient_c::send_client_ready() {
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, data_client_ready);
	client->send_message(lebuf, count);		// bem curtinha a mensagem mesmo...
}

// check if password has changed.
// if NO, do nothing
// if YES: send a new request to the master server
// V0.4.6 : always re-log client if ENTER pressed! (if player doesn't want to log again, should press ESC instead)
void gameclient_c::check_change_pass_command() {
	//player_password_set = false is a flag for the token retrieve thread
	//join with it
	if (player_password_set) {
		player_password_set = false;
		pthread_join( passthread, 0 );
	}

	//NO TOKEN, not anymore...
	player_token_set = false;

	//empty?
	if (editplayerpass.empty()) {
		namestatus = "NO PASSWORD SET";
		namestatus_code = 0;
		player_password = "";

	}
	else {
		//non-empty! copy stuff
		namestatus = "STARTING LOGIN...";
		player_password = editplayerpass;

		//setup stuff for the new thread
		player_password_set = true;	//don't quit the thread
		player_token_new = true;	//getting a NEW token, not refreshing the token

		//*** request new token for this password --- a new, independent thread ***
		pthread_create(&passthread, 0, thread_clientpassword_f, (void *)this);	//will call function below (sucks but works)
	}
}

//THREAD for getting a token from a password. nonblocking TCP operations, if
// player_password_set == false, then quit immediately
void gameclient_c::client_password_thread(void *) {

	NLsocket sock = NL_INVALID;

	while (player_password_set == true) {

		//open a nonblocking socket
		nlDisable(NL_BLOCKING_IO);
		sock = nlOpen(0, NL_RELIABLE);
		if (sock == NL_INVALID) {
			//show "cant open socket to master" error
			namestatus = "SOCKET ERROR. RETRYING...";
			MS_SLEEP(3000);	//five secs
			continue;				//again...
		}

		//connect the nonblocking way
		nlConnect(sock, &master_address);

		//build query
		char blux[1024];
		if (player_token_new)
			sprintf(blux, "GET /servlet/fcecin.tk1/index.html?%s&new&name=%s&password=%s\n\n", TK1_VERSION_STRING, playername.c_str(), player_password.c_str());
		else
			sprintf(blux, "GET /servlet/fcecin.tk1/index.html?%s&old&name=%s&password=%s\n\n", TK1_VERSION_STRING, playername.c_str(), player_password.c_str());

		char querybuf[1024]; int qcount = 0;
		writeString(querybuf, qcount, blux);
		qcount--;	//take the zero out

		namestatus = "SENDING LOGIN...";

		//keep trying to write the query.
		NLint result;
		do {
			result = nlWrite(sock, querybuf, qcount);
			MS_SLEEP(50);

			//qutting?
			if (player_password_set == false)
				break;

		} while ((result == NL_INVALID) && (nlGetError() == NL_CON_PENDING));

		//qutting?
		if (player_password_set == false)
			continue;

		namestatus = "WAITING RESPONSE...";

		//try to read the reply
		//parse the response (should be <HTML><BODY> etc... with "@I @I @I ... @K" on it
		bool html_end = false;
		int nostuffcound = 0;
		char lebuf[65536];
		int n = 0;
		do {

			//read
			result = nlRead(sock, &(lebuf[n]), 1);

			//quitting?
			if (player_password_set == false)
				break;

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
						namestatus = "NO RESPONSE. RETRYING...";
						MS_SLEEP(3000);
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
				namestatus = "ERROR. RETRYING...";
				MS_SLEEP(3000);
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
					html_end = true;
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

		//found it?
		if (html_end) {

			//FIRST THINGS FIRST: close the socket
			nlClose(sock);
			sock = NL_INVALID;

			//parse the result (FIXME)
			//if SUCCESS (got token) then semi-busy-wait until told to quit or time
			//  to send a new token request
			//if FAILED then update the status and quit

			namestatus = "RECEIVED RESPONSE!";

			bool ok = false, wrongid = false, unavailable = false;
			LOG1("RECV RESPONSE n = %i\n", n);
			for (int i = 0; i < n; i++) {

				//0.4.7: "can't contact servlet runner.." > service unavailable
				if ((i>21)
					&&
					(
					((lebuf[i-21] == 'C') || (lebuf[i-21] == 'c')) &&
					((lebuf[i-20] == 'O') || (lebuf[i-20] == 'o')) &&
					((lebuf[i-19] == 'N') || (lebuf[i-19] == 'n')) &&
					((lebuf[i-18] == 'T') || (lebuf[i-18] == 't')) &&
					((lebuf[i-17] == 'A') || (lebuf[i-17] == 'a')) &&
					((lebuf[i-16] == 'C') || (lebuf[i-16] == 'c')) &&
					((lebuf[i-15] == 'T') || (lebuf[i-15] == 't')) &&
					((lebuf[i-14] == ' ') || (lebuf[i-14] == ' ')) &&
					((lebuf[i-13] == 'S') || (lebuf[i-13] == 's')) &&
					((lebuf[i-12] == 'E') || (lebuf[i-12] == 'e')) &&
					((lebuf[i-11] == 'R') || (lebuf[i-11] == 'r')) &&
					((lebuf[i-10] == 'V') || (lebuf[i-10] == 'v')) &&
					((lebuf[i-9] == 'L') || (lebuf[i-9] == 'l')) &&
					((lebuf[i-8] == 'E') || (lebuf[i-8] == 'e')) &&
					((lebuf[i-7] == 'T') || (lebuf[i-7] == 't')) &&
					((lebuf[i-6] == ' ') || (lebuf[i-6] == ' ')) &&
					((lebuf[i-5] == 'R') || (lebuf[i-5] == 'r')) &&
					((lebuf[i-4] == 'U') || (lebuf[i-4] == 'u')) &&
					((lebuf[i-3] == 'N') || (lebuf[i-3] == 'n')) &&
					((lebuf[i-2] == 'N') || (lebuf[i-2] == 'n')) &&
					((lebuf[i-1] == 'E') || (lebuf[i-1] == 'e')) &&
					((lebuf[i] == 'R') || (lebuf[i] == 'r')))) {
					unavailable = true;
				}
				//control char
				else if (lebuf[i] == '@') {

					//LOG("(( @ ))\n");
					i++;

					if (lebuf[i] == 'K') {

						//LOG("(( @K ))\n");
						//scan the token....
						i++;
						player_token[0]=0;
						int pt=0;
						while (lebuf[i] != '#') {
							//LOG("(( TOK ))\n");
							player_token[pt]=lebuf[i];		//cat char
							player_token[pt+1]=0;
							if (pt > 15) {
								//error... give up... tokens aren't so long...
								break;
							}
							i++;	//next.
							pt++;
						}
						//okeydokey...?
						if (pt <= 15) {
							ok = true;
						}
					}
					else if ((lebuf[i] == 'E') || (lebuf[i] == 'F')) //query Error / Failed login (wrong name/pass)
					{
						//LOG("(( @E || @F ))\n");
						wrongid = true;
					}
					else {
						//UNKNOWN code... yuck.
					}
				}
			}

			//OK?
			if (ok) {
				namestatus = "LOGGED [";
				namestatus += player_token;
				namestatus += "]";
				namestatus_code = 1;

				//OK!
				player_token_set = true;
				player_token_new = false;

				//--- if connected, update token ---
				if (connected)
					send_player_token();

				//wait xxx minutes to send again	//*2 == halfsecond
				for (int busy=0;busy<60*10*2;busy++) {		//10 MINUTES
					MS_SLEEP(500);
					if (player_password_set == false)
						break;
				}
			}
			//WRONG ID?
			else if (wrongid) {
				namestatus = "ERROR: WRONG ID!";
				namestatus_code = 2;
				LOG2("ERROR WRONG ID. QUERY='''%s''' LEBUF='''%s'''\n", blux, lebuf);
				//wait xxx minutes to send again	//*2 == halfsecond
				for (int busy=0;busy<60*10*2;busy++) {		//10 MINUTES
					MS_SLEEP(500);
					if (player_password_set == false)
						break;
				}
			}
			//UNAVAILABLE?
			else if (unavailable) {
				namestatus = "SERVER UNAVAILABLE";
				namestatus_code = 2;
				LOG2("ERROR UNKNOWN!!! QUERY='''%s''' LEBUF='''%s'''\n", blux, lebuf);
				//wait xxx minutes to send again	//*2 == halfsecond
				for (int busy=0;busy<60*1*2;busy++) {		//1 MINUTE
					MS_SLEEP(500);
					if (player_password_set == false)
						break;
				}
			}
			//WHAT???
			else {
				namestatus = "UNKNOWN ERROR!";
				namestatus_code = 2;
				LOG2("ERROR UNKNOWN!!! QUERY='''%s''' LEBUF='''%s'''\n", blux, lebuf);
				//wait xxx minutes to send again	//*2 == halfsecond
				for (int busy=0;busy<60*10*2;busy++) {		//10 MINUTES
					MS_SLEEP(500);
					if (player_password_set == false)
						break;
				}
			}
		}
		else {
			//failed, just retry (go on with the loop)
		}

	}//WHILE(password set)

	// =====
	//CLOSE SOCKET AND WAIT UNTIL SETPASS==FALSE
	// =====
	if (sock != NL_INVALID)
		nlClose(sock);
	while (player_password_set == true) {
		MS_SLEEP(500);
	}
}

// incoming chunk of requested file by UDP
void gameclient_c::process_udp_download_chunk(int last, NLulong pos, int len, char* buf) {

	//progress...
	fdp = pos + len;

	//write it
	fseek(ud_fout, pos, 0);		//0 == "SEEK_SET" ??
	int amount = fwrite(buf, 1, len, ud_fout);

	//this is bad! but will never happen.
	if (amount != len) {
		LOG2("BAD BAD ERROR! process_udp_download_chunk can't fwrite len %i amount %i !!!", len, amount);
		// FIXME: better handling
	}

	//send the reply
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, data_file_ack);
	writeLong(lebuf, count, pos);		// acked pos (just to be sure...)
	client->send_message(lebuf, count);

	//if done, then we're done!
	// - take out of queue
	// - notify app
	if (last) {
		pthread_mutex_lock ( &udpdq_mutex );

		//close the file
		fclose(ud_fout);

		// THANK GOD I DON'T HAVE TO REWRITE THIS!!
		download_file_complete(udpdq[udpdq_ptr]);

		// remove from queue - record deleted by download_file_complete() call above
		udpdq[udpdq_ptr] = 0;
		udpdq_size--;		//less one

		// no download
		udpdq_ptr = -1;

		// readily check for next download, if any on the queue
		//
		for (int i=0;i<MAX_UDPDQ;i++)
			if (udpdq[i] != 0) {
				//found: start the new download right now
				udpdq_ptr = i;
				client_udp_setup_download();
				break;
			}

		pthread_mutex_unlock ( &udpdq_mutex );
	}
}

//do the download setup
//must be called with udpdq_mutex locked
void gameclient_c::client_udp_setup_download() {

	//to simplify things...
	download_runes_t  *r = udpdq[udpdq_ptr];

	//open dest file for output (ud_fout)
	ud_fout = fopen(r->dest, "wb");
	if (ud_fout) {
		LOG1("UDP client_download_thread() file '%s' opened\n", r->dest);
	}
	else {
		//do something if can't write to the file (disconnect player/whatever)
		LOG1("UDP client_download_thread() can't write output file!! (%s)", r->dest);
		disconnect_command();		//FIXME make it better
		return;
	}

	//reset ud_fp
	ud_fp = 0;			//file pointer (progress...)

	//request the file and wait...
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, data_file_request);
	writeString(lebuf, count, r->type);
	writeString(lebuf, count, r->name);
	client->send_message(lebuf, count);
}

//add to UDP DOWNLOAD QUEUE
void gameclient_c::client_udp_download(download_runes_t  *rune) {

	pthread_mutex_lock ( &udpdq_mutex );

	for (int i=0;i<MAX_UDPDQ;i++)
		if (udpdq[i] == 0) {

			//add to empty pos on queue
			udpdq[i] = rune;		//copy to "queue"
			udpdq_size++;				//another one

			//setup ptr to download, if ptr free
			if (udpdq_ptr == -1) {
				udpdq_ptr = i;
				//check download
				client_udp_setup_download();
			}

			//anyway, we're done
			pthread_mutex_unlock ( &udpdq_mutex );
			return;
		}

	//error that will never happen even in a million years
	LOG("BAD BAD **ERROR** : UDPDQ IS FULL");
	throw 66640;	//BAD ERROR

	pthread_mutex_unlock ( &udpdq_mutex );
}

//file download complete
void gameclient_c::download_file_complete(download_runes_t  *r) {

	LOG3("download_file_complete '%s' '%s' '%s'\n", r->type, r->name, r->dest);

	//map complete
	if (!strcmp(r->type, "map")) {

		//if expected map, change now
		if (!strcmp(r->name, servermap)) {
			bool ok = fd.load_map(CLIENT_MAPS_DIR, r->name) && fx.load_map(CLIENT_MAPS_DIR, r->name);	//#fix
			if (!ok)
				LOG1("AFTER DOWNLOAD: MAP '%s' NOT FOUND\n", r->name)
			else {
				LOG1("AFTER DOWNLOAD: MAP '%s' LOADED SUCCESSFULLY!\n", r->name);

				//load ok!  (FIXME: tell server)
				//
				client_graphics.update_minimap_background(fx.map);	// recalc minimap
				predraw();
				map_ready = true;								// map ready to show
				send_client_ready();				//send "client ready" to server
			}
		}
	}

	//delete the download record
	delete r; r = 0;
}

//start downloading a server file
void gameclient_c::download_server_file(const char *type, const char *name, char *dest) {

	//new download request
	download_runes_t	*rune = new download_runes_t();
	strcpy(rune->type, type);
	strcpy(rune->name, name);
	strcpy(rune->dest, dest);

	client_udp_download(rune);
}

//server tells client of current map / map change
// client must attempt to load map from "cmaps" dir
// if map file not there, or the CRC's don't match, ask to download the map from the server
void gameclient_c::server_map_command(const char *mapname, NLushort server_crc) {

	LOG1("CLIENT: server_map_command : '%s'\n", mapname);

	//try to load the map. will fail if not found
	bool ok = fd.load_map(CLIENT_MAPS_DIR, mapname) && fx.load_map(CLIENT_MAPS_DIR, mapname);	//#fix

	if (!ok)
		LOG1("MAP '%s' NOT FOUND\n", mapname)
	else if (fx.map.crc != server_crc)
		LOG3("MAP '%s' FOUND BUT ITS CRC %i DIFFERS FROM SERVER MAP CRC %i\n", mapname, fx.map.crc, server_crc)
	else {
		LOG1("MAP '%s' LOADED SUCCESSFULLY!\n", mapname);

		//load ok!  (FIXME: tell server)
		//
		client_graphics.update_minimap_background(fx.map);  // recalc minimap
		predraw();
		map_ready = true;  // map ready to show
		send_client_ready();				//send "client ready" to server
	}

	// download map from server (ask file)
	if (!ok || fx.map.crc != server_crc) {

		char lix[256];
		sprintf(lix, "Client: downloading map '%s' (CRC %i)...", mapname, server_crc);
		print_message(lix);

		LOG(lix);

		// MAKE DOWNLOAD -- ASK FILE

		char fname[256];
		strcpy(fname, CLIENT_MAPS_DIR);
		put_backslash(fname);
		strcat(fname, mapname);
		strcat(fname, ".txt");

		char dest[1024];	//full destination path for file
		append_filename(dest, wheregamedir, fname, WHERE_PATH_SIZE);

		//copy to name of map waiting -- for when download_server_file completes
		strcpy(servermap, mapname);

		//download server file -- opens new thread and TCP conection
		download_server_file("map", mapname, dest);
	}
}

// sounds
void gameclient_c::sound(int s) const {
	client_sounds.play(s);
}

//update the scoreboard
void gameclient_c::update_scoreboard() {

	//reset used players/used scoreboard entries
	bool scoreused[MAX_PLAYERS];
	for (int f=0;f<MAX_PLAYERS;f++) {
		scoreused[f] = false;
		scoreboard[f] = -1;
	}

	// fill each team
	for (int t=0;t<2;t++)	{

		//team delta
		int td = t * TSIZE;

		//itera do 1o ao 8o slot
		for (int s=td;s<TSIZE+td;s++) {

			//itera do 1o jogador ao 8o jogador do time
			//busca o maior que ainda nao foi usado
			int maxfrag = -666;
			int maxwho = -1;
			for (int i=td;i<TSIZE+td;i++)
			if (fx.player[i].used)
			if (!scoreused[i])		// ainda nao usado
			if (fx.player[i].frags > maxfrag) {
				//achou maior
				maxfrag = fx.player[i].frags;
				maxwho = i;
			}

			//aloca se achou
			if (maxwho != -1) {
				scoreboard[s] = maxwho;
				scoreused[maxwho] = true;
			}

		}//itera slots
	}//itera times
}

//show a specific menu screen
void gameclient_c::set_menu(Menu_selection menumber) {
	menu = menumber;
	clear_keybuf(); //clear keystrokes buffer
}

//disconnect command
void gameclient_c::disconnect_command() {

	//disconnect the client here if was connected, else does nothing
	client->connect(false);

	//dialogz
	dialogmessage = "You are disconnected. Press ESC.";
	dialogmessage2.clear();
	set_menu(menu_dialog);
}

void gameclient_c::client_connected(char *data, int length) {
	(void)length;

	//not trying anymore
	trying_connection = false;

	//"data" from connection accepted:
	//  BYTE		maxplayers
	//  STRING	hostname
	int count = 0;
	NLubyte maxpl;
	readByte(data, count, maxpl);
	maxplayers = maxpl;

	readStr(data, count, hostname);
	hostname = hostname.substr(0, 32);	//truncate at 32 chars

	chat_visible = 8;

	//set window title: the hostname
	ostringstream caption;
	caption << "Connected to: " << hostname << " (" << address << ')';
	server_status_string(caption.str());

	//default scoreboard state
	for (int i = 0; i < MAX_PLAYERS; i++)
		scoreboard[i] = i;

	//don't want to change teams by default
	want_change_teams = false;

	//don't want to exit map by default
	want_map_exit = false;

	//avoid "dropped" plaque
	lastpackettime = get_time() + 1.0;

	clFrameSent = clFrameWorld = 0;
	frameReceiveTime = 0;

	// reset gamestate?
	connected = true;
	gameshow = true;
	menu = menu_none;
	fx.frame = fd.frame = 0;
	fx.skipped = fd.skipped = true;
	me = -1;	//don't know who am I

	//reset chat buffer
	talkbuffer.clear();
	chatbuffer.clear();
	chaterasetime = get_time() + 10.0;

	//reset world data
	// players

	for (int i = 0; i < MAX_PLAYERS; i++)
		fx.player[i].clear(false, i, "(name unknown)");

	//reset FPS count vars
	framecount = 0;
	frameCountStartTime = get_time();
	FPS = 666.0;
	
	//reset map time
	map_time_limit = false;
	map_end_time = 0;

	//send name update request
	issue_change_name_command();

	//init game frame state
	//fx.frame = 0;				//hmm
	//fx.skipped = true;	//hmm
	//fd.frame = 0;				//hmm
	//fd.skipped = true;	//hmm

	// MOVED FROM GAMECLIENT_C::START():

	// default map
	//load_default_map(&map);
	map_ready = false;		// NO map change commands from server yet
	servermap[0]=0;

	maps.clear();

	//not showing gameover plaque
	gameover_plaque = NEXTMAP_NONE;

	//clear client side effects
	client_graphics.clear_fx();

	send_frame(true);
}

void gameclient_c::client_disconnected() {

	//restore window title
	server_status_string("Outgun client - CTRL+F12 to quit");

	// the gamestate?
	connected = false;
	gameshow = false;

	// show a message
	dialogmessage = "You have been disconnected. Press ESC.";
	dialogmessage2.clear();
	set_menu(menu_dialog);

	//namestatus
	if (namestatus_code == 0)
		namestatus = "NO PASSWORD SET?";
	else if ((namestatus_code == 1) || (namestatus_code == 3)) {
		namestatus = "LOGGED (";
		namestatus += player_token;
		namestatus += ")";
	}
	else if (namestatus_code == 2) {
		namestatus = "ERROR: WRONG ID?";
	}

}

void gameclient_c::connect_failed_denied(char *data, int length) {
	//not trying anymore
	trying_connection = false;

	//extract message
	string message;
	if (length > 0) {
		int count = 0;
		readStr(data, count, message);
	}
	else
		message = "no reason given.";

	// show a message
	dialogmessage = "Connection refused. Press ESC.";
	dialogmessage2 = message;
	if (message == "PASSWORD")
		set_menu(menu_server_password);
	else
		set_menu(menu_dialog);
}

void gameclient_c::connect_failed_unreachable() {

	//not trying anymore
	trying_connection = false;

	// show a message
	dialogmessage = "No response from server. Press ESC.";
	dialogmessage2.clear();
	set_menu(menu_dialog);
}

//refresh servers command
void gameclient_c::refresh_command() {

	if (showmaster)
		refresh_command_2(mgamespy);
	else
		refresh_command_2(gamespy);
}

//refresh servers command
void gameclient_c::refresh_command_2(gamespy_t *gamespy) {

	client_graphics.show_progress("", "Refreshing servers...", "");

	NLsocket sock = nlOpen(0, NL_UNRELIABLE);

	if (sock == NL_INVALID) {
		LOG2("LIXAO!!!!!! %s %s\n", nlGetErrorStr(nlGetError()), nlGetSystemErrorStr(nlGetSystemError()) );
		return;
	}

	char   lebuf[512];

	//debug
	char dinfo[2000];
	char lix[256];
	strcpy(dinfo, "D=");

	// no response from all calc addresses num_valid
	//
	double st[MAX_GAMESPY][4];	//send time
	int	rc[MAX_GAMESPY];		//resposta count
	double rt[MAX_GAMESPY];		//resposta time
	int num_valid = 0;
	for (int i = 0; i < MAX_GAMESPY; i++) {

		rc[i] = 0;	//no responses
		rt[i] = 0;	//no time

		gamespy[i].noresponse = true;
		gamespy[i].invalid = true; // invalid entry for default
		gamespy[i].refreshed = true;	//refreshed now

		nlOpen(0, 0);//force invalid enum error

		nlStringToAddr(gamespy[i].address, &gamespy[i].addr);

		//test if the address is invalid (important)
		if (nlGetError() == NL_INVALID_ENUM) {

			nlOpen(0, 0);//force invalid enum error

			//v0.4.2: if address has no port or has invalid port, set it to the default value (25000)
			int door = nlGetPortFromAddr(&gamespy[i].addr);
			if (door == 0)
				nlSetAddrPort(&gamespy[i].addr, DEFAULT_UDP_PORT); //port);//server PORT!!!!!!

			//test if set was ok
			if (nlGetError() == NL_INVALID_ENUM) {

				gamespy[i].invalid = false; // non-invalid entry
				num_valid ++;
			}
		}
	}

	//send
	//
	for (int t=0;t<4;t++) { //send four times

		// (1) SEND
		//
		for (int i = 0; i < MAX_GAMESPY; i++)
		if (!gamespy[i].invalid)
		{
			int count = 0;
			writeLong(lebuf, count, 0);			//special packet
			writeLong(lebuf, count, 200);		//serverinfo request
			writeByte(lebuf, count, (NLubyte)i);		//connect entry (am I lazy or what)
			writeByte(lebuf, count, (NLubyte)t);		//packet number

			nlSetRemoteAddr(sock, &gamespy[i].addr);
			int res = nlWrite(sock, lebuf, count);	//send
			st[i][t] = get_time();	//for ping measure

			sprintf(lix, "%i,", res);
			strcat(dinfo, lix);

		}//send loop


		//(2) pause before each send
		//
	for (int bla = 0; bla < 20; bla++)
	{
		MS_SLEEP(5);			//*** NO CPU PROBLEM HERE ***

		//(3) collect any responses so far
		//
		// [h0ly] 'i' will be setted by the readByte later on
		int i;
		int am = 0;
		do {
			am = nlRead(sock, lebuf, 512);
			if (am > 0) {
				strcat(dinfo, "R,");

				int count = 0;
				NLulong along;
				NLubyte pack;
				readLong(lebuf, count, along); // should be 0..
				if (along == 0) {
					readLong(lebuf, count, along); // should be 200...
					if (along == 200) {
						readByte(lebuf, count, i); // client's gamespy entry
						readByte(lebuf, count, pack); // packet #
						readString(lebuf, count, gamespy[i].info);

						//add to ping statistics
						rc[i]++;
						rt[i] += get_time() - st[i][pack];

						if (gamespy[i].noresponse)	//dec replies expected count
							num_valid--;
						gamespy[i].noresponse = false;	//response obtained
					}
				}
			}
		} while (am > 0);
	}

	}

	//(4) wait for mising responses for a timeout period  (1.5 seconds)
	//
	for (int wa=0;wa<300;wa++) {

		//quit when done
		if (num_valid <= 0)
			break;

		//sleep a bit
		MS_SLEEP(5);	//*** NO CPU PROBLEM HERE ***

		//collect responses

		int i;
		int am = 0;
		do {
			am = nlRead(sock, lebuf, 512);
			if (am > 0) {

				strcat(dinfo,"R,");

				int count = 0;
				NLulong along;
				NLubyte pack;
				readLong(lebuf, count, along); // should be 0..
				if (along == 0) {
					readLong(lebuf, count, along); // should be 200...
					if (along == 200) {
						readByte(lebuf, count, i); // client's gamespy entry
						readByte(lebuf, count, pack); // packet #
						readString(lebuf, count, gamespy[i].info);

						//add to ping statistics
						rc[i]++;
						rt[i] += ( get_time() - st[i][pack] );

						if (gamespy[i].noresponse)	//dec replies expected count
							num_valid--;
						gamespy[i].noresponse = false;	//response obtained
					}
				}
			}
		} while (am > 0);

	}

	// add ping to statistics
	//
	for (int i = 0; i < MAX_GAMESPY; i++)
		if (!gamespy[i].noresponse)	{	//got at least 1 response?
			int daping;
			if (rc[i] > 0)
				daping = (int)(1000.0 * rt[i] / rc[i]);
			else
				daping = -666;

			char thelix[2000];
			sprintf(thelix, "%4i %s", daping, gamespy[i].info);
			strcpy(gamespy[i].info, thelix);
	}

	nlClose(sock);
}

//connect command
void gameclient_c::connect_command() {

	// disconnect
	//
	client->connect(false);

	// copy gamespy address
	//
	if (showmaster)
		address = mgamespy[gi].address;
	else
		address = gamespy[gi].address;

	// start connecting to specified IP/port
	// connection results will come through the CFUNC_CONNECTION_UPDATE callback
	if (address.empty()) {	//empty address == my own ip
		NLaddress myadr;
		get_self_IP(&myadr);
		nlSetAddrPort(&myadr, port);
		char addr[256];
		nlAddrToString(&myadr, addr);
		address = addr;
	}
	else if (address.find(':') == string::npos) {	// no port, use default
		ostringstream port;
		port << ':' << DEFAULT_UDP_PORT;
		address += port.str();
	}

	client->set_server_address(address.c_str());

	//set connect-data (goes in every connect packet): outgun game name and version strings
	char lebuf[256]; int count = 0;
	writeString(lebuf, count, GAME_STRING);
	writeString(lebuf, count, GAME_PROTOCOL);
	if (!edit_server_password.empty()) {
		writeStr(lebuf, count, edit_server_password);
		edit_server_password.clear();	// clear password to avoid sending it everywhere
	}

	client->set_connect_data(lebuf, count);

	client->connect(true);

	// set flags, show dialog...
	trying_connection = true;
	dialogmessage = "Trying to connect... ESC = cancel";
	dialogmessage2.clear();
	set_menu(menu_dialog);
}

//send player token message
void gameclient_c::send_player_token() {
	if (player_token_set) {
		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, data_registration_token);
		writeString(lebuf, count, player_token);
		client->send_message(lebuf, count);
	}
}

//issue change name command
void gameclient_c::issue_change_name_command() {
	//regular change name
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, data_name_update);
	if (playername.length() > 16)
		playername.erase(15);			//truncate player name, max 16 chars
	writeStr(lebuf, count, playername);	// the name
	client->send_message(lebuf, count);

	//name changed:
	player_token_new = true;	//getting a NEW token, not refreshing the token

	//get it (V0.4.9) -- força um "ENTER" logo apos o cara conectar ou trocar de nome ao estar
	//conectado...
	check_change_pass_command();

	//FIXME: code == 2 (?)
	if ((namestatus_code == 1) || (namestatus_code == 3))
		namestatus = "NAME CHANGED...";
	else
		namestatus_code = 0;						//take the "*" out of the name

	//v0.4.4 follow-up-message: since changing name DISABLES the registration status
	//  of the player, we need to send a new message requesting registration, if the
	//  player has a token
	//v0.4.9 : substituido pelo "enter" acima do check_...()
	//send_player_token();
}

//change name command
void gameclient_c::change_name_command() {
	//set new name, close menu
	playername = editplayername;
	if (menu != menu_none)
		set_menu(menu_main);

	//send reliable net message with the name
	if (connected)
		issue_change_name_command();
}

//send the client's frame to server (keypresses)
void gameclient_c::send_frame(bool newFrame) {
	char lebuf[256]; int count = 0;

	if (newFrame)
		++clFrameSent;
	controlHistory[clFrameSent].fromKeyboard();

	writeByte(lebuf, count, clFrameSent);
	writeByte(lebuf, count, controlHistory[clFrameSent].toNetwork(false));

	client->send_frame(lebuf, count);
}

//process incoming data
void gameclient_c::process_incoming_data(char *data, int length) {
	(void)length;

	//this is a HACK:
	int whatme = 0;

	//lock frame mutex
	//LOG("locking INCOMING\n");
	pthread_mutex_lock( &frame_mutex );
	//LOG("locked! INCOMING\n");

	// (0) update lastpackettime
	lastpackettime = get_time();

	//(1) process server frame data
	//
	int count = 0;
	NLulong svframe;	//server's frame
	readLong(data, count, svframe);

	#ifdef WATCH_CONNECTION
	if (svframe != fx.frame + 1) {
		ostringstream dstr;
		if (svframe == fx.frame)
			dstr << "@WS>C packet duplicated: " << svframe;
		else if (svframe < fx.frame)
			dstr << "@WS>C packet order: prev " << fx.frame << " this " << svframe;
		else
			dstr << "@WS>C packet lost : prev " << fx.frame << " this " << svframe;
		print_message(dstr.str().c_str());
	}
	#endif
	//discard older frames
	//overwrite always the newer frames
	// TARGET FRAME: just one
	if (svframe > fx.frame) {
		frameReceiveTime = get_time();
		int currentLag = static_cast<NLubyte>(clFrameSent - clFrameWorld);
		nAssert(currentLag < 128);
		averageLag = averageLag*.99 + currentLag*.01;

		nAssert(fx.frame == (int)fx.frame);

		ClientPhysicsCallbacks cb(*this);
		fx.rocketFrameAdvance(static_cast<int>(svframe - fx.frame), cb);
		fx.frame = svframe;

		NLulong	players_present;		//LONG players present (32 players max)
		readLong(data, count, players_present);
		for (int i = 0; i < maxplayers; i++) {
			//decode players_present: sets if "player" record is used or not, in clientside
			if (players_present & (1 << i))
				fx.player[i].used = true;
			else
				fx.player[i].used = false;
		}

		//UGLY HACK
		static NLulong old_players_present;
		if (old_players_present != players_present) {
			old_players_present = players_present;
			update_scoreboard();
		}

		//----- PLAYER SPECIFIC DATA -----

		readByte(data, count, clFrameWorld);
		#ifdef SEND_FRAMEOFFSET
		NLubyte fo;
		readByte(data, count, fo);
		float offsetDelta = (fo / 256.) - .5;	// the deviation from aim, in frames
		frameOffsetDeltaTotal += offsetDelta;
		if (++frameOffsetDeltaNum == 10) {	// try to fix deviations every 10 frames
			frameOffsetDeltaTotal /= 10.;
			if (fabs(frameOffsetDeltaTotal) > .2) {
				if (frameOffsetDeltaTotal > 0.)	// frames arrive too late to server
					++client_netsend_counter;
				else
					--client_netsend_counter;	// frames arrive too early to server
			}
			frameOffsetDeltaTotal = 0;
			frameOffsetDeltaNum = 0;
		}
		#endif

		//extra byte of information
		// BIT 0: extra health
		// BIT 1: extra energy
		// BIT 2 (****VERY IMPORTANT****): NO MORE DATA ON PACKET BECAUSE PLAYER IS NOT READY
		NLubyte xtra;
		readByte(data, count, xtra);

		//moved below: after health assignment
		//if (xtra & 1) player[me].health += 256;
		//if (xtra & 2) player[me].energy += 256;

		bool empty_frame_cause_not_ready_yet;
		if (xtra & 4)
			empty_frame_cause_not_ready_yet = true;
		else
			empty_frame_cause_not_ready_yet = false;

		//read "me" (v0.3.9 tentando espantar bug com tiro de canhao!)
		// BITS 3..8 == what player id
		whatme = 0;
		if (xtra & 8)   whatme += 1;
		if (xtra & 16)  whatme += 2;
		if (xtra & 32)  whatme += 4;
		if (xtra & 64)  whatme += 8;
		if (xtra & 128) whatme += 16;

		// v0.4.1: ISSO AQUI TAVA FALTANDO. como que tu vai ler um monte de coisa
		//    dependente do "me" sem ter ele definido??????
		//
		me = whatme;

		//EMPTY FRAME? if yes, do something about it, if not, parse it
		if (empty_frame_cause_not_ready_yet) {

			//an empty frame

			// mark somewhere that the frame (fx) should not be read/simulated?
			//					hmm...
			fx.skipped = true;

		}
		else {

			//a regular frame
			fx.skipped = false;

			//V 0.3.9 NEW : read screen of "me" player
			NLubyte  scr;
			readByte(data, count, scr);		//player.x
			fx.player[me].roomx = scr;
			readByte(data, count, scr);		//player.y
			fx.player[me].roomy = scr;

			//read "players onscreen" vector
			NLulong	players_onscreen;
			readLong(data, count, players_onscreen);

			//decode players_onscreen and update player data
			for (int i = 0; i < maxplayers; i++) {
				//decode players_onscreen: sets if "player" record is there to be read
				if (players_onscreen & (1 << i))
					fx.player[i].onscreen = true;
				else
					fx.player[i].onscreen = false;

				//if player on screen, parse the data
				if (fx.player[i].onscreen) {
					ClientPlayer &h = fx.player[i];

					//V0.3.9: took out screen reading, replacing for the same screen of "me"
					// that is set above
					h.roomx = fx.player[me].roomx;	//same screen since it's on the "players on same screen" vector
					h.roomy = fx.player[me].roomy;

					//coords & speeds
					NLubyte xy;
					//V0.3.9 : transmissao x,y de 4 bytes para 3
					//256+512+1024+2048 = 3840    last 4 bits mask
					//xy = ((hx & 3840) >> 8) + ((hy & 3840) >> 4); //x: bit 8-11 to 0-3  y: bit 8-11 to 4-7
					//writeByte(lebuf, lecount, xy);   //last 4 bits x + last 4 bits y

					readByte(data, count, xy);		//first 8 bits x
					h.lx = static_cast<double>(xy);
					readByte(data, count, xy);		//first 8 bits y
					h.ly = static_cast<double>(xy);
					readByte(data, count, xy);		//first 4 bits x + first 4 bits y
					h.lx += static_cast<double>( (xy &  15) << 8 );	//bits 0-3 to 8-11
					h.ly += static_cast<double>( (xy & 240) << 4 ); //bits 4-7 to 8-11

					//V0.3.9 speed em bytes, xinelao mesmo
					NLbyte sxy;
					readByte(data, count, sxy);	//sx
					h.sx = static_cast<double>(sxy) / 2.0;
					readByte(data, count, sxy);	//sy
					h.sy = static_cast<double>(sxy) / 2.0;

					//EXTRA BYTE
					NLubyte byt, extra;
					readByte(data, count, extra);			//extra byte

					//FLAGS BYTE
					//
					h.dead = (extra & 1) != 0;  //DEAD PLAYER = extra bit 0
					h.item_deathbringer = (extra & 2) != 0;		//deathbringer: extra bit 1
					h.deathbringer_affected = (extra & 4) != 0; //deathbringer-affected: extra bit 2
					// ITEMS: movido para este byte
					h.item_shield = (extra & 8) != 0;
					h.item_speed = (extra & 16) != 0;
					h.item_quad = (extra & 32) != 0;

					//verifica se acabou de morrer - play death sound
					if (h.dead && !h.old_dead)
						client_sounds.play(SAMPLE_DEATH + rand() % 2);
					h.old_dead = h.dead;

					NLubyte ccb;
					readByte(data, count, ccb);
					h.controls.fromNetwork(ccb, true);

					//bits 5..7 : gundir= 0..7
					h.gundir = ccb >> 5;

					//read items
					//readByte(data, count, byt);
					//player[i].item_shield =    ((byt & 1) != 0);
					//player[i].item_speed =    ((byt & 2) != 0);
					//player[i].item_quad =    ((byt & 4) != 0);

					//read helm byte
					readByte(data, count, byt);
					h.visibility = byt;
				}
			}

			//read "enemies on team vislist"
			NLushort eviz;
			readShort(data, count, eviz);
			if (me >= 0)
				fx.player[me].enemyvis = eviz;

			//read who,x,y
			NLubyte who,whox,whoy;
			readByte(data, count, who);
			readByte(data, count, whox);
			readByte(data, count, whoy);

			//update this player's px,py,x,y
			//ignore self and anybody onscreen -- because then I've got better accuracy
			if (who != me && !fx.player[who].onscreen) {
				fx.player[who].roomx = whox / (255/fx.map.w);	//screen = 0..255 / (WXMAX/255)
				fx.player[who].roomy = whoy / (255/fx.map.h);
				fx.player[who].lx = (whox % (255/fx.map.w)) * plw / (255/fx.map.w);	//posicao dentro da tela especifica
				fx.player[who].ly = (whoy % (255/fx.map.h)) * plh / (255/fx.map.h);
			}

			//read player's health and energy
			NLubyte healt, energ;
			readByte(data, count, healt);
			if (me >= 0) {
				fx.player[me].health = healt;
				//EXTRA BIT FROM WAYY ABOVE
				if (xtra & 1) fx.player[me].health += 256;
			}

			readByte(data, count, energ);
			if (me >= 0) {
				fx.player[me].energy = energ;
				//EXTRA BIT FROM WAYY ABOVE
				if (xtra & 2) fx.player[me].energy += 256;
			}

			//read ping of player frame % MAX_PLAYERS
			NLushort daping;
			readShort(data, count, daping);
			fx.player[ svframe % maxplayers ].ping = daping;
		}//frame not empty
	}

	//(2) process messages
	//
	char *msg;
	char *lebuf;
	int msglen;
	NLulong		fragz;

	//switch vars
	char *chatmsg;

	do {
		lebuf = msg = client->receive_message(&msglen);
		if (msg != 0) {

			//switch tempvars
			char mapname[128];
			NLubyte rteampower, rpx, rpy, code, pid, team, carried, abyte, rockid, iid, rpow, rdir, sx, sy;
			NLshort   rokx, roky;	//rocket hit msg 8
			NLushort	usho, hx, hy;
			NLulong frameno, prank, pscore, nscore;	//v0.4.8 NEG SCORE
			NLfloat aflo;
			char debuf[666]; debuf[0]=0;
			NLshort	ashort, rx, ry;
			int k = 0;
			int count = 0;
			//get msg code
			readByte(msg, count, code);

			LOG1("SERVER MESSAGE CODE = %i\n", code);

			//parse rest of message
			switch (static_cast<Network_data_codes>(code)) {
				// name update
				case data_name_update:
					readByte(msg, count, pid);
					readStr(msg, count, fx.player[pid].name);
					update_scoreboard();		//tentando consertar bug change teams
					break;

				//text message
				case data_text_message: {
					chatmsg = &(msg[1]);		//avoid a useless readString...
					print_message(chatmsg);		//print it to the "console"
					if (message_logging) {
						// print message to log
						// date and time
						time_t tt = time(0);
						struct tm* tmb = localtime(&tt);
						message_log << tmb->tm_year + 1900 << '-' << setfill('0') << setw(2) << tmb->tm_mon + 1
							<< '-' << setfill('0') << setw(2) << tmb->tm_mday
							<< ' ' << setw(2) << tmb->tm_hour << ':' << setfill('0') << setw(2) << tmb->tm_min << ':'
							<< setfill('0') << setw(2) << tmb->tm_sec << "  ";
						// message
						message_log << (chatmsg[0] == '@' ? chatmsg + 2 : chatmsg) << '\n';
					}

					//talk sound
					if ((strlen(chatmsg) >= 2) && (chatmsg[0] == '@') && (chatmsg[1] == 'I')) {
						//don't play talk
					}
					else
						client_sounds.play(SAMPLE_TALK);
					break;
				}

				//"hello" one-time server information ("first packet")
				case data_first_packet: {
					readByte(msg, count, pid);	//"who am I"

					//DEBUG msg
					if (pid != whatme) {
						char lixoverde[200];
						sprintf(lixoverde, "###WARNING###: me %i memsg %i whatme %i\n", me, pid, whatme);
						send_chat(lixoverde);
					}

					me = pid;

					NLchar map_nr;
					readByte(msg, count, map_nr);	//current map number
					current_map = map_nr;

					//reset want-change-teams: this message is send when players are swapped also
					want_change_teams = false;

					readByte(msg, count, abyte);	//team 0 score
					fx.teams[0].set_score(abyte);
					readByte(msg, count, abyte);	//team 1 score
					fx.teams[1].set_score(abyte);

					//server physics parameters
					readFloat(msg, count, aflo);
					svp_fric = aflo;
					readFloat(msg, count, aflo);
					svp_accel = aflo;
					readFloat(msg, count, aflo);
					svp_maxspeed = aflo;
					readFloat(msg, count, aflo);
					svp_fric_run = aflo;
					readFloat(msg, count, aflo);
					svp_accel_run = aflo;
					readFloat(msg, count, aflo);
					svp_maxspeed_run = aflo;
					readFloat(msg, count, aflo);
					svp_fric_turbo = aflo;
					readFloat(msg, count, aflo);
					svp_accel_turbo = aflo;
					readFloat(msg, count, aflo);
					svp_maxspeed_turbo = aflo;
					readFloat(msg, count, aflo);
					svp_fric_turborun = aflo;
					readFloat(msg, count, aflo);
					svp_accel_turborun = aflo;
					readFloat(msg, count, aflo);
					svp_maxspeed_turborun = aflo;
					readFloat(msg, count, aflo);
					svp_flag_penalty = aflo;
					readByte(msg, count, abyte);
					svp_friendly_fire = abyte & 0x01;
					svp_friendly_db = abyte & 0x02;

					// room is probably changed
					fx.player[me].oldx = -1;
					fx.player[me].oldy = -1;

					//update scoreboard!
					update_scoreboard();

					break;
				}

				//frags update
				case data_frags_update:
					readByte(msg, count, pid);	// what player
					readLong(msg, count, fragz);	// new frag value
					fx.player[pid].frags = fragz;
					update_scoreboard();		// update clientside scoreboard
					break;

				//CTF flag update
				case data_flag_update: {
					NLbyte flags;
					readByte(msg, count, team);		// team of the flag
					readByte(msg, count, flags);	// how many flags
					LOG1("Flag message, %d flags.\n", flags);
					for (int i = 0; i < flags; i++) {
						if (i >= static_cast<int>(fx.teams[team].flags().size()))
							fx.teams[team].add_flag(spoint_t());
						readByte(msg, count, carried);	// 0==not carried 1==carried
						if (carried == 0) {
							//not carried: update position
							readByte(msg, count, abyte);		//px
							const int px = abyte;
							readByte(msg, count, abyte);		//py
							const int py = abyte;
							readShort(msg, count, ashort);		//x
							const int x = ashort;
							readShort(msg, count, ashort);		//y
							const int y = ashort;
							fx.teams[team].drop_flag(i, spoint_t(px, py, x, y));
						}
						else {
							//carried: get carrier
							readByte(msg, count, abyte);	//carrier
							fx.teams[team].steal_flag(i, abyte);
							client_sounds.play(SAMPLE_CTF_GOT);
						}
					}
					break;
				}

				//rocket fire notification
				case data_rocket_fire: {
					// add to clientside rocket objects list
					//
					//readByte(lebuf, count, rpowdir);	// rocket powerdir
					readByte(lebuf, count, rpow);	// rocket powerdir
					readByte(lebuf, count, rdir);	// rocket powerdir

					NLubyte rids[16];
					for (k=0;k<rpow;k++)
						readByte(lebuf, count, rids[k]);

					readLong(lebuf, count, frameno);	// frame # of shot
					readByte(lebuf, count, rteampower);	// team (bit 1) and power (bit 0)

					bool power = ((rteampower & 1) != 0);
					int team = (rteampower & 2) >> 1;
					readByte(lebuf, count, rpx);
					readByte(lebuf, count, rpy);
					readShort(lebuf, count, rx);
					readShort(lebuf, count, ry);

					ClientPhysicsCallbacks cb(*this);
					fx.shootRockets(cb, 0, rpow, rdir, rids, static_cast<int>(fx.frame-frameno), team, power, rpx, rpy, rx, ry);

					//play sound if rocket on screen
					if (me >= 0 && rpx == fx.player[me].roomx && rpy == fx.player[me].roomy)
						if (power)
							client_sounds.play(SAMPLE_QUAD_FIRE);
						else
							client_sounds.play(SAMPLE_FIRE);
					break;
				}

				//rocket deletion notification
				case data_rocket_delete:
					readByte(lebuf, count, rockid);	// rocket object id
					readByte(lebuf, count, abyte);	// target player
					//hit position
					readShort(lebuf, count, rokx);
					readShort(lebuf, count, roky);
					fx.rock[rockid].owner = -1;
					if (abyte != 255) {	// hit player
						if (abyte < 250)	// blink player if not hit shield (252)
							fx.player[abyte].hitfx = get_time() + .3;
						client_graphics.create_gunexplo((int)rokx, (int)roky, fx.rock[rockid].px, fx.rock[rockid].py);
						client_sounds.play(SAMPLE_HIT);
					}
					break;

				//CTF team score update
				case data_score_update: {
					NLubyte score;
					readByte(lebuf, count, abyte);		//team
					readByte(lebuf, count, score);		//new score
					fx.teams[abyte].set_score(score);	// update the score
					break;
				}

				//sound event
				case data_sound:
					readByte(lebuf, count, abyte);		// sample #
					client_sounds.play(abyte);
					break;

				//pickup visible
				case data_pup_visible:
					//print_message("POWERUP_VISIBLE!!!");
					readByte(lebuf, count, iid);		// item id
					readByte(lebuf, count, abyte);		// kind
					fx.item[iid].kind = abyte;
					readByte(lebuf, count, abyte);		// screen x
					fx.item[iid].px = abyte;
					readByte(lebuf, count, abyte);		// screen y
					fx.item[iid].py = abyte;
					readShort(lebuf, count, usho);		// pos x
					fx.item[iid].x = usho;
					readShort(lebuf, count, usho);		// pos y
					fx.item[iid].y = usho;
					break;

				//pickup picked
				case data_pup_picked:
					readByte(lebuf, count, iid);
					fx.item[iid].kind = 0;		// no more!
					break;

				//powerup clientside timer set
				case data_pup_timer:
					readByte(lebuf, count, iid);	//kind
					readShort(lebuf, count, usho);	//amount of time
					if (me >= 0) {
						if (iid == 2)
							fx.player[me].item_speed_time = get_time() + ((double)usho);
						else if (iid == 3)
							fx.player[me].item_helm_time = get_time() + ((double)usho);
						else if (iid == 4)
							fx.player[me].item_quad_time = get_time() + ((double)usho);
					}
					break;

				//my weapon notify change
				case data_weapon_change:
					readByte(lebuf, count, abyte);	// weapon level
					if (me >= 0) {
						fx.player[me].weapon = abyte;
					}
					break;

				//server commands client to change map
				case data_map_change:
					map_ready = false;	// map NOT ready anymore: must load/change
					want_map_exit = false;		// and player does not want to exit the map anymore
					fx.teams[0].remove_flags();
					fx.teams[1].remove_flags();
					readByte(lebuf, count, abyte);			// read map kind (1=builtin 2=custom)
					if (abyte == 2) {
						readShort(lebuf, count, usho);				//read CRC16 of map
						readString(lebuf, count, mapname);		//read map name
						server_map_command(mapname, usho);
						NLchar map_nr;
						readByte(lebuf, count, map_nr);
						current_map = map_nr;
						if (map_vote == current_map)
							map_vote = -1;
					}
					else {
						//FIXME: unknown map kind
					}
					for (int iid = 0; iid < MAX_PICKUPS; ++iid)
						fx.item[iid].kind = 0;
					fx.player[me].oldx = -1;
					fx.player[me].oldy = -1;
					break;

				//server shows gameover plaque
				case data_gameover_show:
					readByte(lebuf, count, abyte);
					gameover_plaque = abyte;		// kind of plaque (capture limit or vote exit)
					if (gameover_plaque == NEXTMAP_CAPTURE_LIMIT || gameover_plaque == NEXTMAP_VOTE_EXIT) {
						readByte(lebuf, count, abyte);	//RED team final score
						red_final_score = abyte;
						readByte(lebuf, count, abyte);  //BLUE team final score
						blue_final_score = abyte;
					}
					else
						gameover_plaque = NEXTMAP_NONE;
					break;

				//server hides gameover plaque
				case data_gameover_hide:
					gameover_plaque = NEXTMAP_NONE;		//hide
					break;

				//deathbringer shot
				case data_deathbringer:
					//print_message("DEATHBRINGER!!!");
					readByte(lebuf, count, abyte);	//what player
					readLong(lebuf, count, frameno);		// start time
					//spawn clientside fx at the owner's position
					//V0.4.6: sending explicitly the screen/coord of the shot
					readByte(lebuf, count, sx);
					readByte(lebuf, count, sy);
					readShort(lebuf, count, hx);
					readShort(lebuf, count, hy);
					client_graphics.create_deathbringer(abyte, get_time() + (fx.frame - frameno) * 0.1, hx, hy, sx, sy);
					client_sounds.play(SAMPLE_USEDEATHBRINGER);

					//cfx_create_deathbringer(abyte, get_time() + (fx.frame - frameno) * 0.1, (int)fx.player[abyte].x, (int)fx.player[abyte].y, player[abyte].x, player[abyte].y);
					//if (player[abyte].x == player[me].x)
					//if (player[abyte].y == player[me].y)
					//print_message("DEATHBRINGER ON MY SCREEENN!!!");
					/*
					writeByte(lebuf, count, 26);	//26==deathbringer
					writeByte(lebuf, count, ((NLubyte)target));	//team/target player
					//LIE: x,y can be taken from the player since it's rock-dead on the bringer's position
					//v0.4.6: somehow the graphical effect is playing on the wrong screen of the clients
					//  trying sending screen x,y and player x y
					writeByte(lebuf, count, ((NLubyte)player[target].x));
					writeByte(lebuf, count, ((NLubyte)player[target].y));
					writeShort(lebuf, count, ((NLushort)world.player[target].x));
					writeShort(lebuf, count, ((NLushort)world.player[target].y));
					writeLong(lebuf, count, frame);		//frame # of the bringer shot (message can be delayed)
					*/
					//sprintf(debuf, "t %i sx %i sy %i x %i y %i
					//print_message
					break;

				//v0.4.4: UDP FILE DOWNLOAD: incoming chunk
				case data_file_download:
					readByte(lebuf, count, abyte);		//"last chunk"?
					readLong(lebuf, count, frameno);	//absolute pos of the chunk on file
					readShort(lebuf, count, usho);		//chunk size
					//PROCESS IT
					process_udp_download_chunk(abyte, frameno, usho, &(lebuf[count]));
					break;

				//v0.4.4: registration response from server
				case data_registration_response:
					readByte(lebuf, count, abyte);
					if (abyte == 1) {
						//success!
						namestatus = "RECORDING (";
						namestatus += player_token;
						namestatus += ")";
						//code
						namestatus_code = 3;
					}
					else {
						//fail
						namestatus = "REJECTED (";
						namestatus += player_token;
						namestatus += ")";
					}

					break;

				//v0.4.5: CRAPZ UPDATE message -- updates lots of crap about a player
				case data_crap_update:
					readByte(lebuf, count, pid);			//waht player slot
					readByte(lebuf, count, abyte);		//reg char
					readLong(lebuf, count, prank);		//ranking#
					readLong(lebuf, count, pscore);		//score
					readLong(lebuf, count, nscore);		//score	NEG v0.4.8
					readLong(lebuf, count, max_world_rank);		//world players count
					readLong(lebuf, count, max_world_score);		//world score max
					fx.player[pid].reg_status = (char)abyte;
					fx.player[pid].rank = (int)prank;
					fx.player[pid].score = (int)pscore;
					fx.player[pid].neg_score = (int)nscore;
					//LOG4("CRAPZ UPDATE %i %c %i %i\n", pid, abyte, prank, pscore);
					break;

				// map time left
				case data_map_time: {
					int time_left;
					readLong(lebuf, count, time_left);
					map_end_time = (int)get_time() + time_left;
					map_time_limit = true;
					LOG("Map time left received.\n");
					break;
				}

				// server map list
				case data_map_list: {
					NLchar width, height, votes;
					gameserver_c::MapInfo mapinfo;
					readStr(lebuf, count, mapinfo.title);
					readStr(lebuf, count, mapinfo.author);
					readByte(lebuf, count, width);
					readByte(lebuf, count, height);
					readByte(lebuf, count, votes);
					mapinfo.width = width;
					mapinfo.height = height;
					mapinfo.votes = votes;
					maps.push_back(mapinfo);
					break;
				}

				case data_map_votes_update: {
					NLchar total, map_nr, votes;
					readByte(lebuf, count, total);
					for (int i = 0; i < total; i++) {
						readByte(lebuf, count, map_nr);
						readByte(lebuf, count, votes);
						if (map_nr >= 0 && map_nr < static_cast<int>(maps.size()))
							maps[map_nr].votes = votes;
					}
					break;
				}

				case data_capture: {
					NLchar pid;
					readByte(lebuf, count, pid);
					fx.player[pid].stats().add_capture();
					break;
				}

				case data_kill: {
					NLchar attacker, target;
					readByte(lebuf, count, attacker);
					readByte(lebuf, count, target);
					const bool deathbringer = attacker & 0x80;
					attacker &= ~0x80;
					const bool flag = target & 0x80;
					target &= ~0x80;
					fx.player[attacker].stats().add_kill(deathbringer);
					fx.teams[attacker / TSIZE].add_kill();
					fx.player[target].stats().add_death(deathbringer, static_cast<int>(get_time()));
					fx.teams[target / TSIZE].add_death();
					if (flag) {
						fx.player[attacker].stats().add_carrier_kill();
						fx.player[target].stats().add_flag_drop();
						fx.teams[target / TSIZE].add_flag_drop();
					}
					break;
				}

				case data_flag_take: {
					NLchar pid;
					readByte(lebuf, count, pid);
					fx.player[pid].stats().add_flag_take();
					fx.teams[pid / TSIZE].add_flag_take();
					break;
				}

				case data_flag_return: {
					NLchar pid;
					readByte(lebuf, count, pid);
					fx.player[pid].stats().add_flag_return();
					fx.teams[pid / TSIZE].add_flag_return();
					break;
				}

				case data_flag_drop: {
					NLchar pid;
					readByte(lebuf, count, pid);
					fx.player[pid].stats().add_flag_drop();
					fx.teams[pid / TSIZE].add_flag_drop();
					break;
				}

				case data_suicide: {
					NLchar pid;
					readByte(lebuf, count, pid);
					const bool flag = pid & 0x80;
					pid &= ~0x80;
					fx.player[pid].stats().add_suicide(static_cast<int>(get_time()));
					fx.teams[pid / TSIZE].add_suicide();
					if (flag) {
						fx.player[pid].stats().add_flag_drop();
						fx.teams[pid / TSIZE].add_flag_drop();
					}
					break;
				}

				case data_spawn: {
					NLchar pid;
					readByte(lebuf, count, pid);
					fx.player[pid].stats().set_spawn_time(static_cast<int>(get_time()));
					break;
				}

				case data_movements_shots: {
					for (int i = 0; i < MAX_PLAYERS; i++) {
						if (!fx.player[i].used)
							continue;
						NLlong movement;
						readLong(lebuf, count, movement);
						fx.player[i].stats().set_movement(movement);
						NLshort data;
						readShort(lebuf, count, data);
						fx.player[i].stats().set_shots(data);
						readShort(lebuf, count, data);
						fx.player[i].stats().set_hits(data);
						readShort(lebuf, count, data);
						fx.player[i].stats().set_shots_taken(data);
					}
					break;
				}

				case data_stats: {
					for (int i = 0; i < MAX_PLAYERS; i++) {
						if (!fx.player[i].used)
							continue;
						NLubyte data;
						readByte(lebuf, count, data);
						fx.player[i].stats().set_kills(data);
						readByte(lebuf, count, data);
						fx.player[i].stats().set_deaths(data);
						readByte(lebuf, count, data);
						fx.player[i].stats().set_cons_kills(data);
						readByte(lebuf, count, data);
						fx.player[i].stats().set_current_cons_kills(data);
						readByte(lebuf, count, data);
						fx.player[i].stats().set_cons_deaths(data);
						readByte(lebuf, count, data);
						fx.player[i].stats().set_current_cons_deaths(data);
						readByte(lebuf, count, data);
						fx.player[i].stats().set_suicides(data);
						readByte(lebuf, count, data);
						fx.player[i].stats().set_captures(data);
						readByte(lebuf, count, data);
						fx.player[i].stats().set_flags_taken(data);
						readByte(lebuf, count, data);
						fx.player[i].stats().set_flags_dropped(data);
						readByte(lebuf, count, data);
						fx.player[i].stats().set_flags_returned(data);
						readByte(lebuf, count, data);
						fx.player[i].stats().set_carriers_killed(data);
					}
					break;
				}

				default:
					LOG1("BAD ERROR: UNKNOWN SERVER MESSAGE CODE = %i!!\n", code);
					break;
			}
		}
	} while (msg != 0);

	// (3) calc stuff dependant on received data
	//

	//detect screen changes / clear powerup fx's
	if (me >= 0 && (fx.player[me].roomx != fx.player[me].oldx) || (fx.player[me].roomy != fx.player[me].oldy)) {
		//screen changed.

		//V0.4.7: now, why would I want to do that??
		//clear all clientside fx's
		//for (int f=0;f<MAX_CLIENTFX;f++)
		//	cfx[f].used = false;

		for (int j=0;j<MAX_PICKUPS;j++)
			if (fx.item[j].px==fx.player[me].oldx && fx.item[j].py==fx.player[me].oldy)
				fx.item[j].kind = 0;	// erase

		//set new old's
		fx.player[me].oldx = fx.player[me].roomx;
		fx.player[me].oldy = fx.player[me].roomy;

		// predraw new room
		predraw();
	}

	//this is a HACK:
	me = whatme;

	//unlock frame mutex
	//LOG("unlocking INCOMING\n");
	pthread_mutex_unlock( &frame_mutex );
	//LOG("unlocked! INCOMING\n");
}

//send chat message
void gameclient_c::send_chat(const string& msg) {
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, data_text_message);
	writeStr(lebuf, count, msg);
	client->send_message(lebuf, count);
}

void gameclient_c::erase_first_message() {
	if (!chatbuffer.empty())
		chatbuffer.pop_front();
	chaterasetime = get_time() + 10.0;
}

//print message to "console"
void gameclient_c::print_message(const string& msg) {
	if (chatbuffer.size() == chat_size)
		chatbuffer.pop_front();
	chatbuffer.push_back(msg);
	chaterasetime = get_time() + 10.0;
}

void gameclient_c::save_screenshot() {
	// FIXME: make screenshots possible everywhere in the game
	string filename;
	for (int i = 0; i < 1000; i++) {
		// filename: screens/outgxxx.pcx
		ostringstream fname;
		fname << "screens" << directory_separator;
		fname << "outg" << setfill('0') << setw(3) << i << ".pcx";
		if (!file_exists(fname.str().c_str(), FA_ARCH | FA_DIREC | FA_HIDDEN | FA_RDONLY | FA_SYSTEM, 0)) {
			filename = fname.str();
			break;
		}
	}

	ostringstream message;
	if (client_graphics.save_screenshot(filename))
		message << "Saved screenshot to " << filename << '.';
	else
		message << "Could not save screenshot to " << filename << '.';
	print_message(message.str().c_str());
}

//toggle help screen
void gameclient_c::toggle_help() {
	helpshow = !helpshow;

	if (helpshow)
		client_sounds.play(SAMPLE_WEAPON_UP);
	else
		client_sounds.play(SAMPLE_FIRE);
}

//show progress / press any key / dialog
void gameclient_c::show_dialog(char *t1, char *t2, char *t3, int fg, int bg) {
	clear_keybuf();
	do {
		client_graphics.show_progress(t1, t2, t3, fg, bg);
		if (keypressed())
			break;
		MS_SLEEP(50);
	} while (1);
	//wait until ESC released
	while (key[KEY_ESC]) ;
	clear_keybuf();
}

//GET SERVERS FROM MASTER!!!
void gameclient_c::get_servers_from_master() {
	NLsocket sock;

	//open a nonblocking socket
	nlDisable(NL_BLOCKING_IO);
	sock = nlOpen(0, NL_RELIABLE);
	if (sock == NL_INVALID) {
		//show "cant open socket to master" error
		show_dialog("ERROR", "Can't open socket!", "Press any key.", 0,makecol(0xff,0xaa,0xaa));
		return;
	}

	//connect the nonblocking way
	nlConnect(sock, &master_address);

	//build query
	char querybuf[1024]; int qcount = 0;
	writeString(querybuf, qcount, "GET /servlet/fcecin.m3/index.html?get=x\n\n");
	qcount--;	//take the zero out

	client_graphics.show_progress("Getting updated internet server list", "Contacting server...", "Press ESC to cancel");

	//keep trying to write the query until user presses ESC
	NLint result;
	do {
		result = nlWrite(sock, querybuf, qcount);
		MS_SLEEP(10);
		if (key[KEY_ESC]) {
			// 'attempt cancelled'
			nlClose(sock);
			clear_keybuf(); //clear keystrokes buffer
			while (key[KEY_ESC]); //wait to release esc
			return;
		}
	} while ((result == NL_INVALID) && (nlGetError() == NL_CON_PENDING));

	//check bogus
	if (result == NL_INVALID) {
		nlClose(sock);
		// show 'some problem occured try later'
		show_dialog("Problem connecting to master server (1).", "Try again later.", "Press any key.", 0, makecol(0xff,0x88,0x88));
		return;
	}

	client_graphics.show_progress("Getting updated internet server list", "Waiting response...", "Press ESC to cancel");

	//log ok
	LOG3("QUERY TO MASTER '%s', result = %i, count = %i\n", querybuf, result, qcount);

	client_graphics.show_progress("Getting updated internet server list", "Request sent. Waiting a reply...", "Press ESC to cancel");

	//try to read the reply or until user presses ESC
	//parse the response (should be <HTML><BODY> etc... with "@I @I @I ... @K" on it
	bool html_end = false;
	int nostuffcound = 0;
	char lebuf[65536];
	int n = 0;
	do {

		//read
		result = nlRead(sock, &(lebuf[n]), 1);

		//no byte
		if (result == 0) {

			if (nostuffcound > 0) {
				nostuffcound++;

				if (html_end) {
					if (nostuffcound > 200) {		//2 seconds after it came some stuff but now without coming more stuff
						lebuf[n+1] = 0;
						LOG("2 SEC TIMEOUT READING STUFF AFTER </HTML>");
						LOG1("---- Full response START ----\n%s\n---- Full response END ----\n", lebuf);
						break;
					}
				}
				//did not receive end yet -- then wait...
				else {
				}
			}

			MS_SLEEP(10);
			if (key[KEY_ESC]) {
				// 'attempt cancelled'
				nlClose(sock);
				clear_keybuf(); //clear keystrokes buffer
				while (key[KEY_ESC]); //wait to release esc
				return;
			}
		}

		//error occured
		if (result == NL_INVALID) {

			//if already got html_end, no error
			if (html_end)
				break;

			LOG1("MASTER CLIENT QUERY ERROR READING RESPONSE result = %i\n", result);
			//show 'some problem occured try later'
			/*
			sprintf(dialogmessage, "Problem connecting! Try later. (2)");//%i %s %i %s", nlGetError(), nlGetErrorStr(nlGetError()), nlGetSystemError(), nlGetSystemErrorStr(nlGetSystemError()));
			strcpy(dialogmessage2, "Press ESC");
			set_menu(2);	//dialog menu
			*/
			nlClose(sock);
			show_dialog("Problem connecting to master server (2).", "Try again later.", "Press any key.", 0, makecol(0xff,0x88,0x88));
			return;
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
				LOG1("CLIENT MASTER QUERY RECEIVED </HTML>! SUCCESS!! n=%i\n", n);
				html_end = true;
				nostuffcound = 1;
				lebuf[n+1] = 0;
				LOG1("---- Full response START ----\n%s\n---- Full response END ----\n", lebuf);
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
				html_end = false;
				n = -1;
			}
		}

		//read next
		n++;

	} while (1);

	//clear the old gamespy master screen
	//
	for (int j = 0; j < MAX_GAMESPY; j++) {
		mgamespy[j].address[0] = 0;
		mgamespy[j].refreshed = false;
		mgamespy[j].invalid = false;	//don't know the status yet
		mgamespy[j].favs = false;
	}

	//parse the successful response into the gamespy screen
	//
	int c = 0;
	int m = 0;		//gamespy entry
	bool found_k = false;
	bool found_i = false;
	do {

		// check command char
		if (lebuf[c++] == '@') {

			//check IP char
			if (lebuf[c] == 'I') {

				found_i = true; //found an @I

				//point to first char of IP
				c++;

				//parse IP into a buf
				char ipbuf[30];
				int  ic = 0;
				do {

					//copy one
					ipbuf[ic++] = lebuf[c++];

				} while (
					//V0.4.2: ":" para port number
					(lebuf[c] == '1') || (lebuf[c] == '4') || (lebuf[c] == '7') || (lebuf[c] == '0') ||
					(lebuf[c] == '2') || (lebuf[c] == '5') || (lebuf[c] == '8') || (lebuf[c] == '.') ||
					(lebuf[c] == '3') || (lebuf[c] == '6') || (lebuf[c] == '9') || (lebuf[c] == ':')
				);

				//zero terminate
				ipbuf[ic] = 0;

				//copy if enough room
				if (m < MAX_GAMESPY) {
					strcpy(mgamespy[m].address, ipbuf);
					m++;	//next entry
				}
			}
			//check DONE char
			else if (lebuf[c] == 'K') {
				//done!
				found_k = true;
				break;
			}
		}

	} while (lebuf[c] != 0);

	//copy addresses from favourites to holes in master entries, anyway
	//if (!found_k) {
	// for (int h=0;h<MAX_GAMESPY;h++)
	// strcpy(mgamespy[h].address, gamespy[h].address);
	//}
	int f = 0;	//favorites entry
	int minf = m;		//first favorites entry
	while ((m < MAX_GAMESPY) && (f < MAX_GAMESPY)) {		//slot in master list
		if (gamespy[f].address[0] != '\0') {

			//scan for duplicate: then ignore
			bool dup = false;
			for (int i = 0; i < MAX_GAMESPY; i++)
				if (!strcmp(gamespy[f].address, mgamespy[i].address)) {
					dup = true;		//dup
					break;
				}

			//no dup? add
			if (!dup) {
				strcpy(mgamespy[m].address, gamespy[f].address);
				mgamespy[m].favs = true;
				m++;	//next m slot
			}
		}
		f++;	//next f anyways
	}

	//refresh (has own progress dialog)
	//
	// OBS.: will refresh even if master server fails -- refreshes favourites
	//
	refresh_command();

	//remove all invalid IPs
	//
	int e;
	for (e=0;e<MAX_GAMESPY;e++)
		if (mgamespy[e].invalid)	//erase address
			mgamespy[e].address[0]=0;

	//remove all "no response"s below minf
	//
	for (e=minf;e<MAX_GAMESPY;e++)
		if (mgamespy[e].noresponse)	{ //erase address
			mgamespy[e].address[0]=0;
			mgamespy[e].invalid = true;
		}

	//compress entries
	//
	gamespy_t  temp[MAX_GAMESPY];
	memcpy(temp, mgamespy, sizeof(gamespy_t)*MAX_GAMESPY);	//copy to temp
	for (e=0;e<MAX_GAMESPY;e++) {
		mgamespy[e].address[0]=0;	//erase all master
		mgamespy[e].favs=false;
		//0.4.1:
		mgamespy[e].invalid = true;		//address is invalid
	}
	int next=0;	//master entry next
	for (e=0;e<MAX_GAMESPY;e++)
		if (temp[e].address[0] != 0) {
			memcpy(&(mgamespy[next]), &(temp[e]), sizeof(gamespy_t));	//copy back to master
			next++;
		}

	//show an error dialog if no @K in message
	if (!found_k) {
		if (found_i)
			show_dialog("ERROR: corrupted response.", "Try again later.", "Press any key.", 0, makecol(0xff,0x88,0x88));
		else
			show_dialog("ERROR: service unavailable", "Try again in a minute.", "Press any key.", 0, makecol(0xff,0x88,0x88));
	}

	//close socket
	//
	nlClose(sock);
}

//loop
void gameclient_c::loop() {

	bool notquit = true;

	//show menu and not game yet
	set_menu(menu_main);
	gameshow = false;

	//reset speed counter
	speed_counter = 0;
	client_netsend_counter = 0;

	//loop
	bool quick_fix, key_fire = false, key_kill = false, key_swap = false, key_votexit = false, key_drop_flag = false;
	char key_up=0, key_down=0, key_left=0, key_right=0;
	int i;
	while (notquit && !force_exit) {
		//LOG("** another loop()...\n");

		// (-1) try to release "reverse" voices that have finished playing
		//
		// versao 0.4.1 : EARLY OPTIMIZATION IS THE ROOT OF ALL EVIL
		// versao 0.4.1 : EARLY OPTIMIZATION IS THE ROOT OF ALL EVIL
		// versao 0.4.1 : EARLY OPTIMIZATION IS THE ROOT OF ALL EVIL
		/*
		for (int voi=0;voi<16;voi++)		//FIXME: just guessing at 16 voices...
			if (voice_check(voi) != 0)
				if (voice_get_position(voi) == -1) {
					//char vois[200];
					//sprintf(vois, "voice %i finished", voi);
					//print_message(vois);
					release_voice(voi);
				}
		*/

		// (0) check for time to send delayed messages
		//

		// (1) loop doing input/sleep before next simulation/draw time
		//
		quick_fix = true;
		do {

			//quit key
			if ((key[KEY_LCONTROL]) || (key[KEY_RCONTROL]))
			if (key[KEY_F12]) {
				notquit = false;
				break;
			}

			//help showing
			if (helpshow) {
				while (keypressed()) {
					//get key
					int ch = readkey();
					int sc = ch >> 8;	//scancode
					ch = ch & 0xff;			//char

					//toggle help
					if (sc == KEY_F1)
						toggle_help();
					//screenshot
					else if (sc == KEY_F11)
						screenshot = true;
				}
			}
			//menu keypresses (from char buf) - ESC already dealed with, ignore
			else if (menu != menu_none) {
				while (keypressed()) {
					string lerud_abloxon;

					//get key
					int ch = readkey();
					int sc = ch >> 8;	//scancode
					ch = ch & 0xff;			//char

					//screenshot
					if (sc == KEY_F11)
						screenshot = true;

					//toggle help
					if (sc == KEY_F1)
						toggle_help();
					else if (menu == menu_maps || menu == menu_players || menu == menu_teams) {
						if (sc == KEY_F2)
							set_menu(menu == menu_maps ? menu_none : menu_maps);
						else if (sc == KEY_TAB)
							set_menu(menu == menu_players ? menu_none : menu_players);
						else if (sc == KEY_F5)
							set_menu(menu == menu_teams ? menu_none : menu_teams);
					}

					//test key
					switch (menu) {
						//main menu
						case menu_main:
							if (key[KEY_SPACE] && sc == KEY_F8) {
								port++;
								if (port > DEFAULT_UDP_PORT + 5)
									port = DEFAULT_UDP_PORT;
							}
							else if (sc == KEY_F10) {
								editplayername = RandomName();
								change_name_command();
							}
							switch (ch) {
								case '1': set_menu(menu_server_list); break;
								case '2': disconnect_command(); break;
								case '3':
									editplayername = playername;
									editplayerpass = player_password;
									name_selected = true;
									set_menu(menu_name_password);
									break;
								case '4': // start/stop listenserver
									if (listen_server_running)
										listen_stop();
									else
										listen_start();
									break;
								case '5':
									winclient = !winclient;
									client_graphics.reset_video_mode();
									client_graphics.update_minimap_background(fx.map);
									predraw();
									break;
								case '6': client_sounds.next_theme(); break;
								case '7':
									client_graphics.next_theme();
									predraw();
									break;
								default:;
							}
							break;
						//connect screen
						case menu_server_list:
							if (showmaster)
								i = strlen(mgamespy[gi].address);
							else
								i = strlen(gamespy[gi].address);
							if (
										//v0.4.2: including +6 chars for :xxxxx (port)
										(i < 21)
										//(i < 16)	// max length of IP address typein
										&&
										//v0.4.2 ":" para port#
										(((ch >= '0') && (ch <= '9')) || (ch == '.') || (ch == ':'))
							) {
								if (showmaster) {
									mgamespy[gi].address[i] = (char)ch;
									mgamespy[gi].address[i+1] = 0;
									mgamespy[gi].refreshed = false;
								}
								else {
									gamespy[gi].address[i] = (char)ch;
									gamespy[gi].address[i+1] = 0;
									gamespy[gi].refreshed = false;
								}
							}
							else if (sc == KEY_UP) {
								gi--;
								if (gi < 0)
									gi = MAX_GAMESPY-1;
							}
							else if (sc == KEY_DOWN) {
								gi++;
								if (gi >= MAX_GAMESPY)
									gi = 0;
							}
							else if (sc == KEY_SPACE) {
								refresh_command();
								clear_keybuf();	//clear the key buffer to stop too much refreshing.
							}
							else if (sc == KEY_BACKSPACE) {
								if (i > 0) {
									if (showmaster) {
										mgamespy[gi].address[i-1] = 0;
										mgamespy[gi].refreshed = false;
									}
									else {
										gamespy[gi].address[i-1] = 0;
										gamespy[gi].refreshed = false;
									}
								}
							}
							else if (sc == KEY_ENTER || sc == KEY_ENTER_PAD) {
								connect_command();
							}
							else if (sc == KEY_TAB) {
								showmaster = !showmaster;

								//make the first refresh of favorites when showing it
								if ((!showmaster) && (first_fav_refresh == false)) {
									first_fav_refresh = true;
									refresh_command();
								}
							}
							else if ((sc == KEY_F2) && (showmaster)) {
								//update from master
								get_servers_from_master();
							}
							break;
						//dialog screen : just ESC
						case menu_dialog:
							break;
						//change name screen
						case menu_name_password:
							if (name_selected)
								i = editplayername.length();
							else
								i = editplayerpass.length();

							if (((ch >= '0') && (ch <= '9')) || ((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z')) || (ch == '-') || (ch == '_')) {

								if (name_selected) {
									if (i < 15) {
										editplayername += static_cast<char>(ch);
										editplayerpass = ""; //reset password after editing name
									}
								}
								else {
									if (i < 8)
										editplayerpass += static_cast<char>(ch);
								}
							}
							else if (sc == KEY_BACKSPACE) {
								if (i > 0) {
									if (name_selected) {
										editplayername.erase(editplayername.end() - 1);
										editplayerpass = ""; //reset password after editing name
									}
									else
										editplayerpass.erase(editplayerpass.end() - 1);
								}
							}
							else if (sc == KEY_ENTER || sc == KEY_ENTER_PAD) {
								change_name_command();
								//get it (V0.4.9) -- força um "ENTER" logo apos o cara conectar ou trocar de nome ao estar
								//conectado...
								check_change_pass_command();
							}
							else if (sc == KEY_TAB)		// switch fields
								name_selected = !name_selected;
							break;
						// server password requesting dialog
						case menu_server_password:
							if (sc == KEY_BACKSPACE && !edit_server_password.empty())
								edit_server_password.erase(edit_server_password.end() - 1);
							else if ((sc == KEY_ENTER || sc == KEY_ENTER_PAD) && !edit_server_password.empty())
								connect_command();
							else if (ch >= 32)
								edit_server_password += static_cast<char>(ch);
							break;
						case menu_maps:
							if (key[KEY_UP])
								client_graphics.map_list_prev();
							if (key[KEY_DOWN])
								client_graphics.map_list_next();
							if (isdigit(ch) && edit_map_vote.size() < 3)
								edit_map_vote += ch;
							else if (sc == KEY_BACKSPACE) {
								if (!edit_map_vote.empty())
									edit_map_vote.erase(edit_map_vote.end() - 1);
							}
							else if (sc == KEY_ENTER || sc == KEY_ENTER_PAD) {
								int new_vote = atoi(edit_map_vote.c_str()) - 1;
								edit_map_vote = "";
								if (new_vote != map_vote && (new_vote >= 0 && new_vote < static_cast<int>(maps.size()) ||
												map_vote >= 0 && map_vote < static_cast<int>(maps.size()))) {
									map_vote = new_vote;

									// send map vote
									char lebuf[16];
									int count = 0;
									writeByte(lebuf, count, data_map_vote);
									writeByte(lebuf, count, map_vote);
									client->send_message(lebuf, count);
								}
							}
							break;
						case menu_players:
							if (key[KEY_UP])
								player_stats_page = max(0, player_stats_page - 1);
							if (key[KEY_DOWN])
								player_stats_page = min(3, player_stats_page + 1);
							break;
						default:;
					}
				}
			}
			// menu not showing: send keypresses to the game
			else {
				bool sendnow = false;

				// ctrl == fire event
				if (key[KEY_LCONTROL] || key[KEY_RCONTROL]) {
					if (!key_fire) {
						key_fire = true;

						//"fire" message (+ATTACK)
						char lebuf[16]; int count = 0;
						writeByte(lebuf, count, data_fire_on);
						client->send_message(lebuf, count);

						//send early keys packet
						sendnow = true;
					}
				}
				else {
					if (key_fire) {
						key_fire = false;

						//"un-fire" message (-ATTACK)
						char lebuf[16]; int count = 0;
						writeByte(lebuf, count, data_fire_off);
						client->send_message(lebuf, count);

						//send early keys packet
						sendnow = true;
					}
				}

				// del == suicide event
				if (key[KEY_DEL]) {
					if (!key_kill) {
						key_kill = true;

						//"suicide" message
						char lebuf[16]; int count = 0;
						writeByte(lebuf, count, data_suicide);
						client->send_message(lebuf, count);
					}
				}
				else key_kill = false;

				// page down == drop flag
				if (key[KEY_PGDN]) {
					if (!key_drop_flag) {
						key_drop_flag = true;
						char lebuf[16]; int count = 0;
						writeByte(lebuf, count, data_drop_flag);
						client->send_message(lebuf, count);
					}
				}
				else if (key_drop_flag) {
					key_drop_flag = false;
					char lebuf[16]; int count = 0;
					writeByte(lebuf, count, data_stop_drop_flag);
					client->send_message(lebuf, count);
				}

				// end == want/don't want to change team
				if (key[KEY_END]) {
					if (!key_swap) {
						key_swap = true;

						//toggle my local option
						want_change_teams = !want_change_teams;

						//want to swap/dont want  message
						char lebuf[16]; int count = 0;
						if (want_change_teams)
							writeByte(lebuf, count, data_change_team_on);
						else
							writeByte(lebuf, count, data_change_team_off);
						client->send_message(lebuf, count);
					}
				}
				else key_swap = false;

				// l,r,u,d,fire game keys
				if ((key[KEY_UP]    != key_up)    ||
					(key[KEY_DOWN]  != key_down)  ||
					(key[KEY_LEFT]  != key_left)  ||
					(key[KEY_RIGHT] != key_right)) {
					//keys changed
					key_up    = key[KEY_UP];
					key_down  = key[KEY_DOWN];
					key_left  = key[KEY_LEFT];
					key_right = key[KEY_RIGHT];
					//send early keys packet
					sendnow = true;
				}

				// send client's input packet now
				if (sendnow)
					send_frame(false);

				// keypresses to talk prompt
				while (keypressed()) {
					//get key
					int ch = readkey();
					int sc = ch >> 8;	// scancode
					ch = ch & 0xff;		// char

					// toggle help
					if (sc == KEY_F1)
						toggle_help();
					else if (sc == KEY_F2)
						set_menu(menu_maps);
					else if (sc == KEY_TAB)
						set_menu(menu_players);
					else if (sc == KEY_F5)
						set_menu(menu_teams);

					// change colours
					if (sc == KEY_HOME) {
						if (key[KEY_LCONTROL] || key[KEY_RCONTROL])
							client_graphics.reset_playground_colors();
						else
							client_graphics.random_playground_colors();
						// predraw new colours
						predraw();
					}

					// Insert: show more messages
					if (sc == KEY_INSERT) {
						if (chat_visible < chat_size)
							chat_visible = chat_size;
						else
							chat_visible = 8;
					}

					// F10: change name
					if (sc == KEY_F10) {
						editplayername = RandomName();
						change_name_command();
					}
					else if (sc == KEY_F3) {
						option_show_names = !option_show_names;
					}
					// F11: screenshot
					else if (sc == KEY_F11) {
						screenshot = true;
					}
					// Backspace: erase one character
					else if (sc == KEY_BACKSPACE) {
						if (!talkbuffer.empty())
							talkbuffer.erase(talkbuffer.end() - 1);
					}
					// Enter: send text
					else if (sc == KEY_ENTER || sc == KEY_ENTER_PAD) {
						if (!talkbuffer.empty()) {
							send_chat(talkbuffer);
							talkbuffer.clear();
						}
					}
					// Add character to text, max text length 60 chars.
					else if (talkbuffer.length() < 60 && ch >= 32)
						talkbuffer += static_cast<char>(ch);
				}
			}

			// F4 == want/don't want to exit map
			if ((menu == menu_none || menu == menu_maps) && key[KEY_F4]) {
				if (!key_votexit) {
					key_votexit = true;

					//toggle my local option
					want_map_exit = !want_map_exit;

					//want to swap/dont want  message
					char lebuf[16]; int count = 0;
					if (want_map_exit)
						writeByte(lebuf, count, data_map_exit_on);
					else
						writeByte(lebuf, count, data_map_exit_off);
					client->send_message(lebuf, count);
				}
			}
			else
				key_votexit = false;

			//ESC = show/hide menu, go back menu (special key)
			static bool kesc = false;
			if (key[KEY_ESC]) {
				if (!kesc) {
					kesc = true;
					if (!talkbuffer.empty()) // cancel chat
						talkbuffer.clear();
					else if (trying_connection) {		//trying connection
						trying_connection = false;	//not anymore

						//this cancels the attempt to connect
						client->connect(false);
						//go back to main screen
						set_menu(menu_main);
					}
					//se mostrando help, quita
					else if (helpshow)
						toggle_help();
					else if (menu == menu_none)		// no menu, show
						set_menu(menu_main);
					else {		// menu
						if (menu == menu_dialog || menu == menu_name_password || menu == menu_server_list || menu == menu_server_password)
							set_menu(menu_main);	// go back one screen
						else if (gameshow || menu != menu_main)
							set_menu(menu_none);	// hide menu
					}
				}
			}
			else
				kesc = false;

			//sleep a bit
			if (quick_fix)
				quick_fix = false;
			else
				MS_SLEEP(2);				// *** OPTIMIZE THIS ***

		} while (speed_counter < 1);

		//LOG("** ...exited spd counter>0\n");

		//must be time for another frame later
		while (speed_counter > 0) {
			speed_counter--;
			client_netsend_counter++;
		}

		while (client_netsend_counter >= targetfps / 10) {
			client_netsend_counter = 0;
			send_frame(true);
		}

		// (2) if game is showing, simulate
		//
		if (gameshow) {
			pthread_mutex_lock( &frame_mutex );
			ClientPhysicsCallbacks cb(*this);
			#ifdef CLIENT_PREDICTION
			float timeDelta = max<float>(0., averageLag - lagWanted) + (get_time() - frameReceiveTime) * 10.;
			NLubyte firstFrame;
			if (clFrameSent == clFrameWorld)
				firstFrame = clFrameWorld;
			else
				firstFrame = clFrameWorld + 1;
			NLubyte lastFrame = firstFrame;
			while (lastFrame != clFrameSent && timeDelta > 1.) {
				++lastFrame;
				timeDelta -= 1;
			}

			if (timeDelta > 5.)
				timeDelta = 5.;
			fd.extrapolate(fx, cb, me, controlHistory, firstFrame, lastFrame, timeDelta);
			#else
			fd.extrapolate(fx, cb, me, controlHistory, clFrameWorld, clFrameWorld, (get_time() - frameReceiveTime) * 10.);
			#endif
			pthread_mutex_unlock( &frame_mutex );
		}

		// (3) draw operations
		//
		//if (page_flipping) {
			//LOG1("acquire_bitmap/gs=%i...",gameshow);
			//acquire_bitmap(drawbuf);
			//LOG("OK\n");
		//}

		if (gameshow) {
			//clear_to_color(drawbuf, makecol(rand(),0,0));	// clear buffer
			//LOG("draw_game_frame()\n");
			draw_game_frame(); // draw game frame
			//LOG("exit draw_game_frame()\n");
		} else {
			client_graphics.clear();
			//int co = makecol(0x22, 0x22, 0x22);
			//textprintf(drawbuf, font, 0, 0, co, "page-flipping = %i", page_flipping);
			//textprintf(drawbuf, font, 0, 10, co, "port = %i", port);
			//menu = menu_main;
		}

		if (helpshow)
			client_graphics.game_help();
		else if (menu != menu_none)
			draw_game_menu();

		//if (page_flipping) {
			//LOG("** releasing bitmap...");
			//release_bitmap(drawbuf);
			//LOG("OK!\n");
		//}

		// (4) flip or blt
		//
		/*if (page_flipping) {
			show_video_bitmap(drawbuf);

			if (drawbuf == vidpage1)
				drawbuf = vidpage2;
			else
				drawbuf = vidpage1;
		}
		else*/
		client_graphics.draw_screen();
		if (screenshot) {
			save_screenshot();
			screenshot = false;
		}
	}

	//client exit cleanup: done at stop wich needs to be called after loop
}

//stop
void gameclient_c::stop() {

	//at least disconnect
	disconnect_command();

	//join with token request thread, if any
	if (player_password_set) {
		LOG("**** CLIENT JOINING PASSWORD-TOKEN THREAD.... ****\n");
		player_password_set = false;
		pthread_join( passthread, 0 );
	}

	//save configuration file
	//try to load client configuration
	char dest[WHERE_PATH_SIZE];
	append_filename(dest, wheregamedir, "clconfig.txt", WHERE_PATH_SIZE);
	LOG1("dest for clconfig.txt OUT = %s\n", dest);

	ofstream cfg(dest);
	if (cfg) {
		if (client_sounds.no_sounds())
			cfg << "-\n";
		else
			cfg << client_sounds.theme_dir() << '\n';
		if (client_graphics.basic())
			cfg << "-\n";
		else
			cfg << client_graphics.theme_dir() << '\n';

		if (!playername.empty())
			cfg << playername << '\n';
		else
			cfg << "Unnamed_Bastard\n";

		for (int i = 0; i < MAX_GAMESPY; i++) {
			LOG1("Saving gamespy address = '%s'\n", gamespy[i].address);
			cfg << gamespy[i].address << '\n';
		}
		cfg.close();
	}

	//save client's password
	LOG("Saving password file...\n");
	append_filename(dest, wheregamedir, "password.bin", WHERE_PATH_SIZE);
	FILE *psf = fopen(dest, "wb");
	if (psf) {

		char cha;
		for (int c=0;c<PASSBUFFER;c++) {
			if (c < static_cast<int>(player_password.length())) {
				//write toggling bits
				int rot = player_password[c];
				rot = 255 - rot;
				cha = (char)rot;
				fputc(cha, psf);
			}
			else
				fputc(255, psf);		//255 = 0 toggled! (important)
		}

		fclose(psf);
	}
	else
		LOG("ERROR: CANNOT OPEN PASSWORD FILE FOR WRITING\n");

	//clear udpdq
	for (int uq=0;uq<MAX_UDPDQ;uq++)
		if (udpdq[uq] != 0) {
			delete udpdq[uq];
			udpdq[uq] = 0;
		}
	if (message_logging)
		message_log.close();

	// stop listenserver if it was running
	listen_stop();
}

//ctor
gameclient_c::gameclient_c():
	current_map(-1),
	map_vote(-1),
	player_stats_page(0),
	name_selected(true),
	screenshot(false)
{

	//net client
	client = 0;

	//not showing
	option_show_names = false;

	//all the players to show including me
	//player_t player[MAX_PLAYERS];
	for (int p=0;p<MAX_PLAYERS;p++)
		fx.player[p].used=false;

	//wich player I am
	me = -1;

	//time of last packet received
	lastpackettime=0;

	//menu showing?
	menu = menu_main;		//menu screen #

	//game showing?
	gameshow = false;

	//frames and seconds for FPS counter
	FPS=0;
	framecount = 0;
	totalframecount = 0;
	frameCountStartTime = 0;

	//if player wants to changeteams
	want_change_teams = false;

	//trying connection? if true, ESC cancels it
	trying_connection = false;

	//connected? (that is, "connection accepted")
	connected = false;

	//connect screen, my "mini-gamespy"
	gi=0;	//what game entry

	chaterasetime = 0;				// time to erase a chat message from the list

	pthread_mutex_init(&frame_mutex, 0);

	pthread_mutex_init(&udpdq_mutex, 0);		//UDP download queue
	udpdq_size = 0;
	message_logging = false;
}

//dtor
gameclient_c::~gameclient_c() {

	if (client) {
		delete client;
		client = 0;
	}

	pthread_mutex_destroy(&frame_mutex);

	pthread_mutex_destroy(&udpdq_mutex);
}

void gameclient_c::rocketHitWallCallback(int rid, bool power, float x, float y, int roomx, int roomy) {
	if (power) {
		graphics().create_quadwallexplo(static_cast<int>(x), static_cast<int>(y), roomx, roomy);
		sounds().play(SAMPLE_QUADWALLHIT);
	}
	else {
		graphics().create_wallexplo(static_cast<int>(x), static_cast<int>(y), roomx, roomy);
		sounds().play(SAMPLE_WALLHIT);
	}
	fd.rock[rid].owner = fx.rock[rid].owner = -1;	// erase from clientside simulation
}

void gameclient_c::rocketOutOfBoundsCallback(int rid) {
	fd.rock[rid].owner = fx.rock[rid].owner = -1;	// erase from clientside simulation
}

void gameclient_c::playerHitWallCallback(int pid) {
	// play bounce sample if minimum time elapsed
	float currTime = get_time();	//#fix
	if (currTime > fx.player[pid].wall_sound_time) {
		fx.player[pid].wall_sound_time = currTime + 0.2;
		sound(SAMPLE_WALLBOUNCE);
	}
}

bool gameclient_c::shouldApplyPhysicsToPlayerCallback(int pid) {
	return fx.player[pid].onscreen;
}

void gameclient_c::predraw() {
	client_graphics.draw_playground();
	// draw walls
	if (fx.player[me].roomx >= 0 && fx.player[me].roomx < fx.map.w &&
		fx.player[me].roomy >= 0 && fx.player[me].roomy < fx.map.h)
			client_graphics.predraw_room_ground(fx.map.room[fx.player[me].roomx][fx.player[me].roomy]);
	// draw flag position mark
	for (int team = 0; team < 2; team++)
		for (vector<spoint_t>::const_iterator pi = fx.map.tinfo[team].flags.begin(); pi != fx.map.tinfo[team].flags.end(); ++pi)
			if (fx.player[me].roomx == pi->px && fx.player[me].roomy == pi->py)
				client_graphics.draw_flagpos_mark(team, pi->x, pi->y);
	// draw walls
	if (fx.player[me].roomx >= 0 && fx.player[me].roomx < fx.map.w &&
		fx.player[me].roomy >= 0 && fx.player[me].roomy < fx.map.h)
			client_graphics.predraw_room_walls(fx.map.room[fx.player[me].roomx][fx.player[me].roomy]);
	client_graphics.draw_minimap_background();
}

//draw the whole game screen
void gameclient_c::draw_game_frame() {
	// erase old chat messages (this shouldn't be here really but wtf..)
	if (chaterasetime < get_time())
		erase_first_message();

	//lock frame mutex
	pthread_mutex_lock( &frame_mutex );

	// hiding stuff?
	// v0.4.1 : hide stuff if frame skipped
	bool hide_game = !map_ready || gameover_plaque!=NEXTMAP_NONE || fx.skipped || me<0 || me>maxplayers;

	// the playground: border, walls and pits
	if (hide_game) {
		client_graphics.draw_empty_background();

		// game over message
		if ((gameover_plaque == NEXTMAP_CAPTURE_LIMIT) || (gameover_plaque == NEXTMAP_VOTE_EXIT)) {
			if (red_final_score > blue_final_score)
				client_graphics.draw_scores("RED TEAM WINS", 0, red_final_score, blue_final_score);
			else if (blue_final_score > red_final_score)
				client_graphics.draw_scores("BLUE TEAM WINS", 1, blue_final_score, red_final_score);
			else
				client_graphics.draw_scores("GAME TIED", -1, blue_final_score, red_final_score);
		}
		else
			client_graphics.draw_one_line_message("Connecting...");

		client_graphics.draw_waiting_map_message("Waiting game start - next map is:", fx.map.title);
		if (!map_ready) {
			ostringstream text;
			text << "Loading map: " << fdp << " bytes";
			client_graphics.draw_loading_map_message(text.str());
		}
	}
	else
		client_graphics.draw_background();

	// frame is valid?
	if (!hide_game && fd.frame >= 0) {
		// FIXME: y-ordering of draw not maintained
		// draw any item pickups
		//
		if (me >= 0)
			for (int i = 0; i < MAX_PICKUPS; i++)
				// used power-ups, not respawning, on my screen
				if (fx.item[i].kind > 0 && fx.item[i].kind != 255 &&
						fx.item[i].px == fx.player[me].roomx && fx.item[i].py == fx.player[me].roomy) {
					client_graphics.draw_pup(fx.item[i], get_time());
					//deathbringer
					if (fx.item[i].kind == 7)
						client_graphics.create_deathcarrier(fx.item[i].x + rand() % 30 - 15, fx.item[i].y + rand() % 30 - 5,
      						fx.item[i].px, fx.item[i].py, 0);
				}

		// draw speed effect
		client_graphics.draw_speedfx(fx.player[me].roomx, fx.player[me].roomy, get_time());

		// FIXME: y-ordering of draw not maintained
		// draw any dropped flags (use fx since flags don't move)
		//
		for (int t = 0; t < 2; t++)
			for (vector<Flag>::const_iterator fi = fx.teams[t].flags().begin(); fi != fx.teams[t].flags().end(); ++fi)
				// not carried, on same screen
				if (!fi->carried() && fi->position().px == fx.player[me].roomx && fi->position().py == fx.player[me].roomy)
					client_graphics.draw_flag(t, fi->position().x, fi->position().y);

		// FIXME: y-ordering of draw not maintained
		// draw any rockets
		for (int i = 0; i < MAX_ROCKETS; i++)
			if (fx.rock[i].owner != -1 && fx.rock[i].px == fx.player[me].roomx && fx.rock[i].py == fx.player[me].roomy) {
				fd.rock[i].team = fx.rock[i].team;
				fd.rock[i].power = fx.rock[i].power;
				client_graphics.draw_rocket(fd.rock[i], get_time());
			}

		// sort order of drawing of the players
		//
		for (int i = 0; i < maxplayers; i++) {
			fd.player[i].drawused = 0;
			fd.player[i].drawptr = -1;
		}

		double miny;
		int minyid;

		int i;
		for (i = 0; i < maxplayers; i++) {
			minyid = -1;
			miny = 999999;

			for (int j = 0; j < maxplayers; j++)
			if (fd.player[j].used)	{
				if (fd.player[j].drawused == 0)
				if (fd.player[j].ly < miny) {
					miny = fd.player[j].ly;
					minyid = j;
				}
			}

			if (minyid == -1)
				break;

			fd.player[minyid].drawused = 1;
			fd.player[i].drawptr = minyid;
		}

		// the PLAY AREA: the players!
		for (int k = 0; k < maxplayers; k++) {
			//HACK REMENDEX: predict item_helm
			if (fd.player[i].item_helm()) {
				int hspd = static_cast<int>((fd.frame - fx.frame) * 10.);
				fd.player[i].visibility = fx.player[i].visibility - hspd;
				if (fd.player[i].visibility < 0)
					fd.player[i].visibility = 0;
			}

			//indirection: draw in y-order
			int i;
			i = fd.player[k].drawptr;

			if (i >= 0 && fx.player[i].onscreen) {		// draw only players on my screen
				//calcula alfa do player
				int alpha = fd.player[i].visibility;
				if (i / TSIZE == me / TSIZE && alpha < MIN_ALPHA_FRIENDS)
					alpha = MIN_ALPHA_FRIENDS;
				client_graphics.draw_player_shadow(fx.player[i], alpha);	//#fix? fx -> fd to make shadow not jump
				// DRAW FLAG IF PLAYER IS CARRIER OF A FLAG
				for (int t = 0; t < 2; t++)
					for (vector<Flag>::const_iterator fi = fx.teams[t].flags().begin(); fi != fx.teams[t].flags().end(); ++fi)
						if (fi->carrier() == i)
							client_graphics.draw_flag(t, (int)fd.player[i].lx, (int)fd.player[i].ly + 15);
				if (fx.player[i].dead) {
					if ((fx.player[i].frags >= 10) && (fx.player[i].frags % 10 == 0))
						client_graphics.draw_virou_sorvete((int)fx.player[i].lx, (int)fx.player[i].ly);
					else
						client_graphics.draw_player_dead((int)fx.player[i].lx, (int)fx.player[i].ly);
				}
				// desenha player vivo
				else {
					// turbo effect
					if (fx.player[i].item_speed && (fabs(fx.player[i].sx) > svp_maxspeed || fabs(fx.player[i].sy) > svp_maxspeed) &&
						get_time() > fx.player[i].speed_drop_time)		// intervalo entre drop de efeito bolinha
					{
						//tempo minimo pra soltar outra bolinha fade
						fx.player[i].speed_drop_time = get_time() + 0.05;
						//solta a bolinha
						client_graphics.create_speedfx((int)fx.player[i].lx, (int)fx.player[i].ly, fx.player[i].roomx, fx.player[i].roomy, i / TSIZE, i % TSIZE, fx.player[i].gundir);
					}

					//draw player
					client_graphics.draw_player((int)fd.player[i].lx, (int)fd.player[i].ly, i / TSIZE, i % TSIZE, fx.player[i].gundir, fx.player[i].hitfx, fx.player[i].item_quad, alpha, get_time());

					//draw deathbringer carrier effect
					if (fx.player[i].item_deathbringer) {
						// intervalo entre drop de efeito
						if (get_time() > fx.player[i].death_drop_time) {
							//tempo p/ proximo efeito
							fx.player[i].death_drop_time = get_time() + 0.01;
							//drop it
							client_graphics.create_deathcarrier((int)fd.player[i].lx + rand()%40-20, (int)fd.player[i].ly + rand()%40, fx.player[i].roomx, fx.player[i].roomy, i/TSIZE);
							client_graphics.create_deathcarrier((int)fd.player[i].lx + rand()%40-20, (int)fd.player[i].ly + rand()%40, fx.player[i].roomx, fx.player[i].roomy, i/TSIZE);
						}
					}
					// draw deathbringer affected effect
					if (fx.player[i].deathbringer_affected)
						client_graphics.draw_deathbringer_affected((int)fd.player[i].lx, (int)fd.player[i].ly, i / TSIZE);
					// shield
					if (fx.player[i].item_shield)
						client_graphics.draw_shield((int)fd.player[i].lx, (int)fd.player[i].ly, SHIELD_RADIUS, alpha);
				}
			}

			//draw player's name -- nao interessa se vivo ou morto
			//NOT an invisible enemy
			if (option_show_names && fx.player[i].used && !(fx.player[i].visibility < 10 && i / TSIZE != me / TSIZE) &&
				fx.player[i].roomx == fx.player[me].roomx && fx.player[i].roomy == fx.player[me].roomy) {
				int ttx = (int)fd.player[i].lx;
				int tty = (int)fd.player[i].ly;
				client_graphics.draw_player_name(fx.player[i].name, ttx, tty, i / TSIZE);
			}
		}

		for (int i = 0; i < maxplayers; i++)
			if (fx.player[i].used && fx.player[i].roomx == fx.player[me].roomx && fx.player[i].roomy == fx.player[me].roomy &&
				fx.player[i].onscreen && fx.player[i].item_deathbringer)
					client_graphics.draw_deathbringer_carrier_effect((int)fd.player[i].lx, (int)fd.player[i].ly);

		client_graphics.draw_effects(fx.player[me].roomx, fx.player[me].roomy, get_time());
	}

	//do not draw stuff below if map not ready to show
	if (!hide_game) {
		// the MINIMAP
		//client_graphics.draw_minimap_background();

		//draw the miniflags
		// - qualquer flag no chao (na base ou nao, carried == false)
		for (int t = 0; t < 2; t++)
			for (vector<Flag>::const_iterator fi = fx.teams[t].flags().begin(); fi != fx.teams[t].flags().end(); ++fi)
				if (!fi->carried())
					client_graphics.draw_mini_flag(t, *fi, fx.map);

		vector<bool> roomvis(fx.map.w * fx.map.h, (me >= 0 && fx.player[me].item_helm()) ? true : false);

		// draw all teammates and enemies on screens where there are teammates
		//draw all the players - put a pixel where they are
		if (me >= 0 && fx.frame >= 0)
			for (int i = 0; i < maxplayers; i++)
				if (fx.player[i].used && fx.player[i].roomx >= 0 && fx.player[i].roomy >= 0 && fx.player[i].roomx < fx.map.w && fx.player[i].roomy < fx.map.h &&
						(i / TSIZE == me / TSIZE || (fx.player[me].enemyvis & (1 << (i % TSIZE))))) {
					roomvis[fx.player[i].roomy * fx.map.w + fx.player[i].roomx] = true;

					//verifica se o jogador a ser desenhado é um carrier de flag inimiga
					const int enemy = 1 - i / TSIZE;
					int f = 0;
					for (vector<Flag>::const_iterator fi = fx.teams[enemy].flags().begin(); fi != fx.teams[enemy].flags().end(); ++fi, ++f)
						if (fi->carrier() == i) {
							// update flag position for draw
							fx.teams[enemy].move_flag(f, spoint_t(fx.player[i].roomx, fx.player[i].roomy,
								static_cast<int>(fx.player[i].lx), static_cast<int>(fx.player[i].ly)));

							// draw the miniflag here
							client_graphics.draw_mini_flag(enemy, *fi, fx.map);
						}

					if (i != me)
						client_graphics.draw_minimap_player(fx.map, fx.player[i], i / TSIZE, i % TSIZE);
					else // myself: draw differently
						client_graphics.draw_minimap_me(fx.map, fx.player[i], i / TSIZE, get_time());
				}

		// paint fog of war in all invisible rooms
		//
		for (int ry = 0; ry < fx.map.h; ry++)
			for (int rx = 0; rx < fx.map.w; rx++)
				if (!roomvis[ry * fx.map.w + rx])
					client_graphics.draw_minimap_room(fx.map, rx, ry);
	}//!hide_game

	//
	// the SCOREBOARD
	//

	ostringstream red;
	red << "Red Team:    " << setw(2) << fx.teams[0].score() << " capt";
	client_graphics.draw_scoreboard_caption(0, red.str());
	ostringstream blue;
	blue << "Blue Team:   " << setw(2) << fx.teams[1].score() << " capt";
	client_graphics.draw_scoreboard_caption(1, blue.str());

	/*vector<ClientPlayer> plrs(fx.player, fx.player + MAX_PLAYERS);
	client_graphics.draw_scoreboard(plrs);*/
	int pix[2]; pix[0]=pix[1]=0;
	for (int fw=0;fw<2;fw++)		//first count, then draw
	{
		const int NAMEYDELTA_MIN = 8;
		int NAMEYDELTA = 12;	//default value
		if (fw == 1) {
			//most players in a team
			int thepix;
			if (pix[0] > pix[1])
				thepix = pix[1];
			else
				thepix = pix[0];
			//calc the new NAMEYDELTA
			while (thepix * NAMEYDELTA > 16 * 8 - 8 - 2) {
				if (NAMEYDELTA == NAMEYDELTA_MIN)
					break;	//the minimum is the minimum...
				NAMEYDELTA--;
			}
		}

		for (int dp=0;dp<maxplayers;dp++)
		{
			// i = player #   dp = draw slot (0-7 = red team's  8-15 = blue team's)
			int i = scoreboard[dp];

			//i = dp;

			//draw it
			if (i != -1)
			if (fx.player[i].used)
			{
				int what_y;
				if (i < TSIZE)
					what_y = sby + 8 + dp * NAMEYDELTA;
				else
					what_y = sby + 19 * NAMEYDELTA_MIN + (dp - TSIZE) * NAMEYDELTA;

				//just count
				if (fw == 0) {
//						if (pix[i/TSIZE] < i%TSIZE)
//							pix[i/TSIZE] = i%TSIZE;
					if (pix[i/TSIZE] < dp%TSIZE)
						pix[i/TSIZE] = dp%TSIZE;
				}
				//draw
				else {
					// show name
					client_graphics.draw_scoreboard_name(what_y, i % TSIZE, fx.player[i]);

					// show frags
					client_graphics.draw_scoreboard_points(what_y, i / TSIZE, fx.player[i].frags);
				}
			}
		}
	}

	// the STATUSBAR : traffic
	//
	//int bpsin = client->get_socket_stat(NL_AVE_BYTES_RECEIVED);
	//int bpsout = client->get_socket_stat(NL_AVE_BYTES_SENT);
	//int bpstraffic = bpsin + bpsout;
	//textprintf(drawbuf, font, 72*8-2, ply+plh+  5, col[COLINFO], "BPS:%4i", bpstraffic);
	//textprintf(drawbuf, font, 71*8-2, ply+plh+ 15, col[COLINFO], "%4i:%4i", bpsin, bpsout);

	//FPS
	client_graphics.draw_fps(FPS);

	// Time left if time limit is on.
	if (map_time_limit)
		if (map_end_time > (unsigned int)get_time())
			client_graphics.map_time(map_end_time - (unsigned int)get_time());
		else
			client_graphics.map_time(0);

	// player's power-ups
	if (me >= 0) {
		double val;
		if (fx.player[me].item_quad) {
			val = fx.player[me].item_quad_time - get_time();
			if (val < 0) val = 0;
			client_graphics.draw_player_power(val);
		}
		if (fx.player[me].item_speed) {
			val = fx.player[me].item_speed_time - get_time();
			if (val < 0) val = 0;
			client_graphics.draw_player_turbo(val);
		}
		if (fx.player[me].item_helm()) {
			val = fx.player[me].item_helm_time - get_time();
			if (val < 0) val = 0;
			client_graphics.draw_player_shadow(val);
		}

		//WEAPON LEVEL
		client_graphics.draw_player_weapon(fx.player[me].weapon + 1);
	}

	//show "want change teams" flag if active
	if (want_change_teams)
		client_graphics.draw_change_team_message(get_time());
	//show "want change map" flag if active
	if (want_map_exit)
		client_graphics.draw_change_map_message(get_time());

	// the STATUSBAR : health energy, bars ....
	if (me >= 0) {
		client_graphics.draw_player_health(fx.player[me].health);
		client_graphics.draw_player_energy(fx.player[me].energy);
	}

	// the HUD: message output
	int start = static_cast<int>(chatbuffer.size()) - static_cast<int>(chat_visible);
	if (start < 0)
		start = 0;
	list<string>::const_iterator msg = chatbuffer.begin();
	for (int i = 0; i < start; ++i, ++msg);
	client_graphics.print_chat_messages(msg, chatbuffer.end(), talkbuffer);

	//"server not responding... connection may have dropped" plaque
	if (get_time() > lastpackettime + 1.0)
		client_graphics.show_not_responding_message();

	/*if (key[KEY_TAB]) {
		drawing_mode(DRAW_MODE_TRANS, 0,0,0);
		set_trans_blender(0,0,0,150);

		int w = 440;
		int h = 420;
		int mx = SCREEN_W / 2;
		int my = SCREEN_H / 2;
		int x1 = mx - w/2;
		int y1 = my - h/2;
		int x2 = mx + w/2;
		int y2 = my + h/2;
		int xc = (x1+x2)/2;

		rectfill(drawbuf, x1,y1,x2,y2, 0);

		solid_mode();

		int XLEFTPAD = x1+40;
		int YDEL;
		char sorry[256];
		int p, redt = 0, bluet = 0;
		double redpow = 0.0, bluepow = 0.0;

		// FIXME: "max world score"? "max world rating"?
		textprintf_centre(drawbuf, font, xc, y1+10, col[COLWHITE], "Ranking - %lu players", max_world_rank); //, max_world_score);

		textprintf(drawbuf, font, XLEFTPAD, y1+45, col[COLWHITE], "Rank Power Score Name            Frags Ping");

		YDEL = 60;

		for (p=0;p<TSIZE;p++)
		if (scoreboard[p] >= 0)
		{
			int i = scoreboard[p];
			redt++;

			sorry[0]=0;
			if (fx.player[i].reg_status == ' ' || fx.player[i].reg_status == '?')
				strcpy(sorry, " ");

			if (sorry[0]==0) {
				textprintf(drawbuf,font,XLEFTPAD,y1+YDEL, col[COLLRED], "%4i %5.2f %5i %-15s %5i %4i",
					fx.player[i].rank,
					( ( ((double)fx.player[i].score) + 1.0) / ( ((double)fx.player[i].neg_score) + 1.0) ),
					fx.player[i].score - fx.player[i].neg_score,
					fx.player[i].name.c_str(),
					fx.player[i].frags,
					fx.player[i].ping
				);
				//V0.4.8
				redpow += ((double)(fx.player[i].score+1)) / ((double)(fx.player[i].neg_score+1));
			}
			else {
				textprintf(drawbuf,font,XLEFTPAD,y1+YDEL, col[COLLRED], "%16s %-15s %5i %4i",
					sorry,
					fx.player[i].name.c_str(),
					fx.player[i].frags,
					fx.player[i].ping
				);
				redpow += DEFAULT_PLAYER_RATE;//V0.4.8
			}

			//next
			YDEL += 9;
		}

		textprintf(drawbuf, font, XLEFTPAD, y1+240, col[COLWHITE], "Rank Power Score Name            Frags Ping");

		YDEL = 255;

		for (p=TSIZE;p<maxplayers;p++)
		if (scoreboard[p] >= 0)
		{
			bluet++;
			int i = scoreboard[p];

			sorry[0]=0;
			if (fx.player[i].reg_status == ' ' || fx.player[i].reg_status == '?')
				strcpy(sorry, " ");

			if (sorry[0]==0) {
				textprintf(drawbuf,font,XLEFTPAD,y1+YDEL, col[COLLBLUE], "%4i %5.2f %5i %-15s %5i %4i",
					fx.player[i].rank,
					( ( ((double)fx.player[i].score) + 1.0) / ( ((double)fx.player[i].neg_score) + 1.0) ),
					fx.player[i].score - fx.player[i].neg_score,
					fx.player[i].name.c_str(),
					fx.player[i].frags,
					fx.player[i].ping
				);
				bluepow += ((double)(fx.player[i].score+1)) / ((double)(fx.player[i].neg_score+1)); //V0.4.8
			}
			else {
				textprintf(drawbuf,font,XLEFTPAD,y1+YDEL, col[COLLBLUE], "%16s %-15s %5i %4i",
					sorry,
					fx.player[i].name.c_str(),
					fx.player[i].frags,
					fx.player[i].ping
				);
				bluepow += DEFAULT_PLAYER_RATE;	//V0.4.8
			}

			//next
			YDEL += 9;
		}

		//V0.4.8
		textprintf_centre(drawbuf, font, xc, y1+30, col[COLLRED], "Red Team - Power %.2f", redpow);
		textprintf_centre(drawbuf, font, xc, y1+225, col[COLLBLUE], "Blue Team - Power %.2f", bluepow);
	}*/

	// debug panel
	if (key[KEY_F9]) {
		const int bpsin = client->get_socket_stat(NL_AVE_BYTES_RECEIVED);
		const int bpsout = client->get_socket_stat(NL_AVE_BYTES_SENT);
		client_graphics.debug_panel(fx.player, me, bpsin, bpsout);

		/*int p;
		for (p=0;p<maxplayers;p++) {
			textprintf(drawbuf,font,0,10+p*10,col[COLWHITE], "p.%i u=%i ons=%i evs=%lu sxy=%i,%i HR:p=%.1f,%.1f s=%.1f,%.1f",
				p, fx.player[p].used, fx.player[p].onscreen, fx.player[p].enemyvis, fx.player[p].roomx, fx.player[p].roomy,

				//					fx.player[p].x, fx.player[p].y, fx.player[p].sx, fx.player[p].sy,
				fd.player[p].lx, fd.player[p].ly, fd.player[p].sx, fd.player[p].sy
				);
		}

		int bpsin = client->get_socket_stat(NL_AVE_BYTES_RECEIVED);
		int bpsout = client->get_socket_stat(NL_AVE_BYTES_SENT);
		int bpstraffic = bpsin + bpsout;
		textprintf(drawbuf, font, 72*8-2, ply+plh+  5, col[COLINFO], "BPS:%4i", bpstraffic);
		textprintf(drawbuf, font, 71*8-2, ply+plh+ 15, col[COLINFO], "%4i:%4i", bpsin, bpsout);*/
	}


	//unlock frame mutex
	//LOG1("unlocking HOW=%i",HOWMANY);
	pthread_mutex_unlock( &frame_mutex );
	//LOG1("unlocked! HOW=%i",HOWMANY);

	// another frame, calc FPS...
	//
	totalframecount++;
	framecount++;
	double baixo = get_time() - frameCountStartTime;
	if (baixo > 0) {
		if (baixo > 1.0) {
			FPS = ((double)framecount) / baixo;
			frameCountStartTime = get_time();
			framecount = 0;
		}
	}
}

//draws the game menu
void gameclient_c::draw_game_menu() {
	switch (menu) {
		case menu_main:
			client_graphics.main_menu(connected, address, playername, namestatus,
				listen_server_running, listen_port_running, client_sounds);
			break;
		case menu_server_list:
			if (showmaster) {
				vector<gamespy_t> servers(mgamespy, mgamespy + MAX_GAMESPY);
				client_graphics.public_servers(servers, gi);
			}
			else {
				vector<gamespy_t> servers(gamespy, gamespy + MAX_GAMESPY);
				client_graphics.favourite_servers(servers, gi);
			}
			break;
		case menu_dialog:
			client_graphics.dialog(dialogmessage, dialogmessage2);
			break;
		case menu_name_password:
			client_graphics.name_password_menu(editplayername, editplayerpass.length(), name_selected, namestatus);
			break;
		case menu_server_password:
			client_graphics.server_password_menu(edit_server_password.length());
			break;
		case menu_maps:
			client_graphics.map_list(maps, current_map, map_vote, edit_map_vote);
			break;
		case menu_players:
			client_graphics.draw_statistics(fx.player, player_stats_page, static_cast<int>(get_time()));
			break;
		case menu_teams:
			client_graphics.team_statistics(fx.teams);
			break;
		default: ;
	}
}

void gameclient_c::close_button_callback() {
	force_exit = true;
}

int cfunc_connection_update(client_runes_t *arg) {

	int count;
	char lebuf[256];

	switch (arg->connect_result) {
	case 0:
		LOG("client connected.\n");
		gameclient->client_connected(arg->data, arg->length);
		break;
	case 1:
		LOG("client disconnected.\n");
		gameclient->client_disconnected();
		break;
	case 2:
		LOG("cannot connect, server denied (full?)\n");
		gameclient->connect_failed_denied(arg->data, arg->length);
		break;
	case 3:
		LOG("cannot connect, server not responding\n");
		gameclient->connect_failed_unreachable();
		break;
	case 4:
		LOG("cannot connect, net-server is full!");
		count = 0;
		writeString(lebuf, count, "Server is full.");
		gameclient->connect_failed_denied(lebuf, count);
		break;
	default:
		LOG1("WARNING: client connection update unknown code = %i\n", arg->connect_result);
		break;
	}
	return 0;
}

int cfunc_server_data(client_runes_t *arg) {

	//LOG1("client data=%i\n", arg->length);

	gameclient->process_incoming_data(arg->data, arg->length);

	return 0;
}

//============================================================
//  client make login (user,password) to get a token
//============================================================

//client password-to-token retrieval thread
void *thread_clientpassword_f(void *arg) {

	gameclient->client_password_thread(arg);

	return 0;
}

