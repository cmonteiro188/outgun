#ifndef WORLD_H_INC
#define WORLD_H_INC

#include <vector>
#include <list>
#include <string>
#include <algorithm>
#include "commont.h"
#include "nassert.h"
#include "protocol.h"	// needed for possible definition of SEND_FRAMEOFFSET
#include "utility.h"

typedef std::pair<double, double> Coords;
typedef std::pair<double, Coords> BounceData;

class RectWall {	// rectangular wall
public:
	RectWall() { }
	RectWall(float a_, float b_, float c_, float d_, int tex_, int alpha_)
			: a(a_), b(b_), c(c_), d(d_), tex(tex_), alpha(alpha_) { if (c<a) std::swap(a, c); if (d<b) std::swap(b, d); }

	bool intersects_rect(double x1, double y1, double x2, double y2) const { return x1<=c && x2>=a && y1<=d && y2>=b; }	// perfect
	bool intersects_circ(double x, double y, double r) const;	// perfect

	float x1() const { return a; }
	float y1() const { return b; }
	float x2() const { return c; }
	float y2() const { return d; }
	int texture() const { return tex; }

private:
	float a, b, c, d;	// rectangle coords (a,b)->(c,d)
	int tex;	// texture id
	int alpha;

	friend void tryBounce(BounceData* bd, const RectWall& w, double stx, double sty, double mx, double my, double plyRadius);
};

class TriWall {	// triangular wall
public:
	TriWall() { }
	TriWall(float x1, float y1, float x2, float y2, float x3, float y3, int tex_, int alpha_);

	bool intersects_rect(double rx1, double ry1, double rx2, double ry2) const;	// perfect
	bool intersects_circ(double x, double y, double r) const;	// very much imperfect (uses bounding rectangle)

	float x1() const { return p1x; }
	float y1() const { return p1y; }
	float x2() const { return p2x; }
	float y2() const { return p2y; }
	float x3() const { return p3x; }
	float y3() const { return p3y; }
	int texture() const { return tex; }

private:
	float p1x, p1y, p2x, p2y, p3x, p3y;
	float boundx1, boundy1, boundx2, boundy2;
	int tex, alpha;

	friend void tryBounce(BounceData* bd, const TriWall& w, double stx, double sty, double mx, double my, double plyRadius);
};

class CircWall {	// circular wall
public:
	CircWall() { }
	CircWall(float x_, float y_, float ro_, float ri_, float ang1, float ang2, int tex_, int alpha_);

	bool intersects_rect(double x1, double y1, double x2, double y2) const;	// very much imperfect (uses bounding circle)
	bool intersects_circ(double rcx, double rcy, double rr) const;	// imperfect

	float X() const { return x; }
	float Y() const { return y; }
	float radius() const { return ro; }
	float radius_in() const { return ri; }
	const float* angles() const { return angle; }
	const Coords& angle_vector_1() const { return va1; }
	const Coords& angle_vector_2() const { return va2; }
	int texture() const { return tex; }

private:
	float x, y, ro, ri;
	float angle[2];
	Coords va1, va2, midvec;
	float anglecos;
	int tex, alpha;

	friend void tryBounce(BounceData* bd, const CircWall& w, double stx, double sty, double mx, double my, double plyRadius);
};

struct Room {
	std::vector<RectWall> rwalls, rground;	// ground: optional list of textures for ground
	std::vector<TriWall>  twalls, tground;
	std::vector<CircWall> cwalls, cground;

	bool fall_on_wall(int x1, int y1, int x2, int y2) const;	// this check follows the quality of *Wall::intersects_rect and isn't perfect
	bool fall_on_wall(int x, int y, int r) const;	// this check follows the quality of *Wall::intersects_circ and isn't perfect
};

//entity locale
struct WorldCoords {
	WorldCoords(int px_, int py_, int x_, int y_): px(px_), py(py_), x(x_), y(y_) { }
	WorldCoords() { }
	int px, py;	//screen (if px == -1, unused)
	int x, y;	//relative (to screen) X,Y position
};

//team info
struct MapTeam {
	std::vector<WorldCoords> flags;	//flag positions
	std::vector<WorldCoords> spawn;	//team spawn points
	unsigned int lastspawn;			//last team spawn point used

	MapTeam() : lastspawn(0) { }
};

class Map {
	bool parse_file(LogSet& log, std::istream& in);
	bool parse_line(LogSet& log, const std::string& line, const std::vector<std::pair<std::string, std::vector<std::string> > >& label_lines,
					int& crx, int& cry, float& scalex, float& scaley, bool label_block = false);

public:
	MapTeam tinfo[2];	//team information for red=0 and blue=1 teams
	std::vector<WorldCoords> wild_flags;
	std::vector< std::vector<Room> > room;	// accessed by [x][y]

	std::string title;	//map title
	std::string author;
	int	ver;	// map version
	int w, h;	// width height
	NLushort crc;	//map's 16bit CRC

	Map() : ver(-1), w(0), h(0), crc(0) { }

	bool fall_on_wall(int px, int py, int x1, int y1, int x2, int y2) const {
if (px<0 || py<0 || px>=w || py>=h) return false;	//#fix: remove this and track why these are given sometimes
		nAssert(px>=0 && py>=0 && px<w && py<h);
		return room[px][py].fall_on_wall(x1, y1, x2, y2);
	}
	bool fall_on_wall(int px, int py, int x, int y, int r) const {
if (px<0 || py<0 || px>=w || py>=h) return false;	//#fix: remove this and track why these are given sometimes
		nAssert(px>=0 && py>=0 && px<w && py<h);
		return room[px][py].fall_on_wall(x, y, r);
	}
	bool load(LogSet& log, const char *mapdir, const std::string& mapname);
};

class MapInfo {
public:
	std::string title, author, file;
	int width, height;
	int votes;
	bool votes_changed;

	MapInfo();
	bool load(LogSet& log, const std::string& mapName);
};

class Statistics {
public:
	Statistics();

	void clear();

	void set_frags(int n) { total_frags = n; }
	void set_kills(int n) { total_kills = n; }
	void set_deaths(int n) { total_deaths = n; }
	void set_cons_kills(int n) { most_consecutive_kills = n; }
	void set_current_cons_kills(int n) { current_consecutive_kills = n; }
	void set_cons_deaths(int n) { most_consecutive_deaths = n; }
	void set_current_cons_deaths(int n) { current_consecutive_deaths = n; }
	void set_suicides(int n) { total_suicides = n; }
	void set_captures(int n) { total_captures = n; }
	void set_flags_taken(int n) { total_flags_taken = n; }
	void set_flags_dropped(int n) { total_flags_dropped = n; }
	void set_flags_returned(int n) { total_flags_returned = n; }
	void set_carriers_killed(int n) { total_flag_carriers_killed = n; }
	void set_shots(int n) { total_shots = n; }
	void set_hits(int n) { total_hits = n; }
	void set_shots_taken(int n) { total_shots_taken = n; }
	void set_movement(double amount) { total_movement = amount; }
	void set_spawn_time(float time) { last_spawn_time = time; }
	void set_start_time(float time) { starttime = time; }
	void set_lifetime(float time) { total_lifetime = time; }
	void set_flag_carrying_time(double time) { total_flag_carrying_time = time; }
	void set_flag_take_time(double time) { flag_taking_time = time; }
	void set_flag(bool f) { flag = f; }

	void spawn(float time) { set_spawn_time(time); dead = false; }

	void add_frag(int n = 1) { total_frags += n; }
	void add_kill(bool deathbringer);
	void add_death(bool deathbringer, double time);
	void add_suicide(double time);
	void add_capture(double time);
	void add_flag_take(double time);
	void add_flag_drop(double time);
	void add_flag_return() { ++total_flags_returned; }
	void add_carrier_kill() { ++total_flag_carriers_killed; }
	void add_shot() { ++total_shots; }
	void add_hit() { ++total_hits; }
	void add_shot_take() { ++total_shots_taken; }
	void add_movement(double amount) { total_movement += amount; }

	void finish_stats(double time);

	void save_speed(double time) { saved_speed = speed(time); }

	int frags() const { return total_frags; }
	int kills() const { return total_kills; }
	int deaths() const { return total_deaths; }
	int cons_kills() const { return most_consecutive_kills; }
	int current_cons_kills() const { return current_consecutive_kills; }
	int cons_deaths() const { return most_consecutive_deaths; }
	int current_cons_deaths() const { return current_consecutive_deaths; }
	int suicides() const { return total_suicides; }
	int captures() const { return total_captures; }
	int flags_taken() const { return total_flags_taken; }
	int flags_dropped() const { return total_flags_dropped; }
	int flags_returned() const { return total_flags_returned; }
	int carriers_killed() const { return total_flag_carriers_killed; }
	int shots() const { return total_shots; }
	int hits() const { return total_hits; }
	float accuracy() const;
	int shots_taken() const { return total_shots_taken; }
	float spawn_time() const { return last_spawn_time; }
	float lifetime(double time) const;			// in seconds
	float average_lifetime(double time) const;	// in seconds
	float playtime(double time) const;			// in seconds
	double movement() const;					// in Outgun units
	float speed(double time) const;				// in Outgun units per second
	float old_speed() const { return saved_speed; }
	float start_time() const { return starttime; }
	double flag_carrying_time(double time) const;
	double flag_take_time() const { return flag_taking_time; }

private:
	int total_frags;
	int total_kills;
	int total_deaths;
	int total_deathbringer_kills;
	int total_deathbringer_deaths;
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
	float last_spawn_time;
	float total_lifetime;
	double total_movement;
	float saved_speed;
	float starttime;
	bool dead;
	bool flag;
	double total_flag_carrying_time;
	double flag_taking_time;
};

class PlayerBase {
protected:
	PlayerBase() { }

	int team_nr;
	int personal_color;
	Statistics player_stats;

public:
	bool item_deathbringer;
	bool item_shield;
	bool item_quad;
	bool item_speed;
	int visibility;		// alpha

	int roomx, roomy;
	double lx, ly, sx, sy;	// position within room and speed
	ClientControls controls;
	int gundir;	// gun direction 0-7 (0 = right 1 = right-down 2 = down ...... 7 = right-up

// get rid of (or move elsewhere)
	bool used;
	int id;	// as in pid
	std::string name;
	int ping;
	//int frags;
	bool dead;
	ClientLoginStatus reg_status;
	int score, rank;
	int neg_score;

	virtual ~PlayerBase() { }
	void move(double fraction) { lx += sx*fraction; ly += sy*fraction; }
	void clear(bool enable, int _pid, const std::string& _name, int team_id);

	void set_team(int t) { team_nr = t; }
	void set_color(int c) { personal_color = c; }

	const Statistics& stats() const { return player_stats; }
	Statistics& stats() { return player_stats; }

	bool item_helm() const { return visibility < 255; }
	int team() const { return team_nr; }
	int color() const { return personal_color; }
	virtual bool under_deathbringer_effect(double curr_time) const =0;
};

bool compare_players(const PlayerBase* a, const PlayerBase* b);

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

	size_t current_map_list_item;

	int mapVote;
	int idleFrames;
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
	bool localIP;
	int oldfrags;	// last value informed to clients
	int megabonus;

	bool drop_key;
	bool dropped_flag;
	double next_shoot_time;
	double respawn_time;
	bool respawn_to_base;

	double talk_temp;
	double talk_hotness;

	bool under_deathbringer_effect(double curr_time) const { return deathbringer_end >= curr_time; }

	void clear(bool enable, int _pid, int _cid, const std::string& _name, int team_id);

	void set_fav_colors(const std::vector<char>& colors) { fav_col = colors; }
	const std::vector<char>& fav_colors() const { return fav_col; }

	void take_flag() { carrying_flag = true; }
	void drop_flag() { carrying_flag = false; }
	bool flag() const { return carrying_flag; }

private:
	bool carrying_flag;
	std::vector<char> fav_col;
};

class ClientPlayer : public PlayerBase {
public:
	bool deathbringer_affected;
	double death_drop_time;
	double speed_drop_time;
	double wall_sound_time;
	double player_sound_time;
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

	void clear(bool enable, int _pid, const std::string& _name, int team_id);
};

// a rocket-shot
class Rocket {
public:
	int	owner;	//owning player-id (-1 == unused)
	int team;
	bool power;

	NLulong vislist;	//notification list (bitfield, bit0=player0, bit1=player1... etc.)
	int px, py;			//screen coords
	double x, y;		//start position or current position
	double sx, sy;		//speed
	int direction;
	NLulong time;		//time of shot or current time

	Rocket() { owner = -1; }
	void move(double fraction) { x += sx*fraction; y += sy*fraction; }
};

class Flag {
public:
	Flag(const WorldCoords& pos_);

	void take(int carr);
	void take(int carr, double time);
	void return_to_base();
	void drop();
	void move(const WorldCoords& new_pos) { pos = new_pos; }

	bool carried() const { return status == status_carried; }
	bool at_base() const { return status == status_at_base; }

	int carrier() const { return carrier_id; }
	double grab_time() const { return grab_t; }

	const WorldCoords& position() const { return pos; }
	const WorldCoords& home_position() const { return home_pos; }

private:
	enum Status { status_at_base, status_carried, status_dropped };

	Status status;
	int carrier_id;
	double grab_t;
	WorldCoords home_pos;
	WorldCoords pos;
};

class Team {
public:
	Team();

	void clear_stats();

	void set_score(int n) { points = n; }
 	void set_kills(int n) { total_kills = n; }
	void set_deaths(int n) { total_deaths = n; }
	void set_suicides(int n) { total_suicides = n; }
	void set_flags_taken(int n) { total_flags_taken = n; }
	void set_flags_dropped(int n) { total_flags_dropped = n; }
	void set_flags_returned(int n) { total_flags_returned = n; }
	void set_shots(int n) { total_shots = n; }
	void set_hits(int n) { total_hits = n; }
	void set_shots_taken(int n) { total_shots_taken = n; }
	void set_base_score(int n) { start_score = n; }
	void set_movement(double amount) { total_movement = amount; }
	void set_power(float pow) { tournament_power = pow; }

	void add_score(double time, const std::string& player);
	void add_kill() { ++total_kills; }
	void add_death() { ++total_deaths; }
	void add_suicide() { ++total_suicides; ++total_deaths; }
	void add_flag_take() { ++total_flags_taken; }
	void add_flag_drop() { ++total_flags_dropped; }
	void add_flag_return() { ++total_flags_returned; }
	void add_shot() { ++total_shots; }
	void add_hit() { ++total_hits; }
	void add_shot_take() { ++total_shots_taken; }
	void add_movement(double amount) { total_movement += amount; }

	void add_flag(const WorldCoords& pos);
	void remove_flags();

	void steal_flag(int n, int carrier);
	void steal_flag(int n, int carrier, double time);

	void return_all_flags();
	void return_flag(int n);
	void drop_flag(int n, const WorldCoords& pos);
	void move_flag(int n, const WorldCoords& pos);

	int score() const { return points; }
	int kills() const { return total_kills; }
	int deaths() const { return total_deaths; }
	int suicides() const { return total_suicides; }
	int flags_taken() const { return total_flags_taken; }
	int flags_dropped() const { return total_flags_dropped; }
	int flags_returned() const { return total_flags_returned; }
	int shots() const { return total_shots; }
	int hits() const { return total_hits; }
	int shots_taken() const { return total_shots_taken; }
	double movement() const { return total_movement; }
	float accuracy() const;
	float power() const { return tournament_power; }

	const Flag& flag(int n) const { return team_flags[n]; }
	const std::vector<Flag>& flags() const { return team_flags; }

	const std::vector<std::pair<int, std::string> >& captures() const { return caps; }
	int base_score() const { return start_score; }

private:
	int points;
	int total_kills;
	int total_deaths;
	int total_suicides;
	int total_flags_taken;
	int total_flags_dropped;
	int total_flags_returned;
	int total_shots;
	int total_hits;
	int total_shots_taken;
	double total_movement;
	float tournament_power;
	std::vector<Flag> team_flags;
	std::vector<std::pair<int, std::string> > caps;	// time and player name
	int start_score;	// for players who join in the middle of the game
};

class Powerup {
public:
	enum Pup_type {
		pup_shield,
		pup_turbo,
		pup_shadow,
		pup_power,
		pup_weapon,
		pup_health,
		pup_deathbringer,
		pup_last_real = pup_deathbringer,
		pup_unused,
		pup_respawning
	};

	Pup_type kind;	// type of powerup

	double respawn_time;		// time to respawn

	int px;	//screen
	int py;
	int x;	//position
	int y;

	Powerup(): kind(pup_unused) { }
};

template<class Type> class PointerContainer {	// doesn't delete the objects!
	Type* ptr;

public:
	PointerContainer() : ptr(0) { }
	PointerContainer(Type* p) : ptr(p) { }

	void setPtr(Type* p) { ptr = p; }

	      Type* getPtr()       { return ptr; }
	const Type* getPtr() const { return ptr; }

	operator       Type&()       { nAssert(ptr); return *ptr; }
	operator const Type&() const { nAssert(ptr); return *ptr; }
};

class PhysicalSettings {
public:
	float fric, drag, accel;
	float run_mul, turbo_mul, flag_mul;
	bool friendly_fire, friendly_db;
	bool player_collisions;

	float max_run_speed;	// max speed without turbo, for turbo effect in client

	PhysicalSettings();
	void calc_max_run_speed();
	void read(char* lebuf, int& count);
	void write(char* lebuf, int& count) const;
	void print(LineReceiver& printer) const;
};

class PhysicsCallbacksBase {
public:
	virtual ~PhysicsCallbacksBase() { }
	virtual bool collideToRockets() const =0;	// should player to rocket collisions be checked at all
	virtual bool gatherMovementDistance() const =0;	// should addMovementDistance be called with player movements
	virtual bool allowRoomChange() const =0;
	virtual void addMovementDistance(int pid, float dist) =0;	// player pid has moved the distance dist
	virtual void playerScreenChange(int pid) =0;	// player pid has moved to a new room (called max. once per frame per player)
	virtual void rocketHitWall(int rid, bool power, float x, float y, int roomx, int roomy) =0;	// caller doesn't remove the rocket
	virtual bool rocketHitPlayer(int rid, int pid) =0;	// returns true if player dies (to be removed from further simulation)
	virtual void playerHitWall(int pid) =0;
	virtual void playerHitPlayer(int pid1, int pid2) =0;
	virtual void rocketOutOfBounds(int rid) =0;	// caller doesn't remove the rocket
	virtual bool shouldApplyPhysicsToPlayer(int pid) =0;	// returns true physics should be run to player pid
};

class WorldBase {
	void addRocket(int i, int playernum, int team, int px, int py, int x, int y,
													bool power, int dir, int xdelta, int frameAdvance, PhysicsCallbacksBase& cb);

	static BounceData genGetTimeTillWall(const Room& room, double x, double y, double mx, double my, double radius, float maxFraction);
	static BounceData getTimeTillBounce(const Room& room, const PlayerBase& pl, double plyRadius, float maxFraction);
	static double getTimeTillWall(const Room& room, const Rocket& rock, float maxFraction);
	static double getTimeTillCollision(const PlayerBase& pl, const Rocket& rock, double collRadius);
	static double getTimeTillCollision(const PlayerBase& pl1, const PlayerBase& pl2, double collRadius);
	void applyPlayerAcceleration(int pid);
	void executeBounce(PlayerBase& ply, const Coords& bounceVec, double plyRadius);	// needs plyRadius as a shortcut to bounceVec's length
	void executeBounce(PlayerBase& pl1, PlayerBase& pl2) const;
	void applyPhysicsToRoom(const Room& room, std::vector<int>& rply, std::vector<int>& rrock, PhysicsCallbacksBase& callback, double plyRadius, float fraction);

 	void print_team_stats_row(std::ostream& out, const std::string& header, int amount1, int amount2, const std::string& postfix = "") const;

protected:
	WorldBase(): player(MAX_PLAYERS) { }

public:
	void applyPhysics(PhysicsCallbacksBase& callback, double plyRadius, float fraction);
	void rocketFrameAdvance(int frames, PhysicsCallbacksBase& callback);

	void setMaxPlayers(int num) { maxplayers = num; }

	Map map;

	int maxplayers;	// actual
	std::vector<PointerContainer<PlayerBase> > player;
	Team teams[2];

	std::vector<Flag> wild_flags;	// both teams can capture these (team ID is 2)

	Rocket rock[MAX_ROCKETS];
	Powerup item[MAX_PICKUPS];

	PhysicalSettings physics;

	virtual ~WorldBase() { }

	void shootRockets(PhysicsCallbacksBase& cb, int playernum, int pow, int dir, NLubyte* rids,
										int frameAdvance, int team, bool power, int px, int py, int x, int y);

	void run_server_player_physics(int pid);
	virtual bool load_map(LogSet& log, const char *mapdir, const std::string& mapname) { return map.load(log, mapdir, mapname); }
	virtual void returnAllFlags();
	virtual void returnFlag(int team, int flag);
	virtual void dropFlag(int team, int flag, int roomx, int roomy, int lx, int ly);
	virtual void stealFlag(int team, int flag, int carrier);

	static std::string getTeamName(int team);

	void save_stats(const std::string& dir, const std::string& map_name) const;
};

class PowerupSettings {
	int pups_by_percent(int percentage, const Map& map) const;

public:
	int pups_min, pups_max, pups_respawn_time, pup_chance_shield, pup_chance_turbo, pup_chance_shadow,
		pup_chance_power, pup_chance_weapon, pup_chance_megahealth, pup_chance_deathbringer;
	bool pups_min_percentage, pups_max_percentage;
	int pup_add_time, pup_max_time;
	bool pup_deathbringer_switch;
	float pup_deathbringer_time;
	bool pups_drop_at_death;
	int pup_health_bonus;
	float pup_power_damage;
	int pup_weapon_max;

	void reset();
	void print(LineReceiver& printer) const;

	Powerup::Pup_type choose_powerup_kind() const;
	int getMinPups(const Map& map) const { return pups_min_percentage?pups_by_percent(pups_min, map):pups_min; }
	int getMaxPups(const Map& map) const { return pups_max_percentage?pups_by_percent(pups_max, map):pups_max; }
	int getRespawnTime() const { return pups_respawn_time; }
	bool getDeathbringerSwitch() const { return pup_deathbringer_switch; }
	double addTime(double t) const { t += pup_add_time; if (t > pup_max_time) t = pup_max_time; return t; }
};

class WorldSettings {
public:
	enum Team_balance { TB_disabled = 0, TB_balance, TB_balance_and_shuffle };

	double respawn_time, waiting_time_deathbringer;
	int shadow_minimum;	// smallest alpha value allowed; 0 is when even the coordinates are not sent
	int rocket_damage;
	NLulong time_limit;
	NLulong extra_time;
	bool sudden_death;
	int capture_limit;
	Team_balance balance_teams;

	static const int shadow_minimum_normal;

	void reset();
	void print(LineReceiver& printer) const;

	double getRespawnTime() const { return respawn_time; }
	double getDeathbringerWaitingTime() const { return waiting_time_deathbringer; }
	int getShadowMinimum() const { return shadow_minimum; }
	int getCaptureLimit() const { return capture_limit; }
	NLulong getTimeLimit() const { return time_limit; }
	NLulong getExtraTime() const { return extra_time; }

	Team_balance balanceTeams() const { return balance_teams; }
	bool suddenDeath() const { return sudden_death; }
};

class ServerNetworking;
class Server;	//#fix: get rid of non-networking callbacks?

class ServerWorld : public WorldBase {
	Server* host;
	ServerNetworking* net;
	PowerupSettings pupConfig;
	WorldSettings config;
	LogSet log;

	NLubyte getFreeRocket();	// may give an existing rocket to overwrite if the table is full
	void drop_pickup(const ServerPlayer& player);

	void player_steals_flag(int pid, int team, int flag);
	void player_captures_flag(int pid, int team, int flag);

public:
	NLulong frame;
	NLulong map_start_time;	// frame #
	ServerPlayer player[MAX_PLAYERS];

	ServerWorld(Server* hostp, ServerNetworking* netp, LogSet logset) :
					host(hostp), net(netp), log(logset), frame(0), map_start_time(0) {
		for (int i = 0; i < MAX_PLAYERS; ++i)
			WorldBase::player[i].setPtr(&player[i]);
	}

	void setConfig(const WorldSettings& ws, const PowerupSettings& ps) { config = ws; pupConfig = ps; }

	const WorldSettings& getConfig() const { return config; }
	const PowerupSettings& getPupConfig() const { return pupConfig; }

	// common (virtual in base) extended functions
	bool load_map(const char *mapdir, const std::string& mapname);
	void returnAllFlags();
	void returnFlag(int team, int flag);
	void dropFlag(int team, int flag, int roomx, int roomy, int lx, int ly);
	void stealFlag(int team, int flag, int carrier);
	int getMapTime() const { return frame - map_start_time; }
	bool isTimeLimit() const { return config.getTimeLimit() > 0; }
	int getTimeLeft() const { return config.getTimeLimit() - getMapTime(); }
	int getExtraTimeLeft() const { return config.getTimeLimit() + config.getExtraTime() - getMapTime(); }

	// server specific functions
	void reset();
	void reset_time() { map_start_time = frame; }
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
	bool check_flag_touch(const Flag& flag, int px, int py, int x, int y);
	void game_player_screen_change(int p);

	bool dropFlagIfAny(int pid, bool purpose = false);
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

	std::vector<ClientPlayer> player;

	ClientWorld() : skipped(true), player(MAX_PLAYERS) {
		for (int i=0; i<MAX_PLAYERS; ++i)
			WorldBase::player[i].setPtr(&player[i]);
	}
	// extrapolate : advances from source, a frame per every ctrl listed except the last one which gets subFrameAfter, controls are for player me
	void ClientWorld::extrapolate(ClientWorld& source, PhysicsCallbacksBase& physCallbacks, int me,
						ClientControls* ctrlTab, NLubyte ctrlFirst, NLubyte ctrlLast, float subFrameAfter);

	/*void save_stats(const std::string& dir, const Team* teams,
				const std::vector<ClientPlayer*>& players, const std::string& map_name) const;*/
};

#endif
