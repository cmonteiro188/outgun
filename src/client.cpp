#include "commont.h"
#include "client.h"

//colors
enum {
	//player's colors
	COLGREEN,
	COLYELLOW,
	COLWHITE,
	COLMAG,
	COLCYAN,
	COLORA,
	COLLRED,		// light red
	COLLBLUE,		// light blue
	//MORE player colors
	COL9,
	COL10,
	COL11,
	COL12,
	COL13,
	COL14,
	COL15,
	COL16,

	//team colors
	COLRED,			//team 1 (color 0)
	COLBLUE,		//team 2 (color 1)

	//base colors
	COLBRED,			//team 1 (color 0)
	COLBBLUE,		//team 2 (color 1)

	//other
	COLFOGOFWAR,
	COLMENUWHITE,
	COLMENUBLACK,
	COLMENUGRAY,
	COLGROUND,
	COLWALL,
	COLNOLIFE,
	COLDARKGRAY,
	COLSHADOW,
	COLLIMBO,
	COLDARKORA,
	COLINFO,
	COLENER3,
	NUM_OF_COL
};

int teamcol[2];
int teamlcol[2];	//light colours for statusbar
int teamdcol[2];	//dark colours for player name

int	col[NUM_OF_COL];

void setcolors() {

	//the first 8 colors are player's colors
	col[COLGREEN] = makecol(0,0xff,0);
	col[COLYELLOW] = makecol(0xff,0xff,0);
	col[COLWHITE] = makecol(0xff,0xff,0xff);
	col[COLMAG]	= makecol(0xff, 0, 0xff);
	col[COLCYAN] = makecol(0, 0xff, 0xff);
	col[COLORA]	= makecol(0xff, 0xb0, 0);
	col[COLLRED] = makecol(0xff,0x55,0x44);
	col[COLLBLUE] = makecol(0x44,0x55,0xff);
	//MORE player colors
	col[COL9] = makecol(242, 158, 224);
	col[COL10] = makecol(134, 143, 57);
	col[COL11] = makecol( 14, 148, 87);
	col[COL12] = makecol( 33, 132, 137);
	col[COL13] = makecol(100, 100, 100);
	col[COL14] = makecol(166, 166, 166);
	col[COL15] = makecol(202, 1, 56);	//wine
	col[COL16] = makecol(0xbf, 0x70, 0);	//darkora

	// team solid colors
	col[COLBLUE] = makecol(0,0,0xff);
	col[COLRED] = makecol(0xff,0,0);

	// base minimap background colors
	col[COLBBLUE] = makecol(0,0,0x66);
	col[COLBRED] = makecol(0x66,0,0);

	//other
	col[COLFOGOFWAR] = makecol(0xff, 0xff, 0xff);
	col[COLMENUWHITE] = makecol(0xc0,0xc0,0xc0);
	col[COLMENUGRAY] = makecol(0x68,0x68,0x68);
	col[COLMENUBLACK] = makecol(0x40,0x40,0x40);
	col[COLGROUND] = makecol(0x10, 0x40, 0);
	col[COLWALL] = makecol(0x30, 0xC0, 0);
	col[COLNOLIFE] = makecol(0,0,0);
	col[COLDARKGRAY] = makecol(0x30, 0x30, 0x30);
	col[COLSHADOW] = makecol(0x18,0x18,0x18);
	col[COLLIMBO] = makecol(0x10, 0x10, 0x10);
	col[COLDARKORA]	= makecol(0xbf, 0x70, 0);
	col[COLINFO] = col[COLDARKORA];		//color of statusbar non-game info (hostname, IP, net traffic)
	col[COLENER3] = makecol(125, 100, 255);

	//teams 0 & 1 (playernum(0..15) / 8) colors:
	teamcol[0] = col[COLRED];
	teamcol[1] = col[COLBLUE];

	//light colours for text
	teamlcol[0] = col[COLLRED];
	teamlcol[1] = col[COLLBLUE];

	//light colours for team text bg
	teamdcol[0] = col[COLBRED];
	teamdcol[1] = col[COLBBLUE];
}

bool reset_video_mode() {

		char err1[1024];
		char err2[1024];
		char err3[1024];
		char err4[1024];

		//un-show any video bitmaps?
		//show_video_bitmap(screen);

		//destroy all old surfaces
		if (vidpage1) { LOG("destroying vidpage1\n"); destroy_bitmap(vidpage1); vidpage1 = 0; }
		if (vidpage2) { LOG("destroying vidpage2\n"); destroy_bitmap(vidpage2); vidpage2 = 0; }
		if (backbuf) { LOG("destroying backbuf\n"); destroy_bitmap(backbuf); backbuf = 0; }

		int notok;

		set_color_depth(16); // hicolor
		if (winclient) // set mode
			notok = set_gfx_mode(WINMODE, RESOL_X, RESOL_Y, 0, 0);
		else
			notok = set_gfx_mode(FULLMODE, RESOL_X, RESOL_Y, 0, 0);

// ***** INICIO *******

		if (notok < 0) {
			LOG1("ERROR: cannot set 640x480x16 windowed?=%i graphics mode!\n", winclient);
			LOG1("Allegro error: '%s'\n", allegro_error);
			strcpy(err1, allegro_error);

			//try again...
			winclient = !winclient;
			set_color_depth(16); // hicolor
			if (winclient) // set mode
				notok = set_gfx_mode(WINMODE, RESOL_X, RESOL_Y, 0, 0);
			else
				notok = set_gfx_mode(FULLMODE, RESOL_X, RESOL_Y, 0, 0);

			if (notok < 0) {
				LOG1("ERROR: cannot set 640x480x16 windowed?=%i graphics mode!\n", winclient);
				LOG1("Allegro error: '%s'\n", allegro_error);
				strcpy(err2, allegro_error);

				//try again...
				winclient = !winclient;
				set_color_depth(15); // ===> different color depth
				if (winclient) // set mode
					notok = set_gfx_mode(WINMODE, RESOL_X, RESOL_Y, 0, 0);
				else
					notok = set_gfx_mode(FULLMODE, RESOL_X, RESOL_Y, 0, 0);

				if (notok < 0) {

					LOG1("ERROR: cannot set 640x480x15 windowed?=%i graphics mode!\n", winclient);
					LOG1("Allegro error: '%s'\n", allegro_error);
					strcpy(err3, allegro_error);

					//AND try again.....
					winclient = !winclient;
					set_color_depth(15); // ===> different color depth
					if (winclient) // set mode
						notok = set_gfx_mode(WINMODE, RESOL_X, RESOL_Y, 0, 0);
					else
						notok = set_gfx_mode(FULLMODE, RESOL_X, RESOL_Y, 0, 0);

					if (notok < 0) {

						LOG1("ERROR: cannot set 640x480x15 windowed?=%i graphics mode!\n", winclient);
						LOG1("Allegro error: '%s'\n", allegro_error);
						strcpy(err4, allegro_error);

						char elmsg[4096];
						sprintf(elmsg, "ERROR: cannot initialize graphics! reasons:\n1 = '%s'\n2 = '%s'\n3 = '%s'\n4 = '%s'", err1, err2, err3, err4);
						allegro_message(elmsg);
						return false;	// FATAL error
					}

				}
			}
		}

// ***** FIM *******

		text_mode(-1);	//transparent text

#ifndef SWITCH_PAUSE_CLIENT
		if (set_display_switch_mode(SWITCH_BACKAMNESIA) == -1) {
			if (set_display_switch_mode(SWITCH_BACKGROUND) == -1) // allow running in the background
			{
				LOG("ERROR: client cannot run in the background!\n");
				return false; // FATAL
			}
			else LOG("switch_background set ok.");
		}
		else LOG("switch_BACKAMNESIA set ok.");
#endif

		//attempt to create two video bitmaps, else use RAM backbuffer
		if (trypageflip) {
			vidpage1 = create_video_bitmap(SCREEN_W, SCREEN_H);
			vidpage2 = create_video_bitmap(SCREEN_W, SCREEN_H);
			LOG2("create vidpage1=%p vidpage2=%p\n", vidpage1, vidpage2);
		}

		//delete any partial video pages
		if ((!vidpage1) || (!vidpage2)) {

			if (trypageflip)
				LOG("PAGE FLIPPING: not enought video memory. using bruteforce doublebuffering\n")
			else
				LOG("USING SAFE MODE VIDEO -- DOUBLE-BUFFERING (option -dbuf). for page flipping use -flip\n")

			if (vidpage1) { destroy_bitmap(vidpage1); vidpage1 = 0; LOG("destroyed vidpage1\n"); }
			if (vidpage2) { destroy_bitmap(vidpage2); vidpage2 = 0; LOG("destroyed vidpage2\n");}

			//create RAM backbuffer
			backbuf = create_bitmap(SCREEN_W, SCREEN_H);
			if (!backbuf) {
				LOG("ERROR: cannot create memory backbuffer!\n");
				return false; // FATAL
			}
			drawbuf = backbuf;
			page_flipping = false;
		}
		else {
			drawbuf = vidpage1;
			page_flipping = true;
		}
		setcolors();

		return true; //ok
}

gameclient_c *gameclient;

BITMAP *drawbuf, *vidpage1=0, *vidpage2=0, *backbuf=0;
bool		page_flipping;

// client callbacks
int cfunc_connection_update(client_runes_t *arg);
int cfunc_server_data(client_runes_t *arg);

//client downloader thread prototype
void *thread_clientdownloader_f(void *arg);

//client password-to-token retrieval thread
void *thread_clientpassword_f(void *arg);

bool gameclient_c::start() {

	//host ad
	hostad = 0;
	hostadname[0]=0;

	// open message log file
	if (message_logging)
		message_log.open("message.log", ios::app);

	//default physics parameters
	//set_default_physics();
	//LOG3("\nNORMAL   fri %.1f acc %.1f mxs %.1f\n", svp_fric, svp_accel, svp_maxspeed);
	//LOG3("RUN      fri %.1f acc %.1f mxs %.1f\n", svp_fric_run, svp_accel_run, svp_maxspeed_run);
	//LOG3("TURBO    fri %.1f acc %.1f mxs %.1f\n", svp_fric_turbo, svp_accel_turbo, svp_maxspeed_turbo);
	//LOG3("TURBORUN fri %.1f acc %.1f mxs %.1f\n", svp_fric_turborun, svp_accel_turborun, svp_maxspeed_turborun);

	//cursors: default at name
	strcpy(namecursor, "_");
	strcpy(passcursor, "");

	//clear UDPDQ
	for (int uq=0;uq<MAX_UDPDQ;uq++) udpdq[uq] = 0;
	udpdq_ptr = -1;
	udpdq_size = 0;

	//hide helpscreen
	helpshow = false;

	totalframecount = 0;
	framecount = 0;

	// default map
	//load_default_map(&map);
	map_ready = false;		// NO map change commands from server yet
	servermap[0]=0;

	flagpos_buf[0] = 0;
	flagpos_buf[1] = 0;
	flagpos_ready = false;

	//not showing gameover plaque
	gameover_plaque = NEXTMAP_NONE;

	//update minimap background
	minibg = create_bitmap(100, 100);
	//update_minimap_background();

	//clear fx
	clear_fx();

	trying_connection = false;
	connected = false;

	client = new_client_c();
	client->set_callback(CFUNC_CONNECTION_UPDATE, cfunc_connection_update);
	client->set_callback(CFUNC_SERVER_DATA, cfunc_server_data);

	//init gamespy/adresses
	first_fav_refresh = false;
	showmaster = true;
	address[0]=0;
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
	player_password[0]=0;
	editplayerpass[0]=0;
	strcpy(namestatus, "NO PASSWORD SET");
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
			pas[8]=0;
			strcpy(editplayerpass, pas);
			//copy to editpass and simulate pressing ENTER on the name/pass screen...
			check_change_pass_command();
		}

		fclose(psf);
	}

	//try to load client configuration
	bool randomname = true; // give random name
	append_filename(dest, wheregamedir, "clconfig.txt", WHERE_PATH_SIZE);
	LOG1("dest for clconfig.txt = %s\n", dest);

	FILE *cfg = fopen(dest, "r");
	if (cfg) {
		char lebuf[4096];

		//read starting directory name
		sfxthemedir[0]=0;
		if (fgets(sfxthemedir, 256, cfg)) { // load sucessful
			//ok
			sfxthemedir[strlen(sfxthemedir) - 1] = 0;
			LOG1("sfxthemedir default = %s\n", sfxthemedir);
		}

		//read name
		if (fscanf(cfg, "%s", lebuf) == 1) { //if load sucessful
			lebuf[15]=0;		// max needed for name=15!

			//if not an asterisk, load name
			if (strcmp(lebuf, "*"))	{
				randomname=false;
				strcpy(playername, lebuf);
			}
		}

		//read addresses
		for (int i=0;i<MAX_GAMESPY;i++) {

			if (fscanf(cfg, "%s", lebuf) == 1) {
				lebuf[21]=0;		// max needed for IP=15!
				strcpy(gamespy[i].address, lebuf);
				strcpy(mgamespy[i].address, lebuf); //copy to master list too!
			}
		}

		fclose(cfg);
	}

	//give a random name
	if (randomname) {
		string nome_tri_legal = RandomName();
		strcpy(playername, nome_tri_legal.c_str() );
	}

	//no themes set yet
	validtheme = false;

	//no samples loaded -- important so unload_samples don't crash
	for (int n=0;n<NUM_OF_SAMPLES;n++)
		sample[n] = 0;

	//try the last theme directory first
	char themepath[512];
	make_sfx_theme_path(themepath, sfxthemedir);

	LOG1("\ntheme searching '%s'\n", themepath);

	if (0==al_findfirst(themepath, &sfxthemeffblk, FA_DIREC|FA_ARCH|FA_RDONLY))
		set_theme_dir(0);	// OK: load ; 0 = no change
	else {
		// sound theme not found. find the first one
		make_sfx_theme_path(themepath, "*.*");

		int result = al_findfirst(themepath, &sfxthemeffblk, FA_DIREC|FA_ARCH|FA_RDONLY);
		for (; result==0; result = al_findnext(&sfxthemeffblk))
			if ((sfxthemeffblk.attrib&FA_DIREC) && strcmp(sfxthemeffblk.name, ".")!=0 && strcmp(sfxthemeffblk.name, "..")!=0) {
				set_theme_dir(sfxthemeffblk.name);
				break;
			}
	}

	//refresh master!
	get_servers_from_master();

	return true;
}

//send "client ready" message to server (when map load and/or download completes)
void gameclient_c::send_client_ready() {
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, 21);		// 21 = "client ready"
	client->send_message(lebuf, count);		// bem curtinha a mensagem mesmo...
}

// check if password has changed.
// if NO, do nothing
// if YES: send a new request to the master server
// V0.4.6 : always re-log client if ENTER pressed! (if player doesn't want to log again, should press ESC instead)
void gameclient_c::check_change_pass_command() {

	//password changed
	// V0.4.6: check removed
	//if (strcmp(editplayerpass, player_password))

	//player_password_set = false is a flag for the token retrieve thread
	//join with it
	if (player_password_set) {
		player_password_set = false;
		pthread_join( passthread, 0 );
	}

	//NO TOKEN, not anymore...
	player_token_set = false;

	//empty?
	if (editplayerpass[0] == 0) {

		strcpy(namestatus, "NO PASSWORD SET");
		namestatus_code = 0;
		player_password[0]=0;

	}
	else {
		//non-empty! copy stuff
		strcpy(namestatus, "STARTING LOGIN...");
		strcpy(player_password, editplayerpass);

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
			strcpy(namestatus, "SOCKET ERROR. RETRYING...");
			MS_SLEEP(3000);	//five secs
			continue;				//again...
		}

		//connect the nonblocking way
		nlConnect(sock, &master_address);

		//build query
		char blux[1024];
		if (player_token_new)
			sprintf(blux, "GET /servlet/fcecin.tk1/index.html?%s&new&name=%s&password=%s\n\n", TK1_VERSION_STRING, playername, player_password);
		else
			sprintf(blux, "GET /servlet/fcecin.tk1/index.html?%s&old&name=%s&password=%s\n\n", TK1_VERSION_STRING, playername, player_password);

		char querybuf[1024]; int qcount = 0;
		writeString(querybuf, qcount, blux);
		qcount--;	//take the zero out

		strcpy(namestatus, "SENDING LOGIN...");

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

		strcpy(namestatus, "WAITING RESPONSE...");

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
						strcpy(namestatus, "NO RESPONSE. RETRYING...");
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
				strcpy(namestatus, "ERROR. RETRYING...");
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

			strcpy(namestatus, "RECEIVED RESPONSE!");

			bool ok = false, wrongid = false, unavailable = false;
			int i;
			LOG1("RECV RESPONSE n = %i\n", n);
			for (i=0;i<n;i++) {

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
				strcpy(namestatus, "LOGGED (");
				strcat(namestatus, player_token);
				strcat(namestatus, ")");
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
				strcpy(namestatus, "ERROR: WRONG ID!");
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
				strcpy(namestatus, "SERVER UNAVAILABLE");
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
				strcpy(namestatus, "UNKNOWN ERROR!");
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
	writeByte(lebuf, count, 29);		//29 = file chunk ACK
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
	writeByte(lebuf, count, 27);		//27= request file
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
			bool ok = load_map(CLIENT_MAPS_DIR, r->name, &map);
			if (!ok)
				LOG1("AFTER DOWNLOAD: MAP '%s' NOT FOUND\n", r->name)
			else {
				LOG1("AFTER DOWNLOAD: MAP '%s' LOADED SUCESSFULLY!\n", r->name);

				//load ok!  (FIXME: tell server)
				//
				update_minimap_background();		// recalc minimap
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

	// v0.4.4 if not using TCP socket, add to the download queue
	if (server_no_tcp || no_tcp_download) {

		//ADD TO QUEUE
		client_udp_download(rune);
	}
	else {

		//start new thread wich will call client_download_thread below (yes sucks but works)
		pthread_t			downloader_thread;
		pthread_create(&downloader_thread, 0, thread_clientdownloader_f, (void *)(rune));
	}
}

//downloads a server file
void gameclient_c::client_download_thread(void *arg) {

	//get pointer to runes
	download_runes_t  *r = (download_runes_t*)arg;

	//open blocking, thank god, TCP connection
	nlEnable(NL_BLOCKING_IO);
	NLsocket sok = nlOpen(0, NL_RELIABLE);
	nlDisable(NL_BLOCKING_IO);

	if (sok == NL_INVALID) {		//d'oh!
		LOG("ERROR client_download_thread() can't open socket!");

		//disconnect client here
		//disconnect_command();

		//V0.4.4: add to UDP download queue and set to not use TCP anymore
		no_tcp_download = true;
		client_udp_download(r);

		return;
	}

	//connect to server IP :

	//v0.4.2 : custom TCP port
	int tcp_port = 24999 - (port - 25000);

	char addr_n_port[256];
	sprintf(addr_n_port, "%s:%i", address, tcp_port);	//v0.4.2 custom TCP port
	NLaddress addr;
	nlStringToAddr(addr_n_port, &addr);

	NLboolean ok = nlConnect(sok, &addr);
	if (ok == NL_FALSE) {

		LOG1("ERROR client_download_thread() can't connect to %s!", addr_n_port);

		//disconnect socket
		nlClose(sok);

		//disconnect_command();

		//V0.4.4: add to UDP download queue and set to not use TCP anymore
		no_tcp_download = true;
		client_udp_download(r);

		return;
	}

	//request file
	char lebuf[65536];
	int count = 0;
	writeByte(lebuf, count, 1);		//1 = request file download
	writeString(lebuf, count, r->type);	//filetype
	writeString(lebuf, count, r->name);	//filename
	NLint result = nlWrite(sok, lebuf, count);
	// check result of write
	if (result != count) {

		LOG("ERROR client_download_thread() send error (1)!");

		nlClose(sok);
		disconnect_command();		//FIXME make it better
		return;
	}

	//download file
	result = nlRead(sok, lebuf, 1);		// read response byte
	//check result
	if (result != 1) {

		LOG1("ERROR client_download_thread() error reading response byte; result = %i", result);

		nlClose(sok);
		disconnect_command();		//FIXME make it better
		return;
	}

	NLubyte ans;
	count = 0;
	readByte(lebuf, count, ans);

	//FIXME: deal with other answers
	if (ans == 2)    // 2 = file request ok, sending file
	{
		//read file CRC
		result = nlRead(sok, lebuf, 2);
		if (result != 2) {

			LOG1("ERROR client_download_thread() error reading crc; result = %i", result);

			nlClose(sok);
			disconnect_command();		//FIXME make it better
			return;
		}
		NLushort incrc;
		count = 0;
		readShort(lebuf, count, incrc);

		//read file SIZE
		result = nlRead(sok, lebuf, 4);
		if (result != 4) {

			LOG1("ERROR client_download_thread() error reading filesize; result = %i", result);

			nlClose(sok);
			disconnect_command();		//FIXME make it better
			return;
		}
		NLulong filesize;
		count = 0;
		readLong(lebuf, count, filesize);

		//read the file in 1 big chunk
		result = nlRead(sok, lebuf, filesize);
		if (result != (int)filesize) {

			LOG2("ERROR client_download_thread() error reading file; result = %i filesize = %lu", result, filesize);

			nlClose(sok);
			disconnect_command();		//FIXME make it better
			return;
		}

		//write to the file
		FILE *fw = fopen(r->dest, "wb");
//size_t fwrite(const void* ptr, size_t size, size_t nobj, FILE* stream);
//Writes to stream stream, nobj objects of size size from array ptr. Returns number of objects written.
		if (fw) {
			int amount = fwrite(lebuf, 1, filesize, fw);

			fclose(fw);

			LOG3("client_download_thread() file '%s' written %i of %lu\n", r->dest, amount, filesize);

			//file download complete
			download_file_complete(r);
		}
		else {
			//do something if can't write to the file (disconnect player/whatever)

			LOG1("ERROR client_download_thread() can't write output file!! (%s)", r->dest);

			nlClose(sok);
			disconnect_command();		//FIXME make it better
			return;
		}

		//disconnect
		int count = 0;
		writeByte(lebuf, count, 3);		//3 = bye
		nlWrite(sok, lebuf, count);
		//FIXME: check result

		//wait a bit
		MS_SLEEP(3000);

		//drop connection
		nlClose(sok);
	}
	else {
		//unknown answer code

		LOG1("ERROR client_download_thread() answer not 2, it's %i", ans);

		nlClose(sok);
		disconnect_command();		//FIXME make it better
		return;
	}

	//close connection

	//do stuff in the event of download complete

	// FIXME: if error with connection, quit.

}

//server tells client of current map / map change
// client must attempt to load map from "cmaps" dir
// if map file not there, or the CRC's don't match, ask to download the map from the server
void gameclient_c::server_map_command(const char *mapname, NLushort server_crc) {

	LOG1("CLIENT: server_map_command : '%s'", mapname);

	//try to load the map. will fail if not found
	bool ok = load_map(CLIENT_MAPS_DIR, mapname, &map);

	if (!ok)
		LOG1("MAP '%s' NOT FOUND\n", mapname)
	else if (map.crc != server_crc)
		LOG3("MAP '%s' FOUND BUT IT'S CRC %i DIFFERS FROM SERVER MAP CRC %i\n", mapname, map.crc, server_crc)
	else {
		LOG1("MAP '%s' LOADED SUCESSFULLY!\n", mapname);

		//load ok!  (FIXME: tell server)
		//
		update_minimap_background();  // recalc minimap
		map_ready = true;  // map ready to show
		send_client_ready();				//send "client ready" to server
	}

	// download map from server (ask file)
	if (!ok || map.crc != server_crc) {

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

void gameclient_c::next_sfx_theme() {
	char themepath[512];

	//no valid theme, just give up...
	if (!validtheme)
		return;

	bool round1 = true;

	make_sfx_theme_path(themepath, sfxthemedir);
	for (;;) {
		int result = al_findnext(&sfxthemeffblk);
		if (result) {
			//not found, go back to first ones...
			if (!round1) {
				validtheme = false;
				return;
			}
			round1 = false;
			make_sfx_theme_path(themepath, "*.*");
			result = al_findfirst(themepath, &sfxthemeffblk, FA_DIREC|FA_ARCH|FA_RDONLY);
			if (result) {
				validtheme = false;
				return;
			}
		}
		if ((sfxthemeffblk.attrib&FA_DIREC) && strcmp(sfxthemeffblk.name, ".")!=0 && strcmp(sfxthemeffblk.name, "..")!=0) {
			set_theme_dir(sfxthemeffblk.name);
			break;
		}
	}
}

void gameclient_c::make_sfx_theme_path(char *themepath, char *themedir) {

	char soundname[1024];

	strcpy(soundname, "sound");  //sound/
	put_backslash(soundname);
	strcat(soundname, themedir);  //theme dir name

	char dest[1024];
	append_filename(dest, wheregamedir, soundname, WHERE_PATH_SIZE);

	strcpy(themepath, dest);

	LOG1("make sfx theme path = '%s'\n", themepath);
}

void gameclient_c::set_theme_dir(char *dirname) {

	if (dirname)
		strcpy(sfxthemedir, dirname);

	validtheme = true;

	unload_samples();		//unload old (if any)

	load_samples();			//load new

	// load sfx theme description
	//
	char soundname[256];
	strcpy(soundname, "sound");

	put_backslash(soundname);
	strcat(soundname, sfxthemedir);

	put_backslash(soundname);
	strcat(soundname, "theme.txt");

	char dest[WHERE_PATH_SIZE];
	append_filename(dest, wheregamedir, soundname, WHERE_PATH_SIZE);

	FILE *theme = fopen(dest, "r");
	if (theme) {
		if (fgets(sfxthemename, 256, theme)) {
			sfxthemename[strlen(sfxthemename)-1] =0;
		}
		else
			strcpy(sfxthemename, "(unnamed theme)");
		fclose(theme);
	}

	//play a sample
	sound( rand() % NUM_OF_SAMPLES );

}

//append the correct path
SAMPLE* gameclient_c::load_outgun_sample(char *fname, int slot, bool try_redirect, bool reverse) {

	//soundname: add "sound/" to the filename
	char soundname[256];
	strcpy(soundname, "sound");

	//additional: sfx theme dir name
	put_backslash(soundname);
	strcat(soundname, sfxthemedir);

	put_backslash(soundname);
	strcat(soundname, fname);
	strcat(soundname, ".wav");

	//add soundname to where game dir
	char dest[WHERE_PATH_SIZE];
	append_filename(dest, wheregamedir, soundname, WHERE_PATH_SIZE);

	//try load
	SAMPLE* ret = sample[slot] = load_sample(dest);

	//sample must be played in reverse?
	sample_reverse[slot] = reverse;

	LOG4("load_sample[%i]: '%s' = %p  rev = %i\n", slot, dest, ret, sample_reverse[slot]);

	//V0.3.10: if not found, look for .txt redirect
	if (try_redirect)	// don't go into endless loop
	if (ret == 0) {

		//txt filename
		strcpy(soundname, "sound");
		put_backslash(soundname);
		strcat(soundname, sfxthemedir);
		put_backslash(soundname);
		strcat(soundname, fname);
		strcat(soundname, ".txt");
		append_filename(dest, wheregamedir, soundname, WHERE_PATH_SIZE);

		FILE *f = fopen(dest, "r");
		if (f) {
			char redirwavname[256];
			fscanf(f, "%s", redirwavname);
			bool is_reversed = false;

			// versao 0.4.1 : EARLY OPTIMIZATION IS THE ROOT OF ALL EVIL
			// versao 0.4.1 : EARLY OPTIMIZATION IS THE ROOT OF ALL EVIL
			// versao 0.4.1 : EARLY OPTIMIZATION IS THE ROOT OF ALL EVIL
			//if (!strcmp("REVERSE", redirwavname)) {
			//	is_reversed = true;	//want reversed
			//	fscanf(f, "%s", redirwavname);		//scan again the name of the wav
			//}

			fclose(f);

			//retry once ("false": don't try redirect again if fails)
			return load_outgun_sample(redirwavname, slot, false, is_reversed);
		}
	}

	return ret;
}

//sample try loads
void gameclient_c::load_samples() {

	if (!sound_inited) return;
	load_outgun_sample("fire", SAMPLE_FIRE);
	load_outgun_sample("hit", SAMPLE_HIT);
	load_outgun_sample("wallhit", SAMPLE_WALLHIT);	//new 0.3.9
	load_outgun_sample("qwallhit", SAMPLE_QUADWALLHIT);	//new 0.3.9

	load_outgun_sample("getdb", SAMPLE_GETDEATHBRINGER);	// new! v0.3.9 -- get deathbringer powerup (voz sinistra)
	load_outgun_sample("usedb", SAMPLE_USEDEATHBRINGER);	// new! v0.3.9 -- use deathbringer powerup (carrier dies) ("GRRRAAWWKKLLLL!!")
	load_outgun_sample("hitdb", SAMPLE_HITDEATHBRINGER);	// new! v0.3.9 -- target is hit by the deathbringer ("PWRRLLW!")
	load_outgun_sample("diedb", SAMPLE_DIEDEATHBRINGER);	// new! v0.3.9 -- target dies by the deathbringer		("HaHaHaHa!")

	load_outgun_sample("death1", SAMPLE_DEATH);
	load_outgun_sample("death2", SAMPLE_DEATH_2);
	//sample[SAMPLE_RESPAWN] = load_outgun_sample("respawn");
	load_outgun_sample("entergam", SAMPLE_ENTERGAME);
	load_outgun_sample("leftgam", SAMPLE_LEFTGAME);
	load_outgun_sample("chanteam", SAMPLE_CHANGETEAM);
	load_outgun_sample("talk", SAMPLE_TALK);
	load_outgun_sample("wabounce", SAMPLE_WALLBOUNCE);

	load_outgun_sample("weaponup", SAMPLE_WEAPON_UP);  //new
	load_outgun_sample("megaheal", SAMPLE_MEGAHEALTH); // new
	load_outgun_sample("shieldp", SAMPLE_SHIELD_PICKUP);
	load_outgun_sample("shieldd", SAMPLE_SHIELD_DAMAGE);
	load_outgun_sample("shieldl", SAMPLE_SHIELD_LOST);
	load_outgun_sample("speedon", SAMPLE_BOOTS_ON);
	load_outgun_sample("speedoff", SAMPLE_BOOTS_OFF);
	load_outgun_sample("quadon", SAMPLE_QUAD_ON);
	load_outgun_sample("quadfire", SAMPLE_QUAD_FIRE);
	load_outgun_sample("quadoff", SAMPLE_QUAD_OFF);
	load_outgun_sample("helmon", SAMPLE_HELM_ON);
	load_outgun_sample("helmoff", SAMPLE_HELM_OFF);

	load_outgun_sample("got", SAMPLE_CTF_GOT);
	load_outgun_sample("lost", SAMPLE_CTF_LOST);
	load_outgun_sample("return", SAMPLE_CTF_RETURN);
	load_outgun_sample("capture", SAMPLE_CTF_CAPTURE);
	load_outgun_sample("gameover", SAMPLE_CTF_GAMEOVER);
}

//unload samples
void gameclient_c::unload_samples() {
	if (!sound_inited) return;
	for (int i=0;i<NUM_OF_SAMPLES;i++)
		if (sample[i])
			destroy_sample(sample[i]);
}

//play sample
void gameclient_c::sound(int s) {
	if (sound_enabled)
	if (sample[s]) {

		//kill any voice playing that sample
		stop_sample(sample[s]);

		// versao 0.4.1 : EARLY OPTIMIZATION IS THE ROOT OF ALL EVIL
		// versao 0.4.1 : EARLY OPTIMIZATION IS THE ROOT OF ALL EVIL
		// versao 0.4.1 : EARLY OPTIMIZATION IS THE ROOT OF ALL EVIL
		/*
		if (sample_reverse[s]) {

			//play_sample(sample[s], 255, 127, 1000, false);		//reversed play

			//allocate new voice
			int v = allocate_voice(sample[s]);

			//set up to play backwards
			voice_set_playmode(v, PLAYMODE_BACKWARD);


			//go!
			voice_start(v);
		}
		else
		*/
			//regular play
			play_sample(sample[s], 255, 127, 1000, false);		//regular play
	}
}

//clear clientside fx's
void gameclient_c::clear_fx() {
	for (int i=0;i<MAX_CLIENTFX;i++)
		cfx[i].used = false;
}

//find new clientside fx
int gameclient_c::get_new_cfx() {
	for (int i=0;i<MAX_CLIENTFX;i++)
	if (!cfx[i].used)
		return i;
	//print_message("overflow");
	return rand() % MAX_CLIENTFX;	//overwrite algum sorteado....
}

//create wall explosion fx
void gameclient_c::cfx_create_wallexplo(int x, int y, int px, int py) {

	int f = get_new_cfx();

	cfx[f].used = true;
	cfx[f].type = 2;		// WALL EXPLOSION
	cfx[f].x = x;
	cfx[f].y = y;
	cfx[f].time = get_time();
	cfx[f].px = px;
	cfx[f].py = py;

	//sound
	sound(SAMPLE_WALLHIT);
}

//create quad wall explosion fx
void gameclient_c::cfx_create_quadwallexplo(int x, int y, int px, int py) {

	int f = get_new_cfx();

	cfx[f].used = true;
	cfx[f].type = 3;		// QUAD WALL EXPLOSION
	cfx[f].x = x;
	cfx[f].y = y;
	cfx[f].time = get_time();
	cfx[f].px = px;
	cfx[f].py = py;

	//sound
	sound(SAMPLE_QUADWALLHIT);
}

//create deathbringer explosion fx
void gameclient_c::cfx_create_deathbringer(int owner, double start_time, int x, int y, int px, int py) {

	int f = get_new_cfx();

	cfx[f].used = true;
	cfx[f].owner = owner;		//deathbringer owner
	cfx[f].type = 4;		// DEATHBRINGER EXPLOSION
	cfx[f].x = x;
	cfx[f].y = y;
	cfx[f].time = start_time;
	cfx[f].px = px;
	cfx[f].py = py;

	//sound
	sound(SAMPLE_USEDEATHBRINGER);
}

//create deathbringer carrier trail fx
void gameclient_c::cfx_create_deathcarrier(int x, int y, int px, int py, int team) {

	int f = get_new_cfx();

	cfx[f].used = true;
	cfx[f].type = 5;	//death carrier cloud fx
	cfx[f].x = x;
	cfx[f].y = y;
	cfx[f].px = px;
	cfx[f].py = py;
	cfx[f].time = get_time();

	//owner: set color
	int r = rand() %100;
	if (team) {
		if (r < 50)
			cfx[f].col1 = makecol(0,0,0xff);
		else if (r < 75)
			cfx[f].col1 = makecol(0,0xff,0);
		else
			cfx[f].col1 = 0;
	} else {
		if (r < 50)
			cfx[f].col1 = makecol(0xff,0,0);
		else if (r < 75)
			cfx[f].col1 = makecol(0,0xff,0);
		else
			cfx[f].col1 = 0;
	}

	//JUST BLACK
	cfx[f].col1 = 0;
}

//create explosion fx
void gameclient_c::cfx_create_gunexplo(int x, int y, int px, int py) {

	int f = get_new_cfx();

	cfx[f].used = true;
	cfx[f].type = 0;		// GUN EXPLOSION
	cfx[f].x = x;
	cfx[f].y = y;
	cfx[f].time = get_time();
	cfx[f].px = px;
	cfx[f].py = py;

	//sound
	sound(SAMPLE_HIT);
}

//create speed bolinha fx
void gameclient_c::cfx_create_speedfx(int x, int y, int px, int py, int col1, int col2, int gundir) {

	int f = get_new_cfx();

	cfx[f].used = true;
	cfx[f].type = 1;	//speed fx
	cfx[f].x = x;
	cfx[f].y = y;
	cfx[f].px = px;
	cfx[f].py = py;
	cfx[f].time = get_time();

	cfx[f].col1 = col1;
	cfx[f].col2 = col2;
	cfx[f].gundir = gundir;
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
			if (player[i].used)
			if (!scoreused[i])		// ainda nao usado
			if (player[i].frags > maxfrag) {
				//achou maior
				maxfrag = player[i].frags;
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

//calc the game frame
void gameclient_c::calc_game_frame() {

	int i;

	pthread_mutex_lock( &frame_mutex );

	//frame was skipped
	if (fx.skipped) {
		fd.skipped = true;
		pthread_mutex_unlock( &frame_mutex );
		return;
	}

	//make fancy extrapolation from the most recent frame from the server

	if (fx.frame > 0)	// valid?
	{
		//calcula framedelta (d)
		fd.time = get_time();
		double d = (fd.time - fx.time) / 0.1;

		//just to draw
		fd.frame = fx.frame + d;

		//player extrapolation
		//
		hero_t  *h;
		for (i=0;i<maxplayers;i++)

		if (player[i].onscreen) {	// nao eh suficiente usar platyer[i].USED !!!
			fd.hero[i] = fx.hero[i];

			if (player[i].x<0 || player[i].y<0 || player[i].x>=map.w || player[i].y>=map.h) continue;	//#fix: remove this and track why these are given sometimes
			const Room& room = map.room[player[i].x][player[i].y];
			bool carryFlag = fx.flag[1-(i/TSIZE)].carried && fx.flag[1-(i/TSIZE)].carrier == i;

			h = &fd.hero[i];

			//delta counter
			double dc, f;
			dc = d;

			while (dc > 0) {
				//calc amount of movement
				f = dc;
				if (f > 1.0)
					f = 1.0;

				//dec dc
				dc -= 1.0;

				//run physics
				#ifdef SV_SERVER_PHYSICS
				if (NR_applyPhysics(&fd.hero[i], room, f, player[i].item_speed, carryFlag, player[i].deathbringer_affected)) {
					//player bounced: play bounce sample if minimum time elapsed
					if (get_time() > player[i].wall_sound_time) {
						player[i].wall_sound_time = get_time() + 0.2;
						sound(SAMPLE_WALLBOUNCE);
					}
				}
				#else	// SV_SERVER_PHYSICS
				if (applyDefaultPhysics(&fd.hero[i], room, f, player[i].item_speed, carryFlag, player[i].deathbringer_affected)) {
					//player bounced: play bounce sample if minimum time elapsed
					if (get_time() > player[i].wall_sound_time) {
						player[i].wall_sound_time = get_time() + 0.2;
						sound(SAMPLE_WALLBOUNCE);
					}
				}
				#endif	// SV_SERVER_PHYSICS else
			}
		}

		//rocket "interpolation"?
		//
		for (i=0;i<MAX_ROCKETS;i++)
		if (fx.rock[i].owner != -1)
		{
			rocket_c *rd = &(fd.rock[i]);
			rocket_c *rx = &(fx.rock[i]);

			//still drawing only - update pos
			if (!rx->dontdraw) {
				//find pos for draw
				// pos = startpos + sin/cos deg * timetravel * speed
				rd->x = (int)( rx->x + (fd.frame - rx->cl_time) * cos(rx->deg) * ROCKET_SPEED );
				rd->y = (int)( rx->y + (fd.frame - rx->cl_time) * sin(rx->deg) * ROCKET_SPEED );
			}

			//SPECIAL CASE: check if rocket just died
			if ((rx->hit_time > 0) && (get_time() > rx->hit_time))
			{
				rx->owner = -1;		// nao rola mais
				rd->x = rx->hitx;	// hit coords
				rd->y = rx->hity;

				//quad-hit wall?
				/*
				FIXED 0.3.9 (2) : wall collision+explosion is totally clientside

				if (rx->hit_target == 253) {

					//spawn clientside fx
					cfx_create_quadwallexplo((int)rd->x, ((int)rd->y) - 10, rx->px, rx->py);

				}
				//hit wall?
				else if (rx->hit_target == 254) {

					//spawn clientside fx
					cfx_create_wallexplo((int)rd->x, ((int)rd->y) - 10, rx->px, rx->py);

				}
				*/
				//just removing rocket == NOP
				//else
				if (rx->hit_target == 255) {
				}
				//hit player
				else {

					// blink player if not hit shield (252)
					if (rx->hit_target < 250)
						player[rx->hit_target].hitfx = get_time() + 0.3;

					//spawn clientside fx
					cfx_create_gunexplo((int)rd->x, ((int)rd->y) - 10, rx->px, rx->py);
				}
			}
			//else if still drawing check collisions/out of screen
			else if (!rx->dontdraw) {

				//0.3.9: check rocket hit a wall (clientside) if not vanished already
				#ifdef SV_SERVER_PHYSICS
				if (map.fall_on_wall(rx->px, rx->py, (int)rd->x-2, (int)rd->y-SV_SHIFTY-2, (int)rd->x+2, (int)rd->y-SV_SHIFTY+2)) {
				#else
				if (map.fall_on_wall(rx->px, rx->py, (int)rd->x, (int)rd->y-SV_SHIFTY, (int)rd->x, (int)rd->y-SV_SHIFTY)) {
				#endif
					//probably hit wall
					rx->dontdraw = true;
					rx->clremove = get_time() + 5.0;
					// IF the rocket is in the same room of "me" player
					if (rx->px == player[me].x)
					if (rx->py == player[me].y) {
						//then SPAWN a client-side hit-wall FX for the rocket
						if (player[rx->owner].item_quad)
							cfx_create_quadwallexplo((int)rd->x, ((int)rd->y) - 10, rx->px, rx->py);	//quad hit wall
						else
							cfx_create_wallexplo((int)rd->x, ((int)rd->y) - 10, rx->px, rx->py);		//normal hit wall
					}
				}
				// check out of screen (erase)
				else if ((rd->x < 0) || (rd->y < 5) || (rd->x > plw) || (rd->y > plh)) {
					rx->dontdraw = true;
					rx->clremove = get_time() + 1.0;	//enough...
				}
			}

			// check rocket expired
			if (rx->dontdraw)
			if (get_time() <= rx->clremove)
				rx->owner = -1;	// erase from clientside simulation
		}
	}

	pthread_mutex_unlock( &frame_mutex );
}

//draw a flag  t=team 0/1   x,y: coord relative to playarea
void gameclient_c::draw_flag_at(BITMAP *drawbuf, int t, int x, int y) {
	//draw shadow
	ellipsefill(drawbuf,
		plx + x,
		ply + y,
		12, 3, col[COLSHADOW]
	);
	//draw mastro
	rectfill(drawbuf,
		plx + x - 3,
		ply + y - 40,
		plx + x + 3,
		ply + y,
		col[COLYELLOW]
	);
	//draw bandeira
	rectfill(drawbuf,
		plx + x,
		ply + y - 38,
		plx + x + 20,
		ply + y - 20,
		teamcol[t]
	);
}

//draw minimap flag
void gameclient_c::draw_mini_flag(BITMAP *drawbuf, int whatteam) {

	int f = whatteam;

		double px, py;
		px = ((double)fx.flag[f].pos.px * (double)plw + fx.flag[f].pos.x) / ((double)plw * map.w);
		py = ((double)fx.flag[f].pos.py * (double)plh + fx.flag[f].pos.y) / ((double)plh * map.h);
		int pix = mmx + 21 + ((int)(px*98));
		int piy = mmy + 01 + ((int)(py*98));

		//draw mastro
		rectfill(drawbuf, pix, piy-5, pix, piy, col[COLYELLOW]);
		//draw bandeira
		rectfill(drawbuf, pix+1,piy-5,pix+5,piy-2,teamcol[f]);
}

//update the minimap background
void gameclient_c::update_minimap_background() {
	LOG2("update_minimap map.w = %i map.h = %i\n", map.w, map.h);
	map.draw_minimap(minibg);
}

//draws a player object
void gameclient_c::draw_player(BITMAP *drawbuf, int x, int y, int gundir, int pc1, int pc2, int alpha) {

	//draw the gun (direction facing)
	int xg, yg;
	switch (gundir) {
		case 0: xg= 40; yg=  0; break;
		case 1: xg= 28; yg= 28; break;
		case 2: xg=  0; yg= 40; break;
		case 3: xg=-28; yg= 28; break;
		case 4: xg=-40; yg=  0; break;
		case 5: xg=-28; yg=-28; break;
		case 6: xg=  0; yg=-40; break;
		case 7: xg= 28; yg=-28; break;
		default: xg= 0; yg=  0; break;
	}
	xg = (int)( ((double)xg) * 0.7);
	yg = (int)( ((double)yg) * 0.7);

	xg += x;
	yg += y - 15;

	if (alpha < 255) {
		set_trans_blender(0,0,0,alpha);
		drawing_mode(DRAW_MODE_TRANS, 0,0,0);
	}

	//desenha arma antes se dir 5,6,7
	if (gundir >= 5) {
		line(drawbuf, 0+plx + x, 0+ply + y - 15, 0+plx + xg, 0+ply + yg, pc1);
		line(drawbuf, 1+plx + x, 0+ply + y - 15, 1+plx + xg, 0+ply + yg, pc1);
		line(drawbuf, 1+plx + x, 1+ply + y - 15, 1+plx + xg, 1+ply + yg, pc1);
	}

	// outer color: team color
	circlefill(drawbuf, plx+x, ply+y-15, 15, pc1);
	// inner color: self color
	circlefill(drawbuf, plx+x, ply+y-15, 10, pc2);

	//desenha arma depois se dir 0,1,2,3,4
	if (gundir < 5) {
		line(drawbuf, 0+plx + x, 0+ply + y - 15, 0+plx + xg, 0+ply + yg, pc1);
		line(drawbuf, 1+plx + x, 0+ply + y - 15, 1+plx + xg, 0+ply + yg, pc1);
		line(drawbuf, 1+plx + x, 1+ply + y - 15, 1+plx + xg, 1+ply + yg, pc1);
	}

	if (alpha < 255)
		solid_mode();
}

//draw the whole game screen
void gameclient_c::draw_game_frame(BITMAP *drawbuf) {

	//static int HOWMANY = 0;
	//rectfill(drawbuf, 20, 20, 100, 40, makecol(rand(),rand(),rand()));
	//textprintf(drawbuf,font, 25,25,makecol(rand(),rand(),rand()),"HOW=%i",++HOWMANY);

	// erase old chat messages (this shouldn't be here really but wtf..)
	//
	if (chaterasetime < get_time())
		erase_first_message();

	//do not draw anything if "me" not known
	if (me < 0)
	if (me >= maxplayers) {
		clear_to_color(drawbuf, col[COLYELLOW]);
		return;
	}

	//lock frame mutex
	//LOG1("locking HOW=%i",HOWMANY);
	pthread_mutex_lock( &frame_mutex );
	//LOG1("locked! HOW=%i",HOWMANY);

	//SUPER USELESS MODE   //if (rand() % 10 < 2) SEEP(30);
	//clear_to_color(drawbuf, makecol(rand(),0,0));	// clear buffer

	// game screen background
	//
	clear_to_color(drawbuf, col[COLSHADOW]);

	// hiding stuff?
	// v0.4.1 : hide stuff if frame skipped
	bool hide_game = ((!map_ready) || (gameover_plaque != NEXTMAP_NONE) || (fx.skipped));

	// the PLAY AREA: border, walls and pits
	//
	if (hide_game) {

		//draw THE PLAQUE
		rectfill(drawbuf, plx, ply, plx + plw, ply + plh, 0);

		//DEBUG FIXME REMOVEME:
		//if (fx.skipped)
		//	textprintf_centre(drawbuf, font, plx+plw/2, ply+plh/2 - 80, col[COLDARKGRAY], "fx.skipped");

		//qual mensagem
		if ((gameover_plaque == NEXTMAP_CAPTURE_LIMIT) || (gameover_plaque == NEXTMAP_VOTE_EXIT)) {
			if (red_final_score > blue_final_score) {
				textprintf_centre(drawbuf, font, plx+plw/2, ply+plh/2 - 40, col[COLLRED], "RED TEAM WINS");
				textprintf_centre(drawbuf, font, plx+plw/2, ply+plh/2 - 20, col[COLLRED], "SCORE: %i x %i", red_final_score, blue_final_score);
			}
			else if (blue_final_score > red_final_score) {
				textprintf_centre(drawbuf, font, plx+plw/2, ply+plh/2 - 40, col[COLLBLUE], "BLUE TEAM WINS");
				textprintf_centre(drawbuf, font, plx+plw/2, ply+plh/2 - 20, col[COLLBLUE], "SCORE: %i x %i", blue_final_score, red_final_score);
			}
			else {
				textprintf_centre(drawbuf, font, plx+plw/2, ply+plh/2 - 40, col[COLMENUGRAY], "GAME TIED");
				textprintf_centre(drawbuf, font, plx+plw/2, ply+plh/2 - 20, col[COLMENUGRAY], "SCORE: %i x %i", blue_final_score, red_final_score);
			}
		}
		else {
			textprintf_centre(drawbuf, font, plx+plw/2, ply+plh/2, col[COLGREEN], "Connecting...");
		}

		if (map_ready) {
			textprintf_centre(drawbuf, font, plx+plw/2, ply+plh/2 + 20, col[COLGREEN], "Waiting game start - next map is:");
			textprintf_centre(drawbuf, font, plx+plw/2, ply+plh/2 + 50, col[COLORA], "%s", map.title.c_str());
		}
		else
			textprintf_centre(drawbuf, font, plx+plw/2, ply+plh/2 + 20, col[COLGREEN], "Loading map: %lu bytes", fdp);
	}
	else {

		// map ground
		//
		//0.4.0 check if base ground, then paint differently
		/*
		if ((player[me].x == map.tinfo[0].flag.px) && (player[me].y == map.tinfo[0].flag.py)) {
			rectfill(drawbuf, plx, ply, plx + plw, ply + plh, col[COLBRED]);
		}
		else if ((player[me].x == map.tinfo[1].flag.px) && (player[me].y == map.tinfo[1].flag.py)) {
			rectfill(drawbuf, plx, ply, plx + plw, ply + plh, col[COLBBLUE]);
		}
		//regular ground
		else
		*/
		rectfill(drawbuf, plx, ply, plx + plw, ply + plh, col[COLGROUND]);

		// place of flag
		for (int team = 0; team < 2; team++)
			if (player[me].x == map.tinfo[team].flag.px && player[me].y == map.tinfo[team].flag.py) {
				check_flagpos_marks();
				int flag_x = map.tinfo[team].flag.x;
				int flag_y = map.tinfo[team].flag.y;
				int x1 = max(0, CL_FLAGPOS_RAD - flag_x);
				int y1 = max(0, CL_FLAGPOS_RAD - flag_y);
				int x2 = min(2 * CL_FLAGPOS_RAD, plw - flag_x + CL_FLAGPOS_RAD + 1);
				int y2 = min(2 * CL_FLAGPOS_RAD, plh - flag_y + CL_FLAGPOS_RAD + 1);
				blit(flagpos_buf[team], drawbuf, x1, y1,
					plx + flag_x - CL_FLAGPOS_RAD + x1, ply + flag_y - CL_FLAGPOS_RAD + y1, x2 - x1, y2 - y1);
			}

		// map walls
		if (player[me].x >= 0 && player[me].y >= 0 && player[me].x < map.w && player[me].y < map.h)
			map.room[player[me].x][player[me].y].draw(drawbuf, plx, ply, 1., 1., col[COLWALL]);
	}

	// frame is valid?
	if (!hide_game)		// do not draw if map not set yet
	if (fd.frame >= 0) {

		int i;

		// FIXME: y-ordering of draw not maintained
		// draw any item pickups
		//
		if (me >= 0)
		for (i=0;i<MAX_PICKUPS;i++)
		if (fx.item[i].kind > 0)		// used pickups
		if (fx.item[i].kind != 255)	//not respawning
		if (fx.item[i].px == player[me].x)	// on my screen
		if (fx.item[i].py == player[me].y)
		{
			pickup_c *it = &fx.item[i];

			ellipsefill(drawbuf, plx + it->x, ply + it->y + 12, 12, 3, col[COLSHADOW]);

			//shield
			if (it->kind == 1) {

				ellipse(drawbuf, plx + it->x, ply + it->y, 14+rand()%3, 14+rand()%3, makecol(rand(),rand(),rand()));
				ellipse(drawbuf, plx + it->x, ply + it->y, 14+rand()%5, 14+rand()%5, makecol(rand(),rand(),rand()));
				ellipse(drawbuf, plx + it->x, ply + it->y, 14+rand()%9, 14+rand()%9, makecol(rand(),rand(),rand()));

				circlefill(drawbuf, plx + it->x, ply + it->y, 12, col[COLGREEN]);

			}
			//boots
			else if (it->kind == 2) {

				circlefill(drawbuf, plx + it->x + rand()%6-3, ply + it->y + rand()%6-3, 12, col[COLDARKORA]);
				circlefill(drawbuf, plx + it->x + rand()%8-4, ply + it->y + rand()%8-4, 12, col[COLORA]);
				circlefill(drawbuf, plx + it->x + rand()%12-6, ply + it->y + rand()%12-6, 12, col[COLYELLOW]);
			}
			//helm
			else if (it->kind == 3) {

				drawing_mode(DRAW_MODE_TRANS, 0,0,0);
				int alpha = ((int)(get_time() * 600.0)) % 400;
				if (alpha > 200)
					alpha = 400 - alpha;
				set_trans_blender(0,0,0,55+alpha);
				circlefill(drawbuf, plx + it->x, ply + it->y, 12, col[COLMAG]);
				solid_mode();
			}
			//instant kill shots
			else if (it->kind == 4) {

				if (((int)(get_time() * 30)) % 2)
					circlefill(drawbuf, plx + it->x, ply + it->y, 13, col[COLWHITE]);
				else
					circlefill(drawbuf, plx + it->x, ply + it->y, 11, col[COLCYAN]);
			}
			//weapon upgrade
			else if (it->kind == 5) {

				//graus de rotacao
				for (int b=0;b<4;b++) {
					//deg: 0..360
					double deg = ((int) (get_time() * 1000.0)) % 1000;		//thousand ticker
					deg = deg / 1000.0;		//0..1 inteiro
					deg = deg * 6.2832;		// 0..1 vezes 2 pi (1 volta)
					deg = deg + 1.5708 * b; //mais b's (90 grauses)

					//pos
					double dx = 10 * cos(deg);
					double dy = 10 * sin(deg);

					//draw a ball
					switch (b) {
					case 0: circlefill(drawbuf, plx + it->x + (int)dx, ply + it->y + (int)dy, 4, col[COLGREEN]); break;
					case 1: circlefill(drawbuf, plx + it->x + (int)dx, ply + it->y + (int)dy, 4, col[COLBLUE]); break;
					case 2: circlefill(drawbuf, plx + it->x + (int)dx, ply + it->y + (int)dy, 4, col[COLRED]); break;
					case 3: circlefill(drawbuf, plx + it->x + (int)dx, ply + it->y + (int)dy, 4, col[COLYELLOW]); break;
					}
				}
			}
			//megahealth
			else if (it->kind == 6) {

				//caixa de saude pulsante
				int varia = ((int)(get_time() * 15)) % 10;

				if (varia > 5)
					varia = 10 - varia;

				int itemsize = 11 + varia;
				int crossize = 8 + varia;
				int crosslar = 3;//aria/2;

				// health box black border
				rectfill(drawbuf, plx + it->x - itemsize - 2, ply + it->y - itemsize - 2, plx + it->x + itemsize + 2, ply + it->y + itemsize + 2, 0);

				// health box
				rectfill(drawbuf, plx + it->x - itemsize, ply + it->y - itemsize, plx + it->x + itemsize, ply + it->y + itemsize, col[COLWHITE]);

				// red cross
				rectfill(drawbuf, plx + it->x - crossize, ply + it->y - crosslar, plx + it->x + crossize, ply + it->y + crosslar, col[COLRED]);
				rectfill(drawbuf, plx + it->x - crosslar, ply + it->y - crossize, plx + it->x + crosslar, ply + it->y + crossize, col[COLRED]);
			}
			//deathbringer
			else if (it->kind == 7) {

				//bola preta
				circlefill(drawbuf, plx + it->x, ply + it->y, 12, makecol(0x22,0x33,0x22));

				//smoke da bola preta
				cfx_create_deathcarrier(it->x + rand()%30-15, it->y + rand()%30-5, it->px, it->py, 0);
			}
		}

		// draw clientside fx -- efeitos ATRAS das coisas
		//
		if (me >= 0)	// where am I?
		for (i=0;i<MAX_CLIENTFX;i++)
		if (cfx[i].used)	//fx used?
		if (cfx[i].px == player[me].x)	//on same screen?
		if (cfx[i].py == player[me].y)
		{
			double tim = get_time();

			//speed rastro
			if (cfx[i].type == 1) {

				double delta = tim - cfx[i].time;
				if (delta > 0.3) {
					cfx[i].used = false;
				}
				else {
					int alpha = 90 - ((int)(delta * 300.0));
					draw_player(drawbuf, cfx[i].x, cfx[i].y, cfx[i].gundir, cfx[i].col1, cfx[i].col2, alpha);
				}
			}

		}

		// FIXME: y-ordering of draw not maintained
		// draw any dropped flags (use fx since flags don't move)
		//
		for (int t=0;t<2;t++)
			if (me >= 0)
			if (fx.flag[t].carried == false)	// not carried == dropped
			if (fx.flag[t].pos.px == player[me].x)  // on same screen than me
			if (fx.flag[t].pos.py == player[me].y)  // on same screen than me
			{
				draw_flag_at(drawbuf, t, fx.flag[t].pos.x, fx.flag[t].pos.y);
			}

		// FIXME: y-ordering of draw not maintained
		// draw any rockets
		if (me >= 0)
		for (i=0;i<MAX_ROCKETS;i++)
		if (fx.rock[i].owner != -1)		// valid
		if (fx.rock[i].dontdraw == false)	// DO draw?
		if (fx.rock[i].px == player[me].x)	//in same screen
		if (fx.rock[i].py == player[me].y)
		{
			rocket_c *r = &(fd.rock[i]);

			//QUAD ROCKET!
			if (player[ fx.rock[i].owner ].item_quad) {
				//draw rocket shadow
				ellipsefill(drawbuf, plx + (int)r->x, ply + (int)r->y, 6, 3, col[COLSHADOW]);
				//draw the rocket
				if (((int)(get_time() * 30)) % 2)
					circlefill(drawbuf, plx + (int)r->x, ply + (int)r->y - 15, 6, col[COLWHITE]);	//y-12?
				else
					circlefill(drawbuf, plx + (int)r->x, ply + (int)r->y - 15, 4, teamlcol[fx.rock[i].team]); //y-12??
			}
			else {
				//draw rocket shadow
				ellipsefill(drawbuf, plx + (int)r->x, ply + (int)r->y, 4, 2, col[COLSHADOW]);
				//draw the rocket
				circlefill(drawbuf, plx + (int)r->x, ply + (int)r->y - 15, 4, teamcol[fx.rock[i].team]); //y-10??
			}
		}

		// sort order of drawing of the players
		//
		for (i=0;i<maxplayers;i++) {
			player[i].drawused = 0;
			player[i].drawptr = -1;
		}

		double miny;
		int minyid;

		for (i=0;i<maxplayers;i++) {
			minyid = -1;
			miny = 999999;

			for (int j=0;j<maxplayers;j++)
			if (player[j].used)	{
				if (player[j].drawused == 0)
				if (fd.hero[j].y < miny) {
					miny = fd.hero[j].y;
					minyid = j;
				}
			}

			if (minyid == -1)
				break;

			player[minyid].drawused = 1;
			player[i].drawptr = minyid;
		}

		// the PLAY AREA: the players!
		//
		for (int k=0;k<maxplayers;k++) {

			//HACK REMENDEX: predict item_helm
			if (player[i].item_helm > 0) {
				int hspd = (int) ((fd.time - fx.time) * 100.0);
				player[i].item_helm = player[i].item_helm - hspd;
				if (player[i].item_helm < 1)
					player[i].item_helm = 1;
			}

			//indirection: draw in y-order
			int i;
			i = player[k].drawptr;

			if (i >= 0)
			if (player[i].onscreen)		// draw only players on my screen
			{
				//calcula alfa do player
				//
				int alpha = 255;
				if (player[i].item_helm > 0) {
					drawing_mode(DRAW_MODE_TRANS, 0,0,0);
					alpha = player[i].item_helm;
					if (me >= 0)
					if (i/TSIZE == me/TSIZE)	// teammate or myself
					if (alpha < MIN_ALPHA_FRIENDS) alpha = MIN_ALPHA_FRIENDS;
					set_trans_blender(0,0,0,alpha);
				}

				// the player's shadow: showing last valid position
				ellipsefill(drawbuf, plx + (int)fx.hero[i].x, ply + (int)fx.hero[i].y, 15, 3, col[COLSHADOW]);

				if (player[i].item_helm > 0)
					solid_mode();

				// DRAW FLAG IF PLAYER IS CARRIER OF A FLAG
				for (int t=0; t<2; t++)
					if (fx.flag[t].carried == true)
					if (fx.flag[t].carrier == i)
					{
						draw_flag_at(drawbuf, t, (int)fd.hero[i].x, (int)fd.hero[i].y);
					}

				// se player morto, desenha poca de sangue
				if (player[i].dead) {

					// FIXME: draw different deathbringer death (green stuff?)

					// virou sorvete!
					if ((player[i].frags >= 10) && (player[i].frags % 10 == 0)) {
						ellipsefill(drawbuf, plx + (int)fx.hero[i].x, ply + (int)fx.hero[i].y - 15, 6, 15, col[COLORA]);
						circlefill(drawbuf, plx + (int)fx.hero[i].x - 8, ply + (int)fx.hero[i].y - 10-15, 8, col[COLBLUE]);
						circlefill(drawbuf, plx + (int)fx.hero[i].x + 8, ply + (int)fx.hero[i].y - 10-15, 8, col[COLMAG]);
						circlefill(drawbuf, plx + (int)fx.hero[i].x + 0, ply + (int)fx.hero[i].y - 20-15, 8, col[COLGREEN]);
						textprintf_centre(drawbuf, font, plx + (int)fx.hero[i].x + 0, ply + (int)fx.hero[i].y - 20-43, col[COLWHITE], "VIROU");
						textprintf_centre(drawbuf, font, plx + (int)fx.hero[i].x + 0, ply + (int)fx.hero[i].y - 20-33, col[COLWHITE], "SORVETE!");
					}
					else {
						ellipsefill(drawbuf, plx + (int)fx.hero[i].x, ply + (int)fx.hero[i].y, 20, 6, col[COLRED]);
						circlefill(drawbuf, plx + (int)fx.hero[i].x, ply + (int)fx.hero[i].y - 10, 12, col[COLRED]);
					}
				}
				// desenha player vivo
				else {

					// verifica turbo drop efeitos
					//
					if (player[i].item_speed)		// tem speed
					if (
							(fabs(fx.hero[i].sx) > svp_maxspeed)		// ta rapído
							||
							(fabs(fx.hero[i].sy) > svp_maxspeed)
						)
					if (get_time() > player[i].speed_drop_time)		// intervalo entre drop de efeito bolinha
					{
						//tempo minimo pra soltar outra bolinha fade
						player[i].speed_drop_time = get_time() + 0.05;

						//solta a bolinha
						cfx_create_speedfx((int)fx.hero[i].x, (int)fx.hero[i].y, player[i].x, player[i].y, teamcol[i/TSIZE], col[i%TSIZE], fx.hero[i].gundir);
					}

					//blink player when hit
					//
					int pc1,pc2;
					pc1 = teamcol[i/TSIZE];
					pc2 = col[i%TSIZE];
					double deltafx = player[i].hitfx - get_time();
					if (deltafx > 0) {
						NLubyte rgb = (NLubyte) ( 70.0 + deltafx * 600.0 );  // var 180
						pc1 = pc2 = makecol(rgb,rgb,rgb);
					}
					else if (player[i].item_quad) {
						//pisca branco
						if ( ((int)(get_time() * 10)) % 2 ) {
							pc1 = col[COLWHITE];
							pc2 = col[COLCYAN];
						}
					}

					//draw player
					draw_player(drawbuf, (int)fd.hero[i].x, (int)fd.hero[i].y, fd.hero[i].gundir, pc1, pc2, alpha);

					//draw deathbringer carrier effect
					if (player[i].item_deathbringer) {
						// intervalo entre drop de efeito
						if (get_time() > player[i].death_drop_time)
						{
							//tempo p/ proximo efeito
							player[i].death_drop_time = get_time() + 0.01;
							//drop it
							cfx_create_deathcarrier((int)fd.hero[i].x + rand()%40-20, (int)fd.hero[i].y + rand()%40-10, player[i].x, player[i].y, i/TSIZE);
							cfx_create_deathcarrier((int)fd.hero[i].x + rand()%40-20, (int)fd.hero[i].y + rand()%40-10, player[i].x, player[i].y, i/TSIZE);
						}
					}

					//draw deathbringer affected effect
					if (player[i].deathbringer_affected) {
						drawing_mode(DRAW_MODE_TRANS, 0,0,0);
						set_trans_blender(0,0,0,128);
						int q,co;
						if (i/TSIZE)
							co = makecol(0xA0,0,0);
						else
							co = makecol(0,0,0xff);
						for (q=0;q<5;q++)
							circlefill(drawbuf, plx + (int)fd.hero[i].x+rand()%40-20, ply + (int)fd.hero[i].y+rand()%40-20 - 15, 15, co);
						for (q=0;q<5;q++)
							circlefill(drawbuf, plx + (int)fd.hero[i].x+rand()%40-20, ply + (int)fd.hero[i].y+rand()%40-20 - 15, 15, 0);
						solid_mode();
					}

					// SHIELD FX!!
					if (player[i].item_shield) {
						ellipse(drawbuf, plx + (int)fd.hero[i].x, ply + (int)fd.hero[i].y - 15, 24+rand()%3, 24+rand()%3, makecol(rand(),rand(),rand()));
						ellipse(drawbuf, plx + (int)fd.hero[i].x, ply + (int)fd.hero[i].y - 15, 24+rand()%5, 24+rand()%5, makecol(rand(),rand(),rand()));
						ellipse(drawbuf, plx + (int)fd.hero[i].x, ply + (int)fd.hero[i].y - 15, 24+rand()%9, 24+rand()%9, makecol(rand(),rand(),rand()));
					}
				}

				//draw player's name -- nao interessa se vivo ou morto
				if (option_show_names)
				if (me >= 0)
				if (me < maxplayers)
				//NOT an invisible enemy
				if (!((player[i].item_helm) && (i/TSIZE != me/TSIZE))) {
					int ttx = (int)fd.hero[i].x + plx;
					int tty = (int)fd.hero[i].y + ply - 40;
					int supercol = teamdcol[i/TSIZE];
					int midcol = col[COLWHITE];

					textprintf_centre(drawbuf, font, ttx-1, tty-1, supercol, "%s", player[i].name);
					textprintf_centre(drawbuf, font, ttx+1, tty-1, supercol, "%s", player[i].name);
					textprintf_centre(drawbuf, font, ttx-1, tty+1, supercol, "%s", player[i].name);
					textprintf_centre(drawbuf, font, ttx+1, tty+1, supercol, "%s", player[i].name);
					textprintf_centre(drawbuf, font, ttx-1, tty , supercol, "%s", player[i].name);
					textprintf_centre(drawbuf, font, ttx+1, tty , supercol, "%s", player[i].name);
					textprintf_centre(drawbuf, font, ttx, tty-1, supercol, "%s", player[i].name);
					textprintf_centre(drawbuf, font, ttx, tty+1, supercol, "%s", player[i].name);

					textprintf_centre(drawbuf, font, ttx, tty, midcol, "%s", player[i].name);
				}
			}
		}

		// DEATHBRINGER: darken ground for carriers
		//
		for (int o=0;o<maxplayers;o++)
		if (player[o].used)	//valid
		if (player[o].x == player[me].x)	//in visible screen
		if (player[o].y == player[me].y)
		if (player[o].onscreen)						//REALLY in visible screen??
		if (player[o].item_deathbringer)		// has deathbringer
		{
			set_clip(drawbuf, plx, ply, plx + plw, ply + plh);
			//darken ground
			drawing_mode(DRAW_MODE_TRANS, 0,0,0);
			for (int i=50;i>0;i -= 5) {
				set_trans_blender(0,0,0,50-i);
				circlefill(drawbuf, plx + (int)fd.hero[o].x, ply + (int)fd.hero[o].y - 10, i, 0);
			}
			solid_mode();
			set_clip(drawbuf, 0, 0, drawbuf->w - 1, drawbuf->h - 1);
		}

		// draw clientside fx apos players
		//
		if (me >= 0)	// where am I?
		for (i=0;i<MAX_CLIENTFX;i++)
		if (cfx[i].used)	//fx used?
		if (cfx[i].px == player[me].x)	//on same screen?
		if (cfx[i].py == player[me].y)
		{
			double tim = get_time();

			//gun explosion
			if (cfx[i].type == 0) {

				double delta = tim - cfx[i].time;
				if (delta > 0.4)
					cfx[i].used = false;
				else {
					for (int e=0;e<3;e++) {
						int rad = 4 + e + (int)(delta * 40);
						circle(drawbuf, plx + cfx[i].x, ply + cfx[i].y, rad, makecol(rand(),rand(),rand()));
					}
				}
			}
			//wall explosion
			else if (cfx[i].type == 2) {

				double delta = tim - cfx[i].time;
				if (delta > 0.2)
					cfx[i].used = false;
				else {
					for (int e=0;e<2;e++) {
						int rad = 4 + e + (int)(delta * 40);
						circle(drawbuf, plx + cfx[i].x, ply + cfx[i].y, rad, makecol(rand(),rand(),rand()));
					}
				}
			}
			//quad wall explosion
			else if (cfx[i].type == 3) {

				double delta = tim - cfx[i].time;
				if (delta > 0.2)
					cfx[i].used = false;
				else {
					for (int e=0;e<3;e++) {
						int rad = 4 + e + (int)(delta * 60);
						circle(drawbuf, plx + cfx[i].x, ply + cfx[i].y, rad, makecol(rand(),rand(),rand()));
					}
				}
			}
			// deathcarrier rastro
			else if (cfx[i].type == 5) {

				double delta = tim - cfx[i].time;
				if (delta > 0.6) {
					cfx[i].used = false;
				}
				else {
					int alpha = 120 - ((int)(delta * 200.0));
					drawing_mode(DRAW_MODE_TRANS, 0,0,0);
					set_trans_blender(0,0,0,alpha);
					double drad = (3.0 + 9.0 * (0.6 - delta));
					int rad = (int)drad;
					int subdist = (int)( (96.0 - drad * 8.0) );
					circlefill(drawbuf, plx + cfx[i].x, ply + cfx[i].y - subdist, rad, cfx[i].col1);
					solid_mode();
				}
			}
			//the deathbringer
			else if (cfx[i].type == 4) {

				double delta = tim - cfx[i].time;
				if (delta > 3.0) {
					cfx[i].used = false;
				}
				else {
					//radius
					int e,rad,co;
					if (delta < 1.0)
						rad = (int)(delta * 100);
					else
						rad = 100 + (int)((delta - 1.0) * (delta - 1.0) * 800);
					int maxxd = max(cfx[i].x, plw-cfx[i].x), maxyd = max(cfx[i].y, plh-cfx[i].y);
					if (maxxd*maxxd + maxyd*maxyd >= rad*rad) {
						set_clip(drawbuf, plx, ply, plx + plw, ply + plh);
						//brightening ring
						for (e=0;e<30;e++) {
							if (cfx[i].owner/TSIZE)
								co = makecol(0,0,14+8*e);
							else
								co = makecol(14+8*e,0,0);
							circle(drawbuf, plx + cfx[i].x, ply + cfx[i].y, rad, co);
							//ellipse(drawbuf, plx + cfx[i].x+1, ply + cfx[i].y, rad, rad, co);
							//ellipse(drawbuf, plx + cfx[i].x, ply + cfx[i].y+1, rad, rad, co);
							rad++;
						}
						//darkening ring
						for (e=0;e<10;e++) {
							if (cfx[i].owner/TSIZE)
								co = makecol(0,0,255-14*e);
							else
								co = makecol(255-14*e,0,0);
							circle(drawbuf, plx + cfx[i].x, ply + cfx[i].y, rad, co);
							circle(drawbuf, plx + cfx[i].x+1, ply + cfx[i].y, rad, co);
							circle(drawbuf, plx + cfx[i].x, ply + cfx[i].y+1, rad, co);
							rad++;
						}
						set_clip(drawbuf, 0, 0, drawbuf->w - 1, drawbuf->h - 1);
					}
				}
			}
		}
	}

	//do not draw stuff below if map not ready to show
	if (!hide_game) {

		// the MINIMAP
		//
		blit(minibg, drawbuf, 0, 0, mmx + 20, mmy, minibg->w, minibg->h);	// minimap bg

		//draw the miniflags
		// - qualquer flag no chao (na base ou nao, carried == false)
		for (int f=0;f<2;f++)
		if (fx.flag[f].carried == false)
			draw_mini_flag(drawbuf, f);

		vector<bool> roomvis;
		roomvis.resize(map.w*map.h, (me>=0 && player[me].item_helm>0)?true:false);

		// draw all teammates and enemies on screens where there are teammates
		//draw all the players - put a pixel where they are
		if (me>=0 && fx.frame>=0)
			for (int i=0;i<maxplayers;i++)
				if (player[i].used && player[i].x>=0 && player[i].y>=0 && player[i].x<map.w && player[i].y<map.h &&
						(i/TSIZE == me/TSIZE || (player[me].enemyvis & (1<<(i%TSIZE)) ))) {
					roomvis[player[i].y*map.w+player[i].x] = true;

					// coord on minimap
					double px, py;
					px = ((double)player[i].x * (double)plw + fx.hero[i].x) / ((double)plw * map.w);
					py = ((double)player[i].y * (double)plh + fx.hero[i].y) / ((double)plh * map.h);
					int pix = mmx + 21 + ((int)(px*98));
					int piy = mmy +  1 + ((int)(py*98));

					//verifica se o jogador a ser desenhado é um carrier de flag inimiga
					int enemyteam = 1-i/TSIZE;
					if (fx.flag[enemyteam].carried)
					if (fx.flag[enemyteam].carrier == i) {

						// update flag position for draw
						fx.flag[enemyteam].pos.px = player[i].x;
						fx.flag[enemyteam].pos.py = player[i].y;
						fx.flag[enemyteam].pos.x = (int)fx.hero[i].x;
						fx.flag[enemyteam].pos.y = (int)fx.hero[i].y;

						// draw the miniflag here
						draw_mini_flag(drawbuf, enemyteam);
					}

					if (i != me) {
						putpixel(drawbuf, pix+0, piy+0, teamcol[i/TSIZE]);	//3 pixel teamcol
						putpixel(drawbuf, pix+1, piy+0, teamcol[i/TSIZE]);	//3 pixel teamcol
						putpixel(drawbuf, pix+0, piy+1, teamcol[i/TSIZE]);	//3 pixel teamcol
						putpixel(drawbuf, pix+1, piy+1, col[i%TSIZE]);		// 1 pixel personal-color
					}
					else {
						//myself: draw differently
						if ( (int(get_time() * 15)) % 3 > 0 ) {
							circlefill(drawbuf, pix, piy, 2, col[COLYELLOW]);
							circlefill(drawbuf, pix, piy, 1, teamlcol[i/TSIZE]);
						}
						else
							circlefill(drawbuf, pix, piy, 2, 0);
					}
				}

		// paint fog of war in all invisible rooms
		//
		for (int ry=0; ry<map.h; ry++)
			for (int rx=0; rx<map.w; rx++)
				if (!roomvis[ry*map.w+rx]) {
					drawing_mode(DRAW_MODE_TRANS, 0,0,0);
					set_trans_blender(0,0,0,0x38);
					int a,b,c,d;
					a = mmx+21 +  rx   *98/map.w  ;
					b = mmy+ 1 +  ry   *98/map.h  ;
					c = mmx+21 + (rx+1)*98/map.w-1;
					d = mmy+ 1 + (ry+1)*98/map.h-1;
					rectfill(drawbuf, a,b,c,d, col[COLFOGOFWAR]);
				}
		solid_mode();
	}//!hide_game

	//
	// the SCOREBOARD
	//
	#define NAMEYDELTA_MIN 8
	int NAMEYDELTA = 12;	//default value

	if (key[KEY_TAB]) {
		textprintf(drawbuf, font, sbx + 4, sby-4, teamlcol[0], "Red Team:   (PINGS)");
		textprintf(drawbuf, font, sbx + 4, sby-4 + 18*NAMEYDELTA_MIN, teamlcol[1], "Blue Team:  (PINGS)");
	}
	else {
		textprintf(drawbuf, font, sbx + 4, sby-4, teamlcol[0], "Red Team:    %2i capt", fx.flag[0].score);
		textprintf(drawbuf, font, sbx + 4, sby-4 + 18*NAMEYDELTA_MIN, teamlcol[1], "Blue Team:   %2i capt", fx.flag[1].score);
	}
	int i;
	int pix[2]; pix[0]=pix[1]=0;
	for (int fw=0;fw<2;fw++)		//first count, then draw
	{
		if (fw == 1) {
			//most players in a team
			int thepix;
			if (pix[0] > pix[1])
				thepix = pix[1];
			else
				thepix = pix[0];
			//calc the new NAMEYDELTA
			while ( (thepix * NAMEYDELTA) > 16 * 8 - 8 - 2) {
				if (NAMEYDELTA == NAMEYDELTA_MIN)
					break;	//the minimum is the minimum...
				NAMEYDELTA--;
			}
		}

		for (int dp=0;dp<maxplayers;dp++)
		{
			// i = player #   dp = draw slot (0-7 = red team's  8-15 = blue team's)
			i = scoreboard[dp];

			//i = dp;

			//draw it
			if (i != -1)
			if (player[i].used)
			{
				// print player name
				//textprintf(drawbuf, font, sbx + 4, sby + 2 +13 + dp * NAMEYDELTA + (dp/TSIZE)*(26-2), col[i%TSIZE], "%s", player[i].name);
				int what_y;
				if (i < TSIZE) {
					what_y = sby + 8 + dp * NAMEYDELTA;
				}
				else {
					what_y = sby + 19*NAMEYDELTA_MIN + (dp-TSIZE) * NAMEYDELTA;
				}

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
					textprintf(drawbuf, font, sbx + 4, what_y, col[i%TSIZE], "%c%s", player[i].reg_status, player[i].name);

					// show ping or frags
					if (key[KEY_TAB]) {
						if (player[i].ping > 9999) player[i].ping = 9999;			//fix ping if too big
						textprintf(drawbuf, font, sbx + 4 + 16*8, what_y, teamlcol[i/TSIZE], "%4i", player[i].ping);
					}
					else
						textprintf(drawbuf, font, sbx + 4 + 16*8, what_y, teamlcol[i/TSIZE], "%4i", player[i].frags);
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
	textprintf(drawbuf, font, plx+10, ply+plh-14, 0, "FPS:%3.0f", FPS);

#ifdef CL_SHOW_TIME_LEFT
	// Time left if time limit is on.
	if (time_limit > 0)
		textprintf(drawbuf, font, plx+10, ply+6, 0, "TIME: %3d:%02d", seconds / 60, seconds % 60);
#endif

	// QUAD DAMAGE
	if (me >= 0) {

		double val;
		if (player[me].item_quad) {
			val = player[me].item_quad_time - get_time();
			if (val < 0) val = 0;
			textprintf(drawbuf, font, plx+244, ply+plh+5, col[COLCYAN], "POWER:  %2.0f", val);
		}
		if (player[me].item_speed) {
			val = player[me].item_speed_time - get_time();
			if (val < 0) val = 0;
			textprintf(drawbuf, font, plx+244, ply+plh+15, col[COLYELLOW], "TURBO:  %2.0f", val);
		}
		if (player[me].item_helm) {
			val = player[me].item_helm_time - get_time();
			if (val < 0) val = 0;
			textprintf(drawbuf, font, plx+244, ply+plh+25, col[COLMAG], "SHADOW: %2.0f", val);
		}

		//WEAPON LEVEL
		textprintf(drawbuf, font, plx+340, ply+plh+5, col[COLWHITE], "WEAPON: %i", player[me].weapon + 1);
	}

	//server hostname
	//textprintf(drawbuf, font, plx+6*8+334+(32-strlen_hostname)*8, ply+plh+25, col[COLINFO], "%s", hostname);

	//show "want change teams" flag if active
	if (want_change_teams)
	{
		int c = col[COLWHITE];
		if ( ( (int) (get_time() * 2.0) ) % 2 )	// blink!
			c = col[COLRED];
		textprintf(drawbuf, font, +0 + plx + plw - 6*8 - 10,     ply + plh - 18, c, "CHANGE");
		textprintf(drawbuf, font, +0 + plx + plw - 6*8 - 10 + 4, ply + plh -  9, c, "TEAMS");
	}
	if (want_map_exit) {
		int c = col[COLWHITE];
		if ( ( (int) (get_time() * 2.0) ) % 2 )	// blink!
			c = col[COLRED];
		textprintf(drawbuf, font, -40 + plx + plw - 6*8 - 10,     ply + plh - 18, c, "EXIT");
		textprintf(drawbuf, font, -40 + plx + plw - 6*8 - 10 + 4, ply + plh -  9, c, "MAP");
	}

	// the STATUSBAR : health energy, bars ....
	//
	if (me >= 0)
		textprintf(drawbuf, font, 10, ply+plh+ 5, col[COLWHITE],  "Health: %4i  Energy: %4i", player[me].health, player[me].energy);

	rectfill(drawbuf, 10, ply+plh+18, 10 + 100, ply+plh+18+10, col[COLNOLIFE]); //lifebar bg
	rectfill(drawbuf, 10+14*8, ply+plh+18, 10+14*8 + 100, ply+plh+18+10, col[COLNOLIFE]); // enerbar bg

	if (me >= 0) {
		if (player[me].health > 0) {

			//barra vermelha 0..100
			int redtarg = player[me].health;
			if (redtarg > 100) redtarg = 100;
			rectfill(drawbuf, 10, ply+plh+18, 10 + redtarg, ply+plh+18+10, col[COLRED]); //lifebar

			//barra magenta 100..200
			int magtarg = player[me].health - 100;
			if (magtarg > 100) magtarg = 100;
			if (magtarg > 0)
				rectfill(drawbuf, 10, ply+plh+18, 10 + magtarg, ply+plh+18+10, col[COLYELLOW]);

			//barra 3o nivel
			int targ3 = player[me].health - 200;
			if (targ3 > 100) targ3 = 100;
			if (targ3 > 0)
				rectfill(drawbuf, 10, ply+plh+18, 10 + targ3, ply+plh+18+10, col[COLMAG]);
		}
		if (player[me].energy > 0) {

			//barra azul 0..100
			int bluetarg = player[me].energy;
			if (bluetarg > 100) bluetarg = 100;
			rectfill(drawbuf, 10+14*8, ply+plh+18, 10+14*8 + bluetarg, ply+plh+18+10, col[COLBLUE]); // enerbar

			//barra verde 100..200
			int magtarg = player[me].energy - 100;
			if (magtarg > 100) magtarg = 100;
			if (magtarg > 0)
				rectfill(drawbuf, 10+14*8, ply+plh+18, 10+14*8 + magtarg, ply+plh+18+10, col[COLGREEN]); // enerbar

			//barra 3o nivel
			int targ3 = player[me].energy - 200;
			if (targ3 > 100) targ3 = 100;
			if (targ3 > 0)
				rectfill(drawbuf, 10+14*8, ply+plh+18, 10+14*8 + targ3, ply+plh+18+10, col[COLENER3]);
		}
	}

	// the HUD: message output
	//
	char lix[16];
	char *themsg;
	int top = 0;
	for (i=0;i<CHAT_SIZE;i++)
		if (chatbuffer[i][0] != '\0') {
			//default text color (normal chat)
			int tcol = col[COLORA];

			//change color if team chat
			strncpy(lix, chatbuffer[i], 2);	//2 primeiros chars da string
			lix[2]=0;
			themsg = &chatbuffer[i][2];
			if (!strcmp(lix, "@T"))		// T eam message
				tcol = col[COLYELLOW];
			else if (!strcmp(lix, "@I")) // I nformation
				tcol = col[COLGREEN];
			else if (!strcmp(lix, "@W")) // W warning
				tcol = col[COLLRED];
			else
				themsg = chatbuffer[i];	//don't discard 2 chars because there's no "@x" rpefix

			//colorful text
			textprintf(drawbuf, font, 3, 3+top*11, tcol, "%s", themsg);
			top++;
		}

	// the HUD: input text on "top" of message output
	//
	if (talkbuffer[0] != 0) {

		static char themsg[128];
		sprintf(themsg, "Say: %s_", talkbuffer);

		//nice border
		textprintf(drawbuf, font, +1+3, +0+3+top*11, 0, "%s", themsg);
		textprintf(drawbuf, font, +1+3, +1+3+top*11, 0, "%s", themsg);
		textprintf(drawbuf, font, +0+3, +1+3+top*11, 0, "%s", themsg);
		textprintf(drawbuf, font, -1+3, +1+3+top*11, 0, "%s", themsg);
		textprintf(drawbuf, font, -1+3, +0+3+top*11, 0, "%s", themsg);
		textprintf(drawbuf, font, -1+3, -1+3+top*11, 0, "%s", themsg);
		textprintf(drawbuf, font, +0+3, -1+3+top*11, 0, "%s", themsg);
		textprintf(drawbuf, font, +1+3, -1+3+top*11, 0, "%s", themsg);

		// the prompt text
		textprintf(drawbuf, font, 3, 3+top*11, col[COLWHITE], "%s", themsg);
	}

	//"server not responding... connection may have dropped" plaque
	if (get_time() > lastpackettime + 1.0) {
		rect(drawbuf,  194,  199, 444, 279, col[COLMENUWHITE]);
		rect(drawbuf, 196, 201, 446, 281, col[COLMENUBLACK]);
		rectfill(drawbuf, 195, 200, 445, 280, col[COLMENUGRAY]);
		textprintf(drawbuf, font, 220, 220, col[COLWHITE], "SERVER NOT RESPONDING...");
		textprintf(drawbuf, font, 220, 240, col[COLWHITE], "May be heavy packet loss,");
		textprintf(drawbuf, font, 220, 255, col[COLWHITE], "or the server disconnected");
	}

	// V0.4.4 : player scores overlay
	if (key[KEY_TAB]) {
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
			i = scoreboard[p];
			redt++;

			sorry[0]=0;
			if (player[i].reg_status == ' ')
				strcpy(sorry, " ");
			else if (player[i].reg_status == '?')
				strcpy(sorry, " ");

			if (sorry[0]==0) {
				textprintf(drawbuf,font,XLEFTPAD,y1+YDEL, col[COLLRED], "%4i %5.2f %5i %-15s %5i %4i",
					player[i].rank,
					( ( ((double)player[i].score) + 1.0) / ( ((double)player[i].neg_score) + 1.0) ),
					player[i].score - player[i].neg_score,
					player[i].name,
					player[i].frags,
					player[i].ping
				);
				//V0.4.8
				redpow += ((double)(player[i].score+1)) / ((double)(player[i].neg_score+1));
			}
			else {
				textprintf(drawbuf,font,XLEFTPAD,y1+YDEL, col[COLLRED], "%16s %-15s %5i %4i",
					sorry,
					player[i].name,
					player[i].frags,
					player[i].ping
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
			i = scoreboard[p];

			sorry[0]=0;
			if (player[i].reg_status == ' ')
				strcpy(sorry, " ");
			else if (player[i].reg_status == '?')
				strcpy(sorry, " ");

			if (sorry[0]==0) {
				textprintf(drawbuf,font,XLEFTPAD,y1+YDEL, col[COLLBLUE], "%4i %5.2f %5i %-15s %5i %4i",
					player[i].rank,
					( ( ((double)player[i].score) + 1.0) / ( ((double)player[i].neg_score) + 1.0) ),
					player[i].score - player[i].neg_score,
					player[i].name,
					player[i].frags,
					player[i].ping
				);
				bluepow += ((double)(player[i].score+1)) / ((double)(player[i].neg_score+1)); //V0.4.8
			}
			else {
				textprintf(drawbuf,font,XLEFTPAD,y1+YDEL, col[COLLBLUE], "%16s %-15s %5i %4i",
					sorry,
					player[i].name,
					player[i].frags,
					player[i].ping
				);
				bluepow += DEFAULT_PLAYER_RATE;	//V0.4.8
			}

			//next
			YDEL += 9;
		}

		//calc powers
		/*
		double rtp, btp, rtpmean, btpmean;

		if ((redt == 0) || (redt == redu))
			rtp = 0;			//can't know...
		else {
			rtpmean = redpow / (redt - redu);
			rtp = (rtpmean * redt) / 3000.0;
		}

		if ((bluet == 0) || (bluet == blueu))
			btp = 0;			//can't know...
		else {
			btpmean = bluepow / (bluet - blueu);
			btp = (btpmean * bluet) / 3000.0;
		}
		*/

		//V0.4.8
		textprintf_centre(drawbuf, font, xc, y1+30, col[COLLRED], "Red Team - Power %.2f", redpow);
		textprintf_centre(drawbuf, font, xc, y1+225, col[COLLBLUE], "Blue Team - Power %.2f", bluepow);
	}

	// debug panel
	if (key[KEY_F9]) {
		clear_to_color(drawbuf, col[COLSHADOW]);

		textprintf(drawbuf,font,0,0,col[COLWHITE], "me=%i ", me);

		int p;
		for (p=0;p<maxplayers;p++) {
			textprintf(drawbuf,font,0,10+p*10,col[COLWHITE], "p.%i u=%i ons=%i evs=%lu sxy=%i,%i HR:p=%.1f,%.1f s=%.1f,%.1f o=%.1f,%.1f",
				p, player[p].used, player[p].onscreen, player[p].enemyvis, player[p].x, player[p].y,

				//					fx.hero[p].x, fx.hero[p].y, fx.hero[p].sx, fx.hero[p].sy,
				fd.hero[p].x, fd.hero[p].y, fd.hero[p].sx, fd.hero[p].sy, fd.hero[p].ox, fd.hero[p].oy
				);
		}

		int bpsin = client->get_socket_stat(NL_AVE_BYTES_RECEIVED);
		int bpsout = client->get_socket_stat(NL_AVE_BYTES_SENT);
		int bpstraffic = bpsin + bpsout;
		textprintf(drawbuf, font, 72*8-2, ply+plh+  5, col[COLINFO], "BPS:%4i", bpstraffic);
		textprintf(drawbuf, font, 71*8-2, ply+plh+ 15, col[COLINFO], "%4i:%4i", bpsin, bpsout);
	}


	//unlock frame mutex
	//LOG1("unlocking HOW=%i",HOWMANY);
	pthread_mutex_unlock( &frame_mutex );
	//LOG1("unlocked! HOW=%i",HOWMANY);

	// another frame, calc FPS...
	//
	totalframecount++;
	framecount++;
	double baixo = get_time() - starttime;
	if (baixo > 0) {
		if (baixo > 1.0) {
			FPS = ((double)framecount) / baixo;
			starttime = get_time();
			framecount = 0;
		}
	}
}

// draw help
void gameclient_c::draw_game_help() {
	clear_to_color(drawbuf, col[COLMENUGRAY]);

	int x = -40;
	int y = -70;

	textprintf(drawbuf, font, x+100, y+100, col[COLWHITE], "Outgun : HELP      --> Press ESC or F1 to go back. <--");

	textprintf(drawbuf, font, x+100, y+120, col[COLWHITE], "For more information access these websites:");
	textprintf(drawbuf, font, x+100, y+130, col[COLWHITE], "  the original Outgun website");
	textprintf(drawbuf, font, x+364, y+130, col[COLGREEN], "http://www.amok.com.br/outgun/en/");
	textprintf(drawbuf, font, x+100, y+140, col[COLWHITE], "  Nix's Outgun development page");
	textprintf(drawbuf, font, x+364, y+140, col[COLGREEN], "http://koti.mbnet.fi/npr/outgun/");

	textprintf(drawbuf, font, x+100, y+160, col[COLWHITE], "           MOVING            ARROW KEYS = MOVE");
	textprintf(drawbuf, font, x+100, y+170, col[COLWHITE], "            YOUR             CONTROL    = SHOOT!");
	textprintf(drawbuf, font, x+100, y+180, col[COLWHITE], "          CHARACTER:         ALT        = STRAFE");
	textprintf(drawbuf, font, x+100, y+190, col[COLWHITE], "            >>>>>            SHIFT      = RUN");

	textprintf(drawbuf, font, x+100, y+210, col[COLWHITE], "TALKING TO ALL PLAYERS: Just type your message and hit ENTER");

	textprintf(drawbuf, font, x+100, y+230, col[COLWHITE], "TALKING JUST TO YOUR TEAM: Just place a dot ('.') at the very");
	textprintf(drawbuf, font, x+100, y+240, col[COLWHITE], " beginning of your message (first char)");

	textprintf(drawbuf, font, x+100, y+260, col[COLWHITE], "GAME CONCEPT: You are a member of a team, either RED or BLUE,");
	textprintf(drawbuf, font, x+100, y+270, col[COLWHITE], " assigned to you at random when you connect. Your goal is to");
	textprintf(drawbuf, font, x+100, y+280, col[COLWHITE], " help your team to win, by capturing 8 (default) times the enemy");
	textprintf(drawbuf, font, x+100, y+290, col[COLWHITE], " flag. To capture the flag, a member of your team must steal");
	textprintf(drawbuf, font, x+100, y+300, col[COLWHITE], " the enemy flag and bring it to your team's flag, provided your");
	textprintf(drawbuf, font, x+100, y+310, col[COLWHITE], " flag has not been stolen already! Capiche?");

	textprintf(drawbuf, font, x+100, y+330, col[COLWHITE], "HEALTH AND ENERGY: If your health reaches zero, you die. Energy");
	textprintf(drawbuf, font, x+100, y+340, col[COLWHITE], " is used for running, shooting and health protection when you");
	textprintf(drawbuf, font, x+100, y+350, col[COLWHITE], " have the SHIELD powerup (you'll know when you see it...).");
	textprintf(drawbuf, font, x+100, y+360, col[COLWHITE], " Health and energy regenerate with time.");

	textprintf(drawbuf, font, x+100, y+380, col[COLWHITE], "MINIMAP: On the upper-right corner of the screen is the minimap.");
	textprintf(drawbuf, font, x+100, y+390, col[COLWHITE], " It shows the contents of all rooms of the map that have at least");
	textprintf(drawbuf, font, x+100, y+400, col[COLWHITE], " one player of your team.");

	textprintf(drawbuf, font, x+100, y+420, col[COLWHITE], "CHANGING TEAMS: Hit the END key to set whether you want to");
	textprintf(drawbuf, font, x+100, y+430, col[COLWHITE], " change team or not. You will change team when appropriate.");

	textprintf(drawbuf, font, x+100, y+450, col[COLWHITE], "POWERUPS: If you see an animated item lying on the ground, grab");
	textprintf(drawbuf, font, x+100, y+460, col[COLWHITE], " it. It's a special power-up item.");

	textprintf(drawbuf, font, x+100, y+480, col[COLWHITE], "ETC.: Hit DEL to kill yourself. Hold TAB to see other players'");
	textprintf(drawbuf, font, x+100, y+490, col[COLWHITE], " ping times (in milliseconds) on the scoreboard.");
	textprintf(drawbuf, font, x+100, y+500, col[COLWHITE], " Hit HOME to change world colors and CTRL+HOME to restore them.");
	textprintf(drawbuf, font, x+100, y+510, col[COLWHITE], " Hit F10 to receive a random name. Hit F11 to take a screenshot.");
}

//draws the game menu
void gameclient_c::draw_game_menu() {

	//"3d" menu
	if (menu != 1) {
		rect(drawbuf,  99,  69, 539, 409, col[COLMENUWHITE]);
		rect(drawbuf, 101, 71, 541, 411, col[COLMENUBLACK]);
		rectfill(drawbuf, 100, 70, 540, 410, col[COLMENUGRAY]);
		textprintf(drawbuf, font, 150, 120, col[COLWHITE], "Outgun         version %s", GAME_VERSION);
		textprintf(drawbuf, font, 150, 135, col[COLGREEN], "http://koti.mbnet.fi/npr/outgun/");
	}

	if (menu == 0) {
		static int DELY = 10;

		textprintf(drawbuf, font, 150, 185-DELY, col[COLWHITE], "  [ 1 ]   Connect");
		textprintf(drawbuf, font, 150, 200-DELY, col[COLWHITE], "  [ 2 ]   Disconnect");
		if (connected)
			textprintf(drawbuf, font, 150+22*8, 200-DELY, col[COLGREEN], "(%s)", address);
		textprintf(drawbuf, font, 150, 215-DELY, col[COLWHITE], "  [ 3 ]   Change Player Name & Password");
		textprintf(drawbuf, font, 150, 227-DELY, col[COLGREEN], "          '%s' (%s)", playername, namestatus);
		textprintf(drawbuf, font, 150, 243-DELY, col[COLWHITE], "  [ 4 ]   Start/stop local server");
		if (listen_server_running)
			textprintf(drawbuf, font, 150, 255-DELY, col[COLGREEN], "          SERVER RUNNING ON PORT %i", listen_port_running);
		textprintf(drawbuf, font, 150, 271-DELY, col[COLWHITE], "  [ 5 ]   Toggle fullscreen/windowed mode");

		if (validtheme) {
			textprintf(drawbuf, font, 150, 286-DELY, col[COLWHITE], "  [ 6 ]   Change sound theme: (%s)", sfxthemedir);
			textprintf_centre(drawbuf, font, 150+180, 300-DELY, col[COLGREEN], "'%s'", sfxthemename);
		}
		else {
			textprintf(drawbuf, font, 150, 286-DELY, col[COLWHITE], "  [ 6 ]   Change sound theme:");
			textprintf(drawbuf, font, 150, 300-DELY, col[COLGREEN], "          no sfx themes found.");
		}
		textprintf(drawbuf, font, 150, 340-DELY, col[COLWHITE], "Hit CTRL+F12 to EXIT THE GAME");
		textprintf(drawbuf, font, 150, 355-DELY, col[COLWHITE], "Hit ESC to HIDE OR SHOW THIS MENU");
		textprintf(drawbuf, font, 150, 370-DELY, col[COLORA], "Hit F1 to SHOW THE HELP SCREEN");
	}
	else if (menu == 1) {

		//Big F Menu
		rect(drawbuf,  19,  19, 620, 460, col[COLMENUWHITE]);
		rect(drawbuf,  21,  21, 621, 461, col[COLMENUBLACK]);

		int lotext = makecol(0x99, 0x99, 0x99);


		if (showmaster) {

			int hi = makecol(0x68, 0x68, 0x88); //col[COLMENUGRAY]; //makecol(0x99,0x99,0x99);
			int lo = makecol(0x68,0x48,0x48);
			//hilight all
			rectfill(drawbuf, 20, 20, 620, 460, hi);
			//first bar hi vs lo
			rectfill(drawbuf, 20, 20, 320, 50, hi);
			rectfill(drawbuf, 320, 19, 621, 50, 0);
			rectfill(drawbuf, 320, 24, 616, 50, lo);
			vline(drawbuf, 320, 20, 50, col[COLMENUBLACK]);
			hline(drawbuf, 320, 50, 621, col[COLMENUWHITE]);
			hline(drawbuf, 320, 24, 616, lotext);
			vline(drawbuf, 616, 24, 49, col[COLMENUBLACK]);
			textprintf_centre(drawbuf, font, 170, 35, col[COLWHITE], "INTERNET SEARCH");
			textprintf_centre(drawbuf, font, 470, 35, lotext, "FAVORITES");
			//textprintf_centre(drawbuf, font, 320, 40, col[COLWHITE], "Showing INTERNET LISTING page (TAB = FAVORITES)");

			if (((int)(get_time() * 1)) % 2)
				textprintf_centre(drawbuf, font, 320, 65, col[COLGREEN], "F2 = UPDATE LIST OF SERVERS");
			else
				textprintf_centre(drawbuf, font, 320, 65, col[COLYELLOW], "F2 = UPDATE LIST OF SERVERS");

			textprintf_centre(drawbuf, font, 320, 80, col[COLWHITE], "Press SPACE to refresh the servers");
			//textprintf_centre_x(drawbuf, font, 320, 75, col[COLGREEN], 0, "TAB = Change to FAVORITES page");

			//textprintf_centre(drawbuf, font, 320, 115, col[COLWHITE], "ARROWS:Select - ENTER:Connect - ESC:Cancel - SPACE:Refresh");

			textprintf_centre(drawbuf, font, 320, 440, col[COLWHITE], "TAB:Favorites  ARROWS:Select  ENTER:Connect  ESC:Cancel  SPACE:Refresh");
		}
		else {

			int hi = makecol(0x88, 0x68, 0x68); //col[COLMENUGRAY]; //makecol(0x99,0x99,0x99);
			int lo = makecol(0x48,0x48,0x68);
			//hilight all
			rectfill(drawbuf, 20, 20, 620, 460, hi);
			//first bar lo vs hi
			rectfill(drawbuf, 320, 20, 620, 50, hi);
			rectfill(drawbuf, 19, 19, 320, 50, 0);
			rectfill(drawbuf, 24, 24, 320, 50, lo);
			vline(drawbuf, 320, 19, 50, col[COLMENUWHITE]);//?
			hline(drawbuf, 19, 50, 320, col[COLMENUWHITE]);
			hline(drawbuf, 24, 24, 320, lotext);
			vline(drawbuf, 24, 24, 49, col[COLMENUWHITE]);
			textprintf_centre(drawbuf, font, 170, 35, lotext, "INTERNET SEARCH");
			textprintf_centre(drawbuf, font, 470, 35, col[COLWHITE], "FAVORITES");

			//textprintf_centre(drawbuf, font, 320, 40, col[COLWHITE], "Showing FAVORITES page (TAB = INTERNET LISTING)");
			textprintf_centre(drawbuf, font, 320, 65, col[COLWHITE], "Type the IP address of the server and hit ENTER");
			textprintf_centre(drawbuf, font, 320, 80, col[COLWHITE], "Press SPACE to refresh the servers");
			//textprintf_centre_x(drawbuf, font, 320, 75, col[COLYELLOW], 0, "TAB = Change to INTERNET LISTING page");

			textprintf_centre(drawbuf, font, 320, 440, col[COLWHITE], "TAB:Internet  ARROWS:Select  ENTER:Connect  ESC:Cancel  SPACE:Refresh");
		}

		int xi = 50 - 8*2;

		textprintf(drawbuf, font, xi, 105, col[COLWHITE], "IP Address             Ping #P Version/Hostname");

		char blinkchar[2];

		int yi;

		for (int i=0;i<MAX_GAMESPY;i++) {

			yi = 120 + i*13;

			//selectr
			if (gi == i) {
				rectfill(drawbuf, xi-3,yi-3,xi+550+8*3,yi+12,col[COLSHADOW]);

				//blink cursor
				if (((int)(get_time() * 4)) % 2)
					blinkchar[0]=' ';
				else
					blinkchar[0]='<';
				blinkchar[1]=0;
			}
			else
				blinkchar[0]=0;

			//server edit prompt
			if (showmaster) {
				textprintf(drawbuf, font, xi, yi, col[COLGREEN], ":%s%s",mgamespy[i].address, blinkchar);

				//favs watermarks
				if (mgamespy[i].favs)
					textprintf(drawbuf, font, xi - 12, yi, makecol(0x99,0x78,0x78), "*");
			}
			else
				textprintf(drawbuf, font, xi, yi, col[COLGREEN], ":%s%s",gamespy[i].address, blinkchar);

			//draw gamespy entry
			bool refreshed, invalid, noresponse;
			if (showmaster) {
				refreshed  = mgamespy[i].refreshed;
				invalid    = mgamespy[i].invalid;
				noresponse = mgamespy[i].noresponse;
			}
			else {
				refreshed  = gamespy[i].refreshed;
				invalid    = gamespy[i].invalid;
				noresponse = gamespy[i].noresponse;
			}

			if (!refreshed) { // not refreshed
				//server info
				textprintf(drawbuf, font, xi + (18+5)*8, yi, col[COLWHITE], "press SPACEBAR to refresh...");
			}
			else if (invalid) {	//refreshed, invalid
				//server info
				textprintf(drawbuf, font, xi + (18+5)*8, yi, col[COLWHITE], "---");
			}
			else if (noresponse) {	//refreshed, no response
				//server info
				textprintf(drawbuf, font, xi + (18+5)*8, yi, col[COLWHITE], "no response.");
			}
			else {  //refreshed, valid
				//server info
				if (showmaster)
					textprintf(drawbuf, font, xi + (18+5)*8, yi, col[COLGREEN], "%s", mgamespy[i].info);
				else
					textprintf(drawbuf, font, xi + (18+5)*8, yi, col[COLGREEN], "%s", gamespy[i].info);
			}
		}
	}
	else if (menu == 2) {
		textprintf(drawbuf, font, 150, 230, col[COLWHITE], dialogmessage);
		textprintf(drawbuf, font, 150, 250, col[COLWHITE], dialogmessage2);
	}
	else if (menu == 3) {
		textprintf(drawbuf, font, 150, 170, col[COLWHITE], "Type in your player name. If you have");
		textprintf(drawbuf, font, 150, 185, col[COLWHITE], "registered your name on the Outgun");
		textprintf(drawbuf, font, 150, 200, col[COLWHITE], "website, then type in your password!");

		textprintf(drawbuf, font, 150, 220, col[COLWHITE], "ENTER = OK   ESC = CANCEL  TAB = NEXT FIELD");
		textprintf(drawbuf, font, 150, 260, col[COLGREEN], "NAME     :%s%s", editplayername, namecursor);

		//password field: '********'
		char starpass[32]; int c=0;
		for (; editplayerpass[c]; c++) starpass[c] = '*';
		starpass[c] = 0;

		textprintf(drawbuf, font, 150, 285, col[COLGREEN], "PASSWORD :%s%s", starpass, passcursor);

		textprintf(drawbuf, font, 150, 350, col[COLWHITE], "Registration status: %s", namestatus);
	}
	else {
		textprintf(drawbuf, font, 150, 150, col[COLRED], "unknown menu %i", menu);
	}
}

//show a specific menu screen
void gameclient_c::set_menu(int menumber) {
	menu = menumber;
	clear_keybuf(); //clear keystrokes buffer
}

//disconnect command
void gameclient_c::disconnect_command() {

	//disconnect the client here if was connected, else does nothing
	client->connect(false);

	//dialogz
	strcpy(dialogmessage, "You are disconnected. Press ESC");
	strcpy(dialogmessage2, "");
	menushow = true;
	set_menu(2);	//dialog menu
}

void gameclient_c::client_connected(char *data, int length) {
	(void)length;

	//not trying anymore
	trying_connection = false;

	//no host ad yet
	hostad = 0;
	hostadname[0]=0;

	//"data" from connection accepted:
	//  BYTE		maxplayers
	//  STRING	hostname
	int count = 0;
	NLubyte maxpl;
	readByte(data, count, maxpl);
	maxplayers = maxpl;
	//strcpy(hostname, data);
	readString(data, count, hostname);
	hostname[32]=0;		//truncate at 32 chars
	strlen_hostname = strlen(hostname);	//for drawing

	//V0.4.4: read server's NOTCP flag value
	NLubyte noflag;
	readByte(data, count, noflag);
	server_no_tcp = (noflag > 0);

	//set window title: the hostname
	char lecaption[256];
	sprintf(lecaption, "Connected to: %s (%s)", hostname, address);
	server_status_string(lecaption);

	int i;
	//default scoreboard state
	for (i=0;i<MAX_PLAYERS;i++)	scoreboard[i]=i;

	//don't want to change teams by default
	want_change_teams = false;

	//don't want to exit map by default
	want_map_exit = false;

	//avoid "dropped" plaque
	lastpackettime = get_time() + 1.0;

	// reset gamestate?
	connected = true;
	gameshow = true;
	fx.frame = -1.0;		// no data
	fd.frame = -1.0;		// no data
	me = -1;	//don't know who am I

	//hide menu : must be AFTER gameshow = true
	menushow = false;  // hide menu

	//reset chat buffer
	talkbuffer[0]=0;
	for (i=0;i<CHAT_SIZE;i++)
		chatbuffer[i][0]=0;
	chaterasetime = get_time() + 10.0;

	//reset world data
	// players

	for (i=0;i<MAX_PLAYERS;i++)
		player[i].clear(false, i, 0, "(name unknown)");

	//reset FPS count vars
	framecount = 0;
	starttime = get_time();
	FPS = 666.0;

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

	//not showing gameover plaque
	gameover_plaque = NEXTMAP_NONE;

	//clear fx
	clear_fx();
}

void gameclient_c::client_disconnected() {

	//restore window title
	server_status_string("Outgun client - CTRL+F12 to quit");

	// the gamestate?
	connected = false;
	gameshow = false;

	// show a message
	strcpy(dialogmessage, "You have been disconnected. Press ESC");
	strcpy(dialogmessage2, "");
	menushow = true;
	set_menu(2);	//dialog menu

	//namestatus
	if (namestatus_code == 0)
		strcpy(namestatus, "NO PASSWORD SET?");
	else if ((namestatus_code == 1) || (namestatus_code == 3)) {
		strcpy(namestatus, "LOGGED (");
		strcat(namestatus, player_token);
		strcat(namestatus, ")");
	}
	else if (namestatus_code == 2) {
		strcpy(namestatus, "ERROR: WRONG ID?");
	}

}

void gameclient_c::connect_failed_denied(char *data, int length) {

	//not trying anymore
	trying_connection = false;

	//extract message
	char message[256];
	if (length > 0) {
		int count = 0;
		readString(data, count, message);
	}
	else
		strcpy(message, "no reason given.");

	// show a message
	strcpy(dialogmessage, "Connection refused     (press ESC)");
	strcpy(dialogmessage2, message);
	menushow = true;
	set_menu(2);	//dialog menu
}

void gameclient_c::connect_failed_unreachable() {

	//not trying anymore
	trying_connection = false;

	// show a message
	strcpy(dialogmessage, "No response from server. Press ESC");
	strcpy(dialogmessage2, "");
	menushow = true;
	set_menu(2);	//dialog menu
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

	show_progress("", "Refreshing servers...", "");

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

	int i;

	// no response from all calc addresses num_valid
	//
	double st[MAX_GAMESPY][4];	//send time
	int	rc[MAX_GAMESPY];		//resposta count
	double rt[MAX_GAMESPY];		//resposta time
	int num_valid = 0;
	for (i=0;i<MAX_GAMESPY;i++) {

		rc[i]=0;	//no responses
		rt[i]=0;	//no time

		gamespy[i].noresponse = true;
		gamespy[i].invalid = true; // invalid entry for default
		gamespy[i].refreshed = true;	//refreshed now

		nlOpen(0,0);//force invalid enum error

		nlStringToAddr(gamespy[i].address, &gamespy[i].addr);

		//test if the address is invalid (important)
		if (nlGetError() == NL_INVALID_ENUM) {

			nlOpen(0,0);//force invalid enum error

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
		for (int i=0;i<MAX_GAMESPY;i++)
		if (!gamespy[i].invalid)
		{
			int count =0;
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
	for (int bla=0;bla<20;bla++)
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
	for (i=0;i<MAX_GAMESPY;i++)
	if (!gamespy[i].noresponse)	//got at least 1 response?
	{
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
		strcpy(address, mgamespy[gi].address);
	else
		strcpy(address, gamespy[gi].address);

	// start connecting to specified IP/port
	// connection results will come through the CFUNC_CONNECTION_UPDATE callback
	if (address[0] == '\0')	{ //empty address == my own ip
		NLaddress myadr;
		get_self_IP(&myadr);
		nlSetAddrPort(&myadr, port);
		nlAddrToString(&myadr, address);
/*
		if (showmaster)
			strcpy(mgamespy[gi].address, address);	//copy to gamespy
		else
			strcpy(gamespy[gi].address, address);	//copy to gamespy
*/
	}
	else if (strchr(address, ':')==NULL)
		strcat(address, ":25000");

	client->set_server_address(address);

	//set connect-data (goes in every connect packet): outgun game name and version strings
	char lebuf[256]; int count = 0;
	writeString(lebuf, count, GAME_STRING);
	writeString(lebuf, count, GAME_PROTOCOL);
	client->set_connect_data(lebuf, count);

	client->connect(true);

	// set flags, show dialog...
	trying_connection = true;
	sprintf(dialogmessage, "trying to connect... ESC=CANCEL");
	dialogmessage2[0]='\0';
	set_menu(2);	// dialog
}

//send player token message
void gameclient_c::send_player_token() {
	if (player_token_set) {
		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 30);	// 30 = pass registration token to server
		writeString(lebuf, count, player_token);
		client->send_message(lebuf, count);
	}
}

//issue change name command
void gameclient_c::issue_change_name_command() {

	//regular change name
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, 1);		// "1" = client request name change
	playername[15] = 0;	//truncate player name, max 16 chars
	writeString(lebuf, count, playername);	// the name
	client->send_message(lebuf, count);

	//name changed:
	player_token_new = true;	//getting a NEW token, not refreshing the token

	//get it (V0.4.9) -- força um "ENTER" logo apos o cara conectar ou trocar de nome ao estar
	//conectado...
	check_change_pass_command();

	//FIXME: code == 2 (?)
	if ((namestatus_code == 1) || (namestatus_code == 3))
		strcpy(namestatus, "NAME CHANGED...");
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
	strcpy(playername, editplayername);
	if (menushow)
		set_menu(0);

	//send reliable net message with the name
	if (connected) {
		issue_change_name_command();
	}
}

//send the client's frame to server (keypresses)
void gameclient_c::send_frame() {

	char lebuf[256]; int count = 0;

	//client send: l,r,u,d keys and strafe
	NLubyte  keys = 0;
	if (key[KEY_LEFT])  keys += 1;
	if (key[KEY_RIGHT]) keys += 2;
	if (key[KEY_UP])    keys += 4;
	if (key[KEY_DOWN])  keys += 8;
	if ((key[KEY_ALT]) || (key[KEY_ALTGR]))   //strafe
		keys += 16;
	if ((key[KEY_LSHIFT] || (key[KEY_RSHIFT])))	//run
		keys += 32;
	writeByte(lebuf, count, keys);

	//send the client input
	client->send_frame(lebuf, count);
}

//helper do helper: reconstitui 1 rocket
void gameclient_c::client_set_rocket(int id, int dir, NLulong frameno, int owner, int px, int py, int x, int y, int xdelta) {

	rocket_c  *rock = &fx.rock[id];

	rock->hit_time = 0;
	rock->deg = dir * PIOIT;

	//REMENDO: avanca 0,5 frame
	//rock->time = frameno;
	rock->cl_time = (double)frameno - 0.5;	// "meio frame" atras, isto, e o tiro adianta
																					//porque foi atirado mais antes

	rock->owner = owner;
	rock->dontdraw = false;		//DO draw...
	rock->team = owner/TSIZE;
	rock->x = x;
	rock->y = y;
	rock->px = px;
	rock->py = py;
	rock->x += cos(rock->deg + PI/2) * xdelta;
	rock->y += sin(rock->deg + PI/2) * xdelta;
}

//helper: reconstitui varios rockets de uma mensagem "7" tipo rocket fire.
void gameclient_c::client_rebuild_shot(int pow, int dir, int *rids, NLulong frameno, int owner, int px, int py, int x, int y) {

	//char lix[2000];
	//sprintf(lix, "rids = %i %i %i", rids[0], rids[1], rids[2]);
	//print_message(lix);

	switch (pow) {
	case 1:
		client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, 0);
		break;
	case 2:
		client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, - SHOT_DELTAX);
		client_set_rocket(rids[1], dir,frameno,owner,px,py,x,y, + SHOT_DELTAX);
		break;
	case 3:
		client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, 0);
		client_set_rocket(rids[1], dir,frameno,owner,px,py,x,y, - SHOT_DELTAX * 2);
		client_set_rocket(rids[2], dir,frameno,owner,px,py,x,y, + SHOT_DELTAX * 2);
		// V0.4.8 : NEW TRIPEL SHOT
		//client_set_rocket(rids[1], dir+1,frameno,owner,px,py,x,y, 0);
		//client_set_rocket(rids[2], dir-1,frameno,owner,px,py,x,y, 0);
		break;
	case 4:
		client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, - SHOT_DELTAX);
		client_set_rocket(rids[1], dir,frameno,owner,px,py,x,y, + SHOT_DELTAX);
		client_set_rocket(rids[2], dir+1,frameno,owner,px,py,x,y, 0);
		client_set_rocket(rids[3], dir-1,frameno,owner,px,py,x,y, 0);
		break;
	case 5:
		client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, 0);
		client_set_rocket(rids[1], dir,frameno,owner,px,py,x,y, - SHOT_DELTAX * 2);
		client_set_rocket(rids[2], dir,frameno,owner,px,py,x,y, + SHOT_DELTAX * 2);
		client_set_rocket(rids[3], dir+2,frameno,owner,px,py,x,y, 0);
		client_set_rocket(rids[4], dir-2,frameno,owner,px,py,x,y, 0);
		break;
	case 6:
		client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, - SHOT_DELTAX);
		client_set_rocket(rids[1], dir,frameno,owner,px,py,x,y, + SHOT_DELTAX);
		client_set_rocket(rids[2], dir+1,frameno,owner,px,py,x,y, 0);
		client_set_rocket(rids[3], dir-1,frameno,owner,px,py,x,y, 0);
		client_set_rocket(rids[4], dir+2,frameno,owner,px,py,x,y, 0);
		client_set_rocket(rids[5], dir-2,frameno,owner,px,py,x,y, 0);
		break;
	case 7:
		client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, 0);
		client_set_rocket(rids[1], dir,frameno,owner,px,py,x,y, - SHOT_DELTAX * 2);
		client_set_rocket(rids[2], dir,frameno,owner,px,py,x,y, + SHOT_DELTAX * 2);
		client_set_rocket(rids[3], dir+2,frameno,owner,px,py,x,y, 0);
		client_set_rocket(rids[4], dir-2,frameno,owner,px,py,x,y, 0);
		client_set_rocket(rids[5], dir+3,frameno,owner,px,py,x,y, 0);
		client_set_rocket(rids[6], dir-3,frameno,owner,px,py,x,y, 0);
		break;
	case 8:
		client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, - SHOT_DELTAX);
		client_set_rocket(rids[1], dir,frameno,owner,px,py,x,y, + SHOT_DELTAX);
		client_set_rocket(rids[2], dir+1,frameno,owner,px,py,x,y, 0);
		client_set_rocket(rids[3], dir-1,frameno,owner,px,py,x,y, 0);
		client_set_rocket(rids[4], dir+2,frameno,owner,px,py,x,y, 0);
		client_set_rocket(rids[5], dir-2,frameno,owner,px,py,x,y, 0);
		client_set_rocket(rids[6], dir+3,frameno,owner,px,py,x,y, 0);
		client_set_rocket(rids[7], dir-3,frameno,owner,px,py,x,y, 0);
		break;
	case 9:
		client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, 0);
		client_set_rocket(rids[1], dir,frameno,owner,px,py,x,y, - SHOT_DELTAX * 2);
		client_set_rocket(rids[2], dir,frameno,owner,px,py,x,y, + SHOT_DELTAX * 2);
		client_set_rocket(rids[3], dir+1,frameno,owner,px,py,x,y, 0);
		client_set_rocket(rids[4], dir-1,frameno,owner,px,py,x,y, 0);
		client_set_rocket(rids[5], dir+2,frameno,owner,px,py,x,y, 0);
		client_set_rocket(rids[6], dir-2,frameno,owner,px,py,x,y, 0);
		client_set_rocket(rids[7], dir+3,frameno,owner,px,py,x,y, 0);
		client_set_rocket(rids[8], dir-3,frameno,owner,px,py,x,y, 0);
		break;
	}
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

	//discard older frames
	//overwrite always the newer frames
	// TARGET FRAME: just one
	if (svframe > fx.frame)
	{
		fx.frame = svframe;
		fx.time  = get_time();		//hope it's good enough... needed 10ms clock (1/100 sec) at least.

		NLulong	players_present;		//LONG players present (32 players max)
		readLong(data, count, players_present);
		int i;
		for (i=0;i<maxplayers;i++) {
			//decode players_present: sets if "player" record is used or not, in clientside
			if (players_present & (1 << i))
				player[i].used = true;
			else
				player[i].used = false;
		}

		//UGLY HACK
		static NLulong old_players_present;
		if (old_players_present != players_present) {
			old_players_present = players_present;
			update_scoreboard();
		}

		//----- PLAYER SPECIFIC DATA -----

		//extra byte of information
		// BIT 0: extra health
		// BIT 1: extra energy
		// BIT 2 (****VERY IMPORTANT****): NO MORE DATA ON PACKET BECAUSE PLAYER IS NOT READY
		NLubyte xtra = 0;
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
			player[me].x = scr;
			readByte(data, count, scr);		//player.y
			player[me].y = scr;

			//read "players onscreen" vector
			NLulong	players_onscreen;
			readLong(data, count, players_onscreen);

			//decode players_onscreen and update player data
			for (i=0;i<maxplayers;i++) {

				//decode players_onscreen: sets if "player" record is there to be read
				if (players_onscreen & (1 << i))
					player[i].onscreen = true;
				else
					player[i].onscreen = false;

				//if player on screen, parse the data
				hero_t	*h;
				if (player[i].onscreen) {

					h = &(fx.hero[i]);

					//V0.3.9: took out screen reading, replacing for the same screen of "me"
					// that is set above
					player[i].x = player[me].x;	//same screen since it's on the "players on same screen" vector
					player[i].y = player[me].y;

					//coords & speeds
//						NLshort sho;
					NLubyte xy;
					//V0.3.9 : transmissao x,y de 4 bytes para 3
					//256+512+1024+2048 = 3840    last 4 bits mask
					//xy = ((hx & 3840) >> 8) + ((hy & 3840) >> 4); //x: bit 8-11 to 0-3  y: bit 8-11 to 4-7
					//writeByte(lebuf, lecount, xy);   //last 4 bits x + last 4 bits y

					readByte(data, count, xy);		//first 8 bits x
					h->x = ((double)xy);
					readByte(data, count, xy);		//first 8 bits y
					h->y = ((double)xy);
					readByte(data, count, xy);		//first 4 bits x + first 4 bits y
					h->x += ((double)  ( (xy &  15) << 8 )   );	//bits 0-3 to 8-11
					h->y += ((double)  ( (xy & 240) << 4 )   ); //bits 4-7 to 8-11

					//readShort(data, count, sho);		//x
					//h->x = ((double)sho);
					//readShort(data, count, sho);		//y
					//h->y = ((double)sho);

					//V0.3.9 speed em bytes, xinelao mesmo
					NLbyte sxy;
					readByte(data, count, sxy);	//sx
					h->sx = ((double)sxy) / 2.0;
					readByte(data, count, sxy);	//sy
					h->sy = ((double)sxy) / 2.0;

					//readShort(data, count, sho);		//sx
					//h->sx = ((double)sho) / 100.0;
					//readShort(data, count, sho);		//sy
					//h->sy = ((double)sho) / 100.0;

					//EXTRA BYTE
					NLubyte byt, extra;
					readByte(data, count, extra);			//extra byte

					//FLAGS BYTE
					//
					player[i].dead = (extra & 1) != 0;  //DEAD PLAYER = extra bit 0
					player[i].item_deathbringer = (extra & 2) != 0;		//deathbringer: extra bit 1
					player[i].deathbringer_affected = (extra & 4) != 0; //deathbringer-affected: extra bit 2
					// ITEMS: movido para este byte
					player[i].item_shield = (extra & 8) != 0;
					player[i].item_speed = (extra & 16) != 0;
					player[i].item_quad = (extra & 32) != 0;

					//verifica se acabou de morrer - play death sound
					if ((player[i].dead) && (!player[i].old_dead))
						sound(SAMPLE_DEATH + rand() % 2);
					player[i].old_dead = player[i].dead;

					//l,r,u,d  accel
					readByte(data, count, h->keys);
					h->l = ((h->keys & 1) != 0);
					h->r = ((h->keys & 2) != 0);
					h->u = ((h->keys & 4) != 0);
					h->d = ((h->keys & 8) != 0);

					//RUN!!! (COMO QUE NAO TINHA ISSO???)
					//  devo ter apagado sem querer... ????
					h->run = ((h->keys & 16) != 0);

					//bits 5..7 : gundir= 0..7
					h->gundir = (h->keys & (32+64+128)) / 32;

					//read items
					//readByte(data, count, byt);
					//player[i].item_shield =    ((byt & 1) != 0);
					//player[i].item_speed =    ((byt & 2) != 0);
					//player[i].item_quad =    ((byt & 4) != 0);

					//read helm byte
					readByte(data, count, byt);
					player[i].item_helm = byt;
				}
			}

			//read "enemies on team vislist"
			NLushort eviz;
			readShort(data, count, eviz);
			if (me >= 0)
				player[me].enemyvis = eviz;

			//read who,x,y
			NLubyte who,whox,whoy;
			readByte(data, count, who);
			readByte(data, count, whox);
			readByte(data, count, whoy);

			//update this player's px,py,x,y
			//ignore self and anybody onscreen -- because then I've got better accuracy
			if (who != me)
			if (!player[who].onscreen) {
				player[who].x = whox / (255/map.w);	//screen = 0..255 / (WXMAX/255)
				player[who].y = whoy / (255/map.h);
				fx.hero[who].x = (whox % (255/map.w)) * plw / (255/map.w);	//posicao dentro da tela especifica
				fx.hero[who].y = (whoy % (255/map.h)) * plh / (255/map.h);
			}

			//read player's health and energy
			NLubyte healt, energ;
			readByte(data, count, healt);
			if (me >= 0) {
				player[me].health = healt;
				//EXTRA BIT FROM WAYY ABOVE
				if (xtra & 1) player[me].health += 256;
			}

			readByte(data, count, energ);
			if (me >= 0) {
				player[me].energy = energ;
				//EXTRA BIT FROM WAYY ABOVE
				if (xtra & 2) player[me].energy += 256;
			}

			//read ping of player frame % MAX_PLAYERS
			NLushort daping;
			readShort(data, count, daping);
			player[ svframe % maxplayers ].ping = daping;
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
			int rids[16]; //rocket ids pra msg 7
			NLubyte rowner, rpx, rpy, code, pid, team, carried, abyte, rockid, iid, rpow, rdir, sx, sy;
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

			//parse rest of message
			switch (code) {

			// name update
			case 1:
				readByte(msg, count, pid);		// player id
				readString(msg, count, player[pid].name);		// da name
				player[pid].name[15] = 0;		// force terminate string (paranoia)
				update_scoreboard();		//tentando consertar bug change teams
				break;

			//text message
			case 2: {
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
					sound(SAMPLE_TALK);
				break;
			}

			//"hello" one-time server information ("first packet")
			case 3:
				readByte(msg, count, pid);	//"who am I"

				//DEBUG msg
				if (pid != whatme) {
					char lixoverde[200];
					sprintf(lixoverde, "###WARNING###: me %i memsg %i whatme %i\n", me, pid, whatme);
					send_chat(lixoverde);
				}

				me = pid;

				//reset want-change-teams: this message is send when players are swapped also
				want_change_teams = false;

				readByte(msg, count, abyte);	//team 0 score
				fx.flag[0].score = abyte;
				readByte(msg, count, abyte);	//team 1 score
				fx.flag[1].score = abyte;

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

				//update scoreboard!
				update_scoreboard();

				break;

			//frags update
			case 4:
				readByte(msg, count, pid);	// what player
				readLong(msg, count, fragz);	// new frag value
				player[pid].frags = fragz;
				update_scoreboard();		// update clientside scoreboard
				break;

			//CTF flag update
			case 6:
				readByte(msg, count, team);	// team of the flag
				readByte(msg, count, carried); // 0==not carried 1==carried
				if (carried == 0) {
					fx.flag[team].carried = false;
					//not carried: update position
					readByte(msg, count, abyte);		//px
					fx.flag[team].pos.px = abyte;
					readByte(msg, count, abyte);		//py
					fx.flag[team].pos.py = abyte;
					readShort(msg, count, ashort);	//x
					fx.flag[team].pos.x = ashort;
					readShort(msg, count, ashort);	//y
					fx.flag[team].pos.y = ashort;
				}
				else {
					fx.flag[team].carried = true;
					//carried: get carrier
					readByte(msg, count, abyte);	//carrier
					fx.flag[team].carrier = abyte;

					sound(SAMPLE_CTF_GOT);
				}
				break;

			//rocket fire notification
			case 7:

				// add to clientside rocket objects list
				//
				//readByte(lebuf, count, rpowdir);	// rocket powerdir
				readByte(lebuf, count, rpow);	// rocket powerdir
				readByte(lebuf, count, rdir);	// rocket powerdir

				//para cada pow, tira um id de shot pra alocar
				for (k=0;k<rpow;k++) {
					readByte(lebuf, count, rockid);	// rocket powerdir
					rids[k] = (int)rockid;
				}

				readLong(lebuf, count, frameno);	// frame # of shot
				readByte(lebuf, count, rowner);	// owner
				readByte(lebuf, count, rpx); //px
				readByte(lebuf, count, rpy); //py
				readShort(lebuf, count, rx); //x
				readShort(lebuf, count, ry); //y

				//rebuild client-side shot
				client_rebuild_shot(rpow, rdir, rids, frameno, rowner, rpx, rpy, rx, ry);

				//play sound if rocket on screen
				if (me >= 0)
				if (rpx == player[me].x)
				if (rpy == player[me].y) {

					//fir sound
					sound(SAMPLE_FIRE);

					//if owner is quaded, play quad sound
					if (player[rowner].item_quad) {
						if (get_time() > player[rowner].quad_sound_finished) {
							sound(SAMPLE_QUAD_FIRE);
							player[rowner].quad_sound_finished = get_time() + 0.1;		//0.3.9 : quad_sound disabled, play always
						}
					}
				}
				break;

			//rocket deletion notification
			case 8:
				readByte(lebuf, count, rockid);	// rocket object id
				readByte(lebuf, count, abyte);	// target player
				//hit position
				readShort(lebuf, count, rokx);
				readShort(lebuf, count, roky);
				fx.rock[rockid].hitx = rokx;
				fx.rock[rockid].hity = roky;
				//signal died
				fx.rock[rockid].hit_time = get_time(); //die now
				//target
				fx.rock[rockid].hit_target = abyte;
				break;

			//CTF team score update
			case 9:
				readByte(lebuf, count, abyte);		//team
				readByte(lebuf, count, rockid);		//new score
				fx.flag[abyte].score = rockid;	// update the score
				break;

			//sound event
			case 14:
				readByte(lebuf, count, abyte);		// sample #
				sound(abyte);
				break;

			//pickup visible
			case 15:
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
			case 16:
				readByte(lebuf, count, iid);
				fx.item[iid].kind = 0;		// no more!
				break;

			//powerup clientside timer set
			case 17:
				readByte(lebuf, count, iid);	//kind
				readShort(lebuf, count, usho);	//amount of time
				if (me >= 0) {
					if (iid == 2)
						player[me].item_speed_time = get_time() + ((double)usho);
					else if (iid == 3)
						player[me].item_helm_time = get_time() + ((double)usho);
					else if (iid == 4)
						player[me].item_quad_time = get_time() + ((double)usho);
				}
				break;

			//my weapon notify change
			case 18:
				readByte(lebuf, count, abyte);	// weapon level
				if (me >= 0) {
					player[me].weapon = abyte;
				}
				break;

			//server commands client to change map
			case 20:
				map_ready = false;	// map NOT ready anymore: must load/change
				want_map_exit =false;		// and player does not want to exit the map anymore
				readByte(lebuf, count, abyte);			// read map kind (1=builtin 2=custom)
				if (abyte == 2) {
					readShort(lebuf, count, usho);				//read CRC16 of map
					readString(lebuf, count, mapname);		//read map name
					server_map_command(mapname, usho);
				}
				else {
					//FIXME: unknown map kind
				}
				for (int iid=0; iid<MAX_PICKUPS; ++iid)
					fx.item[iid].kind = 0;
				break;

			//server shows gameover plaque
			case 24:
				readByte(lebuf, count, abyte);
				gameover_plaque = abyte;		// kind of plaque (capture limit or vote exit)
				if (gameover_plaque == NEXTMAP_CAPTURE_LIMIT || gameover_plaque == NEXTMAP_VOTE_EXIT) {
					readByte(lebuf, count, abyte);	//RED team final score
					red_final_score = abyte;
					readByte(lebuf, count, abyte);  //BLUE team final score
					blue_final_score = abyte;
				}
				break;

			//server hides gameover plaque
			case 25:
				gameover_plaque = NEXTMAP_NONE;		//hide
				break;

			//deathbringer shot
			case 26:
				//print_message("DEATHBRINGER!!!");
				readByte(lebuf, count, abyte);	//what player
				readLong(lebuf, count, frameno);		// start time
				//spawn clientside fx at the owner's position
				//V0.4.6: sending explicitly the screen/coord of the shot
				readByte(lebuf, count, sx);
				readByte(lebuf, count, sy);
				readShort(lebuf, count, hx);
				readShort(lebuf, count, hy);
				cfx_create_deathbringer(abyte, get_time() + (fx.frame - frameno) * 0.1, hx, hy, sx, sy);

				//cfx_create_deathbringer(abyte, get_time() + (fx.frame - frameno) * 0.1, (int)fx.hero[abyte].x, (int)fx.hero[abyte].y, player[abyte].x, player[abyte].y);
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
				writeShort(lebuf, count, ((NLushort)world.hero[target].x));
				writeShort(lebuf, count, ((NLushort)world.hero[target].y));
				writeLong(lebuf, count, frame);		//frame # of the bringer shot (message can be delayed)
				*/
				//sprintf(debuf, "t %i sx %i sy %i x %i y %i
				//print_message
				break;

			//v0.4.4: UDP FILE DOWNLOAD: incoming chunk
			case 28:
				readByte(lebuf, count, abyte);		//"last chunk"?
				readLong(lebuf, count, frameno);	//absolute pos of the chunk on file
				readShort(lebuf, count, usho);		//chunk size
				//PROCESS IT
				process_udp_download_chunk(abyte, frameno, usho, &(lebuf[count]));
				break;

			//v0.4.4: registration response from server
			case 31:
				readByte(lebuf, count, abyte);
				if (abyte == 1) {
					//success!
					strcpy(namestatus, "RECORDING (");
					strcat(namestatus, player_token);
					strcat(namestatus, ")");
					//code
					namestatus_code = 3;
				}
				else {
					//fail
					strcpy(namestatus, "REJECTED (");
					strcat(namestatus, player_token);
					strcat(namestatus, ")");
				}

				break;

			//v0.4.5: CRAPZ UPDATE message -- updates lots of crap about a player
			case 32:
				readByte(lebuf, count, pid);			//waht player slot
				readByte(lebuf, count, abyte);		//reg char
				readLong(lebuf, count, prank);		//ranking#
				readLong(lebuf, count, pscore);		//score
				readLong(lebuf, count, nscore);		//score	NEG v0.4.8
				readLong(lebuf, count, max_world_rank);		//world players count
				readLong(lebuf, count, max_world_score);		//world score max
				player[pid].reg_status = (char)abyte;
				player[pid].rank = (int)prank;
				player[pid].score = (int)pscore;
				player[pid].neg_score = (int)nscore;
				//LOG4("CRAPZ UPDATE %i %c %i %i\n", pid, abyte, prank, pscore);
				break;

			default:
				LOG1("BAD ERROR: UNKNOWN SERVER MESSAGE CODE = %i!!\n", code);
				break;
			}
		}
	} while (msg != 0);

	// (3) calc stuff dependant on received data
	//

	//detect screen changes / clear powerup fx's
	if (me >= 0)
	if ((player[me].x != player[me].oldx) || (player[me].y != player[me].oldy)) {
		//screen changed.

		//V0.4.7: now, why would I want to do that??
		//clear all clientside fx's
		//for (int f=0;f<MAX_CLIENTFX;f++)
		//	cfx[f].used = false;

		//clear all powerups on my old screen
		for (int j=0;j<MAX_PICKUPS;j++)
		if (fx.item[j].kind)	// item present
		if (fx.item[j].px == player[me].oldx)	//on old screen
		if (fx.item[j].py == player[me].oldy)
			fx.item[j].kind = 0;	// erase

		//set new old's
		player[me].oldx = player[me].x;
		player[me].oldy = player[me].y;
	}

	//this is a HACK:
	me = whatme;

	//unlock frame mutex
	//LOG("unlocking INCOMING\n");
	pthread_mutex_unlock( &frame_mutex );
	//LOG("unlocked! INCOMING\n");
}

//send chat message
void gameclient_c::send_chat(char *msg) {
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, 2);	//want to chat!
	writeString(lebuf, count, msg);	// the message
	client->send_message(lebuf, count);
}

void gameclient_c::erase_first_message() {
	for (int i=1; i<CHAT_SIZE; ++i)
		strcpy(chatbuffer[i-1], chatbuffer[i]);
	chatbuffer[CHAT_SIZE-1][0] = '\0';
	chaterasetime = get_time() + 10.0;
}

//print message to "console"
void gameclient_c::print_message(const char *msg) {
	int i;
	for (i=CHAT_SIZE-1; i>0; --i)
		if (chatbuffer[i][0]=='\0')
			break;
	// i points to first shift target (0 if no spaces were found)
	for (++i; i<CHAT_SIZE; ++i)
		strcpy(chatbuffer[i-1], chatbuffer[i]);
	strcpy(chatbuffer[CHAT_SIZE-1], msg);
	chaterasetime = get_time() + 10.0;
}

//save screenshot
void gameclient_c::save_screenshot() {

	// find the filename
	char fname[256];
	int i = 0;
	do {
		if (i<10) sprintf(fname, "outgun0%i.tga", i); else sprintf(fname, "outgun%i.tga", i);
		if (!exists(fname))
			break;
		i++;
	} while (i < 99);

	//dump
BITMAP *bmp;
PALETTE pal;
get_palette(pal);
bmp = create_sub_bitmap(screen, 0, 0, SCREEN_W, SCREEN_H);
save_bitmap(fname, bmp, pal);
destroy_bitmap(bmp);

	//nice message
	char lixox[256];
	sprintf(lixox, "saved screenshot to %s.", fname);
	print_message(lixox);
}

//toggle help screen
void gameclient_c::toggle_help() {

	helpshow = !helpshow;

	if (helpshow)
		sound(SAMPLE_WEAPON_UP);
	else
		sound(SAMPLE_FIRE);
}

//show progress (for tight loops that don't work with the regular screen flip loop)
void gameclient_c::show_progress(char *t1, char *t2, char *t3, int fg, int bg) {
	if (fg == -1) fg = col[COLWHITE];
	vsync();
	acquire_screen();
	rect(screen, 320 - 200-1, 240 - 50-1, 320 + 200-1, 240 + 50-1, col[COLWHITE]);
	rect(screen, 320 - 200+1, 240 - 50+1, 320 + 200+1, 240 + 50+1, col[COLDARKGRAY]);
	rectfill(screen, 320 - 200, 240 - 50, 320 + 200, 240 + 50, bg);
	textprintf_centre(screen, font, 320, 240 - 25, fg, t1);
	textprintf_centre(screen, font, 320, 240     , fg, t2);
	textprintf_centre(screen, font, 320, 240 + 25, fg, t3);
	release_screen();
}

//show progress / press any key / dialog
void gameclient_c::show_dialog(char *t1, char *t2, char *t3, int fg, int bg) {
	clear_keybuf();
	do {
		show_progress(t1,t2,t3,fg,bg);
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

	int i = 0;
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

	show_progress("Getting updated internet server list", "Contacting server...", "Press ESC to cancel");

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

	show_progress("Getting updated internet server list", "Waiting response...", "Press ESC to cancel");

	//log ok
	LOG3("QUERY TO MASTER '%s', result = %i, count = %i\n", querybuf, result, qcount);

	//FIXME: show 'request sent... waiting reply'

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
	for (int j=0;j<MAX_GAMESPY;j++) {
		mgamespy[j].address[0]=0;
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
			for (i=0;i<MAX_GAMESPY;i++)
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
	menushow = true;
	set_menu(0);
	gameshow = false;

	//reset speed counter
	speed_counter = 0;
	client_netsend_counter = 0;

	//loop
	bool quick_fix, key_fire=false, key_kill=false, key_swap=false, key_votexit=false;
	char key_up=0, key_down=0, key_left=0, key_right=0;
	int i;
	while (notquit) {

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
				}
			}
			//menu keypresses (from char buf) - ESC already dealed with, ignore
			else if (menushow) {
				while (keypressed()) {
					string lerud_abloxon;

					//get key
					int ch = readkey();
					int sc = ch >> 8;	//scancode
					ch = ch & 0xff;			//char

					//screenshot
					if (sc == KEY_F11) {
						save_screenshot();
					}

					//toggle help
					if (sc == KEY_F1)
						toggle_help();

					//test key
					switch (menu) {
					//main menu
					case 0:
						if ((key[KEY_SPACE]) && (sc == KEY_F8)) {
							port++;
							if (port > 25005)
								port = 25000;
						}
						else if (sc == KEY_F10) {
							lerud_abloxon = RandomName();
							strcpy(editplayername, lerud_abloxon.c_str());
							change_name_command();
						}
						switch (ch) {
						case '1': set_menu(1); break;
						case '2': disconnect_command(); break;
						case '3':
							strcpy(editplayername, playername);
							strcpy(editplayerpass, player_password);
							strcpy(namecursor, "_");
							strcpy(passcursor, "");
							set_menu(3);
							break;
						case '4': // start/stop listenserver
							if (listen_server_running)
								listen_stop();
							else
								listen_start();
							break;
						case '5':
							winclient = !winclient;
							reset_video_mode();
							break;
						case '6': next_sfx_theme(); break;
						default:;
						}
						break;
					//connect screen
					case 1:
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
						else if (sc == KEY_ENTER) {
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
					case 2:
						break;
					//change name screen
					case 3:

						if (namecursor[0]=='_')
							i = strlen(editplayername);
						else
							i = strlen(editplayerpass);

						if (((ch >= '0') && (ch <= '9')) || ((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z')) || (ch == '-') || (ch == '_')) {

							if (namecursor[0]=='_') {
								if (i < 15) {
									editplayername[i] = (char)ch;
									editplayername[i+1] = 0;
									editplayerpass[0]=0; //reset password after editing name
								}
							}
							else {
								if (i < 8) {
									editplayerpass[i] = (char)ch;
									editplayerpass[i+1] = 0;
								}
							}
						}
						else if (sc == KEY_BACKSPACE) {
							if (i > 0) {
								if (namecursor[0]=='_') {
									editplayername[i-1] = 0;
									editplayerpass[0]=0; //reset password after editing name
								}
								else
									editplayerpass[i-1] = 0;
							}
						}
						else if (sc == KEY_ENTER) {
							change_name_command();
							//get it (V0.4.9) -- força um "ENTER" logo apos o cara conectar ou trocar de nome ao estar
							//conectado...
							check_change_pass_command();
						}
						else if (sc == KEY_TAB) {
							//switch fields
							if (namecursor[0]==0) {
								strcpy(namecursor, "_");
								strcpy(passcursor, "");
							}
							else {
								strcpy(namecursor, "");
								strcpy(passcursor, "_");
							}
						}
						break;
					//wtf?
					default:;
					}
				}
			}
			// menu not showing: send keypresses to the game
			else {

				bool sendnow = false;

				// ctrl == fire event
				//
				if (key[KEY_LCONTROL] || key[KEY_RCONTROL]) {
					if (!key_fire) {
						key_fire = true;

						//"fire" message (+ATTACK)
						char lebuf[16]; int count = 0;
						writeByte(lebuf, count, 5);	// 5 = +attack (fire button down)
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
						writeByte(lebuf, count, 11);	// 11 = -attack (fire button up)
						client->send_message(lebuf, count);

						//send early keys packet
						sendnow = true;
					}
				}

				// del == suicide event
				//
				if (key[KEY_DEL]) {
					if (!key_kill) {
						key_kill = true;

						//"suicide" message
						char lebuf[16]; int count = 0;
						writeByte(lebuf, count, 10);	// 10 = suicide!!
						client->send_message(lebuf, count);
					}
				}
				else key_kill = false;

				// page down == drop flag
				//
				if (key[KEY_PGDN]) {
					char lebuf[16]; int count = 0;
					writeByte(lebuf, count, 34);	// 34 = drop flag
					client->send_message(lebuf, count);
				}

				// end == want/don't want swap team
				//
				if (key[KEY_END]) {
					if (!key_swap) {
						key_swap = true;

						//toggle my local option
						want_change_teams = !want_change_teams;

						//want to swap/dont want  message
						char lebuf[16]; int count = 0;
						if (want_change_teams)
							writeByte(lebuf, count, 12);	// 12 -- want
						else
							writeByte(lebuf, count, 13);	// 13 -- dont want
						client->send_message(lebuf, count);
					}
				}
				else key_swap = false;

				// F4 == want/don't want to exit map
				//
				if (key[KEY_F4]) {
					if (!key_votexit) {
						key_votexit = true;

						//toggle my local option
						want_map_exit = !want_map_exit;

						//want to swap/dont want  message
						char lebuf[16]; int count = 0;
						if (want_map_exit)
							writeByte(lebuf, count, 22);	// 22 -- want
						else
							writeByte(lebuf, count, 23);	// 23 -- dont want
						client->send_message(lebuf, count);
					}
				}
				else key_votexit = false;

				// l,r,u,d,fire game keys
				//
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

				//send client's input packet now
				if (sendnow) {
					//V0.3.9: just speed up increasing the netsend counter if it's too small
					//send_frame();
					//client_netsend_counter = 3;	//should be set to 0 but we want to send a new packet soon after this one
					if (client_netsend_counter < 4)
						client_netsend_counter = 4;
				}

				// keypresses to talk prompt
				//
				while (keypressed()) {
					//get key
					int ch = readkey();
					int sc = ch >> 8;	//scancode
					ch = ch & 0xff;			//char

					i = strlen(talkbuffer);

					//toggle help
					if (sc == KEY_F1)
						toggle_help();

					//change colours
					if (sc == KEY_HOME) {
						flagpos_ready = false;	// flag position mark colour not right anymore
						if (key[KEY_LCONTROL] || key[KEY_RCONTROL]) {
							col[COLGROUND] = makecol(0x10, 0x40, 0);
							col[COLWALL] = makecol(0x30, 0xC0, 0);

							//col[COLGREEN] = makecol(0,0xff,0);
						}
						else {
							col[COLGROUND] = makecol(rand() % 256, rand() % 256, rand() % 256);
							col[COLWALL] = makecol(rand() % 256, rand() % 256, rand() % 256);

							/*
							int r,g,b;
							r = rand() % 256;
							g = rand() % 256;
							b = rand() % 256;

							col[COLGREEN] = makecol(r,g,b);

							char mes[200];
							sprintf(mes, "%3i %3i %3i", r,g,b);
							print_message(mes);
							*/
						}
					}

					// ins == change name
					//
					if (sc == KEY_F10) {
						string lerud_abloxon = RandomName();
						strcpy(editplayername, lerud_abloxon.c_str());
						change_name_command();
					}
					else if (sc == KEY_PGUP) {
						//v0.4.9 DEBUGGING: request broadcast my crap
						char lebuf[4]; int count = 0;
						writeByte(lebuf, count, 33);		// 33 = "refresh crap"
						client->send_message(lebuf, count);
					}
					else if (sc == KEY_F3) {
						option_show_names = !option_show_names;
					}
					// F11:screenshot
					else if (sc == KEY_F11) {
						save_screenshot();
					}
					//backspace erase one
					else if (sc == KEY_BACKSPACE) {
						if (i>0) talkbuffer[i-1] = 0;
					}
					//enter key: submit text
					else if (sc == KEY_ENTER || sc == KEY_ENTER_PAD) {
						if (i > 0) {
							send_chat(talkbuffer);
							talkbuffer[0]=0;
						}
					}
					//else text add keys. max text length = 60
					else if (i < 60 && ch >= 32) {
						talkbuffer[i] = (char)ch;
						talkbuffer[i+1] = 0;
					}
				}
			}

			//ESC = show/hide menu, go back menu (special key)
			static bool kesc = false;
			if (key[KEY_ESC]) {
				if (!kesc) {
					kesc = true;
					if (talkbuffer[0] != '\0') // cancel chat
						talkbuffer[0] = '\0';
					else if (trying_connection) {		//trying connection
						trying_connection = false;	//not anymore

						//this cancels the attempt to connect
						client->connect(false);
						//go back to main screen
						set_menu(0);
					}
					//se mostrando help, quita
					else if (helpshow) {
						toggle_help();
					}
					else if (!menushow) {		// no menu, show
						menushow = true;
						set_menu(0);
					}
					else {		// menu
						if (menu > 0)		// go back one screen
							set_menu(0);
						else						// hide menu
							menushow = false;
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

		//speed_counter == 60 FPS
		// client netsend == 10 FPS
		// so when netsend == 6, it's reset to zero and the packet is sent
		if (client_netsend_counter >= (targetfps / 10) ) {
			client_netsend_counter = 0;
			send_frame();
		}

		// (2) if game is showing, simulate
		//
		if (gameshow) {
			//LOG("** ...calc game frame\n");
			calc_game_frame(); //calculate game frame to show
			//LOG("** ...game frame calced ok\n");
		}

		// (3) draw operations
		//
		//if (page_flipping) {
			//LOG1("acquire_bitmap/gs=%i...",gameshow);
			acquire_bitmap(drawbuf);
			//LOG("OK\n");
		//}

		if (gameshow) {
			//clear_to_color(drawbuf, makecol(rand(),0,0));	// clear buffer
			//LOG("draw_game_frame()\n");
			draw_game_frame(drawbuf); // draw game frame
			//LOG("exit draw_game_frame()\n");
		} else {
			clear_to_color(drawbuf, 0);	// clear buffer
			int co = makecol(0x22, 0x22, 0x22);
			textprintf(drawbuf, font, 0, 0, co, "page-flipping = %i", page_flipping);
			textprintf(drawbuf, font, 0, 10, co, "port = %i", port);
		}

		//force menu open if no game
		if (!gameshow)
			menushow = true;

		if (menushow)
			draw_game_menu();	// draw the game menu

		if (helpshow)
			draw_game_help();	// draw help

		//if (page_flipping) {
			//LOG("** releasing bitmap...");
			release_bitmap(drawbuf);
			//LOG("OK!\n");
		//}

		// (4) flip or blt
		//
		if (page_flipping) {
			show_video_bitmap(drawbuf);

			if (drawbuf == vidpage1)
				drawbuf = vidpage2;
			else
				drawbuf = vidpage1;
		}
		else {
			acquire_screen();
			blit(drawbuf, screen, 0,0, 0,0, SCREEN_W,SCREEN_H);
			release_screen();
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

	//clear all samples
	unload_samples();

	for (int i = 0; i < 2; i++)
		destroy_bitmap(flagpos_buf[i]);

	//save configuration file
	//try to load client configuration
	char dest[WHERE_PATH_SIZE];
	append_filename(dest, wheregamedir, "clconfig.txt", WHERE_PATH_SIZE);
	LOG1("dest for clconfig.txt OUT = %s\n", dest);

	FILE *cfg = fopen(dest, "w");
	if (cfg) {

		//0.4.7: no theme dir?
		if (validtheme)
			fprintf(cfg, "%s\n", sfxthemedir);
		else
			fprintf(cfg, "NO_SFX_THEME_DIR\n");

		//v0.4.7: empty name?
		if (playername[0] != '\0')
			fprintf(cfg, "%s\n", playername);
		else
			fprintf(cfg, "Unnamed_Bastard\n");

		for (int i=0;i<MAX_GAMESPY;i++) {
			LOG1("SAVING GAMESPY ADDRESS = '%s'\n", gamespy[i].address);
			fprintf(cfg, "%s\n", gamespy[i].address);
		}
		fclose(cfg);
	}

	//save client's password
	LOG("Saving password file...\n");
	append_filename(dest, wheregamedir, "password.bin", WHERE_PATH_SIZE);
	FILE *psf = fopen(dest, "wb");
	if (psf) {

		char cha;
		for (int c=0;c<PASSBUFFER;c++) {
			if (c < (int)strlen(player_password)) {
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
}

//ctor
gameclient_c::gameclient_c() {

	//net client
	client = 0;

	//not showing
	option_show_names = false;

	//all the players to show including me
	//player_t player[MAX_PLAYERS];
	for (int p=0;p<MAX_PLAYERS;p++)
		player[p].used=false;

	//explosion fx's
	for (int x=0;x<MAX_CLIENTFX;x++)
		cfx[x].used = false;

	//wich player I am
	me = -1;

	//time of last packet received
	lastpackettime=0;

	//audio samples
	for (int s=0;s<NUM_OF_SAMPLES;s++)
		sample[s]=0;

	//menu showing?
	menushow = false;
	menu = 0;		//menu screen #

	//game showing?
	gameshow = false;

	//frames and seconds for FPS counter
	FPS=0;
	framecount = 0;
	totalframecount = 0;
	starttime = 0;

	//if player wants to changeteams
	want_change_teams = false;

	//trying connection? if true, ESC cancels it
	trying_connection = false;

	//connected? (that is, "connection accepted")
	connected = false;

	//connect screen, my "mini-gamespy"
	gi=0;	//what game entry

	playername[0]=0; //the player's name (max name len = 16)
	namestatus[0]=0;
	editplayername[0]=0; //the player's name edit buffer
	address[0]=0;		//server IP address
	dialogmessage[0]=0;	//dialog message
	dialogmessage2[0]=0;	//dialog message line 2
	talkbuffer[0]=0;			// chat input buffer
	for (int i=0;i<CHAT_SIZE;i++)	// last chat messages list
		chatbuffer[i][0]=0;
	chaterasetime = 0;				// time to erase a chat message from the list
	minibg = 0;		//bitmap for minimap background (level walls, screen grid)

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

void gameclient_c::check_flagpos_marks() {
	if (!flagpos_ready) {
		const int radius = CL_FLAGPOS_RAD;
		for (int i = 0; i < 2; i++) {
			if (!flagpos_buf[i])
  				flagpos_buf[i] = create_bitmap(2 * radius, 2 * radius);
			clear_to_color(flagpos_buf[i], col[COLGROUND]);
		}
		int r1, r2;
		int b1, b2;
		r1 = r2 = getr(col[COLGROUND]);
		const int g = getg(col[COLGROUND]);
		b1 = b2 = getb(col[COLGROUND]);
		const int step = 10;
		for (int i = radius; i >= 0; i--) {
			// red flag
			r1 = min(r1 + step, 255);
			circlefill(flagpos_buf[0], radius, radius, i, makecol(r1, g, b1));
			// blue flag
			b2 = min(b2 + step, 255);
			circlefill(flagpos_buf[1], radius, radius, i, makecol(r2, g, b2));
		}
		flagpos_ready = true;
	}
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
//  client "download file from server" thread
//============================================================

void *thread_clientdownloader_f(void *arg) {

	gameclient->client_download_thread(arg);

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

