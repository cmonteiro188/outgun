#ifndef CLIENT_H_INC
#define CLIENT_H_INC
#include "graphics.h"
#include "sounds.h"
#include "world.h"

#define CL_MINIMAP_FLAGPOS  // paint minimap more intelligently according to flag positions
#define CL_SHOW_FLAGPOS // show a flag position marker on the ground

// number of chat messages in the buffer
#define CHAT_SIZE 8

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

class client_c;	// of leetnet

class gameclient_c {
	// world
	ClientWorld fd, fx;	//#fix: two maps, etc.
	int me;
	pthread_mutex_t frame_mutex;

	// network
	client_c *client;
	double lastpackettime;
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
	bool player_token_new;	//TRUE if first call to token servled
	bool player_token_set;
	char player_token[64];
	char player_password[16];
	pthread_t passthread;

	NLulong fdp, fdp_max;
	NLulong max_world_score, max_world_rank;

	bool want_change_teams;
	bool want_map_exit;
	bool option_show_names;
	bool map_time_limit;
	NLulong map_end_time;

	// GUI
	bool menushow;
	int menu;	//menu screen #
	bool gameshow;
	bool helpshow;
	double FPS;
	int framecount, totalframecount;
	double frameCountStartTime;
	int gameover_plaque;
	int red_final_score, blue_final_score;
	int scoreboard[MAX_PLAYERS];
	char hostname[256];
	int strlen_hostname;
	bool showmaster;	//showing master screen (opposite: showing favourites screen)
	bool first_fav_refresh;	//first refresh of favorites page already done?
	gamespy_t gamespy[MAX_GAMESPY];
	int gi;	//what game entry
	gamespy_t mgamespy[MAX_GAMESPY];	//gamespy of masterserver
	char playername[256];	//the player's name (max name len = 16)
	char namestatus[64];	// v0.4.4: NAME STATUS (unregistered, registering..., registered!)
	char editplayername[256];
	char address[256];	//server IP address
	char dialogmessage[256];
	char dialogmessage2[256];
	char talkbuffer[256];
	char chatbuffer[CHAT_SIZE][256];
	double chaterasetime;
	char editplayerpass[64];
	char namecursor[2];
	char passcursor[2];
	int namestatus_code;	//0==NONE  1==LOGGED w/ token  2==LOGIN FAILED by last attempt  3==LOGGED+RECORDING
	Graphics client_graphics;
	bool screenshot;
	Sounds client_sounds;

	ofstream message_log;

public:
	bool message_logging;

	gameclient_c();
	virtual ~gameclient_c();
	bool start();
	void loop();
	void stop();

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
	void send_chat(char *msg);
	void send_frame();
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
	void print_message(const char *msg);

	void save_screenshot();
	void toggle_help();
	void show_progress(char *t1, char *t2, char *t3, int fg = -1, int bg = 0);
	void show_dialog(char *t1, char *t2, char *t3, int fg = -1, int bg = 0);
	void draw_game_frame();
	void draw_game_menu();
	void update_scoreboard();
	void set_menu(int menumber);
};

extern gameclient_c *gameclient;

#endif
