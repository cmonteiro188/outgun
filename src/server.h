#ifndef SERVER_H_INC
#define SERVER_H_INC

#define SV_NAME_AUTHORIZATION   // enable player IP based filtering : name authorization and ban
#define SV_NO_PUP_SWITCHING // disable the changing of power-ups lying on the ground
#define SV_VOTE_ANNOUNCE_INTERVAL 5 // in seconds, how often a changing voting status will be announced
#define SV_SHADOW_MINIMUM_NORMAL 7  // the shadow visibility factor

#ifdef SV_NAME_AUTHORIZATION
#include "nameauth.h"
#endif

#include "world.h"
#include "servnet.h"

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
		token[0]=0;
		intoken=666;
	}
};

class gameserver_c {
	// pelaajien hallinta
	vector<string>	welcome_message;	// welcome message line by line
	vector<string>	info_message;	// the message /info shows, line by line
	string			sayadmin_comment;
	bool			sayadmin_enabled;
	double			team_smul[2];
	NLulong 		next_vote_announce_frame;
	int				last_vote_announce_votes, last_vote_announce_needed;
	ClientData		client[MAX_PLAYERS];

	// pelimaailma
	ServerWorld		world;
	bool			gameover;
	double			gameover_time;		//timeout for gameover plaque

	// verkko
	ServerNetworking network;

	// asetukset
	PowerupSettings pupConfig;
	WorldSettings worldConfig;

public:
	struct MapInfo {
		string title, author, file;
		int width, height;
		int votes;
		bool votes_changed;
		MapInfo();
		bool load(string mapName);
	};

private:
	vector<MapInfo> maprot;
	int currmap;		// current map in maprot
	bool random_maprot;
	#ifdef SV_NAME_AUTHORIZATION
	NameAuthorizationDatabase authorizations;
	#endif
	int vote_block_time;	// how long a mapchange can't be voted (except unanimously), in frames (in gamemod, it is minutes)

public:

	gameserver_c();
	virtual ~gameserver_c();
	bool start(int target_maxplayers);
	void loop(volatile bool *running_flag);
	void stop();

	void ctf_game_restart();
	void simulate_and_broadcast_frame();
	void server_think_after_broadcast();

	// pelaajien hallinta
	void mutePlayer(int pid, int mode);
	void kickPlayer(int pid, bool ban=false);
	#ifdef SV_NAME_AUTHORIZATION
	void banPlayer(int pid);
	bool isBanned(int cid) { return authorizations.isBanned(network.get_client_address(cid)); }
	#endif

   int check[MAX_PLAYERS];
   int checount;
	void check_team_changes();
	void check_player_change_teams(int pid);
	void move_player(int f, int t);
	void swap_players(int a, int b);
	void game_remove_player(int pid);

	void nameChange(int id, int pid, const string& tempname);
	void chat(int id, int pid, const char* sbuf);	//#fix: separate console handling

	const ClientData& getClientData(int cid) const { return client[cid]; }
	      ClientData& getClientData(int cid)       { return client[cid]; }
	bool changeRegistration(int id, const string& token);	// returns true if the token is different from before
	void resetPlayer(int cid) { client[cid].reset(); }
	void clearWorldRankingDeltas();
	void refresh_team_score_modifiers();
	void check_map_exit();
	void score_frag(int p, int amount);
	void score_neg(int p, int amount);
	int getLessScoredTeam() const;	// using team_smul ; call refresh_team_score_modifiers before calling this

	bool load_rotation_map(int pos);
	bool server_next_map(int reason);
	const MapInfo& current_map() const { return maprot[currmap]; }
	int current_map_nr() const { return currmap; }
	const string& getCurrentMapFile() const { return maprot[currmap].file; }
	const vector<MapInfo>& maplist() const { return maprot; }
	vector<MapInfo>& maplist() { return maprot; }

	const vector<string>& getWelcomeMessage() const { return welcome_message; }

	// asetukset
	void load_game_mod();
	bool reset_settings(bool keepMap);
};

#endif
