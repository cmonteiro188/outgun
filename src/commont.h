// commont.h : temporary common stuff

#ifndef COMMONT_H_INC
#define COMMONT_H_INC

#include <allegro.h>
#ifdef ALLEGRO_WINDOWS
#include <winalleg.h>
#endif

#include <vector>
#include <list>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
using namespace std;

#include <cstdio>          // for -text (v0.5.0)
#include <cstring>
#include <cmath>

#include <pthread.h>
#include <sched.h>
#include <nl.h>             // HawkNL

//log utils
//#define LOG_NOLOG     // uncomment to disable logging
#define LOG_EXPR game_log
#define LOG_TIMEFUNC get_time()
#include "leetnet/log.h"
extern FILE *game_log;

enum MESSAGE_TYPE { MSG_NORMAL, MSG_TEAM, MSG_INFO, MSG_WARNING };

class ClientControls {
	NLubyte data;

public:
	ClientControls() : data(0) { }
	NLubyte toNetwork(bool server) const { if (server) return data & 31; else return data; }
	void fromNetwork(NLubyte d, bool server) { data = d; if (server) data &= 31; }
	void fromKeyboard();
	bool     isUp() const { return (data& 1)!=0; }
	bool   isDown() const { return (data& 2)!=0; }
	bool   isLeft() const { return (data& 4)!=0; }
	bool  isRight() const { return (data& 8)!=0; }
	bool    isRun() const { return (data&16)!=0; }
	bool isStrafe() const { return (data&32)!=0; }
};

void rotate_angle(float& angle, float shift);

template<class T> T bound(T val, T lb, T hb) { return val<=lb?lb:val>=hb?hb:val; }

// strspnp: (Watcom definition) find from str the first char not in charset
char* strspnp(char* str, const char* charset);
const char* strspnp(const char* str, const char* charset);

class LineReceiver {
protected:
	LineReceiver() { }

public:
	virtual LineReceiver& operator()(const string& str) =0;
};

inline void readStr(const char* buf, int& count, string& dst) {
	dst.clear();
	while (buf[count])
		dst += buf[count++];
	++count;
}
inline void writeStr(char* buf, int& count, const string& src) {
	memcpy(&buf[count], src.data(), src.length());
	count += src.length();
	buf[count++] = '\0';
}

//play area width/height
#define plw 472
#define plh 354

#define PLAYER_RADIUS 15
#define SHIELD_RADIUS 24
#define ROCKET_RADIUS 4
#define QUAD_ROCKET_RADIUS 6

//RANKING defines
#define DEFAULT_PLAYER_RATE 1.0

#define PASSBUFFER  32      //size of password file

//quick debugs
//#define MIN_ALPHA_FRIENDS 1           //debug value
#define MIN_ALPHA_FRIENDS 64

#define ROCKET_SPEED 50.0       //in pixels/0.1s

#define MIN_HEALTH_FOR_RUN_PENALTY 40

//#define DEBUG_POWERUPS
//#define REALLY_DEBUG_POWERUPS     //define only if DEBUG_POWERUPS defined

// GAME VERSION / GAME STRING
//
#define GAME_STRING "Outgun"
#define GAME_PROTOCOL "Nix test v0.1"
#define GAME_VERSION "NTST-01"

#define TK1_VERSION_STRING "v048"

//************************************************************
//  common stuff
//************************************************************

//the default game port
const NLushort DEFAULT_UDP_PORT = 25000;

//the master server address (www.mycgiserver.com:80)
extern NLaddress        master_address;

//directories for save/load maps
#define SERVER_MAPS_DIR "maps"
#define CLIENT_MAPS_DIR "cmaps"

// system directory separator
extern char directory_separator;

//root path (game executable path)
#define WHERE_PATH_SIZE 256
extern char wheregamedir[WHERE_PATH_SIZE];

// server game phisics parameters
extern double svp_fric, svp_accel, svp_maxspeed;
extern double svp_fric_run, svp_accel_run, svp_maxspeed_run;
extern double svp_fric_turbo, svp_accel_turbo, svp_maxspeed_turbo;
extern double svp_fric_turborun, svp_accel_turborun, svp_maxspeed_turborun;
extern double svp_flag_penalty;
extern bool   svp_friendly_fire, svp_friendly_db;

void set_default_physics();

// number-of-players
#define MAX_PLAYERS 32                          // the MAXIMUM MAXIMUM number of players EVER
extern int          maxplayers;     // the maximum number of players configured for this server (must be <= MAX_PLAYERS and an EVEN NUMBER == NUMERO PAR)
#define TSIZE (maxplayers/2)                // CTF TEAM SIZE

#define MAX_ROCKETS 256     // maximum number of rockets (nao pode ser mais que 256 pq eh usado um unsigned char p/ passar ids)

#define MAX_PICKUPS MAX_PLAYERS // the MAXIMUM MAXIMUM number of pickups laying on the ground at one time in the game

extern bool dedserver;		//dedicated server? -ded
extern bool textserver;		//textmode dedicated server for UNIX/LINUX (V0.5.0) (WON'T WORK ON WINDOWS...)
extern bool privateserver;	//private server? (will not publish)
extern bool winclient;		//windowed client?  -win / -fs
extern bool trypageflip;	//try page flipping? -flip / -dbuf
extern bool nosound;		//disable sound? -nosound
extern int targetfps;		//target (MAX) frames-per-second
extern int port;			//the server port
extern int server_maxplayers;	//default maxplayers of the server
extern bool sound_inited;	//install_sound succeeded?
extern bool force_ip;		//force IP?
extern char force_ip_name[32];	//force IP to what?

extern volatile bool force_exit;	// this is set true when the user tries to close the window

void closeButtonCallback();

void server_status_string(const std::string& str);
double get_time();

extern volatile int server_speed_counter;
extern volatile int speed_counter;
extern volatile int client_netsend_counter;
extern volatile unsigned long time_counter;

class ProfileTimer {
	const char* n;
	unsigned long val;

public:
	ProfileTimer(const char* name) : n(name), val(0) { }
	~ProfileTimer() { std::cerr << n << " : " << val << '\n'; }
	unsigned long* getPtr() { return &val; }
};

class Profiler {
	unsigned long* sump;
	unsigned long start;

public:
	Profiler(ProfileTimer& timer) : sump(timer.getPtr()), start(time_counter) { }
	~Profiler() { *sump += time_counter - start; }
};

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

//server record
struct gamespy_t {
    NLaddress addr;
    char address[128];  //IP-address typein buffer
    bool invalid;
    bool noresponse;
    bool favs;  //hack
    bool refreshed; //if data below is valid -------------
    char info[128];
};

// Reads a line, stops to \n or \r and skips empty lines.
std::istream& getline_smart(std::istream& in, std::string& str);

// Convert string to uppercase.
string toupper(string str);

#endif
