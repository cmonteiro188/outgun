#ifndef WORLD_H_INC
#define WORLD_H_INC

#include "network.h"	//#fix: needed for possible definition of SEND_FRAMEOFFSET
#include "nassert.h"

typedef pair<double, double> Coords;
typedef pair<double, Coords> BounceData;

class RectWall {	// rectangular wall
public:
	RectWall() { }
	RectWall(int a_, int b_, int c_, int d_, int tex_, int alpha_)
			: a(a_), b(b_), c(c_), d(d_), tex(tex_), alpha(alpha_) { if (c<a) swap(a, c); if (d<b) swap(b, d); }
	bool intersects_rect(double x1, double y1, double x2, double y2) const { return x1<=c && x2>=a && y1<=d && y2>=b; }
	int x1() const { return a; }
	int y1() const { return b; }
	int x2() const { return c; }
	int y2() const { return d; }

private:
	int a, b, c, d;	// rectangle coords (a,b)->(c,d)
	int tex;	// texture id
	int alpha;

	friend void tryBounce(BounceData* bd, const RectWall& w, double stx, double sty, double mx, double my, double plyRadius);
};

class TriWall {	// triangular wall
public:
	TriWall() { }
	TriWall(int x1, int y1, int x2, int y2, int x3, int y3, int tex_, int alpha_)
			: p1x(x1), p1y(y1), p2x(x2), p2y(y2), p3x(x3), p3y(y3), tex(tex_), alpha(alpha_) {
		if (p2y < p1y) { swap(p1x, p2x); swap(p1y, p2y); }	// 1, 2 sorted
		if (p3y < p2y) {
			swap(p2x, p3x); swap(p2y, p3y);	// 1, 3 and 2, 3 sorted
			if (p2y < p1y) { swap(p1x, p2x); swap(p1y, p2y); }	// all sorted
		}
		boundx1=min(p1x, min(p2x, p3x)), boundy1=min(p1y, min(p2y, p3y));
		boundx2=max(p1x, max(p2x, p3x)), boundy2=max(p1y, max(p2y, p3y));
	}
	bool intersects_rect(double rx1, double ry1, double rx2, double ry2) const;
	int x1() const { return p1x; }
	int y1() const { return p1y; }
	int x2() const { return p2x; }
	int y2() const { return p2y; }
	int x3() const { return p3x; }
	int y3() const { return p3y; }

private:
	int p1x, p1y, p2x, p2y, p3x, p3y;
	int boundx1, boundy1, boundx2, boundy2;
	int tex, alpha;

	friend void tryBounce(BounceData* bd, const TriWall& w, double stx, double sty, double mx, double my, double plyRadius);
};

class CircWall {	// circular wall
public:
	CircWall() { }
	CircWall(int x_, int y_, int ro_, int ri_, float ang1, float ang2, int tex_, int alpha_);

	bool intersects_rect(double x1, double y1, double x2, double y2) const;

	int X() const { return x; }
	int Y() const { return y; }
	int radius() const { return ro; }
	int radius_in() const { return ri; }
	const float* angles() const { return angle; }
	const Coords& angle_vector_1() const { return va1; }
	const Coords& angle_vector_2() const { return va2; }
	int texture() const { return tex; }

private:
	int x, y, ro, ri;
	float angle[2];
	Coords va1, va2, midvec;
	float anglecos;
	int tex, alpha;

	friend void tryBounce(BounceData* bd, const CircWall& w, double stx, double sty, double mx, double my, double plyRadius);
};

struct Room {
	vector<RectWall> rwalls, rground;	// ground: optional list of textures for ground [not used]
	vector<TriWall>  twalls, tground;
	vector<CircWall> cwalls, cground;

	bool fall_on_wall(int x1, int y1, int x2, int y2) const {	// note: this is only a bounding-box check - no accurate checks possible for circular walls yet
		for (vector<RectWall>::const_iterator rwi=rwalls.begin(); rwi!=rwalls.end(); ++rwi)
			if (rwi->intersects_rect(x1, y1, x2, y2))
				return true;
		for (vector<TriWall>::const_iterator twi=twalls.begin(); twi!=twalls.end(); ++twi)
			if (twi->intersects_rect(x1, y1, x2, y2))
				return true;
		for (vector<CircWall>::const_iterator cwi=cwalls.begin(); cwi!=cwalls.end(); ++cwi)
			if (cwi->intersects_rect(x1, y1, x2, y2))
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
	bool parse_label(FILE *f, const char *label, int crx = 0, int cry = 0, float scalex = 1, float scaley = 1);	// crx,cry = "current room pointer"

public:
	bool valid_for_scoring;	//v0.4.7: map is valid for scoring?
	teaminfo_t tinfo[2];	//team information for red=0 and blue=1 teams
	vector< vector<Room> > room;	// accessed by [x][y]

	string title;	//map title
	string author;
	int	ver;	// map version
	int w, h;	// width height
	NLushort crc;	//map's 16bit CRC

	Map() : valid_for_scoring(true), ver(-1), w(0), h(0), crc(0) { }

	bool fall_on_wall(int px, int py, int x1, int y1, int x2, int y2) const {
if (px<0 || py<0 || px>=w || py>=h) return false;	//#fix: remove this and track why these are given sometimes
		nAssert(px>=0 && py>=0 && px<w && py<h);
		return room[px][py].fall_on_wall(x1, y1, x2, y2);
	}
	bool load(const char *mapdir, const string& mapname);
};

class PlayerBase {
protected:
	PlayerBase() { }

public:
	bool item_deathbringer;
	bool item_shield;
	bool item_quad;
	bool item_speed;
	int item_helm;	// 0 == no   1+ == yes, alpha

	int roomx, roomy;
	double lx, ly, sx, sy;	// position within room and speed
	ClientControls controls;
	int gundir;	// gun direction 0-7 (0 = right 1 = right-down 2 = down ...... 7 = right-up

// get rid of (or move elsewhere)
	bool used;
	int id;
	string name;
	int ping;
	int frags;
	bool dead;
	char reg_status;
	int score, rank;
	int neg_score;

	virtual ~PlayerBase() { }
	void move(double fraction) { lx += sx*fraction; ly += sy*fraction; }
	void clear(bool enable, int _pid, const string& _name) {
		ping = 0;
		frags = 0;
		id = _pid;
		name = _name;
		item_deathbringer = item_shield = item_quad = item_speed = false;
		item_helm = 0;
		roomx = roomy = 0;
		lx = ly = sx = sy = 0;
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

	int weapon;
	bool attack;	// if player is holding attack button

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
	NLubyte lastClientFrame;	// client set frame identifier of the latest data received
	#ifdef SEND_FRAMEOFFSET
	float frameOffset;	// at what time within the frame the client's packet arrived
	#endif
	double waitnametime;
	int oldfrags;	// last value informed to clients
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

	void clear(bool enable, int _pid, int _cid, const string& _name) {
		PlayerBase::clear(enable, _pid, _name);

		attack = false;
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

		lastClientFrame = 0;
		#ifdef SEND_FRAMEOFFSET
		frameOffset = 0;
		#endif
		awaiting_client_ready = false;
		item_deathbringer_time = 0;
		deathbringer_end = 0;
		deathbringer_attacker = 0;
		item_quad_time = item_speed_time = item_helm_time = 0;
		health = energy = 0;
		megabonus = 0;
		weapon = 0;
		dropped_flag = false;
		respawn_time = 0;
		respawn_to_base = false;
	}
};

class PlayerQueueAdder : public LineReceiver {
	ServerPlayer& ply;

public:
	PlayerQueueAdder(ServerPlayer& player) : ply(player) { }
	PlayerQueueAdder& operator()(const string& str) { ply.add_to_queue(str); return *this; }
};

class ClientPlayer : public PlayerBase {
public:
	bool deathbringer_affected;
	double death_drop_time;
	double speed_drop_time;
	double wall_sound_time;
	bool onscreen;
	NLulong	enemyvis;
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
	int weapon;

	bool under_deathbringer_effect(double curr_time) const { (void)curr_time; return deathbringer_affected; }

	void clear(bool enable, int _pid, const string& _name) {
		PlayerBase::clear(enable, _pid, _name);

		item_quad_time = item_speed_time = item_helm_time = 0;
		health = energy = 0;
		weapon = 0;

		speed_drop_time = wall_sound_time = 0;
		onscreen = false;
		enemyvis = 0;
		deathbringer_affected = false;
		death_drop_time = 0;
		hitfx = 0;
		drawptr = drawused = 0;
		old_dead = false;
		oldx = oldy = 0;
	}
};

// a rocket-shot
class rocket_c {
public:
	int	owner;	//owning player-id (-1 == unused)
	int team;
	bool power;

	NLulong vislist;	//notification list (bitfield, bit0=player0, bit1=player1... etc.)
	int px, py;			//screen coords
	double x, y;		//start position or current position
	double sx, sy;		//speed
	NLulong time;		//time of shot or current time

	rocket_c() { owner = -1; }
	void move(double fraction) { x += sx*fraction; y += sy*fraction; }
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
	operator       Type&()       { nAssert(ptr); return *ptr; }
	operator const Type&() const { nAssert(ptr); return *ptr; }
};

class PhysicsCallbacksBase {
public:
	virtual bool collideToRockets() const =0;	// should player to rocket collisions be checked at all
	virtual bool gatherMovementDistance() const =0;	// should addMovementDistance be called with player movements
	virtual bool allowRoomChange() const =0;
	virtual void addMovementDistance(int pid, float dist) =0;	// player pid has moved the distance dist
	virtual void playerScreenChange(int pid) =0;	// player pid has moved to a new room (called max. once per frame per player)
	virtual void rocketHitWall(int rid, bool power, float x, float y, int roomx, int roomy) =0;	// caller doesn't remove the rocket
	virtual bool rocketHitPlayer(int rid, int pid) =0;	// returns true if player dies (to be removed from further simulation)
	virtual void playerHitWall(int pid) =0;
	virtual void rocketOutOfBounds(int rid) =0;	// caller doesn't remove the rocket
	virtual bool shouldApplyPhysicsToPlayer(int pid) =0;	// returns true physics should be run to player pid
};

class WorldBase {
	void addRocket(int i, int playernum, int team, int px, int py, int x, int y,
													bool power, int dir, int xdelta, int frameAdvance, PhysicsCallbacksBase& cb);

	static BounceData genGetTimeTillWall(const Room& room, double x, double y, double mx, double my, double radius, float maxFraction);
	static BounceData getTimeTillBounce(const Room& room, const PlayerBase& pl, double plyRadius, float maxFraction);
	static double getTimeTillWall(const Room& room, const rocket_c& rock, float maxFraction);
	static double getTimeTillCollision(const PlayerBase& pl, const rocket_c& rock, double collRadius);
	void applyPlayerAcceleration(int pid);
	void executeBounce(PlayerBase& ply, const BounceData& b, double plyRadius);	// needs plyRadius as a shortcut to b.second's length
	void applyPhysicsToRoom(const Room& room, vector<int>& rply, vector<int>& rrock, PhysicsCallbacksBase& callback, double plyRadius, float fraction);

protected:
	WorldBase() { }

public:
	void applyPhysics(PhysicsCallbacksBase& callback, double plyRadius, float fraction);
	void rocketFrameAdvance(int frames, PhysicsCallbacksBase& callback);

	Map map;

	PointerContainer<PlayerBase> player[MAX_PLAYERS];
	ctflag_t flag[2];
	rocket_c rock[MAX_ROCKETS];
	pickup_c item[MAX_PICKUPS];

	virtual ~WorldBase() { }

	void shootRockets(PhysicsCallbacksBase& cb, int playernum, int pow, int dir, NLubyte* rids,
										int frameAdvance, int team, bool power, int px, int py, int x, int y);

	void run_server_player_physics(int pid);
	virtual bool load_map(const char *mapdir, const string& mapname) { return map.load(mapdir, mapname); }
	virtual void returnFlag(int team);
	virtual void dropFlag(int team, int roomx, int roomy, int lx, int ly);
	virtual void stealFlag(int team, int carrier);
};

class PowerupSettings {
	int pups_by_percent(int percentage, const Map& map) const;

public:
	int pups_min, pups_max, pups_respawn_time, pup_chance_shield, pup_chance_turbo, pup_chance_shadow,
			pup_chance_power, pup_chance_weapon, pup_chance_megahealth, pup_chance_deathbringer;
	bool pups_min_percentage, pups_max_percentage;
	int pup_add_time, pup_max_time;
	bool pup_deathbringer_switch;

	void reset();
	void print(LineReceiver& printer) const;

	int choose_powerup_kind() const;
	int getMinPups(const Map& map) const { return pups_min_percentage?pups_by_percent(pups_min, map):pups_min; }
	int getMaxPups(const Map& map) const { return pups_max_percentage?pups_by_percent(pups_max, map):pups_max; }
	int getRespawnTime() const { return pups_respawn_time; }
	bool getDeathbringerSwitch() const { return pup_deathbringer_switch; }
	double addTime(double t) const { t += pup_add_time; if (t > pup_max_time) t = pup_max_time; return t; }
};

class WorldSettings {
public:
	double respawn_time, waiting_time_deathbringer;
	int shadow_minimum;	// smallest alpha value allowed; 1 is when even the coordinates are not sent
	NLulong time_limit;
	int capture_limit;

	void reset();
	void print(LineReceiver& printer) const;

	double getRespawnTime() const { return respawn_time; }
	double getDeathbringerWaitingTime() const { return waiting_time_deathbringer; }
	int getShadowMinimum() const { return shadow_minimum; }
	int getCaptureLimit() const { return capture_limit; }
	NLulong getTimeLimit() const { return time_limit; }
};

class ServerNetworking;
class gameserver_c;	//#fix: get rid of non-networking callbacks?

class ServerWorld : public WorldBase {
	gameserver_c* host;
	ServerNetworking* net;
	PowerupSettings pupConfig;
	WorldSettings config;

	NLubyte getFreeRocket();	// may give an existing rocket to overwrite if the table is full

public:
	NLulong frame;
	NLulong map_start_time;	// frame #
	ServerPlayer player[MAX_PLAYERS];

	ServerWorld(gameserver_c* hostp, ServerNetworking* netp) : host(hostp), net(netp), frame(0), map_start_time(0) { for (int i=0; i<MAX_PLAYERS; ++i) WorldBase::player[i].setPtr(&player[i]); }

	void setConfig(const WorldSettings& ws, const PowerupSettings& ps) { config = ws; pupConfig = ps; }

	// common (virtual in base) extended functions
	bool load_map(const char *mapdir, const string& mapname) { map_start_time = frame; return WorldBase::load_map(mapdir, mapname); }
	void returnFlag(int team);
	void dropFlag(int team, int roomx, int roomy, int lx, int ly);
	void stealFlag(int team, int carrier);
	int getMapTime() const { return frame - map_start_time; }
	bool isTimeLimit() const { return config.getTimeLimit() > 0; }
	int getTimeLeft() const { return config.getTimeLimit() - getMapTime(); }

	// server specific functions
	void reset();
	void respawnPlayer(int pid);
	void printTimeStatus(LineReceiver& printer);

	void resetPlayer(int target, float time_penalty = 0.);
	void killPlayer(int target, bool time_penalty);
	void damagePlayer(int target, int attacker, int damage, bool deathbringer);
	void removePlayer(int pid);
	void suicide(int pid);
	void respawn_pickup(int p);
	void check_pickup_creation(bool instant);
	void game_touch_pickup(int p, int pk);
	bool check_flag_touch(int px, int py, int x, int y, int t);
	void game_player_screen_change(int p);

	bool dropFlagIfAny(int pid);
	void shootRockets(int pid, int numshots);
	void deleteRocket(int r, NLshort hitx, NLshort hity, int targ);
	void changeRocketsOwner(int source, int target);
	void swapRocketOwners(int a, int b);

	void simulateFrame();

	void addMovementDistanceCallback(int pid, float dist);
	void playerScreenChangeCallback(int pid);
	void rocketHitWallCallback(int rid);
	bool rocketHitPlayerCallback(int rid, int pid);
	void rocketOutOfBoundsCallback(int rid);
	bool shouldApplyPhysicsToPlayerCallback(int pid);
};

class ClientWorld : public WorldBase {
public:
	bool skipped;	// frame is invalid -- when frame is skipped in the broadcast
	double frame;

	ClientPlayer player[MAX_PLAYERS];
	ClientWorld() : skipped(true) { for (int i=0; i<MAX_PLAYERS; ++i) WorldBase::player[i].setPtr(&player[i]); }
	// extrapolate : advances from source, a frame per every ctrl listed except the last one which gets subFrameAfter, controls are for player me
	void ClientWorld::extrapolate(ClientWorld& source, PhysicsCallbacksBase& physCallbacks, int me,
						ClientControls* ctrlTab, NLubyte ctrlFirst, NLubyte ctrlLast, float subFrameAfter);
};

#endif
