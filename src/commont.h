// commont.h : temporary common stuff

#ifndef COMMONT_H_INC
#define COMMONT_H_INC

#define PHYS_NEW
//#define PHYS_VECTOR_ACC

/* PHYS_SHIFTY is used for bounce checks: 15 aligns with the map, 0 is the buggy default behaviour */
#ifdef PHYS_NEW
#define PHYS_SHIFTY 15
#else
#define PHYS_SHIFTY 0
#endif

#include <vector>
#include <list>
#include <fstream>
#include <sstream>
#include <iomanip>

template<class T> T bound(T val, T lb, T hb) { return val<=lb?lb:val>=hb?hb:val; }

// strspnp: (Watcom definition) find from str the first char not in charset
char* strspnp(char* str, const char* charset);
const char* strspnp(const char* str, const char* charset);

//play area width/height
#define plw 472
#define plh 354

// ***** FORTIFY !!! *****

#include "fortfy22/fortify.h"

// ***** FORTIFY !!! *****

//macros for allegro video mode

//#define WINMODE GFX_GDI       -- can't pageflip

//#define WINMODE GFX_DIRECTX_ACCEL
//#define FULLMODE GFX_DIRECTX_ACCEL

#define WINMODE GFX_AUTODETECT_WINDOWED

#define FULLMODE GFX_AUTODETECT

//DEBUGGING ranking?
//#define DEBUG_RANKING

//same as PLAYER RADIUS (15) + ROCKET RADIUS (3)
#define SHOT_DELTAX 17  // V0.4.8 : A HAIR LESS!

//minimum time between flag steal at base and capture, to consider a map to be valid for scoring
#define MINIMUM_GRAB_TO_CAPTURE_TIME 6.0

//RANKING defines
#define DEFAULT_PLAYER_RATE 1.0
#define MINIMUM_POSITIVE_SCORE_FOR_RANKING 100

//#define SWITCH_PAUSE_CLIENT

//#define ALWAYS_FRICTION

#define PI M_PI //3.1416

#define PIOIT M_PI_4 //0.7854 //DOIS PI SOBRE 8 = PI SOBRE 4 = 0.7854

#define PASSBUFFER  32      //size of password file

//quick debugs
//#define MIN_ALPHA_FRIENDS 1           //debug value
#define MIN_ALPHA_FRIENDS 64

#define ROCKET_SPEED 50.0       //in pixels/0.1s

#define MIN_HEALTH_FOR_RUN_PENALTY 40

#define NUMBER_OF_POWERUP_KINDS 7   //quad shield shadow turbo weapon-up megahealth deathbringer

//#define DEBUG_POWERUPS
//#define REALLY_DEBUG_POWERUPS     //define only if DEBUG_POWERUPS defined

// GAME VERSION / GAME STRING
//
#define GAME_STRING "Outgun"
#define GAME_PROTOCOL "14"
#define GAME_VERSION "0.5.0-E"

#define TK1_VERSION_STRING "v048"

#include "allegro.h"    // Allegro

//patching main / _main / WinMain link errors...
#ifdef ALLEGRO_WINDOWS
#include "winalleg.h"
#include "windows.h"
#endif

#include <stdio.h>          // for -text (v0.5.0)

#include "nl.h"             // HawkNL

#include "leetnet/server.h"     // l33t server
#include "leetnet/client.h"     // l33t client
#include "leetnet/rudp.h"           // get_self_I
#include "leetnet/sleep.h"      // sleep util

#include "string.h"
#include "math.h"

#include "pthread.h"
#include "sched.h"

#include <string>
using namespace std;
#include "names.h"      //the COOLEST random-name generator by Renato Hentschke

//admin shell protocol
#include "admshell.h"

//log utils
//#define LOG_NOLOG     // uncomment to disable logging
#define LOG_EXPR game_log
#define LOG_TIMEFUNC get_time()
#include "leetnet/log.h"
extern FILE *game_log;

#if defined ALLEGRO_WINDOWS || WIN32 || WIN64
#include <conio.h>
#endif

//************************************************************
//  common stuff
//************************************************************

//the default game port
#define DEFAULT_UDP_PORT 25000

//the master server address (www.mycgiserver.com:80)
extern NLaddress        master_address;

//directories for save/load maps
#define SERVER_MAPS_DIR "maps"
#define CLIENT_MAPS_DIR "cmaps"

//root path (game executable path)
#define WHERE_PATH_SIZE 256
extern char wheregamedir[WHERE_PATH_SIZE];

// server game phisics parameters
extern double svp_fric, svp_accel, svp_maxspeed;
extern double svp_fric_run, svp_accel_run, svp_maxspeed_run;
extern double svp_fric_turbo, svp_accel_turbo, svp_maxspeed_turbo;
extern double svp_fric_turborun, svp_accel_turborun, svp_maxspeed_turborun;
extern double svp_flag_penalty;

void set_default_physics();

// number-of-players
#define MAX_PLAYERS 32                          // the MAXIMUM MAXIMUM number of players EVER
extern int          maxplayers;     // the maximum number of players configured for this server (must be <= MAX_PLAYERS and an EVEN NUMBER == NUMERO PAR)
#define TSIZE (maxplayers/2)                // CTF TEAM SIZE

#define MAX_ROCKETS 256     // maximum number of rockets (nao pode ser mais que 256 pq eh usado um unsigned char p/ passar ids)

#define MAX_PICKUPS MAX_PLAYERS // the MAXIMUM MAXIMUM number of pickups laying on the ground at one time in the game

//arg switches (+ default values)
extern bool dedserver;      //dedicated server? -ded
extern bool textserver;     //textmode dedicated server for UNIX/LINUX (V0.5.0) (WON'T WORK ON WINDOWS...)
extern bool privateserver;  //private server? (will not publish)
extern bool winclient;      //windowed client?  -win / -fs
extern bool trypageflip;    //try page flipping? -flip / -dbuf
extern bool nosound;            //disable sound? -nosound
extern int targetfps;           //target (MAX) frames-per-second
extern int port;                //the server port
extern bool showinfo;       //apenas show info e desliga server
#define TARGET_PRIO_UNSPECIFIED -666666
extern bool defaultprio;    //select default server threads priority
extern int targetprio;  //unspecified
extern int server_maxplayers;       //default maxplayers of the server
extern bool sound_inited;       //install_sound succeeded?
extern bool sound_enabled;      // player wants sounds?
extern bool no_tcp_download;        // V0.4.7: CHANGED DEFAULT : disable use of the TCP socket for file transfers (use the regular UDP leetnet connection)
extern bool force_ip;       //force IP?
extern char force_ip_name[32];      //force IP to what?

void server_status_string(char *str);
double get_time();

extern volatile int server_speed_counter;
extern volatile int speed_counter;
extern volatile int client_netsend_counter;
extern volatile unsigned long time_counter;

//server_next_map() reasons
enum {
    NEXTMAP_NONE,
    NEXTMAP_CAPTURE_LIMIT,
    NEXTMAP_VOTE_EXIT,
    NUM_OF_NEXTMAP
};

//audio samples : codes
enum {

    SAMPLE_FIRE,            //ok
    SAMPLE_HIT,             //ok

    SAMPLE_WALLHIT,             //new! v0.3.9
    SAMPLE_QUADWALLHIT,     //new! v0.3.9

	SAMPLE_DEATH,			//ok
	SAMPLE_DEATH_2,		//ok
	SAMPLE_ENTERGAME,	//ok
	SAMPLE_LEFTGAME,	//ok
	SAMPLE_CHANGETEAM,
	SAMPLE_TALK,
	SAMPLE_WALLBOUNCE,

	SAMPLE_WEAPON_UP,	// new! v0.1.2

	SAMPLE_MEGAHEALTH, // new! v0.3.0

	SAMPLE_GETDEATHBRINGER, // new! v0.3.9 -- get deathbringer powerup (voz sinistra)
	SAMPLE_USEDEATHBRINGER, // new! v0.3.9 -- use deathbringer powerup (carrier dies) ("GRRRAAWWKKLLLL!!")
	SAMPLE_HITDEATHBRINGER,	// new! v0.3.9 -- target is hit by the deathbringer ("PWRRLLW!")
	SAMPLE_DIEDEATHBRINGER,	// new! v0.3.9 -- target dies by the deathbringer		("HaHaHaHa!")

	SAMPLE_SHIELD_PICKUP,
	SAMPLE_SHIELD_DAMAGE,
	SAMPLE_SHIELD_LOST,

	SAMPLE_BOOTS_ON,
	SAMPLE_BOOTS_OFF,

	SAMPLE_QUAD_ON,
	SAMPLE_QUAD_FIRE,
	SAMPLE_QUAD_OFF,

	SAMPLE_HELM_ON,
	SAMPLE_HELM_OFF,

	SAMPLE_CTF_GOT,			//ok
	SAMPLE_CTF_LOST,		//ok
	SAMPLE_CTF_RETURN,	//ok
	SAMPLE_CTF_CAPTURE,	//ok
	SAMPLE_CTF_GAMEOVER,	//ok

	NUM_OF_SAMPLES
};

extern char teamname[2][5];

extern int listen_port_running;
extern volatile bool	listen_server_running;
extern pthread_t	listen_server_thread;
void listen_start();
void listen_stop();

#endif
