#ifndef WORLD_H_INC
#define WORLD_H_INC

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
	bool load(const char *mapdir, const string& mapname);
};

class PlayerBase {
protected:
	PlayerBase() { }

public:
	int weapon;

	bool item_deathbringer;
	bool item_shield;
	bool item_quad;
	bool item_speed;
	int item_helm;	// 0 == no   1+ == yes, alpha

	bool attack;	// if player is holding attack button

	int roomx, roomy;
	double lx, ly, sx, sy;	// position within room and speed
	bool l, r, u, d;	// left, right, up, down acceleration keys
	bool strafe;
	bool run;
	int gundir;	// gun direction 0-7 (0 = right 1 = right-down 2 = down ...... 7 = right-up

// get rid of (or move elsewhere)
	bool used;
	int id;
	char name[64];
	int ping;
	int frags;
	bool dead;
	char reg_status;
	int score, rank;
	int neg_score;

	virtual ~PlayerBase() { }
	void clear(bool enable, int _pid, const char* _name) {
		ping = 0;
		frags = 0;
		attack = false;
		id = _pid;
		strcpy(name, _name);	//the default name
		weapon = 0;
		item_deathbringer = item_shield = item_quad = item_speed = false;
		item_helm = 0;
		roomx = roomy = 0;
		lx = ly = sx = sy = 0;
		l = r = u = d = strafe = run = false;
		gundir = 0;
		dead = false;
		reg_status = enable ? '-' : ' ';
		score = 0;
		neg_score = 0;
		rank = 0;
		used = enable;
	}
	virtual bool under_deathbringer_effect(double curr_time) const =0;
};

class ServerPlayer : public PlayerBase {
public:
	int health;
	int energy;

	double item_quad_time;
	double item_speed_time;
	double item_helm_time;

	long item_deathbringer_time;	// explosion of this players deathbringer
	double deathbringer_end;	// end of effect of another players deathbringer
	int deathbringer_attacker;	// whose deathbringer it is

	bool awaiting_client_ready;
	bool want_map_exit;

	int mapVote;
	typedef list< pair<int, string> > DMQueueT;
	DMQueueT delayedMessages;	// int is the # of server frames the message has delay after the previous one
	int kickTimer;
	int muted;	// 0 = no, 1 = yes, 2 = silently

	bool want_change_teams;
	double team_change_time;
	bool team_change_pending;

	int cid;	// client id (network identity)
	double waitnametime;
	int oldfrags;	// last value informed to the client
	int megabonus;

	bool dropped_flag;
	double next_shoot_time;
	double respawn_time;
	bool respawn_to_base;

	double talk_temp;
	double talk_hotness;

	//admin shell stats
	int total_kills;
	int total_deaths;
	int most_consecutive_kills;
	int current_consecutive_kills;
	int most_consecutive_deaths;
	int current_consecutive_deaths;
	int total_suicides;
	int total_captures;
	int total_flags_taken;
	int total_flags_dropped;
	int total_flags_returned;
	int total_flag_carriers_killed;
	int total_shots;
	int total_hits;
	int total_shots_taken;
	int last_spawn_time;
	int lifetime;
	double total_movement;
	int start_time;

	bool under_deathbringer_effect(double curr_time) const { return deathbringer_end >= curr_time; }

	//#fix: move these to a message queue type, store in client data, not player data
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
		PlayerBase::clear(enable, _pid, _name);

		oldfrags = -666;
		want_map_exit = false;		//by default don't want change maps
		mapVote=-1;
		delayedMessages.clear();
		kickTimer=0;
		muted=0;
		want_change_teams = false;	// don't want to change teams yet
		team_change_time = 0;
		team_change_pending = false;
		next_shoot_time = 0;
		talk_temp = 0.0;
		talk_hotness = 1.0;
		cid=_cid;
		waitnametime = get_time() - 666.0;	//can change name right now

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

		awaiting_client_ready = false;
		item_deathbringer_time = 0;
		deathbringer_end = 0;
		deathbringer_attacker = 0;
		item_quad_time = item_speed_time = item_helm_time = 0;
		health = energy = 0;
		megabonus = 0;
		dropped_flag = false;
		respawn_time = 0;
		respawn_to_base = false;
	}
};

class ClientPlayer : public PlayerBase {
public:
	bool deathbringer_affected;
	double death_drop_time;
	double speed_drop_time;
	double wall_sound_time;
	bool onscreen;
	NLulong	enemyvis;
	double quad_sound_finished;
	double hitfx;
	int drawptr;
	int drawused;
	bool old_dead;	// to detect time to play death sound
	int oldx, oldy;	// detect room changes

	// get rid of these since they are only known for the local player
	double item_quad_time;
	double item_speed_time;
	double item_helm_time;
	int health;
	int energy;

	bool under_deathbringer_effect(double curr_time) const { (void)curr_time; return deathbringer_affected; }

	void clear(bool enable, int _pid, const char* _name) {
		PlayerBase::clear(enable, _pid, _name);

		item_quad_time = item_speed_time = item_helm_time = 0;
		health = energy = 0;

		speed_drop_time = wall_sound_time = 0;
		onscreen = false;
		enemyvis = 0;
		deathbringer_affected = false;
		death_drop_time = 0;
		quad_sound_finished = hitfx = 0;
		drawptr = drawused = 0;
		old_dead = false;
		oldx = oldy = 0;
	}
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

template<class Type> class PointerContainer {	// doesn't delete the objects!
	Type* ptr;

public:
	PointerContainer() : ptr(0) { }
	PointerContainer(Type* p) : ptr(p) { }
	void setPtr(Type* p) { ptr=p; }
	      Type* getPtr()       { return ptr; }
	const Type* getPtr() const { return ptr; }
	operator       Type&()       { assert(ptr); return *ptr; }
	operator const Type&() const { assert(ptr); return *ptr; }
};

class WorldBase {
protected:
	WorldBase() { }

public:
	bool applyPhysics(int i, const Room& room, float fraction, bool turbo, bool carryFlag, bool deathbringer_affected);

	Map map;

	PointerContainer<PlayerBase> player[MAX_PLAYERS];
	ctflag_t flag[2];
	rocket_c rock[MAX_ROCKETS];
	pickup_c item[MAX_PICKUPS];

	virtual ~WorldBase() { }

	bool load_map(const char *mapdir, const string& mapname) { return map.load(mapdir, mapname); }
	void run_server_player_physics(int pid);
	virtual void returnFlag(int team);
	virtual void dropFlag(int team, int roomx, int roomy, int lx, int ly);
	virtual void stealFlag(int team, int carrier);
};

class gameserver_c;	//#fix: get rid of this callback system

class ServerWorld : public WorldBase {
	gameserver_c* host;

public:
	ServerPlayer player[MAX_PLAYERS];
	ServerWorld(gameserver_c* hostp) : host(hostp) { for (int i=0; i<MAX_PLAYERS; ++i) WorldBase::player[i].setPtr(&player[i]); }

	void returnFlag(int team);
	void dropFlag(int team, int roomx, int roomy, int lx, int ly);
	void stealFlag(int team, int carrier);
	void respawnPlayer(int pid);
};

class ClientWorld : public WorldBase {
public:
	bool skipped;	// frame is invalid -- when frame is skipped in the broadcast
	double frame;
	double time;	// real time (clientside) of the frame

	ClientPlayer player[MAX_PLAYERS];
	ClientWorld() { frame = time = 0; for (int i=0; i<MAX_PLAYERS; ++i) WorldBase::player[i].setPtr(&player[i]); }
};

#endif
