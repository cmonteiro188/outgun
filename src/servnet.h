#ifndef SERVNET_H_INC
#define SERVNET_H_INC

#include "network.h"	// for NetworkResult
#include "protocol.h"	// needed for possible definition of SEND_FRAMEOFFSET, and otherwise
#include "thread.h"
#include "utility.h"

#include <map>

class gameserver_c;
class masterjob_c;
class Powerup;
class rocket_c;
class server_c;
class ServerHelloResult;
class ServerPlayer;
class ServerWorld;

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

	std::map<std::string, std::string> master_parameters(const std::string& address) const;
	std::map<std::string, std::string> website_parameters(const std::string& address) const;
	std::string website_maplist() const;
	std::string build_http_data(const std::map<std::string, std::string>& parameters) const;
	NetworkResult post_http_data(NLsocket& socket, const volatile bool* abortFlag, int timeout,
							const std::string& script, const std::string& parameters, const std::string& auth = "") const;	// timeout in ms
	NetworkResult save_http_response(NLsocket& socket, std::ostream& out, const volatile bool* abortFlag, int timeout) const;	// timeout in ms

	gameserver_c*	host;
	ServerWorld&	world;
	int				maxplayers;

	server_c*		server;

	LogSet			log;

	#ifdef SEND_FRAMEOFFSET
	double			frameSentTime;	// at what time the last frame was sent
	#endif

	volatile bool	mjob_exit;				//flag for all pending master jobs to quit now
	volatile bool	mjob_fastretry;			//flag for all pending master jobs to stop waiting and retry immediately
	volatile int	mjob_count;
	pthread_mutex_t	mjob_mutex;				//mutex for socket list

	int				max_world_rank;

	ClientTransferData fileTransfer[MAX_PLAYERS];
	volatile bool	file_threads_quit;		//#fix: this is used by all kinds of threads even though file threads no longer exist

	NLsocket		shellssock;	// set NL_INVALID when no connection; otherwise admin shell messages can be sent to this socket
	Thread			shellmthread;

	Thread			mthread;
	Thread			webthread;
	
	std::string		hostname;
	std::string		server_password;
	int				ping_send_client;
	int				ctop[256];			// client id-to-player id index
	int				player_count;

	// web site settings
	std::vector<std::string> web_servers;
	std::string web_script;
	std::string web_auth;
	int web_refresh;

	void upload_next_file_chunk(int i);
	int  get_download_file(char *lebuf, char *ftype, char *fname);

	void clientHello(int client_id, char* data, int length, ServerHelloResult* res);
	int  client_connected(int id);
	void client_disconnected(int id);
	void ping_result(int client_id, int ping_time);
	void incoming_client_data(int id, char *data, int length);

	void master_job_response(masterjob_c *j);
	void run_masterjob_thread(masterjob_c* job);
	void run_mastertalker_thread();

	bool read_string_from_TCP(NLsocket sock, char *buf);
	void run_shellmaster_thread(int port);
	void run_shellslave_thread(volatile bool* quitFlag);

	void run_website_thread();

public:
	ServerNetworking(gameserver_c* hostp, ServerWorld& w, LogSet logs);
	~ServerNetworking();
	void setMaxPlayers(int num) { maxplayers = num; }

	bool start();
	void stop();

	void update_serverinfo();
	double getTraffic();

	void removePlayer(int pid);	// call only when moving players around; this actually does close to nothing
	void disconnect_client(int cid, int timeout, Disconnect_reason reason);
	int getPid(int cid) { return ctop[cid]; }	//#fix: this shouldn't be necessary

	void send_me_packet(int pid);
	void send_player_name_update(int cid, int pid);
	void broadcast_new_player_notice(int pid);
	void broadcast_player_name(int pid);
	void send_player_crap_update(int cid, int pid);
	void broadcast_player_crap(int pid);

	void broadcast_capture(const ServerPlayer& player) const;
	void broadcast_flag_take(const ServerPlayer& player) const;
	void broadcast_flag_return(const ServerPlayer& player) const;
	void broadcast_flag_drop(const ServerPlayer& player) const;
	void broadcast_kill(const ServerPlayer& attacker, const ServerPlayer& target, bool deathbringer, bool flag) const;
	void broadcast_suicide(const ServerPlayer& player, bool flag) const;
	void broadcast_spawn(const ServerPlayer& player) const;
	void send_movements_and_shots(const ServerPlayer& player) const;	// Send player's movement and shots to everyone.
	void send_stats(const ServerPlayer& player) const;					// Send everyone's stats to player.
	void send_team_movements_and_shots(const ServerPlayer& player) const;
	void send_team_stats(const ServerPlayer& player) const;

	void send_map_info(const ServerPlayer& player);
	void broadcast_map_votes_update();
	void send_map_change_message(int pid, int reason, const char* mapname);
	void send_map_time(int cid);
	void send_server_settings(const ServerPlayer& player);
	void ctf_net_flag_status(int cid, int team);
	void ctf_update_teamscore(int t);
	void move_update_player(int a, bool silent = false);
	void client_report_status(int id);
	void sendWorldReset();
	void sendStartGame();
	void sendWeaponPower(int pid);
	void sendRocketMessage(int shots, int gundir, NLubyte* sid, int team, bool power, int px, int py, int x, int y);	// sid = shot-id: array of NLubyte[shots]
	void sendOldRocketVisible(int pid, int rid, const rocket_c& rocket);
	void sendRocketDeletion(NLulong plymask, int rid, NLshort hitx, NLshort hity, int targ);
	void sendDeathbringer(int pid, const ServerPlayer& ply);
	void sendPickupVisible(int pid, int pup_id, const Powerup& it);
	void sendPupTime(int pid, NLubyte pupType, double timeLeft);
	void sendFragUpdate(int pid, NLulong frags);
	void sendNameAuthorizationRequest(int pid);

	void broadcast_sample(int code);
	void broadcast_screen_sample(int p, int code);
	void broadcast_team_message(int team, const std::string& text);
	void broadcast_screen_message(int px, int py, char *lebuf, int count);
	void bprintf(Message_type type, const char *fs, ...);
	void plprintf(int pid, Message_type type, const char* fmt, ...);
	void player_message(int pid, Message_type type, const std::string& text);
	void broadcast_message(Message_type type, const std::string& text);

	void forwardSayadminMessage(int cid, const std::string& message);

	void broadcast_frame(bool gameRunning);

	void set_hostname(const std::string& name);
	NLaddress get_client_address(int cid) const;
	int get_player_count() const { return player_count; }

	void clear_web_servers() { web_servers.clear(); }
	void add_web_server(const std::string& server) { web_servers.push_back(server); }
	void set_web_script(const std::string& script) { web_script = script; }
	void set_web_auth(const std::string& auth) { web_auth = auth; }
	bool set_web_refresh(int refresh);

	void set_server_password(const std::string& passwd) { server_password = passwd; }
};

#endif
