#ifndef SERVER_H_INC
#define SERVER_H_INC

#define SV_NAME_AUTHORIZATION   // enable player IP based filtering : name authorization and ban
#define SV_NO_PUP_SWITCHING // disable the changing of power-ups lying on the ground
#define SV_VOTE_ANNOUNCE_INTERVAL 5 // in seconds, how often a changing voting status will be announced
#define SV_SHADOW_MINIMUM_NORMAL 7  // the shadow visibility factor

#ifdef SV_NAME_AUTHORIZATION
#include "nameauth.h"
#endif

#include "world.h"

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

	char		request[512];

	bool		html_end;

	char		lebuf[65536];		//lebuf for collecting response
	int			n;	// lebuf length

	int			code;
	int			cid;

	//return values of the callback
	bool		retry;

	masterjob_c() {
		lebuf[0]=0;
		html_end = false;
		request[0]=0;
	}
};

class gameserver_c {
	// yhteydet verkkoon
	server_c	*server;
	char hostadname[128];
	NLsocket		msock;
	pthread_t		mthread;
	double			master_talk_time;	//time to talk?
	bool			master_pre_exiting_ok;		// if no need to kill the master socket...
	bool			master_exiting_ok;		// if no need to kill the master socket...
	bool			master_never_talked;		// if never talked to master, then no need to unregister the server when qutting (optimization)
	bool			mjob_exit;				//flag for all pending master jobs to quit now
	bool			mjob_fastretry;		//flag for all pending master jobs to stop waiting and retry immediately
	int				mjob_count;
	pthread_mutex_t	mjob_mutex;  //mutex for socket list
	bool			file_threads_quit;		//terminate all file server threads/sockets now
	NLsocket		filesock;
	pthread_t		server_filemaster_thread;  // thread for server filemaster
	pthread_mutex_t	fslavesock_mutex;  //mutex for socket list
	NLsocket		fslavesock[MAX_PLAYERS];
	pthread_t		fslavethr[MAX_PLAYERS];
	NLsocket		shellmsock;
	pthread_t		shellmthread;
	NLsocket		shellssock;
	pthread_t		shellsthread;
	char			hostname[256];
	double			server_kbps_traffic;
	int				ping_send_counter, ping_send_client;

	// pelaajien hallinta
	vector<string>	welcome_message;	// welcome message line by line
	vector<string>	info_message;	// the message /info shows, line by line
	string			sayadmin_comment;
	bool			sayadmin_enabled;
	int				ctop[256];			// client id-to-player id index
	oneclient_c		client[MAX_PLAYERS];
	int				max_world_score, max_world_rank;
	double			team_smul[2];
	NLulong 		next_vote_announce_frame;
	int				last_vote_announce_votes, last_vote_announce_needed;

	// pelimaailma
	ServerWorld		world;
	bool			gameover;
	double			gameover_time;		//timeout for gameover plaque

	// asetukset
	PowerupSettings pupConfig;
	WorldSettings worldConfig;
	struct MapInfo {
		string title, file;
		int width, height;
		int votes;
		MapInfo();
		bool load(string mapName);
	};
	vector<MapInfo> maprot;
	int currmap;		// current map in maprot
	bool random_maprot;
	#ifdef SV_NAME_AUTHORIZATION
	NameAuthorizationDatabase authorizations;
	#endif
	int vote_block_time;	// how long a mapchange can't be voted (except unanimously), in frames (in gamemod, it is minutes)

public:

	gameserver_c();
	virtual ~gameserver_c();
	bool start(int target_maxplayers);
	void loop(volatile bool *running_flag);
	void stop();

	// pelaajien hallinta
	void mutePlayer(int pid, int mode);
	void kickPlayer(int pid, bool ban=false);
	#ifdef SV_NAME_AUTHORIZATION
	void banPlayer(int pid);
	bool isBanned(int cid) { return authorizations.isBanned(server->get_client_address(cid)); }
	#endif

   int check[MAX_PLAYERS];
   int checount;
	void check_team_changes();
	void check_player_change_teams(int pid);
	void move_update_player(int a);
	void move_player(int f, int t);
	void swap_players(int a, int b);
	void game_remove_player(int pid) { ctop[world.player[pid].cid] = -1; world.removePlayer(pid); }

	void clearWorldRankingDeltas();
	void ctf_update_teamscore(int t);
	void refresh_team_score_modifiers();
	void client_report_status(int id);
	void check_map_exit();
	void score_frag(int p, int amount);
	void score_neg(int p, int amount);

	bool load_rotation_map(int pos);
	void send_map_change_message(int pid, int reason, const char* mapname);
	bool server_next_map(int reason);

	// yhteydet verkkoon
	void upload_next_file_chunk(int i);
	int  get_download_file(char *lebuf, char *ftype, char *fname);
	void run_filemaster_thread(void *);
	void run_fileslave_thread(void *arg);

	void update_serverinfo();
	void server_think_after_broadcast();

	void send_me_packet(int pid);
	void send_player_name_update(int cid, int pid);
	void broadcast_player_name(int pid);
	void send_player_crap_update(int cid, int pid);
	void broadcast_player_crap(int pid);

	void ctf_net_flag_status(int cid, int team);
	void sendWeaponPower(int pid);
	void sendRocketMessage(int shots, int gundir, NLubyte* sid, int team, bool power, int px, int py, int x, int y);	// sid = shot-id; array of NLubyte[shots]
	void sendRocketDeletion(NLulong plymask, int rid, NLshort hitx, NLshort hity, int targ);
	void sendDeathbringer(int pid, const ServerPlayer& ply);
	void sendPickupVisible(int pid, int pup_id, const pickup_c& it);
	void sendPupTime(int pid, NLubyte pupType, double timeLeft);
	void sendFragUpdate(int pid, NLulong frags);

	void broadcast_sample(int code);
	void broadcast_screen_sample(int p, int code);
	void broadcast_team_message(int team, char *text);
	void broadcast_screen_message(int px, int py, char *lebuf, int count);
	void bprintf(const char *fs, ...);
	void plprintf(int pid, const char* fmt, ...);
	void player_message(int pid, const char *text);
	void broadcast_message(const char *text);

	int  client_connected(int id);
	void client_disconnected(int id);
	void ping_result(int client_id, int ping_time);
	void incoming_client_data(int id, char *data, int length);
	char *get_hostname();

	void master_job_response(masterjob_c *j);
	void run_masterjob_thread(void *arg);
	bool check_private_IP(char *address);
	void run_mastertalker_thread(void *);

	bool read_string_from_TCP(NLsocket sock, char *buf);
	void run_shellmaster_thread(void *);
	void run_shellslave_thread(void *);

	void simulate_and_broadcast_frame();

	// asetukset
	void load_game_mod();
	bool reset_settings();
	void reload_hostname();
};

extern gameserver_c *gameserver;

#endif
