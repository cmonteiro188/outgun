#ifndef CLIENT_H_INC
#define CLIENT_H_INC

#include "client_menus.h"
#include "function_utility.h"
#include "gameserver_interface.h"
#include "graphics.h"
#include "log.h"
#include "menu.h"
#include "mutex.h"
#include "names.h"
#include "protocol.h"
#include "sounds.h"
#include "thread.h"
#include "world.h"

#define CL_MINIMAP_FLAGPOS  // paint minimap more intelligently according to flag positions
#define CL_SHOW_FLAGPOS // show a flag position marker on the ground

// size of udp download queue (only 1 should be needed but...)
#define MAX_UDPDQ 16

//server record
struct gamespy_t {
	NLaddress	addr;
	std::string	address;  //IP-address typein buffer
	bool		refreshed;
	bool		noresponse;
	int			ping;
	std::string	info;

	gamespy_t() : refreshed(false), noresponse(true), ping(0) { }
};

struct download_runes_t {
	int did;	//download id

	char type[64];	//type of file to download
	char name[256];	//name of file to download
	char dest[512];	//full destination path+name for downloaded file
};

enum Menu_selection {	// screens that aren't quite menus //#fix: get rid
	menu_none,
	menu_maps,
	menu_players,
	menu_teams
};

class ServerThreadOwner {
	LogSet log;
	bool threadFlag, quitFlag;	// threadFlag is true whenever the thread hasn't been joined, quitFlag false only when the server is successfully running
	int runPort;
	Thread serverThread;

	void threadFn(const ServerExternalSettings& config);

public:
	ServerThreadOwner(LogSet logs) : log(logs), threadFlag(false), quitFlag(true) { }
	~ServerThreadOwner() { if (threadFlag) stop(); }
	bool running() { if (quitFlag && threadFlag) stop(); nAssert(quitFlag != threadFlag); return !quitFlag; }
	int port() const { return runPort; }
	void start(int port, const ServerExternalSettings& config);
	void stop();
};

class TournamentPasswordManager {
public:
	typedef HookFunctionHolder1<void, std::string> TokenCallbackT;	// an empty string is given to indicate no token

	enum PasswordStatus {
		PS_noPassword,
		PS_starting,
		PS_socketError,
		PS_sending,
		PS_sendError,
		PS_receiving,
		PS_recvError,
		PS_invalidResponse,
		PS_unavailable,
		PS_tokenReceived,
		PS_badLogin,
		// these follow from server status and passStatus is never one of these:
		PS_tokenSent,
		PS_tokenAccepted,
		PS_tokenRejected
	};

	TournamentPasswordManager(LogSet logs, TokenCallbackT tokenCallbackFunction);	// warning: the callback will be called from a background thread

	void stop();
	void changeData(const std::string& newName, const std::string& newPass);
	PasswordStatus status() const { if (passStatus == PS_tokenReceived && servStatus != PS_noPassword) return servStatus; else return passStatus; }
	const char* statusAsString() const;
	std::string getToken() const { return token.read(); }

	void serverProcessingToken()	{ servStatus = PS_tokenSent;		}
	void serverAcceptsToken()		{ servStatus = PS_tokenAccepted;	}
	void serverRejectsToken()		{ servStatus = PS_tokenRejected;	}
	void disconnectedFromServer()	{ servStatus = PS_noPassword;		}	// actually, no server

private:
	LogSet log;
	TokenCallbackT tokenCallback;

	volatile bool quitThread;	// set to quit the thread
	volatile PasswordStatus passStatus;	// set by both the thread and the regular interface to indicate the status
	Thread thread;

	PasswordStatus servStatus;

	std::string name, password;	// constant while the thread is running
	Threadsafe<std::string> token;

	void start();
	void threadFn();
	void setToken(const std::string& newToken);
};

class ClientExternalSettings {
public:
	int winclient;		// windowed client? ; -1 = undefined, 0 = false, 1 = true (-win / -fs)
	int trypageflip;	// try page flipping? ; -1 = undefined, 0 = false, 1 = true (-flip / -dbuf)
	bool nosound;		// disable sound? -nosound
	int targetfps;		// target (MAX) frames-per-second ; -1 = undefined

	typedef void StatusOutputFnT(const std::string& str);
	StatusOutputFnT* statusOutput;

	ClientExternalSettings() : winclient(-1), trypageflip(-1), nosound(false), targetfps(-1) { }
};

class client_c;	// of leetnet
class client_runes_t;

class gameclient_c {
	FileLog normalLog;
	SupplementaryLog<MemoryLog> errorLog;
	SupplementaryLog<FileLog> securityLog;
	mutable LogSet log;

	std::vector<std::vector<std::string> > load_all_player_passwords() const;
	std::string load_player_password(const std::string& name, const std::string& address) const;
	void save_player_password(const std::string& name, const std::string& address, const std::string& password) const;
	void remove_player_password(const std::string& name, const std::string& address) const;
	int remove_player_passwords(const std::string& name) const;

	void save_stats() const;
	void print_team_stats_row(std::ostream& out, const std::string& header, int amount1, int amount2, const std::string& postfix = "") const;

	ServerThreadOwner listenServer;

	// world
	ClientWorld fd, fx;	//#fix: two maps, etc.
	std::vector<ClientPlayer*> players_sb;	// player pointers for scoreboard
	int me;
	pthread_mutex_t frame_mutex;
	int maxplayers;

	// network
	client_c *client;
	double lastpackettime;
	NLubyte clFrameSent, clFrameWorld;
	#ifdef SEND_FRAMEOFFSET
	float frameOffsetDeltaTotal;
	int frameOffsetDeltaNum;
	volatile int netsendAdjustment;
	#endif
	float averageLag;
	double frameReceiveTime;	// when fx was received
	ClientControls controlHistory[256];	// the section between clFrameWorld and clFrameSent (circularly) is in use on a given moment
	NLulong svFrameHistory[256];	// the section between clFrameWorld and clFrameSent (circularly) is in use on a given moment
	volatile bool connected;
	bool map_ready;
	std::string old_map;
	char servermap[64]; //last map command from server

	pthread_mutex_t udpdq_mutex;
	int udpdq_size;
	download_runes_t *udpdq[MAX_UDPDQ];	//the udp download queue
	int udpdq_ptr;	//current download. if -1, no current downloads
	int ud_fp;	//file pointer for read/write
	FILE *ud_fout;	//input or output file

	TournamentPasswordManager tournamentPassword;

	NLulong fdp, fdp_max;
	float max_world_score;
	NLulong max_world_rank;

	pthread_mutex_t mapInfoMutex;
	std::vector<MapInfo> maps;
	int current_map;
	int map_vote;
	bool want_change_teams;
	bool want_map_exit;
	bool map_time_limit;
	NLulong map_start_time;
	NLulong map_end_time;

	// GUI
	Menu_main menu;
	Menu_text m_connectProgress;
	Menu_text m_dialog;	// take care not to open multiple dialogs (same goes to other menus too); to allow that, a vector of Menu_text should be created
	Menu_text m_errors;
	Menu_playerPassword m_playerPassword;
	Menu_serverPassword m_serverPassword;
	Menu_text m_serverInfo;
	Menu_help m_help;

	MenuStack openMenus;

	Menu_selection menusel;	// a special screen rather than menu: maplist, stats
	bool gameshow;
	double FPS;
	int framecount, totalframecount;
	double frameCountStartTime;
	int gameover_plaque;
	int red_final_score, blue_final_score;
	std::string hostname;
	std::string edit_map_vote;
	int player_stats_page;

	std::vector<gamespy_t> gamespy;
	std::vector<gamespy_t> mgamespy;	//gamespy of master server
	MutexHolder serverListMutex;

	std::string playername;	//the player's name (max name len = 16)
	std::string address;	//server IP address

	volatile bool abortThreads;

	enum RefreshStatus { RS_none, RS_running, RS_failed, RS_contacting, RS_connecting, RS_receiving };
	volatile RefreshStatus refreshStatus;	// thread communication variable

	std::string password_file;

	std::string talkbuffer;
	std::list<Message> chatbuffer;
	bool show_all_messages;
	Graphics client_graphics;
	bool screenshot;
	Sounds client_sounds;

	std::ofstream message_log;

	const ClientExternalSettings extConfig;
	const ServerExternalSettings serverExtConfig;

	class GFXMode {
	public:
		int width, height, depth;
		bool windowed;

		GFXMode() : width(-1) { }
		GFXMode(int w, int h, int d, bool win) : width(w), height(h), depth(d), windowed(win) { }
		bool used() const { return width != -1; }
	};

	GFXMode workingGfxMode;

	void initMenus();

	// menu callback functions
	void MCF_menuOpener(Menu& menu);
	void MCF_menuCloser();
	void MCF_connect(Textarea& target);
	void MCF_cancelConnect();
	void MCF_disconnect();
	void MCF_startServer();
	void MCF_stopServer();
	void MCF_prepareMainMenu();
	void MCF_prepareNameMenu();
	void MCF_prepareDrawNameMenu();
	void MCF_nameMenuClose();
	void MCF_nameChange();	// only function to clear the password
	void MCF_randomName();
	void MCF_removePasswords();
	void MCF_prepareGameMenu();
	void MCF_joystick();
	void MCF_messageLogging();
	void MCF_screenDepthChange();
	void MCF_screenModeChange();
	void MCF_gfxThemeChange();
	void MCF_antialiasChange();
	void MCF_statsBgChange();
	void MCF_prepareGfxMenu();
	void MCF_prepareDrawGfxMenu();
	void MCF_sndEnableChange();
	void MCF_sndVolumeChange();
	void MCF_sndThemeChange();
	void MCF_prepareSndMenu();
	void MCF_prepareServerMenu();
	void MCF_updateServers();
	void MCF_refreshServers();
	void MCF_prepareAddServer();
	void MCF_addServer();
	bool MCF_addRemoveServer(Textarea& target, char scan, unsigned char chr);
	void MCF_loadHelp();
	void MCF_playerPasswordAccept();
	void MCF_serverPasswordAccept();
	void MCF_clearErrors();

	void CB_tournamentToken(std::string token);	// callback called by tournamentPassword from another thread

	bool screenModeChange();	// the return value should be tested at the first call

	void setMaxPlayers(int num) { maxplayers = num; fx.setMaxPlayers(num); fd.setMaxPlayers(num); }

public:
	gameclient_c(LogSet hostLogs, const ClientExternalSettings& config, const ServerExternalSettings& serverConfig);
	virtual ~gameclient_c();
	bool start();
	void loop(volatile bool* quitFlag);
	void stop();

	// world	//#fix: should these be moved to ClientWorld?
	void rocketHitWallCallback(int rid, bool power, float x, float y, int roomx, int roomy);
	void rocketOutOfBoundsCallback(int rid);
	void playerHitWallCallback(int pid);
	void playerHitPlayerCallback(int pid1, int pid2);
	bool shouldApplyPhysicsToPlayerCallback(int pid);

	// network
	void connect_command(bool loadPassword);
	void disconnect_command();
	void connection_update(client_runes_t *arg);
	void client_connected(char* data, int length);
	void client_disconnected(const char* data, int length);
	void connect_failed_denied(char* data, int length);
	void connect_failed_unreachable();
	void send_player_token();
	void send_tournament_participation();
	void issue_change_name_command();
	void change_name_command();
	void send_client_ready();
	void send_chat(const std::string& msg);
	void send_frame(bool newFrame);
	void process_incoming_data(char* data, int length);

	const char* refreshStatusAsString() const;
	void getServerListThread();
	void refreshThread();
	bool refresh_all_servers();
	bool refresh_servers(std::vector<gamespy_t>& gamespy);
	bool getServerList();

	void process_udp_download_chunk(int last, NLulong pos, int len, char* buf);
	void client_udp_setup_download();
	void client_udp_download(download_runes_t* rune);
	void download_file_complete(download_runes_t* r);
	void download_server_file(const char* type, const char* name, const char* dest);
	void server_map_command(const char* mapname, NLushort server_crc);

	// GUI
	void erase_first_message();
	void print_message(Message_type type, const std::string& msg);

	void save_screenshot();
	void toggle_help();
	template<class MenuT> void showMenu(MenuT& menu) { nAssert(openMenus.safeTop() != &m_connectProgress.menu); openMenus.open(&menu.menu); }
	void predraw();
	void draw_game_frame();
	void draw_player(int pid);
	void draw_game_menu();
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
