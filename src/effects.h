#ifndef EFFECTS_H_INC
#define EFFECTS_H_INC
#include "commont.h"
#include "graphics.h"
#include "sounds.h"

// size of clientside visual fx array
#define MAX_CLIENTFX 128

//explosion clientside fx
struct clientfx_t {
    bool        used;       //used record?

    int         type;       // type of fx   0==gun explosion
    int         px,py;  // screen where it spawned. if changed when time to redraw, delete it
    double time;        // start time

    //fx specific vars
    int x;                  // screen x  of fx
    int y;                  // screen y  of fx

    //speed fx
    int col1, col2, gundir;

    //deathbringer owner
    int owner;
};

class Effects {
    clientfx_t cfx[MAX_CLIENTFX];

public:
	Effects();
	~Effects() { }

    void clear_fx();
    int get_new_cfx() const;

    void draw(Graphics& graphics, int room_x, int room_y, double time);
    void draw_speedfx(Graphics& graphics, int room_x, int room_y, double time);

    void create_wallexplo(int x, int y, int px, int py, const Sounds& sounds);
    void create_quadwallexplo(int x, int y, int px, int py, const Sounds& sounds);
    void create_deathbringer(int owner, double start_time, int x, int y, int px, int py, const Sounds& sounds);
    void create_deathcarrier(int x, int y, int px, int py, int team);
    void create_gunexplo(int x, int y, int px, int py, const Sounds& sounds);
    void create_speedfx(int x, int y, int px, int py, int col1, int col2, int gundir);
};

#endif // EFFECTS_H_INC

