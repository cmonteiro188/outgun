// commont.h : temporary common stuff

#ifndef COMMONT_H_INC
#define COMMONT_H_INC

#define SV_SERVER_PHYSICS
//#define SV_PHYS_VECTOR_ACC

// ---- server side defines

#define SV_CONSOLE	// enable console commands
#define SV_NAME_AUTHORIZATION	// enable player IP based filtering : name authorization and ban
#define SV_NO_PUP_SWITCHING	// disable the changing of power-ups lying on the ground
#define SV_VOTE_ANNOUNCE_INTERVAL 5	// in seconds, how often a changing voting status will be announced
#define SV_SHADOW_MINIMUM_NORMAL 7	// the shadow visibility factor

// ---- client side defines

#define CL_MINIMAP_FLAGPOS	// paint minimap more intelligently according to flag positions
#define CL_SHOW_FLAGPOS	// show a flag position marker on the ground
#define CL_FLAGPOS_RAD 30	// the radius of the flag position marker
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

//#define WINMODE GFX_GDI		-- can't pageflip

//#define WINMODE GFX_DIRECTX_ACCEL
//#define FULLMODE GFX_DIRECTX_ACCEL

#define WINMODE GFX_AUTODETECT_WINDOWED

#define FULLMODE GFX_AUTODETECT

//DEBUGGING ranking?
//#define DEBUG_RANKING

//same as PLAYER RADIUS (15) + ROCKET RADIUS (3)
#define	SHOT_DELTAX	17	// V0.4.8 : A HAIR LESS!

//minimum time between flag steal at base and capture, to consider a map to be valid for scoring
#define MINIMUM_GRAB_TO_CAPTURE_TIME 6.0

//RANKING defines
#define DEFAULT_PLAYER_RATE 1.0
#define MINIMUM_POSITIVE_SCORE_FOR_RANKING 100

//#define SWITCH_PAUSE_CLIENT

//#define ALWAYS_FRICTION

#define PI M_PI //3.1416

#define PIOIT M_PI_4 //0.7854 //DOIS PI SOBRE 8 = PI SOBRE 4 = 0.7854

#define PASSBUFFER	32		//size of password file

//quick debugs
//#define MIN_ALPHA_FRIENDS 1			//debug value
#define MIN_ALPHA_FRIENDS 64

#define ROCKET_SPEED 50.0		//in pixels/0.1s

#define MIN_HEALTH_FOR_RUN_PENALTY 40

#define NUMBER_OF_POWERUP_KINDS 7	//quad shield shadow turbo weapon-up megahealth deathbringer

//#define DEBUG_POWERUPS
//#define REALLY_DEBUG_POWERUPS		//define only if DEBUG_POWERUPS defined

// GAME VERSION / GAME STRING
//
#define GAME_STRING "Outgun"
#define GAME_PROTOCOL "14"
#define GAME_VERSION "0.5.0-E"

#define TK1_VERSION_STRING "v048"

#include "allegro.h"	// Allegro

//patching main / _main / WinMain link errors...
#ifdef ALLEGRO_WINDOWS
#include "winalleg.h"
#include "windows.h"
#endif

#include <stdio.h>			// for -text (v0.5.0)

#include "nl.h"				// HawkNL

#include "leetnet/server.h"		// l33t server
#include "leetnet/client.h"		// l33t client
#include "leetnet/rudp.h"			// get_self_I
#include "leetnet/sleep.h"		// sleep util

#include "string.h"
#include "math.h"

#include "pthread.h"
#include "sched.h"

#include <string>
using namespace std;
#include "names.h"		//the COOLEST random-name generator by Renato Hentschke

//admin shell protocol
#include "admshell.h"

//log utils
//#define LOG_NOLOG		// uncomment to disable logging
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
#define mmx (plx + plw + 16)	//push 8 to left
#define mmy ply

//scoreboard offset
#define sbx (plx + plw)
#define sby (mmy + 110)			// + XXX = minimap panel height


//************************************************************
//  common stuff
//************************************************************

//the default game port
#define DEFAULT_UDP_PORT 25000

//the master server address (www.mycgiserver.com:80)
extern NLaddress		master_address;

//directories for save/load maps
#define SERVER_MAPS_DIR "maps"
#define CLIENT_MAPS_DIR "cmaps"

//root path (game executable path)
#define WHERE_PATH_SIZE 256
extern char wheregamedir[WHERE_PATH_SIZE];

//function that resets the video mode
bool reset_video_mode();

// server game phisics parameters
extern double svp_fric, svp_accel, svp_maxspeed;
extern double svp_fric_run, svp_accel_run, svp_maxspeed_run;
extern double svp_fric_turbo, svp_accel_turbo, svp_maxspeed_turbo;
extern double svp_fric_turborun, svp_accel_turborun, svp_maxspeed_turborun;
extern double svp_flag_penalty;

void set_default_physics();

// number-of-players
#define MAX_PLAYERS	32							// the MAXIMUM MAXIMUM number of players EVER
extern int			maxplayers;		// the maximum number of players configured for this server (must be <= MAX_PLAYERS and an EVEN NUMBER == NUMERO PAR)
#define TSIZE (maxplayers/2)				// CTF TEAM SIZE

#define MAX_ROCKETS 256		// maximum number of rockets (nao pode ser mais que 256 pq eh usado um unsigned char p/ passar ids)

#define MAX_PICKUPS MAX_PLAYERS	// the MAXIMUM MAXIMUM number of pickups laying on the ground at one time in the game

//arg switches (+ default values)
extern bool dedserver;		//dedicated server? -ded
extern bool textserver;		//textmode dedicated server for UNIX/LINUX (V0.5.0) (WON'T WORK ON WINDOWS...)
extern bool privateserver;	//private server? (will not publish)
extern bool winclient;		//windowed client?	-win / -fs
extern bool trypageflip;	//try page flipping? -flip / -dbuf
extern bool nosound;			//disable sound? -nosound
extern int targetfps;			//target (MAX) frames-per-second
extern int port;				//the server port
extern bool showinfo;		//apenas show info e desliga server
#define TARGET_PRIO_UNSPECIFIED -666666
extern bool defaultprio;	//select default server threads priority
extern int targetprio;	//unspecified
extern int server_maxplayers;		//default maxplayers of the server
extern bool sound_inited;		//install_sound succeeded?
extern bool sound_enabled;		// player wants sounds?
extern bool no_tcp_download;		// V0.4.7: CHANGED DEFAULT : disable use of the TCP socket for file transfers (use the regular UDP leetnet connection)
extern bool force_ip;		//force IP?
extern char force_ip_name[32];		//force IP to what?

void server_status_string(char *str);
double get_time();

extern volatile int server_speed_counter;
extern volatile int speed_counter;
extern volatile int client_netsend_counter;
extern volatile unsigned long time_counter;

//audio samples : codes
enum {

	SAMPLE_FIRE,			//ok
	SAMPLE_HIT,				//ok

	SAMPLE_WALLHIT,				//new! v0.3.9
	SAMPLE_QUADWALLHIT,		//new! v0.3.9

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

extern int teamcol[2];
extern int teamlcol[2];	//light colours for statusbar
extern int teamdcol[2];	//dark colours for player name

extern char teamname[2][5];

extern int	col[NUM_OF_COL];
void setcolors();

//server record
struct gamespy_t {
	NLaddress addr;
	char address[128];	//IP-address typein buffer
	bool invalid;
	bool noresponse;
	bool favs;	//hack
	bool refreshed;	//if data below is valid -------------
	char info[128];
};

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


//per-client struct (statically allocated to a single client)
class oneclient_c {
public:

	//v0.4.4 UDP FILE transfer
	bool		serving_udp_file;			//if TRUE, already serving a file
	char		*data;					//the file data
	NLulong		dp,old_dp,fsize;				//the file pointer and the total size

	//v0.4.4 PLAYER REGISTRATION STATUS
	bool		token_have;					//player claims to be registered with (name,token)
	bool		token_valid;		//player (name,token) is validated
	char		token[64];					//the player's token
	int			intoken;						//integer version of token

	//v0.4.4 client statistics
	int			delta_score;		//the player's score accumulator
	int			neg_delta_score;		//NEG score accum 0.4.8

	double fdp;		//DOUBLE delta accums. os acima sao apenas o "trunc atual"
	double fdn;

	int			rank;						//current ranking position
	int			score;					//current score POS -- SOMATORIO (né?!?!?)
	int			neg_score;					//current score NEG 0.4.8 -- SOMATORIO (né?!?!?)

	oneclient_c() {
		serving_udp_file = false;
		data = 0;
		reset();
	}

	//chamado no fim do UDP!
	void download_reset() {
		if ((serving_udp_file) && (data)) {
			delete data;
			data = 0;
		}
		serving_udp_file = false;
	}

	void reset() {
		delta_score = 0;
		neg_delta_score = 0;
		fdp = 0.0;
		fdn = 0.0;
		score = 0;
		neg_score = 0;
		rank = 0;

		token_have = false;
		token_valid = false;
		token[0]=0;
		intoken=666;

		download_reset();
	}

	~oneclient_c() {
		download_reset();
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

//master job struct
class masterjob_c {
public:

	char								request[512];		//http request to be sent

	bool		html_end;			//received a response(request fullfilled)

	char				lebuf[65536];		//lebuf for collecting response
	int					n;			//lebuf length

	int			code;			//job code

	//VARS FOR EACH SPECIFIC JOB CODE
	int			cid;		//code 1 - client id

	//return values of the callback
	bool		retry;		//if true, wait a bit and retry

	masterjob_c() {
		lebuf[0]=0;
		html_end = false;
		request[0]=0;
	}
};

class gameserver_c {
public:
	Map		map;
	server_c	*server;
	char hostadname[128];
	NLsocket		msock;
	pthread_t		mthread;
	double			master_talk_time;	//time to talk?
	bool				master_pre_exiting_ok;		// if no need to kill the master socket...
	bool				master_exiting_ok;		// if no need to kill the master socket...
	bool				master_never_talked;		// if never talked to master, then no need to unregister the server when qutting (optimization)
	bool							mjob_exit;				//flag for all pending master jobs to quit now
	bool							mjob_fastretry;		//flag for all pending master jobs to stop waiting and retry immediately
	int								mjob_count;
	pthread_mutex_t		mjob_mutex;  //mutex for socket list
	bool							file_threads_quit;		//terminate all file server threads/sockets now
	NLsocket					filesock;
	pthread_t					server_filemaster_thread;  // thread for server filemaster
	pthread_mutex_t		fslavesock_mutex;  //mutex for socket list
	NLsocket					fslavesock[MAX_PLAYERS];
	pthread_t					fslavethr[MAX_PLAYERS];
	NLsocket		shellmsock;
	pthread_t		shellmthread;
	NLsocket		shellssock;
	pthread_t		shellsthread;
	char		hostname[256];
	vector<string> welcome_message;	// welcome message line by line
	vector<string> info_message;	// the message /info shows, line by line
	string sayadmin_comment;
	bool sayadmin_enabled;
	player_t	player[MAX_PLAYERS];
	int				ctop[256];			// client id-to-player id index
	oneclient_c	client[MAX_PLAYERS];
	int max_world_score, max_world_rank;
	double team_smul[2];
	frame_t		world;
	NLulong		frame;
	double server_kbps_traffic;
	int ping_send_counter, ping_send_client;
	struct MapInfo {
		string title, file;
		int width, height;
		int votes;
		MapInfo();
		bool load(string mapName);
	};
	vector<MapInfo> maprot;
	int currmap;		// current map in maprot
	#ifdef SV_NAME_AUTHORIZATION
	NameAuthorizationDatabase authorizations;
	#endif
	bool random_maprot;
	NLulong next_vote_announce_frame;
	int last_vote_announce_votes, last_vote_announce_needed;
	NLulong map_start_time;	// frame #
	bool	gameover;
	double		gameover_time;		//timeout for gameover plaque
	NLulong time_limit;
	int capture_limit;
	int vote_block_time;	// how long a mapchange can't be voted (except unanimously), in frames (in gamemod, it is minutes)
	int pups_min, pups_max, pups_respawn_time, pup_chance_shield, pup_chance_turbo, pup_chance_shadow,
			pup_chance_power, pup_chance_weapon, pup_chance_megahealth, pup_chance_deathbringer;
	bool pups_min_percentage, pups_max_percentage;
	int pup_add_time, pup_max_time;
	bool pup_deathbringer_switch;
	int shadow_minimum;	// smallest alpha value allowed; 1 is when even the coordinates are not sent
	double respawn_time, waiting_time_deathbringer;
	gameserver_c();
	virtual ~gameserver_c();
	void mutePlayer(int pid, int mode);
	void kickPlayer(int pid, bool ban=false);
	#ifdef SV_NAME_AUTHORIZATION
	void banPlayer(int pid);
	#endif
	int choose_powerup_kind();
	void upload_next_file_chunk(int i);
	int get_download_file(char *lebuf, char *ftype, char *fname);
	void run_filemaster_thread(void *);
	void run_fileslave_thread(void *arg);
	void send_me_packet(int pid);
	void send_player_name_update(int cid, int pid);
	void broadcast_player_name(int pid);
	void send_player_crap_update(int cid, int pid);
	void broadcast_player_crap(int pid);
	int check[MAX_PLAYERS];
	int checount;
	void check_team_changes();
	void check_player_change_teams(int pid);
	void move_update_player(int a);
	void move_player(int f, int t);
	void swap_players(int a, int b);
	void broadcast_sample(int code);
	void broadcast_screen_sample(int p, int code);
	void ctf_net_flag_status(int cid, int team);
	void ctf_return_flag(int team);
	void ctf_drop_flag(int team, int px, int py, int x, int y);
	void ctf_steal_flag(int team, int carrier);
	void ctf_update_teamscore(int t);
	void game_respawn_player(int pid);
	void game_delete_rocket(int r, NLshort hitx, NLshort hity, int targ);
	void make_damn_rocket(int i, int playernum, int px, int py, int x, int y, double deg, int xdelta);
	NLubyte game_do_shoot_rocket(int playernum, int px, int py, int x, int y, double deg, int xdelta);
	void game_shoot_rocket(int playernum, int shots, int px, int py, int x, int y, int gundir);
	bool ctf_drop_flag_if_any(int pid);
	void refresh_team_score_modifiers();
	void score_frag(int p, int amount);
	void score_neg(int p, int amount);
	void client_report_status(int id);
	void game_reset_player(int target, float time_penalty = 0.);
	void game_kill_player(int target, bool time_penalty);
	void game_damage_player(int target, int attacker, int damage, bool deathbringer);
	void game_remove_player(int pid);
	void ctf_game_restart();
	void respawn_pickup(int p);
	int pups_by_percent(int percentage) const;
	void check_pickup_creation(bool instant);
	void game_touch_pickup(int p, int pk);
	void game_player_screen_change(int p);
	void broadcast_team_message(int team, char *text);
	void broadcast_screen_message(int px, int py, char *lebuf, int count);
	void bprintf(const char *fs, ...);
	void plprintf(int pid, const char* fmt, ...);
	void player_message(int pid, const char *text);
	void broadcast_message(const char *text);
	void load_game_mod();
	bool load_rotation_map(int pos);
	void send_map_change_message(int pid, int reason, const char* mapname);
	bool server_next_map(int reason);
	void check_map_exit();
	bool reset_settings();
	bool start(int target_maxplayers);
	void reload_hostname();
	void update_serverinfo();
	int client_connected(int id);
	void client_disconnected(int id);
	void ping_result(int client_id, int ping_time);
	void incoming_client_data(int id, char *data, int length);
	bool check_flag_touch(int px, int py, int x, int y, int t);
	void run_server_player_physics(int i, frame_t *src, frame_t *dest);
	void simulate_and_broadcast_frame();
	void server_think_after_broadcast();
	void loop(volatile bool *running_flag);
	void master_job_response(masterjob_c *j);
	void run_masterjob_thread(void *arg);
	bool check_private_IP(char *address);
	void run_mastertalker_thread(void *);
	bool read_string_from_TCP(NLsocket sock, char *buf);
	void run_shellmaster_thread(void *);
	void run_shellslave_thread(void *);
	void stop();
	char *get_hostname();
};

//************************************************************
//  client stuff
//************************************************************

// number of chat messages in the buffer
#define CHAT_SIZE 8

// size of clientside visual fx array
#define MAX_CLIENTFX 128

// size of connect screen
#define MAX_GAMESPY 24

// size of udp download queue (only 1 should be needed but...)
#define MAX_UDPDQ 16

// drawing screens
extern BITMAP *drawbuf, *vidpage1, *vidpage2, *backbuf;
extern bool		page_flipping;

//explosion clientside fx
struct clientfx_t {

	bool		used;		//used record?

	int			type;		// type of fx	0==gun explosion
	int			px,py;	// screen where it spawned. if changed when time to redraw, delete it
	double time;		// start time

	//fx specific vars
	int x;					// screen x  of fx
	int y;					// screen y  of fx

	//speed fx
	int col1, col2, gundir;

	//deathbringer owner
	int owner;
};

struct download_runes_t {

	int did;		//download id

	char type[64];	//type of file to download
	char name[256];	//name of file to download
	char dest[512];	//full destination path+name for downloaded file

};

class gameclient_c {
public:
	Map		map;
	player_t player[MAX_PLAYERS];
	clientfx_t		cfx[MAX_CLIENTFX];
	int	me;
	bool option_show_names;
	bool player_password_set;	//flag for the thread
	bool player_token_new;		//TRUE if first call to token servled
	bool player_token_set;		//TRUE if player now holds a valid token
	char player_token[64];		// the token
	char player_password[16];
	pthread_t	passthread;
	frame_t		fd, fx;
	pthread_mutex_t		frame_mutex;
	pthread_mutex_t		udpdq_mutex;
	int udpdq_size;		//size
	download_runes_t		*udpdq[MAX_UDPDQ];		//the udp download queue
	int udpdq_ptr;		//current download. if -1, no current downloads
	int ud_fp;			//file pointer for read/write
	FILE *ud_fout;	//input or output file
	NLulong fdp, fdp_max;
	NLulong max_world_score, max_world_rank;
	double lastpackettime;
	client_c *client;
	SAMPLE *sample[NUM_OF_SAMPLES];
	bool sample_reverse[NUM_OF_SAMPLES];
	char sfxthemedir[256];
	char sfxthemename[256];
	al_ffblk	sfxthemeffblk;	//for al_find*
	bool	validtheme;		// if sfxthemedir points to valid dir
	bool menushow;
	int menu;		//menu screen #
	bool gameshow;
	bool helpshow;
	double FPS;
	int framecount, totalframecount;
	double starttime;
	bool want_change_teams;
	bool want_map_exit;
	int scoreboard[MAX_PLAYERS];
	volatile bool trying_connection;
	volatile bool connected;
	char	hostname[256];
	int		strlen_hostname;	//strlen precalculado
	bool				showmaster;		//showing master screen (opposite: showing favourites screen)
	bool				first_fav_refresh;		//first refresh of favorites page already done?
	gamespy_t		gamespy[MAX_GAMESPY];
	int					gi;	//what game entry
	gamespy_t		mgamespy[MAX_GAMESPY];		//gamespy of masterserver
	char playername[256]; //the player's name (max name len = 16)
	char namestatus[64];		// v0.4.4: NAME STATUS (unregistered, registering..., registered!)
	char editplayername[256]; //the player's name edit buffer
	char address[256];		//server IP address
	char dialogmessage[256];	//dialog message
	char dialogmessage2[256];	//dialog message line 2
	char talkbuffer[256];			// chat input buffer
	char chatbuffer[CHAT_SIZE][256];		// last chat messages list
	double chaterasetime;				// time to erase a chat message from the list
	char editplayerpass[64]; //the player's password edit buffer
	char namecursor[2];
	char passcursor[2];
	int		namestatus_code;		//0==NONE  1==LOGGED w/ token  2==LOGIN FAILED by last attempt  3==LOGGED+RECORDING
	BITMAP *minibg;
	BITMAP* flagpos_buf[2];
	bool flagpos_ready;
	bool map_ready;
	char servermap[64];	//last map command from server
	int gameover_plaque;
	int red_final_score, blue_final_score;		//final scores for showing in the gameover plaque
	bool server_no_tcp;
	BITMAP *hostad;
	char    hostadname[128];
	bool message_logging;
	ofstream message_log;

	void check_flagpos_marks();
	bool start();
	void send_client_ready();
	void check_change_pass_command();
	void client_password_thread(void *);
	void process_udp_download_chunk(int last, NLulong pos, int len, char* buf);
	void client_udp_setup_download();
	void client_udp_download(download_runes_t  *rune);
	void download_file_complete(download_runes_t  *r);
	void download_server_file(const char *type, const char *name, char *dest);
	void client_download_thread(void *arg);
	void server_builtin_map_command(NLubyte num);
	void server_map_command(const char *mapname, NLushort server_crc);
	void next_sfx_theme();
	void make_sfx_theme_path(char *themepath, char *themedir);
	void set_theme_dir(char *dirname);
	SAMPLE *load_outgun_sample(char *fname, int slot, bool try_redirect = true, bool reverse = false);
	void load_samples();
	void unload_samples();
	void sound(int s);
	void clear_fx();
	int get_new_cfx();
	void cfx_create_wallexplo(int x, int y, int px, int py);
	void cfx_create_quadwallexplo(int x, int y, int px, int py);
	void cfx_create_deathbringer(int owner, double start_time, int x, int y, int px, int py);
	void cfx_create_deathcarrier(int x, int y, int px, int py, int team);
	void cfx_create_gunexplo(int x, int y, int px, int py);
	void cfx_create_speedfx(int x, int y, int px, int py, int col1, int col2, int gundir);
	void update_scoreboard();
	void calc_game_frame();
	void draw_flag_at(BITMAP *drawbuf, int t, int x, int y);
	void draw_mini_flag(BITMAP *drawbuf, int whatteam);
	void update_minimap_background();
	void draw_player(BITMAP *drawbuf, int x, int y, int gundir, int pc1, int pc2, int alpha);
	void draw_game_frame(BITMAP *drawbuf);
	void draw_game_help();
	void draw_game_menu();
	void set_menu(int menumber);
	void disconnect_command();
	void client_connected(char *data, int length);
	void client_disconnected();
	void connect_failed_denied(char *data, int length);
	void connect_failed_unreachable();
	void refresh_command();
	void refresh_command_2(gamespy_t *gamespy);
	void connect_command();
	void send_player_token();
	void issue_change_name_command();
	void change_name_command();
	void send_frame();
	void client_set_rocket(int id, int dir, NLulong frameno, int owner, int px, int py, int x, int y, int xdelta);
	void client_rebuild_shot(int pow, int dir, int *rids, NLulong frameno, int owner, int px, int py, int x, int y);
	void process_incoming_data(char *data, int length);
	void send_chat(char *msg);
	void erase_first_message();
	void print_message(const char *msg);
	void save_screenshot();
	void toggle_help();
	void show_progress(char *t1, char *t2, char *t3, int fg = -1, int bg = 0);
	void show_dialog(char *t1, char *t2, char *t3, int fg = -1, int bg = 0);
	void get_servers_from_master();
	void loop();
	void stop();
	gameclient_c();
	virtual ~gameclient_c();
};

extern gameclient_c *gameclient;
extern gameserver_c *gameserver;

#endif
