#ifndef CLIENT_H_INC
#define CLIENT_H_INC

#include "world.h"

#define CL_MINIMAP_FLAGPOS  // paint minimap more intelligently according to flag positions
#define CL_SHOW_FLAGPOS // show a flag position marker on the ground
#define CL_FLAGPOS_RAD 30   // the radius of the flag position marker
//#define CL_SHOW_TIME_LEFT

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
extern bool     page_flipping;

bool reset_video_mode();
void setcolors();

//explosion clientside fx
struct clientfx_t {

    bool        used;       //used record?

    int         type;       // type of fx   0==gun explosion
    int         px,py;  // screen where it spawned. if changed when time to redraw, delete it
    double time;        // start time

    //fx specific vars
    int x;                  // screen x  of fx
    int y;                  // screen y  of fx

    //speed fx
    int col1, col2, gundir;

    //deathbringer owner
    int owner;
};

struct download_runes_t {

    int did;        //download id

    char type[64];  //type of file to download
    char name[256]; //name of file to download
    char dest[512]; //full destination path+name for downloaded file

};

//server record
struct gamespy_t {
    NLaddress addr;
    char address[128];  //IP-address typein buffer
    bool invalid;
    bool noresponse;
    bool favs;  //hack
    bool refreshed; //if data below is valid -------------
    char info[128];
};

class gameclient_c {
public:
    player_t player[MAX_PLAYERS];
    clientfx_t      cfx[MAX_CLIENTFX];
    int me;
    bool option_show_names;
    bool player_password_set;   //flag for the thread
    bool player_token_new;      //TRUE if first call to token servled
    bool player_token_set;      //TRUE if player now holds a valid token
    char player_token[64];      // the token
    char player_password[16];
    pthread_t   passthread;
    World fd, fx;	//#fix: two maps, etc.
    pthread_mutex_t     frame_mutex;
    pthread_mutex_t     udpdq_mutex;
    int udpdq_size;     //size
    download_runes_t        *udpdq[MAX_UDPDQ];      //the udp download queue
    int udpdq_ptr;      //current download. if -1, no current downloads
    int ud_fp;          //file pointer for read/write
    FILE *ud_fout;  //input or output file
    NLulong fdp, fdp_max;
    NLulong max_world_score, max_world_rank;
    double lastpackettime;
    client_c *client;
    SAMPLE *sample[NUM_OF_SAMPLES];
    bool sample_reverse[NUM_OF_SAMPLES];
    char sfxthemedir[256];
    char sfxthemename[256];
    al_ffblk    sfxthemeffblk;  //for al_find*
    bool    validtheme;     // if sfxthemedir points to valid dir
    bool menushow;
    int menu;       //menu screen #
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
    char    hostname[256];
    int     strlen_hostname;    //strlen precalculado
    bool                showmaster;     //showing master screen (opposite: showing favourites screen)
    bool                first_fav_refresh;      //first refresh of favorites page already done?
    gamespy_t       gamespy[MAX_GAMESPY];
    int                 gi; //what game entry
    gamespy_t       mgamespy[MAX_GAMESPY];      //gamespy of masterserver
    char playername[256]; //the player's name (max name len = 16)
    char namestatus[64];        // v0.4.4: NAME STATUS (unregistered, registering..., registered!)
    char editplayername[256]; //the player's name edit buffer
    char address[256];      //server IP address
    char dialogmessage[256];    //dialog message
    char dialogmessage2[256];   //dialog message line 2
    char talkbuffer[256];           // chat input buffer
    char chatbuffer[CHAT_SIZE][256];        // last chat messages list
    double chaterasetime;               // time to erase a chat message from the list
    char editplayerpass[64]; //the player's password edit buffer
    char namecursor[2];
    char passcursor[2];
    int     namestatus_code;        //0==NONE  1==LOGGED w/ token  2==LOGIN FAILED by last attempt  3==LOGGED+RECORDING
    BITMAP *minibg;
    BITMAP* flagpos_buf[2];
    bool flagpos_ready;
    bool map_ready;
    char servermap[64]; //last map command from server
    int gameover_plaque;
    int red_final_score, blue_final_score;      //final scores for showing in the gameover plaque
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

#endif
