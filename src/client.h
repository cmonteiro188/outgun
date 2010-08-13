/*
 *  client.h
 *
 *  Copyright (C) 2002 - Fabio Reis Cecin
 *  Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009, 2010 - Niko Ritari
 *  Copyright (C) 2003, 2004, 2005, 2006, 2008 - Jani Rivinoja
 *  Copyright (C) 2006 - Peter Kosyh
 *
 *  This file is part of Outgun.
 *
 *  Outgun is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Outgun is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Outgun; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef CLIENT_H_INC
#define CLIENT_H_INC

#ifndef DEDICATED_SERVER_ONLY
#include <map>
#include <set>
#include <sstream>

#include "client_menus.h"
#include "graphics.h"
#include "menu.h"
#include "sounds.h"
#endif

#include "client_interface.h"
#include "function_utility.h"
#include "gameserver_interface.h"
#include "log.h"
#include "map_with_helpers.h"
#include "mutex.h"
#include "pointervector.h"
#include "thread.h"
#include "world.h"

class BinaryReader;

#ifndef DEDICATED_SERVER_ONLY
//server record
class ServerListEntry {
public:
    bool refreshed;
    bool noresponse;
    int ping;
    std::string info;

    ServerListEntry() throw () : refreshed(false), noresponse(true), ping(0) { }

    const Network::Address& address() const throw () { return addr; }
    std::string addressString() const throw ();
    bool setAddress(const std::string& address) throw ();    // returns false if address is invalid
    void setAddress(const Network::Address& address) throw ();

private:
    Network::Address addr;
};

class FileDownload {
public:
    std::string fileType, shortName, fullName;

    FileDownload(const std::string& type, const std::string& name, const std::string& filename) throw ();
    ~FileDownload() throw ();
    bool isActive() const throw () { return (fp != 0); }
    int progress() const throw ();
    bool start() throw ();
    bool save(ConstDataBlockRef data) throw ();
    void finish() throw ();

private:
    FILE* fp;
};

enum Menu_selection {   // screens that aren't quite menus //#fix: get rid
    menu_none,
    menu_maps,
    menu_players,
    menu_teams
};

class ServerThreadOwner {
    LogSet log;
    bool threadFlag, quitFlag;  // threadFlag is true whenever the thread hasn't been joined, quitFlag false only when the server is successfully running
    int runPort;
    Thread serverThread;

    void threadFn(const ServerExternalSettings& config) throw ();

public:
    ServerThreadOwner(LogSet logs) throw () : log(logs), threadFlag(false), quitFlag(true) { }
    ~ServerThreadOwner() throw () { if (threadFlag) stop(); }
    bool running() throw () { if (quitFlag && threadFlag) stop(); nAssert(quitFlag != threadFlag); return !quitFlag; }
    int port() const throw () { return runPort; }
    void start(int port, const ServerExternalSettings& config) throw ();
    void stop() throw ();
};

class RankingPasswordManager {
public:
    typedef HookFunctionHolder1<void, std::string> TokenCallbackT;  // an empty string is given to indicate no token

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

    RankingPasswordManager(LogSet logs, TokenCallbackT tokenCallbackFunction, int threadPriority) throw ();   // warning: the callback will be called from a background thread
    ~RankingPasswordManager() throw () { stop(); }

    void stop() throw ();
    void changeData(const std::string& newName, const std::string& newPass) throw ();
    PasswordStatus status() const throw () { if (passStatus == PS_tokenReceived && servStatus != PS_noPassword) return servStatus; else return passStatus; }
    std::string statusAsString() const throw ();
    std::string getToken() const throw () { return token.read(); }

    void serverProcessingToken()  throw () { servStatus = PS_tokenSent;        }
    void serverAcceptsToken()     throw () { servStatus = PS_tokenAccepted;    }
    void serverRejectsToken()     throw () { servStatus = PS_tokenRejected;    }
    void disconnectedFromServer() throw () { servStatus = PS_noPassword;       }   // actually, no server

private:
    LogSet log;
    TokenCallbackT tokenCallback;

    volatile bool quitThread;   // set to quit the thread
    volatile PasswordStatus passStatus; // set by both the thread and the regular interface to indicate the status
    Thread thread;
    int priority;

    PasswordStatus servStatus;

    std::string name, password; // constant while the thread is running
    Threadsafe<std::string> token;

    void start() throw ();
    void threadFn() throw ();
    void setToken(const std::string& newToken) throw ();
};
#endif

class ClientBase;
class client_c; // of leetnet
class client_runes_t;

/* ThreadMessage represents a class of actions that can't be performed by callbacks within a network thread in a thread-safe way.
 * These are queued for the main thread to execute.
 * Note: they will be executed in order relative to each other but out of order relative to messages processed in the network thread.
 * Handling anything that isn't protected by mutexes (frameMutex and downloadMutex) should be applied only through the ThreadMessage queue.
 */
class ThreadMessage {   // the subclasses (named starting with TM_) except TM_ServerSettings are direct property of the ClientBase class and mostly internally declared in client.cpp
public:
    virtual ~ThreadMessage() throw () { }
    virtual void execute(ClientBase* cl) const throw () = 0;
};

class ClientBase {
    friend class ClientPhysicsCallbacks;
    friend class TM_DoDisconnect;
    friend class TM_Text;
    friend class TM_Sound;
    friend class TM_MapChange;
    #ifndef DEDICATED_SERVER_ONLY
    friend class TM_NameAuthorizationRequest;
    friend class TM_GunexploEffect;
    #endif
    friend class TM_ConnectionUpdate;

protected:

    MemoryLog& externalErrorLog;    // this is emptied to the error dialog as we go; only rare leftovers are left to caller
    DualLog errorLog;
    // currently not in use:    SupplementaryLog<FileLog> securityLog;
    mutable LogSet log; // this is the only access to logs in normal operation

    // world (these variables all locked by frameMutex)
    ClientWorld fx; //#fix fx and fd: two maps, etc.
    #ifndef DEDICATED_SERVER_ONLY
    ClientWorld fd;
    std::vector<ClientPlayer*> players_sb;  // player pointers for scoreboard
    #endif
    int me;
    Mutex frameMutex;
    int maxplayers;

    // network
    client_c *client;
    double lastpackettime;
    uint8_t clFrameSent, clFrameWorld;
    double frameOffsetDeltaTotal;
    int frameOffsetDeltaNum;
    double netsendAdjustment;
    double averageLag;
    double frameReceiveTime;    // when fx was received
    ClientControls controlHistory[256]; // the section between clFrameWorld and clFrameSent (circularly) is in use on a given moment
    ClientControls sentControls;
    double svFrameHistory[256];    // the section between clFrameWorld and clFrameSent (circularly) is in use on a given moment

    GunDirection gunDir; // used only with allowFreeTurning
    double gunDirRefreshedTime;
    double next_respawn_time;
    int flag_return_delay;
    volatile bool connected;
    bool map_ready;
    int clientReadiesWaiting;
    std::string servermap;  //last map command from server

    int protocolExtensions; // -1 means unextended protocol, 0 up are extension version numbers (<= PROTOCOL_EXTENSIONS_VERSION)

    std::deque<ThreadMessage*> messageQueue;    // access with frameMutex locked; delete the object when removing from the queue

    #ifndef DEDICATED_SERVER_ONLY
    bool map_time_limit;
    int map_start_time; // in get_time() seconds -> can be negative
    int map_end_time;
    bool extra_time_running;
    #endif
    int8_t remove_flags;
    bool lock_team_flags_in_effect;
    bool lock_wild_flags_in_effect;
    bool capture_on_team_flags_in_effect;
    bool capture_on_wild_flags_in_effect;

    bool gameshow;
    int gameover_plaque;

    std::string playername;
    Network::Address serverIP;

    #ifndef DEDICATED_SERVER_ONLY
    bool botmode;
    #else
    static const bool botmode = true;
    #endif

    volatile bool abortThreads;

    #ifndef DEDICATED_SERVER_ONLY
    bool replaying;
    bool spectating;
    #else
    static const bool replaying = false; // To avoid lots of ifdefs.
    #endif
    volatile bool mapChanged;

    const ClientExternalSettings extConfig;
    #ifndef DEDICATED_SERVER_ONLY
    SimpleGameSettings gameSettings; // for stats only
    #endif

    void addThreadMessage(ThreadMessage* msg) throw () { messageQueue.push_back(msg); }
    void setMaxPlayers(int num) throw () {
        maxplayers = num;
        fx.setMaxPlayers(num);
        #ifndef DEDICATED_SERVER_ONLY
        fd.setMaxPlayers(num);
        #endif
    }

    // world    //#fix: should these be moved to ClientWorld?
    virtual void rocketHitWallCallback(int rid, bool power, double x, double y, int roomx, int roomy) throw ();
    void rocketOutOfBoundsCallback(int rid) throw ();
    virtual void playerHitWallCallback(int pid) throw () { (void)pid; }
    virtual void playerHitPlayerCallback(int pid1, int pid2) throw () { (void)(pid1 && pid2); }
    bool shouldApplyPhysicsToPlayerCallback(int pid) throw ();

    void remove_useless_flags() throw ();

    // network
    void prepareForConnect() throw (); // call with frameMutex locked
    void connect(const std::string& serverAddress, const std::string& serverPassword, const std::string& playerPassword) throw (); // call with frameMutex locked
    void disconnect_command() throw ();  // do not call from a network thread
    void connection_update(int connect_result, ConstDataBlockRef data) throw ();
    virtual void client_connected(ConstDataBlockRef data) throw ();    // call with frameMutex locked
    void disconnected_base(ConstDataBlockRef data) throw ();
    void sendMinimapBandwidthAny(int players) throw ();
    void issue_change_name_command() throw ();
    void send_client_ready() throw ();
    void readMinimapPlayerPosition(BinaryReader& reader, int pid) throw ();
    bool process_live_frame_data(ConstDataBlockRef data) throw (); // returns false if an error occured that requires disconnecting
    bool process_message(ConstDataBlockRef data) throw (); // if returns false, discard the server/replay
    void process_incoming_data(ConstDataBlockRef data) throw ();

    void server_map_command(const std::string& mapname, uint16_t server_crc) throw ();
    bool load_map(const std::string& directory, const std::string& mapname, uint16_t server_crc) throw ();

    void handlePendingThreadMessages() throw (); // should only be called by the main thread; call with frameMutex locked

    // client callbacks
    static void cfunc_connection_update(void* customp, int connect_result, ConstDataBlockRef data) throw ();
    static void cfunc_server_data(void* customp, ConstDataBlockRef data) throw ();

    // functionality from subclasses; if there is a default implementation (doing nothing), it's used for Robot
    virtual void print_message(Message_type type, const std::string& msg, int sender_team = -1) throw () { (void)type; (void)msg; (void)sender_team; }
    virtual void play_sound(int sample) throw () { (void)sample; }
    virtual void client_disconnected(ConstDataBlockRef data) throw () = 0;
    virtual void connect_failed_denied(ConstDataBlockRef data) throw () { nAssert(0); (void)data; }
    virtual void connect_failed_unreachable() throw () { nAssert(0); }
    virtual void connect_failed_socket() throw () { nAssert(0); }
    virtual void download_server_file(const std::string& type, const std::string& name) throw () { nAssert(0); (void)type; (void)name; } // ### FIX: Override in Robot to disconnect bot or something.
    virtual void process_udp_download_chunk(ConstDataBlockRef data, bool last) throw () { nAssert(0); (void)data; (void)last; }
    virtual void processNameAuthorizationRequest() throw () { nAssert(0); }
    virtual void createGunexploEffect(const WorldCoords& pos, int team, double time) throw () { (void)pos; (void)team; (void)time; }
    virtual void process_replay_packet(ConstDataBlockRef data) throw () { nAssert(0); (void)data; }

    virtual void netRocketFired(int rpx, int rpy, int rx, int ry, bool power) throw () { (void)(rpx && rpy && rx && ry && power); }
    virtual void netRocketHitPlayer(int rockid, int rokx, int roky, double time) throw () { (void)(rockid && rokx && roky && time); }
    virtual void netPowerCollision(int target, double time) throw () { (void)(target && time); }
    virtual void net_data_sound(BinaryReader& read) throw () { (void)read; }
    virtual void net_data_registration_response(BinaryReader& read) throw () { nAssert(0); (void)read; }
    virtual void net_data_map_list(BinaryReader& read) throw () { (void)read; }
    virtual void net_data_crap_update(BinaryReader& read) throw () { (void)read; }
    virtual void net_data_reset_map_list(BinaryReader& read) throw () { (void)read; }
    virtual void net_data_map_vote(BinaryReader& read) throw () { (void)read; }
    virtual void net_data_map_votes_update(BinaryReader& read) throw () { (void)read; }
    virtual void net_data_text_message(BinaryReader& read) throw () { (void)read; }
    virtual void netKill(int attacker, int target, DamageType cause, bool carrier_defended, bool flag_defended, bool flag, bool wild_flag, bool spree_ended, bool spree_started) throw (); // empty
    virtual void netSuicide(int pid, bool flag, bool wild_flag, bool spree_ended) throw () { (void)(pid && flag && wild_flag && spree_ended); }
    virtual void netFlagTake(int pid, bool wild_flag) throw () { (void)(pid && wild_flag); }
    virtual void netFlagReturn(int pid) throw () { (void)pid; }
    virtual void netFlagDrop(int pid, bool wild_flag) throw () { (void)(pid && wild_flag); }
    virtual void netTeamChange(int pl1, int pl2 = -1) throw () { (void)(pl1 && pl2); }
    virtual void netStatsReady() throw () { }
    virtual void netMapChange(const std::string& maptitle, const int map_number, const int total_maps) throw () { (void)maptitle; (void)(map_number && total_maps); }
    virtual void netGameoverPeriodStart(uint32_t redScore, uint32_t blueScore, int caplimit, int timelimit) throw () { (void)(redScore && blueScore && caplimit && timelimit); }
    virtual void netGameoverPeriodEnd() throw () { }
    virtual void netGameStarted() throw () { }
    virtual void netPhysicsChanged() throw () { }
    virtual void netSetHostname(const std::string& name) throw () { (void)name; }
    virtual void netSetCurrentMap(int idx) throw () { (void)idx; }

    virtual std::string getPlayerPassword() const throw () = 0;

    void startBase(const std::string& leetnetLogPostfix = std::string()) throw ();
    virtual void stop() throw ();

public:
    ClientBase(const ClientExternalSettings& config, Log& clientLog, MemoryLog& externalErrorLog_) throw ();
    virtual ~ClientBase() throw ();
};

class ClientPhysicsCallbacks : public PhysicsCallbacksBase {
    ClientBase& c;

public:
    ClientPhysicsCallbacks(ClientBase& c_) throw () : c(c_) { }

    bool collideToRockets() const throw () { return false; }
    bool collidesToRockets(int) const throw () { return false; }
    bool collidesToPlayers(int) const throw () { return true; }
    bool gatherMovementDistance() const throw () { return false; }
    bool allowRoomChange() const throw () { return false; }
    void addMovementDistance(int, double) throw () { }
    void playerScreenChange(int) throw () { }
    void rocketHitWall(int rid, bool power, double x, double y, int roomx, int roomy) throw () { c.rocketHitWallCallback(rid, power, x, y, roomx, roomy); }
    bool rocketHitPlayer(int, int) throw () { return false; }
    void playerHitWall(int pid) throw () { c.playerHitWallCallback(pid); }
    PlayerHitResult playerHitPlayer(int pid1, int pid2, double) throw () { c.playerHitPlayerCallback(pid1, pid2); return PlayerHitResult(false, false, 1., 1.); }
    void rocketOutOfBounds(int rid) throw () { c.rocketOutOfBoundsCallback(rid); }
    bool shouldApplyPhysicsToPlayer(int pid) throw () { return c.shouldApplyPhysicsToPlayerCallback(pid); }
};

class TM_DoDisconnect : public ThreadMessage {
public:
    void execute(ClientBase* cl) const throw () { cl->disconnect_command(); }
};

class TM_Text : public ThreadMessage {
    Message_type type;
    std::string text;
    int team;   // -1 for non-team messages

public:
    TM_Text(Message_type type_, const std::string& text_, int team_ = -1) throw () : type(type_), text(text_), team(team_) { }
    ~TM_Text() throw () { }
    void execute(ClientBase* cl) const throw () {
        cl->print_message(type, text, team);
    }
};

class TM_Sound : public ThreadMessage {
    int sample;

public:
    TM_Sound(int sample_) throw () : sample(sample_) { }
    void execute(ClientBase* cl) const throw () {
        cl->play_sound(sample);
    }
};

#ifndef DEDICATED_SERVER_ONLY
class TM_GunexploEffect : public ThreadMessage {
    int team;
    WorldCoords pos;
    double time;

public:
    TM_GunexploEffect(int team_, double time_, const WorldCoords& pos_) throw () : team(team_), pos(pos_), time(time_) { }
    void execute(ClientBase* cl) const throw () {
        cl->createGunexploEffect(pos, team, time);
    }
};

class GuiClient;

class TM_ServerSettings : public ThreadMessage { // implementation in guiclient.cpp
    uint8_t caplimit, timelimit, extratime, extratime_periods;
    uint16_t misc1, pupMin, pupMax, pupAddTime, pupMaxTime;
    int flag_return_delay;

    void addLine(GuiClient* cl, const std::string& caption, const std::string& value) const throw ();

public:
    TM_ServerSettings(uint8_t caplimit_, uint8_t timelimit_, uint8_t extratime_, uint8_t extratime_periods_, uint16_t misc1_,
                      uint16_t pupMin_, uint16_t pupMax_, uint16_t pupAddTime_, uint16_t pupMaxTime_, int flag_return_delay_) throw () :
        caplimit(caplimit_), timelimit(timelimit_), extratime(extratime_), extratime_periods(extratime_periods_), misc1(misc1_),
        pupMin(pupMin_), pupMax(pupMax_), pupAddTime(pupAddTime_), pupMaxTime(pupMaxTime_), flag_return_delay(flag_return_delay_) { }
    void execute(ClientBase* cl) const throw ();
};

class GuiClient : private ClientBase, public ClientInterface {
    friend class TM_ServerSettings;

    static const int disappearedFlagAlpha = 150;

    ServerThreadOwner listenServer;

    bool mapWrapsX, mapWrapsY;

    Mutex downloadMutex;
    std::list<FileDownload> downloads;

    RankingPasswordManager rankingPassword;
    uint32_t fdp;
    uint32_t max_world_rank;

    Mutex mapInfoMutex;
    std::vector<MapInfo> maps;
    std::vector< std::pair<const MapInfo*, int> > sortedMaps;

    MapListSortKey mapListSortKey;
    bool mapListChangedAfterSort;

    std::set<std::string> fav_maps;
    int current_map;
    int map_vote;
    bool want_change_teams;
    bool want_map_exit;
    bool want_map_exit_delayed;

    // GUI
    Menu_main menu;
    Menu_text m_connectProgress;
    Menu_text m_dialog; // take care not to open multiple dialogs (same goes to other menus too); to allow that, a vector of Menu_text should be created
    Menu_text m_errors;
    Menu_playerPassword m_playerPassword;
    Menu_serverPassword m_serverPassword;
    Menu_text m_serverInfo;
    Menu_text m_notResponding; // not to be put to openMenus

    Menu_language m_initialLanguage; // only for setting the language at startup

    MenuStack openMenus;

    Menu_selection menusel; // a special screen rather than menu: maplist, stats
    bool stats_autoshowing;

    bool quitCommand;

    std::string hostname;
    int red_final_score, blue_final_score;

    std::string edit_map_vote;
    int player_stats_page;
    double lastAltEnterTime;
    bool deadAfterHighlighted;

    double FPS;
    int framecount, totalframecount;
    double frameCountStartTime;

    std::vector<ServerListEntry> gamespy;
    std::vector<ServerListEntry> mgamespy;  //gamespy of master server
    Mutex serverListMutex;

    RegisterMouseClicks mouseClicked;

    enum RefreshStatus { RS_none, RS_running, RS_failed, RS_contacting, RS_connecting, RS_receiving };
    volatile RefreshStatus refreshStatus;   // thread communication variable

    std::string password_file;

    std::string talkbuffer;
    int talkbuffer_cursor;
    std::list<Message> chatbuffer;
    bool show_all_messages;
    std::vector<std::string> highlight_text;

    Graphics graphics;
    bool screenshot;
    std::ifstream replay;
    double replay_rate, replayTime, replaySubFrame;
    bool replay_paused;
    bool replay_stopped;
    bool replay_first_frame_loaded;
    unsigned replay_start_frame;
    unsigned replay_length;
    std::pair<int, int> replayTopLeftRoom;
    double visible_rooms;

    Network::TCPSocket spectate_socket;
    bool spectate_data_received;
    std::stringstream spectate_buffer;

    Sounds client_sounds;

    std::pair<int, int> predrawnRoom;
    int predrawnVisibleRooms;
    bool predrawnWithScroll;

    std::ofstream message_log;
    bool messageLogOpen;

    ServerExternalSettings serverExtConfig;

    class GFXMode {
    public:
        int width, height, depth;
        bool windowed, flipping;

        GFXMode() throw () : width(-1) { }
        GFXMode(int w, int h, int d, bool win, int flip) throw () : width(w), height(h), depth(d), windowed(win), flipping(flip) { }
        bool used() const throw () { return width != -1; }
    };

    GFXMode workingGfxMode;

    class SettingManager : public SettingCollector {
    public:
        typedef std::map<ClientCfgSetting, SaverLoader*> MapT;

        ~SettingManager() throw () { for (MapT::iterator i = settings.begin(); i != settings.end(); ++i) delete i->second; }

        void add(ClientCfgSetting key, SaverLoader* sl) throw () { settings[key] = sl; }

        SettingCollector::SaverLoader* find(ClientCfgSetting key) const throw () { MapT::const_iterator i = settings.find(key); return i == settings.end() ? 0 : i->second; }
        const MapT& read() const throw () { return settings; }

    private:
        MapT settings;
    };
    SettingManager settings;

    std::vector<std::vector<std::string> > load_all_player_passwords() const throw ();
    std::string load_player_password(const std::string& name, const std::string& address) const throw ();
    void save_player_password(const std::string& name, const std::string& address, const std::string& password) const throw ();
    void remove_player_password(const std::string& name, const std::string& address) const throw ();
    int remove_player_passwords(const std::string& name) const throw ();

    void initMenus() throw ();

    // menu callback functions
    void MCF_menuOpener(Menu& menu) throw ();
    void MCF_menuCloser() throw ();
    void MCF_connect(Textarea& target) throw ();
    void MCF_cancelConnect() throw ();
    void MCF_disconnect() throw ();
    void MCF_exitOutgun() throw ();
    void MCF_replay(Textarea& target) throw ();
    void MCF_prepareReplayMenu() throw ();
    void MCF_prepareMainMenu() throw ();
    void MCF_preparePlayerMenu() throw ();
    void MCF_prepareDrawPlayerMenu() throw ();
    void MCF_playerMenuClose() throw ();
    void MCF_nameChange() throw ();  // only function to clear the password
    void MCF_randomName() throw ();
    void MCF_removePasswords() throw ();
    void MCF_prepareGameMenu() throw ();
    void MCF_prepareControlsMenu() throw ();
    void MCF_keyboardLayout() throw ();
    void MCF_joystick() throw ();
    void MCF_messageLogging() throw ();
    void MCF_screenDepthChange() throw ();
    void MCF_screenModeChange() throw ();
    void MCF_gfxThemeChange() throw ();
    void MCF_fontChange() throw ();
    void MCF_visibleRoomsPlayChange() throw ();
    void MCF_visibleRoomsReplayChange() throw ();
    void MCF_antialiasChange() throw ();
    void MCF_transpChange() throw ();
    void MCF_statsBgChange() throw ();
    void MCF_prepareScrModeMenu() throw ();
    void MCF_prepareDrawScrModeMenu() throw ();
    void MCF_prepareGfxThemeMenu() throw ();
    void MCF_sndEnableChange() throw ();
    void MCF_sndVolumeChange() throw ();
    void MCF_sndThemeChange() throw ();
    void MCF_prepareSndMenu() throw ();
    void MCF_refreshLanguages() throw ();
    void MCF_acceptLanguage(Textarea& target) throw ();
    void MCF_acceptInitialLanguage(Textarea& target) throw ();
    void MCF_acceptBugReporting() throw ();
    void MCF_prepareServerMenu() throw ();
    void MCF_updateServers() throw ();
    void MCF_refreshServers() throw ();
    void MCF_prepareAddServer() throw ();
    void MCF_addServer() throw ();
    bool MCF_addressEntryKeyHandler(char scan, unsigned char chr) throw ();
    bool MCF_addRemoveServer(Textarea& target, char scan, unsigned char chr) throw ();
    bool MCF_spectateEntryKeyHandler(char scan, unsigned char chr) throw ();
    void MCF_playerPasswordAccept() throw ();
    void MCF_serverPasswordAccept() throw ();
    void MCF_clearErrors() throw ();
    void MCF_prepareOwnServerMenu() throw ();
    void MCF_startServer() throw ();
    void MCF_playServer() throw ();
    void MCF_stopServer() throw ();

    int refreshLanguages(Menu_language& lang_menu) throw ();
    void acceptLanguage(const std::string& lang, bool restart_message) throw ();

    void load_highlight_texts() throw ();
    void load_fav_maps() throw ();
    void apply_fav_maps() throw ();

    void loadHelp() throw ();
    void addSplashLine(std::string line) throw (); // internal to loadSplashScreen
    void loadSplashScreen() throw ();
    void openMessageLog() throw ();
    void closeMessageLog() throw ();
    void CB_rankingToken(std::string token) throw (); // callback called by rankingPassword from another thread

    bool screenModeChange() throw ();    // the return value should be tested at the first call

    // network
    void connect_command(bool loadPassword) throw ();    // call with frameMutex locked

    void client_connected(ConstDataBlockRef data) throw ();    // call with frameMutex locked
    void client_disconnected(ConstDataBlockRef data) throw ();
    void connect_failed_denied(ConstDataBlockRef data) throw ();
    void connect_failed_unreachable() throw ();
    void connect_failed_socket() throw ();

    void send_player_token() throw ();
    void send_ranking_participation() throw ();
    void sendFavoriteColors() throw ();
    void sendMinimapBandwidth() throw ();

    void change_name_command() throw ();

    void send_chat(const std::string& msg) throw ();
    void send_frame(bool newFrame, bool forceSend) throw ();

    void process_replay_packet(ConstDataBlockRef data) throw ();
    int process_replay_frame_data(ConstDataBlockRef data) throw (); // returns number of bytes read - not necessarily all of data

    std::string refreshStatusAsString() const throw ();
    void getServerListThread() throw ();
    void refreshThread() throw ();
    bool refresh_all_servers() throw ();
    bool getServerList() throw ();
    bool get_local_servers() throw ();
    bool parseServerList(std::istream& response) throw ();

    void check_download() throw ();  // call with downloadMutex locked
    void process_udp_download_chunk(ConstDataBlockRef, bool last) throw ();
    void download_server_file(const std::string& type, const std::string& name) throw ();

    void processNameAuthorizationRequest() throw ();

    WorldCoords playerPos(int pid) const throw ();
    WorldCoords viewTopLeft() const throw ();
    std::pair<int, int> topLeftRoom() const throw ();

    // GUI
    void erase_first_message() throw ();
    void print_message(Message_type type, const std::string& msg, int sender_team = -1) throw ();

    void save_screenshot() throw ();
    void toggle_help() throw ();
    template<class MenuT> void showMenu(MenuT& menu) throw () { openMenus.open(&menu.menu); }
    void predraw() throw ();

    bool on_screen(int x, int y) const throw (); // returns true if any part of room (x,y) is on screen
    bool on_screen(int rx, int ry, double lx, double ly, double fudge = 0) const throw (); // coordinates within "fudge" local units from screen border are also considered on screen
    bool on_screen_exact(int x, int y) const throw ();
    bool on_screen_exact(int rx, int ry, double lx, double ly, double fudge = 0) const throw ();
    bool player_on_screen(int pid) const throw ();
    bool player_on_screen_exact(int pid) const throw ();

    typedef Graphics::VisibilityMap VisibilityMap;

    void draw_game_frame() throw ();
    void draw_map(const VisibilityMap& roomVis) throw ();
    void draw_playfield() throw ();
    VisibilityMap calculateVisibilities() throw (); // calculates how well each player's position is known (applies to out-of-screen players only) and returns a map of how well each room is known (according to the most visible player there)
    int calculatePlayerAlpha(int pid) const throw ();
    void draw_player(int pid, double time, bool live) throw ();
    void draw_game_menu() throw ();

    ClientControls readControls(bool canUseKeypad, bool useCursorKeys) const throw ();
    ClientControls readControlsInGame() const throw ();
    bool firePressed() throw ();
    void refreshGunDir() throw ();

    void handleKeypress(int sc, int ch, bool withControl, bool alt_sequence) throw ();   // sc = scancode, ch = character, as returned by readkey
    bool handleInfoScreenKeypress(int sc, int ch, bool withControl, bool alt_sequence) throw ();  // sc = scancode, ch = character, as returned by readkey
    void handleGameKeypress(int sc, int ch, bool withControl, bool alt_sequence) throw ();   // sc = scancode, ch = character, as returned by readkey

    void play_sound(int sample) throw ();

    void start_replay(const std::string& filename) throw ();
    bool start_replay(std::istream& in) throw ();
    void continue_replay() throw ();
    void continue_replay(std::istream& in) throw ();
    void stop_replay() throw ();
    void start_spectating(const std::string& host) throw ();
    void start_spectating(const Network::Address& address) throw ();
    void continue_spectating() throw ();

    void createGunexploEffect(const WorldCoords& pos, int team, double time) throw ();

    void netRocketFired(int rpx, int rpy, int rx, int ry, bool power) throw ();
    void netRocketHitPlayer(int rockid, int rokx, int roky, double time) throw ();
    void netPowerCollision(int target, double time) throw ();
    void net_data_sound(BinaryReader& read) throw ();
    void net_data_registration_response(BinaryReader& read) throw ();
    void net_data_map_list(BinaryReader& read) throw ();
    void net_data_crap_update(BinaryReader& read) throw ();
    void net_data_reset_map_list(BinaryReader& read) throw ();
    void net_data_map_vote(BinaryReader& read) throw ();
    void net_data_map_votes_update(BinaryReader& read) throw ();
    void net_data_text_message(BinaryReader& read) throw ();

    void netKill(int attacker, int target, DamageType cause, bool carrier_defended, bool flag_defended, bool flag, bool wild_flag, bool spree_ended, bool spree_started) throw ();
    void netSuicide(int pid, bool flag, bool wild_flag, bool spree_ended) throw ();
    void netFlagTake(int pid, bool wild_flag) throw ();
    void netFlagReturn(int pid) throw ();
    void netFlagDrop(int pid, bool wild_flag) throw ();
    void netTeamChange(int pl1, int pl2 = -1) throw ();
    void netStatsReady() throw ();
    void netMapChange(const std::string& maptitle, const int map_number, const int total_maps) throw ();
    void netGameoverPeriodStart(uint32_t redScore, uint32_t blueScore, int caplimit, int timelimit) throw ();
    void netGameoverPeriodEnd() throw ();
    void netGameStarted() throw ();
    void netPhysicsChanged() throw ();
    void netSetHostname(const std::string& name) throw () { hostname = name; }
    void netSetCurrentMap(int idx) throw () { current_map = idx; }

    void rocketHitWallCallback(int rid, bool power, double x, double y, int roomx, int roomy) throw ();
    void playerHitWallCallback(int pid) throw ();
    void playerHitPlayerCallback(int pid1, int pid2) throw ();

    std::string getPlayerPassword() const throw () { return m_playerPassword.password(); }

    class ConstDisappearedFlagIterator : public ConstFlagIterator {
        const GuiClient& c;

        void findValid() throw ();

    public:
        ConstDisappearedFlagIterator(const GuiClient* host) throw () : ConstFlagIterator(host->fx), c(*host) { findValid(); }
        ConstDisappearedFlagIterator& operator++() throw () { next(); findValid(); return *this; }
    };

public:
    GuiClient(const ClientExternalSettings& config, const ServerExternalSettings& serverConfig, Log& clientLog, MemoryLog& externalErrorLog_) throw ();
    ~GuiClient() throw ();

    bool start() throw ();
    void stop() throw ();
    void loop(volatile bool* quitFlag, bool firstTimeSplash) throw ();
    void language_selection_start(volatile bool* quitFlag) throw ();
};
#endif

#endif
