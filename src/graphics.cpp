#define AL_FUNC_DEPRECATED AL_FUNC
#define AL_PRINTFUNC_DEPRECATED AL_PRINTFUNC
#define AL_INLINE_DEPRECATED AL_INLINE

#include "commont.h"
#include <allegro.h>
#include "graphics.h"

using std::string;

Graphics::Graphics(int scr_w, int scr_h):
	flagpos_ready(false)
{
	reset_video_mode();
	flagpos_buf[0] = 0;
	flagpos_buf[1] = 0;
	drawbuf = create_bitmap(scr_w, scr_h);
	minibg = create_bitmap(100, 100);
	setcolors();
}

Graphics::~Graphics() {
	destroy_bitmap(drawbuf);
	destroy_bitmap(minibg);
	destroy_bitmap(flagpos_buf[0]);
	destroy_bitmap(flagpos_buf[1]);
}

void Graphics::draw_screen() const {
	acquire_screen();
	blit(drawbuf, screen, 0,0, 0,0, SCREEN_W,SCREEN_H);
	release_screen();
}

void Graphics::setcolors() {
	//the first 8 colors are player's colors
	col[COLGREEN] = makecol(0,0xff,0);
	col[COLYELLOW] = makecol(0xff,0xff,0);
	col[COLWHITE] = makecol(0xff,0xff,0xff);
	col[COLMAG]	= makecol(0xff, 0, 0xff);
	col[COLCYAN] = makecol(0, 0xff, 0xff);
	col[COLORA]	= makecol(0xff, 0xb0, 0);
	col[COLLRED] = makecol(0xff,0x55,0x44);
	col[COLLBLUE] = makecol(0x44,0x55,0xff);
	//MORE player colors
	col[COL9] = makecol(242, 158, 224);
	col[COL10] = makecol(134, 143, 57);
	col[COL11] = makecol( 14, 148, 87);
	col[COL12] = makecol( 33, 132, 137);
	col[COL13] = makecol(100, 100, 100);
	col[COL14] = makecol(166, 166, 166);
	col[COL15] = makecol(202, 1, 56);	//wine
	col[COL16] = makecol(0xbf, 0x70, 0);	//darkora

	// team solid colors
	col[COLBLUE] = makecol(0,0,0xff);
	col[COLRED] = makecol(0xff,0,0);

	// base minimap background colors
	col[COLBBLUE] = makecol(0,0,0x66);
	col[COLBRED] = makecol(0x66,0,0);

	//other
	col[COLFOGOFWAR] = makecol(0xff, 0xff, 0xff);
	col[COLMENUWHITE] = makecol(0xc0,0xc0,0xc0);
	col[COLMENUGRAY] = makecol(0x68,0x68,0x68);
	col[COLMENUBLACK] = makecol(0x40,0x40,0x40);
	col[COLGROUND_DEF] = col[COLGROUND] = makecol(0x10, 0x40, 0);
	col[COLWALL_DEF] = col[COLWALL] = makecol(0x30, 0xC0, 0);
	col[COLNOLIFE] = makecol(0,0,0);
	col[COLDARKGRAY] = makecol(0x30, 0x30, 0x30);
	col[COLSHADOW] = makecol(0x18,0x18,0x18);
	col[COLLIMBO] = makecol(0x10, 0x10, 0x10);
	col[COLDARKORA]	= makecol(0xbf, 0x70, 0);
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

	text_mode(-1);	//transparent text

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
	const int pix = mmx + 21 + ((int)(px * 98));
	const int piy = mmy + 01 + ((int)(py * 98));
	//draw flagpole
	rectfill(drawbuf, pix, piy - 5, pix, piy, col[COLYELLOW]);
	//draw the flag itself
	rectfill(drawbuf, pix + 1, piy - 5, pix + 5, piy - 2, teamcol[team]);
}

void Graphics::draw_minimap_player(int x, int y, int team, int player) {
	putpixel(drawbuf, x + 0, y + 0, teamcol[team]);	//3 pixel teamcol
	putpixel(drawbuf, x + 1, y + 0, teamcol[team]);	//3 pixel teamcol
	putpixel(drawbuf, x + 0, y + 1, teamcol[team]);	//3 pixel teamcol
	putpixel(drawbuf, x + 1, y + 1, col[player]);	// 1 pixel personal-color
}

void Graphics::draw_minimap_me(int x, int y, int team, double time) {
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
	rectfill(drawbuf, mmx + x1, mmy + y1, mmx + x2, mmy + y2, col[COLFOGOFWAR]);
	solid_mode();
}

void Graphics::draw_minimap_background() {
	blit(minibg, drawbuf, 0, 0, mmx + 20, mmy, minibg->w, minibg->h);
}

void Graphics::update_minimap_background(const Map& map) {
	update_minimap_background(minibg, map, false);
}

void Graphics::update_minimap_background(BITMAP* buffer, const Map& map, bool flagPaintSimple) {
	//black background
	clear_to_color(buffer, 0);

	//draw screen boundaries
	int MMSCRW = (int)(98.0 / map.w);
	int MMSCRH = (int)(98.0 / map.h);
	for (int i = 1; i < map.w; i++)
		line(buffer, 2 + MMSCRW * i, 1, 2 + MMSCRW * i, 100, col[COLSHADOW]);
	for (int i = 1; i < map.h; i++)
		line(buffer, 1, 2 + MMSCRH * i, 100, 2 + MMSCRH * i, col[COLSHADOW]);

	double maxx = plw * map.w;
	double maxy = plh * map.h;

	//draw bases
	if (flagPaintSimple) {
		int  red_rx = map.tinfo[0].flag.px,  red_ry = map.tinfo[0].flag.py;
		int blue_rx = map.tinfo[1].flag.px, blue_ry = map.tinfo[1].flag.py;
		if (red_rx == blue_rx && red_ry == blue_ry) {
			// for lack of mathematical enthusiasm, the half-way line is not calculated analytically; instead, determine each pixel individually (slow)
			// this is OK since this function is called once per map instead of every frame
			int xmin = 2 + MMSCRW * red_rx, xmax = MMSCRW * (red_rx + 1);
			int ymin = 2 + MMSCRH * red_ry, ymax = MMSCRH * (red_ry + 1);
			for (int y = ymin; y <= ymax; ++y) {
				float roomy = float(y + 1 - ymin) / float(MMSCRH) * plh;
				float ydist_r2 = pow(map.tinfo[0].flag.y - roomy, 2);
				float ydist_b2 = pow(map.tinfo[1].flag.y - roomy, 2);
				for (int x = xmin; x <= xmax; ++x) {
					float roomx = float(x + 1 - xmin) / float(MMSCRW) * plw;
					float xdist_r2 = pow(map.tinfo[0].flag.x - roomx, 2);
					float xdist_b2 = pow(map.tinfo[1].flag.x - roomx, 2);
					int color = (xdist_r2 + ydist_r2 < xdist_b2 + ydist_b2) ? col[COLBRED] : col[COLBBLUE];
					putpixel(buffer, x, y, color);
				}
			}
		}
		else {
			rectfill(buffer, 2 + MMSCRW * red_rx,  2 + MMSCRH * red_ry,  MMSCRW * (red_rx + 1),  MMSCRH * (red_ry + 1),  col[COLBRED]);
			rectfill(buffer, 2 + MMSCRW * blue_rx, 2 + MMSCRH * blue_ry, MMSCRW * (blue_rx + 1), MMSCRH * (blue_ry + 1), col[COLBBLUE]);
		}
	}

	//draw solid walls
	float xmul = 98. / maxx, ymul = 98. / maxy;
	for (int y = 0; y < map.h; y++) {
		float by = 1. + y * plh * ymul;
		for (int x = 0; x < map.w; x++) {
			float bx = 1. + x * plw * xmul;
			map.room[x][y].draw(buffer, bx, by, xmul, ymul, makecol(0x00, 0x77, 0x00));
		}
	}

	//green border
	rect(buffer, 0, 0, buffer->w -1, buffer->h -1, col[COLGREEN]);

	int  red_px = int(1 + (map.tinfo[0].flag.px * plw + map.tinfo[0].flag.x) / maxx * 98.);
	int  red_py = int(1 + (map.tinfo[0].flag.py * plh + map.tinfo[0].flag.y) / maxy * 98.);
	int blue_px = int(1 + (map.tinfo[1].flag.px * plw + map.tinfo[1].flag.x) / maxx * 98.);
	int blue_py = int(1 + (map.tinfo[1].flag.py * plh + map.tinfo[1].flag.y) / maxy * 98.);
	if (!flagPaintSimple) {
		if (getpixel(buffer, red_px, red_py) != 0)	// is painted with any color
			return update_minimap_background(buffer, map, true);	// restart with basic painting
		floodfill(buffer, red_px, red_py, col[COLBRED]);

		if (getpixel(buffer, blue_px, blue_py) != 0)	// is painted with any color (including by previous red floodfill)
			return update_minimap_background(buffer, map, true);	// restart with basic painting
		floodfill(buffer, blue_px, blue_py, col[COLBBLUE]);
	}
	circle(buffer,  red_px,  red_py, 3, col[COLRED ]);
	circle(buffer, blue_px, blue_py, 3, col[COLBLUE]);
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
	yg += y - 15;

	if (alpha < 255) {
		set_trans_blender(0, 0, 0, alpha);
		drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
	}

	//desenha arma antes se dir 5,6,7
	if (gundir >= 5) {
		line(drawbuf, 0 + plx + x, 0 + ply + y - 15, 0 + plx + xg, 0 + ply + yg, pc1);
		line(drawbuf, 1 + plx + x, 0 + ply + y - 15, 1 + plx + xg, 0 + ply + yg, pc1);
		line(drawbuf, 1 + plx + x, 1 + ply + y - 15, 1 + plx + xg, 1 + ply + yg, pc1);
	}

	// outer color: team color
	circlefill(drawbuf, plx + x, ply + y - 15, 15, pc1);
	// inner color: self color
	circlefill(drawbuf, plx + x, ply + y - 15, 10, pc2);

	//desenha arma depois se dir 0,1,2,3,4
	if (gundir < 5) {
		line(drawbuf, 0 + plx + x, 0 + ply + y - 15, 0 + plx + xg, 0 + ply + yg, pc1);
		line(drawbuf, 1 + plx + x, 0 + ply + y - 15, 1 + plx + xg, 0 + ply + yg, pc1);
		line(drawbuf, 1 + plx + x, 1 + ply + y - 15, 1 + plx + xg, 1 + ply + yg, pc1);
	}

	if (alpha < 255)
		solid_mode();
}

void Graphics::draw_player_shadow(const ClientPlayer& player, int alpha) {
	drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
	set_trans_blender(0, 0, 0, alpha);
	ellipsefill(drawbuf, plx + (int)player.lx, ply + (int)player.ly, 15, 3, col[COLSHADOW]);
	solid_mode();
}

void Graphics::draw_virou_sorvete(int x, int y) {
	ellipsefill(drawbuf, plx + x, ply + y - 15, 6, 15, col[COLORA]);
	circlefill(drawbuf, plx + x - 8, ply + y - 10-15, 8, col[COLBLUE]);
	circlefill(drawbuf, plx + x + 8, ply + y - 10-15, 8, col[COLMAG]);
	circlefill(drawbuf, plx + x + 0, ply + y - 20-15, 8, col[COLGREEN]);
	textprintf_centre(drawbuf, font, plx + x + 0, ply + y - 20-43, col[COLWHITE], "VIROU");
	textprintf_centre(drawbuf, font, plx + x + 0, ply + y - 20-33, col[COLWHITE], "SORVETE!");
}

void Graphics::draw_player_dead(int x, int y) {
	ellipsefill(drawbuf, plx + x, ply + y, 20, 6, col[COLRED]);
	circlefill(drawbuf, plx + x, ply + y - 10, 12, col[COLRED]);
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
		circlefill(drawbuf, plx + x + rand() % 40 - 20, ply + y + rand() % 40 - 20 - 15, 15, teamcol[team]);
		for (int j = 0; j < 5; j++)
			circlefill(drawbuf, plx + x + rand() % 40 - 20, ply + y + rand() % 40 - 20 - 15, 15, 0);
	solid_mode();
}

void Graphics::draw_deathbringer_carrier_effect(int x, int y) {
	set_clip(drawbuf, plx, ply, plx + plw, ply + plh);
	//darken ground
	drawing_mode(DRAW_MODE_TRANS, 0,0,0);
	for (int r = 50; r > 0; r -= 5) {
		set_trans_blender(0, 0, 0, 50 - r);
		circlefill(drawbuf, plx + x, ply + y - 10, r, 0);
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
	int c = teamdcol[team];
	textprintf_centre(drawbuf, font, x - 1, y - 1, c, "%s", name.c_str());
	textprintf_centre(drawbuf, font, x + 1, y - 1, c, "%s", name.c_str());
	textprintf_centre(drawbuf, font, x - 1, y + 1, c, "%s", name.c_str());
	textprintf_centre(drawbuf, font, x + 1, y + 1, c, "%s", name.c_str());
	textprintf_centre(drawbuf, font, x - 1, y, c, "%s", name.c_str());
	textprintf_centre(drawbuf, font, x + 1, y, c, "%s", name.c_str());
	textprintf_centre(drawbuf, font, x, y - 1, c, "%s", name.c_str());
	textprintf_centre(drawbuf, font, x, y + 1, c, "%s", name.c_str());
	textprintf_centre(drawbuf, font, x, y, col[COLWHITE], "%s", name.c_str());
}

void Graphics::draw_rocket(const rocket_c& rocket, double time) {
	if (rocket.power) {	// powered rocket
		//draw rocket shadow
		ellipsefill(drawbuf, plx + (int)rocket.x, ply + (int)rocket.y, 6, 3, col[COLSHADOW]);
		//draw the rocket
		if ((int)(time * 30) % 2)
			circlefill(drawbuf, plx + (int)rocket.x, ply + (int)rocket.y - 15, 6, col[COLWHITE]);	//y-12?
		else
			circlefill(drawbuf, plx + (int)rocket.x, ply + (int)rocket.y - 15, 4, teamlcol[rocket.team]); //y-12??
	}
	else {				// normal rocket
		//draw rocket shadow
		ellipsefill(drawbuf, plx + (int)rocket.x, ply + (int)rocket.y, 4, 2, col[COLSHADOW]);
		//draw the rocket
		circlefill(drawbuf, plx + (int)rocket.x, ply + (int)rocket.y - 15, 4, teamcol[rocket.team]); //y-10??
	}
}

void Graphics::draw_playground() {
	rectfill(drawbuf, plx, ply, plx + plw, ply + plh, col[COLGROUND]);
}

void Graphics::draw_flagpos_mark(int team, int flag_x, int flag_y) {
	build_flagpos_marks();	// draw flag position mark sprites
	int x1 = max(0, flagpos_radius - flag_x);
	int y1 = max(0, flagpos_radius - flag_y);
	int x2 = min(2 * flagpos_radius, plw - flag_x + flagpos_radius + 1);
	int y2 = min(2 * flagpos_radius, plh - flag_y + flagpos_radius + 1);
	blit(flagpos_buf[team], drawbuf, x1, y1,
		plx + flag_x - flagpos_radius + x1, ply + flag_y - flagpos_radius + y1, x2 - x1, y2 - y1);
}

void Graphics::draw_walls(const Room& room) {
	room.draw(drawbuf, plx, ply, 1., 1., col[COLWALL]);
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
		case 7: draw_pup_deathbringer(pup.x, pup.y); break;	//deathbringer
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
	textprintf_centre(drawbuf, font, plx + plw / 2, ply + plh / 2, col[COLGREEN], message.c_str());
}

void Graphics::draw_waiting_map_message(const string& caption, const string& map) {
	textprintf_centre(drawbuf, font, plx + plw / 2, ply + plh / 2 + 20, col[COLGREEN], caption.c_str());
	textprintf_centre(drawbuf, font, plx + plw / 2, ply + plh / 2 + 50, col[COLORA], map.c_str());
}

void Graphics::draw_scores(const string& text, int team, int score1, int score2) {
	int c;
	switch (team) {
		case 0: c = col[COLLRED]; break;
		case 1: c = col[COLLBLUE]; break;
		default: c = col[COLMENUGRAY]; break;
	}
	textprintf_centre(drawbuf, font, plx + plw / 2, ply + plh / 2 - 40, c, text.c_str());
	textprintf_centre(drawbuf, font, plx + plw / 2, ply + plh / 2 - 20, c, "SCORE: %i - %i", score1, score2);
}

void Graphics::draw_scoreboard_caption(int team, const string& caption) {
	const int nameydelta_min = 8;
	textprintf(drawbuf, font, sbx + 4, sby - 4 + team * 18 * nameydelta_min, teamlcol[team], caption.c_str());
}

void Graphics::draw_scoreboard_name(int y, int pcol, const ClientPlayer& player) {
	textprintf(drawbuf, font, sbx + 4, y, col[pcol], "%c%s", player.reg_status, player.name);
}

void Graphics::draw_scoreboard_points(int y, int team, int points) {
	textprintf(drawbuf, font, sbx + 4 + 16 * 8, y, teamlcol[team], "%4i", points);
}

void Graphics::draw_fps(double fps) {
	textprintf(drawbuf, font, plx + 10, ply + plh - 14, 0, "FPS:%3.0f", fps);
}

void Graphics::draw_player_power(double val) {
	textprintf(drawbuf, font, plx + 244, ply + plh + 5, col[COLCYAN], "POWER:  %2.0f", val);
}

void Graphics::draw_player_turbo(double val) {
	textprintf(drawbuf, font, plx + 244, ply + plh + 15, col[COLYELLOW], "TURBO:  %2.0f", val);
}

void Graphics::draw_player_shadow(double val) {
	textprintf(drawbuf, font, plx + 244, ply + plh + 25, col[COLMAG], "SHADOW: %2.0f", val);
}

void Graphics::draw_player_weapon(int level) {
	textprintf(drawbuf, font, plx + 340, ply + plh + 5, col[COLWHITE], "WEAPON: %i", level);
}

void Graphics::draw_change_team_message(double time) {
	int c = col[COLWHITE];
	if ((int)(time * 2.0) % 2)	// blink!
		c = col[COLRED];
	textprintf(drawbuf, font, plx + plw - 6 * 8 - 10,     ply + plh - 18, c, "CHANGE");
	textprintf(drawbuf, font, plx + plw - 6 * 8 - 10 + 4, ply + plh -  9, c, "TEAMS");
}

void Graphics::draw_change_map_message(double time) {
	int c = col[COLWHITE];
	if ((int)(time * 2.0) % 2)	// blink!
		c = col[COLRED];
	textprintf(drawbuf, font, -40 + plx + plw - 6 * 8 - 10,     ply + plh - 18, c, "EXIT");
	textprintf(drawbuf, font, -40 + plx + plw - 6 * 8 - 10 + 4, ply + plh -  9, c, "MAP");
}

void Graphics::draw_player_health(int health) {
	// health value
	textprintf(drawbuf, font, 10, ply + plh + 5, col[COLWHITE],  "Health: %4i", health);
	// health bar
	rectfill(drawbuf, 10, ply + plh + 18, 10 + 100, ply + plh + 18 + 10, col[COLNOLIFE]);
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
	textprintf(drawbuf, font, 10 + 14 * 8, ply + plh + 5, col[COLWHITE],  "Energy: %4i", energy);
	// energy bar
	rectfill(drawbuf, 10 + 14 * 8, ply + plh + 18, 10 + 14 * 8 + 100, ply + plh + 18 + 10, col[COLNOLIFE]);
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
	textprintf(drawbuf, font, 3, 3 + line * 11, c, "%s", message.c_str());
}

void Graphics::print_chat_input(int line, const string& message) {
	const int x = 3;
	const int y = 3;
	const int lh = 11;
	//nice border
	textprintf(drawbuf, font, x + 1, y + 0 + line * lh, 0, "%s", message.c_str());
	textprintf(drawbuf, font, x + 1, y + 1 + line * lh, 0, "%s", message.c_str());
	textprintf(drawbuf, font, x + 0, y + 1 + line * lh, 0, "%s", message.c_str());
	textprintf(drawbuf, font, x - 1, y + 1 + line * lh, 0, "%s", message.c_str());
	textprintf(drawbuf, font, x - 1, y + 0 + line * lh, 0, "%s", message.c_str());
	textprintf(drawbuf, font, x - 1, y - 1 + line * lh, 0, "%s", message.c_str());
	textprintf(drawbuf, font, x + 0, y - 1 + line * lh, 0, "%s", message.c_str());
	textprintf(drawbuf, font, x + 1, y - 1 + line * lh, 0, "%s", message.c_str());
	// the prompt text
	textprintf(drawbuf, font, x, y + line * lh, col[COLWHITE], "%s", message.c_str());
}

void Graphics::show_not_responding_message() {
	rect(drawbuf,  194,  199, 444, 279, col[COLMENUWHITE]);
	rect(drawbuf, 196, 201, 446, 281, col[COLMENUBLACK]);
	rectfill(drawbuf, 195, 200, 445, 280, col[COLMENUGRAY]);
	textprintf(drawbuf, font, 220, 220, col[COLWHITE], "SERVER NOT RESPONDING...");
	textprintf(drawbuf, font, 220, 240, col[COLWHITE], "May be heavy packet loss,");
	textprintf(drawbuf, font, 220, 255, col[COLWHITE], "or the server disconnected");
}

// draw help
void Graphics::draw_game_help() {
	clear_to_color(drawbuf, col[COLMENUGRAY]);

	int x = -40;
	int y = -70;

	textprintf(drawbuf, font, x+100, y+100, col[COLWHITE], "Outgun : HELP      --> Press ESC or F1 to go back. <--");

	textprintf(drawbuf, font, x+100, y+120, col[COLWHITE], "For more information access these websites:");
	textprintf(drawbuf, font, x+100, y+130, col[COLWHITE], "  the original Outgun website");
	textprintf(drawbuf, font, x+364, y+130, col[COLGREEN], "http://www.amok.com.br/outgun/en/");
	textprintf(drawbuf, font, x+100, y+140, col[COLWHITE], "  Nix's Outgun development page");
	textprintf(drawbuf, font, x+364, y+140, col[COLGREEN], "http://koti.mbnet.fi/npr/outgun/");

	textprintf(drawbuf, font, x+100, y+160, col[COLWHITE], "           MOVING            ARROW KEYS = MOVE");
	textprintf(drawbuf, font, x+100, y+170, col[COLWHITE], "            YOUR             CONTROL    = SHOOT!");
	textprintf(drawbuf, font, x+100, y+180, col[COLWHITE], "          CHARACTER:         ALT        = STRAFE");
	textprintf(drawbuf, font, x+100, y+190, col[COLWHITE], "            >>>>>            SHIFT      = RUN");

	textprintf(drawbuf, font, x+100, y+210, col[COLWHITE], "TALKING TO ALL PLAYERS: Just type your message and hit ENTER");

	textprintf(drawbuf, font, x+100, y+230, col[COLWHITE], "TALKING JUST TO YOUR TEAM: Just place a dot ('.') at the very");
	textprintf(drawbuf, font, x+100, y+240, col[COLWHITE], " beginning of your message (first char)");

	textprintf(drawbuf, font, x+100, y+260, col[COLWHITE], "GAME CONCEPT: You are a member of a team, either RED or BLUE,");
	textprintf(drawbuf, font, x+100, y+270, col[COLWHITE], " assigned to you at random when you connect. Your goal is to");
	textprintf(drawbuf, font, x+100, y+280, col[COLWHITE], " help your team to win, by capturing 8 (default) times the enemy");
	textprintf(drawbuf, font, x+100, y+290, col[COLWHITE], " flag. To capture the flag, a member of your team must steal");
	textprintf(drawbuf, font, x+100, y+300, col[COLWHITE], " the enemy flag and bring it to your team's flag, provided your");
	textprintf(drawbuf, font, x+100, y+310, col[COLWHITE], " flag has not been stolen already! Capiche?");

	textprintf(drawbuf, font, x+100, y+330, col[COLWHITE], "HEALTH AND ENERGY: If your health reaches zero, you die. Energy");
	textprintf(drawbuf, font, x+100, y+340, col[COLWHITE], " is used for running, shooting and health protection when you");
	textprintf(drawbuf, font, x+100, y+350, col[COLWHITE], " have the SHIELD powerup (you'll know when you see it...).");
	textprintf(drawbuf, font, x+100, y+360, col[COLWHITE], " Health and energy regenerate with time.");

	textprintf(drawbuf, font, x+100, y+380, col[COLWHITE], "MINIMAP: On the upper-right corner of the screen is the minimap.");
	textprintf(drawbuf, font, x+100, y+390, col[COLWHITE], " It shows the contents of all rooms of the map that have at least");
	textprintf(drawbuf, font, x+100, y+400, col[COLWHITE], " one player of your team.");

	textprintf(drawbuf, font, x+100, y+420, col[COLWHITE], "CHANGING TEAMS: Hit the END key to set whether you want to");
	textprintf(drawbuf, font, x+100, y+430, col[COLWHITE], " change team or not. You will change team when appropriate.");

	textprintf(drawbuf, font, x+100, y+450, col[COLWHITE], "POWERUPS: If you see an animated item lying on the ground, grab");
	textprintf(drawbuf, font, x+100, y+460, col[COLWHITE], " it. It's a special power-up item.");

	textprintf(drawbuf, font, x+100, y+480, col[COLWHITE], "ETC.: Hit DEL to kill yourself. Hold TAB to see other players'");
	textprintf(drawbuf, font, x+100, y+490, col[COLWHITE], " ping times (in milliseconds) on the scoreboard.");
	textprintf(drawbuf, font, x+100, y+500, col[COLWHITE], " Hit HOME to change world colors and CTRL+HOME to restore them.");
	textprintf(drawbuf, font, x+100, y+510, col[COLWHITE], " Hit F10 to receive a random name. Hit F11 to take a screenshot.");
}

/*
//draws the game menu
void Graphics::draw_game_menu(int menu) {
	//"3d" menu
	if (menu != 1) {
		rect(drawbuf,  99,  69, 539, 409, col[COLMENUWHITE]);
		rect(drawbuf, 101, 71, 541, 411, col[COLMENUBLACK]);
		rectfill(drawbuf, 100, 70, 540, 410, col[COLMENUGRAY]);
		textprintf(drawbuf, font, 150, 120, col[COLWHITE], "Outgun         version %s", GAME_VERSION);
		textprintf(drawbuf, font, 150, 135, col[COLGREEN], "http://koti.mbnet.fi/npr/outgun/");
	}
	if (menu == 0) {
		static int DELY = 10;

		textprintf(drawbuf, font, 150, 185-DELY, col[COLWHITE], "  [ 1 ]   Connect");
		textprintf(drawbuf, font, 150, 200-DELY, col[COLWHITE], "  [ 2 ]   Disconnect");
		if (connected)
			textprintf(drawbuf, font, 150+22*8, 200-DELY, col[COLGREEN], "(%s)", address);
		textprintf(drawbuf, font, 150, 215-DELY, col[COLWHITE], "  [ 3 ]   Change Player Name & Password");
		textprintf(drawbuf, font, 150, 227-DELY, col[COLGREEN], "          '%s' (%s)", playername, namestatus);
		textprintf(drawbuf, font, 150, 243-DELY, col[COLWHITE], "  [ 4 ]   Start/stop local server");
		if (listen_server_running)
			textprintf(drawbuf, font, 150, 255-DELY, col[COLGREEN], "          SERVER RUNNING ON PORT %i", listen_port_running);
		textprintf(drawbuf, font, 150, 271-DELY, col[COLWHITE], "  [ 5 ]   Toggle fullscreen/windowed mode");

		if (validtheme) {
			textprintf(drawbuf, font, 150, 286-DELY, col[COLWHITE], "  [ 6 ]   Change sound theme: (%s)", sfxthemedir);
			textprintf_centre(drawbuf, font, 150+180, 300-DELY, col[COLGREEN], "'%s'", sfxthemename);
		}
		else {
			textprintf(drawbuf, font, 150, 286-DELY, col[COLWHITE], "  [ 6 ]   Change sound theme:");
			textprintf(drawbuf, font, 150, 300-DELY, col[COLGREEN], "          no sfx themes found.");
		}
		textprintf(drawbuf, font, 150, 340-DELY, col[COLWHITE], "Hit CTRL+F12 to EXIT THE GAME");
		textprintf(drawbuf, font, 150, 355-DELY, col[COLWHITE], "Hit ESC to HIDE OR SHOW THIS MENU");
		textprintf(drawbuf, font, 150, 370-DELY, col[COLORA], "Hit F1 to SHOW THE HELP SCREEN");
	}
	else if (menu == 1) {

		//Big F Menu
		rect(drawbuf,  19,  19, 620, 460, col[COLMENUWHITE]);
		rect(drawbuf,  21,  21, 621, 461, col[COLMENUBLACK]);

		int lotext = makecol(0x99, 0x99, 0x99);


		if (showmaster) {

			int hi = makecol(0x68, 0x68, 0x88); //col[COLMENUGRAY]; //makecol(0x99,0x99,0x99);
			int lo = makecol(0x68,0x48,0x48);
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
			textprintf_centre(drawbuf, font, 170, 35, col[COLWHITE], "INTERNET SEARCH");
			textprintf_centre(drawbuf, font, 470, 35, lotext, "FAVORITES");
			//textprintf_centre(drawbuf, font, 320, 40, col[COLWHITE], "Showing INTERNET LISTING page (TAB = FAVORITES)");

			if ((int)time % 2)
				textprintf_centre(drawbuf, font, 320, 65, col[COLGREEN], "F2 = UPDATE LIST OF SERVERS");
			else
				textprintf_centre(drawbuf, font, 320, 65, col[COLYELLOW], "F2 = UPDATE LIST OF SERVERS");

			textprintf_centre(drawbuf, font, 320, 80, col[COLWHITE], "Press SPACE to refresh the servers");
			//textprintf_centre_x(drawbuf, font, 320, 75, col[COLGREEN], 0, "TAB = Change to FAVORITES page");

			//textprintf_centre(drawbuf, font, 320, 115, col[COLWHITE], "ARROWS:Select - ENTER:Connect - ESC:Cancel - SPACE:Refresh");

			textprintf_centre(drawbuf, font, 320, 440, col[COLWHITE], "TAB:Favorites  ARROWS:Select  ENTER:Connect  ESC:Cancel  SPACE:Refresh");
		}
		else {

			int hi = makecol(0x88, 0x68, 0x68); //col[COLMENUGRAY]; //makecol(0x99,0x99,0x99);
			int lo = makecol(0x48,0x48,0x68);
			//hilight all
			rectfill(drawbuf, 20, 20, 620, 460, hi);
			//first bar lo vs hi
			rectfill(drawbuf, 320, 20, 620, 50, hi);
			rectfill(drawbuf, 19, 19, 320, 50, 0);
			rectfill(drawbuf, 24, 24, 320, 50, lo);
			vline(drawbuf, 320, 19, 50, col[COLMENUWHITE]);//?
			hline(drawbuf, 19, 50, 320, col[COLMENUWHITE]);
			hline(drawbuf, 24, 24, 320, lotext);
			vline(drawbuf, 24, 24, 49, col[COLMENUWHITE]);
			textprintf_centre(drawbuf, font, 170, 35, lotext, "INTERNET SEARCH");
			textprintf_centre(drawbuf, font, 470, 35, col[COLWHITE], "FAVORITES");

			//textprintf_centre(drawbuf, font, 320, 40, col[COLWHITE], "Showing FAVORITES page (TAB = INTERNET LISTING)");
			textprintf_centre(drawbuf, font, 320, 65, col[COLWHITE], "Type the IP address of the server and hit ENTER");
			textprintf_centre(drawbuf, font, 320, 80, col[COLWHITE], "Press SPACE to refresh the servers");
			//textprintf_centre_x(drawbuf, font, 320, 75, col[COLYELLOW], 0, "TAB = Change to INTERNET LISTING page");

			textprintf_centre(drawbuf, font, 320, 440, col[COLWHITE], "TAB:Internet  ARROWS:Select  ENTER:Connect  ESC:Cancel  SPACE:Refresh");
		}

		int xi = 50 - 8*2;

		textprintf(drawbuf, font, xi, 105, col[COLWHITE], "IP Address             Ping #P Version/Hostname");

		char blinkchar[2];

		int yi;

		for (int i=0;i<MAX_GAMESPY;i++) {

			yi = 120 + i*13;

			//selectr
			if (gi == i) {
				rectfill(drawbuf, xi-3,yi-3,xi+550+8*3,yi+12,col[COLSHADOW]);

				//blink cursor
				if ((int)(time * 4) % 2)
					blinkchar[0]=' ';
				else
					blinkchar[0]='<';
				blinkchar[1]=0;
			}
			else
				blinkchar[0]=0;

			//server edit prompt
			if (showmaster) {
				textprintf(drawbuf, font, xi, yi, col[COLGREEN], ":%s%s",mgamespy[i].address, blinkchar);

				//favs watermarks
				if (mgamespy[i].favs)
					textprintf(drawbuf, font, xi - 12, yi, makecol(0x99,0x78,0x78), "*");
			}
			else
				textprintf(drawbuf, font, xi, yi, col[COLGREEN], ":%s%s",gamespy[i].address, blinkchar);

			//draw gamespy entry
			bool refreshed, invalid, noresponse;
			if (showmaster) {
				refreshed  = mgamespy[i].refreshed;
				invalid    = mgamespy[i].invalid;
				noresponse = mgamespy[i].noresponse;
			}
			else {
				refreshed  = gamespy[i].refreshed;
				invalid    = gamespy[i].invalid;
				noresponse = gamespy[i].noresponse;
			}

			if (!refreshed) { // not refreshed
				//server info
				textprintf(drawbuf, font, xi + (18+5)*8, yi, col[COLWHITE], "press SPACEBAR to refresh...");
			}
			else if (invalid) {	//refreshed, invalid
				//server info
				textprintf(drawbuf, font, xi + (18+5)*8, yi, col[COLWHITE], "---");
			}
			else if (noresponse) {	//refreshed, no response
				//server info
				textprintf(drawbuf, font, xi + (18+5)*8, yi, col[COLWHITE], "no response.");
			}
			else {  //refreshed, valid
				//server info
				if (showmaster)
					textprintf(drawbuf, font, xi + (18+5)*8, yi, col[COLGREEN], "%s", mgamespy[i].info);
				else
					textprintf(drawbuf, font, xi + (18+5)*8, yi, col[COLGREEN], "%s", gamespy[i].info);
			}
		}
	}
	else if (menu == 2) {
		textprintf(drawbuf, font, 150, 230, col[COLWHITE], dialogmessage);
		textprintf(drawbuf, font, 150, 250, col[COLWHITE], dialogmessage2);
	}
	else if (menu == 3) {
		textprintf(drawbuf, font, 150, 170, col[COLWHITE], "Type in your player name. If you have");
		textprintf(drawbuf, font, 150, 185, col[COLWHITE], "registered your name on the Outgun");
		textprintf(drawbuf, font, 150, 200, col[COLWHITE], "website, then type in your password!");

		textprintf(drawbuf, font, 150, 220, col[COLWHITE], "ENTER = OK   ESC = CANCEL  TAB = NEXT FIELD");
		textprintf(drawbuf, font, 150, 260, col[COLGREEN], "NAME     :%s%s", editplayername, namecursor);

		//password field: '********'
		char starpass[32]; int c=0;
		for (; editplayerpass[c]; c++) starpass[c] = '*';
		starpass[c] = 0;

		textprintf(drawbuf, font, 150, 285, col[COLGREEN], "PASSWORD :%s%s", starpass, passcursor);

		textprintf(drawbuf, font, 150, 350, col[COLWHITE], "Registration status: %s", namestatus);
	}
	else {
		textprintf(drawbuf, font, 150, 150, col[COLRED], "unknown menu %i", menu);
	}
}
*/

//show progress (for tight loops that don't work with the regular screen flip loop)
void Graphics::show_progress(const string& t1, const string& t2, const string& t3, int fg, int bg) {
	if (fg == -1)
		fg = col[COLWHITE];
	vsync();
	acquire_screen();
	rect(screen, 320 - 200-1, 240 - 50-1, 320 + 200-1, 240 + 50-1, col[COLWHITE]);
	rect(screen, 320 - 200+1, 240 - 50+1, 320 + 200+1, 240 + 50+1, col[COLDARKGRAY]);
	rectfill(screen, 320 - 200, 240 - 50, 320 + 200, 240 + 50, bg);
	textprintf_centre(screen, font, 320, 240 - 25, fg, "%s", t1.c_str());
	textprintf_centre(screen, font, 320, 240     , fg, "%s", t2.c_str());
	textprintf_centre(screen, font, 320, 240 + 25, fg, "%s", t3.c_str());
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

