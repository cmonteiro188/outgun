#ifndef EFFECTS_H_INC
#define EFFECTS_H_INC

// client side effects

enum FX_TYPE {
	FX_GUN_EXPLOSION,
	FX_SPEED,
	FX_WALL_EXPLOSION,
	FX_POWER_WALL_EXPLOSION,
	FX_DEATHBRINGER_EXPLOSION,
	FX_DEATHCARRIER_SMOKE
};

struct clientfx_t {
	FX_TYPE type;		// type of fx
	int px, py;			// screen where it spawned. if changed when time to redraw, delete it
	double time;		// start time

	//fx specific vars
	int x;	// screen x  of fx
	int y;	// screen y  of fx

	//speed fx
	int col1, col2, gundir;

	//deathbringer owner
	int owner;
};

#endif // EFFECTS_H_INC

