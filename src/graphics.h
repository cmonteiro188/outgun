#ifndef GRAPHICS_H_INC
#define GRAPHICS_H_INC

#include <string>
#include <list>

// ---- client screen layout ----

//resolution
#define RESOL_X 640
#define RESOL_Y 480

//play area offset
//#define plx 0
//#define ply 90

//minimap offset
//#define mmx (plx + plw + 4)
//#define mmy ply

//scoreboard offset
//#define sbx (plx + plw)
//#define sby (mmy + 110)         // + XXX = minimap panel height

class Map;
class MapInfo;
class RectWall;
class TriWall;
class CircWall;
class ClientPlayer;
class Sounds;
class gamespy_t;
class clientfx_t;
class Team;
class Message;

class Graphics {
public:
	Graphics(int scr_w = RESOL_X, int scr_h = RESOL_Y, bool reset_video = true);
	~Graphics();
	
	BITMAP* drawbuffer() const { return drawbuf; }
	
	int minimap_width() const { return minimap_w; }
	int minimap_height() const { return minimap_h; }

	void draw_screen() const;
	bool save_screenshot(const std::string& filename) const;

	bool reset_video_mode();
	void clear();

	void setcolors();
	void reset_playground_colors();
	void random_playground_colors();

	void predraw(const Room& room, const std::vector< std::pair<int, const spoint_t*> >& flags);

	void draw_background();
	void draw_empty_background();

	void predraw_room_ground(const Room& room);
	void predraw_room_walls(const Room& room);

	void draw_flag(int team, int x, int y);
	void draw_flagpos_mark(int team, int flag_x, int flag_y);
	void draw_mini_flag(int team, const Flag& flag, const Map& map);
	void draw_minimap_background();
	void update_minimap_background(const Map& map);
	void draw_minimap_player(const Map& map, const ClientPlayer& player, int team, int pc);
	void draw_minimap_me(const Map& map, const ClientPlayer& player, int team, double time);
	void draw_minimap_room(const Map& map, int rx, int ry);

	void draw_player(int x, int y, int team, int pli, int gundir, double hitfx, bool power, int alpha, double time);
	void draw_player_shadow(const ClientPlayer& player, int alpha);
	void draw_player_name(const std::string& name, int x, int y, int team);
	void draw_player_dead(int x, int y);

	void draw_rocket(const rocket_c& rocket, double time);
	void draw_gun_explosion(int x, int y, int rad);
	void draw_deathbringer_smoke(int x, int y, double time);
	void draw_deathbringer(int x, int y, int team, double time);

	void draw_player_health(int health);
	void draw_player_energy(int energy);

	void draw_deathbringer_affected(int x, int y, int team);
	void draw_deathbringer_carrier_effect(int x, int y);
	void draw_shield(int x, int y, int r, int alpha = 255, int team = -1);

	void draw_virou_sorvete(int x, int y);

	void draw_one_line_message(const std::string& message);
	void draw_waiting_map_message(const std::string& caption, const std::string& map);
	void draw_loading_map_message(const std::string& text);
	void show_not_responding_message();
	void draw_scores(const std::string& text, int col, int score1, int score2);
	void print_chat_messages(std::list<Message>::const_iterator begin, const std::list<Message>::const_iterator& end,
							 const std::string& talkbuffer);

	void draw_scoreboard(const std::vector<ClientPlayer*>& players, const Team* teams);

	void team_statistics(const Team* teams);
	void draw_statistics(const std::vector<ClientPlayer>& players, int page, int time);
	void debug_panel(const std::vector<ClientPlayer>& players, int me, int bpsin, int bpsout);

	void map_list(const std::vector<MapInfo>& maps, int current = -1,
				  int own_vote = -1, const std::string& edit_vote = "");

	void map_list_prev();
	void map_list_next();
	void map_list_prev_page();
	void map_list_next_page();
	void map_list_begin();
	void map_list_end();

	void team_captures_prev();
	void team_captures_next();

	void draw_player_power(double val);
	void draw_player_turbo(double val);
	void draw_player_shadow(double val);
	void draw_player_weapon(int level);
	void map_time(int seconds);
	void draw_fps(double fps);
	void draw_change_team_message(double time);
	void draw_change_map_message(double time);

	// power-ups
	void draw_pup(const Powerup& pup, double time);
	void draw_pup_shield(int x, int y);
	void draw_pup_turbo(int x, int y);
	void draw_pup_shadow(int x, int y, double time);
	void draw_pup_power(int x, int y, double time);
	void draw_pup_weapon(int x, int y, double time);
	void draw_pup_health(int x, int y, double time);
	void draw_pup_deathbringer(int x, int y);

	// client side effects
	void draw_effects(int room_x, int room_y, double time);
	void draw_speedfx(int room_x, int room_y, double time);

	void clear_fx();

	void create_wallexplo(int x, int y, int px, int py);
	void create_quadwallexplo(int x, int y, int px, int py);
	void create_deathbringer(int owner, double start_time, int x, int y, int px, int py);
	void create_deathcarrier(int x, int y, int px, int py, int team);
	void create_gunexplo(int x, int y, int px, int py);
	void create_speedfx(int x, int y, int px, int py, int col1, int col2, int gundir);

	// menus
	void main_menu(bool connected, const std::string& address, const std::string& playername, const std::string& namestatus,
				   bool listen_server_running, int listen_port_running, const Sounds& sounds);
	void public_servers(const std::vector<gamespy_t>& servers, int selection);
	void favourite_servers(const std::vector<gamespy_t>& servers, int selection);
	void name_password_menu(const std::string& name, int password_len, bool name_selected, const std::string& namestatus);
	void password_menu(const std::string& caption, int password_len, bool selected = true);
	void password_menu_save(const std::string& caption, int password_len, bool save_password, bool pw_selected);
	void game_help();
	void show_progress(const std::string& t1, const std::string& t2, const std::string& t3, int fg = -1, int bg = 0);
	void dialog(const std::string& t1, const std::string& t2);

	bool save_map_picture(const std::string& filename, const Map& map);

	void search_themes();
	void next_theme();
	void set_theme_dir(const std::string& dir);
	const std::string& theme_dir() const { return themedir; }
	bool basic() const { return no_theme; }

	enum Antialiasing_mode { AA_none, AA_map, AA_both };

	Antialiasing_mode antialiasing_mode() const { return antialiasing; }
	void toggle_antialiasing();
	void set_antialiasing(Antialiasing_mode mode) { antialiasing = mode; }

private:
	void update_minimap_background(BITMAP* buffer, const Map& map, bool save_map_pic = false);

	// texture should really be const BITMAP* but Allegero function needs BITMAP* for some reason
	void draw_room_ground(BITMAP* buffer, const Room& room, float x, float y, float scale, int color, bool texture);
	void draw_room_walls(BITMAP* buffer, const Room& room, float x, float y, float scale, int color, bool texture);

	void draw_rect_wall(BITMAP* buffer, const RectWall& wall, float x0, float y0, float scale, int color, BITMAP* texture);
	void draw_tri_wall(BITMAP* buffer, const TriWall& wall, float x0, float y0, float scale, int color, BITMAP* texture);
	void draw_circ_wall(BITMAP* buffer, const CircWall& wall, float x0, float y0, float scale, int color, BITMAP* texture);

	std::pair<int, int> calculate_minimap_coordinates(const Map& map, const ClientPlayer& player) const;

	void server_list(const std::vector<gamespy_t>& servers, int selection, bool showmaster);
	void menu_caption();

	void draw_player_statistics(const ClientPlayer& player, int team, int x, int y, int page, int time);

	void draw_scoreboard_caption(int team, const std::string& caption);
	void draw_scoreboard_name(const std::string& name, int x, int y, int pcol);
	void draw_scoreboard_points(int points, int x, int y, int team);

	void print_chat_message(Message_type type, const std::string& message, int x, int y);
	void print_chat_input(const std::string& message, int x, int y);

	void print_text_border(const std::string& text, int x, int y, int textcol, int bordercol, int bgcol);
	void print_text_border_centre(const std::string& text, int x, int y, int textcol, int bordercol, int bgcol);

	void print_text_border(const std::string& text, int x, int y, int textcol, int bordercol, int bgcol, bool centring);

	void scrollbar(int x, int y, int height, int bar_y, int bar_h, int col1, int col2);

	std::string make_theme_path(const std::string& theme_dir);
	void load_theme(const std::string& dirname = "");
	void load_pictures();

	void load_floor_textures(const std::string& filename);
	void load_wall_textures(const std::string& filename);
	BITMAP* get_floor_texture(int texid);
	BITMAP* get_wall_texture(int texid);

	void load_player_sprite(const std::string& filename_team, const std::string& filename_personal);
	void create_player_sprite(BITMAP* sprite, BITMAP* team, BITMAP* personal, int tcol, int pcol) const;

	void unload_pictures();
	void unload_floor_textures();
	void unload_wall_textures();
	void unload_player_sprites();

	// scaleable primitive drawing functions
	int scale(double value) const;
	void rectfill_sc(BITMAP* buff, double x1, double y1, double x2, double y2, int color) const;
	void triangle_sc(BITMAP* buff, double x1, double y1, double x2, double y2, double x3, double y3, int color) const;
	void circle_sc(BITMAP* buff, double x, double y, double r, int color) const;
	void circlefill_sc(BITMAP* buff, double x, double y, double r, int color) const;
	void ellipse_sc(BITMAP* buff, double x, double y, double rx, double ry, int color) const;
	void ellipsefill_sc(BITMAP* buff, double x, double y, double rx, double ry, int color) const;
	void putpixel_sc(BITMAP* buff, double x, double y, int color) const;
	void line_sc(BITMAP* buff, double x1, double y1, double x2, double y2, int color) const;
	void hline_sc(BITMAP* buff, double x1, double y, double x2, int color) const;
	void vline_sc(BITMAP* buff, double x, double y1, double y2, int color) const;

	BITMAP* drawbuf;	// main draw buffer
	BITMAP* background;	// draw buffer for floor, walls and minimap
	BITMAP* minibg;		// minimap draw buffer
	//int plw, plh;

	int plx, ply;		// playground position on the screen
	int mmx, mmy;		// minimap position
	int sbx, sby;		// scoreboard position

	int minimap_w, minimap_h;
	int minimap_place_w, minimap_place_h;
	int minimap_start_x, minimap_start_y;

	BITMAP* roombg;		// room background sub-bitmap

	static const int flagpos_radius = 30;
	double scr_mul;	// screen size multiplier

	std::vector<BITMAP*> floor_texture;
	std::vector<BITMAP*> wall_texture;
	std::vector<BITMAP*> player_sprite[2];
	BITMAP* player_sprite_power;

	int map_list_size;
	int map_list_start;
	
	int team_captures_size;
	int team_captures_start;

	// drawing screens
	BITMAP* vidpage1;
	BITMAP* vidpage2;
	BITMAP* backbuf;
	bool page_flipping;

    std::list<clientfx_t> cfx;

    std::string themedir;
    std::string theme_name;
    al_ffblk themeffblk;	// for al_find*
    bool no_theme;

	Antialiasing_mode antialiasing;

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
		COLDARKGREEN,
		NUM_OF_COL
	};

	int teamcol[3];
	int teamlcol[2];	// light colours for statusbar
	int teamdcol[2];	// dark colours for player name

	int	col[NUM_OF_COL];
};

#endif // GRAPHICS_H_INC

