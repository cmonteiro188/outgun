#ifndef SERVER_H_INC
#define SERVER_H_INC

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

extern gameserver_c *gameserver;

#endif
