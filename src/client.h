#ifndef CLIENT_H_INC
#define CLIENT_H_INC
#include "graphics.h"
#include "sounds.h"
#include "world.h"
#include "network.h"

#define CL_MINIMAP_FLAGPOS  // paint minimap more intelligently according to flag positions
#define CL_SHOW_FLAGPOS // show a flag position marker on the ground

// size of connect screen
#define MAX_GAMESPY 24

// size of udp download queue (only 1 should be needed but...)
#define MAX_UDPDQ 16

struct download_runes_t {
	int did;	//download id

	char type[64];	//type of file to download
	char name[256];	//name of file to download
	char dest[512];	//full destination path+name for downloaded file
};

enum Menu_selection {
	menu_none,
	menu_main,
	menu_dialog,
	menu_server_list,
	menu_name_password,
	menu_maps,
	menu_players,
	menu_teams,
	menu_server_password,
	menu_player_password
};

class client_c;	// of leetnet

class gameclient_c {
	std::vector<std::vector<std::string> > load_all_player_passwords() const;
	std::string load_player_password(const std::string& name, const std::string& address) const;
	void save_player_password(const std::string& name, const std::string& address, const std::string& password) const;
	void remove_player_password(const std::string& name, const std::string& address) const;

	// world
	ClientWorld fd, fx;	//#fix: two maps, etc.
	std::vector<ClientPlayer*> players_sb;	// player pointers for scoreboard
	int me;
	pthread_mutex_t frame_mutex;

	// network
	client_c *client;
	double lastpackettime;
	NLubyte clFrameSent, clFrameWorld;
	#ifdef SEND_FRAMEOFFSET
	float frameOffsetDeltaTotal;
	int frameOffsetDeltaNum;
	#endif
	float averageLag;
	double frameReceiveTime;	// when fx was received
	ClientControls controlHistory[256];	// the section between clFrameWorld and clFrameSent (circularly) is in use on a given moment
	NLulong svFrameHistory[256];	// the section between clFrameWorld and clFrameSent (circularly) is in use on a given moment
	volatile bool trying_connection;
	volatile bool connected;
	bool map_ready;
	char servermap[64]; //last map command from server

	pthread_mutex_t udpdq_mutex;
	int udpdq_size;
	download_runes_t *udpdq[MAX_UDPDQ];	//the udp download queue
	int udpdq_ptr;	//current download. if -1, no current downloads
	int ud_fp;	//file pointer for read/write
	FILE *ud_fout;	//input or output file

	bool player_password_set;
	bool player_token_new;	//TRUE if first call to token servlet
	bool player_token_set;
	char player_token[64];
	std::string player_password;
	pthread_t passthread;

	NLulong fdp, fdp_max;
	NLulong max_world_score, max_world_rank;

	pthread_mutex_t mapInfoMutex;
	std::vector<MapInfo> maps;
	int current_map;
	int map_vote;
	std::string edit_map_vote;
	int player_stats_page;
	bool want_change_teams;
	bool want_map_exit;
	bool option_show_names;
	bool map_time_limit;
	NLulong map_start_time;
	NLulong map_end_time;

	// GUI
	bool menushow;
	//int menu;	//menu screen #
	Menu_selection menu;
	bool gameshow;
	bool helpshow;
	double FPS;
	int framecount, totalframecount;
	double frameCountStartTime;
	int gameover_plaque;
	int red_final_score, blue_final_score;
	int scoreboard[MAX_PLAYERS];
	std::string hostname;
	int strlen_hostname;
	bool showmaster;	//showing master screen (opposite: showing favourites screen)
	bool first_fav_refresh;	//first refresh of favorites page already done?
	gamespy_t gamespy[MAX_GAMESPY];
	int gi;	//what game entry
	gamespy_t mgamespy[MAX_GAMESPY];	//gamespy of masterserver

	std::string playername;	//the player's name (max name len = 16)
	std::string namestatus;	// v0.4.4: NAME STATUS (unregistered, registering..., registered!)
	std::string editplayername;
	std::string address;	//server IP address

	std::string edit_server_password;
	std::string edit_player_password;
	bool save_pl_password;
	bool save_password_selected;
	std::string password_file;
	bool autoconnect;
	
	std::string dialogmessage;
	std::string dialogmessage2;
	std::string talkbuffer;
	std::list<Message> chatbuffer;
	static const std::size_t chat_size;
	std::size_t chat_visible;
	std::string editplayerpass;
	bool name_selected;
	int namestatus_code;	//0==NONE  1==LOGGED w/ token  2==LOGIN FAILED by last attempt  3==LOGGED+RECORDING
	Graphics client_graphics;
	bool screenshot;
	Sounds client_sounds;

	std::ofstream message_log;

	static bool force_exit;
	static void close_button_callback();

public:
	bool message_logging;

	gameclient_c();
	virtual ~gameclient_c();
	bool start();
	void loop();
	void stop();

	// world	//#fix: should these be moved to ClientWorld?
	void rocketHitWallCallback(int rid, bool power, float x, float y, int roomx, int roomy);
	void rocketOutOfBoundsCallback(int rid);
	void playerHitWallCallback(int pid);
	bool shouldApplyPhysicsToPlayerCallback(int pid);

	// network
	void connect_command();
	void disconnect_command();
	void client_connected(char *data, int length);
	void client_disconnected();
	void connect_failed_denied(char *data, int length);
	void connect_failed_unreachable();
	void refresh_command();
	void refresh_command_2(gamespy_t *gamespy);
	void send_player_token();
	void issue_change_name_command();
	void change_name_command();
	void send_client_ready();
	void send_chat(const std::string& msg);
	void send_frame(bool newFrame);
	void process_incoming_data(char *data, int length);

	void check_change_pass_command();
	void client_password_thread(void *);
	void get_servers_from_master();

	void process_udp_download_chunk(int last, NLulong pos, int len, char* buf);
	void client_udp_setup_download();
	void client_udp_download(download_runes_t  *rune);
	void download_file_complete(download_runes_t  *r);
	void download_server_file(const char *type, const char *name, char *dest);
	void server_map_command(const char *mapname, NLushort server_crc);

	// sounds
	void sound(int s) const;
	Sounds& sounds() { return client_sounds; }

	// GUI
	Graphics& graphics() { return client_graphics; }

	void erase_first_message();
	void print_message(Message_type type, const std::string& msg);

	void save_screenshot();
	void toggle_help();
	void show_progress(char *t1, char *t2, char *t3, int fg = -1, int bg = 0);
	void show_dialog(char *t1, char *t2, char *t3, int fg = -1, int bg = 0);
	void predraw();
	void draw_game_frame();
	void draw_player(int pid);
	void draw_game_menu();
	void update_scoreboard();
	void set_menu(Menu_selection menumber);
};

extern gameclient_c *gameclient;

class Message {
public:
	Message(Message_type type_, const std::string& txt, int time_):
		msg_type(type_), msg_text(txt), msg_time(time_) { }

	Message_type type() const { return msg_type; }
	const std::string& text() const { return msg_text; }
	int time() const { return msg_time; }

private:
	Message_type msg_type;
	std::string msg_text;
	int msg_time;
};

#endif
