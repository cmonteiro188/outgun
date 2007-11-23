/*
 *  graphics.h
 *
 *  Copyright (C) 2002 - Fabio Reis Cecin
 *  Copyright (C) 2003, 2004, 2005, 2006 - Niko Ritari
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007 - Jani Rivinoja
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

#ifndef GRAPHICS_H_INC
#define GRAPHICS_H_INC

#include <string>
#include <list>
#include <vector>

#include "colour.h"
#include "incalleg.h"
#include "mutex.h"
#include "utility.h"
#include "world.h"

// ---- client screen layout ----

class Map;
class MapInfo;
class Room;
class RectWall;
class TriWall;
class CircWall;
class ClientPlayer;
class Sounds;
class GraphicsEffect;
class Team;
class Flag;
class Powerup;
class Message;
class WorldCoords;
class Rocket;

class ScreenMode {
public:
    int width, height;

    ScreenMode(int w, int h) : width(w), height(h) { }
    bool operator==(const ScreenMode& o) const { return width == o.width && height == o.height; }
    bool operator<(const ScreenMode& o) const { return width < o.width || (width == o.width && height < o.height); }
};

class Bitmap {
    BITMAP* ptr;

public:
    Bitmap() : ptr(0) { }
    Bitmap(BITMAP* ptr_) : ptr(ptr_) { }
    ~Bitmap() { free(); }

    Bitmap(const Bitmap& o) : ptr(0) { nAssert(!o.ptr); }
    void operator=(const Bitmap& o) { nAssert(!ptr); nAssert(!o.ptr); }

    void free() { if (ptr) { destroy_bitmap(ptr); ptr = 0; } }
    const Bitmap& operator=(BITMAP* ptr_) { nAssert(!ptr); ptr = ptr_; return *this; }

    operator BITMAP*() const { return ptr; }
    BITMAP* operator->() const { return ptr; }
};

class TemporaryClipRect {
    BITMAP* b;
    int x1, y1, x2, y2;

public:
    TemporaryClipRect(BITMAP* bitmap, int x1_, int y1_, int x2_, int y2_, bool respectOld);
    ~TemporaryClipRect();
};

enum MapListSortKey { MLSK_Number, MLSK_Votes, MLSK_Title, MLSK_Size, MLSK_Author, MLSK_Favorite, MLSK_COUNT };

class Message {
public:
    Message(Message_type type_, const std::string& txt, int time_, int team_ = -1)
        : msg_type(type_), msg_text(txt), msg_time(time_), sender_team(team_), is_highlighted(false) { }

    void highlight() { is_highlighted = true; }

    Message_type type() const { return msg_type; }
    const std::string& text() const { return msg_text; }
    int time() const { return msg_time; }
    int team() const { return sender_team; }
    bool highlighted() const { return is_highlighted; }

private:
    Message_type msg_type;
    std::string msg_text;
    int msg_time;
    int sender_team; // -1 for non-team messages or unknown team
    bool is_highlighted;
};

class Graphics {
public:
    static const int
                  item_max_size_in_world = 30,
         extended_item_max_size_in_world = 60, // include some space for deathbringer smoke
                  flag_max_size_in_world = 100,
         extended_flag_max_size_in_world = 120, // include space for emphasis markers
                rocket_max_size_in_world = 2 * 2 * ROCKET_RADIUS,
       extended_rocket_max_size_in_world = 2 * (POWER_ROCKET_RADIUS + 11), // max(rocket_max_size_in_world, 2 * (POWER_ROCKET_RADIUS + 11)) (include space for shadow)
                player_max_size_in_world = 2 * 2 * PLAYER_RADIUS,
       extended_player_max_size_in_world = extended_flag_max_size_in_world; // max(extended_flag_max_size_in_world, player_max_size_in_world + 50) (include space for carried flag and deathbringer smoke)

    static const std::string save_extension; // file extension like ".pcx", depending on configured libraries (and potentially selectable by user in the future)

    Graphics(LogSet logs);
    ~Graphics();

    bool depthAvailable(int depth) const;
    std::vector<ScreenMode> getResolutions(int depth, bool forceTryIfNothing = true) const; // returns a sorted list of unique resolutions
    bool init(int width, int height, int depth, bool windowed, bool flipping);
    void videoMemoryCorrupted();    // call this when that happens with page flipping

    void setRoomLayout(const Map& map, double visible_rooms, bool repeatMap); // call before any playfield draw operation, including the playfield version of draw_background
    typedef std::vector<std::vector<NLubyte> > VisibilityMap;
    void draw_background(bool map_ready);
    void draw_background(const Map& map, const VisibilityMap& roomVis, const WorldCoords& topLeft, bool continuousTextures, bool mapInfoMode);

    void startDraw();   // call startDraw before any drawing operations are done and endDraw when done, before drawScreen
    void endDraw();
    void startPlayfieldDraw();  // between calls to startPlayfieldDraw and endPlayfieldDraw, anything is only drawn to within the playfield area
    void endPlayfieldDraw();

    void draw_screen(bool acquireWithFlipping);
    bool save_screenshot(const std::string& filename) const;

    void reset_playground_colors();
    void random_playground_colors();

    void draw_flag(int team, const WorldCoords& pos, bool flash, int alpha = 255, bool emphasize = false, double timeUnknown = 0);
    void draw_mini_flag(int team, const Flag& flag, const Map& map, bool flash = false, bool old = false);
    void update_minimap_background(const Map& map);
    void draw_minimap_player(const Map& map, const ClientPlayer& player);
    void draw_minimap_me(const Map& map, const ClientPlayer& player, double time);
    void draw_minimap_room(const Map& map, int rx, int ry, float visibility);
    void highlight_minimap_rooms();

    void draw_neighbor_marker(bool flag, const WorldCoords& pos, int team, bool old = false);

    void draw_player(const WorldCoords& pos, int team, int colorId, GunDirection gundir, double hitfx, bool power, int alpha, double time);
    void draw_player_name(const std::string& name, const WorldCoords& pos, int team, bool highlight = false);
    void draw_player_dead(const ClientPlayer& player, double respawn_delay = 0.);
    void draw_me_highlight(const WorldCoords& pos, double size);
    void draw_aim(const Room& room, const WorldCoords& pos, GunDirection gundir, int team);

    void draw_rocket(const Rocket& rocket, bool shadow, double time);
    void draw_gun_explosion(const WorldCoords& pos, int rad, int team);
    void draw_deathbringer_smoke(const WorldCoords& pos, double time, double alpha);
    void draw_deathbringer(const DeathbringerExplosion& db, double frame);

    void draw_player_health(int value);
    void draw_player_energy(int value);

    void draw_deathbringer_affected(const WorldCoords& pos, int team, int alpha);
    void draw_deathbringer_carrier_effect(const WorldCoords& pos, int alpha);
    void draw_shield(const WorldCoords& pos, int r, bool live, int alpha = 255, int team = -1, GunDirection direction = GunDirection());

    void draw_virou_sorvete(const WorldCoords& pos, double respawn_delay = 0.);

    void draw_waiting_map_message(const std::string& caption, const std::string& map);
    void draw_loading_map_message(const std::string& text);
    void draw_scores(const std::string& text, int col, int score1, int score2);
    void print_chat_messages(std::list<Message>::const_iterator begin, const std::list<Message>::const_iterator& end,
                             const std::string& talkbuffer, int cursor_pos);

    void draw_scoreboard(const std::vector<ClientPlayer*>& players, const Team* teams, int maxplayers, bool pings = false, bool underlineMasterAuthenticated = false, bool underlineServerAuthenticated = false);

    void team_statistics(const Team* teams);
    void draw_statistics(const std::vector<ClientPlayer*>& players, int page, int time, int maxplayers, int max_world_rank = 0);
    void debug_panel(const std::vector<ClientPlayer>& players, int me, int bpsin, int bpsout,
                     const std::vector<std::vector<std::pair<int, int> > >& sticks, const std::vector<int>& buttons);

    void map_list(const std::vector< std::pair<const MapInfo*, int> >& maps, MapListSortKey sortedBy, int current = -1,
                  int own_vote = -1, const std::string& edit_vote = "");

    void map_list_prev() { --map_list_start; }
    void map_list_next() { ++map_list_start; }
    void map_list_prev_page() { map_list_start -= map_list_size; }
    void map_list_next_page() { map_list_start += map_list_size; }
    void map_list_begin() { map_list_start = 0; }
    void map_list_end() { map_list_start = INT_MAX; }

    void team_captures_prev() { --team_captures_start; }
    void team_captures_next() { ++team_captures_start; }

    void draw_player_power(double val);
    void draw_player_turbo(double val);
    void draw_player_shadow(double val);
    void draw_player_weapon(int level);
    void draw_map_time(int seconds, bool extra_time = false);
    void draw_fps(double fps);
    void draw_change_team_message(double time);
    void draw_change_map_message(double time, bool delayed = false);

    void draw_pup(const Powerup& pup, double time, bool live);

    // client side effects
    void draw_effects(double time);
    void draw_turbofx(double time);

    void clear_fx();

    void create_wallexplo(const WorldCoords& pos, int team, double time);
    void create_powerwallexplo(const WorldCoords& pos, int team, double time);
    void create_deathcarrier(WorldCoords pos, int alpha, double time, bool for_item = false);
    void create_turbofx(const WorldCoords& pos, int col1, int col2, GunDirection gundir, int alpha, double time);
    void create_gunexplo(const WorldCoords& pos, int team, double time);

    bool save_map_picture(const std::string& filename, const Map& map);

    void search_themes(LineReceiver& dst_theme, LineReceiver& dst_bg, LineReceiver& dst_colours) const;
    void select_theme(const std::string& name, const std::string& bg_dir, bool use_theme_bg, const std::string& colour_dir, bool use_theme_colours);

    void search_fonts(LineReceiver& dst_font) const;
    void select_font(const std::string& file);

    void set_antialiasing(bool enable) { antialiasing = enable; roomGraphicsChanged(); }

    void set_min_transp(bool enable) { min_transp = enable; }

    int player_color(int index) const { nAssert(index >= 0 && index <= MAX_PLAYERS / 2); return col[index]; }

    // How many lines fit on the chat area and screen.
    int chat_lines() const;
    int chat_max_lines() const;

    BITMAP* drawbuffer() { return drawbuf; }

    // public only for Mappic
    void setColors();

    void set_stats_alpha(int alpha) { stats_alpha = alpha; }

    const Colour_manager& colours() const { return colour; }

    void draw_replay_info(float rate, unsigned position, unsigned length, bool stopped);
    void toggle_full_playfield();

    bool needRedrawMap() const { return mapNeedsRedraw; }

    void mapChanged() { roomGraphicsChanged(); mapNeedsRedraw = true; }

    double get_visible_rooms_x() const { return roomLayout.visibleRoomsX(); }
    double get_visible_rooms_y() const { return roomLayout.visibleRoomsY(); }

private:
    void unload_bitmaps();

    bool reset_video_mode(int width, int height, int depth, bool windowed, int pages = 1);

    void make_layout();

    void update_minimap_background(BITMAP* buffer, const Map& map, bool save_map_pic = false);

    void roomGraphicsChanged() { background.invalidateRoomCache(); }

    void drawRoomBackground(BITMAP* roombg, const Map& map, int roomx, int roomy, int texRoomX, int texRoomY, bool mapInfoMode);

    void draw_room_ground(BITMAP* buffer, const Room& room, int x, int y, int texOffsetBaseX, int texOffsetBaseY, double scale);
    void draw_room_walls(BITMAP* buffer, const Room& room, int x, int y, int texOffsetBaseX, int texOffsetBaseY, double scale);

    static void draw_wall(BITMAP* buffer, WallBase* wall, double x, double y, int texOffsetBaseX, int texOffsetBaseY, double scale, int color, BITMAP* tex);
    static void draw_rect_wall(BITMAP* buffer, const RectWall& wall, double x0, double y0, int texOffsetBaseX, int texOffsetBaseY, double scale, int color, BITMAP* texture);
    static void draw_tri_wall (BITMAP* buffer, const TriWall & wall, double x0, double y0, int texOffsetBaseX, int texOffsetBaseY, double scale, int color, BITMAP* texture);
    static void draw_circ_wall(BITMAP* buffer, const CircWall& wall, double x0, double y0, int texOffsetBaseX, int texOffsetBaseY, double scale, int color, BITMAP* texture);

    void draw_flagpos_mark(BITMAP* roombg, int team, int flag_x, int flag_y);

    void draw_pup_shield(const WorldCoords& pos, bool live);
    void draw_pup_turbo(const WorldCoords& pos, bool live);
    void draw_pup_shadow(const WorldCoords& pos, double time);
    void draw_pup_power(const WorldCoords& pos, double time);
    void draw_pup_weapon(const WorldCoords& pos, double time);
    void draw_pup_health(const WorldCoords& pos, double time);
    void draw_pup_deathbringer(const WorldCoords& pos, double time, bool live);

    std::pair<int, int> calculate_minimap_coordinates(const Map& map, const ClientPlayer& player) const;

    void draw_bar(int x, const std::string& caption, int value, int c100, int c200, int c300);
    void draw_powerup_time(int line, const std::string& caption, double val, int c);

    void draw_player_statistics(const FONT* stfont, const ClientPlayer& player, int x, int y, int page, int time);

    void draw_scoreboard_name(const FONT* sbfont, const std::string& name, int x, int y, int pcol, bool underline);
    void draw_scoreboard_points(const FONT* sbfont, int points, int x, int y, int team);

    void print_chat_message(Message_type type, const std::string& message, int team, int x, int y, bool highlight = false);
    void print_chat_input(const std::string& message, int x, int y, int cursor);

    void print_text(const std::string& text, int x, int y, int textcol, int bgcol);
    void print_text_centre(const std::string& text, int x, int y, int textcol, int bgcol);

    void print_text_border(const std::string& text, int x, int y, int textcol, int bordercol, int bgcol);
    void print_text_border_centre(const std::string& text, int x, int y, int textcol, int bordercol, int bgcol);

    void print_text_border_check_bg(const std::string& text, int x, int y, int textcol, int bordercol, int bgcol);
    void print_text_border_centre_check_bg(const std::string& text, int x, int y, int textcol, int bordercol, int bgcol);

    void print_text_border(const std::string& text, int x, int y, int textcol, int bordercol, int bgcol, bool centring);

    void scrollbar(int x, int y, int height, int bar_y, int bar_h, int col1, int col2);

    void make_db_effect();

    BITMAP* load_bitmap(const std::string& file) const;

    void load_background();
    void load_generic_pictures();
    void load_playfield_pictures();
    void reload_playfield_pictures();

    void load_floor_textures(const std::string& filename);
    void load_wall_textures(const std::string& filename);
    BITMAP* get_floor_texture(int texid);
    BITMAP* get_wall_texture(int texid);

    void load_player_sprites(const std::string& path);
    void load_shield_sprites(const std::string& path);
    void load_dead_sprites(const std::string& path);
    void load_rocket_sprites(const std::string& path);
    void load_flag_sprites(const std::string& path);
    void load_pup_sprites(const std::string& path);

    BITMAP* scale_sprite(const std::string& filename, int x, int y) const;
    BITMAP* scale_alpha_sprite(const std::string& filename, int x, int y) const;
    static void set_alpha_channel(BITMAP* bitmap, BITMAP* alpha);
    static void rotate_trans_sprite(BITMAP* bmp, BITMAP* sprite, int x, int y, fixed angle, int alpha); // x,y are destination coords of the sprite center
    static void rotate_alpha_sprite(BITMAP* bmp, BITMAP* sprite, int x, int y, fixed angle);    // x,y are destination coords of the sprite center
    static int colorTo32(int color) { return makecol32(getr(color), getg(color), getb(color)); }
    static void overlayColor(BITMAP* bmp, BITMAP* alpha, int color);    // alpha must be an 8-bit bitmap; give the color in same format as bmp
    static void combine_sprite(BITMAP* sprite, BITMAP* common, BITMAP* team, BITMAP* personal, int tcol, int pcol); // give the colors in same format as sprite

    void unload_pictures();
    void unload_generic_pictures();
    void unload_playfield_pictures();
    void unload_floor_textures();
    void unload_wall_textures();
    void unload_player_sprites();
    void unload_shield_sprites();
    void unload_dead_sprites();
    void unload_rocket_sprites();
    void unload_flag_sprites();
    void unload_pup_sprites();

    void load_font(const std::string& file);

    int scale(double value) const;
    int pf_scale(double value) const { return roomLayout.pf_scale(value); }
    double pf_scaled(double value) const { return roomLayout.pf_scaled(value); }

    class RoomLayoutManager {
        Graphics& g;

        int plx, ply;
        double playfield_scale;
        WorldCoords topLeft;
        double visible_rooms, visible_rooms_x, visible_rooms_y;
        int map_w, map_h;
        int room_w, room_h;

    public:
        RoomLayoutManager(Graphics& host) : g(host) { }

        bool set(int mapWidth, int mapHeight, double visibleRooms, bool repeatMap); // returns true if scale has changed
        void setTopLeft(const WorldCoords& topLeftCoords) { topLeft = topLeftCoords; }

        int x0() const { return plx; }
        int y0() const { return ply; }
        int xMax() const { return plx + static_cast<int>(visible_rooms_x * room_w) - 1; }
        int yMax() const { return ply + static_cast<int>(visible_rooms_y * room_h) - 1; }

        int mapWidth() const { return map_w; }
        int mapHeight() const { return map_h; }
        int roomWidth() const { return room_w; }
        int roomHeight() const { return room_h; }

        const WorldCoords& topLeftCoords() const { return topLeft; }
        double visibleRoomsX() const { return visible_rooms_x; }
        double visibleRoomsY() const { return visible_rooms_y; }

        int pf_scale(double value) const;
        double pf_scaled(double value) const;

        std::vector<int> scale_x(const WorldCoords& pos) const { return scale_x(pos.px, pos.x); }
        std::vector<int> scale_y(const WorldCoords& pos) const { return scale_y(pos.py, pos.y); }
        std::vector<int> scale_x(int roomx, double lx) const;
        std::vector<int> scale_y(int roomy, double ly) const;

        std::vector<int> room_offset_x(int rx) const; // returns the pixel position(s) where room rx begins
        std::vector<int> room_offset_y(int ry) const; // returns the pixel position(s) where room ry begins

        double distanceFromScreenX(int rx, double lx) const; // 0 = on screen, < 0 = as many rooms to the left from screen, > 0 = to the right (wrapping around map edges)
        double distanceFromScreenY(int ry, double ly) const;

        bool on_screen(int rx, int ry) const; // returns true if some part of the room may be on screen
    };
    RoomLayoutManager roomLayout;

    class ScaledCoordSet {
        const std::vector<int> vx, vy;
        CartesianProductIterator i;

    public:
        ScaledCoordSet(const WorldCoords& pos, const Graphics* g) : vx(g->roomLayout.scale_x(pos)), vy(g->roomLayout.scale_y(pos)), i(vx.size(), vy.size()) { }
        ScaledCoordSet(int roomx, int roomy, double lx, double ly, const Graphics* g) : vx(g->roomLayout.scale_x(roomx, lx)), vy(g->roomLayout.scale_y(roomy, ly)), i(vx.size(), vy.size()) { }
        bool next() { return i.next(); }
        int x() const { return vx[i.i1()]; }
        int y() const { return vy[i.i2()]; }
    };

    class RoomPosSet {
        const std::vector<int> vx, vy;
        CartesianProductIterator i;

    public:
        RoomPosSet(int roomx, int roomy, const Graphics* g) : vx(g->roomLayout.room_offset_x(roomx)), vy(g->roomLayout.room_offset_y(roomy)), i(vx.size(), vy.size()) { }
        bool next() { return i.next(); }
        int x() const { return vx[i.i1()]; }
        int y() const { return vy[i.i2()]; }
    };

    class BackgroundManager {
    public:
        BackgroundManager(Graphics& host) : g(host) { }

        void draw_background(BITMAP* drawbuf, bool draw_map, bool reserve_playfield);
        void draw_playfield_background(BITMAP* drawbuf, const Map& map, const VisibilityMap& roomVis, bool continuousTextures, bool mapInfoMode);

        void invalidateRoomCache() { roomCache.clear(); roomCacheIndex.clear(); roomCacheMemoryBitmap.free(); cacheTimestamp = 0; }

        bool allocate(bool videoMemory, int cachePages);
        void free() { roomCacheBitmap.free(); invalidateRoomCache(); }

    private:
        void allocateRoomCache(int room_w, int room_h, int minRooms, int maxRooms);

        void cacheRoom(const Map& map, int roomx, int roomy, bool continuousTextures, bool mapInfoMode);
        void drawRoom(const Map& map, int roomx, int roomy, bool continuousTextures, bool mapInfoMode, bool fogged, BITMAP* target, int tx0, int ty0);

        struct BitmapRegion {
            BITMAP* b;
            int x0, y0, w, h;

            BitmapRegion() : b(0) { }
            BitmapRegion(BITMAP* bm, int x0_, int y0_, int w_, int h_) : b(bm), x0(x0_), y0(y0_), w(w_), h(h_) { }

            void blitTo(BITMAP* target, int tx0, int ty0) const;
        };

        class CachedRoomGfx {
            int roomx, roomy;
            BitmapRegion baseArea;
            BitmapRegion foggedArea;
            bool baseGenerated;
            mutable bool foggedGenerated;
            int fogColor;

            void generateFogged(BITMAP* target, int tx0, int ty0, bool fastFog) const;

        public:
            bool locked;
            unsigned lastUse;

            CachedRoomGfx() { } // uninitialized CachedRoomGfx's should be used nowhere
            CachedRoomGfx(const BitmapRegion& b1, const BitmapRegion& b2, int fogColor_) :
                    roomx(0), roomy(0), baseArea(b1), foggedArea(b2), baseGenerated(false), foggedGenerated(false),
                    fogColor(fogColor_), locked(false), lastUse(0) { nAssert(baseArea.w == foggedArea.w && baseArea.h == foggedArea.h || !foggedArea.b); }

            bool used() const { return baseGenerated; }
            int x() const { return roomx; }
            int y() const { return roomy; }

            BitmapRegion& getAreaForWriting(int roomx_, int roomy_) { roomx = roomx_; roomy = roomy_; baseGenerated = true; foggedGenerated = false; locked = true; return baseArea; }

            void drawUnfogged(BITMAP* target, int tx0, int ty0) const { nAssert(baseGenerated); baseArea.blitTo(target, tx0, ty0); }
            void drawFogged  (BITMAP* target, int tx0, int ty0) const;
            const BitmapRegion& fogged() const;
        };

        std::vector<CachedRoomGfx> roomCache;
        std::vector<std::vector<CachedRoomGfx*> > roomCacheIndex;
        unsigned cacheTimestamp;

        Graphics& g;

        Bitmap roomCacheBitmap, roomCacheMemoryBitmap; // the memory one is never from video memory, and is used (and allocated) only when enough rooms don't fit in roomCacheBitmap

        bool previousContinuousTextures, previousMapInfoMode;
        int previousRoom0x, previousRoom0y, previousXrooms, previousYrooms;
    };

    BackgroundManager background;

    std::vector<std::string> file_extensions;

    // drawing screens
    Bitmap vidpage1;
    Bitmap vidpage2;
    Bitmap backbuf;
    bool page_flipping;
    bool min_transp;

    BITMAP* drawbuf;    // main draw buffer (points to vidpage# or backbuf at a given time)
    Bitmap minibg;      // minimap draw buffer
    Bitmap minibg_fog;  // minimap with fog in every room

    int playfield_x, playfield_y; // playground area position on the screen
    int playfield_w, playfield_h;

    int scoreboard_x1, scoreboard_x2;  // scoreboard position
    int scoreboard_y1, scoreboard_y2;

    int minimap_place_x, minimap_place_y;
    int minimap_place_w, minimap_place_h;
    int minimap_x, minimap_y;
    int minimap_w, minimap_h;
    int indicators_y;
    int health_x, energy_x, pups_x, pups_val_x, weapon_x, time_x, time_y;

    bool show_chat_messages;
    bool show_scoreboard;
    bool show_minimap;

    static const int flagpos_radius = 30;
    double scr_mul; // screen size multiplier

    Bitmap bg_texture;

    std::vector<Bitmap> floor_texture;
    std::vector<Bitmap> wall_texture;

    Bitmap player_sprite_power;
    std::vector<Bitmap> pup_sprite;

    // Team specific sprites
    std::vector<Bitmap> player_sprite[2];
    Bitmap shield_sprite[2];
    Bitmap dead_sprite[2];
    Bitmap rocket_sprite[2];
    Bitmap power_rocket_sprite[2];

    Bitmap flag_sprite[3];  // red, blue and wild flag
    Bitmap flag_flash_sprite[3];

    Bitmap ice_cream;

    Bitmap db_effect;       // the darkening of the ground around the player

    FONT* default_font;
    FONT* border_font;

    int map_list_size;
    int map_list_start;

    int team_captures_start;

    std::list<GraphicsEffect> cfx;

    std::string theme_path;
    std::string bg_path;
    std::string colour_file;

    bool antialiasing;

    int stats_alpha;

    int teamcol[3];
    int teamflashcol[3]; // flag flashing colours
    int teamlcol[2];     // light colours
    int teamdcol[2];     // dark colours

    int col[MAX_PLAYERS / 2 + 1]; // player colours, one extra used for unknown colour
    Colour groundCol, wallCol;

    static const int fogOfWarMaxAlpha = 0x38, playfieldFogOfWarAlpha = 0x38;

    Colour_manager colour;

    bool mapNeedsRedraw, needReloadPlayfieldPictures;

    mutable LogSet log;
};

#endif // GRAPHICS_H_INC
