#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include <cmath>

#include "incalleg.h"
#include "commont.h"
#include "world.h"
#include "effects.h"
#include "sounds.h"
#include "antialias.h"
#include "client.h"
#include "graphics.h"

const bool TEST_FALL_ON_WALL = false;

// Video driver selection: Look at Allegro's documentation for alternate values; changing these will especially help on Linux

//const int WINMODE = GFX_DIRECTX_ACCEL;
//const int FULLMODE = GFX_DIRECTX_ACCEL;
const int WINMODE = GFX_AUTODETECT_WINDOWED;
const int FULLMODE = GFX_AUTODETECT;

const bool SWITCH_PAUSE_CLIENT = false;

using std::ifstream;
using std::istringstream;
using std::left;
using std::list;
using std::max;
using std::min;
using std::ostringstream;
using std::pair;
using std::right;
using std::setfill;
using std::setprecision;
using std::setw;
using std::sort;
using std::string;
using std::vector;

Graphics::Graphics(LogSet logs):
	show_chat_messages(true),
	show_scoreboard(true),
	show_minimap(true),
	map_list_size(20),
	map_list_start(0),
	team_captures_size(16),
	team_captures_start(0),
	no_theme(false),
	antialiasing(AA_both),
	log(logs)
{ }

Graphics::~Graphics() {
	unload_bitmaps();
}

bool Graphics::init(int width, int height, int depth, bool windowed, bool tryFlipping) {
	unload_bitmaps();

	if (!reset_video_mode(width, height, depth, windowed))
		return false;

	if (tryFlipping) {
		vidpage1 = create_video_bitmap(SCREEN_W, SCREEN_H);
		vidpage2 = create_video_bitmap(SCREEN_W, SCREEN_H);
		background = create_video_bitmap(SCREEN_W, SCREEN_H);
	}
	if (!vidpage1 || !vidpage2 || !background) {
		if (tryFlipping)
			log("Not enough video memory. Can't use page flipping.");
		vidpage1.free();
		vidpage2.free();
		background.free();
		backbuf = create_bitmap(SCREEN_W, SCREEN_H);
		nAssert(backbuf);
		background = create_bitmap(SCREEN_W, SCREEN_H);
		nAssert(background);

		drawbuf = backbuf;
		page_flipping = false;
	}
	else {
		drawbuf = vidpage1;
		page_flipping = true;
	}

	scr_mul = static_cast<double>(SCREEN_W) / 640;
	if (SCREEN_H < scr_mul * plh + 35 + 8)			// the window is too low for playground and one line for messages
		scr_mul = static_cast<double>(SCREEN_H - 35 - 8) / plh;
	floor_texture.resize(8);
	wall_texture.resize(8);
	for (int t = 0; t < 2; t++)
		player_sprite[t].resize(MAX_PLAYERS / 2);
	pup_sprite.resize(Powerup::pup_last_real + 1);
	plx = 0;
	ply = SCREEN_H - scale(plh) - 35;
	roombg = create_sub_bitmap(background, plx, ply, static_cast<int>(ceil(scr_mul * plw)), static_cast<int>(ceil(scr_mul * plh)));
	minimap_w = minimap_place_w = SCREEN_W - roombg->w;
	minimap_h = minimap_place_h = scale(100);
	mmx = SCREEN_W - minimap_w;
	if (mmx > 8 * 80)	// check if minimap fits to the right of chat messages
		mmy = 2;
	else
		mmy = ply;
	minibg = create_bitmap(minimap_place_w, minimap_place_h);
	nAssert(minibg);
	sbx = SCREEN_W - (20 * 8 + 4);	// scoreboard is 20 characters wide
	sby = mmy + minimap_place_h + 10;
	indicators_x = 0;
	indicators_y = SCREEN_H - 30;
	reset_playground_colors();
	setColors();
	if (!no_theme)
		load_pictures(theme_path);

	return true;
}

void Graphics::unload_bitmaps() {
	vidpage1.free();
	vidpage2.free();
	backbuf.free();
	background.free();
	minibg.free();
	unload_pictures();
}

void Graphics::startDraw() {
	if (page_flipping)
		acquire_bitmap(drawbuf);
}

void Graphics::endDraw() {
	if (page_flipping)
		release_bitmap(drawbuf);
}

void Graphics::draw_screen() {
	if (page_flipping) {
		show_video_bitmap(drawbuf);

		if (drawbuf == vidpage1)
			drawbuf = vidpage2;
		else
			drawbuf = vidpage1;
	}
	else
		blit(drawbuf, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
}

void Graphics::setColors() {
	//the first 8 colors are player's colors
	col[COLGREEN] = makecol(0x00, 0xFF, 0x00);
	col[COLYELLOW] = makecol(0xFF, 0xFF, 0x00);
	col[COLWHITE] = makecol(0xFF, 0xFF, 0xFF);
	col[COLMAG]	= makecol(0xFF, 0x00, 0xFF);
	col[COLCYAN] = makecol(0, 0xFF, 0xFF);
	col[COLORA]	= makecol(0xFF, 0xA0, 0x00);
	col[COLLRED] = makecol(0xFF, 0x55, 0x44);
	col[COLLBLUE] = makecol(0x44, 0x55, 0xFF);
	//MORE player colors
	col[COL9] = makecol(0x00, 0x80, 0x00);
	col[COL10] = makecol(0xA0, 0xA0, 0xA0);
	col[COL11] = makecol(0x00, 0x00, 0x00);
	col[COL12] = makecol(0x80, 0x00, 0x80);
	col[COL13] = makecol(0xA0, 0x60, 0x00);
	col[COL14] = makecol(0x00, 0x00, 0x80);
	col[COL15] = makecol(0x80, 0x00, 0x00);
	col[COL16] = makecol(0x66, 0x66, 0x66);

	// team solid colors
	col[COLBLUE] = makecol(0, 0, 0xFF);
	col[COLRED] = makecol(0xFF, 0, 0);

	// base minimap background colors
	col[COLBBLUE] = makecol(0, 0, 0x66);
	col[COLBRED] = makecol(0x66, 0, 0);

	//other
	col[COLFOGOFWAR] = makecol(0xFF, 0xFF, 0xFF);
	col[COLMENUWHITE] = makecol(0xC0, 0xC0, 0xC0);
	col[COLMENUGRAY] = makecol(0x68,0x68,0x68);
	col[COLMENUBLACK] = makecol(0x40,0x40,0x40);
	col[COLNOLIFE] = makecol(0, 0, 0);
	col[COLDARKGRAY] = makecol(0x30, 0x30, 0x30);
	col[COLSHADOW] = makecol(0x18, 0x18, 0x18);
	col[COLLIMBO] = makecol(0x10, 0x10, 0x10);
	col[COLDARKORA]	= makecol(0xBF, 0x70, 0x00);
	col[COLINFO] = col[COLDARKORA];		//color of statusbar non-game info (hostname, IP, net traffic)
	col[COLENER3] = makecol(125, 100, 255);
	col[COLDARKGREEN] = makecol(0x00, 0x77, 0x00);

	//teams 0 & 1 (playernum(0..15) / 8) colors:
	teamcol[0] = col[COLRED];
	teamcol[1] = col[COLBLUE];

	// wild flag colour
	teamcol[2] = col[COLGREEN];

	//light colours for text
	teamlcol[0] = col[COLLRED];
	teamlcol[1] = col[COLLBLUE];

	// dark colours for team text bg
	teamdcol[0] = col[COLBRED];
	teamdcol[1] = col[COLBBLUE];

	setPlaygroundColors();
}

void Graphics::setPlaygroundColors() {
	col[COLGROUND] = makecol(groundCol[0], groundCol[1], groundCol[2]);
	col[COLWALL] = makecol(wallCol[0], wallCol[1], wallCol[2]);
}

void Graphics::reset_playground_colors() {
	static const int groundInit[] = { 0x10, 0x40, 0x00 };
	static const int wallInit  [] = { 0x30, 0xC0, 0x00 };
	for (int i = 0; i < 3; ++i) {
		groundCol[i] = groundInit[i];
		wallCol  [i] = wallInit  [i];
	}
	setPlaygroundColors();
}

void Graphics::random_playground_colors() {
	for (int i = 0; i < 3; ++i) {
		groundCol[i] = rand() % 256;
		wallCol  [i] = rand() % 256;
	}
	setPlaygroundColors();
}

int Graphics::chat_lines() const {
	return max(1, ply / 11);
}

int Graphics::chat_max_lines() const {
	return max(1, (ply + scale(plh)) / 11);
}

void Graphics::clear() {
	clear_to_color(drawbuf, 0);
}

bool Graphics::depthAvailable(int depth) const {
	return !getResolutions(depth, false).empty();	//#opt: no need to go through all modes or create a vector
}

vector<ScreenMode> Graphics::getResolutions(int depth, bool forceTryIfNothing) const {	// returns a sorted list of unique resolutions
	vector<ScreenMode> mvec;

	#ifdef ALLEGRO_WINDOWS
	GFX_MODE_LIST* modes = get_gfx_mode_list(GFX_DIRECTX);
	if (modes) {
		int depth2 = (depth == 16) ? 15 : depth;	// 15 and 16 bit modes are considered equal
		for (int i = 0; i < modes->num_modes; i++) {
			const GFX_MODE& mode = modes->mode[i];
			if (mode.width >= 640 && mode.height >= 400 && (mode.bpp == depth || mode.bpp == depth2))
				mvec.push_back(ScreenMode(mode.width, mode.height));
		}
		destroy_gfx_mode_list(modes);
	}
	if (mvec.empty())
		log("No usable %d-bit DirectX fullscreen modes autodetected.", depth);
	#endif

	ifstream resFile((wheregamedir + "config" + directory_separator + "gfxmodes.txt").c_str());
	for (;;) {
		string line;
		if (!getline_skip_comments(resFile, line))
			break;
		istringstream ss(line);
		int width, height, bits;
		char nullc;
		ss >> width >> height >> bits;
		bool ok = ss;
		ss >> nullc;
		if (!ok || ss) {
			log.error("Syntax error in gfxmodes.txt, line '%s'.", line.c_str());
			break;
		}
		if (width < 640 || height < 400 || (bits != 16 && bits != 24 && bits != 32)) {
			log.error("Unusable mode in gfxmodes.txt : %d×%d×%d (should be at least 640×400 with bits 16, 24 or 32)",
							width, height, bits);
			break;
		}
		if (bits == depth)
			mvec.push_back(ScreenMode(width, height));
	}

	if (mvec.empty() && forceTryIfNothing)
		mvec.push_back(ScreenMode(640, 480));	// just try something

	sort(mvec.begin(), mvec.end());
	mvec.erase(std::unique(mvec.begin(), mvec.end()), mvec.end());
	return mvec;
}

bool Graphics::reset_video_mode(int width, int height, int depth, bool windowed) {
	log("Setting video mode: %d×%d×%d %s", width, height, depth, windowed ? "windowed" : "fullscreen");
	set_color_depth(depth);
	if (set_gfx_mode(windowed ? WINMODE : FULLMODE, width, height, 0, 0)) {
		log("Error: '%s'", allegro_error);
		if (depth == 16) {	// try equivalent 15-bit mode too
			set_color_depth(15);
			if (set_gfx_mode(windowed ? WINMODE : FULLMODE, width, height, 0, 0)) {
				log("Error with equivalent 15-bit mode: '%s'", allegro_error);
				return false;
			}
		}
		else
			return false;
	}

	if (!SWITCH_PAUSE_CLIENT) {
		if (set_display_switch_mode(SWITCH_BACKAMNESIA) == -1) {
			if (set_display_switch_mode(SWITCH_BACKGROUND) == -1) { // allow running in the background
				log("Client cannot run in the background!");
				return false; // FATAL
			}
			else
				log("Switch_background set ok.");
		}
		else
			log("Switch_backamnesia set ok.");
	}

	log("Testing. If Outgun hangs here, restarting Windows should help. To avoid the problem, don't run Outgun with certain programs that use overlays (e.g. TV software).");
	acquire_screen();
	release_screen();
	log("Hang test complete, no problems.");

	return true;
}

void Graphics::predraw(const Room& room, const vector< pair<int, const spoint_t*> >& flags, const vector< pair<int, const spoint_t*> >& spawns, bool grid) {
	acquire_bitmap(background);
	clear_to_color(background, 0);
	if (antialiasing == AA_both) {
		SceneAntialiaser scene;
		scene.setScaling(0, 0, scr_mul);

		// add bottom ground to drawing system
		scene.addRectangle(0, 0, plw, plh, 0);

		// add additional ground textures
		for (vector<RectWall>::const_iterator rwi = room.rground.begin(); rwi != room.rground.end(); ++rwi)
			scene.addRectWall(*rwi, rwi->texture());
		for (vector< TriWall>::const_iterator twi = room.tground.begin(); twi != room.tground.end(); ++twi)
			scene.addTriWall (*twi, twi->texture());
		for (vector<CircWall>::const_iterator cwi = room.cground.begin(); cwi != room.cground.end(); ++cwi)
			scene.addCircWall(*cwi, cwi->texture());

		// add flag markers as overlays
		const float fr = flagpos_radius;
		for (int fi = 0; fi < static_cast<int>(flags.size()); ++fi) {
			const float fx = flags[fi].second->x, fy = flags[fi].second->y;
			scene.addRectangle(fx - fr, fy - fr, fx + fr, fy + fr, fi + floor_texture.size() + wall_texture.size(), true);
		}

		// add walls
		const int texShift = floor_texture.size();
		for (vector<RectWall>::const_iterator rwi = room.rwalls.begin(); rwi != room.rwalls.end(); ++rwi)
			scene.addRectWall(*rwi, rwi->texture() + texShift);
		for (vector< TriWall>::const_iterator twi = room.twalls.begin(); twi != room.twalls.end(); ++twi)
			scene.addTriWall (*twi, twi->texture() + texShift);
		for (vector<CircWall>::const_iterator cwi = room.cwalls.begin(); cwi != room.cwalls.end(); ++cwi)
			scene.addCircWall(*cwi, cwi->texture() + texShift);

		scene.setClipping(0, 0, plw, plh);
		scene.clipAll();

		// prepare the textures
		vector<TextureData> textures;

		TextureData backupTexture;
		TextureData td;
		if (floor_texture.front())
			backupTexture.setTexture(floor_texture.front());
		else
			backupTexture.setSolid(col[COLGROUND]);
		for (vector<Bitmap>::const_iterator ti = floor_texture.begin(); ti != floor_texture.end(); ++ti) {
			if (*ti) { td.setTexture(*ti); textures.push_back(td); }
			else textures.push_back(backupTexture);
		}

		if (wall_texture.front())
			backupTexture.setTexture(wall_texture.front());
		else
			backupTexture.setSolid(col[COLWALL]);
		for (vector<Bitmap>::const_iterator ti = wall_texture.begin(); ti != wall_texture.end(); ++ti) {
			if (*ti) { td.setTexture(*ti); textures.push_back(td); }
			else textures.push_back(backupTexture);
		}

		for (vector< pair<int, const spoint_t*> >::const_iterator fi = flags.begin(); fi != flags.end(); ++fi) {
			td.setFlagmarker(teamcol[fi->first], fi->second->x * scr_mul, fi->second->y * scr_mul, flagpos_radius * scr_mul);	// note: assumes 0,0,1. scaling
			textures.push_back(td);
		}
		// draw
		Texturizer tex(roombg, 0, 0, textures);
		scene.render(tex);
		tex.finalize();
	}
	else {
		// draw floor
		if (floor_texture.front()) {
			drawing_mode(DRAW_MODE_COPY_PATTERN, floor_texture.front(), 0, 0);
			rectfill(roombg, 0, 0, roombg->w - 1, roombg->h - 1, col[COLGROUND]);
			solid_mode();
		}
		else
			clear_to_color(roombg, col[COLGROUND]);
		predraw_room_ground(room);
		// draw flag position marks
		for (vector< pair<int, const spoint_t*> >::const_iterator fi = flags.begin(); fi != flags.end(); ++fi)
			draw_flagpos_mark(fi->first, fi->second->x, fi->second->y);
		// draw walls
		predraw_room_walls(room);
	}
	for (vector< pair<int, const spoint_t*> >::const_iterator si = spawns.begin(); si != spawns.end(); ++si)
		circlefill(roombg, scale(si->second->x), scale(si->second->y), scale(PLAYER_RADIUS), teamcol[si->first]);
	if (grid) {
		for (int y = 1; y < 12; ++y)
			hline(roombg, 0, scale(plh * y / 12.), scale(plw), y == 6 ? col[COLYELLOW] : col[COLWHITE]);
		for (int x = 1; x < 16; ++x)
			vline(roombg, scale(plw * x / 16.), 0, scale(plh), x == 8 ? col[COLYELLOW] : col[COLWHITE]);
	}
	if (TEST_FALL_ON_WALL)
		for (int y = 0; y < plh; y += 2)
			for (int x = 0; x < plw; x += 2)
				putpixel(roombg, scale(x), scale(y), room.fall_on_wall(x, y, 15) ? makecol(255, 0, 0) : makecol(255, 255, 255));
	draw_minimap_background();
	release_bitmap(background);
}

void Graphics::draw_empty_background() {
	clear_to_color(drawbuf, 0);
}

void Graphics::draw_background() {
	blit(background, drawbuf, 0, 0, 0, 0, background->w, background->h);
}

void Graphics::predraw_room_ground(const Room& room) {
	draw_room_ground(roombg, room, 0, 0, scr_mul, col[COLGROUND], true);
}

void Graphics::draw_room_ground(BITMAP* buffer, const Room& room, float x, float y, float scale, int color, bool texture) {
	for (vector<RectWall>::const_iterator rwi = room.rground.begin(); rwi != room.rground.end(); ++rwi)
		draw_rect_wall(buffer, *rwi, x, y, scale, color, texture ? get_floor_texture(rwi->texture()) : 0);
	for (vector<TriWall>::const_iterator twi = room.tground.begin(); twi != room.tground.end(); ++twi)
		draw_tri_wall(buffer, *twi, x, y, scale, color, texture ? get_floor_texture(twi->texture()) : 0);
	for (vector<CircWall>::const_iterator cwi = room.cground.begin(); cwi != room.cground.end(); ++cwi)
		draw_circ_wall(buffer, *cwi, x, y, scale, color, texture ? get_floor_texture(cwi->texture()) : 0);
}

void Graphics::predraw_room_walls(const Room& room) {
	draw_room_walls(roombg, room, 0, 0, scr_mul, col[COLWALL], true);
}

void Graphics::draw_room_walls(BITMAP* buffer, const Room& room, float x, float y, float scale, int color, bool texture) {
	for (vector<RectWall>::const_iterator rwi = room.rwalls.begin(); rwi != room.rwalls.end(); ++rwi)
		draw_rect_wall(buffer, *rwi, x, y, scale, color, texture ? get_wall_texture(rwi->texture()) : 0);
	for (vector<TriWall>::const_iterator twi = room.twalls.begin(); twi != room.twalls.end(); ++twi)
		draw_tri_wall(buffer, *twi, x, y, scale, color, texture ? get_wall_texture(twi->texture()) : 0);
	for (vector<CircWall>::const_iterator cwi = room.cwalls.begin(); cwi != room.cwalls.end(); ++cwi)
		draw_circ_wall(buffer, *cwi, x, y, scale, color, texture ? get_wall_texture(cwi->texture()) : 0);
}

void Graphics::draw_rect_wall(BITMAP* buffer, const RectWall& wall, float x0, float y0, float scale, int color, BITMAP* texture) {
	if (texture)
		drawing_mode(DRAW_MODE_COPY_PATTERN, texture, 0, 0);
	rectfill(buffer, iround(x0 + scale * wall.x1()), iround(y0 + scale * wall.y1()),
					 iround(x0 + scale * wall.x2()), iround(y0 + scale * wall.y2()), color);
	if (texture)
		solid_mode();
}

void Graphics::draw_tri_wall(BITMAP* buffer, const TriWall& wall, float x0, float y0, float scale, int color, BITMAP* texture) {
	if (texture)
		drawing_mode(DRAW_MODE_COPY_PATTERN, texture, 0, 0);
	triangle(buffer,
		iround(x0 + scale * wall.x1()), iround(y0 + scale * wall.y1()),
		iround(x0 + scale * wall.x2()), iround(y0 + scale * wall.y2()),
		iround(x0 + scale * wall.x3()), iround(y0 + scale * wall.y3()), color);
	if (texture)
		solid_mode();
}

void Graphics::draw_circ_wall(BITMAP* buffer, const CircWall& wall, float x0, float y0, float scale, int color, BITMAP* texture) {
	const float x = wall.X();
	const float y = wall.Y();
	const float ro = wall.radius();
	const float ri = wall.radius_in();
	const float* const angle = wall.angles();
	if (ri == 0 && angle[0] == angle[1]) {	// simple filled circle
		if (texture)
			drawing_mode(DRAW_MODE_COPY_PATTERN, texture, 0, 0);
		circlefill(buffer, iround(x0 + scale * x), iround(y0 + scale * y), iround(scale * ro), color);
		if (texture)
			solid_mode();
		return;
	}
	// ring or sector
	Bitmap cbuff = create_bitmap(iround(2 * scale * ro) + 1, iround(2 * scale * ro) + 1);
	nAssert(cbuff);
	const int transparent = bitmap_mask_color(cbuff);
	clear_to_color(cbuff, transparent);
	if (texture)
		drawing_mode(DRAW_MODE_COPY_PATTERN, texture, static_cast<int>(scale * (ro - x)), static_cast<int>(scale * (ro - y)));
	circlefill(cbuff, iround(scale * ro), iround(scale * ro), iround(scale * ro), color);
	if (texture)
		solid_mode();
	if (ri > 0)						// ring
		circlefill(cbuff, iround(scale * ro), iround(scale * ro), iround(scale * ri - 1), transparent);
	if (angle[0] != angle[1]) {		// sector
		const double vx[] = { wall.angle_vector_1().first, wall.angle_vector_2().first };
		const double vy[] = { wall.angle_vector_1().second, wall.angle_vector_2().second };
		// remove unnecessary   2 1
		// quarters             3 4
		float ang1 = angle[0];
		float ang2 = angle[1];
		if (ang1 >= 90 && (ang1 <= ang2 || ang2 == 0))	// quarter 1
			rectfill(cbuff, iround(scale * ro), 0, iround(scale * 2 * ro), iround(scale * ro), transparent);
		rotate_angle(ang1, 90);
		rotate_angle(ang2, 90);
		if (ang1 >= 90 && (ang1 <= ang2 || ang2 == 0))	// quarter 2
			rectfill(cbuff, 0, 0, iround(scale * ro), iround(scale * ro), transparent);
		rotate_angle(ang1, 90);
		rotate_angle(ang2, 90);
		if (ang1 >= 90 && (ang1 <= ang2 || ang2 == 0))	// quarter 3
			rectfill(cbuff, 0, iround(scale * ro), iround(scale * ro), iround(scale * 2 * ro), transparent);
		rotate_angle(ang1, 90);
		rotate_angle(ang2, 90);
		if (ang1 >= 90 && (ang1 <= ang2 || ang2 == 0))	// quarter 4
			rectfill(cbuff, iround(scale * ro), iround(scale * ro), iround(scale * 2 * ro), iround(scale * 2 * ro), transparent);
		// remove the rest unnecessary sectors of the circle
		const float k = 1.5;
		float diff = angle[1] - angle[0];
		if (diff < 0)
			diff += 360;
		if (vx[0] * vx[1] > 0 && vy[0] * vy[1] > 0 && diff > 90) {	// remove a sector (<90°) between the angles
			triangle(cbuff, iround(scale * ro), iround(scale * ro),
					iround(scale * (ro + k * vx[0] * ro)), iround(scale * (ro + k * (-vy[0]) * ro)),
					iround(scale * (ro + k * vx[1] * ro)), iround(scale * (ro + k * (-vy[1]) * ro)), transparent);
		}
		else {											// remove sectors between the angles and n·90°
			for (int i = 0; i < 2; i++) {
				int tx, ty;
				if (angle[i] < 90) {
					tx = 0 + i;
					ty = 1 - i;
				}
				else if (angle[i] > 270) {
					tx = -1 + i;
					ty = 0 + i;
				}
				else if (angle[i] > 180 && angle[i] < 270) {
					tx = 0 - i;
					ty = -1 + i;
				}
				else if (angle[i] > 90 && angle[i] < 180) {
					tx = 1 - i;
					ty = 0 - i;
				}
				else {
					tx = 0;
					ty = 0;
				}
				if (tx != 0 || ty != 0)
					triangle(cbuff, iround(scale * ro), iround(scale * ro),
						iround(scale * (ro + k * vx[i] * ro)), iround(scale * (ro + k * (-vy[i]) * ro)),
						iround(scale * (ro + k * tx * ro)), iround(scale * (ro + k * (-ty) * ro)), transparent);
			}
		}
		// draw back removed lines at n·90°
		if (texture)
			drawing_mode(DRAW_MODE_COPY_PATTERN, texture, static_cast<int>(scale * (ro - x)), static_cast<int>(scale * (ro - y)));
		for (int i = 0; i < 2; i++) {
			if (angle[i] == 0)
				vline(cbuff, iround(scale * ro), iround(scale * (ro - ri)), 0, color);
			else if (angle[i] == 90)
				hline(cbuff, iround(scale * (ro + ri)), iround(scale * ro), iround(scale * 2 * ro), color);
			else if (angle[i] == 180)
				vline(cbuff, iround(scale * ro), iround(scale * (ro + ri)), iround(scale * 2 * ro), color);
			else if (angle[i] == 270)
				hline(cbuff, iround(scale * (ro - ri)), iround(scale * ro), 0, color);
		}
	}
	masked_blit(cbuff, buffer, 0, 0, iround(x0 + scale * (x - ro)), iround(y0 + scale * (y - ro)), cbuff->w, cbuff->h);
	solid_mode();
}

//draw a flag  team 0/1   x, y: coord relative to playarea
void Graphics::draw_flag(int team, int x, int y) {
	x = scale(x);
	y = scale(y);
	const Bitmap& sprite = flag_sprite[team];
	if (sprite) {
		draw_sprite(drawbuf, sprite, plx + x - sprite->w / 2, ply + y - sprite->h / 2);
		return;
	}
	y += scale(20);
	//draw flagpole
	rectfill(drawbuf,
		plx + x - scale(3),
		ply + y - scale(40),
		plx + x + scale(3),
		ply + y,
		col[COLYELLOW]
	);
	//draw the flag itself
	rectfill(drawbuf,
		plx + x,
		ply + y - scale(38),
		plx + x + scale(20),
		ply + y - scale(20),
		teamcol[team]
	);
}

// Minimap functions

void Graphics::draw_mini_flag(int team, const Flag& flag, const Map& map) {
	const double px = static_cast<double>(flag.position().px * plw + flag.position().x) / (plw * map.w);
	const double py = static_cast<double>(flag.position().py * plh + flag.position().y) / (plh * map.h);
	const int pix = static_cast<int>(mmx + minimap_start_x + 1 + px * (minimap_w - 2));
	const int piy = static_cast<int>(mmy + minimap_start_y + 1 + py * (minimap_h - 2));
	const int scl = minimap_place_w;
	//draw flagpole
	rectfill(drawbuf, pix, piy - scl / 32, pix + scl / 160 - 1, piy, col[COLYELLOW]);
	//draw the flag itself
	rectfill(drawbuf, pix + 1, piy - scl / 32, pix + scl / 32, piy - scl / 80, teamcol[team]);
}

void Graphics::draw_minimap_player(const Map& map, const ClientPlayer& player) {
	const pair<int, int> coords = calculate_minimap_coordinates(map, player);
	const int a = scale(1);
	const int b = a / 2;
	rectfill(drawbuf, coords.first - a, coords.second - a, coords.first + a, coords.second + a, teamcol[player.team()]);
	rectfill(drawbuf, coords.first - b, coords.second - b, coords.first + b, coords.second + b, col[player.color()]);
}

void Graphics::draw_minimap_me(const Map& map, const ClientPlayer& player, double time) {
	const pair<int, int> coords = calculate_minimap_coordinates(map, player);
	if (static_cast<int>(time * 15) % 3 > 0) {
		circlefill(drawbuf, coords.first, coords.second, scale(2), col[COLYELLOW]);
		circlefill(drawbuf, coords.first, coords.second, scale(1), teamlcol[player.team()]);
	}
	else
		circlefill(drawbuf, coords.first, coords.second, scale(2), 0);
}

pair<int, int> Graphics::calculate_minimap_coordinates(const Map& map, const ClientPlayer& player) const {
	const double px = (player.roomx * plw + player.lx) / static_cast<double>(plw * map.w);
	const double py = (player.roomy * plh + player.ly) / static_cast<double>(plh * map.h);
	const int x = static_cast<int>(mmx + 1 + px * (minimap_w - 2)) + minimap_start_x;
	const int y = static_cast<int>(mmy + 1 + py * (minimap_h - 2)) + minimap_start_y;
	return pair<int, int>(x, y);
}

void Graphics::draw_minimap_room(const Map& map, int rx, int ry) {
	const int x1 = mmx + minimap_start_x + 1 + rx * (minimap_w - 1) / map.w;
	const int y1 = mmy + minimap_start_y + 1 + ry * (minimap_h - 1) / map.h;
	const int x2 = mmx + minimap_start_x + 1 + (rx + 1) * (minimap_w - 1) / map.w - 1;
	const int y2 = mmy + minimap_start_y + 1 + (ry + 1) * (minimap_h - 1) / map.h - 1;
	drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
	set_trans_blender(0, 0, 0, 0x38);
	rectfill(drawbuf, x1, y1, x2, y2, col[COLFOGOFWAR]);
	solid_mode();
}

void Graphics::draw_minimap_background() {
	if (show_minimap)
		blit(minibg, background, 0, 0, mmx, mmy, minibg->w, minibg->h);
}

void Graphics::update_minimap_background(const Map& map) {
	update_minimap_background(minibg, map);
}

void Graphics::update_minimap_background(BITMAP* buffer, const Map& map, bool save_map_pic) {
	// black background
	clear_to_color(buffer, 0);

	if (map.w == 0 || map.h == 0)
		return;

	// Calculate new minimap size.
	if (map.w * 4 * minimap_place_h > map.h * 3 * minimap_place_w) {
		minimap_w = minimap_place_w;
		minimap_h = static_cast<int>(static_cast<float>((minimap_w - 1) * map.h * 3) / map.w / 4. + 1.);	// important not to round
	}
	else {
		minimap_h = minimap_place_h;
		minimap_w = static_cast<int>(static_cast<float>((minimap_h - 1) * map.w * 4) / map.h / 3. + 1.);	// important not to round
	}

	minimap_start_x = (minimap_place_w - minimap_w) / 2;
	minimap_start_y = (minimap_place_h - minimap_h) / 2;
	const float room_w = float(minimap_w - 1.) / map.w;	// use -1. (not 2) to have half a pixel under the green border on every edge; this is to compensate for error in the value of minimap_? so there's no gap
	const float room_h = float(minimap_h - 1.) / map.h;
	const int room_border_col = save_map_pic ? col[COLMENUGRAY] : makecol(0x30, 0x30, 0x30);

	const double maxx = plw * map.w;
	const double maxy = plh * map.h;
	float xmul = float(minimap_w - 1.) / maxx, ymul = float(minimap_h - 1.) / maxy;

	if (antialiasing != AA_none) {
		SceneAntialiaser scene;
		scene.setScaling(minimap_start_x + .5, minimap_start_y + .5, xmul);

		// add background
		scene.addRectangle(0, 0, maxx, maxy, 0);

		// add room boundaries
		const float halfPixw = .49999 / xmul, halfPixh = .49999 / xmul;
		for (int i = 1; i < map.w; i++)
			scene.addRectangle(plw * i - halfPixw, 0, plw * i + halfPixw, maxy, 2);
		for (int i = 1; i < map.h; i++)
			scene.addRectangle(0, plh * i - halfPixh, maxx, plh * i + halfPixh, 2);

		// add walls
		for (int y = 0; y < map.h; y++) {
			const float by = minimap_start_y + .5 + y * plh * xmul;
			for (int x = 0; x < map.w; x++) {
				const float bx = minimap_start_x + .5 + x * plw * xmul;
				scene.setScaling(bx, by, xmul);
				scene.setClipping(0, 0, plw, plh);
				const Room& room = map.room[x][y];
				for (vector<RectWall>::const_iterator rwi = room.rwalls.begin(); rwi != room.rwalls.end(); ++rwi)
					scene.addRectWallClipped(*rwi, 1);
				for (vector< TriWall>::const_iterator twi = room.twalls.begin(); twi != room.twalls.end(); ++twi)
					scene.addTriWallClipped (*twi, 1);
				for (vector<CircWall>::const_iterator cwi = room.cwalls.begin(); cwi != room.cwalls.end(); ++cwi)
					scene.addCircWallClipped(*cwi, 1);
			}
		}

		// draw
		vector<TextureData> colors;
		TextureData td;
		td.setSolid(0);					colors.push_back(td);	// ground
		td.setSolid(col[COLDARKGREEN]);	colors.push_back(td);	// walls
		td.setSolid(room_border_col);	colors.push_back(td);	// room boundaries
		Texturizer tex(buffer, 0, 0, colors);
		scene.render(tex);
		tex.finalize();
	}
	else {
		//draw room boundaries
		for (int i = 1; i < map.w; i++)
			vline(buffer, static_cast<int>(minimap_start_x + 1 + room_w * i), minimap_start_y, minimap_start_y + minimap_h, room_border_col);
		for (int i = 1; i < map.h; i++)
			hline(buffer, minimap_start_x, static_cast<int>(minimap_start_y + 1 + room_h * i), minimap_start_x + minimap_w, room_border_col);

		//draw solid walls
		for (int y = 0; y < map.h; y++) {
			float by = minimap_start_y + 1 + y * plh * ymul;
			for (int x = 0; x < map.w; x++) {
				float bx = minimap_start_x + 1 + x * plw * xmul;
				set_clip_rect(buffer, static_cast<int>(bx), static_cast<int>(by), static_cast<int>(bx + room_w), static_cast<int>(by + room_h));
				draw_room_walls(buffer, map.room[x][y], bx, by, xmul, col[COLDARKGREEN], false);
				set_clip_rect(buffer, 0, 0, buffer->w, buffer->h);
			}
		}
	}

	//green border
	rect(buffer, minimap_start_x, minimap_start_y, minimap_start_x + minimap_w - 1, minimap_start_y + minimap_h - 1, col[COLGREEN]);

	// draw bases
	Bitmap backup = create_bitmap(minimap_place_w, minimap_place_w);
	nAssert(backup);

	for (int ry = 0; ry < map.h; ++ry)
		for (int rx = 0; rx < map.w; ++rx) {
			// save map
			blit(buffer, backup, 0, 0, 0, 0, buffer->w, buffer->h);
			bool flag[] = { false, false };
			for (int t = 0; t < 2; ++t)
				for (vector<spoint_t>::const_iterator fi = map.tinfo[t].flags.begin(); fi != map.tinfo[t].flags.end(); ++fi)
					if (fi->px == rx && fi->py == ry) {
						flag[t] = true;
						break;
					}
			vector<spoint_t> failure[2];
			if (flag[0] ^ flag[1]) {		// only one team's flags in the room
				const int t = flag[0] ? 0 : 1;
				for (vector<spoint_t>::const_iterator fi = map.tinfo[t].flags.begin(); fi != map.tinfo[t].flags.end(); ++fi) {
					if (fi->px != rx || fi->py != ry)
						continue;
					const int px = static_cast<int>(minimap_start_x + 1 + (fi->px * plw + fi->x) / maxx * (minimap_w - 2));
					const int py = static_cast<int>(minimap_start_y + 1 + (fi->py * plh + fi->y) / maxy * (minimap_h - 2));
					const int c = getpixel(buffer, px, py);
					if (c == 0)
						floodfill(buffer, px, py, teamdcol[t]);
					else if (c != teamdcol[t])
						failure[t].push_back(*fi);
				}
			}
			else if (flag[0] && flag[1]) {	// both team's flags in the room
				for (int t = 0; t < 2; ++t)
					for (vector<spoint_t>::const_iterator fi = map.tinfo[t].flags.begin(); fi != map.tinfo[t].flags.end(); ++fi) {
						if (fi->px != rx || fi->py != ry)
							continue;
						const int px = static_cast<int>(minimap_start_x + 1 + (fi->px * plw + fi->x) / maxx * (minimap_w - 2));
						const int py = static_cast<int>(minimap_start_y + 1 + (fi->py * plh + fi->y) / maxy * (minimap_h - 2));
						const int c = getpixel(buffer, px, py);
						bool successful = true;
						if (c == 0)
							floodfill(buffer, px, py, teamdcol[t]);
						else if (c != teamdcol[t]) {
							failure[t].push_back(*fi);
							successful = false;
						}
						for (vector<spoint_t>::const_iterator fj = map.tinfo[1 - t].flags.begin(); successful && fj != map.tinfo[1 - t].flags.end(); ++fj) {
							if (fj->px != rx || fj->py != ry)
								continue;
							const int px = static_cast<int>(minimap_start_x + 1 + (fj->px * plw + fj->x) / maxx * (minimap_w - 2));
							const int py = static_cast<int>(minimap_start_y + 1 + (fj->py * plh + fj->y) / maxy * (minimap_h - 2));
							const int c = getpixel(buffer, px, py);
							if (c == teamdcol[t]) {
								failure[t].push_back(*fi);
								//failure[1 - t].push_back(*fj);
								successful = false;
								// restore map
								blit(backup, buffer, 0, 0, 0, 0, buffer->w, buffer->h);
								break;
							}
						}
						if (successful)	// save map
							blit(buffer, backup, 0, 0, 0, 0, buffer->w, buffer->h);
					}
			}
			if (failure[0].empty() && failure[1].empty())
				continue;
			const int xmin = static_cast<int>(minimap_start_x + 2 + room_w * rx);
			const int xmax = static_cast<int>(minimap_start_x + room_w * (rx + 1));
			const int ymin = static_cast<int>(minimap_start_y + 2 + room_h * ry);
			const int ymax = static_cast<int>(minimap_start_y + room_h * (ry + 1));
			for (int y = ymin; y <= ymax; ++y) {
				const float roomy = float(y + 1 - ymin) / float(room_h) * plh;
				for (int x = xmin; x <= xmax; ++x) {
					if (getpixel(buffer, x, y) != 0)
						continue;
					const float roomx = float(x + 1 - xmin) / float(room_w) * plw;
					double dist_r2 = INT_MAX;
					for (vector<spoint_t>::const_iterator fi = failure[0].begin(); fi != failure[0].end(); ++fi)
						dist_r2 = min(dist_r2, pow(fi->y - roomy, 2) + pow(fi->x - roomx, 2));
					double dist_b2 = INT_MAX;
					for (vector<spoint_t>::const_iterator fi = failure[1].begin(); fi != failure[1].end(); ++fi)
						dist_b2 = min(dist_b2, pow(fi->y - roomy, 2) + pow(fi->x - roomx, 2));
					const int color = (dist_r2 < dist_b2) ? teamdcol[0] : teamdcol[1];
					putpixel(buffer, x, y, color);
				}
			}
		}

	backup.free();

	//draw circles (or flags) to flag positions
	for (int t = 0; t <= 2; ++t) {
		const vector<spoint_t>& flags = (t == 2 ? map.wild_flags : map.tinfo[t].flags);
		for (vector<spoint_t>::const_iterator fi = flags.begin(); fi != flags.end(); ++fi) {
			const int px = static_cast<int>(minimap_start_x + 1 + (fi->px * plw + fi->x) / maxx * (minimap_w - 2));
			const int py = static_cast<int>(minimap_start_y + 1 + (fi->py * plh + fi->y) / maxy * (minimap_h - 2));
			if (save_map_pic) {
				//draw the flagpole
				rectfill(buffer, px, py - static_cast<int>(room_w / 12), px + static_cast<int>(room_w / 60) - 1, py, col[COLYELLOW]);
				//draw the flag
				rectfill(buffer, px + static_cast<int>(room_w / 60), py - static_cast<int>(room_w / 12),
					px + static_cast<int>(room_w / 12), py - static_cast<int>(room_w / 30), teamcol[t]);
			}
			else
				circle(buffer, px, py, minimap_place_w / 50, teamcol[t]);
		}
	}
}

//draws a basic player object
void Graphics::draw_player(int x, int y, int team, int pli, int gundir, double hitfx, bool item_quad, int alpha, double time) {
	x = scale(x);
	y = scale(y);
	int pc1 = teamcol[team];
	int pc2 = col[pli];
	// test different colours:
	//pc2 = col[(int)time / 2 % 16];
	//blink player when hit
	double deltafx = hitfx - time;
	if (deltafx > 0) {
		int rgb = static_cast<int>(70.0 + deltafx * 600.0);  // var 180
		pc1 = pc2 = makecol(rgb, rgb, rgb);
	}
	else if (item_quad) {
		//pisca branco
		if (static_cast<int>(time * 10) % 2) {
			pc1 = col[COLWHITE];
			pc2 = col[COLCYAN];
		}
	}

	const int player_radius = scale(PLAYER_RADIUS);

	BITMAP* sprite = 0;
	if (item_quad && player_sprite_power && static_cast<int>(time * 10) % 2)
		sprite = player_sprite_power;
	else {
		nAssert(team == 0 || team == 1);
		nAssert(pli >= 0 && pli < MAX_PLAYERS / 2);
		sprite = player_sprite[team][pli];
	}
	if (sprite) {
		if (alpha < 255)
			rotate_trans_sprite(drawbuf, sprite, plx + x, ply + y, itofix(gundir * 32), alpha);
		else
			rotate_sprite(drawbuf, sprite, plx + x - sprite->w / 2, ply + y - sprite->h / 2, itofix(gundir * 32));
		return;
	}

	if (alpha < 255) {
		set_trans_blender(0, 0, 0, alpha);
		drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
	}

	// outer color: team color
	circlefill(drawbuf, plx + x, ply + y, player_radius, pc1);
	// inner color: self color
	circlefill(drawbuf, plx + x, ply + y, player_radius * 2 / 3, pc2);
	// gun direction
	int xg, yg;
	switch (gundir) {
		case 0: xg = 40; yg =  0; break;
		case 1: xg = 28; yg = 28; break;
		case 2: xg =  0; yg = 40; break;
		case 3: xg =-28; yg = 28; break;
		case 4: xg =-40; yg =  0; break;
		case 5: xg =-28; yg =-28; break;
		case 6: xg =  0; yg =-40; break;
		case 7: xg = 28; yg =-28; break;
		default: xg = 0; yg =  0; break;
	}
	xg = scale(xg * 0.7);
	yg = scale(yg * 0.7);
	// draw the gun
	switch (gundir) {
		case 0: case 4:
			rectfill(drawbuf, plx + x + xg / 2, ply + y + yg - 1, plx + x + xg, ply + y + yg + 1, pc1);
			break;
		case 2: case 6:
			rectfill(drawbuf, plx + x + xg - 1, ply + y + yg / 2, plx + x + xg + 1, ply + y + yg, pc1);
			break;
		case 1: case 5:
			line(drawbuf, plx + x + xg / 2 + 0, ply + y + yg / 2 + 1, plx + x + xg - 1, ply + y + yg + 0, pc1);
			line(drawbuf, plx + x + xg / 2 + 0, ply + y + yg / 2 + 0, plx + x + xg + 0, ply + y + yg + 0, pc1);
			line(drawbuf, plx + x + xg / 2 + 1, ply + y + yg / 2 + 0, plx + x + xg + 0, ply + y + yg - 1, pc1);
			break;
		case 3: case 7:
			line(drawbuf, plx + x + xg / 2 + 0, ply + y + yg / 2 - 1, plx + x + xg - 1, ply + y + yg + 0, pc1);
			line(drawbuf, plx + x + xg / 2 + 0, ply + y + yg / 2 + 0, plx + x + xg + 0, ply + y + yg + 0, pc1);
			line(drawbuf, plx + x + xg / 2 + 1, ply + y + yg / 2 + 0, plx + x + xg + 0, ply + y + yg + 1, pc1);
			break;
	}

	solid_mode();
}

void Graphics::set_alpha_channel(BITMAP* bitmap, BITMAP* alpha) {
	set_write_alpha_blender();
	drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
	if (alpha)
		draw_trans_sprite(bitmap, alpha, 0, 0);
	else	// maximum alpha level
		rectfill(bitmap, 0, 0, bitmap->w - 1, bitmap->h - 1, 255);
	solid_mode();
}

void Graphics::rotate_trans_sprite(BITMAP* bmp, BITMAP* sprite, int x, int y, fixed angle, int alpha) {	// x,y are destination coords of the sprite center
	// make room so that rotating won't clip the corners off
	//const int width  = gundir % 2 ? sprite->w : static_cast<int>(ceil(1.415 * sprite->w));
	//const int height = gundir % 2 ? sprite->h : static_cast<int>(ceil(1.415 * sprite->h));
	nAssert(sprite);
	nAssert(sprite->w == sprite->h);	// if otherwise, would have to use max(sprite->w, sprite->h) below, and use more complex coords in rotate
	const int size = sprite->h + sprite->h / 2;
	Bitmap buffer = create_bitmap(size, size);
	nAssert(buffer);
	clear_to_color(buffer, bitmap_mask_color(buffer));
	rotate_sprite(buffer, sprite, sprite->h / 4, sprite->h / 4, angle);
	drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
	set_trans_blender(0, 0, 0, alpha);
	draw_trans_sprite(bmp, buffer, x - buffer->w / 2, y - buffer->h / 2);
	solid_mode();
}

void Graphics::rotate_alpha_sprite(BITMAP* bmp, BITMAP* sprite, int x, int y, fixed angle) {	// x,y are destination coords of the sprite center
	// make room so that rotating won't clip the corners off
	//const int width  = gundir % 2 ? sprite->w : static_cast<int>(ceil(1.415 * sprite->w));
	//const int height = gundir % 2 ? sprite->h : static_cast<int>(ceil(1.415 * sprite->h));
	nAssert(bitmap_color_depth(sprite) == 32);
	nAssert(sprite->w == sprite->h);	// if otherwise, would have to use max(sprite->w, sprite->h) below, and use more complex coords in rotate
	const int size = sprite->h + sprite->h / 2;
	Bitmap buffer = create_bitmap_ex(32, size, size);
	nAssert(buffer);
	clear_to_color(buffer, bitmap_mask_color(buffer));
	rotate_sprite(buffer, sprite, sprite->h / 4, sprite->h / 4, angle);
	drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
	set_alpha_blender();
	draw_trans_sprite(bmp, buffer, x - buffer->w / 2, y - buffer->h / 2);
	solid_mode();
}

void Graphics::draw_player_dead(const ClientPlayer& player) {
	const int x = scale(player.lx);
	const int y = scale(player.ly);
	BITMAP* sprite = dead_sprite[player.team()];
	if (sprite)
		rotate_alpha_sprite(drawbuf, sprite, plx + x, ply + y, itofix(player.gundir * 32));
	else {
		drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
		set_trans_blender(0, 0, 0, 90);
		const int plrScale = scale(PLAYER_RADIUS * 10);
		ellipsefill(drawbuf, plx + x - plrScale / 25, ply + y + plrScale / 20, plrScale / 9, plrScale / 10, col[COLRED]);
		ellipsefill(drawbuf, plx + x                , ply + y - plrScale / 30, plrScale / 8, plrScale / 10, col[COLRED]);
		ellipsefill(drawbuf, plx + x + plrScale / 50, ply + y + plrScale / 40, plrScale / 8, plrScale / 10, col[COLRED]);
		solid_mode();
	}
}

void Graphics::draw_virou_sorvete(int x, int y) {
	x = scale(x);
	y = scale(y);
	if (ice_cream)
		draw_sprite(drawbuf, ice_cream, plx + x - ice_cream->w / 2, ply + y - ice_cream->h / 2);
	else {
		ellipsefill(drawbuf, plx + x, ply + y, 6, 15, col[COLORA]);
		circlefill(drawbuf, plx + x - 8, ply + y - 10, 8, col[COLBLUE]);
		circlefill(drawbuf, plx + x + 8, ply + y - 10, 8, col[COLMAG]);
		circlefill(drawbuf, plx + x + 0, ply + y - 20, 8, col[COLGREEN]);
		textout_centre_ex(drawbuf, font, "VIROU", plx + x + 0, ply + y - 48, col[COLWHITE], -1);
		textout_centre_ex(drawbuf, font, "SORVETE!", plx + x + 0, ply + y - 38, col[COLWHITE], -1);
	}
}

void Graphics::draw_gun_explosion(int x, int y, int rad) {
	x = scale(x);
	y = scale(y);
	circle(drawbuf, plx + x, ply + y, scale(rad), makecol(rand() % 256, rand() % 256, rand() % 256));
}

void Graphics::draw_deathbringer_smoke(int x, int y, double time) {
	x = scale(x);
	y = scale(y);
	int alpha = 120 - static_cast<int>(time * 200.0);
	alpha = max(alpha, 0);
	drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
	set_trans_blender(0, 0, 0, alpha);
	const double drad = 3.0 + 9.0 * (0.6 - time);
	const int rad = scale(drad);
	const int subdist = scale(96.0 - drad * 8.0);
	circlefill(drawbuf, plx + x, ply + y - subdist, rad, 0);
	solid_mode();
}

void Graphics::draw_deathbringer(int x, int y, int team, double time) {
	x = scale(x);
	y = scale(y);
	//radius
	int rad;
	if (time < 1.0)
		rad = scale(time * 100);
	else
		rad = scale(100 + (time - 1.0) * (time - 1.0) * 800);
	int maxxd = max(x, scale(plw) - x);
	int maxyd = max(y, scale(plh) - y);
	if (maxxd * maxxd + maxyd * maxyd >= rad * rad) {
		set_clip_rect(drawbuf, plx, ply, plx + scale(plw), ply + scale(plh));
		//brightening ring
		for (int e = 0; e < scale(30); e++, rad++) {
			int co;
			if (team == 0)
				co = makecol(14 + static_cast<int>(8 * e / scr_mul), 0, 0);
			else
				co = makecol(0, 0, 14 + static_cast<int>(8 * e / scr_mul));
			circle(drawbuf, plx + x, ply + y, rad, co);
		}
		//darkening ring
		for (int e = 0; e < scale(10); e++, rad++) {
			int co;
			if (team == 0)
				co = makecol(255 - static_cast<int>(14 * e / scr_mul), 0, 0);
			else
				co = makecol(0, 0, 255 - static_cast<int>(14 * e / scr_mul));
			circle(drawbuf, plx + x, ply + y, rad, co);
			circle(drawbuf, plx + x + 1, ply + y, rad, co);
			circle(drawbuf, plx + x, ply + y + 1, rad, co);
		}
		set_clip_rect(drawbuf, 0, 0, drawbuf->w - 1, drawbuf->h - 1);
	}
}

void Graphics::draw_deathbringer_affected(int x, int y, int team) {
	x = scale(x);
	y = scale(y);
	drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
	set_trans_blender(0, 0, 0, 128);
	for (int i = 0; i < 5; i++)
		circlefill(drawbuf, plx + x + scale(rand() % 40 - 20), ply + y + scale(rand() % 40 - 20), scale(15), teamcol[team]);
	for (int i = 0; i < 5; i++)
		circlefill(drawbuf, plx + x + scale(rand() % 40 - 20), ply + y + scale(rand() % 40 - 20), scale(15), 0);
	solid_mode();
}

void Graphics::draw_deathbringer_carrier_effect(int x, int y) {
	x = scale(x);
	y = scale(y);
	set_clip_rect(drawbuf, plx, ply, plx + scale(plw), ply + scale(plh));
	//darken ground
	drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
	for (int r = scale(50); r >= 0; r -= 5) {
		set_trans_blender(0, 0, 0, max(50 - static_cast<int>(r / scr_mul), 0));
		circlefill(drawbuf, plx + x, ply + y, r, 0);
	}
	solid_mode();
	set_clip_rect(drawbuf, 0, 0, drawbuf->w - 1, drawbuf->h - 1);
}

void Graphics::draw_shield(int x, int y, int r, int alpha, int team, int direction) {
	x = scale(x);
	y = scale(y);
	r = scale(r);
	if ((team == 0 || team == 1) && shield_sprite[team]) {
		BITMAP* sprite = shield_sprite[team];
		if (alpha < 255)
			rotate_trans_sprite(drawbuf, sprite, plx + x, ply + y, itofix(direction * 32), alpha);
		else
			rotate_sprite(drawbuf, sprite, plx + x - sprite->w / 2, ply + y - sprite->h / 2, itofix(direction * 32));
		return;
	}
	const int v[] = { scale(3), scale(5), scale(9) };
	if (alpha < 255) {
		drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
		set_trans_blender(0, 0, 0, alpha);
	}
	if (team == 0)
		for (int i = 0, c = rand() % 256; i < 3; i++)
			ellipse(drawbuf, plx + x, ply + y, r + rand() % v[i], r + rand() % v[i], makecol(255, c, c));
	else if (team == 1)
		for (int i = 0, c = rand() % 256; i < 3; i++)
			ellipse(drawbuf, plx + x, ply + y, r + rand() % v[i], r + rand() % v[i], makecol(c, c, 255));
	else	// powerup lying on the ground
		for (int i = 0; i < 3; i++)
			ellipse(drawbuf, plx + x, ply + y, r + rand() % v[i], r + rand() % v[i], makecol(rand() % 256, rand() % 256, rand() % 256));
	solid_mode();
}

void Graphics::draw_player_name(const string& name, int x, int y, int team) {
	x = scale(x);
	y = scale(y);
	print_text_border_centre(name, plx + x, ply + y - scale(PLAYER_RADIUS + 10), col[COLWHITE], teamdcol[team], -1);
}

void Graphics::draw_rocket(const rocket_c& rocket, double time) {
	const int x = scale(rocket.x);
	const int y = scale(rocket.y);
	BITMAP* sprite = (rocket.power ? power_rocket_sprite[rocket.team] : rocket_sprite[rocket.team]);
	if (sprite)
		rotate_sprite(drawbuf, sprite, plx + x - sprite->w / 2, ply + y - sprite->h / 2, itofix(rocket.direction * 32));
	else if (rocket.power) {
		//draw rocket shadow
		ellipsefill(drawbuf, plx + x, ply + y + scale(QUAD_ROCKET_RADIUS + 8), scale(QUAD_ROCKET_RADIUS), scale(3), col[COLSHADOW]);
		//draw the rocket
		if (static_cast<int>(time * 30) % 2)
			circlefill(drawbuf, plx + x, ply + y, scale(QUAD_ROCKET_RADIUS), col[COLWHITE]);	//y-12?
		else
			circlefill(drawbuf, plx + x, ply + y, scale(QUAD_ROCKET_RADIUS), teamlcol[rocket.team]); //y-12??
	}
	else {
		//draw rocket shadow
		ellipsefill(drawbuf, plx + x, ply + y + scale(ROCKET_RADIUS + 8), scale(ROCKET_RADIUS), scale(2), col[COLSHADOW]);
		//draw the rocket
		circlefill(drawbuf, plx + x, ply + y, scale(ROCKET_RADIUS), teamcol[rocket.team]); //y-10??
	}
}

void Graphics::draw_flagpos_mark(int team, int flag_x, int flag_y) {
	drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
	const int flagpos_radius = scale(this->flagpos_radius);
	const int step = 300 / flagpos_radius;
	for (int y = scale(flag_y) - flagpos_radius; y < scale(flag_y) + flagpos_radius; y++)
		for (int x = scale(flag_x) - flagpos_radius; x < scale(flag_x) + flagpos_radius; x++) {
			const int dx = scale(flag_x) - x;
			const int dy = scale(flag_y) - y;
			const float dist = sqrt(static_cast<float>(dx * dx + dy * dy));
			if (dist > flagpos_radius)
				continue;
			const int alpha = static_cast<int>(step * (flagpos_radius - dist));
			set_trans_blender(0, 0, 0, min(alpha, 255));
			putpixel(roombg, x, y, teamcol[team]);
		}
	solid_mode();
}

void Graphics::draw_pup(const Powerup& pup, double time) {
	nAssert(pup.kind >= 0 && pup.kind < static_cast<int>(pup_sprite.size()));
	BITMAP* sprite = pup_sprite[pup.kind];
	if (sprite)
		draw_sprite(drawbuf, sprite, plx + scale(pup.x) - sprite->w / 2, ply + scale(pup.y) - sprite->h / 2);
	else
		switch (pup.kind) {
			case Powerup::pup_shield:		draw_pup_shield(pup.x, pup.y); break;
			case Powerup::pup_turbo:		draw_pup_turbo(pup.x, pup.y); break;
			case Powerup::pup_shadow:		draw_pup_shadow(pup.x, pup.y, time); break;
			case Powerup::pup_power:		draw_pup_power(pup.x, pup.y, time); break;
			case Powerup::pup_weapon:		draw_pup_weapon(pup.x, pup.y, time); break;
			case Powerup::pup_health:		draw_pup_health(pup.x, pup.y, time); break;
			case Powerup::pup_deathbringer:	draw_pup_deathbringer(pup.x, pup.y); break;
			default: nAssert(0);
		}
}

void Graphics::draw_pup_shield(int x, int y) {
	draw_shield(x, y, 14);
	circlefill(drawbuf, plx + scale(x), ply + scale(y), scale(12), col[COLGREEN]);
}

void Graphics::draw_pup_turbo(int x, int y) {
	const int r = scale(12);
	circlefill(drawbuf, plx + scale(x + rand() % 6 - 3), ply + scale(y + rand() % 6 - 3), r, col[COLDARKORA]);
	circlefill(drawbuf, plx + scale(x + rand() % 8 - 4), ply + scale(y + rand() % 8 - 4), r, col[COLORA]);
	circlefill(drawbuf, plx + scale(x + rand() % 12 - 6), ply + scale(y + rand() % 12 - 6), r, col[COLYELLOW]);
}

void Graphics::draw_pup_shadow(int x, int y, double time) {
	drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
	int alpha = static_cast<int>(time * 600.0) % 400;
	if (alpha > 200)
		alpha = 400 - alpha;
	set_trans_blender(0, 0, 0, 55 + alpha);
	circlefill(drawbuf, plx + scale(x), ply + scale(y), scale(12), col[COLMAG]);
	solid_mode();
}

void Graphics::draw_pup_power(int x, int y, double time) {
	if (static_cast<int>(time * 30) % 2)
		circlefill(drawbuf, plx + scale(x), ply + scale(y), scale(13), col[COLWHITE]);
	else
		circlefill(drawbuf, plx + scale(x), ply + scale(y), scale(11), col[COLCYAN]);
}

void Graphics::draw_pup_weapon(int x, int y, double time) {
	// rotate item
	for (int b = 0; b < 4; b++) {
		// deg: 0..360
		double deg = static_cast<int>(time * 1000) % 1000;		//thousand ticks
		deg /= 1000.0;		// normalise between 0...1
		deg *= 2 * M_PI;	// 360°
		deg += M_PI_2 * b;	// 90°

		// position
		double dx = 10 * cos(deg);
		double dy = 10 * sin(deg);

		// choose colour
		int c = 0;
		switch (b) {
			case 0: c = col[COLGREEN]; break;
			case 1: c = col[COLBLUE]; break;
			case 2: c = col[COLRED]; break;
			case 3: c = col[COLYELLOW]; break;
		}
		// draw a ball
		circlefill(drawbuf, plx + scale(x + dx), ply + scale(y + dy), scale(4), c);
	}
}

void Graphics::draw_pup_health(int x, int y, double time) {
	x = scale(x);
	y = scale(y);
	//caixa de saude pulsante
	int varia = static_cast<int>(time * 15) % 10;
	if (varia > 5)
		varia = 10 - varia;
	const int itemsize = scale(11 + varia);
	const int crossize = scale(8 + varia);
	const int crosslar = scale(3); //aria/2;
	// health box black border
	rectfill(drawbuf, plx + x - itemsize - 2, ply + y - itemsize - 2, plx + x + itemsize + 2, ply + y + itemsize + 2, 0);
	// health box
	rectfill(drawbuf, plx + x - itemsize, ply + y - itemsize, plx + x + itemsize, ply + y + itemsize, col[COLWHITE]);
	// red cross
	rectfill(drawbuf, plx + x - crossize, ply + y - crosslar, plx + x + crossize, ply + y + crosslar, col[COLRED]);
	rectfill(drawbuf, plx + x - crosslar, ply + y - crossize, plx + x + crosslar, ply + y + crossize, col[COLRED]);
}

void Graphics::draw_pup_deathbringer(int x, int y) {
	circlefill(drawbuf, plx + scale(x), ply + scale(y), scale(12), makecol(0x22, 0x33, 0x22));
}

void Graphics::draw_one_line_message(const string& message) {
	textout_centre_ex(drawbuf, font, message.c_str(), plx + plw / 2, ply + plh / 2, col[COLGREEN], -1);
}

void Graphics::draw_waiting_map_message(const string& caption, const string& map) {
	textout_centre_ex(drawbuf, font, caption.c_str(), plx + plw / 2, ply + plh / 2 + 20, col[COLGREEN], -1);
	textout_centre_ex(drawbuf, font, map.c_str(), plx + plw / 2, ply + plh / 2 + 50, col[COLORA], -1);
}

void Graphics::draw_loading_map_message(const string& text) {
	textout_centre_ex(drawbuf, font, text.c_str(), plx + plw / 2, ply + plh / 2 + 70, col[COLGREEN], -1);
}

void Graphics::draw_scores(const string& text, int team, int score1, int score2) {
	int c;
	switch (team) {
		case 0: case 1: c = teamlcol[team]; break;
		default: c = col[COLMENUGRAY]; break;
	}
	textout_centre_ex(drawbuf, font, text.c_str(), plx + plw / 2, ply + plh / 2 - 40, c, -1);
	textprintf_centre_ex(drawbuf, font, plx + plw / 2, ply + plh / 2 - 20, c, -1, "SCORE: %i - %i", score1, score2);
}

void Graphics::draw_scoreboard(const vector<ClientPlayer*>& players, const Team* teams, int maxplayers) {
	if (!show_scoreboard)
		return;

	const int y_space = SCREEN_H - sby;
	int line_h = y_space / (maxplayers + 2 + 2 + 1);	// players, 2 captions, 2 empty lines, FPS
	if (line_h > 12)
		line_h = 12;
	if (line_h < 8)
		line_h = 8;

	// captions
	ostringstream red;
	red << "Red Team:    " << setw(2) << teams[0].score() << " capt";
 	textout_ex(drawbuf, font, red.str().c_str(), sbx, sby, teamlcol[0], -1);
	ostringstream blue;
	blue << "Blue Team:   " << setw(2) << teams[1].score() << " capt";
 	textout_ex(drawbuf, font, blue.str().c_str(), sbx, sby + (maxplayers / 2 + 1) * line_h, teamlcol[1], -1);

	int line[2] = { 0, 0 };
	for (vector<ClientPlayer*>::const_iterator pi = players.begin(); pi != players.end(); ++pi) {
		const ClientPlayer& player = **pi;
		ostringstream name;
		name << player.name.substr(0, 15);
		const int pcol = col[player.color()];
		const int x = sbx;
		const int y = sby + (line[player.team()] + 1) * line_h + player.team() * (maxplayers / 2 + 1) * line_h;
		draw_scoreboard_name(name.str(), x, y, pcol);
		draw_scoreboard_points(player.stats().frags(), x + 20 * 8, y, player.team());
		line[player.team()]++;
	}
}

void Graphics::draw_scoreboard_name(const string& name, int x, int y, int pcol) {
	textout_ex(drawbuf, font, name.c_str(), x, y, pcol, -1);
}

void Graphics::draw_scoreboard_points(int points, int x, int y, int team) {
	textprintf_right_ex(drawbuf, font, x, y, teamlcol[team], -1, "%d", points);
}

void Graphics::team_statistics(const Team* teams) {
	const int line_height = 12;
	const int w = 300;
	const int h = min<int>(SCREEN_H - 40, (18 + teams[0].captures().size() + teams[1].captures().size()) * line_height);
	const int mx = SCREEN_W / 2;
	const int my = SCREEN_H / 2;
	const int x1 = mx - w / 2;
	const int y1 = my - h / 2;
	const int x2 = mx + w / 2;
	const int y2 = my + h / 2;
	const int team_captures_size = h / line_height - 17;

	if (stats_alpha >= 0) {
		if (stats_alpha < 255) {
			drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
			set_trans_blender(0, 0, 0, stats_alpha);
		}
		rectfill(drawbuf, x1, y1, x2, y2, 0);
		// caption backgrounds
		rectfill(drawbuf, x1, y1 + line_height - 4, x2, y1 + 2 * line_height, col[COLDARKGREEN]);
		rectfill(drawbuf, x1, y1 + 3 * line_height - 4, mx - 1, y1 + 4 * line_height, teamdcol[0]);
		rectfill(drawbuf, mx, y1 + 3 * line_height - 4, x2, y1 + 4 * line_height, teamdcol[1]);
		solid_mode();
	}

	textout_centre_ex(drawbuf, font, "Team stats", mx, y1 + line_height, col[COLWHITE], -1);
	textout_centre_ex(drawbuf, font, "Red Team", (3 * x1 + x2) / 4, y1 + 3 * line_height, col[COLWHITE], -1);
	textout_centre_ex(drawbuf, font, "Blue Team", (x1 + 3 * x2) / 4, y1 + 3 * line_height, col[COLWHITE], -1);

	int line = 5;
	textout_centre_ex(drawbuf, font, "Captures",       mx, y1 + line++ * line_height, col[COLWHITE], -1);
	textout_centre_ex(drawbuf, font, "Kills",          mx, y1 + line++ * line_height, col[COLWHITE], -1);
	textout_centre_ex(drawbuf, font, "Deaths",         mx, y1 + line++ * line_height, col[COLWHITE], -1);
	textout_centre_ex(drawbuf, font, "Suicides",       mx, y1 + line++ * line_height, col[COLWHITE], -1);
	textout_centre_ex(drawbuf, font, "Flags Taken",    mx, y1 + line++ * line_height, col[COLWHITE], -1);
	textout_centre_ex(drawbuf, font, "Flags Dropped",  mx, y1 + line++ * line_height, col[COLWHITE], -1);
	textout_centre_ex(drawbuf, font, "Flags Returned", mx, y1 + line++ * line_height, col[COLWHITE], -1);
	textout_centre_ex(drawbuf, font, "Shots",          mx, y1 + line++ * line_height, col[COLWHITE], -1);
	textout_centre_ex(drawbuf, font, "Hit accuracy",   mx, y1 + line++ * line_height, col[COLWHITE], -1);
	textout_centre_ex(drawbuf, font, "Shots taken",    mx, y1 + line++ * line_height, col[COLWHITE], -1);
	textout_centre_ex(drawbuf, font, "Movement",       mx, y1 + line++ * line_height, col[COLWHITE], -1);
	textout_centre_ex(drawbuf, font, "Power",          mx, y1 + line++ * line_height, col[COLWHITE], -1);

	for (int t = 0; t < 2; t++) {
		const Team& team = teams[t];
		line = 5;
		const int x = (t == 0 ? 3 * x1 + x2 : x1 + 3 * x2) / 4;
		textprintf_centre_ex(drawbuf, font, x, y1 + line++ * line_height, teamlcol[t], -1, "%d", team.score());
		textprintf_centre_ex(drawbuf, font, x, y1 + line++ * line_height, teamlcol[t], -1, "%d", team.kills());
		textprintf_centre_ex(drawbuf, font, x, y1 + line++ * line_height, teamlcol[t], -1, "%d", team.deaths());
		textprintf_centre_ex(drawbuf, font, x, y1 + line++ * line_height, teamlcol[t], -1, "%d", team.suicides());
		textprintf_centre_ex(drawbuf, font, x, y1 + line++ * line_height, teamlcol[t], -1, "%d", team.flags_taken());
		textprintf_centre_ex(drawbuf, font, x, y1 + line++ * line_height, teamlcol[t], -1, "%d", team.flags_dropped());
		textprintf_centre_ex(drawbuf, font, x, y1 + line++ * line_height, teamlcol[t], -1, "%d", team.flags_returned());
		textprintf_centre_ex(drawbuf, font, x, y1 + line++ * line_height, teamlcol[t], -1, "%d", team.shots());
		textprintf_centre_ex(drawbuf, font, x, y1 + line++ * line_height, teamlcol[t], -1, "%.0f%%", 100. * team.accuracy());
		textprintf_centre_ex(drawbuf, font, x, y1 + line++ * line_height, teamlcol[t], -1, "%d", team.shots_taken());
		textprintf_centre_ex(drawbuf, font, x, y1 + line++ * line_height, teamlcol[t], -1, "%.0f u", team.movement() / (2 * PLAYER_RADIUS));
		textprintf_centre_ex(drawbuf, font, x, y1 + line++ * line_height, teamlcol[t], -1, "%.2f", team.power());
	}

	line++;
	const int team_captures_start_y = y1 + line * line_height;

	const int total_captures = static_cast<int>(teams[0].captures().size() + teams[1].captures().size());
	if (team_captures_start >= total_captures - team_captures_size)
		team_captures_start = total_captures - team_captures_size;
	if (team_captures_start < 0)
		team_captures_start = 0;

	int red_score = teams[0].base_score(), blue_score = teams[1].base_score();
	int pos = 0;

	for (vector<pair<int, string> >::const_iterator red = teams[0].captures().begin(), blue = teams[1].captures().begin(); ; pos++) {
		const bool skip = (pos < team_captures_start || pos >= team_captures_start + team_captures_size);
		ostringstream message;
		int color = 0;
		if (red != teams[0].captures().end() && (blue == teams[1].captures().end() || red->first <= blue->first)) {
			++red_score;
			if (!skip) {
				color = teamlcol[0];
				message << setw(3) << red->first / 60 << ':' << setw(2) << setfill('0') << red->first % 60 << setfill(' ');
				message << setw(3) << red_score << " - " << left << setw(3) << blue_score << right;
				message << red->second;
			}
			++red;
		}
		else if (blue != teams[1].captures().end() && (red == teams[0].captures().end() || blue->first <= red->first)) {
			++blue_score;
			if (!skip) {
				color = teamlcol[1];
				message << setw(3) << blue->first / 60 << ':' << setw(2) << setfill('0') << blue->first % 60 << setfill(' ');
				message << setw(3) << red_score << " - " << left << setw(3) << blue_score << right;
				message << blue->second;
			}
			++blue;
		}
		else
			break;
		if (!skip)
			textout_ex(drawbuf, font, message.str().c_str(), x1 + 30, y1 + line++ * line_height, color, -1);
	}
	// draw scrollbar if there are more captures than visible on the screen
	if (team_captures_size < total_captures) {
		const int x = x2 - 30;
		const int y = team_captures_start_y;
		const int height = team_captures_size * line_height;
		const int bar_y = static_cast<int>(static_cast<float>(height * team_captures_start) / total_captures + 0.5);
		const int bar_h = static_cast<int>(static_cast<float>(height * team_captures_size) / total_captures + 0.5);
		scrollbar(x, y, height, bar_y, bar_h, col[COLGREEN], col[COLDARKGREEN]);
	}
}

void Graphics::draw_statistics(const vector<ClientPlayer*>& players, int page, int time, int maxplayers, int max_world_rank) {
	// line usage: 1 blank, red team (2 captions, 1 blank, 1 for every player), 1 blank, blue team, 1 page num
	const int num_lines = maxplayers + 3 + 2 * 3;
	const int line_h = min(12, SCREEN_H / num_lines);	// Preferred line height is 12.
	const int h = num_lines * line_h;
	const int w = 540;
	const int x_margin = 22;
	const int mx = SCREEN_W / 2;
	const int my = SCREEN_H / 2;
	const int x1 = mx - w / 2;
	const int y1 = my - h / 2;
	const int x2 = mx + w / 2;
	const int y2 = my + h / 2;
	const int x_left = x1 + x_margin;
	const int team1y = y1 + line_h, team2y = team1y + (3 + maxplayers / 2 + 1) * line_h, pageNumY = team2y + (3 + maxplayers / 2) * line_h;

	if (stats_alpha >= 0) {
		if (stats_alpha < 255) {
			drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
			set_trans_blender(0, 0, 0, stats_alpha);
		}
		rectfill(drawbuf, x1, y1, x2, y2, 0);
		// caption backgrounds
		rectfill(drawbuf, x1, team1y - 4, x2, team1y + 2 * line_h - 1, teamdcol[0]);
		rectfill(drawbuf, x1, team2y - 4, x2, team2y + 2 * line_h - 1, teamdcol[1]);
		solid_mode();
	}

	// space for (w - 2 * margin) / char_w == 62 characters per line; 4 + 1 + 16 + 1 used for login and name with spacing; 40 characters for stats
	string caption1, caption2;
	switch (page) {
		// max length:     |........................................|
		case 0: caption1 = "     Frags    total/in-row/most";
				caption2 = "Ping     Capt Kills      Deaths Suicides";
				//         |0000  000 00 000/00/00  000/00/00   00  |
				break;
		case 1: caption1 = "         Flags          Carriers  Carry";
				caption2 = "taken dropped returned   killed    time";
				//         |  00      00      00       00     00:00 |
				break;
		case 2: caption1 = "   Accuracy";
				caption2 = "Shots   | Taken  Movement     Speed";
				//         |00000 100% 0000  000000 u  00.00 u/s    |
				break;
		case 3: caption1 = "          Average        Tournament";
				caption2 = "Playtime lifetime    rank  power  score";
				//         |00000 min   00:00    0000  00.00 -00000  |
				break;
		default: nAssert(0);
	}
	const string red =  string("Red Team              ") + caption1;
	const string blue = string("Blue Team             ") + caption1;
	const string row2 = string("                      ") + caption2;
	textout_ex(drawbuf, font,  red.c_str(), x_left, team1y         , col[COLWHITE], -1);
	textout_ex(drawbuf, font, row2.c_str(), x_left, team1y + line_h, col[COLWHITE], -1);
	textout_ex(drawbuf, font, blue.c_str(), x_left, team2y         , col[COLWHITE], -1);
	textout_ex(drawbuf, font, row2.c_str(), x_left, team2y + line_h, col[COLWHITE], -1);

	int i = 0;
	int teamLineY[2] = { team1y + 3 * line_h, team2y + 3 * line_h };
	for (vector<ClientPlayer*>::const_iterator pi = players.begin(); pi != players.end(); ++pi, ++i) {
		const ClientPlayer& player = **pi;
		draw_player_statistics(player, x_left, teamLineY[player.team()], page, time);
		teamLineY[player.team()] += line_h;
	}

	if (page == 3 && max_world_rank > 0)
		textprintf_ex(drawbuf, font, x_left, pageNumY, col[COLGREEN], -1, "%d players in the tournament", max_world_rank);

	ostringstream page_num;
	page_num << page + 1 << '/' << 4;
	textout_right_ex(drawbuf, font, page_num.str().c_str(), x2 - x_margin, pageNumY, col[COLGREEN], -1);
}

void Graphics::draw_player_statistics(const ClientPlayer& player, int x, int y, int page, int time) {
	ostringstream stats;
	stats << player.reg_status.strFlags() << ' ';
	stats << left << setw(17) << player.name << right;
	const Statistics& st = player.stats();
	switch (page) {
		case 0:
			//       Frags    total/in-row/most
			//  Ping     Capt Kills      Deaths Suicides
			// |0000  000 00 000/00/00  000/00/00   00  |
			stats << setw(4) << player.ping
			      << setw(5) << st.frags()
			      << setw(3) << st.captures()
			      << setw(4) << st.kills()  << '/' << setw(2) << st.current_cons_kills()  << '/' << setw(2) << st.cons_kills()
			      << setw(5) << st.deaths() << '/' << setw(2) << st.current_cons_deaths() << '/' << setw(2) << st.cons_deaths()
			      << setw(5) << st.suicides();
			break;
		case 1:
			//           Flags          Carriers  Carry
			//  taken dropped returned   killed    time
			// |  00      00      00       00     00:00 |
			stats << setw(4) << st.flags_taken()
			      << setw(8) << st.flags_dropped()
			      << setw(8) << st.flags_returned()
			      << setw(9) << st.carriers_killed()
			      << setw(7) << static_cast<int>(st.flag_carrying_time(time)) / 60 << ':'
			      << setw(2) << setfill('0') << static_cast<int>(st.flag_carrying_time(time)) % 60 << setfill(' ');
			break;
		case 2:
			//     Accuracy
			//  Shots   | Taken  Movement     Speed
			// |00000 100% 0000  000000 u  00.00 u/s    |
			stats << setw(5) << st.shots()
			      << setw(4) << static_cast<int>(100. * st.accuracy() + 0.5) << '%'
			      << setw(5) << st.shots_taken()
			      << setw(8) << static_cast<int>(st.movement()) / (2 * PLAYER_RADIUS) << " u"
				  << setw(7) << setprecision(2) << std::fixed << st.old_speed() << " u/s";
			break;
		case 3:
			//            Average        Tournament
			//  Playtime lifetime    rank  power  score
			// |00000 min   00:00    0000  00.00 -00000  |
			stats << setw(5) << static_cast<int>(st.playtime(time)) / 60 << " min"
			      << setw(5) << static_cast<int>(st.average_lifetime(time)) / 60 << ':'
			      << setw(2) << setfill('0') << static_cast<int>(st.average_lifetime(time)) % 60 << setfill(' ');
			if (player.reg_status.masterAuth()) {
				stats << setw(8) << player.rank
				      << setw(7) << setprecision(2) << std::fixed << (player.score + 1.0) / (player.neg_score + 1.0)
				      << setw(7) << player.score - player.neg_score;
			}
			break;
	}
	textout_ex(drawbuf, font, stats.str().c_str(), x, y, teamlcol[player.team()], -1);
}

void Graphics::debug_panel(const vector<ClientPlayer>& players, int me, int bpsin, int bpsout,
						   const vector<vector<pair<int, int> > >& sticks, const vector<int>& buttons) {
	clear_to_color(drawbuf, 0);

	int line = 0;
	const int line_h = 10;
	for (vector<ClientPlayer>::const_iterator player = players.begin(); player != players.end(); ++player) {
		const int c = me == line ? col[COLYELLOW] : col[COLWHITE];
		textprintf_ex(drawbuf, font, 0, line++ * line_h, c, -1, "p. %2i u=%i ons=%i evs=%lu sxy=(%i, %i) HR: p=(%.1f, %.1f) s=(%.1f, %.1f)",
			line, player->used, player->onscreen, player->enemyvis, player->roomx, player->roomy,
			player->lx, player->ly, player->sx, player->sy);
	}
	
	line++;
	ostringstream axis_info;
	axis_info << "Joystick axes:";
	for (vector<vector<pair<int, int> > >::const_iterator stick = sticks.begin(); stick != sticks.end(); ++stick) {
		axis_info << " [";
		for (vector<pair<int, int> >::const_iterator axis = stick->begin(); axis != stick->end(); ++axis)
			axis_info << '(' << axis->first << ' ' << axis->second << ')';
		axis_info << ']';
	}
	textout_ex(drawbuf, font, axis_info.str().c_str(), 0, line++ * line_h, col[COLINFO], -1);

	line++;
	ostringstream button_info;
	button_info << "Joystick buttons:";
	for (vector<int>::const_iterator but = buttons.begin(); but != buttons.end(); ++but)
		button_info << ' ' << *but;
	textout_ex(drawbuf, font, button_info.str().c_str(), 0, line++ * line_h, col[COLINFO], -1);

	line++;
	const int bpstraffic = bpsin + bpsout;
	textprintf_ex(drawbuf, font, 0, line++ * line_h, col[COLINFO], -1, "Traffic: %4i B/s", bpstraffic);
	textprintf_ex(drawbuf, font, 0, line++ * line_h, col[COLINFO], -1, "in %4i B/s, out %4i B/s", bpsin, bpsout);
}

void Graphics::map_time(int seconds) {
	textprintf_right_ex(drawbuf, font, plx + scale(plw) - 2, SCREEN_H - 30, col[COLGREEN], -1, "%4d:%02d", seconds / 60, seconds % 60);
}

void Graphics::draw_fps(double fps) {
	textprintf_right_ex(drawbuf, font, SCREEN_W - 2, SCREEN_H - 10, col[COLMENUGRAY], -1, "FPS:%3.0f", fps);
}

void Graphics::map_list(const vector<MapInfo>& maps, int current, int own_vote, const string& edit_vote) {
	const int line_height = 12;
	const int w = 540;
	const int h = map_list_size * line_height + 96;
	const int mx = SCREEN_W / 2;
	const int my = SCREEN_H / 2;
	const int x1 = mx - w / 2;
	const int y1 = my - h / 2;
	const int x2 = mx + w / 2;
	const int y2 = my + h / 2;
	const int x_left = x1 + 30;

	if (stats_alpha >= 0) {
		if (stats_alpha < 255) {
			drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
			set_trans_blender(0, 0, 0, stats_alpha);
		}
		rectfill(drawbuf, x1, y1, x2, y2, 0);
		// caption background
		rectfill(drawbuf, x1, y1 + line_height - 4, x2, y1 + 2 * line_height, col[COLDARKGREEN]);
		solid_mode();
	}

	textout_centre_ex(drawbuf, font, "Server map list", mx, y1 + line_height, col[COLWHITE], -1);

	ostringstream caption;
	caption << left << "Nr Vote " << setw(20) << "Title" << ' ' << setw(5) << "Size" << " Author";
	textout_ex(drawbuf, font, caption.str().c_str(), x_left, y1 + 3 * line_height, col[COLWHITE], -1);

	if (map_list_start >= static_cast<int>(maps.size()) - map_list_size)
		map_list_start = maps.size() - map_list_size;
	if (map_list_start < 0)
		map_list_start = 0;

	for (int i = map_list_start; i < static_cast<int>(maps.size()) && i < map_list_start + map_list_size; ++i) {
		ostringstream mapline;
		mapline << setw(2) << i + 1 << ' ' << setw(2);
		const MapInfo& map = maps[i];
		if (map.votes > 0)
			mapline << map.votes;
		else
			mapline << '-';
		if (own_vote == i)
			mapline << " *";
		else
			mapline << "  ";
		mapline << ' ' << setw(20) << left << map.title.substr(0, 20) << right << ' ';
		mapline << setw(2) << map.width << '×' << setw(2) << left << map.height << right << ' ';
		mapline << map.author.substr(0, 27);
		const int y = y1 + 5 * line_height + line_height * (i - map_list_start);
		textout_ex(drawbuf, font, mapline.str().c_str(), x_left, y, (i == current) ? col[COLYELLOW] : col[COLWHITE], -1);
	}
	// draw scrollbar if there are more maps than visible on the screen
	if (map_list_size < static_cast<int>(maps.size())) {
		const int x = x2 - 30;
		const int y = y1 + 5 * line_height - 4;
		const int height = map_list_size * line_height;
		const int bar_y = static_cast<int>(static_cast<float>(height * map_list_start) / maps.size() + 0.5);
		const int bar_h = static_cast<int>(static_cast<float>(height * map_list_size) / maps.size() + 0.5);
		scrollbar(x, y, height, bar_y, bar_h, col[COLGREEN], col[COLDARKGREEN]);
	}
	ostringstream vote;
	vote << "Vote map number: " << edit_vote << '_';
	const int y = y1 + (5 + map_list_size + 1) * line_height;
	textout_ex(drawbuf, font, vote.str().c_str(), x_left, y, col[COLGREEN], -1);
}

void Graphics::draw_player_power(double val) {
	textprintf_ex(drawbuf, font, indicators_x + 244, indicators_y, col[COLCYAN], -1, "POWER: %3.0f", val);
}

void Graphics::draw_player_turbo(double val) {
	textprintf_ex(drawbuf, font, indicators_x + 244, indicators_y + 10, col[COLYELLOW], -1, "TURBO: %3.0f", val);
}

void Graphics::draw_player_shadow(double val) {
	textprintf_ex(drawbuf, font, indicators_x + 244, indicators_y + 20, col[COLMAG], -1, "SHADOW:%3.0f", val);
}

void Graphics::draw_player_weapon(int level) {
	textprintf_ex(drawbuf, font, indicators_x + 340, indicators_y, col[COLWHITE], -1, "WEAPON: %i", level);
}

void Graphics::draw_change_team_message(double time) {
	int c;
	if (static_cast<int>(time * 2.0) % 2)	// blink!
		c = col[COLRED];
	else
		c = col[COLWHITE];
	textout_ex(drawbuf, font, "CHANGE", plx + scale(plw - 6 * 8 - 10),     ply + scale(plh - 18), c, -1);
	textout_ex(drawbuf, font, "TEAMS",  plx + scale(plw - 6 * 8 - 10 + 4), ply + scale(plh -  9), c, -1);
}

void Graphics::draw_change_map_message(double time) {
	int c;
	if (static_cast<int>(time * 2.0) % 2)	// blink!
		c = col[COLRED];
	else
		c = col[COLWHITE];
	textout_ex(drawbuf, font, "EXIT", plx + scale(plw - 40 - 6 * 8 - 10),     ply + scale(plh - 18), c, -1);
	textout_ex(drawbuf, font, "MAP",  plx + scale(plw - 40 - 6 * 8 - 10 + 4), ply + scale(plh -  9), c, -1);
}

void Graphics::draw_player_health(int health) {
	const int x0 = indicators_x + 10;
	const int y0 = indicators_y;
	// health value
	textprintf_ex(drawbuf, font, x0, y0, col[COLWHITE], -1, "Health: %5i", health);
	// health bar
	rectfill(drawbuf, x0, y0 + 12, x0 + 100, y0 + 12 + 10, col[COLNOLIFE]);
	if (health == 0)
		return;
	// health 0...100
	int targ = min(health, 100);
	rectfill(drawbuf, x0, y0 + 12, x0 + targ, y0 + 12 + 10, col[COLRED]);
	// health 100...200
	targ = min(health - 100, 100);
	if (targ > 0)
		rectfill(drawbuf, x0, y0 + 12, x0 + targ, y0 + 12 + 10, col[COLYELLOW]);
	//health 200...300
	targ = min(health - 200, 100);
	if (targ > 0)
		rectfill(drawbuf, x0, y0 + 12, x0 + targ, y0 + 12 + 10, col[COLMAG]);
}

void Graphics::draw_player_energy(int energy) {
	const int x0 = indicators_x + 10 + 14 * 8;
	const int y0 = indicators_y;
	// energy value
	textprintf_ex(drawbuf, font, x0, y0, col[COLWHITE], -1, "Energy: %5i", energy);
	// energy bar
	rectfill(drawbuf, x0, y0 + 12, x0 + 100, y0 + 12 + 10, col[COLNOLIFE]);
	if (energy == 0)
		return;
	//barra azul 0..100
	int targ = min(energy, 100);
	rectfill(drawbuf, x0, y0 + 12, x0 + targ, y0 + 12 + 10, col[COLBLUE]);
	//barra verde 100..200
	targ = min(energy - 100, 100);
	if (targ > 0)
		rectfill(drawbuf, x0, y0 + 12, x0 + targ, y0 + 12 + 10, col[COLGREEN]);
	//barra 3o nivel
	targ = min(energy - 200, 100);
	if (targ > 0)
		rectfill(drawbuf, x0, y0 + 12, x0 + targ, y0 + 12 + 10, col[COLENER3]);
}

void Graphics::print_chat_messages(list<Message>::const_iterator msg, const list<Message>::const_iterator& end, const string& talkbuffer) {
	if (!show_chat_messages)
		return;
	const int line_height = 11;
	const int marginal = 3;
	int line = 0;
	for (; msg != end; ++msg, ++line) {
		if (msg->text().empty())
			continue;

		// print the message
		print_chat_message(msg->type(), msg->text(), marginal, marginal + line * line_height);
	}
	if (!talkbuffer.empty()) {
		ostringstream message;
		message << "Say: " << talkbuffer << '_';
		print_chat_input(message.str(), marginal, marginal + line * line_height);
	}
}

void Graphics::print_chat_message(Message_type type, const string& message, int x, int y) {
	int c;
	switch (type) {
		case msg_warning: c = col[COLLRED]; break;
		case msg_team: c = col[COLYELLOW]; break;
		case msg_info: c = col[COLGREEN]; break;
		case msg_header: c = col[COLYELLOW]; break;
		case msg_normal: default: c = col[COLORA];
	}
	print_text_border(message, x, y, c, 0, -1);
}

void Graphics::print_chat_input(const string& message, int x, int y) {
	print_text_border(message, x, y, col[COLWHITE], 0, -1);
}

void Graphics::print_text_border(const string& text, int x, int y, int textcol, int bordercol, int bgcol) {
	print_text_border(text, x, y, textcol, bordercol, bgcol, false);
}

void Graphics::print_text_border_centre(const string& text, int x, int y, int textcol, int bordercol, int bgcol) {
	print_text_border(text, x, y, textcol, bordercol, bgcol, true);
}

void Graphics::print_text_border(const string& text, int x, int y, int textcol, int bordercol, int bgcol, bool centring) {
	void (*print)(BITMAP*, const FONT*, const char*, int, int, int, int);
	if (centring)
		print = textout_centre_ex;
	else
		print = textout_ex;
	if (bgcol != -1)
		print(drawbuf, font, text.c_str(), x, y, textcol, bgcol);
	// nice border
	for (int i = -1; i <= 1; i++)
		for (int j = -1; j <= 1; j++) {
			if (i == 0 && j == 0)
				continue;
			print(drawbuf, font, text.c_str(), x + i, y + j, bordercol, -1);
		}
	// text itself
	print(drawbuf, font, text.c_str(), x, y, textcol, -1);
}

void Graphics::scrollbar(int x, int y, int height, int bar_y, int bar_h, int col1, int col2) {
	const int width = 10;
	if (height > 0) {
		rectfill(drawbuf, x, y, x + width - 1, y + height - 1, col2);
		if (bar_h > 0)
			rectfill(drawbuf, x, y + bar_y, x + width - 1, y + bar_y + bar_h - 1, col1);
	}
}

void Graphics::show_not_responding_message() {
	rect(drawbuf, 194, 199, 444, 279, col[COLMENUWHITE]);
	rect(drawbuf, 196, 201, 446, 281, col[COLMENUBLACK]);
	rectfill(drawbuf, 195, 200, 445, 280, col[COLMENUGRAY]);
	textprintf_ex(drawbuf, font, 220, 220, col[COLWHITE], -1, "SERVER NOT RESPONDING...");
	textprintf_ex(drawbuf, font, 220, 240, col[COLWHITE], -1, "May be heavy packet loss,");
	textprintf_ex(drawbuf, font, 220, 255, col[COLWHITE], -1, "or the server disconnected");
}

bool Graphics::save_screenshot(const string& filename) const {
	PALETTE pal;
	get_palette(pal);
	return !save_bitmap(filename.c_str(), drawbuf, pal);
}

// client side effects

//clear clientside fx's
void Graphics::clear_fx() {
	cfx.clear();
}

//create wall explosion fx
void Graphics::create_wallexplo(int x, int y, int px, int py) {
	clientfx_t fx;

	fx.type = FX_WALL_EXPLOSION;
	fx.x = x;
	fx.y = y;
	fx.time = get_time();
	fx.px = px;
	fx.py = py;

	cfx.push_back(fx);
}

//create quad wall explosion fx
void Graphics::create_quadwallexplo(int x, int y, int px, int py) {
	clientfx_t fx;

	fx.type = FX_POWER_WALL_EXPLOSION;
	fx.x = x;
	fx.y = y;
	fx.time = get_time();
	fx.px = px;
	fx.py = py;

	cfx.push_back(fx);
}

//create deathbringer explosion fx
void Graphics::create_deathbringer(int team, double start_time, int x, int y, int px, int py) {
	clientfx_t fx;

	fx.team = team;
	fx.type = FX_DEATHBRINGER_EXPLOSION;
	fx.x = x;
	fx.y = y;
	fx.time = start_time;
	fx.px = px;
	fx.py = py;

	cfx.push_back(fx);
}

// Create deathbringer powerup smoke, but only if there is no deathbringer sprite.
void Graphics::create_smoke(int x, int y, int px, int py) {
	if (!pup_sprite[Powerup::pup_deathbringer])
		create_deathcarrier(x, y, px, py);
}

//create deathbringer carrier trail fx
void Graphics::create_deathcarrier(int x, int y, int px, int py) {
	clientfx_t fx;

	fx.type = FX_DEATHCARRIER_SMOKE;
	fx.x = x;
	fx.y = y;
	fx.px = px;
	fx.py = py;
	fx.time = get_time();
	fx.col1 = 0;	// black

	cfx.push_back(fx);
}

//create explosion fx
void Graphics::create_gunexplo(int x, int y, int px, int py) {
	clientfx_t fx;

	fx.type = FX_GUN_EXPLOSION;
	fx.x = x;
	fx.y = y;
	fx.time = get_time();
	fx.px = px;
	fx.py = py;

	cfx.push_back(fx);
}

//create speed bolinha fx
void Graphics::create_speedfx(int x, int y, int px, int py, int col1, int col2, int gundir) {
	clientfx_t fx;

	fx.type = FX_SPEED;
	fx.x = x;
	fx.y = y;
	fx.px = px;
	fx.py = py;
	fx.time = get_time();

	fx.col1 = col1;
	fx.col2 = col2;
	fx.gundir = gundir;

	cfx.push_back(fx);
}

void Graphics::draw_effects(int room_x, int room_y, double time) {
	for (list<clientfx_t>::iterator fx = cfx.begin(); fx != cfx.end(); ) {
		if (fx->px != room_x || fx->py != room_y) {	// different room
			++fx;
			continue;
		}
		double delta = time - fx->time;
		switch (fx->type) {
			case FX_GUN_EXPLOSION:
				if (delta > 0.4)
					fx = cfx.erase(fx);
				else {
					for (int e = 0; e < 3; e++) {
						int rad = 4 + e + (int)(delta * 40);
						draw_gun_explosion(fx->x, fx->y, rad);
					}
					++fx;
				}
				break;

			case FX_SPEED:		// speed effect, draw another time in another function
				++fx;
				break;

			case FX_WALL_EXPLOSION:
				if (delta > 0.2)
					fx = cfx.erase(fx);
				else {
					for (int e = 0; e < 2; e++) {
						int rad = 4 + e + (int)(delta * 40);
						draw_gun_explosion(fx->x, fx->y, rad);
					}
					++fx;
				}
				break;

			case FX_POWER_WALL_EXPLOSION:
				if (delta > 0.2)
					fx = cfx.erase(fx);
				else {
					for (int e = 0; e < 3; e++) {
						int rad = 4 + e + (int)(delta * 60);
						draw_gun_explosion(fx->x, fx->y, rad);
					}
					++fx;
				}
				break;

			case FX_DEATHBRINGER_EXPLOSION:
				if (delta > 3.0)
					fx = cfx.erase(fx);
				else {
					draw_deathbringer(fx->x, fx->y, fx->team, delta);
					++fx;
				}
				break;

			case FX_DEATHCARRIER_SMOKE:
				if (delta > 0.6)
					fx = cfx.erase(fx);
				else {
					draw_deathbringer_smoke(fx->x, fx->y, delta);
					++fx;
				}
				break;

			default:
				nAssert(0);
		}
	}
}

// draw speed effect
void Graphics::draw_speedfx(int room_x, int room_y, double time) {
	for (list<clientfx_t>::iterator fx = cfx.begin(); fx != cfx.end(); ) {
		if (fx->px != room_x || fx->py != room_y || fx->type != FX_SPEED) {	// different room or wrong type
			++fx;
			continue;
		}
		double delta = time - fx->time;
		if (delta > 0.3)
			fx = cfx.erase(fx);
		else {
			int alpha = 90 - (int)(delta * 300.0);
			draw_player(fx->x, fx->y, fx->col1, fx->col2, fx->gundir, time, false, alpha, time);
			++fx;
		}
	}
}

bool Graphics::save_map_picture(const string& filename, const Map& map) {
	const int old_minimap_p_w = minimap_place_w;
	const int old_minimap_p_h = minimap_place_h;
	minimap_place_w = map.w * 60 + 2;
	minimap_place_h = map.h * 45 + 2;
	Bitmap buffer = create_bitmap(minimap_place_w, minimap_place_h);
	nAssert(buffer);
	update_minimap_background(buffer, map, true);
	PALETTE pal;
	get_palette(pal);
	minimap_place_w = old_minimap_p_w;
	minimap_place_h = old_minimap_p_h;
	bool failure = !save_bitmap(filename.c_str(), buffer, pal);
	return failure;
}

// Theme functions

void Graphics::search_themes(LineReceiver& dst) const {
	dst("<no theme>");

	const string searchPattern = wheregamedir + "graphics" + directory_separator + "*.*";

	log("Graphics theme searching: '%s'", searchPattern.c_str());

	const int attrib = FA_DIREC | FA_ARCH | FA_RDONLY;

 	vector<string> themes;
	struct al_ffblk ffblk;
	for (int error = al_findfirst(searchPattern.c_str(), &ffblk, attrib); !error; error = al_findnext(&ffblk))
		if ((ffblk.attrib & FA_DIREC) && strcmp(ffblk.name, ".") && strcmp(ffblk.name, ".."))
			themes.push_back(ffblk.name);

	sort(themes.begin(), themes.end());
	for (vector<string>::const_iterator ti = themes.begin(); ti != themes.end(); ++ti)
		dst(*ti);
}

void Graphics::select_theme(const string& dir) {
	unload_pictures();
	if (dir == "<no theme>")
		no_theme = true;
	else {
		no_theme = false;
		load_theme(dir.c_str());
	}
}

void Graphics::load_theme(const string& dirname) {
	theme_path = wheregamedir + "graphics" + directory_separator + dirname + directory_separator;
	load_pictures(theme_path);

	ifstream file((theme_path + "theme.txt").c_str());
	if (!getline_skip_comments(file, theme_name))
		theme_name = "(unnamed theme)";

	log("Loaded graphics theme '%s'.", dirname.c_str());
}

void Graphics::load_pictures(const string& path) {
	if (floor_texture.empty())	// kludge: load_theme -> load_pictures might be called before init
		return;
	load_floor_textures(path);
	load_wall_textures(path);
	load_player_sprites(path);
	load_shield_sprites(path);
	load_dead_sprites(path);
	load_rocket_sprites(path);
	load_flag_sprites(path);
	load_pup_sprites(path);
}

void Graphics::load_floor_textures(const string& path) {
	int i = 0;
	floor_texture[i++] = load_bitmap((path + "floor_normal1.pcx").c_str(), NULL);
	floor_texture[i++] = load_bitmap((path + "floor_normal2.pcx").c_str(), NULL);
	floor_texture[i++] = load_bitmap((path + "floor_normal3.pcx").c_str(), NULL);
	floor_texture[i++] = load_bitmap((path + "floor_red.pcx").c_str(), NULL);
	floor_texture[i++] = load_bitmap((path + "floor_blue.pcx").c_str(), NULL);
	floor_texture[i++] = load_bitmap((path + "floor_ice.pcx").c_str(), NULL);
	floor_texture[i++] = load_bitmap((path + "floor_sand.pcx").c_str(), NULL);
	floor_texture[i++] = load_bitmap((path + "floor_mud.pcx").c_str(), NULL);
}

void Graphics::load_wall_textures(const string& path) {
	int i = 0;
	wall_texture[i++] = load_bitmap((path + "wall_normal1.pcx").c_str(), NULL);
	wall_texture[i++] = load_bitmap((path + "wall_normal2.pcx").c_str(), NULL);
	wall_texture[i++] = load_bitmap((path + "wall_normal3.pcx").c_str(), NULL);
	wall_texture[i++] = load_bitmap((path + "wall_red.pcx").c_str(), NULL);
	wall_texture[i++] = load_bitmap((path + "wall_blue.pcx").c_str(), NULL);
	wall_texture[i++] = load_bitmap((path + "wall_metal.pcx").c_str(), NULL);
	wall_texture[i++] = load_bitmap((path + "wall_wood.pcx").c_str(), NULL);
	wall_texture[i++] = load_bitmap((path + "wall_rubber.pcx").c_str(), NULL);
}

BITMAP* Graphics::get_floor_texture(int texid) {
	if (floor_texture[texid])
		return floor_texture[texid];
	else
		return floor_texture.front();
}

BITMAP* Graphics::get_wall_texture(int texid) {
	if (wall_texture[texid])
		return wall_texture[texid];
	else
		return wall_texture.front();
}

void Graphics::load_player_sprites(const string& path) {
	const int size = scale(2 * 2 * PLAYER_RADIUS);
	Bitmap common = scale_sprite(path + "player.pcx", size, size);
	Bitmap team = scale_alpha_sprite(path + "player_team.pcx", size, size);
	Bitmap personal = scale_alpha_sprite(path + "player_personal.pcx", size, size);
	if (common && team && personal) {
		// Make player sprites by combining player image with team and personal colours.
		for (int t = 0; t < 2; t++)
			for (int p = 0; p < MAX_PLAYERS / 2; p++) {
				player_sprite[t][p] = create_bitmap(size, size);
				nAssert(player_sprite[t][p]);
				combine_sprite(player_sprite[t][p], common, team, personal, teamcol[t], col[p]);
			}
		// Make a sprite for player with power.
		player_sprite_power = create_bitmap(size, size);
		nAssert(player_sprite_power);
		combine_sprite(player_sprite_power, common, team, personal, col[COLWHITE], col[COLCYAN]);
	}
}

void Graphics::overlayColor(BITMAP* bmp, BITMAP* alpha, int color) {	// alpha must be an 8-bit bitmap; give the color in same format as bmp
	if (!alpha)
		return;
	nAssert(bmp);
	nAssert(alpha->w == bmp->w && alpha->h == bmp->h);
	drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
	for (int y = 0; y < bmp->h; y++)
		for (int x = 0; x < bmp->h; x++) {
			const int a = _getpixel(alpha, x, y);
			if (a) {
				set_trans_blender(0, 0, 0, a);
				putpixel(bmp, x, y, color);
			}
		}
	solid_mode();
}

void Graphics::combine_sprite(BITMAP* sprite, BITMAP* common, BITMAP* team, BITMAP* personal, int tcol, int pcol) {	// give the colors in same format as sprite
	nAssert(sprite && common);
	blit(common, sprite, 0, 0, 0, 0, common->w, common->h);
	overlayColor(sprite, personal, pcol);
	overlayColor(sprite, team, tcol);
}

void Graphics::load_shield_sprites(const string& path) {
	const int size = scale(2 * 2 * PLAYER_RADIUS);
	Bitmap picture = scale_sprite(path + "player_shield.pcx", size, size);		  
	if (!picture)
		return;
	Bitmap team  = scale_alpha_sprite(path + "player_shield_team.pcx", size, size);
	for (int t = 0; t < 2; t++) {
		shield_sprite[t] = create_bitmap(size, size);
		nAssert(shield_sprite[t]);
		combine_sprite(shield_sprite[t], picture, team, 0, teamcol[t], 0);
	}
}

void Graphics::load_dead_sprites(const string& path) {
	const int size = scale(2 * 2 * PLAYER_RADIUS);
	ice_cream = scale_sprite(path + "ice_cream.pcx", size, size);
	Bitmap picture = scale_sprite(path + "dead.pcx", size, size);
	if (!picture)
		return;
	Bitmap alpha = scale_alpha_sprite(path + "dead_alpha.pcx", size, size);
	Bitmap team  = scale_alpha_sprite(path + "dead_team.pcx" , size, size);
	for (int t = 0; t < 2; t++) {
		dead_sprite[t] = create_bitmap_ex(32, size, size);
		nAssert(dead_sprite[t]);
		combine_sprite(dead_sprite[t], picture, team, 0, colorTo32(teamcol[t]), 0);
		set_alpha_channel(dead_sprite[t], alpha);
	}
}

// Make rocket sprites by combining rocket image with team colour.
void Graphics::load_rocket_sprites(const string& path) {
	const int size = scale(2 * 2 * ROCKET_RADIUS);
	Bitmap normal = scale_sprite(path + "rocket.pcx", size, size);
	if (normal) {
		Bitmap team = scale_alpha_sprite(path + "rocket_team.pcx", size, size);
		for (int t = 0; t < 2; t++) {
			rocket_sprite[t] = create_bitmap(size, size);
			nAssert(rocket_sprite[t]);
			combine_sprite(rocket_sprite[t], normal, team, 0, teamcol[t], 0);
		}
		normal.free();
	}
	Bitmap power = scale_sprite(path + "rocket_pow.pcx", size, size);
	if (power) {
		Bitmap team = scale_alpha_sprite(path + "rocket_pow_team.pcx", size, size);
		for (int t = 0; t < 2; t++) {
			power_rocket_sprite[t] = create_bitmap(size, size);
			nAssert(power_rocket_sprite[t]);
			combine_sprite(power_rocket_sprite[t], power, team, 0, teamcol[t], 0);
		}
	}
}

// Make flag sprites by combining flag image with team colour.
void Graphics::load_flag_sprites(const string& path) {
	const int size = scale(50);
	Bitmap flag = scale_sprite(path + "flag.pcx", size, size);
	if (flag) {
		Bitmap team = scale_alpha_sprite(path + "flag_team.pcx", size, size);
		for (int t = 0; t < 3; t++) {
			flag_sprite[t] = create_bitmap(size, size);
			nAssert(flag_sprite[t]);
			combine_sprite(flag_sprite[t], flag, team, 0, teamcol[t], 0);
		}
	}
}

void Graphics::load_pup_sprites(const string& path) {
	const int size = scale(2 * PLAYER_RADIUS);
	pup_sprite[Powerup::pup_shield      ] = scale_sprite(path + "shield.pcx"      , size, size);
	pup_sprite[Powerup::pup_turbo       ] = scale_sprite(path + "turbo.pcx"       , size, size);
	pup_sprite[Powerup::pup_shadow      ] = scale_sprite(path + "shadow.pcx"      , size, size);
	pup_sprite[Powerup::pup_power       ] = scale_sprite(path + "power.pcx"       , size, size);
	pup_sprite[Powerup::pup_weapon      ] = scale_sprite(path + "weapon.pcx"      , size, size);
	pup_sprite[Powerup::pup_health      ] = scale_sprite(path + "health.pcx"      , size, size);
	pup_sprite[Powerup::pup_deathbringer] = scale_sprite(path + "deathbringer.pcx", size, size);
}

BITMAP* Graphics::scale_sprite(const string& filename, int x, int y) const {
	Bitmap temp = load_bitmap(filename.c_str(), NULL);
	if (!temp)
		return 0;
	BITMAP* target = create_bitmap(x, y);
	nAssert(target);
	stretch_blit(temp, target, 0, 0, temp->w, temp->h, 0, 0, target->w, target->h);
	return target;
}

BITMAP* Graphics::scale_alpha_sprite(const string& filename, int x, int y) const {
	set_color_conversion(COLORCONV_NONE);
	BITMAP* temp = load_bitmap(filename.c_str(), NULL);
	set_color_conversion(COLORCONV_TOTAL);
	if (!temp)
		return 0;
	if (bitmap_color_depth(temp) != 8) {
		log.error("Alpha bitmaps must be 8-bit grayscale images; %s is %d-bit", filename.c_str(), bitmap_color_depth(temp));
		return 0;
	}
	BITMAP* target = create_bitmap_ex(8, x, y);
	nAssert(target);
	stretch_blit(temp, target, 0, 0, temp->w, temp->h, 0, 0, target->w, target->h);
	return target;
}

void Graphics::unload_pictures() {
	unload_floor_textures();
	unload_wall_textures();
	unload_player_sprites();
	unload_shield_sprites();
	unload_dead_sprites();
	unload_rocket_sprites();
	unload_flag_sprites();
	unload_pup_sprites();
}

void Graphics::unload_floor_textures() {
	for (vector<Bitmap>::iterator pl = floor_texture.begin(); pl != floor_texture.end(); ++pl)
		pl->free();
}

void Graphics::unload_wall_textures() {
	for (vector<Bitmap>::iterator pl = wall_texture.begin(); pl != wall_texture.end(); ++pl)
		pl->free();
}

void Graphics::unload_player_sprites() {
	for (int t = 0; t < 2; t++)
		for (vector<Bitmap>::iterator pl = player_sprite[t].begin(); pl != player_sprite[t].end(); ++pl)
			pl->free();
	player_sprite_power.free();
}

void Graphics::unload_shield_sprites() {
	for (int i = 0; i < 2; i++)
		shield_sprite[i].free();
}

void Graphics::unload_dead_sprites() {
	for (int i = 0; i < 2; i++)
		dead_sprite[i].free();
	ice_cream.free();
}

void Graphics::unload_rocket_sprites() {
	for (int i = 0; i < 2; i++) {
		rocket_sprite[i].free();
		power_rocket_sprite[i].free();
	}
}

void Graphics::unload_flag_sprites() {
	for (int i = 0; i < 3; i++)
		flag_sprite[i].free();
}

void Graphics::unload_pup_sprites() {
	for (vector<Bitmap>::iterator pup = pup_sprite.begin(); pup != pup_sprite.end(); ++pup)
		pup->free();
}

inline int Graphics::scale(double value) const {
	return static_cast<int>(scr_mul * value + 0.5);
}

