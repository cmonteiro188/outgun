#include <fstream>
#include <sstream>
#include <string>

#include "incalleg.h"
#include "client.h"
#include "commont.h"
#include "gameserver_interface.h"
#include "mappic.h"
#include "network.h"

using std::ifstream;
using std::ostringstream;
using std::string;

void increment_time_counter() {
	time_counter++;
} END_OF_FUNCTION(increment_time_counter);

void increment_server_speed_counter() {
	server_speed_counter++;
} END_OF_FUNCTION(increment_server_speed_counter);

// this simple task is turning into a major headache...
bool set_shitty_mode(LogSet log) {
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

// Make directory if it does not already exist.
bool check_dir(const string& dir) {
	const string directory = wheregamedir + dir;
	al_ffblk mapffblk;
	const int result = al_findfirst(directory.c_str(), &mapffblk, FA_DIREC | FA_ARCH | FA_RDONLY);
	if (result == 0 && (mapffblk.attrib & FA_DIREC))
		return true;	// exists
	return !mkdir(directory.c_str());
}

class GlobalCloseButtonHook {
	static volatile bool flag;
	static void closeCallback() { flag = true; } END_OF_STATIC_FUNCTION(closeCallback);

public:
	static void install() {
		LOCK_VARIABLE(flag);
		LOCK_FUNCTION(closeCallback);
		set_close_button_callback(closeCallback);
	}
	static volatile bool* flagPtr() { return &flag; }
};

volatile bool GlobalCloseButtonHook::flag = false;

void statusOutputWindow(const string& str) {
	set_window_title(str.c_str());
}

void statusOutputText(const string& str) {
	#ifndef ALLEGRO_WINDOWS
	cout << str << '\n';
	#else
	statusOutputWindow(str);
	#endif
}

int main(int argc, char *argv[]) {
	unsigned long stackGuard = STACK_GUARD;	(void)stackGuard;

	srand((unsigned)time(0));

	// general init
	gameclient = 0;

	// Set the text encoding format for Allegro as 8 bit Ascii
	set_uformat(U_ASCII);

	allegro_init();
	install_keyboard();
	install_timer();

	// Check what the directory separator is.
	char stuff[2] = { 0 };
	put_backslash(stuff);
	directory_separator = stuff[0];

	// find out where we are
	char *path = new char[2048];
	get_executable_name(path, 2048);
	replace_filename(path, path, "", 256); //Replaces the specified path+filename with a new filename tail, storing at most size bytes into the dest buffer. Returns a copy of the dest parameter
	wheregamedir = path;
	delete[] path;

	if (!check_dir("log")) {
		allegro_message("Error: Directory 'log' not found.\nPlease create this directory.\nThe game cannot run without it.");
		return 0;
	}
	if (!check_dir("config"))
		allegro_message("Error: Directory 'config' not found.\nCreate it to be able to save the configuration\nor customize your server.");

	FileLog logFile(wheregamedir + "log" + directory_separator + "log.txt", true);
	LogSet log(&logFile, &logFile, &logFile);

	log("Outgun log file. %s. Game string: %s, protocol: %s, version: %s", date_and_time().c_str(), GAME_STRING, GAME_PROTOCOL, GAME_VERSION);
	if (LOG_THREAD_IDS)
		log("main() ID = %d, prio = %d", pthread_self(), threadPriority());

	bool textserver = false;
	bool showinfo = false;
	bool defaultprio = false;	//select default server threads priority
	int targetprio = 0;
	bool targetprio_specified = false;
	ServerExternalSettings serverCfg;
	ClientExternalSettings clientCfg;

	// check args
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-ded"))
			serverCfg.dedserver = true;
		else if (!strcmp(argv[i], "-text") || !strcmp(argv[i], "-nowindow"))
			textserver = true;
		else if (!strcmp(argv[i], "-priv"))
			serverCfg.privateserver = true;
		else if (!strcmp(argv[i], "-info"))
			showinfo = true;
		else if (!strcmp(argv[i], "-defaultprio"))
			defaultprio = true;
		else if (!strcmp(argv[i], "-prio")) {
			if (++i < argc) {
				targetprio = strtol(argv[i], NULL, 10);
				targetprio_specified = true;
			}
		}
		else if (!strcmp(argv[i], "-win"))
			clientCfg.winclient = 1;
		else if (!strcmp(argv[i], "-flip"))
			clientCfg.trypageflip = 1;
		else if (!strcmp(argv[i], "-dbuf"))
			clientCfg.trypageflip = 0;
		else if (!strcmp(argv[i], "-fs"))
			clientCfg.winclient = 0;
		else if (!strcmp(argv[i], "-fps")) {
			if (++i<argc) {
				clientCfg.targetfps = strtol(argv[i], NULL, 10);
				if (clientCfg.targetfps < 1)
					clientCfg.targetfps = 1;
				if (clientCfg.targetfps > 1000)
					clientCfg.targetfps = 1000;
			}
		}
		else if (!strcmp(argv[i], "-maxp")) {
			if (++i < argc) {
				serverCfg.server_maxplayers = strtol(argv[i], NULL, 10);
				if (serverCfg.server_maxplayers % 2 == 1)	//ímpar: des-impariza
					serverCfg.server_maxplayers++;
				if (serverCfg.server_maxplayers < 2)
					serverCfg.server_maxplayers = 2;
				if (serverCfg.server_maxplayers > MAX_PLAYERS)
					serverCfg.server_maxplayers = MAX_PLAYERS;
			}
		}
		else if (!strcmp(argv[i], "-port")) {
			if (++i < argc)
				serverCfg.port = strtol(argv[i], NULL, 10);
		}
		else if (!strcmp(argv[i], "-nosound"))
			clientCfg.nosound = true;
		else if (!strcmp(argv[i], "-ip")) {
			if (++i < argc) {
				serverCfg.force_ip = true;			//force IP
				serverCfg.force_ip_name = argv[i];	//to next parameter value
			}
		}
		else if (!strcmp(argv[i], "-mappic")) {
			if (!check_dir("mappic")) {
				allegro_message("Error: Directory 'mappic' not found.\nMake this directory.");
				return 0;
			}
			log("Saving map pictures");
			set_window_title("Outgun map picture saver");
			Mappic mappic(log);
			try {
				mappic.run();
				allegro_message("Saved map pictures to mappic directory.");
			} catch (const Mappic::Save_error& s) {
				allegro_message("Error: Could not save map pictures to mappic directory. See the logs.");
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
		allegro_message("Error: Can't init HawkNL.\n%s", getNlErrorString());
		return 0;
	}
	if (nlSelectNetwork(NL_IP) == NL_FALSE) {
		allegro_message("Error: No IP network.");
		return 0;
	}

	// enable statistics
	nlEnable(NL_SOCKET_STATS);

	// resolve master server address
	log("Resolving master server address...");
	ifstream in((wheregamedir + "config" + directory_separator + "master.txt").c_str());
	string name, address;
	if (!getline_skip_comments(in, name))
		name = "koti.mbnet.fi";
	if (!getline_skip_comments(in, address))
		address = "194.100.161.5";
	in.close();
	try {
		nlGetAddrFromName(name.c_str(), &master_address);
	} catch (...) {
		log("Caught exception probably on nlGetAddrFromNameAsync()");
		master_address.valid = NL_FALSE;
	}

	if (master_address.valid == NL_FALSE) {
		log("Can't resolve master server address to IP.");
		nlStringToAddr(address.c_str(), &master_address);
	}

	if (!nlGetPortFromAddr(&master_address))
		nlSetAddrPort(&master_address, 80);
	log("Master server address set: %s (%s), port %d.", name.c_str(), address.c_str(), nlGetPortFromAddr(&master_address));

	// install higher-accuracy timer interrupt
	LOCK_VARIABLE(time_counter);
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
		sprintf(infobuf,
			"Information:\n"
			"\n"
			"Thread priorities for -prio <val> parameter:\n"
			"* Minimum -prio <val> : %i\n"
			"* Maximum -prio <val> : %i\n"
			"* System default (use -defaultprio) : %i\n"
			"\n"
			"Local addresses:\n",
				pmin, pmax, pdef);

		for (int z = 0; z < locsize; z++) {
			strcat(infobuf, addressToString(locals[z]).c_str());
			strcat(infobuf, "\n");
		}

		allegro_message(infobuf);
		return 0;
	}

	GlobalCloseButtonHook::install();

	// run dedicated server
	if (serverCfg.dedserver) {
		if (textserver)
			serverCfg.statusOutput = statusOutputText;
		else {
			if (!set_shitty_mode(log))	// if 320×240 mode can't be set, use textserver
				serverCfg.statusOutput = statusOutputText;
			else
				serverCfg.statusOutput = statusOutputWindow;
		}

		// dedicated server - set process priority (all threads) to a higher value
		if (!defaultprio) {
			int ptarg;
			if (!targetprio_specified) {	//if -prio parameter is unspecified
				//guess one below system maximum (wich usually means realtime and should never be used really)
				ptarg = pmax - 1;
			}
			else
				ptarg = targetprio;

			if (ptarg >= pmin && ptarg <= pmax) {
				param.sched_priority = ptarg;
				policy = SCHED_OTHER;
				pthread_setschedparam(tme, policy, &param);
				log("Priority set for dedicated server: %i", ptarg);
			}
			else
				log("Skipped setting priority: %d doesn't fit on the scale", ptarg);
		}
		else
			log("-defaultprio: Leaving thread priorities on their default values");

		// gfx init
		if (set_display_switch_mode(SWITCH_BACKAMNESIA) == -1) {
			if (set_display_switch_mode(SWITCH_BACKGROUND) == -1) {
				log.error("Switch_backamnesia and switch_background failed: server window can't run in the background.");
				allegro_message("Error: server window can't run in the background.");
				return 0;
			}
			else
				log("Switch_background set ok.");
		}
		else
			log("Switch_backamnesia set ok.");

		// run server
		GameserverInterface* gameserver = new GameserverInterface(log, serverCfg);
		if (gameserver->start(serverCfg.server_maxplayers)) {
			gameserver->loop(GlobalCloseButtonHook::flagPtr(), true);
			gameserver->stop();
		}
		else
			allegro_message("Error: can't start server. See the logs.");
		delete gameserver;
	}
	// run client
	else {
		if (!check_dir(CLIENT_MAPS_DIR)) {
			allegro_message("Error: directory '%s' not found.\nPlease create this directory.\nThe game can't run without it.", CLIENT_MAPS_DIR);
			return 0;
		}
		if (!check_dir("client_stats"))
			log.error("Directory 'client_stats' not found.");
		if (!check_dir("server_stats"))
			log.error("Directory 'server_stats' not found.");

		// run client
		clientCfg.statusOutput = statusOutputWindow;
		serverCfg.statusOutput = statusOutputWindow;
		gameclient = new gameclient_c(log, clientCfg, serverCfg);
		if (gameclient->start()) {
			gameclient->loop(GlobalCloseButtonHook::flagPtr());
			gameclient->stop();
		}
		else
			allegro_message("Error: can't start client. See the logs.");
		delete gameclient;
	}

	log("Exiting");
	// exit HawkNL
	nlShutdown();
	return 0;
} END_OF_MAIN();
