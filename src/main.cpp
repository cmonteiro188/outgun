/*
 *  main.cpp
 *
 *  Copyright (C) 2002 - Fabio Reis Cecin
 *  Copyright (C) 2003, 2004 - Niko Ritari
 *  Copyright (C) 2003, 2004 - Jani Rivinoja
 *
 *  This file is part of Outgun.
 *
 *  Outgun is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Outgun is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Outgun; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <fstream>
#include <sstream>
#include <string>

#include "incalleg.h"
#include "client.h"
#include "commont.h"
#include "debug.h"
#include "gameserver_interface.h"
#include "mappic.h"
#include "network.h"
#include "platform.h"   // platMkdir
#include "language.h"

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
        return true;    // OK

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
    bool exists = (result == 0 && (mapffblk.attrib & FA_DIREC));
    al_findclose(&mapffblk);
    if (exists)
        return true;
    else
        return !platMkdir(directory.c_str());
}

class GlobalCloseButtonHook {
    static volatile bool flag;
    friend void GlobalCloseButtonHook__closeCallback();

public:
    static void install() {
        LOCK_VARIABLE(flag);
        LOCK_FUNCTION(GlobalCloseButtonHook__closeCallback);
        set_close_button_callback(GlobalCloseButtonHook__closeCallback);
    }
    static volatile bool* flagPtr() { return &flag; }
};

volatile bool GlobalCloseButtonHook::flag = false;

void GlobalCloseButtonHook__closeCallback() {
    GlobalCloseButtonHook::flag = true;
} END_OF_FUNCTION(GlobalCloseButtonHook__closeCallback);

void statusOutputWindow(const string& str) {
    set_window_title(str.c_str());
}

void statusOutputText(const string& str) {
    #ifndef ALLEGRO_WINDOWS
    std::cout << str << '\n';
    #else
    statusOutputWindow(str);
    #endif
}

int mainError(const char* fmt, ...) {   // returns 1 so that it may be called like return mainError("...");
    static const int bufSize = 16384;
    char buf[bufSize] = "Error: ";
    va_list args;
    va_start(args, fmt);
    platVsnprintf(buf, bufSize - 8, fmt, args);
    va_end(args);
    allegro_message(buf);
    return 1;
}

int main(int argc, char *argv[]) {
    unsigned long stackGuard = STACK_GUARD; stackGuardHackPtr = &stackGuard;
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

    if (!check_dir("log"))
        return mainError("The directory 'log' was not found.\nPlease create this directory.\nThe game cannot run without it.");
    if (!check_dir("config"))
        allegro_message("The directory 'config' was not found.\nCreate it to be able to save the configuration\nor customize your server.");

    FileLog logFile(wheregamedir + "log" + directory_separator + "log.txt", true);
    LogSet log(&logFile, &logFile, &logFile);

    log("Outgun log file. %s. Game string: %s, protocol: %s, version: %s", date_and_time().c_str(), GAME_STRING, GAME_PROTOCOL, GAME_VERSION);
    logThreadStart("main", log);

    bool textserver = false;
    bool showinfo = false;
    bool defaultprio = false;   //select default server threads priority
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
        else if (!strcmp(argv[i], "-priv")) {
            serverCfg.privateserver = true;
            serverCfg.privSettingForced = true;
        }
        else if (!strcmp(argv[i], "-public")) {
            serverCfg.privateserver = false;
            serverCfg.privSettingForced = true;
        }
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
                int fps = strtol(argv[i], NULL, 10);
                if (fps < 1 || fps > 1000)
                    return mainError("-fps X: X must be in the range of 1 to 1000.");
                clientCfg.targetfps = fps;
            }
            else
                return mainError("-fps must be followed by a space and a number.");
        }
        else if (!strcmp(argv[i], "-maxp")) {
            if (++i < argc) {
                int maxp = strtol(argv[i], NULL, 10);
                if (maxp < 2 || maxp > MAX_PLAYERS || (maxp % 2 == 1))
                    return mainError("-maxp X: X must be an even number in the range of 2 to %d.", MAX_PLAYERS);
                serverCfg.server_maxplayers = maxp;
            }
            else
                return mainError("-maxp must be followed by a space and a player count.");
        }
        else if (!strcmp(argv[i], "-port")) {
            if (++i < argc) {
                serverCfg.port = strtol(argv[i], NULL, 10);
                if (serverCfg.port < 1 || serverCfg.port > 65535)
                    return mainError("-port X: X must be in the range of 1 to 65535.");
                serverCfg.portForced = true;
            }
            else
                return mainError("-port must be followed by a space and a port number.");
        }
        else if (!strcmp(argv[i], "-nosound"))
            clientCfg.nosound = true;
        else if (!strcmp(argv[i], "-ip")) {
            if (++i < argc) {
                if (isValidIP(argv[i])) {
                    serverCfg.force_ip_name = argv[i];
                    serverCfg.ipForced = true;
                }
                else
                    return mainError("-ip X: X must be a valid IP address without :port.");
            }
            else
                return mainError("-ip must be followed by a space and an IP address.");
        }
        else if (!strcmp(argv[i], "-mappic")) {
            if (!check_dir("mappic"))
                return mainError("The directory 'mappic' was not found.\nCreate this directory to be able to run with -mappic.");
            log("Saving map pictures");
            set_window_title("Outgun map picture saver");
            Mappic mappic(log);
            try {
                mappic.run();
                allegro_message("Saved map pictures to mappic directory.");
            } catch (const Mappic::Save_error& s) {
                return mainError("Could not save map pictures to mappic directory. See the logs.");
            }
            return 0;
        }
        else
            return mainError("Unknown command-line argument '%s'.", argv[i]);
    }

    if (nlInit() == NL_FALSE)
        return mainError("Can't init HawkNL.\n%s", getNlErrorString());
    if (nlSelectNetwork(NL_IP) == NL_FALSE)
        return mainError("No IP network.");

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
    else if (!isValidIP(address, true, 1))
        return mainError("'%s', given in master.txt is not a valid IP address", address.c_str());
    in.close();
    try {
        nlGetAddrFromName(name.c_str(), &master_address);
    } catch (...) {
        log("Caught exception probably on nlGetAddrFromNameAsync()");
        master_address.valid = NL_FALSE;
    }

    if (master_address.valid == NL_FALSE) {
        log("Can't resolve master server DNS name to IP.");
        nlStringToAddr(address.c_str(), &master_address);
    }

    if (nlGetPortFromAddr(&master_address) == 0)    // port is unspecified or an error occured
        nlSetAddrPort(&master_address, 80);
    log("Master server address set: %s (%s), port %d.", name.c_str(), address.c_str(), nlGetPortFromAddr(&master_address));

    // install higher-accuracy timer interrupt
    LOCK_VARIABLE(time_counter);
    LOCK_FUNCTION(increment_time_counter);
    install_int_ex(increment_time_counter, BPS_TO_TIMER(200));      //5 ms accuracy is already 10 times better than clock()

    // install server timer (used for both dedicated and listen server)
    LOCK_VARIABLE(server_speed_counter);
    LOCK_FUNCTION(increment_server_speed_counter);
    install_int_ex(increment_server_speed_counter, BPS_TO_TIMER(10));       //10Hz

    // get system thread priorities
    int             pmin = sched_get_priority_min(SCHED_OTHER);
    int             pmax = sched_get_priority_max(SCHED_OTHER);
    int             policy;
    sched_param     param;
    int             rc = pthread_getschedparam(pthread_self(), &policy, &param); // get priority of current thread (which is the default value)
    int             pdef = param.sched_priority;
    log("Thread priorities:");
    log("   rc = %i policy = %i (%i)", rc, policy, SCHED_OTHER);
    log("   pmin %i pmax %i pdef = %i", pmin, pmax, pdef);

    //show info
    if (showinfo) {
        //get all local addresses
        NLaddress *locals;
        NLint locsize;

        locals = nlGetAllLocalAddr(&locsize);

        char infobuf[2048];
        platSnprintf(infobuf, 2048,
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

    clientCfg.priority = clientCfg.lowerPriority = pdef;
    serverCfg.lowerPriority = pdef;
    if (!defaultprio) {
        int ptarg;
        if (targetprio_specified)
            ptarg = targetprio;
        else
            ptarg = pmax - 1;   // guess one below system maximum (which usually means realtime and should never be used really)

        if (ptarg < pmin || ptarg > pmax) {
            if (targetprio_specified) {
                log.error("Given priority %d doesn't fit on the scale", ptarg);
                return mainError("The priority isn't within system limits.\nRun with -info for more information.");
            }
            else    // this mostly happens if pmin == pmax
                log("Couldn't set a higher priority. Using default.");
            ptarg = pdef;
        }

        serverCfg.priority = ptarg;
        log("Priority set for server: %i", ptarg);
    }
    else {
        serverCfg.priority = pdef;
        log("-defaultprio: Leaving thread priorities on their default values");
    }
    serverCfg.networkPriority = clientCfg.networkPriority = serverCfg.priority;

    GlobalCloseButtonHook::install();
    GlobalDisplaySwitchHook::init();

    // run dedicated server
    if (serverCfg.dedserver) {
        if (textserver)
            serverCfg.statusOutput = statusOutputText;
        else {
            if (!set_shitty_mode(log))  // if 320×240 mode can't be set, use textserver
                serverCfg.statusOutput = statusOutputText;
            else
                serverCfg.statusOutput = statusOutputWindow;
        }

        if (set_display_switch_mode(SWITCH_BACKAMNESIA) == -1) {
            if (set_display_switch_mode(SWITCH_BACKGROUND) == -1) {
                log.error("Switch_backamnesia and switch_background failed: server can't run in the background.");
                return mainError("Can't set server to run in the background.");
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
            allegro_message("Error: Can't start server. See the logs.");
        delete gameserver;
    }
    // run client
    else {
        const string lang_file = wheregamedir + "config" + directory_separator + "language.txt";
        ifstream in(lang_file.c_str());
        string lang_str;
        if (getline_skip_comments(in, lang_str)) {
            if (!lang_str.empty() && lang_str.find_first_of(".:/\\") == string::npos)
                language.load(lang_str);	// load() will bug the user if something goes wrong
            else
                log.error("Invalid language '%s' in %s.", lang_str.c_str(), lang_file.c_str());
        }
        in.close();

        if (!check_dir(CLIENT_MAPS_DIR))
            return mainError("The directory '%s' was not found.\nPlease create this directory.\nThe game can't run without it.", CLIENT_MAPS_DIR);
        if (!check_dir("screens"))
            log.error("The directory 'screens' was not found.");
        if (!check_dir("client_stats"))
            log.error("The directory 'client_stats' was not found.");
        if (!check_dir("server_stats"))
            log.error("The directory 'server_stats' was not found.");

        // run client
        clientCfg.statusOutput = statusOutputWindow;
        serverCfg.statusOutput = statusOutputWindow;
        gameclient = new Client(log, clientCfg, serverCfg);
        if (gameclient->start()) {
            gameclient->loop(GlobalCloseButtonHook::flagPtr());
            gameclient->stop();
        }
        else
            allegro_message(_("Error: Can't start client. See the logs.").c_str());
        delete gameclient;
    }

    log("Exiting");
    // exit HawkNL
    nlShutdown();
    return 0;
} END_OF_MAIN();
