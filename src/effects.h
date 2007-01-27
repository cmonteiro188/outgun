/*
 *  effects.h
 *
 *  Copyright (C) 2002 - Fabio Reis Cecin
 *  Copyright (C) 2004 - Jani Rivinoja
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

#ifndef EFFECTS_H_INC
#define EFFECTS_H_INC

// client side effects

enum FX_TYPE {
    FX_GUN_EXPLOSION,
    FX_TURBO,
    FX_WALL_EXPLOSION,
    FX_POWER_WALL_EXPLOSION,
    FX_DEATHBRINGER_EXPLOSION,
    FX_DEATHCARRIER_SMOKE
};

struct GraphicsEffect {
    FX_TYPE type;       // type of fx
    int px, py;         // screen where it spawned. if changed when time to redraw, delete it
    double time;        // start time

    //fx specific vars
    int x;  // screen x  of fx
    int y;  // screen y  of fx

    int team;
    float alpha;  // [0, 1]
    // for turbo effect
    int col1, col2, gundir;

    GraphicsEffect(FX_TYPE type_, int px_, int py_, int x_, int y_, double time_, int team_, float alpha_ = 0, int col1_ = 0, int col2_ = 0, int gundir_ = 0)
        : type(type_), px(px_), py(py_), time(time_), x(x_), y(y_), team(team_), alpha(alpha_), col1(col1_), col2(col2_), gundir(gundir_) { }
};

#endif // EFFECTS_H_INC
