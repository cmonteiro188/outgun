#include <sstream>

#include "commont.h"
#include "server.h"
#include "client.h"
#include "mappic.h"

using std::ostringstream;

void increment_time_counter() {
	time_counter++;
} END_OF_FUNCTION(increment_time_counter);

void increment_speed_counter() {
	speed_counter++;
} END_OF_FUNCTION(increment_speed_counter);

void increment_server_speed_counter() {
	server_speed_counter++;
} END_OF_FUNCTION(increment_server_speed_counter);

// this simple task is turning into a major headache...
bool set_shitty_mode(LogSet log) {
	if (textserver)
		return true;

	int DTC = desktop_color_depth();

	set_color_depth( DTC );

	if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 240, 0, 0))
		log("Could not set gfx mode 320x240 windowed.. try 1 with %i", DTC);
	else
		return true;	// OK

	if ((DTC == 16) || (DTC == 15)) {

		if (DTC == 15)
			DTC = 16;
		else
			DTC = 15;

		set_color_depth( DTC );

		if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 240, 0, 0))
			log("Could not set gfx mode 320x240 windowed.. try 2 with %i", DTC);
		else
			return true;
	}

	// WARNING: this can be buggy for multiple dedicated servers.
	DTC = 8;

	set_color_depth( DTC );

	if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 240, 0, 0)) {
		log("Could not set gfx mode 320x240 windowed.. tried with %i", DTC);
	}
	else
		return true;

	// try safe mode
	if (set_gfx_mode(GFX_SAFE, 320, 240, 0, 0)) {
		log("Could not set a safe gfx mode.");
		return false;
	}
	else
		return true;
}

int main(int argc, char *argv[]) {
	unsigned long stackGuard = STACK_GUARD;	(void)stackGuard;

	FileLog logFile("log.txt", true);
	LogSet log(&logFile, &logFile, &logFile);

	srand((unsigned)time(0));

	// general init
	gameclient = 0;

	//LOG_OPEN("outgun.log");

	// Set the text encoding format for Allegro as 8 bit Ascii
	set_uformat(U_ASCII);

	allegro_init();
	//set_close_button_callback(closeButtonCallback);
	install_keyboard();
	install_timer();

	// Check what the directory separator is.
	char stuff[2] = { 0 };
	put_backslash(stuff);
	directory_separator = stuff[0];

	// find out where we are
	char *exespec = new char[2048];
	get_executable_name(exespec, 2048);
	replace_filename((char*)wheregamedir, (const char *)exespec, "", 256); //Replaces the specified path+filename with a new filename tail, storing at most size bytes into the dest buffer. Returns a copy of the dest parameter
	delete exespec;
	exespec = 0;

	char lognamebuf[1024];

	//open outgun.log at EXE root path
	append_filename(lognamebuf, wheregamedir, "outgun.log", WHERE_PATH_SIZE);

	log("OUTGUN LOG FILE. STRING %s PROTOCOL %s VERSION %s", GAME_STRING, GAME_PROTOCOL, GAME_VERSION);

	bool message_logging = false;
	bool showinfo = false;
	bool defaultprio = false;	//select default server threads priority
	int targetprio = 0;
	bool targetprio_specified = false;

	// check args
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-ded"))
			dedserver = true;
		else if (!strcmp(argv[i], "-text") || !strcmp(argv[i], "-nowindow"))
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
				targetprio_specified = true;
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
		else if (!strcmp(argv[i], "-ip")) {
			if (++i<argc) {
				force_ip = true;			//force IP
				strcpy(force_ip_name, argv[i]);	//to next parameter value
			}
		}
		else if (!strcmp(argv[i], "-log"))
			message_logging = true;
		else if (!strcmp(argv[i], "-mappic")) {
			log("Saving map pictures");
			set_window_title("Outgun map picture saver");
			Mappic mappic(log);
			try {
				mappic.run();
				allegro_message("Saved map pictures to mappic directory.");
			} catch (const Mappic::Save_error& s) {
				allegro_message("ERROR: Could not save map pictures to mappic directory!");
			}
			return 0;
		}
		else {
			ostringstream msg;
			msg << "Unknown command-line argument " << argv[i];
			allegro_message(msg.str().c_str());
			return 0;
		}
	}

	if (nlInit() == NL_FALSE) {
		allegro_message("ERROR: cannot init HawkNL!");
		return 0;
	}
	if (nlSelectNetwork(NL_IP) == NL_FALSE) {
		allegro_message("ERROR: no IP network!");
		return 0;
	}

	// enable statistics
	nlEnable(NL_SOCKET_STATS);

	// resolve master server address
	// #FIXME: read master server address from a file
	log("resolving master server address...");
	try {
		nlGetAddrFromName("www.mycgiserver.com", &master_address);	//www.mycgiserver.com
	} catch (...) {
		log("caught exception probably on nlGetAddrFromNameAsync()");
		master_address.valid = NL_FALSE;
	}

	if (master_address.valid == NL_FALSE) {
		log("can't resolve master server address to IP.");
		nlStringToAddr("212.69.162.53", &master_address);					//last known resolution for www.mycgiserver.com
	} else if (master_address.valid == NL_TRUE) {
		log("address resolved sucessfully.");
	}

	nlSetAddrPort(&master_address, 80);													//port 80
	log("master server address set.");

	// install higher-accuracy timer interrupt
	LOCK_VARIABLE(speed_counter);
	LOCK_FUNCTION(increment_time_counter);
	install_int_ex(increment_time_counter, BPS_TO_TIMER(200));		//5 ms accuracy is already 10 times better than clock()

	// install server timer (used for both dedicated and listen server)
	LOCK_VARIABLE(server_speed_counter);
	LOCK_FUNCTION(increment_server_speed_counter);
	install_int_ex(increment_server_speed_counter, BPS_TO_TIMER(10));		//10Hz

	// get system thread priorities
	int				pmin = sched_get_priority_min(SCHED_OTHER);
	int				pmax = sched_get_priority_max(SCHED_OTHER);
	int				policy;
	pthread_t		tme = pthread_self();
	sched_param		param;
	int				rc = pthread_getschedparam(tme, &policy, &param); // get priority of current thread (wich is the default value)
	int				pdef = param.sched_priority;
	log("Thread priorities:");
	log("   rc = %i policy = %i OTHER=%i sched_prio = %i", rc, policy, SCHED_OTHER, param.sched_priority);
	log("   pmin %i pmax %i pdef = %i", pmin, pmax, pdef);

	//show info
	if (showinfo) {
		//get all local addresses
		NLaddress *locals;
		NLint locsize;

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

	// run dedicated server
	if (dedserver) {
		// here must get the safest and shittiest windowed gfx mode available
		if (!set_shitty_mode(log))
			textserver = true;		// if 320×240 mode can't be set, use textserver

		// dedicated server - set process priority (all threads) to a higher value
		//		--> threads filhas estao com as priorities certas? LOGAR pra  ver. senao mudar p/ INHERIT
		if (!defaultprio) {
			int ptarg;
			if (!targetprio_specified) {	//if -prio parameter is unspecified
				//guess one below system maximum (wich usually means realtime and should never be used really)
				if (pmin < pmax)
					ptarg = pmax - 1;
				else
					ptarg = pmax + 1;
			}
			else
				ptarg = targetprio;

			param.sched_priority = ptarg;
			policy = SCHED_OTHER;
			pthread_setschedparam(tme, policy, &param);
			log("NEW PRIORITY VALUE SET FOR DEDICATED SERVER: %i", ptarg);
		}
		else
			log("-defaultprio: Leaving thread priorities on their default values");

		// gfx init
		if (set_display_switch_mode(SWITCH_BACKAMNESIA) == -1) // allow running in the background
		{
			log("can't set SWITCH_BACKAMNESIA for SERVER");
			if (set_display_switch_mode(SWITCH_BACKGROUND) == -1) {
				allegro_message("ERROR: server window cannot run in the background!");
				return 0;
			}
			else
				log("set SWITCH_BACKGROUND for SERVER");
		}
		else
			log("set SWITCH_BACKAMNESIA for SERVER");

		// run server
		gameserver_c* gameserver = new gameserver_c(log);
		if (!gameserver->start(server_maxplayers)) {
			allegro_message("ERROR: cannot start gameserver!");
			return 0;
		}
		gameserver->loop(0);
		gameserver->stop();
		delete gameserver;
	}
	// run client
	else {
		//require CLIENT_MAPS_DIR directory
		char mappath[256];
		strcpy(mappath, CLIENT_MAPS_DIR);  // cmaps
		char dest[1024];
		append_filename(dest, wheregamedir, mappath, WHERE_PATH_SIZE);	// <FULL-DIR>/maps/*.txt, I hope
		log("CMAPS DIR IS = '%s'", dest);
		al_ffblk mapffblk;	//for al_find*
		int result = al_findfirst(dest, &mapffblk, FA_DIREC|FA_ARCH|FA_RDONLY);
		if (result != 0 || !(mapffblk.attrib&FA_DIREC)) {
			allegro_message("ERROR: directory '%s' not found\n\nPlease create this directory.\n\nThe game cannot run without it.", dest);
			return 0;
		}

		//window title
		server_status_string("Outgun client - CTRL+F12 to quit");

		// try install sound
		if (!nosound) {
			if (install_sound(DIGI_AUTODETECT, MIDI_NONE, 0)) {
			//if (install_sound(DIGI_WAVOUTID(0), MIDI_NONE, 0)) {
				log("INSTALL_SOUND failed. no sound.");
				sound_inited = false;
			}
			else {
				log("INSTALL_SOUND ok.");
				sound_inited = true;
			}
		}
		else
			log("SOUND DISABLED by command line option -nosound");

		// install client timer
		LOCK_VARIABLE(speed_counter);
		LOCK_FUNCTION(increment_speed_counter);
		install_int_ex(increment_speed_counter, BPS_TO_TIMER(targetfps));		//client MAX FPS

		// run client
		gameclient = new gameclient_c(log);
		gameclient->message_logging = message_logging;
		log("Message logging is %s", message_logging ? "enabled" : "disabled");
		if (!gameclient->start()) {
			allegro_message("ERROR: cannot start gameclient!");
			return 0;
		}
		gameclient->loop();

		// disconnect client
		gameclient->stop();
		delete gameclient;
		gameclient = 0;
	}

	// exit HawkNL
	nlShutdown();
	return 0;
} END_OF_MAIN();
