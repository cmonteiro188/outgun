#ifndef SERVER_H_INC
#define SERVER_H_INC

#include "gameserver_interface.h"
#include "log.h"
#include "nameauth.h"
#include "servnet.h"
#include "utility.h"
#include "world.h"

//per-client struct (statically allocated to a single client)
class ClientData {
public:
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

	bool		current_participation;
	bool		next_participation;
	bool		participation_info_received;

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
		token[0] = 0;
		intoken = 666;

		current_participation = false;
		next_participation = false;
		participation_info_received = false;
	}
};

class gameserver_c {
	FileLog normalLog;
	SupplementaryLog<MemoryLog> errorLog;
	SupplementaryLog<FileLog> securityLog;
	LogSet log;

	// client control
	std::vector<std::string> welcome_message;	// welcome message line by line
	std::vector<std::string> info_message;		// the message /info shows, line by line
	std::string		sayadmin_comment;
	bool			sayadmin_enabled;
	double			team_smul[2];
	NLulong 		next_vote_announce_frame;
	int				last_vote_announce_votes, last_vote_announce_needed;
	int				idlekick_time;
	int				game_end_delay;
	ClientData		client[MAX_PLAYERS];
	std::vector<bool> fav_colors[2];

	// world
	ServerWorld		world;
	bool			gameover;
	double			gameover_time;		//timeout for gameover plaque
	int				maxplayers;

	// networking
	ServerNetworking network;

	// settings
	bool tournament;
	bool save_stats;
	const ServerExternalSettings extConfig;
	PowerupSettings pupConfig;
	WorldSettings worldConfig;

	std::vector<MapInfo> maprot;
	int currmap;		// current map in maprot
	bool random_maprot;
	NameAuthorizationDatabase authorizations;
	int vote_block_time;	// how long a mapchange can't be voted (except unanimously), in frames (in gamemod, it is minutes)
	std::string server_website_url;	// the URL of the server website to be sent to master server

	void setMaxPlayers(int num) { maxplayers = num; world.setMaxPlayers(num); network.setMaxPlayers(num); }

	// copying not allowed
	gameserver_c(const gameserver_c& o);
	gameserver_c& operator=(const gameserver_c& o);

public:

	gameserver_c(LogSet& hostLogs, const ServerExternalSettings& config);
	virtual ~gameserver_c();
	bool start(int target_maxplayers);
	void loop(volatile bool *quitFlag, bool quitOnEsc);
	void stop();
	const ServerExternalSettings& config() const { return extConfig; }

	void ctf_game_restart();
	void simulate_and_broadcast_frame();
	void server_think_after_broadcast();

	int get_player_count() const { return network.get_player_count(); }
	void mutePlayer(int pid, int mode, int admin);
	void kickPlayer(int pid, int admin, int minutes = 0);	// if minutes > 0, it's really a ban
	void banPlayer(int pid, int admin, int minutes);
	bool isBanned(int cid) const { return authorizations.isBanned(network.get_client_address(cid)); }
	bool check_name_password(const std::string& name, const std::string& password) const;
	void disconnectPlayer(int pid, Disconnect_reason reason);
	void sendMessage(int pid, Message_type type, const std::string& msg);

   int check[MAX_PLAYERS];
   int checount;

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
	void chat(int pid, const char* sbuf);	//#fix: separate console handling

	const ClientData& getClientData(int cid) const { return client[cid]; }
	      ClientData& getClientData(int cid)       { return client[cid]; }
	bool changeRegistration(int id, const std::string& token);	// returns true if the token is different from before and non-empty
	void resetPlayer(int cid) { client[cid].reset(); }
	void refresh_team_score_modifiers();
	void check_map_exit();
	void score_frag(int p, int amount);
	void score_neg(int p, int amount);
	int getLessScoredTeam() const;	// using team_smul ; call refresh_team_score_modifiers before calling this
	bool isLocallyAuthorized(int pid) const;
	bool isAdmin(int pid) const;

	bool load_rotation_map(int pos);
	bool server_next_map(int reason);
	const MapInfo& current_map() const { return maprot[currmap]; }
	int current_map_nr() const { return currmap; }
	const std::string& getCurrentMapFile() const { return maprot[currmap].file; }
	const std::vector<MapInfo>& maplist() const { return maprot; }
	std::vector<MapInfo>& maplist() { return maprot; }

	const std::vector<std::string>& getWelcomeMessage() const { return welcome_message; }
	std::string getTeamName(int team) const { return world.getTeamName(team); }

	const std::string& server_website() const { return server_website_url; }

	bool tournament_active() const { return tournament; }

	void load_game_mod();
	bool reset_settings(bool keepMap);
};

#endif
