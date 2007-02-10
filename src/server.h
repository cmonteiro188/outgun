/*
 *  server.h
 *
 *  Copyright (C) 2002 - Fabio Reis Cecin
 *  Copyright (C) 2003, 2004, 2006 - Niko Ritari
 *  Copyright (C) 2003, 2004, 2006, 2007 - Jani Rivinoja
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

#ifndef SERVER_H_INC
#define SERVER_H_INC

#include "gameserver_interface.h"
#include "log.h"
#include "auth.h"
#include "servnet.h"
#include "utility.h"
#include "world.h"

class Client; // bots are Clients
class GamemodSetting;

//per-client struct (statically allocated to a single client)
class ClientData {
public:
    //v0.4.4 PLAYER REGISTRATION STATUS
    bool        token_have;                 //player claims to be registered with (name,token)
    bool        token_valid;        //player (name,token) is validated
    std::string token;                  //the player's token
    int         intoken;                        //integer version of token

    //v0.4.4 client statistics
    int         delta_score;        //the player's score accumulator
    int         neg_delta_score;        //NEG score accum 0.4.8

    double fdp;     //DOUBLE delta accums. os acima sao apenas o "trunc atual"
    double fdn;

    int         rank;                       //current ranking position
    int         score;                  //current score POS -- SOMATORIO (né?!?!?)
    int         neg_score;                  //current score NEG 0.4.8 -- SOMATORIO (né?!?!?)

    bool        current_participation;
    bool        next_participation;
    bool        participation_info_received;

    ClientData() {
        reset();
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
        token.clear();
        intoken = 666;

        current_participation = false;
        next_participation = false;
        participation_info_received = false;
    }
};

class Server {
    FileLog normalLog;
    DualLog errorLog;
    SupplementaryLog<FileLog> securityLog;
    SupplementaryLog<FileLog> adminActionLog;
    mutable LogSet log;

    const bool threadLock;    // if true, all concurrency is eliminated; its benefits are lost but there are many opportunities for bad timing to trigger problems so it's often wise
    MutexHolder threadLockMutex;    // used to implement threadLock, if it is enabled

    bool abortFlag;

    // client control
    double          team_smul[2];
    NLulong         next_vote_announce_frame;
    int             last_vote_announce_votes, last_vote_announce_needed;
    ClientData      client[MAX_PLAYERS];
    std::vector<bool> fav_colors[2];

    Thread          botthread;
    std::vector<Client*> bots;
    int extra_bots;
    volatile bool quit_bots;
    NoLog botNoLog;
    MemoryLog botErrorLog;
    bool check_bots;
    bool bot_ping_changed;

    void init_bots();
    void run_bot_thread();

    // world
    ServerWorld     world;
    bool            gameover;
    double          gameover_time;      //timeout for gameover plaque
    int             maxplayers;

    // networking
    ServerNetworking network;

    class SettingManager : public ServerNetworking::Settings {
    public:
        typedef AuthorizationDatabase::AccessDescriptor::GamemodAccessDescriptor GamemodAccessDescriptor;

    private:
        Server& server;
        // handy aliases to server.*:
        ServerNetworking& network;
        ServerWorld& world;

        const ServerExternalSettings extConfig;
        // values copied from extConfig that may be changed:
        std::string     ipAddress;
        int             port;
        bool            privateserver;

        PowerupSettings pupConfig;
        WorldSettings   worldConfig;
        int             game_end_delay;
        int             vote_block_time;    // how long a mapchange can't be voted (except unanimously), in frames (in gamemod, it is minutes)
        bool            require_specific_map_vote;
        std::vector<std::string> welcome_message;   // welcome message line by line
        std::vector<std::string> info_message;      // the message /info shows, line by line
        std::string     sayadmin_comment;
        bool            sayadmin_enabled;
        int             idlekick_time, idlekick_playerlimit;
        int             min_bots;
        int             bots_fill;
        int             bot_ping;
        bool            balance_bot;
        std::string     bot_name_lang;
        bool            tournament;
        int             save_stats;
        bool            random_maprot;
        bool            random_first_map;
        std::string     server_website_url; // the URL of the server website to be sent to master server
        int             recording;

        int             join_start;         // allow joining from this time of a day (in seconds)
        int             join_end;           // disallow joining; set both same to allow always (default)
        std::string     join_limit_message; // when joining is disallowed, this message is sent to asking clients in addition to information about the open times

        std::vector<std::string> web_servers;
        std::string     web_script, web_auth;
        int             web_refresh;

        std::string     server_password;
        std::string     hostname;

        class DisposerBase {
        public:
            virtual ~DisposerBase() { }
            virtual void dispose() = 0;
        };

        template<class T> class Disposer : public DisposerBase {
            T* ptr;
        public:
            Disposer(T* ptr_) : ptr(ptr_) { }
            void dispose() { delete ptr; }
        };

        std::vector<DisposerBase*> redirectFnDisposers;

        template<class T> T* addFn(T* ptr) { redirectFnDisposers.push_back(new Disposer<T>(ptr)); return ptr; }

        struct Category { // no pointers contained are owned by this object
            const char* identifier;
            const char* descriptiveName;
            std::vector<GamemodSetting*> settings;

            Category(const char* id, const char* name) : identifier(id), descriptiveName(name) { }
            void add(GamemodSetting* setting) { settings.push_back(setting); }
        };
        std::vector<Category> categories;
        bool built, builtForReload;

        static bool checkMaxplayerSetting(int val) { return (val >= 2 && val <= MAX_PLAYERS && val % 2 == 0); }
        static bool checkForceIpValue(const std::string& val);
        static std::string returnEmptyString() { return std::string(); }
        bool trySetMaxplayers(int val);
        bool setForceIP(const std::string& val);
        void setRandomMaprot(int val);
        const std::string& getForceIP() const;
        int getMaxplayers() const;
        int getRandomMaprot() const;

        void free();
        void build(bool reload);
        void commit(bool reload);
        void processLine(const std::string& line, LogSet& argLogs, bool allowGet, const GamemodAccessDescriptor& access) const;

    public:
        SettingManager(Server& server_, const ServerExternalSettings& extConfig_) : server(server_), network(server_.network), world(server_.world), extConfig(extConfig_), built(false) { }
        ~SettingManager() { free(); }

        std::vector<std::string> listSettings(const GamemodAccessDescriptor& access);
        std::vector<std::string> executeLine(const std::string& line, const GamemodAccessDescriptor& access);
        void loadGamemod(bool reload);

        bool isGamemodCommand(const std::string& cmd, bool includeCategories); // can't be const because might need to build()

        bool isGamemodCommand(const std::string& cmd) { return isGamemodCommand(cmd, false); }
        bool isGamemodCommandOrCategory(const std::string& cmd) { return isGamemodCommand(cmd, true); }

        void reset();

        void set_min_bots(int val) { min_bots = val; }
        void set_bots_fill(int val) { bots_fill = val; }
        void set_bot_ping(int val) { bot_ping = val; }
        void set_balance_bot(bool val) { balance_bot = val; }

        bool ownScreen() const { return extConfig.ownScreen; }
        ServerExternalSettings::StatusOutputFnT* statusOutput() const { return extConfig.statusOutput; }
        bool showErrorCount() const { return extConfig.showErrorCount; }
        int lowerPriority() const { return extConfig.lowerPriority; }
        int networkPriority() const { return extConfig.networkPriority; }
        int minLocalPort() const { return extConfig.minLocalPort; }
        int maxLocalPort() const { return extConfig.maxLocalPort; }
        bool dedicated() const { return extConfig.dedserver; }

        bool privateServer() const { return privateserver; }
        const std::string& ip() const { return ipAddress; }
        int get_port() const { return port; }

        int  get_game_end_delay() const { return game_end_delay; }
        int  get_vote_block_time() const { return vote_block_time; }
        bool get_require_specific_map_vote() const { return require_specific_map_vote; }

        const std::vector<std::string>& get_welcome_message() const { return welcome_message; }
        const std::vector<std::string>& get_info_message() const { return info_message; }
        const std::string& get_sayadmin_comment() const { return sayadmin_comment; }
        bool get_sayadmin_enabled() const { return sayadmin_enabled; }

        int  get_idlekick_time() const { return idlekick_time; }
        int  get_idlekick_playerlimit() const { return idlekick_playerlimit; }

        int  get_min_bots() const { return min_bots; }
        int  get_bots_fill() const { return bots_fill; }
        int  get_bot_ping() const { return bot_ping; }
        bool get_balance_bot() const { return balance_bot; }
        const std::string& get_bot_name_lang() const { return bot_name_lang; }

        bool get_tournament() const { return tournament; }
        int  get_save_stats() const { return save_stats; }

        bool get_random_maprot() const { return random_maprot; }
        bool get_random_first_map() const { return random_first_map; }

        const std::string& get_server_website_url() const { return server_website_url; }

        int  get_recording() const { return recording; }

        int get_join_start() const { return join_start; }
        int get_join_end() const { return join_end; }
        const std::string& get_join_limit_message() const { return join_limit_message; }

        const std::vector<std::string>& get_web_servers() const { return web_servers; }
        const std::string& get_web_script() const { return web_script; }
        const std::string& get_web_auth() const { return web_auth; }
        int get_web_refresh() const { return web_refresh; }

        const std::string& get_hostname() const { return hostname; }
        const std::string& get_server_password() const { return server_password; }
    };

    SettingManager settings;

    std::vector<MapInfo> maprot;
    int currmap;        // current map in maprot
    AuthorizationDatabase authorizations;

    // recording
    bool recording_started;
    std::string record_filename;
    mutable std::ofstream record;
    mutable std::ostringstream record_frame;
    NLulong record_start_frame;
    std::string record_map;

    bool loadAuthorizations();
    void saveAuthorizations() const;

    AuthorizationDatabase::AccessDescriptor getAccess(int pid);

    void doKickPlayer(int pid, int admin, int minutes);   // if minutes > 0, it's really a ban

    bool trySetMaxplayers(int val); // checks that no players are connected, if that fails, logs an error and returns false
    void setMaxPlayers(int num) { maxplayers = num; world.setMaxPlayers(num); network.setMaxPlayers(num); }

    // copying not allowed
    Server(const Server& o);
    Server& operator=(const Server& o);

    void start_recording();
    void stop_recording();
    void clear_recording();
    void record_init_data();

public:
    Server(LogSet& hostLogs, const ServerExternalSettings& config, Log& externalErrorLog, const std::string& errorPrefix);  // externalErrorLog must outlive the Server object
    virtual ~Server();

    bool start(int target_maxplayers);
    void loop(volatile bool *quitFlag, bool quitOnEsc);
    void stop();

    void ctf_game_restart();
    void simulate_and_broadcast_frame();
    void server_think_after_broadcast();

    int get_player_count() const { return network.get_player_count(); }
    void mutePlayer(int pid, int mode, int admin);
    void kickPlayer(int pid, int admin);
    void banPlayer(int pid, int admin, int minutes);
    bool isBanned(int cid) const { return authorizations.isBanned(network.get_client_address(cid)); }
    bool check_name_password(const std::string& name, const std::string& password) const;
    void disconnectPlayer(int pid, Disconnect_reason reason);
    void sendMessage(int pid, Message_type type, const std::string& msg);

    void remove_bot();
    void set_check_bots() { check_bots = true; }

    void logAdminAction(int admin, const std::string& action, int target = pid_none);

    void balance_teams();
    void shuffle_teams();
    void check_team_changes();
    void check_player_change_teams(int pid);
    void move_player(int f, int t);
    void move_player_inside_team(int source, int target);
    void swap_players(int a, int b);
    void game_remove_player(int pid, bool removeClient);
    void check_fav_colors(int pid);
    void set_fav_colors(int pid, const std::vector<char>& colors);

    void nameChange(int id, int pid, const std::string& tempname, const std::string& password);
    void chat(int pid, const std::string& sbuf);   //#fix: separate console handling

    const ClientData& getClientData(int cid) const { return client[cid]; }
          ClientData& getClientData(int cid)       { return client[cid]; }
    bool changeRegistration(int id, const std::string& token);  // returns true if the token is different from before and non-empty
    void resetClient(int cid) { client[cid].reset(); }
    void refresh_team_score_modifiers();
    void check_map_exit();
    bool specific_map_vote_required() const { return settings.get_require_specific_map_vote(); } //#fix
    void score_frag(int p, int amount);
    void score_neg(int p, int amount);
    int getLessScoredTeam() const;  // using team_smul ; call refresh_team_score_modifiers before calling this
    bool isLocallyAuthorized(int pid) const;
    bool isAdmin(int pid) const;

    bool load_rotation_map(int pos);
    bool server_next_map(int reason);
    const MapInfo& current_map() const { return maprot[currmap]; }
    int current_map_nr() const { return currmap; }
    const std::string& getCurrentMapFile() const { return maprot[currmap].file; }
    const std::vector<MapInfo>& maplist() const { return maprot; }
    std::vector<MapInfo>& maplist() { return maprot; }

    const std::vector<std::string>& getWelcomeMessage() const { return settings.get_welcome_message(); } //#fix?

    const std::string& server_website() const { return settings.get_server_website_url(); } //#fix?

    bool tournament_active() const { return settings.get_tournament(); }

    bool reset_settings(bool reload);   // set reload if reset_settings has already been called to preserve map and ensure fixed values aren't changed

    bool recording_needed() const;
    std::ostream& record_stream() const { return record_frame; }
    const std::string& record_map_data() const { return record_map; }
};

#endif
