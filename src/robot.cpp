/*
 *  robot.cpp
 *
 *  Copyright (C) 2006 - Peter Kosyh
 *  Copyright (C) 2006 - Niko Ritari
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

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <cctype>
#include <cmath>

#include "leetnet/client.h"
#include "commont.h"
#include "nassert.h"
#include "protocol.h"
#include "world.h"

#include "client.h"

#define averageLag (averageLag / 2)

using std::vector;

const int SCAN_RADIUS = ROCKET_RADIUS;

const int S_W = plw;
const int S_H = plh;
const int FADEOUT = 50;

inline int inv_dir(int d) {
    return (d + 4) % 8;
}

bool Client::IsBehindWall(double mex, double mey, double dx, double dy) const {
    double deg;

    if (dx == 0)
        deg = N_PI_2;
    else
        deg = atan(fabs(dy) / fabs(dx));

    if (dx >= 0 && dy <= 0)
        deg = 2 * N_PI - deg;
    else if (dx <= 0 && dy >= 0)
        deg = N_PI - deg;
    else if (dx <= 0 && dy <= 0)
        deg = N_PI + deg;

    const double tx = mex + dx;
    const double ty = mey + dy;
    const double sx = cos(deg) * 2 * SCAN_RADIUS;
    const double sy = sin(deg) * 2 * SCAN_RADIUS;
    const Room& room = fx.map.room[fx.player[me].roomx][fx.player[me].roomy];

    while (1) {
        if (room.fall_on_wall((int)mex, (int)mey, SCAN_RADIUS))
            return true;
        mex += sx;
        mey += sy;
        if (fabs(tx - mex) < PLAYER_RADIUS && fabs(ty - mey) < PLAYER_RADIUS)
            break;
        if (mex > S_W || mex < 0)
            break;
        if (mey > S_H || mey < 0)
            break;
    }
    return false;
}

double Client::ScanDir(double mex, double mey, int dir) const {
    const double deg =  N_PI_4 * dir;

    const double sx = cos(deg) * 2 * SCAN_RADIUS;
    const double sy = sin(deg) * 2 * SCAN_RADIUS;

    double tx = mex;
    double ty = mey;

    const Room& room = fx.map.room[fx.player[me].roomx][fx.player[me].roomy];

    while (1) {
        if (room.fall_on_wall((int)tx, (int)ty, SCAN_RADIUS))
            break;
        tx += sx;
        ty += sy;

        if (tx > S_W || tx < 0)
            break;
        if (ty > S_H || ty < 0)
            break;
    }
    return sqrt((tx - mex) * (tx - mex) + (ty - mey) * (ty - mey));
}

int Client::IsAimed(double mex, double mey, int i) const { // return 1 if in hit point
    // XXX?
    const double ttx = fx.player[i].lx + averageLag * fx.player[i].sx;
    const double tty = fx.player[i].ly + averageLag * fx.player[i].sy;

    double dx = ttx - mex;
    double dy = tty - mey;

    const double dist = sqrt(dx * dx + dy * dy);

    if (dist <= PLAYER_RADIUS)
        return 2;

    const double tm = dist / fx.physics.rocket_speed;
    dx += tm * fx.player[i].sx;
    dy += tm * fx.player[i].sy;

    const int dir = GetDir(dx, dy);

    if (fx.player[me].gundir != dir)
        return 0;

    if (IsBehindWall(mex, mey, dx, dy))
        return 0;

    if (dir == 0 || dir == 4) // left or right
        return fabs(dy) < 1.3 * PLAYER_RADIUS ? 2 : 1;
    if (dir == 2 || dir == 6) // up or down
        return fabs(dx) < 1.3 * PLAYER_RADIUS ? 2 : 1;

    const double a = sqrt(2 * PLAYER_RADIUS * PLAYER_RADIUS); // diagonal?

    if (fabs(dy) <= fabs(dx) + 1.3 * a && fabs(dy) >= fabs(dx) - 1.3 * a)
        return 2;
    else
        return 1;
}

int Client::GetDir(double dx, double dy) const {
    if (dx == 0 && dy == 0)
        return 0;

    double ntg;
    if (fabs(dx) > fabs(dy))
        ntg = fabs(dy) / fabs(dx);
    else
        ntg = fabs(dx) / fabs(dy);

    if (ntg < 0.41) { // h or v
        if (dx >= 0 && fabs(dy) < fabs(dx))
            return 0;
        else if (dy >= 0 && fabs(dy) > fabs(dx))
            return 2;
        else if (dx <= 0 && fabs(dx) > fabs(dy))
            return 4;
        else
            return 6;
    }
    else { // diagonals
        if (dx > 0 && dy > 0)
            return 1;
        else if (dx < 0 && dy > 0)
            return 3;
        else if (dx < 0 && dy < 0)
            return 5;
        else
            return 7;
    }
}

int Client::GetDangerousRocket(double mex, double mey) const {
    int mrock = -1;
    double rdist = 0;

    for (int i = 0; i < MAX_ROCKETS; ++i) {
        const Rocket& rocket = fx.rock[i];
        const ClientPlayer& player = fx.player[me];

        if (rocket.owner == -1 || rocket.team == player.team() ||
            rocket.px != player.roomx || rocket.py != player.roomy)
                continue;

        const double rdx = rocket.x - mex;
        const double rdy = rocket.y - mey;

        const int d = rocket.direction;

        if (IsBehindWall(mex, mey, rdx, rdy))
            continue;

        if ((d == 0 || d == 1 || d == 7) && rdx >  4 * PLAYER_RADIUS)
            continue;
        if ((d == 3 || d == 4 || d == 5) && rdx < -4 * PLAYER_RADIUS)
            continue;
        if ((d == 1 || d == 2 || d == 3) && rdy >  4 * PLAYER_RADIUS)
            continue;
        if ((d == 5 || d == 6 || d == 7) && rdy < -4 * PLAYER_RADIUS)
            continue;

        double dist1 = sqrt(rdx * rdx + rdy * rdy);
        const double dist2 = (dist1 / fx.physics.rocket_speed) * fx.physics.max_run_speed / 2;

        if (d == 0 || d == 4)
            dist1 = fabs(rdy);
        else if (d == 2 || d == 6)
            dist1 = fabs(rdx);
        else if (d == 1 || d == 5)
            dist1 = sqrt((rdy - rdx) * (rdy - rdx) / 2);
        else
            dist1 = sqrt((rdy + rdx) * (rdy + rdx) / 2);

        if (dist1 < 6 * PLAYER_RADIUS && (dist1 + dist2 < rdist || rdist == 0)) { //was 4
            mrock = i;
            rdist = dist1 + dist2;
        }
    }
    return mrock;
}

int Client::GetEasyEnemy(double mex, double mey) const {
    int snap = -1;
    double rdist = 0;

    for (int i = 0; i < maxplayers; ++i) {
        const ClientPlayer& enemy = fx.player[i];
        const ClientPlayer& player = fx.player[me];
        if (!enemy.used || enemy.roomx != player.roomx || enemy.roomy != player.roomy ||
            enemy.team() == player.team() || !enemy.onscreen || enemy.dead)
               continue;

        const double ttx = enemy.lx + averageLag * enemy.sx;
        const double tty = enemy.ly + averageLag * enemy.sy;

        const double dx = ttx - mex;
        const double dy = tty - mey;

        const int d = GetDir(dx, dy);

        double dist1 = sqrt(dx * dx + dy * dy);
        const double dist2 = (dist1 / fx.physics.rocket_speed) * fx.physics.max_run_speed / 2;

        if (d == 0 || d == 4)
            dist1 = fabs(dy);
        else if (d == 2 || d == 6)
            dist1 = fabs(dx);
        else if (d == 1 || d == 5)
            dist1 = sqrt((dy - dx) * (dy - dx) / 2);
        else
            dist1 = sqrt((dy + dx) * (dy + dx) / 2);

        if (dist1 < 2 * PLAYER_RADIUS && (dist1 + dist2 < rdist || rdist == 0)) { //was 4
            if (IsBehindWall(mex, mey, ttx - mex, tty - mey))
                continue;
            snap = i;
            rdist = dist1 + dist2;
        }
    }
#ifdef BOTDEBUG
    if (snap != -1)
        fprintf(stderr,"Looking on easy. enemy %d\n", snap);
#endif
    return snap;
}

int Client::GetDangerousEnemy(double mex, double mey) const {
    int snap = -1;
    double rdist = 0;

    for (int i = 0; i < maxplayers; ++i) {
        const ClientPlayer& enemy = fx.player[i];
        const ClientPlayer& player = fx.player[me];
        if (!enemy.used || enemy.roomx != player.roomx || enemy.roomy != player.roomy ||
            enemy.team() == player.team() || !enemy.onscreen || enemy.dead)
               continue;

        const double ttx = enemy.lx + averageLag * enemy.sx;
        const double tty = enemy.ly + averageLag * enemy.sy;

        const double dx = ttx - mex;
        const double dy = tty - mey;

        const int d = enemy.gundir;
        double dist1 = sqrt(dx * dx + dy * dy);

        const int dd = inv_dir(GetDir(dx, dy));

        if (d != dd && (dist1 > 2*PLAYER_RADIUS) )
            continue;

        const double dist2 = (dist1 / fx.physics.rocket_speed) * fx.physics.max_run_speed/2;

        if (d == 0 || d == 4)
            dist1 = fabs(dy);
        else if (d == 2 || d == 6)
            dist1 = fabs(dx);
        else if (d == 1 || d == 5)
            dist1 = sqrt((dy - dx) * (dy - dx) / 2);
        else
            dist1 = sqrt((dy + dx) * (dy + dx) / 2);

        if ((dist1 < 2 * PLAYER_RADIUS) && (dist1 + dist2 < rdist || rdist == 0)) { //was 4
            if (IsBehindWall(mex, mey, ttx - mex, tty - mey))
                continue;
            snap = i;
            rdist = dist1 + dist2;
        }
    }
#ifdef BOTDEBUG
    if (snap != -1)
        fprintf(stderr,"Looking on dang. enemy %d\n", snap);
#endif
    return snap;
}

int Client::GetNearestEnemy(double mex, double mey) const {
    int snap = -1;
    double mdist = 0;

    for (int i = 0; i < maxplayers; ++i) {
        const ClientPlayer& enemy = fx.player[i];
        const ClientPlayer& player = fx.player[me];
        if (!enemy.used || enemy.roomx != player.roomx || enemy.roomy != player.roomy ||
            enemy.team() == player.team() || !enemy.onscreen || enemy.dead)
                continue;

        // XXX?
        const double ttx = enemy.lx + averageLag * enemy.sx;
        const double tty = enemy.ly + averageLag * enemy.sy;

        const double dx = ttx - mex;
        const double dy = tty - mey;

        const double dist = sqrt(dx * dx + dy * dy);

        if (dist < mdist || mdist == 0) {
            //if (IsBehindWall(mex, mey, dx, dy)) // XXX ?
            //    continue;
            mdist = dist;
            snap = i;
        }
    }
    return snap;
}

bool Client::NeedShoot(double mex, double mey) const {
    for (int i = 0; i < maxplayers; ++i) {  // find nearest
        const ClientPlayer& enemy = fx.player[i];
        const ClientPlayer& player = fx.player[me];
        if (!enemy.used || enemy.roomx != player.roomx || enemy.roomy != player.roomy ||
            enemy.team() == player.team() || !enemy.onscreen || enemy.dead)
               continue;

        if (IsAimed(mex, mey, i) == 2)
            return true;
    }
    return false;
}

ClientControls Client::EscapeRocket(double mex, double mey, int mrock) const {
    const double sdx = fx.rock[mrock].x - mex;
    const double sdy = fx.rock[mrock].y - mey;

    ClientControls ctrl;
    ctrl.setStrafe();
    ctrl.setRun();

    switch (fx.rock[mrock].direction) {
        case 0: // r -> d or up
        case 4: // l -> u or d
            if (sdy > 0)
                ctrl.setUp();
            else
                ctrl.setDown();
            break;
        case 2: // d - > l or r
        case 6: // u -> l or r
            if (sdx > 0)
                ctrl.setLeft();
            else
                ctrl.setRight();
            break;
        case 1: // rd -> ru | ld  "\"
        case 5: // lu -> ru | ld
            if (sdy > sdx) {
                ctrl.setUp();
                ctrl.setRight();
            }
            else {
                ctrl.setDown();
                ctrl.setLeft();
            }
            break;
        case 3: // ld -> rd | lu "/"
        case 7: // ur -> lu | rd
            if (sdy > -sdx) {
                ctrl.setUp();
                ctrl.setLeft();
            }
            else {
                ctrl.setDown();
                ctrl.setRight();
            }
            break;
    }
    return ctrl;
}

ClientControls Client::Aim(double mex, double mey, int i) const {
    ClientControls ctrl;
    ctrl.setRun(); //always run!

    const double ttx = fx.player[i].lx + averageLag * fx.player[i].sx;
    const double tty = fx.player[i].ly + averageLag * fx.player[i].sy;

    const double dx = ttx - mex;
    const double dy = tty - mey;

    const int dir = IsAimed(mex, mey, i);

    if (dir == 1) { // almost aimed
        ctrl.setStrafe();
        if (fx.player[me].gundir == 0 ||
            fx.player[me].gundir == 2 ||
            fx.player[me].gundir == 4 ||
            fx.player[me].gundir == 6)
        {
            if (dx > 0)
                ctrl.setRight();
            else if (dx < 0)
                ctrl.setLeft();
            if (dy > 0)
                ctrl.setDown();
            else if (dy < 0)
                ctrl.setUp();
        }
        else if (fabs(dy) > fabs(dx)) {
            if (dy < 0)
                ctrl.setUp();
            else
                ctrl.setDown();
        }
        else {
            if (dx < 0)
                ctrl.setLeft();
            else
                ctrl.setRight();
        }
        return ctrl;
    }
    // aimed or nor, go to mdir

    if (dir == 2)
        return ctrl;

    return MoveTo(mex, mey, dx, dy);
}


int Client::FreeDir(double mex, double mey) const {
    const int dir = fx.player[me].gundir;
    int mdir = 0;
    double mdist = 0;

    for (int i = dir - 1; i != dir + 2; ++i) {
        int d;
        if (i > 7)
            d = i - 8;
        else if (i < 0)
            d = 8 + i;
        else
            d = i;
        const double dist = ScanDir(mex, mey, d);

        if (dist > mdist || mdist == 0 || dist == mdist && i == dir) {
            mdist = dist;
            mdir = d;
        }
    }
    if (mdist < 1.5 * PLAYER_RADIUS)
        mdir = inv_dir(mdir);
    return mdir;
}

ClientControls Client::MoveDirNoAggregate(int dir) const {
    double dx, dy;
    double sdx = 0;
    double sdy = 0;

    int n = 0;

    for (int i = 0; i < maxplayers; ++i) {
        if (!fx.player[i].used ||
            fx.player[i].team() != fx.player[me].team() ||
            fx.player[i].dead || !fx.player[i].onscreen || (i == me))
                continue;

        if (fx.player[i].roomx != fx.player[me].roomx ||
            fx.player[i].roomy != fx.player[me].roomy)
                continue;
        dx = fx.player[i].lx - fx.player[me].lx;
        dy = fx.player[i].ly - fx.player[me].ly;
        if (fabs(dx) > PLAYER_RADIUS || fabs(dy) > PLAYER_RADIUS)
            continue;
        sdx += dx;
        sdy += dy;
        n++;
    }

    if (!n)
        return MoveDir(dir);

    ClientControls ctrl;

    if (!sdx && !sdy)
        dir = rand()%8;

    sdx /= n;
    sdy /= n;

    ctrl.setRun();
    ctrl.setStrafe();

    switch (dir) {
        case 0:
        case 4:
            if (sdy > 0)
                ctrl.setUp();
            else
                ctrl.setDown();
            break;
        case 2:
        case 6:
            if (sdx > 0)
                ctrl.setLeft();
            else
                ctrl.setRight();
            break;
        case 1:
            if (sdy < sdx)
                ctrl.setDown();
            else
                ctrl.setRight();
            break;
        case 5:
            if (sdy < sdx)
                ctrl.setLeft();
            else
                ctrl.setUp();
            break;
        case 3:
            if (sdy > -sdx)
                ctrl.setLeft();
            else
                ctrl.setDown();
            break;
        case 7:
            if (sdy > -sdx)
                ctrl.setUp();
            else
                ctrl.setRight();
            break;
    }
    return ctrl;
}

ClientControls Client::MoveDir(int dir) const {
    ClientControls ctrl;
    ctrl.setRun();
    switch(dir) {
        case 0:
            ctrl.setRight();
            break;
        case 1:
            ctrl.setRight();
            ctrl.setDown();
            break;
        case 2:
            ctrl.setDown();
            break;
        case 3:
            ctrl.setDown();
            ctrl.setLeft();
            break;
        case 4:
            ctrl.setLeft();
            break;
        case 5:
            ctrl.setLeft();
            ctrl.setUp();
            break;
        case 6:
            ctrl.setUp();
            break;
        case 7:
            ctrl.setUp();
            ctrl.setRight();
            break;
    }
    return ctrl;
}

ClientControls Client::FreeWalk(double mex, double mey) const {
    return MoveDirNoAggregate(FreeDir(mex, mey));
}

ClientControls Client::MoveToNoAggregate(double mex, double mey, double dx, double dy) const {
    int mdir;
    if (IsBehindWall(mex, mey, dx, dy)) {//walking
        mdir = FreeDir(mex, mey);
        return MoveDir(mdir);
    }
    else
        mdir = GetDir(dx, dy);
    return MoveDirNoAggregate(mdir);
}


ClientControls Client::MoveTo(double mex, double mey, double dx, double dy) const {
    int mdir;
    if (IsBehindWall(mex, mey, dx, dy))//walking
        mdir = FreeDir(mex, mey);
    else
        mdir = GetDir(dx, dy);
    return MoveDir(mdir);
}

ClientControls Client::GetPowerup(double mex, double mey) const {
    for (int i = 0; i < MAX_PICKUPS; ++i) {
        if (fx.item[i].kind == Powerup::pup_unused ||
            fx.item[i].kind == Powerup::pup_respawning ||
            fx.item[i].px != fx.player[me].roomx ||
            fx.item[i].py != fx.player[me].roomy)
                continue;
        return MoveTo(mex, mey, fx.item[i].x - mex, fx.item[i].y - mey);
    }
    return ClientControls();
}

ClientControls Client::GetFlag(double mex, double mey) const {
    const bool carry = HaveFlag(me);

    int t = fx.player[me].team();

    const vector<WorldCoords>& tflags = fx.map.tinfo[t].flags;

    bool at_base = false;

    for (vector<Flag>::const_iterator fi = fx.teams[t].flags().begin(); fi != fx.teams[t].flags().end(); ++fi) {
        if (fi->position().px != fx.player[me].roomx || fi->position().py != fx.player[me].roomy)
            continue;
        // found base of this flag

        for (vector<WorldCoords>::const_iterator pi = tflags.begin(); pi != tflags.end(); ++pi) {
            if (fx.player[me].roomx != pi->px || fx.player[me].roomy != pi->py)
                continue;
            if (fi->position().x != pi->x || fi->position().y != pi->y)
                continue;
            at_base = true;
        }

        if (!fi->carried() && (carry || !at_base)) { // my flag and i am carrying or no at base
            const double dx = fi->position().x - mex;
            const double dy = fi->position().y - mey;
            return MoveTo(mex, mey, dx, dy); // to my flag
        }
    }

    t = 1 - fx.player[me].team(); // enemy team

    for (vector<Flag>::const_iterator fi = fx.teams[t].flags().begin(); fi != fx.teams[t].flags().end(); ++fi) {
        if (fi->position().px != fx.player[me].roomx ||
            fi->position().py != fx.player[me].roomy)
                continue;
        if (!fi->carried() && !carry) { // not my and i am not carry
            const double dx = fi->position().x - mex;
            const double dy = fi->position().y - mey;
            return MoveTo(mex, mey, dx, dy);
        }
    }

    for (vector<Flag>::const_iterator fi = fx.wild_flags.begin(); fi != fx.wild_flags.end(); ++fi) {
        if (fi->position().px != fx.player[me].roomx ||
            fi->position().py != fx.player[me].roomy)
                continue;
        if (!fi->carried() && !carry) {
            const double dx = fi->position().x - mex;
            const double dy = fi->position().y - mey;
            return MoveTo(mex, mey, dx, dy);
        }
    }

    return ClientControls();
}

int Client::Teams(int x, int y, int &en, int &fr) const {
    int me_nr = -1;
    for (int i = 0; i < maxplayers; ++i) {
        const ClientPlayer& pl = fx.player[i];
        if (!pl.used || pl.roomx != x || pl.roomy != y || pl.dead)
               continue;
        if (pl.team() == fx.player[me].team()) {
            if (i == me)
                me_nr = fr;
            fr++;
        }
    }

    for (int i = 0; i < maxplayers; ++i) {
        const ClientPlayer& pl = fx.player[i];
        if (!pl.used || pl.roomx != x || pl.roomy != y || pl.dead)
               continue;
        if (pl.team() != fx.player[me].team()) {
            if (fx.frame - pl.posUpdated > FADEOUT)
                continue;
            if (fr && fx.frame - pl.posUpdated > 5)
                continue;
            en++;
        }
    }
    return me_nr;
}

ClientControls Client::Escape(double mex, double mey) const {
    int i = 0;
    int roomx = fx.player[me].roomx;
    int roomy = fx.player[me].roomy;

    if (!HaveFlag(me))
        return ClientControls();
    int enemies =  0;
    int friends = 0;

    Teams(roomx, roomy, enemies, friends);

    if (enemies <= friends)
        return ClientControls();
    // looking for friends

    for (i = 0; i < 4; ++i) {
        int x = roomx;
        int y = roomy;
        if (!fx.map.room[x][y].pass[i])
            continue;
        next_room(x, y, i);
        Teams(x, y, enemies, friends);
        if (friends + 1 > enemies && friends)
            break;
    }
    if (i == 4)
        return ClientControls();
    return MoveToDoor(mex, mey, i);
}

ClientControls Client::FollowFlag(double mex, double mey) const {
    double dx = 0;
    double dy = 0;
    double sx = 0;
    double sy = 0;
    int num = 0;
    for (int i = 0; i < maxplayers; ++i) {
        const ClientPlayer& pl = fx.player[i];
        if (!pl.used || pl.roomx != fx.player[me].roomx || pl.roomy != fx.player[me].roomy ||
            pl.team() != fx.player[me].team() || !pl.onscreen || pl.dead || i == me || !HaveFlag(i))
               continue;

        dx += pl.lx;
        dy += pl.ly;
        sx += pl.sx;
        sy += pl.sy;
        num++;
    }
    if (!num || (!sx && !sy) || IsHome(fx.player[me].roomx, fx.player[me].roomy))
        return ClientControls();
    dx = dx/num - mex;
    dy = dy/num - mey;
    sx = sx/num;
    sy = sy/num;
    double dist = (S_H + S_W) / 4;
    const double tm = dist / fx.physics.max_run_speed;
    dx += sx * tm;
    dy += sy * tm;
    dist = sqrt(dx * dx + dy * dy);
    if (dist < 3 * PLAYER_RADIUS)
        return ClientControls();
    return MoveToNoAggregate(mex, mey, dx, dy);
}

void Client::BuildMap() {
    last_seen = -1;

    for (int x = 0; x < fx.map.w; ++x)
        for (int y = 0; y < fx.map.h; ++y) {
            Room& room = fx.map.room[x][y];
            room.pass[0] =
                !room.fall_on_wall(S_W / 2, 0, PLAYER_RADIUS) ||
                !room.fall_on_wall(S_W / 4, 0, PLAYER_RADIUS) ||
                !room.fall_on_wall(S_W - S_W / 4, 0, PLAYER_RADIUS);

            room.pass[1] =
                !room.fall_on_wall(S_W / 2, S_H, PLAYER_RADIUS) ||
                !room.fall_on_wall(S_W / 4, S_H, PLAYER_RADIUS) ||
                !room.fall_on_wall(S_W - S_W / 4, S_H, PLAYER_RADIUS);

            room.pass[2] =
                !room.fall_on_wall(0, S_H / 2, PLAYER_RADIUS) ||
                !room.fall_on_wall(0, S_H / 4, PLAYER_RADIUS) ||
                !room.fall_on_wall(0, S_H - S_H / 4, PLAYER_RADIUS);

            room.pass[3] =
                !room.fall_on_wall(S_W, S_H / 2, PLAYER_RADIUS) ||
                !room.fall_on_wall(S_W, S_H / 4, PLAYER_RADIUS) ||
                !room.fall_on_wall(S_W, S_H - S_H / 4, PLAYER_RADIUS);
            for (int i = 0; i < Table_Max; i++) {
                room.route[i] = false;
                room.label[i] = -1;
            }
            room.visited_frame = 0;
#ifdef BOTDEBUG
            fprintf(stderr,"%d %d: %d %d %d %d\n", x, y,
                    room.pass[0], room.pass[1], room.pass[2], room.pass[3]);
#endif

        }

        for (int i = 0; i < Table_Max; i++) {
            route_x[i] = route_y[i] = -1;
            routing[i] = Route_None;
        }
}

void Client::next_room(int& x, int& y, int i) const {
    switch (i) {
        case 0:
            --y;
            if (y < 0)
                y = fx.map.h - 1;
            break;
        case 1:
            ++y;
            if (y >= fx.map.h)
                y = 0;
            break;
        case 2:
            --x;
            if (x < 0)
                x = fx.map.w - 1;
            break;
        case 3:
            ++x;
            if (x >= fx.map.w)
                x = 0;
            break;
    }
}

int Client::label_room(int x, int y, int label, RouteTable num) {
    if (fx.map.room[x][y].label[num] != label) // not our label
        return 0;

    int n = 0;

    for (int i = 0; i < 4; ++i) {
        if (!fx.map.room[x][y].pass[i]) // looking for nearest room
            continue;
        int nx = x;
        int ny = y;
        next_room(nx, ny, i);
        if (fx.map.room[nx][ny].label[num] != -1) // already labeled
            continue;
#ifdef BOTDEBUG
        fprintf(stderr,"label_room(%d, %d, %d) -> %d %d\n", x, y, label, nx, ny);
#endif
        fx.map.room[nx][ny].label[num] = label + 1;
        ++n;
    }
    return n;
}

int Client::route_room(int& x, int& y, RouteTable num) {
    int label = fx.map.room[x][y].label[num];
    if (label == -1) // not labeled
        return 0;

    int routes[4];
    int route_nr = 0;

    for (int i = 0; i < 4 && label != 0; i++) {
        int nx = x;
        int ny = y;
        next_room(nx, ny, i);
        if (fx.map.room[nx][ny].label[num] != label - 1)
            continue;
        if (!fx.map.room[nx][ny].pass[i ^ 1])
            continue;
        routes[route_nr++] = i;
    }
    if (route_nr == 0)
        return 0;
    next_room(x, y, routes[rand() % route_nr]);
    fx.map.room[x][y].route[num] = true;
    return route_nr;
}

int Client::BuildRouteTable(int mex, int mey, RouteTable num) {
//    const int mex = fx.player[me].roomx;
//    const int mey = fx.player[me].roomy;
    const int w = fx.map.w;
    const int h = fx.map.h;

    if (!fx.map.room[mex][mey].label[num])
        return 0;

    for (int x = 0; x < w; ++x)
        for (int y = 0; y < h; ++y) {
            fx.map.room[x][y].label[num] = -1;
            fx.map.room[x][y].route[num] = false;
        }

    int label = 0;
    fx.map.room[mex][mey].label[num] = label; // start point
    while (1) {
        int i = 0;
        for (int x = 0; x < w; ++x)
            for (int y = 0; y < h; ++y)
                i += label_room(x, y, label, num);
        if (i == 0) // all alabeled
            break;
        ++label;
    }
#ifdef BOTDEBUG
    fprintf(stderr,"BuildRoute table from %d %d\n", mex, mey);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x)
            fprintf(stderr,"%02d ", fx.map.room[x][y].label[num]);
        fprintf(stderr,"\n");
    }
#endif
    return label;
}

int Client::BuildRoute(int tox, int toy, RouteTable num) {
    #ifdef BOTDEBUG
    static int tox_old = -1, toy_old = -1;
    if (tox != tox_old || toy != toy_old) {
        fprintf(stderr,"Buid route %d to %d %d\n", me, tox, toy);
        tox_old = tox;
        toy_old = toy;
    }
    #endif
    nAssert(tox >= 0 && tox < fx.map.w);
    nAssert(toy >= 0 && toy < fx.map.h);
    nAssert(me >= 0 && me < maxplayers);

    const int label = fx.map.room[tox][toy].label[num];
    const int mex = fx.player[me].roomx;
    const int mey = fx.player[me].roomy;

    if (fx.map.room[mex][mey].route[num] && tox == route_x[num] && toy == route_y[num])
        return label;

    for (int x = 0; x < fx.map.w; ++x) // clear route
        for (int y = 0; y < fx.map.h; ++y)
            fx.map.room[x][y].route[num] = false;

    if (label == -1 || fx.map.room[mex][mey].label[num] == -1 ||
        fx.map.room[mex][mey].label[num] > fx.map.room[tox][toy].label[num])
            nAssert(0);

    int i = 0;

    route_x[num] = tox;
    route_y[num] = toy;

    while (route_room(tox, toy, num))
        ++i;
#ifdef BOTDEBUG
    fprintf(stderr,"BuildRoute  to %d %d\n", tox, toy);
    for (int y = 0; y < fx.map.h; ++y) {
        for (int x = 0; x < fx.map.w; ++x)
            fprintf(stderr,"%02d ", fx.map.room[x][y].route[num]);
        fprintf(stderr,"\n");
    }
#endif
    return i;
}

ClientControls Client::DoRoute(double melx, double mely, RouteTable num) const {
    int dir = -1;
    int enemies =0;
    int friends =0;
    const int mex = fx.player[me].roomx;
    const int mey = fx.player[me].roomy;

    if (routing[num] == Route_None)
        return ClientControls();

    const int label = fx.map.room[mex][mey].label[num];

    if (label == -1)
        return ClientControls();

    if (route_x[num] == mex && route_y[num] == mey)
        return ClientControls();

    for (int i = 0; i < 4; ++i) {
        if (!fx.map.room[mex][mey].pass[i])
            continue;
        int x = mex;
        int y = mey;
        next_room(x, y, i);
        if (fx.map.room[x][y].route[num] && fx.map.room[x][y].label[num] == label + 1) {
            if (HaveFlag(me)) {
                Teams(x, y, enemies, friends);
                if (enemies > friends + 1)
                    continue;
            }
            dir = i;
            break;
        }
    }

    if (dir == -1)
        return ClientControls(); // no need to go

    #ifdef BOTDEBUG
    fprintf(stderr,"i am @ (%d %d) -> (%d %d)\n", fx.player[me].roomx, fx.player[me].roomy, mex, mey);
    #endif

    return MoveToDoor(melx, mely, dir);
}

ClientControls Client::MoveToDoor(double mex, double mey, int dir) const {
    double tox = 0;
    double toy = 0;
    switch (dir) {
    /*break;*/ case 0:
            tox = S_W / 2;
            toy = 0 - PLAYER_RADIUS;
        break; case 1:
            tox = S_W / 2;
            toy = S_H + PLAYER_RADIUS;
        break; case 2:
            tox = 0 - PLAYER_RADIUS;
            toy = S_H / 2;
        break; case 3:
            tox = S_W + PLAYER_RADIUS;
            toy = S_H / 2;
    }
    return MoveToNoAggregate(mex, mey, tox - mex, toy - mey);
}
int Client::GetPlayers(int team) const {
    int npl = 0;
    for (int i = 0; i < maxplayers; ++i) {
        const ClientPlayer& player = fx.player[i];
        if (!player.used || player.team() != team)
            continue;
        npl++;
    }
    return npl;
}

bool Client::IsDefender() {
    // get flag base
    const int team = fx.player[me].team();
    const vector<WorldCoords>& tflags = fx.map.tinfo[team].flags;

    const int npl = GetPlayers(fx.player[me].team());
    const int defNum = npl / 2;

    // for all bases
    for (vector<WorldCoords>::const_iterator pi = tflags.begin(); pi != tflags.end(); ++pi) {
        BuildRouteTable(pi->px, pi->py, Table_Def);
        const int m_label = fx.map.room[fx.player[me].roomx][fx.player[me].roomy].label[Table_Def];
        int nearNum = 0;
        for (int i = 0; i < maxplayers; ++i) {
            const ClientPlayer& player = fx.player[i];
            if (!player.used || player.team() != fx.player[me].team() ||
                 player.dead || i == me) {
                 continue;
            }
            const int label = fx.map.room[player.roomx][player.roomy].label[Table_Def];
            if (label <= m_label || HaveFlag(i))
                nearNum++;
        }
        if (nearNum < defNum)
            return true;
    }
    return false;
}

bool Client::RouteLogic(RouteTable num) { // NEED rewrite
    const bool flag = HaveFlag(me);
    routing[num] = Route_None;

    if (!flag) {
        const bool at_bases = IsFlagsAtBases(fx.player[me].team());

        int efc = !IsCarriersDef(1 - fx.player[me].team());
        int wfc = !IsCarriersDef(2);
        int mfb = IsDefender();
        if (at_bases && !efc && !wfc && !mfb) { // all flags are safe and nothing to support
            TargetRoute(1, 1, 0,
                        0, 0, 0,
                        1, 1, 0,
                        0, 0,
                        0, 0, 0,
                        num);
            if (routing[num] == Route_None) { // we are carry all possible flags
                mfb = efc = wfc = 1; // support ANYthing
            }

        }
        if (routing[num] == Route_None) {
            TargetRoute(1, 1, efc,
                        mfb, 1, 1,
                        1, 1, 1, // was wfc
                        0, 0,
                        0, 0, 0,
                        num);
        }
        if (routing[num] == Route_None) {
            TargetRoute(0, 0, 0,
                        0, 0, 0,
                        0, 0, 0,
                        1, 0,
                        1, 0, 0,
                        num);  // ..., or enemy, or enemy base
        }
    }
    else { // i am flagman ;)
        if (GetPlayers(fx.player[me].team())>1) {
            TargetRoute(0, 0, 0,
                    1, 0, 0,
                    0, 0, 0,
                    0, 0,
                    0, 0, 0,
                    num); // my flag at base
        }
        else {
            TargetRoute(0, 0, 0,
                    1, 1, 0,
                    0, 0, 0,
                    0, 0,
                    0, 0, 0,
                    num); // my flag at base or dropped
        }
        if (routing[num] == Route_None) {
            TargetRoute(0, 0, 0,
                        0, 0, 0,
                        0, 0, 0,
                        0, 0,
                        0, 1, 0,
                        num);  // ok, to our base
        }
    }
    #ifdef BOTDEBUG
    //fprintf(stderr,"RouteLogic: %d\n", i);
    #endif
    return routing[num] != Route_None;
}

bool Client::IsMassive() const {
    double dx = 0, dy = 0;
    int n = 0;
    for (int i = 0; i < maxplayers; ++i) {
        if (!fx.player[i].used || fx.player[i].team() != fx.player[me].team() || fx.player[i].dead || !fx.player[i].onscreen)
            continue;

        if (fx.player[i].roomx != fx.player[me].roomx ||
            fx.player[i].roomy != fx.player[me].roomy)
                continue; //already here

        dx += fabs(fx.player[i].lx - fx.player[me].lx);
        dy += fabs(fx.player[i].ly - fx.player[me].ly);
        ++n;
    }

    if (n > 1) {
        dx = dx / n;
        dy = dy / n;
    }
    else
        return false;

    const double dist = sqrt(dx * dx + dy * dy);
    return dist <= 2 * PLAYER_RADIUS;
}

ClientControls Client::Route(double mex, double mey, RouteTable num) {
    if (routing[num] == Route_None)
        return ClientControls();
    return DoRoute(mex, mey, num);
}

bool Client::HaveFlag(int n) const {
    const int t = 1 - fx.player[n].team();
    nAssert(t == 0 || t == 1);

    // look for enemy flags in team
    for (vector<Flag>::const_iterator fi = fx.teams[t].flags().begin(); fi != fx.teams[t].flags().end(); ++fi)
        if (fi->carried() && fi->carrier() == n)
            return true;

    // looking for wild flags
    for (vector<Flag>::const_iterator fi = fx.wild_flags.begin(); fi != fx.wild_flags.end(); ++fi)
        if (fi->carried() && fi->carrier() == n)
            return true;

    return false;
}

int Client::TargetNearestBase(int& m_label, int& x, int& y, int team, RouteTable num) {
    const vector<WorldCoords>& tflags = fx.map.tinfo[team].flags;
    int label = 0;

    for (vector<WorldCoords>::const_iterator pi = tflags.begin(); pi != tflags.end(); ++pi) {
        label = fx.map.room[pi->px][pi->py].label[num];
        if (label == -1)
            continue;
        if (label < m_label || m_label == -1) {
            m_label = label;
            x = pi->px;
            y = pi->py;
            routing[num] = Route_Base;
        }
    }

    if (label == -1)
        return -1;
    return 0;
}

int Client::TargetNearestTeam(int& m_label, int& x, int& y, int team, RouteTable num) {
    // looking for soldiers
    const bool enemy = (fx.player[me].team() != team);

    int label = 0;
    for (int i = 0; i < maxplayers; ++i) {
        if (i == me || !fx.player[i].used || fx.player[i].team() != team || fx.player[i].dead)
            continue;

        label = fx.map.room[fx.player[i].roomx][fx.player[i].roomy].label[num];
        if (label == -1)
            continue;

        if (enemy) { // if enemy, check fadeout
            const ClientPlayer& pl = fx.player[i];
            if (!pl.used || pl.roomx < 0 || pl.roomy < 0 ||
                pl.roomx >= fx.map.w || pl.roomy >= fx.map.h ||
                fx.frame - pl.posUpdated > FADEOUT) // TODO fadeout
                    continue; // old data
            if (!pl.onscreen) {
                if (pl.roomx == fx.player[me].roomx && pl.roomy == fx.player[me].roomy)
                    continue; // already here
                if (fx.map.room[pl.roomx][pl.roomy].visited_frame > pl.posUpdated)
                    continue; // was her
            }
        }

        if (label < m_label || m_label == -1) {
            m_label = label;
            x = fx.player[i].roomx;
            y = fx.player[i].roomy;
            routing[num] = Route_Team;
        }
    }

    if (label == -1)
        return -1; // not builded
    return 0;
}

bool Client::IsCarriersDef(int team) const {
    const vector<Flag>& flags = (team != 2) ? fx.teams[team].flags() : fx.wild_flags;//.begin();
    int defenders = 0;
    int def_me = -1;
    int players = GetPlayers(fx.player[me].team());
    int n;
    int flags_nr = 0;

    for (vector<Flag>::const_iterator fi = flags.begin(); fi != flags.end(); ++fi) {
        if (!fi->carried())
            continue;
        flags_nr++;
        int enemies = 0, friends = 0;
        const ClientPlayer& pl = fx.player[fi->carrier()];
        n = Teams(pl.roomx, pl.roomy, enemies, friends);
        if (n != -1) {
            if (me < fi->carrier())
                n ++;
            else if (me == fi->carrier())
                n = 0;
            def_me = n + defenders;
        }
        defenders += friends;
    #if 0
        for (int i = 0; i < 4; i++) {
            int nx = pl.roomx;
            int ny = pl.roomy;
            next_room(nx, ny, i);
            if (!fx.map.room[nx][ny].pass[i ^ 1])
                continue;
            n = Teams(nx, ny, enemies, friends);
            if (n != -1)
                def_me = n + defenders;
            defenders += friends;
        }
    #endif
    }
    if (!flags_nr) // nothing to defend
        return true;
    if (defenders < players / 2) // not enouth  defenders
        return false;
    if (def_me == -1) // enouth and not me
        return true;
    if (def_me >= players / 2) // enouth and not me
        return true;
    return false;
}

bool Client::IsHome(int mex, int mey) const {
    const vector<WorldCoords>& tflags = fx.map.tinfo[fx.player[me].team()].flags;
    // our bases
    for (vector<WorldCoords>::const_iterator pi = tflags.begin(); pi != tflags.end(); ++pi)
        if (pi->px == mex && pi->py == mey)
            return true;
    return false;
}

bool Client::IsFlagsAtBases(int team) const {
    const vector<Flag>& flags = (team != 2) ? fx.teams[team].flags() : fx.wild_flags;//.begin();

    for (vector<Flag>::const_iterator fi = flags.begin(); fi != flags.end(); ++fi) {
        if (fi->carried())
            return false;
        const vector<WorldCoords>& tflags = fx.map.tinfo[team].flags;
        bool at_base = false;
        for (vector<WorldCoords>::const_iterator pi = tflags.begin(); pi != tflags.end(); ++pi) {
            if (pi->px != fi->position().px || pi->py != fi->position().py)
                continue;
            at_base = true;
        }
        if (!at_base)
            return false;
    }
    return true;
}

int Client::TargetNearestFlag(int& m_label, int& x, int& y, int team, int state, RouteTable num) {
    // state - 0 - at base, 1 - no at base/droped, 2 - no at base/carry
    const bool on_base = state == 0;

    const vector<WorldCoords>& tflags = fx.map.tinfo[team].flags;
    const vector<Flag>& flags = (team != 2) ? fx.teams[team].flags() : fx.wild_flags;//.begin();

    int label = 0;

    for (vector<Flag>::const_iterator fi = flags.begin(); fi != flags.end(); ++fi) {
        if (state == 2 && !fi->carried())
            continue;
        if ((state == 1 || on_base) && fi->carried()) // dropped wanted but it is carried
            continue;

        bool enemy = fx.player[me].team() != team;

        if (fi->carried() && (team == 2)) // wild flags can be enemy or friend
            enemy = (fx.player[fi->carrier()].team() == team);

        if (fi->carried() && !enemy) { // our flag carried, is there near our forces
            const ClientPlayer& pl = fx.player[fi->carrier()];
            if (!pl.used || pl.roomx < 0 || pl.roomy < 0 ||
                pl.roomx >= fx.map.w || pl.roomy >= fx.map.h ||
                fx.frame - pl.posUpdated > FADEOUT) // TODO fadeout
                    continue; // old data

            if (!pl.onscreen) {
                if (pl.roomx == fx.player[me].roomx && pl.roomy == fx.player[me].roomy)
                    continue; // already here
                if (fx.map.room[pl.roomx][pl.roomy].visited_frame > pl.posUpdated)
                    continue; // was here
            }
        }
        bool at_base = false;
        // at base or not?
        for (vector<WorldCoords>::const_iterator pi = tflags.begin(); pi != tflags.end(); ++pi) {
            if (fi->carried())
                break;

            if (pi->px != fi->position().px || pi->py != fi->position().py)
                continue;

            at_base = pi->x == fi->position().x && pi->y == fi->position().y;
            if (at_base)
                break;
        }

        if (at_base != on_base)
            continue;

        int nx, ny;
        // this flag is ok
        if (fi->carried()) {
            const ClientPlayer& pl = fx.player[fi->carrier()];
            nx = pl.roomx;
            ny = pl.roomy;
        }
        else {
            nx = fi->position().px;
            ny = fi->position().py;
        }

        label = fx.map.room[nx][ny].label[num];
        if (label == -1)
            continue;

        if (label < m_label || m_label == -1) {
            m_label = label;
            x = nx;
            y = ny;
            routing[num] = Route_Flag;
        }
    }
    if (label == -1)
        return label; // not builded

    return 0; // build route to nearest target
}

int Client::TargetRoute(int efb, int efd, int efc,
                        int mfb, int mfd, int mfc,
                        int wfb, int wfd, int wfc,
                        int en,  int fr,
                        int eb,  int fb, int wb,
                        RouteTable num) {
    int m_label = -1;
    int x = -1, y = -1;
    const int t = fx.player[me].team();
    const int et = 1 - t;
    int n = 0;
//    const Room& room = fx.map.room[fx.player[me].roomx][fx.player[me].roomy];

    BuildRouteTable(fx.player[me].roomx, fx.player[me].roomy, num);

    routing[num] = Route_None;

    if (efb)
        n += TargetNearestFlag(m_label, x, y, et, 0, num);

    if (efd)
        n += TargetNearestFlag(m_label, x, y, et, 1, num);

    if (efc)
        n += TargetNearestFlag(m_label, x, y, et, 2, num);

    if (mfb)
        n += TargetNearestFlag(m_label, x, y, t, 0, num);

    if (mfd)
        n += TargetNearestFlag(m_label, x, y, t, 1, num);

    if (mfc)
        n += TargetNearestFlag(m_label, x, y, t, 2, num);

    if (wfb)
        n += TargetNearestFlag(m_label, x, y, 2, 0, num);

    if (wfd)
        n += TargetNearestFlag(m_label, x, y, 2, 1, num);

    if (wfc)
        n += TargetNearestFlag(m_label, x, y, 2, 2, num);

    if (en)
        n += TargetNearestTeam(m_label, x, y, et, num);

    if (fr)
        n += TargetNearestTeam(m_label, x, y, t, num);

    if (eb)
        n += TargetNearestBase(m_label, x, y, et, num);
    if (fb)
        n += TargetNearestBase(m_label, x, y, t, num);
    if (wb)
        n += TargetNearestBase(m_label, x, y, 2, num);

    if (n < 0)
        return -1;

    if (routing[num] == Route_None) // nothing todo
        return 0;

    nAssert(x != -1 && y != -1);

    return BuildRoute(x, y, num);
}

bool Client::IsMission(RouteTable num) const {
    const int to_home = IsHome(route_x[num], route_y[num]);
// if we are looking for flag or going to our base for something
    if (fx.player[me].roomx == route_x[num] && fx.player[me].roomy == route_y[num])
        return false;
    if (HaveFlag(me) || routing[num] == Route_Flag || to_home ||
        (!to_home && routing[num] == Route_Base))
            return true;
    return false;
}

ClientControls Client::Robot() {
    const bool hide_map = !map_ready || gameover_plaque != NEXTMAP_NONE || fx.skipped || me < 0 || me >= maxplayers;

    if (hide_map)
        return ClientControls();

    if (!fx.player[me].used || fx.player[me].dead || fx.player[me].team() != 0 && fx.player[me].team() != 1)
        return ClientControls();

    const double mex = fx.player[me].lx + averageLag * fx.player[me].sx;
    const double mey = fx.player[me].ly + averageLag * fx.player[me].sy;

    fx.map.room[fx.player[me].roomx][fx.player[me].roomy].visited_frame = fx.frame;

    if (NeedShoot(mex, mey)) {
        if (!botPrevFire) { //if not fired
            char lebuf[16]; int count = 0;
            writeByte(lebuf, count, data_fire_on);
            client->send_message(lebuf, count);
            botPrevFire = true;
        }
    }
    else if (botPrevFire) {
        char lebuf[16]; int count = 0;
        writeByte(lebuf, count, data_fire_off);
        client->send_message(lebuf, count);
        botPrevFire = false;
    }

    int i = last_seen; // lost target
    if (i == -1 || !fx.player[i].used ||
        fx.player[i].roomx != fx.player[me].roomx ||
        fx.player[i].roomy != fx.player[me].roomy ||
        fx.player[i].team() == fx.player[me].team() ||
        !fx.player[i].onscreen || fx.player[i].dead)
            last_seen = -1;

    if (!IsMassive())
        i = GetDangerousRocket(mex, mey);
    else
        i = -1;
    if (i != -1) {
        if (last_seen == -1)
            last_seen = GetDangerousEnemy(mex, mey);
        return EscapeRocket(mex, mey, i);
    }

    if (last_seen == -1) {
        ClientControls ctrl = GetFlag(mex, mey);
        if (!ctrl.idle()) // if any
            return ctrl;
    }

    RouteLogic(Table_Main);

    if (IsMission(Table_Main)) // get only dangerous enemy
        last_seen = GetDangerousEnemy(mex, mey);
    else if (last_seen != -1) { // already locked on someone
        i = GetEasyEnemy(mex, mey); // more easy???
        if (i != -1)
            last_seen = i;
    }
    else // get someone
        last_seen = GetNearestEnemy(mex, mey);

    i = last_seen;

    if (i != -1)
        return Aim(mex, mey, i);

    // ok, free tour ;)
    ClientControls ctrl = GetPowerup(mex, mey);
    if (!ctrl.idle())
        return ctrl;
    ctrl = Escape(mex, mey);
    if (!ctrl.idle())
        return ctrl;
    ctrl = Route(mex, mey, Table_Main);
    if (!ctrl.idle())
        return ctrl;
    ctrl = FollowFlag(mex, mey);
    if (!ctrl.idle())
        return ctrl;
    ctrl = FreeWalk(mex, mey);
    ctrl.clearRun();
    return ctrl;
}
