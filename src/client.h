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
#include "protocol.h"	// needed for possible definition of SEND_FRAMEOFFSET
#include "sounds.h"
#include "thread.h"
#include "world.h"

//server record
class ServerListEntry {
public:
	bool		refreshed;
	bool		noresponse;
	int			ping;
	std::string	info;

	ServerListEntry() : refreshed(false), noresponse(true), ping(0) { }

	const NLaddress& address() const { return addr; }
	std::string addressString() const;
	bool setAddress(const std::string& address);	// returns false if address is invalid

private:
	NLaddress	addr;
};

class FileDownload {
public:
	std::string fileType, shortName, fullName;

	FileDownload(const std::string& type, const std::string& name, const std::string& filename);
	~FileDownload();
	bool isActive() const { return (fp != 0); }
	int progress() const;
	bool start();
	bool save(const char* buf, unsigned len);
	void finish();

private:
	FILE* fp;
};

enum Menu_selection {	// screens that aren't quite menus //#fix: get rid
	menu_none,
	menu_maps,
	menu_players,
	menu_teams
};

enum ClientCfgSetting {
	CCS_PlayerName,
	CCS_Tournament,
	CCS_Favorites,
	CCS_FavoriteColors,
	CCS_LagPrediction,
	CCS_LagPredictionAmount,
	CCS_Joystick,
	CCS_MessageLogging,
	CCS_SaveStats,
	CCS_Windowed,
	CCS_GFXMode,
	CCS_Flipping,
	CCS_FPSLimit,
	CCS_GFXTheme,
	CCS_Antialiasing,
	CCS_StatsBgAlpha,
	CCS_ShowNames,
	CCS_SoundEnabled,
	CCS_Volume,
	CCS_SoundTheme,
	CCS_ShowStats,
	CCS_AutoGetServerList,
	CCS_ShowServerInfo,
	CCS_MaxCommand = CCS_ShowServerInfo
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

	TournamentPasswordManager(LogSet logs, TokenCallbackT tokenCallbackFunction, int threadPriority);	// warning: the callback will be called from a background thread

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
	int priority;

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
	int lowerPriority, priority;	// lower is used for non-timecritical background threads

	typedef void StatusOutputFnT(const std::string& str);
	StatusOutputFnT* statusOutput;

	ClientExternalSettings() : winclient(-1), trypageflip(-1), nosound(false), targetfps(-1) { }
};

class Client;
class client_c;	// of leetnet
class client_runes_t;

/* ThreadMessage represents a class of actions that can't be performed by callbacks within a network thread in a thread-safe way.
 * These are queued for the main thread to execute.
 * Note: they will be executed in order relative to each other but out of order relative to messages processed in the network thread.
 * Handling anything that isn't protected by mutexes (frameMutex and downloadMutex) should be applied only through the ThreadMessage queue.
 */
class ThreadMessage {	// the subclasses (named starting with TM_) are direct property of the Client class and internally declared in client.cpp
public:
	virtual ~ThreadMessage() { }
	virtual void execute(Client* cl) const = 0;
};

class Client {
	friend class ClientPhysicsCallbacks;
	friend class TM_DoDisconnect;
	friend class TM_Text;
	friend class TM_Sound;
	friend class TM_MapChange;
	friend class TM_NameAuthorizationRequest;
	friend class TM_GunexploEffect;
	friend class TM_Deathbringer;
	friend class TM_ServerSettings;
	friend class TM_ConnectionUpdate;

	FileLog normalLog;
	SupplementaryLog<MemoryLog> errorLog;
	// currently not in use:	SupplementaryLog<FileLog> securityLog;
	mutable LogSet log;

	std::vector<std::vector<std::string> > load_all_player_passwords() const;
	std::string load_player_password(const std::string& name, const std::string& address) const;
	void save_player_password(const std::string& name, const std::string& address, const std::string& password) const;
	void remove_player_password(const std::string& name, const std::string& address) const;
	int remove_player_passwords(const std::string& name) const;

	ServerThreadOwner listenServer;

	// world
	ClientWorld fd, fx;	//#fix: two maps, etc.
	std::vector<ClientPlayer*> players_sb;	// player pointers for scoreboard
	int me;
	MutexHolder frameMutex;
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
	std::string servermap;	//last map command from server

	std::deque<ThreadMessage*> messageQueue;	// access with frameMutex locked; delete the object when removing from the queue

	MutexHolder downloadMutex;
	std::list<FileDownload> downloads;

	TournamentPasswordManager tournamentPassword;

	NLulong fdp;
	NLulong max_world_rank;

	MutexHolder mapInfoMutex;
	std::vector<MapInfo> maps;
	int current_map;
	int map_vote;
	bool want_change_teams;
	bool want_map_exit;
	bool map_time_limit;
	int map_start_time;	// in get_time() seconds -> can be negative
	int map_end_time;

	// GUI
	Menu_main menu;
	Menu_text m_connectProgress;
	Menu_text m_dialog;	// take care not to open multiple dialogs (same goes to other menus too); to allow that, a vector of Menu_text should be created
	Menu_text m_errors;
	Menu_playerPassword m_playerPassword;
	Menu_serverPassword m_serverPassword;
	Menu_text m_serverInfo;

	MenuStack openMenus;

	bool quitCommand;
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

	std::vector<ServerListEntry> gamespy;
	std::vector<ServerListEntry> mgamespy;	//gamespy of master server
	MutexHolder serverListMutex;

	std::string playername;	//the player's name (max name len = 16)
	NLaddress serverIP;

	volatile bool abortThreads;

	enum RefreshStatus { RS_none, RS_running, RS_failed, RS_contacting, RS_connecting, RS_receiving };
	volatile RefreshStatus refreshStatus;	// thread communication variable

	std::string password_file;

	std::string talkbuffer;
	std::list<Message> chatbuffer;
	bool show_all_messages;
	bool stats_autoshowing;
	Graphics client_graphics;
	bool screenshot;
	volatile bool mapChanged, predrawNeeded;
	Sounds client_sounds;

	std::ofstream message_log;

	const ClientExternalSettings extConfig;
	const ServerExternalSettings serverExtConfig;

	class GFXMode {
	public:
		int width, height, depth;
		bool windowed, flipping;

		GFXMode() : width(-1) { }
		GFXMode(int w, int h, int d, bool win, int flip) : width(w), height(h), depth(d), windowed(win), flipping(flip) { }
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
	void MCF_playServer();
	void MCF_stopServer();
	void MCF_exitOutgun();
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

	void addThreadMessage(ThreadMessage* msg) { messageQueue.push_back(msg); }
	void setMaxPlayers(int num) { maxplayers = num; fx.setMaxPlayers(num); fd.setMaxPlayers(num); }

	// world	//#fix: should these be moved to ClientWorld?
	void rocketHitWallCallback(int rid, bool power, float x, float y, int roomx, int roomy);
	void rocketOutOfBoundsCallback(int rid);
	void playerHitWallCallback(int pid);
	void playerHitPlayerCallback(int pid1, int pid2);
	bool shouldApplyPhysicsToPlayerCallback(int pid);

	// network
	void connect_command(bool loadPassword);
	void disconnect_command();	// do not call from a network thread
	void connection_update(client_runes_t *arg);
	void client_connected(const char* data, int length);	// call with frameMutex locked
	void client_disconnected(const char* data, int length);
	void connect_failed_denied(const char* data, int length);
	void connect_failed_unreachable();
	void send_player_token();
	void send_tournament_participation();
	void issue_change_name_command();
	void change_name_command();
	void send_client_ready();
	void send_chat(const std::string& msg);
	void send_frame(bool newFrame);
	void process_incoming_data(const char* data, int length);

	const char* refreshStatusAsString() const;
	void getServerListThread();
	void refreshThread();
	bool refresh_all_servers();
	bool refresh_servers(std::vector<ServerListEntry>& gamespy);
	bool getServerList();

	void check_download();	// call with downloadMutex locked
	void process_udp_download_chunk(const char* buf, int len, bool last);
	void download_server_file(const std::string& type, const std::string& name);
	void server_map_command(const std::string& mapname, NLushort server_crc);

	//#fix: leetnet callbacks
	static int cfunc_connection_update(client_runes_t *arg);
	static int cfunc_server_data(client_runes_t *arg);

	// GUI
	void erase_first_message();
	void print_message(Message_type type, const std::string& msg);

	void save_screenshot();
	void toggle_help();
	template<class MenuT> void showMenu(MenuT& menu) { openMenus.open(&menu.menu); }
	void predraw();
	void draw_game_frame();
	void draw_player(int pid);
	void draw_game_menu();

public:
	Client(LogSet hostLogs, const ClientExternalSettings& config, const ServerExternalSettings& serverConfig);
	~Client();
	bool start();
	void loop(volatile bool* quitFlag);
	void stop();
};

extern Client *gameclient;

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
