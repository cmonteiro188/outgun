// commont.h : temporary common stuff

#ifndef COMMONT_H_INC
#define COMMONT_H_INC

#define SV_SERVER_PHYSICS
//#define SV_PHYS_VECTOR_ACC

// ---- server side defines

#define SV_CONSOLE  // enable console commands
#define SV_NAME_AUTHORIZATION   // enable player IP based filtering : name authorization and ban
#define SV_NO_PUP_SWITCHING // disable the changing of power-ups lying on the ground
#define SV_VOTE_ANNOUNCE_INTERVAL 5 // in seconds, how often a changing voting status will be announced
#define SV_SHADOW_MINIMUM_NORMAL 7  // the shadow visibility factor

// ---- client side defines

#define CL_MINIMAP_FLAGPOS  // paint minimap more intelligently according to flag positions
#define CL_SHOW_FLAGPOS // show a flag position marker on the ground
#define CL_FLAGPOS_RAD 30   // the radius of the flag position marker
//#define CL_SHOW_TIME_LEFT

// ----

#include <vector>
#include <list>
#include <fstream>
#include <sstream>
#include <iomanip>

/* SV_SHIFTY is used for bounce checks: 15 aligns with the map, 0 is the buggy default behaviour */
#ifdef SV_SERVER_PHYSICS
#define SV_SHIFTY 15
#else
#define SV_SHIFTY 0
#endif

#ifdef SV_NAME_AUTHORIZATION
#include "nameauth.h"
#endif

template<class T> T bound(T val, T lb, T hb) { return val<=lb?lb:val>=hb?hb:val; }

// strspnp: (Watcom definition) find from str the first char not in charset
char* strspnp(char* str, const char* charset);
const char* strspnp(const char* str, const char* charset);

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

// ---- client screen layout ----

//resolution
#define RESOL_X 640
#define RESOL_Y 480

//play area offset
#define plx 0
#define ply 90

//play area width/height
#define plw 472
#define plh 354

//minimap offset
#define mmx (plx + plw + 16)    //push 8 to left
#define mmy ply

//scoreboard offset
#define sbx (plx + plw)
#define sby (mmy + 110)         // + XXX = minimap panel height


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

struct RectWall {	// rectangular wall
	int a, b, c, d;	// rectangle coords (a,b)->(c,d)
	int tex;	// texture id
	int alpha;

	RectWall() { }
	RectWall(int a_, int b_, int c_, int d_, int tex_, int alpha_)
			: a(a_), b(b_), c(c_), d(d_), tex(tex_), alpha(alpha_) { if (c<a) swap(a, c); if (d<b) swap(b, d); }
	bool intersects_rect(float x1, float y1, float x2, float y2) const { return x1<=c && x2>=a && y1<=d && y2>=b; }
};

struct TriWall {	// triangular wall
	int x1, y1, x2, y2, x3, y3;
	int boundx1, boundy1, boundx2, boundy2;
	int tex, alpha;

	TriWall() { }
	TriWall(int x1_, int y1_, int x2_, int y2_, int x3_, int y3_, int tex_, int alpha_)
			: x1(x1_), y1(y1_), x2(x2_), y2(y2_), x3(x3_), y3(y3_), tex(tex_), alpha(alpha_) {
		if (y2<y1) { swap(x1, x2); swap(y1, y2); }	// 1, 2 sorted
		if (y3<y2) {
			swap(x2, x3); swap(y2, y3);	// 1, 3 and 2, 3 sorted
			if (y2<y1) { swap(x1, x2); swap(y1, y2); }	// all sorted
		}
		boundx1=min(x1, min(x2, x3)), boundy1=min(y1, min(y2, y3));
		boundx2=max(x1, max(x2, x3)), boundy2=max(y1, max(y2, y3));
	}
	bool intersects_rect(float x1, float y1, float x2, float y2) const;
};

struct Room {
	vector<RectWall> rwalls, rground;	// ground: optional list of textures for ground [not used]
	vector<TriWall>  twalls, tground;

	void draw(BITMAP* buffer, float x0, float y0, float xScale, float yScale, int color) const {
		for (vector<RectWall>::const_iterator rwi=rwalls.begin(); rwi!=rwalls.end(); ++rwi)
			rectfill(buffer, int(x0+xScale*rwi->a), int(y0+yScale*rwi->b), int(x0+xScale*rwi->c), int(y0+yScale*rwi->d), color);
		for (vector<TriWall>::const_iterator twi=twalls.begin(); twi!=twalls.end(); ++twi)
			triangle(buffer,
					int(x0+xScale*twi->x1), int(y0+yScale*twi->y1),
					int(x0+xScale*twi->x2), int(y0+yScale*twi->y2),
					int(x0+xScale*twi->x3), int(y0+yScale*twi->y3), color);
	}
	bool fall_on_wall(int x1, int y1, int x2, int y2) const {
		for (vector<RectWall>::const_iterator rwi=rwalls.begin(); rwi!=rwalls.end(); ++rwi)
			if (rwi->intersects_rect(x1, y1, x2, y2))
				return true;
		for (vector<TriWall>::const_iterator twi=twalls.begin(); twi!=twalls.end(); ++twi)
			if (twi->intersects_rect(x1, y1, x2, y2))
				return true;
		return false;
	}
};

//entity locale
struct spoint_t {
	int px, py;	//screen (if px == -1, unused)
	int x, y;	//relative (to screen) X,Y position
};

//team info
struct teaminfo_t {
	spoint_t flag;	//flag position
	vector<spoint_t> spawn;	//team spawn points
	unsigned int lastspawn;	//last team spawn point used

	teaminfo_t() : lastspawn(0) { }
};

class Map {
	bool parse_label(FILE *f, const char *label, int crx, int cry);	// crx,cry = "current room pointer"

public:
	bool valid_for_scoring;	//v0.4.7: map is valid for scoring?
	teaminfo_t tinfo[2];	//team information for red=0 and blue=1 teams
	vector< vector<Room> > room;

	string title;	//map title
	int	ver;	// map version
	int w, h;	// width height
	NLushort crc;	//map's 16bit CRC

	Map() : valid_for_scoring(true), ver(-1), w(0), h(0), crc(0) { }

	bool fall_on_wall(int px, int py, int x1, int y1, int x2, int y2) const {
if (px<0 || py<0 || px>=w || py>=h) return false;	//#fix: remove this and track why these are given sometimes
		assert(px>=0 && py>=0 && px<w && py<h);
		return room[px][py].fall_on_wall(x1, y1, x2, y2);
	}
	void draw_minimap(BITMAP* buffer, bool flagPaintSimple=false) const;
	bool load(FILE* f);
};

//************************************************************
//  the game frame common stuff
//************************************************************

// a player's record.
struct player_t {

	bool			used;		// player record valid?

	//server-side flag: waiting for this client to say that it has all the resources needed to play
	bool			awaiting_client_ready;

	//player registration status
	char			reg_status;
	int				score, rank;
	int				neg_score;			// V0.4.8 NEW VAR

	int				weapon;	// poder da arma atual (nivel) 0,1,2,3,4 (...?)

	bool			want_map_exit; //server side - player quer sair p/ proximo mapa na rotacao

	#ifdef SV_CONSOLE
	int mapVote;
	#endif
	typedef list< pair<int, string> > DMQueueT;
	DMQueueT delayedMessages;	// int is the # of server frames the message has delay after the previous one
	int kickTimer;
	int muted;	// 0 = no, 1 = yes, 2 = silently

	bool	want_change_teams;		//server-side: player wants do change teams?
	double	team_change_time;		//server-side time to allow next team change
	bool	team_change_pending;	//server-side hack

	double	speed_drop_time;		//speed powerup FX aux var
	double	wall_sound_time;		// min time to play sound again

	bool	onscreen;	//player onscreen? used only in clientside

	NLulong	enemyvis;	//enemies being viewed . clientside only

	//DEATHBRINGER
	bool	item_deathbringer;	// DEATHBRINGER -- if player carries it
	long	item_deathbringer_time;	//time of start explosion (server simulation)
	double	deathbringer_end;	// end time of deathbringer effect on this player
	bool	deathbringer_affected;		//CLIENT-SIDE: draw/spawn affected deathbringer effect
	double	death_drop_time;
	int		deathbringer_attacker;		//the attacker

	bool	item_shield;		// SHIELD: bit sent always: shield fx / shield present
	bool	item_quad;			// QUAD damage
	bool	item_speed;			// SPEED BOOTS
	int		item_helm;			// HELM 0== no   1+ == yes, alpha  (-1)

	double	item_quad_time;		// time of expiring (to print on clientside screen)
	double	item_speed_time;
	double	item_helm_time;

	double	quad_sound_finished;	// to avoid too much quad sound

	double	hitfx;		// player-hit fx (relative to time): clientside only

	bool	attack;		// if player is holding attack button

	int		id;			// player's id (position on the vectr)

	int		cid;		// client id (network identity)

	char	name[64]; // player's name

	double	waitnametime;		// protect from name change flooding

	int		x, y;			// position in world (screen coordinates)

	int		oldx, oldy;		// old positions (to detect screen changing)

	int		drawptr;		// HACK: id of the player to draw (depth sorting for drawing order)
	int		drawused;		// HACK: id of the player to draw (depth sorting for drawing order)

	int		ping;				// the ping time

	int		frags;			// integer number displayed on the scoreboard ("frags")
	int		oldfrags;		// last value informed to the client

	int		health;			// current health (sent always 2-byte)

	int		energy;			// player's energy (run/shoot)

	int		megabonus;	// bonus left de health e energy

	bool    dead;
	bool	old_dead;		// to detect time to play death sound

	bool	dropped_flag;   // Has player dropped the flag?

	double	next_shoot_time;		// minimum time for next shoot

	double	respawn_time;				// time for respawn

	bool	respawn_to_base;		// force respawning to base

	//talk flood control
	double	talk_temp;			//talk temperature
	double	talk_hotness;		//hotness of talk action

	//admin shell stats
	int		total_kills;
	int		total_deaths;
	int		most_consecutive_kills;
	int		current_consecutive_kills;
	int		most_consecutive_deaths;
	int		current_consecutive_deaths;
	int		total_suicides;
	int		total_captures;
	int		total_flags_taken;
	int		total_flags_dropped;
	int		total_flags_returned;
	int		total_flag_carriers_killed;
	int		total_shots;
	int		total_hits;
	int		total_shots_taken;
	int		last_spawn_time;
	int 	lifetime;
	double	total_movement;
	int		start_time;

	void reset_message_queue_timing() {	// make messages already on queue appear instantly
		for (DMQueueT::iterator m=delayedMessages.begin(); m!=delayedMessages.end(); ++m)
			m->first=0;
	}
	void add_to_queue(const string& str) {
		int time;	// in server frames (1/10 sec)
		if (delayedMessages.size()<=5)
			time=0;
		else
			time=30;
		delayedMessages.push_back(pair<int, string>(time, str));
	}
	void queue_printf(const char* fmt, ...) {
		char buf[16385];
		va_list argptr;
		va_start(argptr, fmt);
		vsprintf(buf, fmt, argptr);
		va_end(argptr);
		add_to_queue(string(buf));
	}
	void clear(bool enable, int _pid, int _cid, const char* _name) {
		ping = 0;
		frags = 0;	//reset score ?
		oldfrags = -666;
		want_map_exit = false;		//by default don't want change maps
		#ifdef SV_CONSOLE
		mapVote=-1;
		#endif
		delayedMessages.clear();
		kickTimer=0;
		muted=0;
		want_change_teams = false;	// don't want to change teams yet
		team_change_time = 0;
		team_change_pending = false;
		next_shoot_time = 0;
		attack = false;
		reg_status = ' ';
		talk_temp = 0.0;
		talk_hotness = 1.0;

		cid=_cid;
		id = _pid;
		strcpy(name, _name);	//the default name
		waitnametime = get_time() - 666.0;	//can change name right now
			//default stats
		total_kills = 0;
		total_deaths = 0;
		most_consecutive_kills = 0;
		current_consecutive_kills = 0;
		most_consecutive_deaths = 0;
		current_consecutive_deaths = 0;
		total_suicides = 0;
		total_captures = 0;
		total_flags_taken = 0;
		total_flags_dropped = 0;
		total_flags_returned = 0;
		total_flag_carriers_killed = 0;
		total_shots = 0;
		total_hits = 0;
		total_shots_taken = 0;
		total_movement = 0;
		start_time = (int)get_time();
		last_spawn_time = start_time;
		lifetime = 0;

			//score...  (V0.4.8 -- was missing!)
		score = 0;
		neg_score = 0;
		rank = 0;

		awaiting_client_ready=false;
		weapon=0;
		speed_drop_time=wall_sound_time=0;
		onscreen=false;
		enemyvis=0;
		item_deathbringer=deathbringer_affected=false;
		item_deathbringer_time=0;
		deathbringer_end=death_drop_time=0;
		deathbringer_attacker=0;
		item_shield=item_quad=item_speed=false;
		item_helm=0;
		item_quad_time=item_speed_time=item_helm_time=0;
		quad_sound_finished=hitfx=0;
		x=y=oldx=oldy=0;
		drawptr=drawused=0;
		health=energy=megabonus=0;
		dead=old_dead=false;
		dropped_flag=false;
		respawn_time=0;
		respawn_to_base=false;

		if (enable) {
			reg_status='-';
			used=true;
		}
		else
			used=false;
	}

};

// a player's sprite state
struct hero_t {
	int tx, ty;		//tela X,Y
	double x, y, sx, sy;	// position and speed
	double ox, oy;	// old coords: garantidamente NAO em paredes
	bool l, r, u, d;	// left, right, up, down keypresses (player acceleration vectrs)
	int gundir;	// gun direction 0-7 (0 = right 1 = right-down 2 = down ...... 7 = right-up
	bool strafe;	// strafing?
	bool run;	//running

	//keypresses (net format)
	// 0,1,2,3 : lrud
	// 4 : strafe?
	// 5,6,7 : gun direction

	//CLIENT writes: l,r,u,d,strafe,shift,unused(2)

	//SERVER writes: l,r,u,d,shift,gundir(3)
	NLubyte		keys;
};

// a rocket-shot
class rocket_c {
public:

	//owning player-id (-1 == unused)
	int	owner;

	//don't draw flag & remove schedule (CLIENT-SIDE): se dontdraw==true, nao desenha em client side e remove quando tempo >= clremove
	bool dontdraw;
	double clremove;

	//team/color
	int team;

	//power	(na verdade a partir da versao 0.1.2, cada rocket pode ser um multi-rocket!
	//NLubyte		power;

	//notification list (bitfield, bit0=player0, bit1=player1... etc.)
	NLulong		vislist;

	//hit position
	NLshort hitx, hity;

	//screen coords
	int px, py;

	//start position or current position
	double x, y;

	//v0.1.2 - how long it moved (in pixels) since creation
	double d;

	//v0.1.2 - em graus : direcao
	double deg;

	//speed
	double sx, sy;

	//time of shot or current time
	NLulong time;

	//time for effective calculation on clientside (not always integer)
	double cl_time;

	//time-of-hit do rocket clientside
	double hit_time;

	//hit_target. se ==255, ninguem em particular.  se ==254 hit wall
	int hit_target;

	rocket_c() { owner = -1; }
};

struct ctflag_t {

	//carried? else dropped somewhere
	bool			carried;

	//if not carried, dropped at base?
	bool			atbase;

	//who owns it if carried
	int				carrier;

	//score of the "flag" (team score)
	int				score;

	//0.4.7 tempo em que adversario pegou a flag do estande na base
	double		grab_time;

	//where is it if dropped
	spoint_t	pos;
};

//pickups
class pickup_c {
public:

	NLubyte kind;		// type of powerup  0==unused     255=valid, but respawning

	double		respawn_time;		// time to respawn

	int				px;	//screen
	int				py;
	int				x;	//position
	int				y;

	pickup_c() { kind=0; }
};

// a game frame, or game state
class frame_t {
public:

	// frame is invalid -- when frame is skipped in the broadcast
	bool skipped;

	// frame number  (simulation time)
	double		frame;

	// real time (clientside) of the frame
	double		time;

	// the player's sprite positions
	hero_t		hero[MAX_PLAYERS];

	// flag objects - one for each team
	ctflag_t		flag[2];

	// rockets
	rocket_c	rock[MAX_ROCKETS];

	// pickups
	pickup_c	item[MAX_PICKUPS];

	//ctor
	frame_t() {
		frame = 0;
		time = 0;
		for (int i=0;i<MAX_PLAYERS;i++)
			memset(&hero[i], 0, sizeof(hero_t));
	}

	//dtor
	virtual ~frame_t() {
	}
};

bool load_map(const char *mapdir, const string& mapname, Map *map);
bool NR_applyPhysics(hero_t* h, const Room& room, float fraction, bool turbo, bool carryFlag, bool deathbringer_affected);
#if !defined(SV_SERVER_PHYSICS)
bool applyDefaultPhysics(hero_t* h, const Room& room, float fraction, bool turbo, bool carryFlag, bool deathbringer_affected) {
#endif

extern int listen_port_running;
extern volatile bool	listen_server_running;
extern pthread_t	listen_server_thread;
void listen_start();
void listen_stop();

#endif
