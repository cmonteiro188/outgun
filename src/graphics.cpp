#include "commont.h"
#include "world.h"
#include "effects.h"
#include "sounds.h"
#include "graphics.h"

//macros for allegro video mode

//#define WINMODE GFX_DIRECTX_ACCEL
//#define FULLMODE GFX_DIRECTX_ACCEL

#ifdef NIX	// use GDI graphics mode instead of DirectX - works better on my computer, slows things down on all computers
#define WINMODE GFX_GDI       // can't pageflip
void textprintf_ex(struct BITMAP* bmp, AL_CONST FONT *f, int x, int y, int color, int bg, AL_CONST char* format, ...) {
	text_mode(bg);
	va_list argptr;
	char xbuf[16384];
	va_start(argptr, format);
	vsprintf(xbuf, format, argptr);
	va_end(argptr);
	textout(bmp, f, xbuf, x, y, color);
}
void textprintf_centre_ex(struct BITMAP* bmp, AL_CONST FONT *f, int x, int y, int color, int bg, AL_CONST char* format, ...) {
	text_mode(bg);
	va_list argptr;
	char xbuf[16384];
	va_start(argptr, format);
	vsprintf(xbuf, format, argptr);
	va_end(argptr);
	textout_centre(bmp, f, xbuf, x, y, color);
}
#else
#define WINMODE GFX_AUTODETECT_WINDOWED
#endif

#define FULLMODE GFX_AUTODETECT

//#define SWITCH_PAUSE_CLIENT

using std::string;
using std::vector;
using std::list;

Graphics::Graphics(int scr_w, int scr_h):
	minimap_start_x(0),
	minimap_start_y(0),
	flagpos_ready(false),
	vidpage1(0),
	vidpage2(0),
	backbuf(0)
{
	reset_video_mode();
	flagpos_buf[0] = 0;
	flagpos_buf[1] = 0;
	drawbuf = create_bitmap(scr_w, scr_h);
	minimap_w = minimap_place_w = 160;
	minimap_h = minimap_place_h = 100;
	minibg = create_bitmap(minimap_place_w, minimap_place_h);
	roombg = create_bitmap(plw, plh);
	transparent = bitmap_mask_color(roombg);
	setcolors();
}

Graphics::~Graphics() {
	destroy_bitmap(drawbuf);
	destroy_bitmap(minibg);
	destroy_bitmap(roombg);
	destroy_bitmap(flagpos_buf[0]);
	destroy_bitmap(flagpos_buf[1]);
}

void Graphics::draw_screen() const {
	acquire_screen();
	blit(drawbuf, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
	release_screen();
}

void Graphics::setcolors() {
	//the first 8 colors are player's colors
	col[COLGREEN] = makecol(0x00, 0xFF, 0x00);
	col[COLYELLOW] = makecol(0xFF, 0xFF, 0x00);
	col[COLWHITE] = makecol(0xFF, 0xFF, 0xFF);
	col[COLMAG]	= makecol(0xFF, 0x00, 0xFF);
	col[COLCYAN] = makecol(0, 0xFF, 0xFF);
	col[COLORA]	= makecol(0xFF, 0xB0, 0x00);
	col[COLLRED] = makecol(0xFF, 0x55, 0x44);
	col[COLLBLUE] = makecol(0x44, 0x55, 0xFF);
	//MORE player colors
	col[COL9] = makecol(242, 158, 224);
	col[COL10] = makecol(134, 143, 57);
	col[COL11] = makecol( 14, 148, 87);
	col[COL12] = makecol( 33, 132, 137);
	col[COL13] = makecol(100, 100, 100);
	col[COL14] = makecol(166, 166, 166);
	col[COL15] = makecol(202, 1, 56);	//wine
	col[COL16] = makecol(0xBF, 0x70, 0);	//darkora

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
	col[COLGROUND_DEF] = col[COLGROUND] = makecol(0x10, 0x40, 0);
	col[COLWALL_DEF] = col[COLWALL] = makecol(0x30, 0xC0, 0);
	col[COLNOLIFE] = makecol(0, 0, 0);
	col[COLDARKGRAY] = makecol(0x30, 0x30, 0x30);
	col[COLSHADOW] = makecol(0x18, 0x18, 0x18);
	col[COLLIMBO] = makecol(0x10, 0x10, 0x10);
	col[COLDARKORA]	= makecol(0xBF, 0x70, 0x00);
	col[COLINFO] = col[COLDARKORA];		//color of statusbar non-game info (hostname, IP, net traffic)
	col[COLENER3] = makecol(125, 100, 255);

	//teams 0 & 1 (playernum(0..15) / 8) colors:
	teamcol[0] = col[COLRED];
	teamcol[1] = col[COLBLUE];

	//light colours for text
	teamlcol[0] = col[COLLRED];
	teamlcol[1] = col[COLLBLUE];

	//light colours for team text bg
	teamdcol[0] = col[COLBRED];
	teamdcol[1] = col[COLBBLUE];
}

void Graphics::reset_playground_colors() {
	col[COLGROUND] = col[COLGROUND_DEF];
	col[COLWALL] = col[COLWALL_DEF];
	flagpos_ready = false;
}

void Graphics::random_playground_colors() {
	col[COLGROUND] = makecol(rand() % 256, rand() % 256, rand() % 256);
	col[COLWALL] = makecol(rand() % 256, rand() % 256, rand() % 256);
	flagpos_ready = false;
}

void Graphics::clear() {
	clear_to_color(drawbuf, 0);
}

bool Graphics::reset_video_mode() {
	string err[4];

	//un-show any video bitmaps?
	//show_video_bitmap(screen);

	//destroy all old surfaces
	if (vidpage1) { LOG("destroying vidpage1\n"); destroy_bitmap(vidpage1); vidpage1 = 0; }
	if (vidpage2) { LOG("destroying vidpage2\n"); destroy_bitmap(vidpage2); vidpage2 = 0; }
	if (backbuf) { LOG("destroying backbuf\n"); destroy_bitmap(backbuf); backbuf = 0; }

	int notok;

	set_color_depth(16); // hicolor
	if (winclient) // set mode
		notok = set_gfx_mode(WINMODE, RESOL_X, RESOL_Y, 0, 0);
	else
		notok = set_gfx_mode(FULLMODE, RESOL_X, RESOL_Y, 0, 0);

// ***** INICIO *******

	if (notok < 0) {
		LOG1("ERROR: cannot set 640x480x16 windowed?=%i graphics mode!\n", winclient);
		LOG1("Allegro error: '%s'\n", allegro_error);
		err[0] = allegro_error;

		//try again...
		winclient = !winclient;
		set_color_depth(16); // hicolor
		if (winclient) // set mode
			notok = set_gfx_mode(WINMODE, RESOL_X, RESOL_Y, 0, 0);
		else
			notok = set_gfx_mode(FULLMODE, RESOL_X, RESOL_Y, 0, 0);

		if (notok < 0) {
			LOG1("ERROR: cannot set 640x480x16 windowed?=%i graphics mode!\n", winclient);
			LOG1("Allegro error: '%s'\n", allegro_error);
			err[1] = allegro_error;

			//try again...
			winclient = !winclient;
			set_color_depth(15); // ===> different color depth
			if (winclient) // set mode
				notok = set_gfx_mode(WINMODE, RESOL_X, RESOL_Y, 0, 0);
			else
				notok = set_gfx_mode(FULLMODE, RESOL_X, RESOL_Y, 0, 0);

			if (notok < 0) {
				LOG1("ERROR: cannot set 640x480x15 windowed?=%i graphics mode!\n", winclient);
				LOG1("Allegro error: '%s'\n", allegro_error);
				err[2] = allegro_error;

				//AND try again.....
				winclient = !winclient;
				set_color_depth(15); // ===> different color depth
				if (winclient) // set mode
					notok = set_gfx_mode(WINMODE, RESOL_X, RESOL_Y, 0, 0);
				else
					notok = set_gfx_mode(FULLMODE, RESOL_X, RESOL_Y, 0, 0);

				if (notok < 0) {
					LOG1("ERROR: cannot set 640x480x15 windowed?=%i graphics mode!\n", winclient);
					LOG1("Allegro error: '%s'\n", allegro_error);
					err[3] = allegro_error;

					char elmsg[4096];
					sprintf(elmsg, "ERROR: cannot initialize graphics! reasons:\n1 = '%s'\n2 = '%s'\n3 = '%s'\n4 = '%s'",
     					err[0].c_str(), err[1].c_str(), err[2].c_str(), err[3].c_str());
					allegro_message(elmsg);
					return false;	// FATAL error
				}
			}
		}
	}

// ***** FIM *******

#ifndef SWITCH_PAUSE_CLIENT
	if (set_display_switch_mode(SWITCH_BACKAMNESIA) == -1) {
		if (set_display_switch_mode(SWITCH_BACKGROUND) == -1) // allow running in the background
		{
			LOG("ERROR: client cannot run in the background!\n");
			return false; // FATAL
		}
		else LOG("switch_background set ok.");
	}
	else LOG("switch_BACKAMNESIA set ok.");
#endif

	//attempt to create two video bitmaps, else use RAM backbuffer
	if (trypageflip) {
		vidpage1 = create_video_bitmap(SCREEN_W, SCREEN_H);
		vidpage2 = create_video_bitmap(SCREEN_W, SCREEN_H);
		LOG2("create vidpage1=%p vidpage2=%p\n", vidpage1, vidpage2);
	}

	//delete any partial video pages
	if ((!vidpage1) || (!vidpage2)) {
		if (trypageflip)
			LOG("PAGE FLIPPING: not enought video memory. using bruteforce doublebuffering\n")
		else
			LOG("USING SAFE MODE VIDEO -- DOUBLE-BUFFERING (option -dbuf). for page flipping use -flip\n")

		if (vidpage1) { destroy_bitmap(vidpage1); vidpage1 = 0; LOG("destroyed vidpage1\n"); }
		if (vidpage2) { destroy_bitmap(vidpage2); vidpage2 = 0; LOG("destroyed vidpage2\n");}

			//create RAM backbuffer
		backbuf = create_bitmap(SCREEN_W, SCREEN_H);
		if (!backbuf) {
			LOG("ERROR: cannot create memory backbuffer!\n");
			return false; // FATAL
		}
		drawbuf = backbuf;
		page_flipping = false;
	}
	else {
		drawbuf = vidpage1;
		page_flipping = true;
	}
	setcolors();

	return true; //ok
}

//draw a flag  team 0/1   x, y: coord relative to playarea
void Graphics::draw_flag(int team, int x, int y) {
	//draw shadow
	ellipsefill(drawbuf,
		plx + x,
		ply + y,
		12, 3, col[COLSHADOW]
	);
	//draw flagpole
	rectfill(drawbuf,
		plx + x - 3,
		ply + y - 40,
		plx + x + 3,
		ply + y,
		col[COLYELLOW]
	);
	//draw the flag itself
	rectfill(drawbuf,
		plx + x,
		ply + y - 38,
		plx + x + 20,
		ply + y - 20,
		teamcol[team]
	);
}

//draw minimap flag
void Graphics::draw_mini_flag(int team, const ctflag_t& flag, const Map& map) {
	const double px = ((double)flag.pos.px * (double)plw + flag.pos.x) / ((double)plw * map.w);
	const double py = ((double)flag.pos.py * (double)plh + flag.pos.y) / ((double)plh * map.h);
	const int pix = int(mmx + minimap_start_x + 1 + px * (minimap_w - 2));
	const int piy = int(mmy + minimap_start_y + 1 + py * (minimap_h - 2));
	//draw flagpole
	rectfill(drawbuf, pix, piy - 5, pix, piy, col[COLYELLOW]);
	//draw the flag itself
	rectfill(drawbuf, pix + 1, piy - 5, pix + 5, piy - 2, teamcol[team]);
}

void Graphics::draw_minimap_player(int x, int y, int team, int player) {
	x += minimap_start_x;
	y += minimap_start_y;
	putpixel(drawbuf, x + 0, y + 0, teamcol[team]);	//3 pixel teamcol
	putpixel(drawbuf, x + 1, y + 0, teamcol[team]);	//3 pixel teamcol
	putpixel(drawbuf, x + 0, y + 1, teamcol[team]);	//3 pixel teamcol
	putpixel(drawbuf, x + 1, y + 1, col[player]);	// 1 pixel personal-color
}

void Graphics::draw_minimap_me(int x, int y, int team, double time) {
	x += minimap_start_x;
	y += minimap_start_y;
	if ((int)(time * 15) % 3 > 0) {
		circlefill(drawbuf, x, y, 2, col[COLYELLOW]);
		circlefill(drawbuf, x, y, 1, teamlcol[team]);
	}
	else
		circlefill(drawbuf, x, y, 2, 0);
}

void Graphics::draw_minimap_room(int x1, int y1, int x2, int y2) {
	drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
	set_trans_blender(0, 0, 0, 0x38);
	rectfill(drawbuf, mmx + minimap_start_x + x1, mmy + minimap_start_y + y1,
			 mmx + minimap_start_x + x2, mmy + minimap_start_y + y2, col[COLFOGOFWAR]);
	solid_mode();
}

void Graphics::draw_minimap_background() {
	blit(minibg, drawbuf, 0, 0, mmx, mmy, minibg->w, minibg->h);
}

void Graphics::update_minimap_background(const Map& map) {
	update_minimap_background(minibg, map, false);
}

void Graphics::update_minimap_background(BITMAP* buffer, const Map& map, bool flagPaintSimple, bool save_map_pic) {
	//black background
	clear_to_color(buffer, col[COLSHADOW]);

	//calculate new minimap size (133×100 for maps with n×n rooms)
	if (map.w > map.h) {
		minimap_w = minimap_place_w;
		minimap_h = static_cast<int>(static_cast<float>(minimap_w * map.h * 3) / map.w / 4 + 0.5);
	}
	else {
		minimap_h = minimap_place_h;
		minimap_w = static_cast<int>(static_cast<float>(minimap_h * map.w * 4) / map.h / 3 + 0.5);
	}

	//draw room boundaries
	minimap_start_x = (minimap_place_w - minimap_w) / 2;
	minimap_start_y = (minimap_place_h - minimap_h) / 2;
	rectfill(buffer, minimap_start_x, minimap_start_y, minimap_start_x + minimap_w - 1, minimap_start_y + minimap_h - 1, 0);
	float room_w = float(minimap_w - 2) / map.w;
	float room_h = float(minimap_h - 2) / map.h;
	for (int i = 1; i < map.w; i++)
		line(buffer, int(minimap_start_x + 1 + room_w * i), minimap_start_y, int(minimap_start_x + room_w * i), minimap_start_y + minimap_h, col[COLSHADOW]);
	for (int i = 1; i < map.h; i++)
		line(buffer, minimap_start_x, int(minimap_start_y + 1 + room_h * i), minimap_start_x + minimap_w, int(minimap_start_y + room_h * i), col[COLSHADOW]);

	double maxx = plw * map.w;
	double maxy = plh * map.h;

	//draw bases
	if (flagPaintSimple) {
		int  red_rx = map.tinfo[0].flag.px,  red_ry = map.tinfo[0].flag.py;
		int blue_rx = map.tinfo[1].flag.px, blue_ry = map.tinfo[1].flag.py;
		if (red_rx == blue_rx && red_ry == blue_ry) {
			// for lack of mathematical enthusiasm, the half-way line is not calculated analytically; instead, determine each pixel individually (slow)
			// this is OK since this function is called once per map instead of every frame
			int xmin = int(minimap_start_x + 2 + room_w * red_rx), xmax = int(minimap_start_x + room_w * (red_rx + 1));
			int ymin = int(minimap_start_y + 2 + room_h * red_ry), ymax = int(minimap_start_y + room_h * (red_ry + 1));
			for (int y = ymin; y <= ymax; ++y) {
				float roomy = float(y + 1 - ymin) / float(room_h) * plh;
				float ydist_r2 = pow(map.tinfo[0].flag.y - roomy, 2);
				float ydist_b2 = pow(map.tinfo[1].flag.y - roomy, 2);
				for (int x = xmin; x <= xmax; ++x) {
					float roomx = float(x + 1 - xmin) / float(room_w) * plw;
					float xdist_r2 = pow(map.tinfo[0].flag.x - roomx, 2);
					float xdist_b2 = pow(map.tinfo[1].flag.x - roomx, 2);
					int color = (xdist_r2 + ydist_r2 < xdist_b2 + ydist_b2) ? col[COLBRED] : col[COLBBLUE];
					putpixel(buffer, x, y, color);
				}
			}
		}
		else {
			rectfill(buffer, int(minimap_start_x + 2 + room_w * red_rx),  int(minimap_start_y + 2 + room_h * red_ry),
					 int(minimap_start_x + room_w * (red_rx + 1)),  int(minimap_start_y + room_h * (red_ry + 1)),  col[COLBRED]);
			rectfill(buffer, int(minimap_start_x + 2 + room_w * blue_rx), int(minimap_start_y + 2 + room_h * blue_ry),
					 int(minimap_start_x + room_w * (blue_rx + 1)), int(minimap_start_y + room_h * (blue_ry + 1)), col[COLBBLUE]);
		}
	}

	//draw solid walls
	float xmul = float(minimap_w - 2) / maxx, ymul = float(minimap_h - 2) / maxy;
	for (int y = 0; y < map.h; y++) {
		float by = minimap_start_y + 1 + y * plh * ymul;
		for (int x = 0; x < map.w; x++) {
			float bx = minimap_start_x + 1 + x * plw * xmul;
			set_clip(buffer, (int)bx, (int)by, int(bx + room_w), int(by + room_h));
			map.room[x][y].draw(buffer, bx, by, xmul, ymul, makecol(0x00, 0x77, 0x00));
			set_clip(buffer, 0, 0, buffer->w, buffer->h);
		}
	}

	//green border
	rect(buffer, minimap_start_x, minimap_start_y, minimap_start_x + minimap_w - 1, minimap_start_y + minimap_h - 1, col[COLGREEN]);

	int  red_px = int(minimap_start_x + 1 + (map.tinfo[0].flag.px * plw + map.tinfo[0].flag.x) / maxx * (minimap_w - 2));
	int  red_py = int(minimap_start_y + 1 + (map.tinfo[0].flag.py * plh + map.tinfo[0].flag.y) / maxy * (minimap_h - 2));
	int blue_px = int(minimap_start_x + 1 + (map.tinfo[1].flag.px * plw + map.tinfo[1].flag.x) / maxx * (minimap_w - 2));
	int blue_py = int(minimap_start_y + 1 + (map.tinfo[1].flag.py * plh + map.tinfo[1].flag.y) / maxy * (minimap_h - 2));
	if (!flagPaintSimple) {
		if (getpixel(buffer, red_px, red_py) != 0) {	// is painted with any color
			update_minimap_background(buffer, map, true);	// restart with basic painting
			return;
		}
		floodfill(buffer, red_px, red_py, col[COLBRED]);

		if (getpixel(buffer, blue_px, blue_py) != 0) {	// is painted with any color (including by previous red floodfill)
			update_minimap_background(buffer, map, true);	// restart with basic painting
			return;
		}
		floodfill(buffer, blue_px, blue_py, col[COLBBLUE]);
	}
	if (save_map_pic) {
		//const int scale =
		//draw flagpoles
		rectfill(buffer,  red_px,  red_py - 5,  red_px,  red_py, col[COLYELLOW]);
		rectfill(buffer, blue_px, blue_py - 5, blue_px, blue_py, col[COLYELLOW]);
		//draw the flags
		rectfill(buffer,  red_px + 1,  red_py - 5,  red_px + 5,  red_py - 2, teamcol[0]);
		rectfill(buffer, blue_px + 1, blue_py - 5, blue_px + 5, blue_py - 2, teamcol[1]);
	}
	else {
		circle(buffer,  red_px,  red_py, 3, col[COLRED ]);
		circle(buffer, blue_px, blue_py, 3, col[COLBLUE]);
	}
}

//draws a basic player object
void Graphics::draw_player(int x, int y, int team, int pli, int gundir, double hitfx, bool item_quad, int alpha, double time) {
	int pc1 = teamcol[team];
	int pc2 = col[pli];
	//blink player when hit
	double deltafx = hitfx - time;
	if (deltafx > 0) {
		NLubyte rgb = (NLubyte)(70.0 + deltafx * 600.0);  // var 180
		pc1 = pc2 = makecol(rgb, rgb, rgb);
	}
	else if (item_quad) {
		//pisca branco
		if ((int)(time * 10) % 2) {
			pc1 = col[COLWHITE];
			pc2 = col[COLCYAN];
		}
	}
	//draw the gun (direction facing)
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
	xg = (int)((double)xg * 0.7);
	yg = (int)((double)yg * 0.7);

	xg += x;
	yg += y;

	if (alpha < 255) {
		set_trans_blender(0, 0, 0, alpha);
		drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
	}

	//desenha arma antes se dir 5,6,7
	if (gundir >= 5) {
		line(drawbuf, 0 + plx + x, 0 + ply + y, 0 + plx + xg, 0 + ply + yg, pc1);
		line(drawbuf, 1 + plx + x, 0 + ply + y, 1 + plx + xg, 0 + ply + yg, pc1);
		line(drawbuf, 1 + plx + x, 1 + ply + y, 1 + plx + xg, 1 + ply + yg, pc1);
	}

	// outer color: team color
	circlefill(drawbuf, plx + x, ply + y, PLAYER_RADIUS, pc1);
	// inner color: self color
	circlefill(drawbuf, plx + x, ply + y, PLAYER_RADIUS*2/3, pc2);

	//desenha arma depois se dir 0,1,2,3,4
	if (gundir < 5) {
		line(drawbuf, 0 + plx + x, 0 + ply + y, 0 + plx + xg, 0 + ply + yg, pc1);
		line(drawbuf, 1 + plx + x, 0 + ply + y, 1 + plx + xg, 0 + ply + yg, pc1);
		line(drawbuf, 1 + plx + x, 1 + ply + y, 1 + plx + xg, 1 + ply + yg, pc1);
	}

	if (alpha < 255)
		solid_mode();
}

void Graphics::draw_player_shadow(const ClientPlayer& player, int alpha) {
	drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
	set_trans_blender(0, 0, 0, alpha);
	ellipsefill(drawbuf, plx + (int)player.lx, ply + (int)player.ly + PLAYER_RADIUS, 15, 3, col[COLSHADOW]);
	solid_mode();
}

void Graphics::draw_virou_sorvete(int x, int y) {
	ellipsefill(drawbuf, plx + x, ply + y, 6, 15, col[COLORA]);
	circlefill(drawbuf, plx + x - 8, ply + y - 10, 8, col[COLBLUE]);
	circlefill(drawbuf, plx + x + 8, ply + y - 10, 8, col[COLMAG]);
	circlefill(drawbuf, plx + x + 0, ply + y - 20, 8, col[COLGREEN]);
	textprintf_centre_ex(drawbuf, font, plx + x + 0, ply + y - 48, col[COLWHITE], -1, "VIROU");
	textprintf_centre_ex(drawbuf, font, plx + x + 0, ply + y - 38, col[COLWHITE], -1, "SORVETE!");
}

void Graphics::draw_player_dead(int x, int y) {
	ellipsefill(drawbuf, plx + x, ply + y + PLAYER_RADIUS*4/5, 20, 6, col[COLRED]);
	circlefill(drawbuf, plx + x, ply + y, PLAYER_RADIUS*4/5, col[COLRED]);
}

void Graphics::draw_gun_explosion(int x, int y, int rad) {
	circle(drawbuf, plx + x, ply + y, rad, makecol(rand() % 256, rand() % 256, rand() % 256));
}

void Graphics::draw_deathbringer_smoke(int x, int y, double time) {
	int alpha = 120 - (int)(time * 200.0);
	alpha = max(alpha, 0);
	drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
	set_trans_blender(0, 0, 0, alpha);
	double drad = 3.0 + 9.0 * (0.6 - time);
	int rad = (int)drad;
	int subdist = (int)(96.0 - drad * 8.0);
	circlefill(drawbuf, plx + x, ply + y - subdist, rad, 0);
	solid_mode();
}

void Graphics::draw_deathbringer(int x, int y, int team, double time) {
	//radius
	int e, rad;
	if (time < 1.0)
		rad = (int)(time * 100);
	else
		rad = 100 + (int)((time - 1.0) * (time - 1.0) * 800);
	int maxxd = max(x, plw - x);
	int maxyd = max(y, plh - y);
	if (maxxd * maxxd + maxyd * maxyd >= rad * rad) {
		set_clip(drawbuf, plx, ply, plx + plw, ply + plh);
		//brightening ring
		for (e = 0; e < 30; e++, rad++) {
			int co;
			if (team == 0)
				co = makecol(14 + 8 * e, 0, 0);
			else
				co = makecol(0, 0, 14 + 8 * e);
			circle(drawbuf, plx + x, ply + y, rad, co);
		}
		//darkening ring
		for (e = 0; e < 10; e++, rad++) {
			int co;
			if (team == 0)
				co = makecol(255 - 14 * e, 0, 0);
			else
				co = makecol(0, 0, 255 - 14 * e);
			circle(drawbuf, plx + x, ply + y, rad, co);
			circle(drawbuf, plx + x + 1, ply + y, rad, co);
			circle(drawbuf, plx + x, ply + y + 1, rad, co);
		}
		set_clip(drawbuf, 0, 0, drawbuf->w - 1, drawbuf->h - 1);
	}
}

void Graphics::draw_deathbringer_affected(int x, int y, int team) {
	drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
	set_trans_blender(0, 0, 0, 128);
	for (int i = 0; i < 5; i++)
		circlefill(drawbuf, plx + x + rand() % 40 - 20, ply + y + rand() % 40 - 20, 15, teamcol[team]);
	for (int i = 0; i < 5; i++)
		circlefill(drawbuf, plx + x + rand() % 40 - 20, ply + y + rand() % 40 - 20, 15, 0);
	solid_mode();
}

void Graphics::draw_deathbringer_carrier_effect(int x, int y) {
	set_clip(drawbuf, plx, ply, plx + plw, ply + plh);
	//darken ground
	drawing_mode(DRAW_MODE_TRANS, 0,0,0);
	for (int r = 50; r > 0; r -= 5) {
		set_trans_blender(0, 0, 0, 50 - r);
		circlefill(drawbuf, plx + x, ply + y + PLAYER_RADIUS/3, r, 0);
	}
	solid_mode();
	set_clip(drawbuf, 0, 0, drawbuf->w - 1, drawbuf->h - 1);
}

void Graphics::draw_shield(int x, int y, int r, int alpha) {
	drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
	set_trans_blender(0, 0, 0, alpha);
	ellipse(drawbuf, plx + x, ply + y, r + rand() % 3, r + rand() % 3, makecol(rand() % 256, rand() % 256, rand() % 256));
	ellipse(drawbuf, plx + x, ply + y, r + rand() % 5, r + rand() % 5, makecol(rand() % 256, rand() % 256, rand() % 256));
	ellipse(drawbuf, plx + x, ply + y, r + rand() % 9, r + rand() % 9, makecol(rand() % 256, rand() % 256, rand() % 256));
	solid_mode();
}

void Graphics::draw_player_name(const string& name, int x, int y, int team) {
	print_text_border_centre(name, plx + x, ply + y - PLAYER_RADIUS - 10, col[COLWHITE], teamdcol[team]);
}

void Graphics::draw_rocket(const rocket_c& rocket, double time) {
	if (rocket.power) {	// powered rocket
		//draw rocket shadow
		ellipsefill(drawbuf, plx + (int)rocket.x, ply + (int)rocket.y + QUAD_ROCKET_RADIUS + 8, QUAD_ROCKET_RADIUS, 3, col[COLSHADOW]);
		//draw the rocket
		if ((int)(time * 30) % 2)
			circlefill(drawbuf, plx + (int)rocket.x, ply + (int)rocket.y, QUAD_ROCKET_RADIUS, col[COLWHITE]);	//y-12?
		else
			circlefill(drawbuf, plx + (int)rocket.x, ply + (int)rocket.y, QUAD_ROCKET_RADIUS, teamlcol[rocket.team]); //y-12??
	}
	else {				// normal rocket
		//draw rocket shadow
		ellipsefill(drawbuf, plx + (int)rocket.x, ply + (int)rocket.y + ROCKET_RADIUS + 8, ROCKET_RADIUS, 2, col[COLSHADOW]);
		//draw the rocket
		circlefill(drawbuf, plx + (int)rocket.x, ply + (int)rocket.y, ROCKET_RADIUS, teamcol[rocket.team]); //y-10??
	}
}

void Graphics::draw_playground() {
	rectfill(drawbuf, plx, ply, plx + plw, ply + plh, col[COLGROUND]);
}

void Graphics::draw_flagpos_mark(int team, int flag_x, int flag_y) {
	build_flagpos_marks();	// draw flag position mark sprites
	set_clip(drawbuf, plx, ply, plx + plw, ply + plh);
	blit(flagpos_buf[team], drawbuf, 0, 0,
		plx + flag_x - flagpos_radius, ply + flag_y - flagpos_radius, 2 * flagpos_radius, 2 * flagpos_radius);
	set_clip(drawbuf, 0, 0, drawbuf->w, drawbuf->h);
}

void Graphics::predraw_room(const Room& room) {
	clear_to_color(roombg, transparent);
	room.draw(roombg, 0, 0, 1., 1., col[COLWALL]);
}

void Graphics::draw_room() {
	masked_blit(roombg, drawbuf, 0, 0, plx, ply, roombg->w, roombg->h);
}

void Graphics::draw_pup(const pickup_c& pup, double time) {
	// pup's shadow
	ellipsefill(drawbuf, plx + pup.x, ply + pup.y + 12, 12, 3, col[COLSHADOW]);
	switch (pup.kind) {
		case 1: draw_pup_shield(pup.x, pup.y); break;		// shield
		case 2:	draw_pup_turbo(pup.x, pup.y); break; 		// turbo
		case 3: draw_pup_shadow(pup.x, pup.y, time); break;	// shadow
		case 4: draw_pup_power(pup.x, pup.y, time); break;	// power
		case 5: draw_pup_weapon(pup.x, pup.y, time); break;	// weapon upgrade
		case 6: draw_pup_health(pup.x, pup.y, time); break;	// megahealth
		case 7: draw_pup_deathbringer(pup.x, pup.y); break;	// deathbringer
	}
}

void Graphics::draw_pup_shield(int x, int y) {
	// makecol(rand(),rand(),rand()) calls changed
	draw_shield(x, y, 14);
	circlefill(drawbuf, plx + x, ply + y, 12, col[COLGREEN]);
}

void Graphics::draw_pup_turbo(int x, int y) {
	circlefill(drawbuf, plx + x + rand()%6-3, ply + y + rand()%6-3, 12, col[COLDARKORA]);
	circlefill(drawbuf, plx + x + rand()%8-4, ply + y + rand()%8-4, 12, col[COLORA]);
	circlefill(drawbuf, plx + x + rand()%12-6, ply + y + rand()%12-6, 12, col[COLYELLOW]);
}

void Graphics::draw_pup_shadow(int x, int y, double time) {
	drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
	int alpha = ((int)(time * 600.0)) % 400;
	if (alpha > 200)
		alpha = 400 - alpha;
	set_trans_blender(0, 0, 0, 55 + alpha);
	circlefill(drawbuf, plx + x, ply + y, 12, col[COLMAG]);
	solid_mode();
}

void Graphics::draw_pup_power(int x, int y, double time) {
	if ((int)(time * 30) % 2)
		circlefill(drawbuf, plx + x, ply + y, 13, col[COLWHITE]);
	else
		circlefill(drawbuf, plx + x, ply + y, 11, col[COLCYAN]);
}

void Graphics::draw_pup_weapon(int x, int y, double time) {
	// rotate item
	for (int b = 0; b < 4; b++) {
		// deg: 0..360
		double deg = (int)(time * 1000) % 1000;		//thousand ticks
		deg = deg / 1000.0;		//0..1 inteiro
		deg = deg * 6.2832;		// 0..1 vezes 2 pi (1 volta)
		deg = deg + 1.5708 * b; //mais b's (90 grauses)

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
		//draw a ball
		circlefill(drawbuf, plx + x + (int)dx, ply + y + (int)dy, 4, c);
	}
}

void Graphics::draw_pup_health(int x, int y, double time) {
	//caixa de saude pulsante
	int varia = (int)(time * 15) % 10;
	if (varia > 5)
		varia = 10 - varia;
	int itemsize = 11 + varia;
	int crossize = 8 + varia;
	int crosslar = 3; //aria/2;
	// health box black border
	rectfill(drawbuf, plx + x - itemsize - 2, ply + y - itemsize - 2, plx + x + itemsize + 2, ply + y + itemsize + 2, 0);
	// health box
	rectfill(drawbuf, plx + x - itemsize, ply + y - itemsize, plx + x + itemsize, ply + y + itemsize, col[COLWHITE]);
	// red cross
	rectfill(drawbuf, plx + x - crossize, ply + y - crosslar, plx + x + crossize, ply + y + crosslar, col[COLRED]);
	rectfill(drawbuf, plx + x - crosslar, ply + y - crossize, plx + x + crosslar, ply + y + crossize, col[COLRED]);
}

void Graphics::draw_pup_deathbringer(int x, int y) {
	//bola preta
	circlefill(drawbuf, plx + x, ply + y, 12, makecol(0x22, 0x33, 0x22));
}

void Graphics::draw_background() {
	clear_to_color(drawbuf, col[COLSHADOW]);
}

void Graphics::draw_empty_playground() {
	rectfill(drawbuf, plx, ply, plx + plw, ply + plh, 0);
}

void Graphics::draw_one_line_message(const string& message) {
	textprintf_centre_ex(drawbuf, font, plx + plw / 2, ply + plh / 2, col[COLGREEN], -1, "%s", message.c_str());
}

void Graphics::draw_waiting_map_message(const string& caption, const string& map) {
	textprintf_centre_ex(drawbuf, font, plx + plw / 2, ply + plh / 2 + 20, col[COLGREEN], -1, "%s", caption.c_str());
	textprintf_centre_ex(drawbuf, font, plx + plw / 2, ply + plh / 2 + 50, col[COLORA], -1, "%s", map.c_str());
}

void Graphics::draw_loading_map_message(const string& text) {
	textprintf_centre_ex(drawbuf, font, plx + plw / 2, ply + plh / 2 + 70, col[COLGREEN], -1, "%s", text.c_str());
}

void Graphics::draw_scores(const string& text, int team, int score1, int score2) {
	int c;
	switch (team) {
		case 0: c = col[COLLRED]; break;
		case 1: c = col[COLLBLUE]; break;
		default: c = col[COLMENUGRAY]; break;
	}
	textprintf_centre_ex(drawbuf, font, plx + plw / 2, ply + plh / 2 - 40, c, -1, "%s", text.c_str());
	textprintf_centre_ex(drawbuf, font, plx + plw / 2, ply + plh / 2 - 20, c, -1, "SCORE: %i - %i", score1, score2);
}

void Graphics::draw_scoreboard_caption(int team, const string& caption) {
	const int nameydelta_min = 8;
	textprintf_ex(drawbuf, font, sbx + 4, sby - 4 + team * 18 * nameydelta_min, teamlcol[team], -1, "%s", caption.c_str());
}

void Graphics::draw_scoreboard_name(int y, int pcol, const ClientPlayer& player) {
	textprintf_ex(drawbuf, font, sbx + 4, y, col[pcol], -1, "%c%s", player.reg_status, player.name.c_str());
}

void Graphics::draw_scoreboard_points(int y, int team, int points) {
	textprintf_ex(drawbuf, font, sbx + 4 + 16 * 8, y, teamlcol[team], -1, "%4i", points);
}

void Graphics::draw_statistics(const vector<ClientPlayer>& players) {
	drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
	set_trans_blender(0, 0, 0, 180);

	int w = 540;
	int h = 420;
	int mx = SCREEN_W / 2;
	int my = SCREEN_H / 2;
	int x1 = mx - w / 2;
	int y1 = my - h / 2;
	int x2 = mx + w / 2;
	int y2 = my + h / 2;
	//int xc = (x1 + x2) / 2;
	const int x_left = x1 + 40;

	rectfill(drawbuf, x1, y1, x2, y2, 0);
	solid_mode();

	int line_height = 10;

	// frags and ping work, other stats are just layout testing
	string text = "      Frags Ping Cap Kil Dea  Acc   Dist Time";
	print_text_border(string("Red Team  ") + text, x_left, y1 + line_height, teamlcol[0], teamdcol[0]);
	print_text_border(string("Blue Team ") + text, x_left, y1 + h / 2 + line_height, teamlcol[1], teamdcol[1]);

	int i = 0;
	for (vector<ClientPlayer>::const_iterator p = players.begin(); p != players.end(); p++, i++) {
		const int y = y1 + 3 * line_height + line_height * (i % TSIZE) + (i / TSIZE) * h / 2;
		if (p->used)
			draw_player_statistics(*p, i / TSIZE, x_left, y);
	}
}

void Graphics::draw_player_statistics(const ClientPlayer& player, int team, int x, int y) {
	ostringstream info;
	info << left << setw(15) << player.name.c_str() << ' ' << right;
	info << setw(5) << player.frags << ' ';
	info << setw(4) << player.ping << ' ';
	// layout testing
	info << "  2  24  18  65%   4210  78 min";
	textprintf_ex(drawbuf, font, x, y, teamlcol[team], -1, "%s", info.str().c_str());
}

void Graphics::map_time(int seconds) {
	textprintf_ex(drawbuf, font, plx + 10, ply + 6, 0, -1, "TIME:%4d:%02d", seconds / 60, seconds % 60);
}

void Graphics::draw_fps(double fps) {
	textprintf_ex(drawbuf, font, plx + 10, ply + plh - 14, 0, -1, "FPS:%3.0f", fps);
}

void Graphics::draw_player_power(double val) {
	textprintf_ex(drawbuf, font, plx + 244, ply + plh + 5, col[COLCYAN], -1, "POWER: %3.0f", val);
}

void Graphics::draw_player_turbo(double val) {
	textprintf_ex(drawbuf, font, plx + 244, ply + plh + 15, col[COLYELLOW], -1, "TURBO: %3.0f", val);
}

void Graphics::draw_player_shadow(double val) {
	textprintf_ex(drawbuf, font, plx + 244, ply + plh + 25, col[COLMAG], -1, "SHADOW:%3.0f", val);
}

void Graphics::draw_player_weapon(int level) {
	textprintf_ex(drawbuf, font, plx + 340, ply + plh + 5, col[COLWHITE], -1, "WEAPON: %i", level);
}

void Graphics::draw_change_team_message(double time) {
	int c = col[COLWHITE];
	if ((int)(time * 2.0) % 2)	// blink!
		c = col[COLRED];
	textprintf_ex(drawbuf, font, plx + plw - 6 * 8 - 10,     ply + plh - 18, c, -1, "CHANGE");
	textprintf_ex(drawbuf, font, plx + plw - 6 * 8 - 10 + 4, ply + plh -  9, c, -1, "TEAMS");
}

void Graphics::draw_change_map_message(double time) {
	int c = col[COLWHITE];
	if ((int)(time * 2.0) % 2)	// blink!
		c = col[COLRED];
	textprintf_ex(drawbuf, font, -40 + plx + plw - 6 * 8 - 10,     ply + plh - 18, c, -1, "EXIT");
	textprintf_ex(drawbuf, font, -40 + plx + plw - 6 * 8 - 10 + 4, ply + plh -  9, c, -1, "MAP");
}

void Graphics::draw_player_health(int health) {
	// health value
	textprintf_ex(drawbuf, font, 10, ply + plh + 5, col[COLWHITE], -1, "Health: %5i", health);
	// health bar
	rectfill(drawbuf, 10, ply + plh + 18, 10 + 100, ply + plh + 18 + 10, col[COLNOLIFE]);
	if (health == 0)
		return;
	// health 0...100
	int targ = min(health, 100);
	rectfill(drawbuf, 10, ply + plh + 18, 10 + targ, ply + plh + 18 + 10, col[COLRED]);
	// health 100...200
	targ = min(health - 100, 100);
	if (targ > 0)
		rectfill(drawbuf, 10, ply + plh + 18, 10 + targ, ply + plh + 18 + 10, col[COLYELLOW]);
	//health 200...300
	targ = min(health - 200, 100);
	if (targ > 0)
		rectfill(drawbuf, 10, ply + plh + 18, 10 + targ, ply + plh + 18 + 10, col[COLMAG]);
}

void Graphics::draw_player_energy(int energy) {
	// energy value
	textprintf_ex(drawbuf, font, 10 + 14 * 8, ply + plh + 5, col[COLWHITE], -1, "Energy: %5i", energy);
	// energy bar
	rectfill(drawbuf, 10 + 14 * 8, ply + plh + 18, 10 + 14 * 8 + 100, ply + plh + 18 + 10, col[COLNOLIFE]);
	if (energy == 0)
		return;
	//barra azul 0..100
	int targ = min(energy, 100);
	rectfill(drawbuf, 10 + 14 * 8, ply + plh + 18, 10 + 14 * 8 + targ, ply + plh + 18 + 10, col[COLBLUE]);
	//barra verde 100..200
	targ = min(energy - 100, 100);
	if (targ > 0)
		rectfill(drawbuf, 10 + 14 * 8, ply + plh + 18, 10 + 14 * 8 + targ, ply + plh + 18 + 10, col[COLGREEN]);
	//barra 3o nivel
	targ = min(energy - 200, 100);
	if (targ > 0)
		rectfill(drawbuf, 10 + 14 * 8, ply + plh + 18, 10 + 14 * 8 + targ, ply + plh + 18 + 10, col[COLENER3]);
}

void Graphics::print_chat_message(int line, const string& message, MESSAGE_TYPE type) {
	int c;
	switch (type) {
		case MSG_WARNING: c = col[COLLRED]; break;
		case MSG_TEAM: c = col[COLYELLOW]; break;
		case MSG_INFO: c = col[COLGREEN]; break;
		default: c = col[COLORA];
	}
	textprintf_ex(drawbuf, font, 3, 3 + line * 11, c, -1, "%s", message.c_str());
}

void Graphics::print_chat_input(int line, const string& message) {
	const int x = 3;
	const int y = 3;
	const int lh = 11;
	print_text_border(message, x, y + line * lh, col[COLWHITE], 0);
}

void Graphics::print_text_border(const string& text, int x, int y, int textcol, int bordercol) {
	print_text_border(text, x, y, textcol, bordercol, false);
}

void Graphics::print_text_border_centre(const string& text, int x, int y, int textcol, int bordercol) {
	print_text_border(text, x, y, textcol, bordercol, true);
}

void Graphics::print_text_border(const string& text, int x, int y, int textcol, int bordercol, bool centring) {
	void (*print)(BITMAP*, const FONT*, int, int, int, int, const char*, ...);
	if (centring)
		print = textprintf_centre_ex;
	else
		print = textprintf_ex;
	// nice border
	print(drawbuf, font, x + 1, y + 0, bordercol, -1, "%s", text.c_str());
	print(drawbuf, font, x + 1, y + 1, bordercol, -1, "%s", text.c_str());
	print(drawbuf, font, x + 0, y + 1, bordercol, -1, "%s", text.c_str());
	print(drawbuf, font, x - 1, y + 1, bordercol, -1, "%s", text.c_str());
	print(drawbuf, font, x - 1, y + 0, bordercol, -1, "%s", text.c_str());
	print(drawbuf, font, x - 1, y - 1, bordercol, -1, "%s", text.c_str());
	print(drawbuf, font, x + 0, y - 1, bordercol, -1, "%s", text.c_str());
	print(drawbuf, font, x + 1, y - 1, bordercol, -1, "%s", text.c_str());
	// text itself
	print(drawbuf, font, x, y, textcol, -1, "%s", text.c_str());
}

void Graphics::show_not_responding_message() {
	rect(drawbuf,  194,  199, 444, 279, col[COLMENUWHITE]);
	rect(drawbuf, 196, 201, 446, 281, col[COLMENUBLACK]);
	rectfill(drawbuf, 195, 200, 445, 280, col[COLMENUGRAY]);
	textprintf_ex(drawbuf, font, 220, 220, col[COLWHITE], -1, "SERVER NOT RESPONDING...");
	textprintf_ex(drawbuf, font, 220, 240, col[COLWHITE], -1, "May be heavy packet loss,");
	textprintf_ex(drawbuf, font, 220, 255, col[COLWHITE], -1, "or the server disconnected");
}

// draw help
void Graphics::game_help() {
	clear_to_color(drawbuf, col[COLMENUGRAY]);

	int x = -40;
	int y = -70;

	textprintf_ex(drawbuf, font, x+100, y+100, col[COLWHITE], -1, "Outgun : HELP      --> Press ESC or F1 to go back. <--");

	textprintf_ex(drawbuf, font, x+100, y+120, col[COLWHITE], -1, "For more information access these websites:");
	textprintf_ex(drawbuf, font, x+100, y+130, col[COLWHITE], -1, "  the original Outgun website");
	textprintf_ex(drawbuf, font, x+364, y+130, col[COLGREEN], -1, "http://www.amok.com.br/outgun/en/");
	textprintf_ex(drawbuf, font, x+100, y+140, col[COLWHITE], -1, "  Nix's Outgun development page");
	textprintf_ex(drawbuf, font, x+364, y+140, col[COLGREEN], -1, "http://koti.mbnet.fi/npr/outgun/");

	textprintf_ex(drawbuf, font, x+100, y+160, col[COLWHITE], -1, "           MOVING            ARROW KEYS = MOVE");
	textprintf_ex(drawbuf, font, x+100, y+170, col[COLWHITE], -1, "            YOUR             CONTROL    = SHOOT!");
	textprintf_ex(drawbuf, font, x+100, y+180, col[COLWHITE], -1, "          CHARACTER:         ALT        = STRAFE");
	textprintf_ex(drawbuf, font, x+100, y+190, col[COLWHITE], -1, "            >>>>>            SHIFT      = RUN");

	textprintf_ex(drawbuf, font, x+100, y+210, col[COLWHITE], -1, "TALKING TO ALL PLAYERS: Just type your message and hit ENTER");

	textprintf_ex(drawbuf, font, x+100, y+230, col[COLWHITE], -1, "TALKING JUST TO YOUR TEAM: Just place a dot ('.') at the very");
	textprintf_ex(drawbuf, font, x+100, y+240, col[COLWHITE], -1, " beginning of your message (first char)");

	textprintf_ex(drawbuf, font, x+100, y+260, col[COLWHITE], -1, "GAME CONCEPT: You are a member of a team, either RED or BLUE,");
	textprintf_ex(drawbuf, font, x+100, y+270, col[COLWHITE], -1, " assigned to you at random when you connect. Your goal is to");
	textprintf_ex(drawbuf, font, x+100, y+280, col[COLWHITE], -1, " help your team to win, by capturing 8 (default) times the enemy");
	textprintf_ex(drawbuf, font, x+100, y+290, col[COLWHITE], -1, " flag. To capture the flag, a member of your team must steal");
	textprintf_ex(drawbuf, font, x+100, y+300, col[COLWHITE], -1, " the enemy flag and bring it to your team's flag, provided your");
	textprintf_ex(drawbuf, font, x+100, y+310, col[COLWHITE], -1, " flag has not been stolen already! Capiche?");

	textprintf_ex(drawbuf, font, x+100, y+330, col[COLWHITE], -1, "HEALTH AND ENERGY: If your health reaches zero, you die. Energy");
	textprintf_ex(drawbuf, font, x+100, y+340, col[COLWHITE], -1, " is used for running, shooting and health protection when you");
	textprintf_ex(drawbuf, font, x+100, y+350, col[COLWHITE], -1, " have the SHIELD powerup (you'll know when you see it...).");
	textprintf_ex(drawbuf, font, x+100, y+360, col[COLWHITE], -1, " Health and energy regenerate with time.");

	textprintf_ex(drawbuf, font, x+100, y+380, col[COLWHITE], -1, "MINIMAP: On the upper-right corner of the screen is the minimap.");
	textprintf_ex(drawbuf, font, x+100, y+390, col[COLWHITE], -1, " It shows the contents of all rooms of the map that have at least");
	textprintf_ex(drawbuf, font, x+100, y+400, col[COLWHITE], -1, " one player of your team.");

	textprintf_ex(drawbuf, font, x+100, y+420, col[COLWHITE], -1, "CHANGING TEAMS: Hit the END key to set whether you want to");
	textprintf_ex(drawbuf, font, x+100, y+430, col[COLWHITE], -1, " change team or not. You will change team when appropriate.");

	textprintf_ex(drawbuf, font, x+100, y+450, col[COLWHITE], -1, "POWERUPS: If you see an animated item lying on the ground, grab");
	textprintf_ex(drawbuf, font, x+100, y+460, col[COLWHITE], -1, " it. It's a special power-up item.");

	textprintf_ex(drawbuf, font, x+100, y+480, col[COLWHITE], -1, "ETC.: Hit DEL to kill yourself. Hold TAB to see other players'");
	textprintf_ex(drawbuf, font, x+100, y+490, col[COLWHITE], -1, " ping times (in milliseconds) on the scoreboard.");
	textprintf_ex(drawbuf, font, x+100, y+500, col[COLWHITE], -1, " Hit HOME to change world colors and CTRL+HOME to restore them.");
	textprintf_ex(drawbuf, font, x+100, y+510, col[COLWHITE], -1, " Hit F10 to receive a random name. Hit F11 to take a screenshot.");
}

void Graphics::server_list(const vector<gamespy_t>& servers, int selection, bool showmaster) {
	const int xi = 50 - 8 * 2;

	textprintf_ex(drawbuf, font, xi, 105, col[COLWHITE], -1, "IP Address             Ping #P Version/Hostname");

	char blinkchar[2];

	int line = 0;
	for (vector<gamespy_t>::const_iterator server = servers.begin(); server != servers.end(); server++) {
		const int yi = 120 + line * 13;

		// selection
		if (selection == line) {
			rectfill(drawbuf, xi - 3, yi - 3, xi + 550 + 8 * 3, yi + 12, col[COLSHADOW]);
			//blink cursor
			if ((int)(get_time() * 4) % 2)
				blinkchar[0] = ' ';
			else
				blinkchar[0] = '<';
			blinkchar[1] = 0;
		}
		else
			blinkchar[0] = 0;

		//server edit prompt
		textprintf_ex(drawbuf, font, xi, yi, col[COLGREEN], -1, ":%s%s", server->address, blinkchar);
		//favs watermarks
		if (showmaster && server->favs)
			textprintf_ex(drawbuf, font, xi - 12, yi, col[COLGREEN], -1, "*");

		//draw gamespy entry
		bool refreshed, invalid, noresponse;
		refreshed  = server->refreshed;
		invalid    = server->invalid;
		noresponse = server->noresponse;

		if (!refreshed) { // not refreshed
			//server info
			textprintf_ex(drawbuf, font, xi + (18+5)*8, yi, col[COLWHITE], -1, "press SPACEBAR to refresh...");
		}
		else if (invalid) {	//refreshed, invalid
			//server info
			textprintf_ex(drawbuf, font, xi + (18+5)*8, yi, col[COLWHITE], -1, "---");
		}
		else if (noresponse) {	//refreshed, no response
			//server info
			textprintf_ex(drawbuf, font, xi + (18+5)*8, yi, col[COLWHITE], -1, "no response.");
		}
		else	//refreshed, valid, show server info
			textprintf_ex(drawbuf, font, xi + (18+5)*8, yi, col[COLGREEN], -1, "%s", server->info);
		line++;
	}
}

void Graphics::public_servers(const vector<gamespy_t>& servers, int selection) {
	//Big F Menu
	rect(drawbuf,  19,  19, 620, 460, col[COLMENUWHITE]);
	rect(drawbuf,  21,  21, 621, 461, col[COLMENUBLACK]);

	const int lotext = makecol(0x99, 0x99, 0x99);

	const int hi = makecol(0x68, 0x68, 0x88);
	const int lo = makecol(0x68,0x48,0x48);
	//hilight all
	rectfill(drawbuf, 20, 20, 620, 460, hi);
	//first bar hi vs lo
	rectfill(drawbuf, 20, 20, 320, 50, hi);
	rectfill(drawbuf, 320, 19, 621, 50, 0);
	rectfill(drawbuf, 320, 24, 616, 50, lo);
	vline(drawbuf, 320, 20, 50, col[COLMENUBLACK]);
	hline(drawbuf, 320, 50, 621, col[COLMENUWHITE]);
	hline(drawbuf, 320, 24, 616, lotext);
	vline(drawbuf, 616, 24, 49, col[COLMENUBLACK]);
	textprintf_centre_ex(drawbuf, font, 170, 35, col[COLWHITE], -1, "INTERNET SEARCH");
	textprintf_centre_ex(drawbuf, font, 470, 35, lotext, -1, "FAVORITES");

	if (((int)(get_time() * 1)) % 2)
		textprintf_centre_ex(drawbuf, font, 320, 65, col[COLGREEN], -1, "F2 = UPDATE LIST OF SERVERS");
	else
		textprintf_centre_ex(drawbuf, font, 320, 65, col[COLYELLOW], -1, "F2 = UPDATE LIST OF SERVERS");

	textprintf_centre_ex(drawbuf, font, 320, 80, col[COLWHITE], -1, "Press SPACE to refresh the servers");
	textprintf_centre_ex(drawbuf, font, 320, 440, col[COLWHITE], -1, "TAB:Favorites  ARROWS:Select  ENTER:Connect  ESC:Cancel  SPACE:Refresh");

	// show the servers
	server_list(servers, selection, true);
}

void Graphics::favourite_servers(const vector<gamespy_t>& servers, int selection) {
	//Big F Menu
	rect(drawbuf,  19,  19, 620, 460, col[COLMENUWHITE]);
	rect(drawbuf,  21,  21, 621, 461, col[COLMENUBLACK]);

	const int lotext = makecol(0x99, 0x99, 0x99);

	const int hi = makecol(0x88, 0x68, 0x68);
	const int lo = makecol(0x48,0x48,0x68);
	//hilight all
	rectfill(drawbuf, 20, 20, 620, 460, hi);
	//first bar lo vs hi
	rectfill(drawbuf, 320, 20, 620, 50, hi);
	rectfill(drawbuf, 19, 19, 320, 50, 0);
	rectfill(drawbuf, 24, 24, 320, 50, lo);
	vline(drawbuf, 320, 19, 50, col[COLMENUWHITE]);
	hline(drawbuf, 19, 50, 320, col[COLMENUWHITE]);
	hline(drawbuf, 24, 24, 320, lotext);
	vline(drawbuf, 24, 24, 49, col[COLMENUWHITE]);
	textprintf_centre_ex(drawbuf, font, 170, 35, lotext, -1, "INTERNET SEARCH");
	textprintf_centre_ex(drawbuf, font, 470, 35, col[COLWHITE], -1, "FAVORITES");

	textprintf_centre_ex(drawbuf, font, 320, 65, col[COLWHITE], -1, "Type the IP address of the server and hit ENTER");
	textprintf_centre_ex(drawbuf, font, 320, 80, col[COLWHITE], -1, "Press SPACE to refresh the servers");
	textprintf_centre_ex(drawbuf, font, 320, 440, col[COLWHITE], -1, "TAB:Internet  ARROWS:Select  ENTER:Connect  ESC:Cancel  SPACE:Refresh");

	// show the servers
	server_list(servers, selection, false);
}

//draw the menu caption
void Graphics::menu_caption() {
	//"3d" menu
	rect(drawbuf,  99,  69, 539, 409, col[COLMENUWHITE]);
	rect(drawbuf, 101, 71, 541, 411, col[COLMENUBLACK]);
	rectfill(drawbuf, 100, 70, 540, 410, col[COLMENUGRAY]);
	textprintf_ex(drawbuf, font, 150, 120, col[COLWHITE], -1, "Outgun         version %s", GAME_VERSION);
	textprintf_ex(drawbuf, font, 150, 135, col[COLGREEN], -1, "http://koti.mbnet.fi/npr/outgun/");
}

//draw the main menu
void Graphics::main_menu(bool connected, const string& address, const string& playername, const string& namestatus,
						 bool listen_server_running, int listen_port_running, const Sounds& sounds)
{
	menu_caption();
	int DELY = 10;
	textprintf_ex(drawbuf, font, 150, 185-DELY, col[COLWHITE], -1, "  [ 1 ]   Connect");
	textprintf_ex(drawbuf, font, 150, 200-DELY, col[COLWHITE], -1, "  [ 2 ]   Disconnect");
	if (connected)
		textprintf_ex(drawbuf, font, 150+22*8, 200-DELY, col[COLGREEN], -1, "(%s)", address.c_str());
	textprintf_ex(drawbuf, font, 150, 215-DELY, col[COLWHITE], -1, "  [ 3 ]   Change Player Name & Password");
	textprintf_ex(drawbuf, font, 150, 227-DELY, col[COLGREEN], -1, "          '%s' (%s)", playername.c_str(), namestatus.c_str());
	textprintf_ex(drawbuf, font, 150, 243-DELY, col[COLWHITE], -1, "  [ 4 ]   Start/stop local server");
	if (listen_server_running)
		textprintf_ex(drawbuf, font, 150, 255-DELY, col[COLGREEN], -1, "          SERVER RUNNING ON PORT %i", listen_port_running);
	textprintf_ex(drawbuf, font, 150, 271-DELY, col[COLWHITE], -1, "  [ 5 ]   Toggle fullscreen/windowed mode");

	if (sounds.valid()) {
		textprintf_ex(drawbuf, font, 150, 286-DELY, col[COLWHITE], -1, "  [ 6 ]   Change sound theme: (%s)", sounds.theme_dir().c_str());
		textprintf_centre_ex(drawbuf, font, 150+180, 300-DELY, col[COLGREEN], -1, "'%s'", sounds.theme_name().c_str());
	}
	else {
		textprintf_ex(drawbuf, font, 150, 286-DELY, col[COLWHITE], -1, "  [ 6 ]   Change sound theme:");
		textprintf_ex(drawbuf, font, 150, 300-DELY, col[COLGREEN], -1, "          no sfx themes found.");
	}
	textprintf_ex(drawbuf, font, 150, 340-DELY, col[COLWHITE], -1, "Hit CTRL+F12 to EXIT THE GAME");
	textprintf_ex(drawbuf, font, 150, 355-DELY, col[COLWHITE], -1, "Hit ESC to HIDE OR SHOW THIS MENU");
	textprintf_ex(drawbuf, font, 150, 370-DELY, col[COLORA], -1, "Hit F1 to SHOW THE HELP SCREEN");
}

// show two-line message
void Graphics::dialog(const string& t1, const string& t2) {
	menu_caption();
	textprintf_ex(drawbuf, font, 150, 230, col[COLWHITE], -1, "%s", t1.c_str());
	textprintf_ex(drawbuf, font, 150, 250, col[COLWHITE], -1, "%s", t2.c_str());
}

//draw the name and password menu
void Graphics::name_password_menu(const string& name, int password_len, bool name_selected, const string& namestatus) {
	menu_caption();

	char namecursor = ' ';
	char passcursor = ' ';
	if (name_selected)
		namecursor = '_';
	else
		passcursor = '_';

	textprintf_ex(drawbuf, font, 150, 170, col[COLWHITE], -1, "Type in your player name. If you have");
	textprintf_ex(drawbuf, font, 150, 185, col[COLWHITE], -1, "registered your name on the Outgun");
	textprintf_ex(drawbuf, font, 150, 200, col[COLWHITE], -1, "website, then type in your password!");

	textprintf_ex(drawbuf, font, 150, 220, col[COLWHITE], -1, "ENTER = OK   ESC = CANCEL  TAB = NEXT FIELD");
	textprintf_ex(drawbuf, font, 150, 260, col[COLGREEN], -1, "NAME:     %s%c", name.c_str(), namecursor);

	//password field: '********'
	const string password(password_len, '*');

	textprintf_ex(drawbuf, font, 150, 285, col[COLGREEN], -1, "PASSWORD: %s%c", password.c_str(), passcursor);

	textprintf_ex(drawbuf, font, 150, 350, col[COLWHITE], -1, "Registration status: %s", namestatus.c_str());
}

//show progress (for tight loops that don't work with the regular screen flip loop)
void Graphics::show_progress(const string& t1, const string& t2, const string& t3, int fg, int bg) {
	if (fg == -1)
		fg = col[COLWHITE];
	vsync();
	acquire_screen();
	rect(screen, 320 - 200-1, 240 - 50-1, 320 + 200-1, 240 + 50-1, col[COLWHITE]);
	rect(screen, 320 - 200+1, 240 - 50+1, 320 + 200+1, 240 + 50+1, col[COLDARKGRAY]);
	rectfill(screen, 320 - 200, 240 - 50, 320 + 200, 240 + 50, bg);
	textprintf_centre_ex(screen, font, 320, 240 - 25, fg, -1, "%s", t1.c_str());
	textprintf_centre_ex(screen, font, 320, 240     , fg, -1, "%s", t2.c_str());
	textprintf_centre_ex(screen, font, 320, 240 + 25, fg, -1, "%s", t3.c_str());
	release_screen();
}

void Graphics::build_flagpos_marks() {
	if (!flagpos_ready) {
		const int radius = flagpos_radius;
		for (int i = 0; i < 2; i++) {
			if (!flagpos_buf[i])
  				flagpos_buf[i] = create_bitmap(2 * radius, 2 * radius);
			clear_to_color(flagpos_buf[i], col[COLGROUND]);
		}
		int r1, r2;
		int b1, b2;
		r1 = r2 = getr(col[COLGROUND]);
		const int g = getg(col[COLGROUND]);
		b1 = b2 = getb(col[COLGROUND]);
		const int step = 10;
		for (int i = radius; i >= 0; i--) {
			// red flag
			r1 = min(r1 + step, 255);
			circlefill(flagpos_buf[0], radius, radius, i, makecol(r1, g, b1));
			// blue flag
			b2 = min(b2 + step, 255);
			circlefill(flagpos_buf[1], radius, radius, i, makecol(r2, g, b2));
		}
		flagpos_ready = true;
	}
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
void Graphics::create_deathbringer(int owner, double start_time, int x, int y, int px, int py) {
	clientfx_t fx;

	fx.owner = owner;	//deathbringer owner
	fx.type = FX_DEATHBRINGER_EXPLOSION;
	fx.x = x;
	fx.y = y;
	fx.time = start_time;
	fx.px = px;
	fx.py = py;

	cfx.push_back(fx);
}

//create deathbringer carrier trail fx
void Graphics::create_deathcarrier(int x, int y, int px, int py, int team) {
	clientfx_t fx;

	fx.type = FX_DEATHCARRIER_SMOKE;
	fx.x = x;
	fx.y = y;
	fx.px = px;
	fx.py = py;
	fx.time = get_time();

	//owner: set color
	int r = rand() %100;
	if (team) {
		if (r < 50)
			fx.col1 = makecol(0,0,0xff);
		else if (r < 75)
			fx.col1 = makecol(0,0xff,0);
		else
			fx.col1 = 0;
	} else {
		if (r < 50)
			fx.col1 = makecol(0xff,0,0);
		else if (r < 75)
			fx.col1 = makecol(0,0xff,0);
		else
			fx.col1 = 0;
	}

	//JUST BLACK
	fx.col1 = 0;

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
	for (list<clientfx_t>::iterator fx = cfx.begin(); fx != cfx.end(); fx++)
		// on same room
		if (fx->px == room_x && fx->py == room_y) {
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
					}
					break;

				case FX_SPEED:		// speed effect, draw another time in another function
					break;

				case FX_WALL_EXPLOSION:
					if (delta > 0.2)
						fx = cfx.erase(fx);
					else {
						for (int e = 0; e < 2; e++) {
							int rad = 4 + e + (int)(delta * 40);
							draw_gun_explosion(fx->x, fx->y, rad);
						}
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
					}
					break;

				case FX_DEATHBRINGER_EXPLOSION:
					if (delta > 3.0)
						fx = cfx.erase(fx);
					else
						draw_deathbringer(fx->x, fx->y, fx->owner / TSIZE, delta);
					break;

				case FX_DEATHCARRIER_SMOKE:
					if (delta > 0.6)
						fx = cfx.erase(fx);
					else
						draw_deathbringer_smoke(fx->x, fx->y, delta);
					break;
			}
		}
}

// draw speed effect
void Graphics::draw_speedfx(int room_x, int room_y, double time) {
	for (list<clientfx_t>::iterator fx = cfx.begin(); fx != cfx.end(); fx++)
		// on same room
		if (fx->px == room_x && fx->py == room_y) {
			if (fx->type == FX_SPEED) {
				double delta = time - fx->time;
				if (delta > 0.3)
					fx = cfx.erase(fx);
				else {
					int alpha = 90 - (int)(delta * 300.0);
					draw_player(fx->x, fx->y, fx->col1, fx->col2, fx->gundir, time, false, alpha, time);
				}
			}
		}
}

bool Graphics::save_map_picture(const string& filename, const Map& map) {
	const int old_minimap_p_w = minimap_place_w;
	const int old_minimap_p_h = minimap_place_h;
	minimap_place_w *= 2;
	minimap_place_h *= 2;
	BITMAP* buffer = create_bitmap(minimap_place_w, minimap_place_h);
	update_minimap_background(buffer, map, false, true);
	BITMAP* clip = create_sub_bitmap(buffer, minimap_start_x, minimap_start_y, minimap_w, minimap_h);
	PALETTE pal;
	get_palette(pal);
	minimap_place_w = old_minimap_p_w;
	minimap_place_h = old_minimap_p_h;
	return !save_bitmap(filename.c_str(), clip, pal);
}

