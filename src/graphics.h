#ifndef GRAPHICS_H_INC
#define GRAPHICS_H_INC

#include <string>
#include "world.h"
#include "sounds.h"

// ---- client screen layout ----

//resolution
#define RESOL_X 640
#define RESOL_Y 480

//play area offset
#define plx 0
#define ply 90

//minimap offset
#define mmx (plx + plw + 16)    //push 8 to left
#define mmy ply

//scoreboard offset
#define sbx (plx + plw)
#define sby (mmy + 110)         // + XXX = minimap panel height

class Graphics {
	BITMAP* drawbuf;
	BITMAP* minibg;
	//int plw, plh;
	//int plx, ply;

	BITMAP* flagpos_buf[2];
	static const int flagpos_radius = 30;
	bool flagpos_ready;

	// drawing screens
	BITMAP* vidpage1;
	BITMAP* vidpage2;
	BITMAP* backbuf;
	bool page_flipping;

	//colors
	enum {
		//player's colors
		COLGREEN,
		COLYELLOW,
		COLWHITE,
		COLMAG,
		COLCYAN,
		COLORA,
		COLLRED,		// light red
		COLLBLUE,		// light blue
		//MORE player colors
		COL9,
		COL10,
		COL11,
		COL12,
		COL13,
		COL14,
		COL15,
		COL16,

		//team colors
		COLRED,			//team 1 (color 0)
		COLBLUE,		//team 2 (color 1)

		//base colors
		COLBRED,			//team 1 (color 0)
		COLBBLUE,		//team 2 (color 1)

		//other
		COLFOGOFWAR,
		COLMENUWHITE,
		COLMENUBLACK,
		COLMENUGRAY,
		COLGROUND,
		COLWALL,
		COLNOLIFE,
		COLDARKGRAY,
		COLSHADOW,
		COLLIMBO,
		COLDARKORA,
		COLINFO,
		COLENER3,
		COLGROUND_DEF,
		COLWALL_DEF,
		NUM_OF_COL
	};

	int teamcol[2];
	int teamlcol[2];	//light colours for statusbar
	int teamdcol[2];	//dark colours for player name

	int	col[NUM_OF_COL];

	void build_flagpos_marks();
	void update_minimap_background(BITMAP* buffer, const Map& map, bool flagPaintSimple);

	void server_list(const std::vector<gamespy_t>& servers, int selection, bool showmaster);
	void menu_caption();

public:
	Graphics(int scr_w = RESOL_X, int scr_h = RESOL_Y);
	~Graphics();
	
	BITMAP* drawbuffer() const { return drawbuf; }

	void draw_screen() const;
	bool save_screenshot(const string& filename) const;

	bool reset_video_mode();
	void clear();

	void setcolors();
	void reset_playground_colors();
	void random_playground_colors();

	void draw_background();

	void draw_playground();
	void draw_empty_playground();

	void draw_walls(const Room& room);

	void draw_flag(int team, int x, int y);
	void draw_flagpos_mark(int team, int flag_x, int flag_y);
	void draw_mini_flag(int team, const ctflag_t& flag, const Map& map);
	void draw_minimap_background();
	void update_minimap_background(const Map& map);
	void draw_minimap_player(int x, int y, int team, int player);
	void draw_minimap_me(int x, int y, int team, double time);
	void draw_minimap_room(int x1, int y1, int x2, int y2);

	void draw_player(int x, int y, int team, int pli, int gundir, double hitfx, bool power, int alpha, double time);
	void draw_player_shadow(const ClientPlayer& player, int alpha);
	void draw_player_name(const std::string& name, int x, int y, int c);
	void draw_rocket(const rocket_c& rocket, double time);
	void draw_gun_explosion(int x, int y, int rad);
	void draw_deathbringer_smoke(int x, int y, double time);
	void draw_deathbringer(int x, int y, int team, double time);

	void draw_player_health(int health);
	void draw_player_energy(int energy);

	void draw_deathbringer_affected(int x, int y, int team);
	void draw_deathbringer_carrier_effect(int x, int y);
	void draw_shield(int x, int y, int r, int alpha = 255);

	void draw_player_dead(int x, int y);

	void draw_virou_sorvete(int x, int y);

	void draw_one_line_message(const std::string& message);
	void draw_waiting_map_message(const std::string& caption, const std::string& map);
	void show_not_responding_message();
	void draw_scores(const std::string& text, int col, int score1, int score2);
	void print_chat_message(int line, const string& message, MESSAGE_TYPE type);
	void print_chat_input(int line, const std::string& message);

	void draw_scoreboard_caption(int team, const std::string& caption);
	void draw_scoreboard_name(int y, int pcol, const ClientPlayer& player);
	void draw_scoreboard_points(int y, int team, int points);

	void draw_player_power(double val);
	void draw_player_turbo(double val);
	void draw_player_shadow(double val);
	void draw_player_weapon(int level);
	void draw_fps(double fps);
	void draw_change_team_message(double time);
	void draw_change_map_message(double time);

	// draw power-ups
	void draw_pup(const pickup_c& pup, double time);
	void draw_pup_shield(int x, int y);
	void draw_pup_turbo(int x, int y);
	void draw_pup_shadow(int x, int y, double time);
	void draw_pup_power(int x, int y, double time);
	void draw_pup_weapon(int x, int y, double time);
	void draw_pup_health(int x, int y, double time);
	void draw_pup_deathbringer(int x, int y);

	// menus
	void main_menu(bool connected, const std::string& address, const std::string& playername, const std::string& namestatus,
				   bool listen_server_running, int listen_port_running, const Sounds& sounds);
	void public_servers(const std::vector<gamespy_t>& servers, int selection);
	void favourite_servers(const std::vector<gamespy_t>& servers, int selection);
	void name_password_menu(const std::string& name, int password_len, bool name_selected, const std::string& namestatus);
	void game_help();
	void show_progress(const std::string& t1, const std::string& t2, const std::string& t3, int fg = -1, int bg = 0);
	void dialog(const std::string& t1, const std::string& t2);
};

#endif // GRAPHICS_H_INC

