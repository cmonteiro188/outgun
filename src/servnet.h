#ifndef SERVNET_H_INC
#define SERVNET_H_INC

#include <map>

class gameserver_c;
class masterjob_c;
class server_c;
class ServerHelloResult;

class ServerNetworking {
	class ClientTransferData {
	public:
		//v0.4.4 UDP FILE transfer
		bool		serving_udp_file;			//if TRUE, already serving a file
		char		*data;					//the file data
		NLulong		dp,old_dp,fsize;				//the file pointer and the total size

	public:
		ClientTransferData() {
			serving_udp_file = false;
			data = 0;
		}
		void reset() {
			if ((serving_udp_file) && (data)) {
				delete data;
				data = 0;
			}
			serving_udp_file = false;
		}
	};

	// server callbacks
	static void sfunc_client_hello			(void* customp, int client_id, char* data, int length, ServerHelloResult* res);
	static void sfunc_client_connected		(void* customp, int client_id);
	static void sfunc_client_disconnected	(void* customp, int client_id);
	static void sfunc_client_data			(void* customp, int client_id, char* data, int length);
	static void sfunc_client_lag_status		(void* customp, int client_id, int status);
	static void sfunc_client_ping_result	(void* customp, int client_id, int pingtime);

	//master job (ACCOUNT/STATS SERVER)
	struct MasterjobThreadData {
		ServerNetworking* host;
		masterjob_c* job;
	};
	static void* thread_masterjob_f(void* arg);
	//thread for master server interaction (LIST SERVER)
	static void* thread_mastertalker_f(void* arg);

	//thread for server shell connections
	static void* thread_shellmaster_f(void* arg);
	static void* thread_shellslave_f(void* arg);

	//thread for server website interaction
	static void* thread_website_f(void* arg);

	std::map<std::string, std::string> website_parameters(const std::string& address) const;
	std::string build_http_data(const std::map<std::string, std::string>& parameters) const;
	NLint post_http_data(const std::string& script, std::string parameters, const std::string& auth) const;
	void ServerNetworking::save_http_response(std::ostream& out) const;
	std::string ServerNetworking::encode_base64(const std::string& data) const;

	gameserver_c*	host;
	ServerWorld&	world;

	server_c*		server;

	NLsocket		msock;
	pthread_t		mthread;
	double			master_talk_time;
	bool			master_pre_exiting_ok;	// if no need to kill the master socket...
	bool			master_exiting_ok;		// if no need to kill the master socket...
	bool			master_never_talked;	// if never talked to master, then no need to unregister the server when qutting (optimization)
	bool			mjob_exit;				//flag for all pending master jobs to quit now
	bool			mjob_fastretry;			//flag for all pending master jobs to stop waiting and retry immediately
	int				mjob_count;
	pthread_mutex_t	mjob_mutex;				//mutex for socket list

	int				max_world_score, max_world_rank;

	ClientTransferData fileTransfer[MAX_PLAYERS];
	bool			file_threads_quit;		//terminate all file server threads/sockets now
	NLsocket		filesock;
	pthread_t		server_filemaster_thread;
	pthread_mutex_t	fslavesock_mutex;
	NLsocket		fslavesock[MAX_PLAYERS];
	pthread_t		fslavethr[MAX_PLAYERS];

	NLsocket		shellmsock;
	pthread_t		shellmthread;
	NLsocket		shellssock;
	pthread_t		shellsthread;
	NLsocket		websock;
	pthread_t		webthread;
	
	bool			website_exiting_ok;

	char			hostname[256];
	int				ping_send_client;
	int				ctop[256];			// client id-to-player id index
	int				player_count;

public:
	ServerNetworking(gameserver_c* hostp, ServerWorld& w);
	~ServerNetworking();

	bool start();
	void stop();

	void upload_next_file_chunk(int i);
	int  get_download_file(char *lebuf, char *ftype, char *fname);
	void run_filemaster_thread();
	void run_fileslave_thread(void *arg);

	void update_serverinfo();
	double getTraffic();

	void send_me_packet(int pid);
	void send_player_name_update(int cid, int pid);
	void broadcast_player_name(int pid);
	void send_player_crap_update(int cid, int pid);
	void broadcast_player_crap(int pid);

	void send_map_change_message(int pid, int reason, const char* mapname);
	void send_map_time(int cid);
	void ctf_net_flag_status(int cid, int team);
	void ctf_update_teamscore(int t);
	void move_update_player(int a);
	void client_report_status(int id);
	void sendEndGameover();
	void sendWeaponPower(int pid);
	void sendRocketMessage(int shots, int gundir, NLubyte* sid, int team, bool power, int px, int py, int x, int y, int bsx, int bsy);	// sid = shot-id: array of NLubyte[shots]
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

	void forwardSayadminMessage(int cid, const char* message);

	void newPlayer(int pid);
	void removePlayer(int pid) { ctop[world.player[pid].cid] = -1; }
	void disconnect_client(int cid, int timeout);
	void clientHello(int client_id, char* data, int length, ServerHelloResult* res);
	int  client_connected(int id);
	void client_disconnected(int id);
	void ping_result(int client_id, int ping_time);
	void incoming_client_data(int id, char *data, int length);

	void master_job_response(masterjob_c *j);
	void run_masterjob_thread(masterjob_c* job);
	bool check_private_IP(char *address);
	void run_mastertalker_thread();

	bool read_string_from_TCP(NLsocket sock, char *buf);
	void run_shellmaster_thread();
	void run_shellslave_thread();

	void run_website_thread();

	void broadcast_frame(bool gameRunning);

	void reload_hostname();
	NLaddress get_client_address(int cid) const;
	int get_player_count() const { return player_count; }
};

bool is_url_safe(char c);

#endif
