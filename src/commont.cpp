#include "commont.h"

char* strspnp(char* str, const char* charset) {
	for (; *str; ++str)
		if (strchr(charset, *str)==NULL)
			return str;
	return NULL;
}

const char* strspnp(const char* str, const char* charset) {
	return strspnp(const_cast<char*>(str), charset);
}

FILE *game_log;
NLaddress		master_address;
char wheregamedir[WHERE_PATH_SIZE];
double svp_fric, svp_accel, svp_maxspeed;
double svp_fric_run, svp_accel_run, svp_maxspeed_run;
double svp_fric_turbo, svp_accel_turbo, svp_maxspeed_turbo;
double svp_fric_turborun, svp_accel_turborun, svp_maxspeed_turborun;
double svp_flag_penalty;

void set_default_physics() {
	svp_fric = 1.5;
	svp_accel = 2.0;
	svp_maxspeed = 12.0;
	svp_fric_run = 1.5;
	svp_accel_run = 2.0;
	svp_maxspeed_run = 22.0;
	svp_fric_turbo = 3.0;
	svp_accel_turbo = 4.0;
	svp_maxspeed_turbo = 18.0;
	svp_fric_turborun = 3.0;
	svp_accel_turborun = 4.0;
	svp_maxspeed_turborun = 33.0;
	svp_flag_penalty = 3.0;
}

int			maxplayers = MAX_PLAYERS;		// the maximum number of players configured for this server (must be <= MAX_PLAYERS and an EVEN NUMBER == NUMERO PAR)

bool dedserver = false;		//dedicated server? -ded
bool textserver = false;		//textmode dedicated server for UNIX/LINUX (V0.5.0) (WON'T WORK ON WINDOWS...)
bool privateserver = false;	//private server? (will not publish)
bool winclient = true;		//windowed client?	-win / -fs
bool trypageflip = false;	//try page flipping? -flip / -dbuf
bool nosound = false;			//disable sound? -nosound
int targetfps = 60;			//target (MAX) frames-per-second
int port = DEFAULT_UDP_PORT;				//the server port
bool showinfo = false;		//apenas show info e desliga server
bool defaultprio = false;	//select default server threads priority
int targetprio = TARGET_PRIO_UNSPECIFIED;	//unspecified
int server_maxplayers = 16;		//default maxplayers of the server
bool sound_inited = false;		//install_sound succeeded?
bool sound_enabled = true;		// player wants sounds?
bool no_tcp_download = true;		// V0.4.7: CHANGED DEFAULT : disable use of the TCP socket for file transfers (use the regular UDP leetnet connection)
bool force_ip = false;		//force IP?
char force_ip_name[32];		//force IP to what?

void server_status_string(char *str) {
	if (textserver)
		printf("%s\n",str);
	else
		set_window_title(str);
}

//server timer (10Hz)
volatile int server_speed_counter = 0;
void increment_server_speed_counter()
{
	server_speed_counter++;
}
END_OF_FUNCTION(increment_server_speed_counter);

//client game timer (60Hz)
volatile int speed_counter = 0;
volatile int client_netsend_counter = 0;		//sub-counter for keypress sending
void increment_speed_counter()
{
	speed_counter++;
}
END_OF_FUNCTION(increment_speed_counter);

//this timer will be used to emulate a better clock()
volatile unsigned long time_counter = 0;
void increment_time_counter() {
	time_counter++;
}
END_OF_FUNCTION(increment_time_counter);

double get_time() {
	return ((double)time_counter) / 200.0;
}

int teamcol[2];
int teamlcol[2];	//light colours for statusbar
int teamdcol[2];	//dark colours for player name

char teamname[2][5];

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

/* subIntersection:
 * returns true if the area between lines (lx1,ly1)-(lx2,ly2) and (rx1,ry1)-(rx2,ry2) intersects the rectangle (rectx1,recty1)-(rectx2,recty2)
 * every line must be y-ordered : ly1<=ly2, ry1<=ry2, recty1<=recty2 ; additionally rectx1<=rectx2 and lx(y)<=rx(y) for all applicable y
 *
 * how it works:
 * for the appropriate range of y, if there exists an y so that lx(y)<=rectx2 AND rx(y)>=rectx1 , there is an intersection in that range
 * those ranges are solved with simple linear equasions since lx and rx are linear
 */
bool subIntersection(float lx1, float ly1,  float lx2, float ly2,  float rx1, float ry1,  float rx2, float ry2,
				float rectx1, float recty1, float rectx2, float recty2) {
	assert(ly1<=ly2 && ry1<=ry2);
	float miny = max(max(ly1, ry1), recty1), maxy=min(min(ly2, ry2), recty2);
	if (maxy < miny)
		return false;
	// first narrow the range by lx(y) <= rectx2
	if (lx1 == lx2) {	// can't formulate a value for intersection-y
		if (lx1 > rectx2)	// lx(y) <= rectx2 identically false => no solutions
			return false;
		// lx(y) <= rectx2 identically true => no narrowing from lx
	}
	else {
		// solve lx(y) == rectx2 , where lx(y) = lx1 + (y-ly1)*(lx2-lx1)/(ly2-ly1)
		float intersect_y = (rectx2 - lx1) * (ly2 - ly1) / (lx2 - lx1) + ly1;
		if (lx2 > lx1) {	// the intersection is at y <= intersect_y
			if (maxy > intersect_y)
				maxy = intersect_y;
		}
		else {	// the intersection is at y >= intersect_y
			if (miny < intersect_y)
				miny = intersect_y;
		}
	}
	if (maxy < miny)
		return false;
	// now narrow the range further by rx(y) >= rectx1, similarly
	if (rx1 == rx2)
		return (rx1 >= rectx1);
	else {
		float intersect_y = (rectx1 - rx1) * (ry2 - ry1) / (rx2 - rx1) + ry1;
		if (rx2 > rx1) {	// the intersection is at y >= intersect_y
			if (miny < intersect_y)
				miny = intersect_y;
		}
		else {	// the intersection is at y <= intersect_y
			if (maxy > intersect_y)
				maxy = intersect_y;
		}
	}
	return (maxy >= miny);
}

bool TriWall::intersects_rect(float rx1, float ry1, float rx2, float ry2) const {
	assert(ry1<=ry2 && rx1<=rx2);
	assert(y1<=y2 && y2<=y3);
	if (rx1>boundx2 || rx2<boundx1 || ry1>boundy2 || ry2<boundy1)
		return false;
	/* idea: triangle is split in two triangles: y<=y2 and y>=y2
	 * for both parts, the right and left edge are checked separately
	 * for each edge there may be a region [yi0, yi1] where (for a right side edge) xr(y)>=x1
	 * if those regions overlap with each other and [ry1, ry2], there exists an intersection
	 */
	if (x2 < x1 + (y2-y1) * (x3-x1) / (y3-y1)) {	// p2 is left to the p1-p3 line
		if (subIntersection(x1,y1, x2,y2, x1,y1, x3,y3, rx1, ry1, rx2, ry2))	// part y<=y2 : L,R sides p1-p2, p1-p3
			return true;
		if (subIntersection(x2,y2, x3,y3, x1,y1, x3,y3, rx1, ry1, rx2, ry2))	// part y>=y2 : L,R sides p2-p3, p1-p3
			return true;
	}
	else {
		if (subIntersection(x1,y1, x3,y3, x1,y1, x2,y2, rx1, ry1, rx2, ry2))	// part y<=y2 : L,R sides p1-p3, p1-p2
			return true;
		if (subIntersection(x1,y1, x3,y3, x2,y2, x3,y3, rx1, ry1, rx2, ry2))	// part y>=y2 : L,R sides p1-p3, p2-p3
			return true;
	}
	return false;
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

// this simple task is turning into a major headache...
//
bool set_shitty_mode() {

	//V0.5.0 : -text  flag
	if (textserver) return true;

	int DTC = desktop_color_depth();

	set_color_depth( DTC );

	if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 240, 0, 0))
		LOG1("ERROR: could not set gfx mode 320x240 windowed.. try 1 with %i", DTC)
	else
		return true;	// OK

	if ((DTC == 16) || (DTC == 15)) {

		if (DTC == 15)
			DTC = 16;
		else
			DTC = 15;

		set_color_depth( DTC );

		if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 240, 0, 0))
			LOG1("ERROR: could not set gfx mode 320x240 windowed.. try 2 with %i", DTC)
		else
			return true;	// OK
	}

	// the last hope. WARNING: this can be buggy for multiple dedicated servers.
	//
	DTC = 8;

	set_color_depth( DTC );

	if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 240, 0, 0)) {
		LOG1("ERROR: could not set gfx mode 320x240 windowed.. tried with %i", DTC);
		return false;	//GIVE UP
	}
	else
		return true;	// OK AT LAST!!!
}

// gameclient instance
gameclient_c *gameclient;
// gameserver instance
gameserver_c *gameserver;

int main(int argc, char *argv[]) {
	//random random
	srand((unsigned)time(0));

	strcpy(teamname[0], "RED");
	strcpy(teamname[1], "BLUE");

	// general init
	//
	gameclient = 0;
	gameserver = 0;

	//LOG_OPEN("outgun.log");

	// Set the text encoding format for Allegro as 8 bit Ascii
	set_uformat(U_ASCII);

	allegro_init();
	install_keyboard();
	install_timer();

	// find out where we are
	//
	char *exespec = new char[2048];
	get_executable_name(exespec, 2048);
	replace_filename((char*)wheregamedir, (const char *)exespec, "", 256); //Replaces the specified path+filename with a new filename tail, storing at most size bytes into the dest buffer. Returns a copy of the dest parameter
	delete exespec;
	exespec = 0;

	char lognamebuf[1024];

	//open outgun.log at EXE root path
	append_filename(lognamebuf, wheregamedir, "outgun.log", WHERE_PATH_SIZE);
	LOG_OPEN(lognamebuf);

	LOG3("OUTGUN LOG FILE. STRING %s PROTOCOL %s VERSION %s", GAME_STRING, GAME_PROTOCOL, GAME_VERSION);

	if (nlInit() == NL_FALSE) {
		LOG("ERROR: cannot init HawkNL!\n");
		return 0;
	}
	if (nlSelectNetwork(NL_IP) == NL_FALSE) {
		LOG("ERROR: no IP network!\n");
		return 0;
	}

	// enable statistics
	//
	nlEnable(NL_SOCKET_STATS);

	bool message_logging = false;

	// check args
	//
	for (int i=1;i<argc;i++) {
		if (!strcmp(argv[i], "-ded"))
			dedserver = true;
		else if (!strcmp(argv[i], "-text"))		//v0.5.0 UNIX TEXTMODE SERVER
			textserver = true;
		else if (!strcmp(argv[i], "-priv"))
			privateserver = true;
		else if (!strcmp(argv[i], "-info"))
			showinfo = true;
		else if (!strcmp(argv[i], "-defaultprio"))
			defaultprio = true;
		else if (!strcmp(argv[i], "-prio")) {
			if (++i<argc) {
				targetprio = strtol(argv[i], NULL, 10);
			}
		}
		else if (!strcmp(argv[i], "-win"))
			winclient = true;
		else if (!strcmp(argv[i], "-flip"))
			trypageflip = true;
		else if (!strcmp(argv[i], "-dbuf"))
			trypageflip = false;
		else if (!strcmp(argv[i], "-fs"))
			winclient = false;
		else if (!strcmp(argv[i], "-fps")) {
			if (++i<argc) {
				targetfps = strtol(argv[i], NULL, 10);
				if (targetfps < 1)
					targetfps = 1;
				if (targetfps > 1000)
					targetfps = 1000;
			}
		}
		else if (!strcmp(argv[i], "-maxp")) {
			if (++i<argc) {
				server_maxplayers = strtol(argv[i], NULL, 10);
				if (server_maxplayers % 2 == 1)	//ímpar: des-impariza
					server_maxplayers++;
				if (server_maxplayers < 2)
					server_maxplayers = 2;
				if (server_maxplayers > MAX_PLAYERS)
					server_maxplayers = MAX_PLAYERS;
			}
		}
		else if (!strcmp(argv[i], "-port")) {
			if (++i<argc) {
				port = strtol(argv[i], NULL, 10);
			}
		}
		else if (!strcmp(argv[i], "-nosound"))
			nosound = true;
		else if (!strcmp(argv[i], "-notcp"))
			no_tcp_download = true;
		else if (!strcmp(argv[i], "-yestcp"))
			no_tcp_download = false;
		else if (!strcmp(argv[i], "-ip")) {
			if (++i<argc) {
				force_ip = true;			//force IP
				strcpy(force_ip_name, argv[i]);	//to next parameter value
			}
		}
		else if (!strcmp(argv[i], "-log"))
			message_logging = true;
		else
			LOG2("WARNING: command-line argument #%i is unknown ('%s')\n", i, argv[i]);
	}

	// resolve master server address
	//
	LOG("resolving master server address...\n");
	try {
		nlGetAddrFromName("www.mycgiserver.com", &master_address);	//www.mycgiserver.com
	} catch (...) {
		LOG("caugth exception probably on nlGetAddrFromNameAsync()\n");
		master_address.valid = NL_FALSE;
	}

	if (master_address.valid == NL_FALSE) {
		LOG("can't resolve master server address to IP.\n");
		nlStringToAddr("212.69.162.53", &master_address);					//last known resolution for www.mycgiserver.com
	} else if (master_address.valid == NL_TRUE) {
		LOG("address resolved sucessfully.\n");
	}

	nlSetAddrPort(&master_address, 80);													//port 80
	LOG("master server address set.\n");

	// here must get the safest and shittiest windowed gfx mode available
	//
	if (set_shitty_mode() == false)
		return 0;		//if this ever executes then the world is a sucky sucky place.

	// install higher-accuracy timer interrupt
	//
	LOCK_VARIABLE(speed_counter);
	LOCK_FUNCTION(increment_time_counter);
	install_int_ex(increment_time_counter, BPS_TO_TIMER(200));		//5 ms accuracy is already 10 times better than clock()

	// install server timer (used for both dedicated and listen server)
	//
	LOCK_VARIABLE(server_speed_counter);
	LOCK_FUNCTION(increment_server_speed_counter);
	install_int_ex(increment_server_speed_counter, BPS_TO_TIMER(10));		//10Hz

	// get system thread priorities
	//
	int						pmin = sched_get_priority_min(SCHED_OTHER);
	int						pmax = sched_get_priority_max(SCHED_OTHER);
	int						policy;
	pthread_t			tme = pthread_self();
	sched_param		param;
	int						rc = pthread_getschedparam(tme, &policy, &param); // get priority of current thread (wich is the default value)
	int						pdef = param.sched_priority;
	LOG("\nThread priorities:");
	LOG4("   rc = %i policy = %i OTHER=%i sched_prio = %i\n", rc, policy, SCHED_OTHER, param.sched_priority);
	LOG3("   pmin %i pmax %i pdef = %i\n", pmin, pmax, pdef);

	//show info
	if (showinfo) {

		//get all local addresses
		NLaddress *locals;
		NLint     locsize;

		locals = nlGetAllLocalAddr(&locsize);

		char infobuf[2048];
		sprintf(infobuf, "Information:\n\nThread priorities for -prio <val> parameter:\n* Minimum -prio <val> : %i\n* Maximum -prio <val> : %i\n* System default (use -defaultprio) : %i\n\nLocal addresses:\n", pmin, pmax, pdef);

		for (int z=0;z<locsize;z++) {
			char adrs[222];
			nlAddrToString( &(locals[z]), adrs );
			strcat(infobuf, adrs);
			strcat(infobuf, "\n");
		}

		allegro_message(infobuf);
		return 0;
	}

	// run server
	//
	if (dedserver) {

		// here must get the safest and shittiest windowed gfx mode available
		//
		if (set_shitty_mode() == false)
			return 0;		//if this ever executes then the world is a sucky sucky place.

		// dedicated server - set process priority (all threads) to a higher value
		//		--> threads filhas estao com as priorities certas? LOGAR pra  ver. senao mudar p/ INHERIT
		int ptarg;

		//change default priority?
		if (!defaultprio) {

			//if -prio parameter is unspecified
			if (targetprio == TARGET_PRIO_UNSPECIFIED) {

				//guess one below system maximum (wich usually means realtime and should never be used really)
				//
				if (pmin < pmax) { // pmin ..... OI! . PMAX
					ptarg = pmax - 1;
				}
				else {             // PMAX . OI! ..... pmin
					ptarg = pmax + 1;
				}

			}
			else
				ptarg = targetprio;

			param.sched_priority = ptarg;
			policy = SCHED_OTHER;
			pthread_setschedparam(tme, policy, &param);
			LOG1("\nNEW PRIORITY VALUE SET FOR DEDICATED SERVER: %i\n", ptarg);
		}
		else
			LOG("\n-defaultprio: Leaving thread priorities on their default values");

		// log
		LOG_CLOSE();

		//open log at EXE root path
		append_filename(lognamebuf, wheregamedir, "out_svr.log", WHERE_PATH_SIZE);
		LOG_OPEN(lognamebuf);

		// gfx init
		//
		if (set_display_switch_mode(SWITCH_BACKAMNESIA) == -1) // allow running in the background
		{
			LOG("can't set SWITCH_BACKAMNESIA for SERVER");
			if (set_display_switch_mode(SWITCH_BACKGROUND) == -1) {
				LOG("ERROR: server window cannot run in the background!\n");
				return 0;
			}
			else
				LOG("set SWITCH_BACKGROUND for SERVER");
		}
		else
			LOG("set SWITCH_BACKAMNESIA for SERVER");
		setcolors();

		// run server
		//
		gameserver = new gameserver_c();
		if (!gameserver->start(server_maxplayers)) {
			LOG("ERROR: cannot start gameserver!\n");
			return 0;
		}
		gameserver->loop(0);
		gameserver->stop();
		delete gameserver;
		gameserver = 0;
	}
	// run client
	//
	else {

		//require CLIENT_MAPS_DIR directory
		char mappath[256];
		strcpy(mappath, CLIENT_MAPS_DIR);  // cmaps
		char dest[1024];
		append_filename(dest, wheregamedir, mappath, WHERE_PATH_SIZE);	// <FULL-DIR>/maps/*.txt, I hope
		LOG1("CMAPS DIR IS = '%s'\n", dest);
		al_ffblk mapffblk;	//for al_find*
		int result = al_findfirst(dest, &mapffblk, FA_DIREC|FA_ARCH|FA_RDONLY);
		if (result != 0 || !(mapffblk.attrib&FA_DIREC)) {
			allegro_message("Error: directory '%s' not found\n\nPlease create this directory.\n\nThe game cannot run without it.", dest);
			return 0;
		}

		//window title
		server_status_string("Outgun client - CTRL+F12 to quit");

		// log
		LOG_CLOSE();

		//open log at EXE root path
		append_filename(lognamebuf, wheregamedir, "out_cli.log", WHERE_PATH_SIZE);
		LOG_OPEN(lognamebuf);

		// try install sound
		//
		if (!nosound) {

			if (install_sound(DIGI_AUTODETECT, MIDI_NONE, 0)) {
			//if (install_sound(DIGI_WAVOUTID(0), MIDI_NONE, 0)) {

				LOG("INSTALL_SOUND failed. no sound.\n");
				sound_inited = false;
			}	else {
				LOG("INSTALL_SOUND ok.\n");
				sound_inited = true;
			}
		}
		else
			LOG("SOUND DISABLED by command line option -nosound\n");

		// gfx init
		//
		if (!reset_video_mode())		// fatal error
			return 0;

		// install client timer
		//
		LOCK_VARIABLE(speed_counter);
		LOCK_FUNCTION(increment_speed_counter);
		install_int_ex(increment_speed_counter, BPS_TO_TIMER(targetfps));		//client MAX FPS

		// run client
		//
		gameclient = new gameclient_c();
		gameclient->message_logging = message_logging;
		LOG1("Message logging is %s\n", message_logging ? "enabled" : "disabled");
		if (!gameclient->start()) {
			LOG("ERROR: cannot start gameclient!\n");
			return 0;
		}
		gameclient->loop();

		// disconnect client
		//
		gameclient->stop();
		delete gameclient;
		gameclient = 0;

		// stop listenserver if it was running
		//
		listen_stop();
	}

	// exit HawkNL
	nlShutdown();

	// log close
	LOG_CLOSE();

	return 0;
} END_OF_MAIN();
