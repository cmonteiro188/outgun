#include "effects.h"

Effects::Effects() {
	clear_fx();
}

//clear clientside fx's
void Effects::clear_fx() {
	for (int i = 0; i < MAX_CLIENTFX; i++)
		cfx[i].used = false;
}

//find new clientside fx
int Effects::get_new_cfx() const {
	for (int i = 0; i < MAX_CLIENTFX; i++)
		if (!cfx[i].used)
			return i;
	//print_message("overflow");
	return rand() % MAX_CLIENTFX;	//overwrite algum sorteado....
}

//create wall explosion fx
void Effects::create_wallexplo(int x, int y, int px, int py, const Sounds& sounds) {
	int f = get_new_cfx();

	cfx[f].used = true;
	cfx[f].type = 2;		// WALL EXPLOSION
	cfx[f].x = x;
	cfx[f].y = y;
	cfx[f].time = get_time();
	cfx[f].px = px;
	cfx[f].py = py;

	//sound
	sounds.play(SAMPLE_WALLHIT);
}

//create quad wall explosion fx
void Effects::create_quadwallexplo(int x, int y, int px, int py, const Sounds& sounds) {
	int f = get_new_cfx();

	cfx[f].used = true;
	cfx[f].type = 3;		// QUAD WALL EXPLOSION
	cfx[f].x = x;
	cfx[f].y = y;
	cfx[f].time = get_time();
	cfx[f].px = px;
	cfx[f].py = py;

	//sound
	sounds.play(SAMPLE_QUADWALLHIT);
}

//create deathbringer explosion fx
void Effects::create_deathbringer(int owner, double start_time, int x, int y, int px, int py, const Sounds& sounds) {
	int f = get_new_cfx();

	cfx[f].used = true;
	cfx[f].owner = owner;		//deathbringer owner
	cfx[f].type = 4;		// DEATHBRINGER EXPLOSION
	cfx[f].x = x;
	cfx[f].y = y;
	cfx[f].time = start_time;
	cfx[f].px = px;
	cfx[f].py = py;

	//sound
	sounds.play(SAMPLE_USEDEATHBRINGER);
}

//create deathbringer carrier trail fx
void Effects::create_deathcarrier(int x, int y, int px, int py, int team) {
	int f = get_new_cfx();

	cfx[f].used = true;
	cfx[f].type = 5;	//death carrier cloud fx
	cfx[f].x = x;
	cfx[f].y = y;
	cfx[f].px = px;
	cfx[f].py = py;
	cfx[f].time = get_time();

	//owner: set color
	int r = rand() %100;
	if (team) {
		if (r < 50)
			cfx[f].col1 = makecol(0,0,0xff);
		else if (r < 75)
			cfx[f].col1 = makecol(0,0xff,0);
		else
			cfx[f].col1 = 0;
	} else {
		if (r < 50)
			cfx[f].col1 = makecol(0xff,0,0);
		else if (r < 75)
			cfx[f].col1 = makecol(0,0xff,0);
		else
			cfx[f].col1 = 0;
	}

	//JUST BLACK
	cfx[f].col1 = 0;
}

//create explosion fx
void Effects::create_gunexplo(int x, int y, int px, int py, const Sounds& sounds) {
	int f = get_new_cfx();

	cfx[f].used = true;
	cfx[f].type = 0;		// GUN EXPLOSION
	cfx[f].x = x;
	cfx[f].y = y;
	cfx[f].time = get_time();
	cfx[f].px = px;
	cfx[f].py = py;

	//sound
	sounds.play(SAMPLE_HIT);
}

//create speed bolinha fx
void Effects::create_speedfx(int x, int y, int px, int py, int col1, int col2, int gundir) {
	int f = get_new_cfx();

	cfx[f].used = true;
	cfx[f].type = 1;	//speed fx
	cfx[f].x = x;
	cfx[f].y = y;
	cfx[f].px = px;
	cfx[f].py = py;
	cfx[f].time = get_time();

	cfx[f].col1 = col1;
	cfx[f].col2 = col2;
	cfx[f].gundir = gundir;
}

void Effects::draw(Graphics& graphics, int room_x, int room_y, double time) {
	for (int i = 0; i < MAX_CLIENTFX; i++)
		//fx used, on same screen
		if (cfx[i].used && cfx[i].px == room_x && cfx[i].py == room_y) {
			//gun explosion
			if (cfx[i].type == 0) {
				double delta = time - cfx[i].time;
				if (delta > 0.4)
					cfx[i].used = false;
				else {
					for (int e = 0; e < 3; e++) {
						int rad = 4 + e + (int)(delta * 40);
						graphics.draw_gun_explosion(cfx[i].x, cfx[i].y, rad);
					}
				}
			}
			//wall explosion
			else if (cfx[i].type == 2) {
				double delta = time - cfx[i].time;
				if (delta > 0.2)
					cfx[i].used = false;
				else {
					for (int e = 0; e < 2; e++) {
						int rad = 4 + e + (int)(delta * 40);
						graphics.draw_gun_explosion(cfx[i].x, cfx[i].y, rad);
					}
				}
			}
			//quad wall explosion
			else if (cfx[i].type == 3) {
				double delta = time - cfx[i].time;
				if (delta > 0.2)
					cfx[i].used = false;
				else {
					for (int e = 0; e < 3; e++) {
						int rad = 4 + e + (int)(delta * 60);
						graphics.draw_gun_explosion(cfx[i].x, cfx[i].y, rad);
					}
				}
			}
			// deathcarrier smoke
			else if (cfx[i].type == 5) {
				double delta = time - cfx[i].time;
				if (delta > 0.6)
					cfx[i].used = false;
				else
					graphics.draw_deathbringer_smoke(cfx[i].x, cfx[i].y, delta);
			}
			//the deathbringer
			else if (cfx[i].type == 4) {
				double delta = time - cfx[i].time;
				if (delta > 3.0)
					cfx[i].used = false;
				else
					graphics.draw_deathbringer(cfx[i].x, cfx[i].y, cfx[i].owner / TSIZE, delta);
			}
		}
}

void Effects::draw_speedfx(Graphics& graphics, int room_x, int room_y, double time) {
	for (int i = 0; i < MAX_CLIENTFX; i++)
		//fx used, on same screen
		if (cfx[i].used && cfx[i].px == room_x && cfx[i].py == room_y) {
			//speed rastro
			if (cfx[i].type == 1) {
				double delta = time - cfx[i].time;
				if (delta > 0.3)
					cfx[i].used = false;
				else {
					int alpha = 90 - (int)(delta * 300.0);
					graphics.draw_player(cfx[i].x, cfx[i].y, cfx[i].col1, cfx[i].col2, cfx[i].gundir, time, false, alpha, time);
				}
			}
		}
}

