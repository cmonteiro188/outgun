// commont.h : temporary common stuff

#ifndef COMMONT_H_INC
#define COMMONT_H_INC

#include <iostream>
#include <fstream>
#include <string>
#include <nl.h>

// Reads a line, stops to \n or \r and skips empty lines.
std::istream& getline_smart(std::istream& in, std::string& str);

class FileReader {	// reads a file non-empty line by line, skipping commented lines (with ';')
public:
	FileReader(const std::string& filename);
	std::string readLine();	// returns an empty string at end of file

private:
	std::ifstream file;
};

enum Message_type { msg_normal, msg_team, msg_info, msg_warning, msg_header };

class ClientControls {
public:
	ClientControls() : data(0) { }
	NLubyte toNetwork(bool server) const { if (server) return data & 31; else return data; }
	void fromNetwork(NLubyte d, bool server) { data = d; if (server) data &= 31; }
	void fromKeyboard();
	void fromJoystick();
	bool     isUp() const { return (data& 1)!=0; }
	bool   isDown() const { return (data& 2)!=0; }
	bool   isLeft() const { return (data& 4)!=0; }
	bool  isRight() const { return (data& 8)!=0; }
	bool    isRun() const { return (data&16)!=0; }
	bool isStrafe() const { return (data&32)!=0; }
	bool     idle() const { return  data    ==0; }

private:
	NLubyte data;

	enum {
		up     =  1,
		down   =  2,
		left   =  4,
		right  =  8,
		run    = 16,
		strafe = 32
	};
};

//play area width/height
#define plw 472
#define plh 354

#define PLAYER_RADIUS 15
#define SHIELD_RADIUS 24
#define ROCKET_RADIUS 4
#define QUAD_ROCKET_RADIUS 6

//RANKING defines
#define DEFAULT_PLAYER_RATE 1.0

#define PASSBUFFER  32	//size of password file

//#define MIN_ALPHA_FRIENDS 64

#define ROCKET_SPEED 50.0	//in pixels/0.1s

#define MIN_HEALTH_FOR_RUN_PENALTY 40

//#define DEBUG_POWERUPS
//#define REALLY_DEBUG_POWERUPS	//define only if DEBUG_POWERUPS defined

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
extern NLaddress master_address;

//directories for save/load maps
#define SERVER_MAPS_DIR "maps"
#define CLIENT_MAPS_DIR "cmaps"

// system directory separator
extern char directory_separator;

//root path (game executable path)
extern std::string wheregamedir;

// server game phisics parameters
extern double svp_fric, svp_accel, svp_maxspeed;
extern double svp_fric_run, svp_accel_run, svp_maxspeed_run;
extern double svp_fric_turbo, svp_accel_turbo, svp_maxspeed_turbo;
extern double svp_fric_turborun, svp_accel_turborun, svp_maxspeed_turborun;
extern double svp_flag_penalty;
extern bool   svp_friendly_fire, svp_friendly_db;
extern bool   svp_player_collisions;

void set_default_physics();

// number-of-players
#define MAX_PLAYERS 32	// the MAXIMUM MAXIMUM number of players EVER
extern int maxplayers;	// the maximum number of players configured for this server (must be <= MAX_PLAYERS and an EVEN NUMBER == NUMERO PAR)
#define TSIZE (maxplayers/2)	// CTF TEAM SIZE

#define MAX_ROCKETS 256	// maximum number of rockets (nao pode ser mais que 256 pq eh usado um unsigned char p/ passar ids)

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
extern bool force_ip;		//force IP?
extern char force_ip_name[32];	//force IP to what?

extern volatile bool force_exit;	// this is set true when the user tries to close the window

void closeButtonCallback();

void server_status_string(const std::string& str);

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
	SAMPLE_FIRE,
	SAMPLE_HIT,

	SAMPLE_WALLHIT,
	SAMPLE_QUADWALLHIT,

	SAMPLE_DEATH,
	SAMPLE_DEATH_2,
	SAMPLE_ENTERGAME,
	SAMPLE_LEFTGAME,
	SAMPLE_CHANGETEAM,
	SAMPLE_TALK,
	SAMPLE_WALLBOUNCE,
	SAMPLE_PLAYERBOUNCE,

	SAMPLE_WEAPON_UP,

	SAMPLE_MEGAHEALTH,

	SAMPLE_GETDEATHBRINGER,
	SAMPLE_USEDEATHBRINGER,
	SAMPLE_HITDEATHBRINGER,
	SAMPLE_DIEDEATHBRINGER,

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

	SAMPLE_CTF_GOT,
	SAMPLE_CTF_LOST,
	SAMPLE_CTF_RETURN,
	SAMPLE_CTF_CAPTURE,
	SAMPLE_CTF_GAMEOVER,

	SAMPLE_5_MIN_LEFT,
	SAMPLE_1_MIN_LEFT,

	SAMPLE_KILLING_SPREE,

	NUM_OF_SAMPLES
};

#endif
