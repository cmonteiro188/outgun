/*
 *  robot.cpp
 *
 *  Copyright (C) 2006, 2008 - Peter Kosyh
 *  Copyright (C) 2006, 2008, 2009, 2010 - Niko Ritari
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

//#define BOTDEBUG
//#define DEBUGSTRATEGY

#include <iomanip>
#include <queue>

#include "binaryaccess.h"
#include "leetnet/client.h"
#include "nassert.h"
#include "network.h"
#include "protocol.h"
#include "timer.h"

#include "robot.h"

#ifdef BOT_TEST_COMPARE_TWO

#ifdef ALTERNATE_BOT

typedef Robot Robot1;
#undef ROBOT_H_INC
#define Robot Robot2 // both for include, and the rest of this file
#include "robot2.h"

BotInterface* BotInterface::newBot(const ClientExternalSettings& config, Log& clientLog, MemoryLog& externalErrorLog_) throw () {
    static bool turn = false;
    turn = !turn;
    if (turn)
        return new Robot2(config, clientLog, externalErrorLog_);
    else
        return new Robot1(config, clientLog, externalErrorLog_);
}

#endif

#else

BotInterface* BotInterface::newBot(const ClientExternalSettings& config, Log& clientLog, MemoryLog& externalErrorLog_) throw () {
    return new Robot(config, clientLog, externalErrorLog_);
}

#endif

using std::list;
using std::make_pair;
using std::max;
using std::min;
using std::pair;
using std::string;
using std::queue;
using std::vector;

const int S_W = plw;
const int S_H = plh;
const int FADEOUT = 50;

inline GunDirection inv_dir(GunDirection dir) throw () { return dir.adjust(4); }
inline int inv_dir(int dir) throw () { return dir ^ 4; }

int Robot::xDelta(Area::Neighbor::Direction dir) throw () { return (dir == Area::Neighbor::Right) ? +1 : (dir == Area::Neighbor::Left) ? -1 : 0; }
int Robot::yDelta(Area::Neighbor::Direction dir) throw () { return (dir == Area::Neighbor::Down ) ? +1 : (dir == Area::Neighbor::Up  ) ? -1 : 0; }

const DeathbringerExplosion* Robot::explosionInRoom(int roomx, int roomy) const throw () {
    for (list<DeathbringerExplosion>::const_iterator dbi = fx.deathbringerExplosions().begin(); dbi != fx.deathbringerExplosions().end(); ++dbi) {
        const WorldCoords& pos = dbi->position();
        if (pos.px == roomx && pos.py == roomy && (dbi->team() != fx.player[me].team() || fx.physics.friendly_db))
            return &*dbi;
    }
    return 0;
}

bool Robot::imminentExplosionHere() const throw () {
    for (int i = 0; i < maxplayers; ++i) {
        const ClientPlayer& pl = fx.player[i];
        if (!pl.used || !pl.onscreen || pl.dead)
            continue;
        if (pl.team() == fx.player[me].team() && !fx.physics.friendly_db)
            continue;
        if (pl.item_deathbringer && pl.under_deathbringer_effect(get_time()))
            return true;
    }
    return false;
}

double Robot::distanceFromDoor(const Area::Neighbor& n, double lx, double ly) const throw () {
    try {
        const Coords door = nearestDoor(n, lx, ly);
        return sqrt(sqr(door.x - lx) + sqr(door.y - ly));
    } catch (AlreadyInRoom) {
        return 0;
    }
}

bool Robot::dangerousExplosionInNeighbor(const Area::Neighbor& neighbor, double mex, double mey) const throw () {
    const DeathbringerExplosion* const dbe = explosionInRoom(neighbor.area->roomx, neighbor.area->roomy);
    if (!dbe)
        return false;
    // see when we can be there at the earliest, don't worry if the explosion has expired then
    const double minMoveTime = distanceFromDoor(neighbor, mex, mey) / fx.physics.max_run_speed; // ignores that we're faster with turbo
    return !dbe->expired(fx.frame + averageLag + minMoveTime);
}

bool Robot::IsBehindWall(double mex, double mey, double dx, double dy, double radius, double maxDistanceFromTarget) const throw () {
    const Room& room = fx.map.room[fx.player[me].roomx][fx.player[me].roomy];
    const double dist = sqrt(sqr(dx) + sqr(dy));
    if (dist == 0)
        return false;
    const double nearEnoughFraction = (dist - maxDistanceFromTarget) / dist;
    if (nearEnoughFraction <= 0.)
        return false;
    return room.genGetTimeTillWall(mex, mey, dx, dy, radius, nearEnoughFraction).first < nearEnoughFraction;
}

double Robot::ScanDir(double mex, double mey, GunDirection dir) const throw () {
    const double deg = dir.toRad();
    const double sx = cos(deg), sy = sin(deg);
    const Room& room = fx.map.room[fx.player[me].roomx][fx.player[me].roomy];
    double maxDist = 1e99;
    if (sx != 0)
        maxDist = min(maxDist, (sx > 0 ? S_W - mex : -mex) / sx);
    if (sy != 0)
        maxDist = min(maxDist, (sy > 0 ? S_H - mey : -mey) / sy);
    return min(maxDist, room.genGetTimeTillWall(mex, mey, sx, sy, PLAYER_RADIUS, maxDist).first);
}

pair<Robot::AimLevel, int> Robot::TryAimTradTurning(double mex, double mey, int target) const throw () {
    nAssert(!fx.physics.allowFreeTurning);

    const double ttx = fx.player[target].lx + averageLag * fx.player[target].sx;
    const double tty = fx.player[target].ly + averageLag * fx.player[target].sy;

    double dx = ttx - mex;
    double dy = tty - mey;

    const double dist = sqrt(dx * dx + dy * dy);

    if (dist <= PLAYER_RADIUS)
        return make_pair(AL_Full, myGundir);

    const double tm = dist / fx.physics.rocket_speed;
    dx += tm * fx.player[target].sx;
    dy += tm * fx.player[target].sy;

    const int dir = GetDir(dx, dy).to8way();

    if (IsBehindWall(mex, mey, dx, dy, ROCKET_RADIUS, PLAYER_RADIUS + ROCKET_RADIUS))
        return make_pair(AL_None, dir);

    static const int rocketsPerWeaponLevel[9] = { 1, 2, 3, 2, 3, 2, 3, 2, 3 };
    const int w = fx.player[me].weapon;
    const int rockets = w >= 1 && w <= 9 ? rocketsPerWeaponLevel[w - 1] : 1;
    static const double treshold = 1.3 * PLAYER_RADIUS + .7 * WorldBase::shot_deltax * (rockets - 1); // both 1.3 and .7 are semi-arbitrary
    bool belowTreshold;
    if (dir == 0 || dir == 4) // left or right
        belowTreshold = fabs(dy) < treshold;
    else if (dir == 2 || dir == 6) // up or down
        belowTreshold = fabs(dx) < treshold;
    else // diagonal
        belowTreshold = fabs(fabs(dy) - fabs(dx)) <= treshold * sqrt(2);
    return make_pair(belowTreshold ? AL_Full : AL_Near, dir);
}

GunDirection Robot::GetDir(double dx, double dy) const throw () {
    return GunDirection().fromRad(atan2(dy, dx));
}

int Robot::GetDangerousRocket() const throw () {
    static const int thinkAheadFrames = 10;
    static const double safetyRoomMul = 4., framesAfterBounce = .5;

    int nearRocket = -1;
    double firstTime = 1e99;

    const ClientPlayer& player = fx.player[me];

    const Room& room = fx.map.room[player.roomx][player.roomy];

    const double bounceTime = fx.getTimeTillBounce(room, player, PLAYER_RADIUS, thinkAheadFrames).first;

    for (int i = 0; i < MAX_ROCKETS; ++i) {
        const Rocket& rocket = fx.rock[i];

        if (rocket.owner == -1 || rocket.team == player.team() && !fx.physics.friendly_fire || rocket.px != player.roomx || rocket.py != player.roomy)
            continue;

        const double collisionTime = fx.getTimeTillCollision(player, rocket, (PLAYER_RADIUS + ROCKET_RADIUS) * safetyRoomMul);
        if (collisionTime > bounceTime + framesAfterBounce || collisionTime >= firstTime)
            continue;

        const double rocketWallTime = fx.getTimeTillWall(room, rocket, thinkAheadFrames);
        if (collisionTime > rocketWallTime)
            continue;

        firstTime = collisionTime;
        nearRocket = i;
    }

    return nearRocket;
}

int Robot::GetEasyEnemy(double mex, double mey) const throw () {
    int target = -1;
    double nearestDist = 1e10;

    for (int i = 0; i < maxplayers; ++i) {
        const ClientPlayer& enemy = fx.player[i];
        const ClientPlayer& player = fx.player[me];
        if (!enemy.used || enemy.team() == player.team() || !enemy.onscreen || enemy.dead)
            continue;

        const double ttx = enemy.lx + averageLag * enemy.sx;
        const double tty = enemy.ly + averageLag * enemy.sy;

        const double dx = ttx - mex;
        const double dy = tty - mey;

        const double playerDist = sqrt(dx * dx + dy * dy);
        const double escapeDist = (playerDist / fx.physics.rocket_speed) * fx.physics.max_run_speed / 2;

        double rocketPlaneDist;
        if (fx.physics.allowFreeTurning)
            rocketPlaneDist = playerDist;
        else {
            const int d = GetDir(dx, dy).to8way();
            if (d == 0 || d == 4)
                rocketPlaneDist = fabs(dy);
            else if (d == 2 || d == 6)
                rocketPlaneDist = fabs(dx);
            else if (d == 1 || d == 5)
                rocketPlaneDist = fabs(dy - dx) / sqrt(2.);
            else
                rocketPlaneDist = fabs(dy + dx) / sqrt(2.);
        }

        if (rocketPlaneDist < 2 * PLAYER_RADIUS && rocketPlaneDist + escapeDist < nearestDist && !IsBehindWall(mex, mey, ttx - mex, tty - mey, ROCKET_RADIUS, PLAYER_RADIUS + ROCKET_RADIUS)) { //was 4
            target = i;
            nearestDist = rocketPlaneDist + escapeDist;
        }
    }
    #ifdef BOTDEBUG
    if (target != -1)
        fprintf(stderr,"Looking on easy. enemy %d\n", target);
    #endif
    return target;
}

int Robot::GetDangerousEnemy(double mex, double mey) const throw () {
    int target = -1;
    double nearestDist = 1e10;

    for (int i = 0; i < maxplayers; ++i) {
        const ClientPlayer& enemy = fx.player[i];
        const ClientPlayer& player = fx.player[me];
        if (!enemy.used || enemy.team() == player.team() || !enemy.onscreen || enemy.dead)
            continue;

        const double ttx = enemy.lx + averageLag * enemy.sx;
        const double tty = enemy.ly + averageLag * enemy.sy;

        const double dx = ttx - mex;
        const double dy = tty - mey;

        const double playerDist = sqrt(dx * dx + dy * dy);
        const GunDirection aimTowardsMe = inv_dir(GetDir(dx, dy));

        bool aimed;
        if (fx.physics.allowFreeTurning) {
            static const double tolerance = N_PI_4; // this in both directions -> angles within a 90° range are considered dangerous
            const double diff = positiveFmod(enemy.gundir.toRad() - aimTowardsMe.toRad(), 2 * N_PI);
            aimed = diff < tolerance || diff > 2 * N_PI - tolerance;
        }
        else
            aimed = enemy.gundir.to8way() == aimTowardsMe.to8way();

        if (!aimed && playerDist > 2 * PLAYER_RADIUS)
            continue;

        const double escapeDist = (playerDist / fx.physics.rocket_speed) * fx.physics.max_run_speed / 2;

        double rocketPlaneDist;
        if (fx.physics.allowFreeTurning)
            rocketPlaneDist = playerDist;
        else {
            const int d = aimTowardsMe.to8way();
            if (d == 0 || d == 4)
                rocketPlaneDist = fabs(dy);
            else if (d == 2 || d == 6)
                rocketPlaneDist = fabs(dx);
            else if (d == 1 || d == 5)
                rocketPlaneDist = fabs(dy - dx) / sqrt(2.);
            else
                rocketPlaneDist = fabs(dy + dx) / sqrt(2.);
        }

        if (rocketPlaneDist < 2 * PLAYER_RADIUS && rocketPlaneDist + escapeDist < nearestDist && !IsBehindWall(mex, mey, ttx - mex, tty - mey, ROCKET_RADIUS, PLAYER_RADIUS + ROCKET_RADIUS)) { //was 4
            target = i;
            nearestDist = rocketPlaneDist + escapeDist;
        }
    }
    #ifdef BOTDEBUG
    if (target != -1)
        fprintf(stderr,"Looking on dang. enemy %d\n", target);
    #endif
    return target;
}

int Robot::GetNearestEnemy(double mex, double mey) const throw () {
    int target = -1;
    double mdist = 0;

    const ClientPlayer& player = fx.player[me];

    for (int i = 0; i < maxplayers; ++i) {
        const ClientPlayer& enemy = fx.player[i];
        if (!enemy.used || enemy.team() == player.team() || !enemy.onscreen || enemy.dead)
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
            target = i;
        }
    }
    return target;
}

pair<bool, GunDirection> Robot::TryAimFreeTurning(double mex, double mey, int target) const throw () {
    nAssert(fx.physics.allowFreeTurning);

    const double ttx = fx.player[target].lx + averageLag * fx.player[target].sx;
    const double tty = fx.player[target].ly + averageLag * fx.player[target].sy;

    double dx = ttx - mex;
    double dy = tty - mey;

    const double dist = sqrt(dx * dx + dy * dy);

    if (dist < 1)
        return make_pair(true, gunDir);

    const double tm = dist / fx.physics.rocket_speed;
    dx += tm * fx.player[target].sx;
    dy += tm * fx.player[target].sy;

    return make_pair(!IsBehindWall(mex, mey, dx, dy, ROCKET_RADIUS, PLAYER_RADIUS + ROCKET_RADIUS), GetDir(dx, dy));
}

double Robot::GetHitTime(double mex, double mey, const GunDirection& dir, int iTarget) const throw () {
    const ClientPlayer& target = fx.player[iTarget];

    if (!target.onscreen || target.dead)
        return 1e100;

    const double ttx = target.lx + averageLag * target.sx;
    const double tty = target.ly + averageLag * target.sy;

    double dx = ttx - mex;
    double dy = tty - mey;

    const double initialDist = sqrt(sqr(dx) + sqr(dy));

    if (initialDist <= PLAYER_RADIUS)
        return 0;

    const double rsx = cos(dir.toRad()) * fx.physics.rocket_speed;
    const double rsy = sin(dir.toRad()) * fx.physics.rocket_speed;

    const double tsx = rsx - target.sx, tsy = rsy - target.sy;

    // divide D=(dx,dy) to components parallel with, and perpendicular to T=(tsx,tsy)

    // D_par = T / |T| * (T dot D) / |T| = T * (T dot D) / |T|²
    // D_perp = D - D_par

    const double ts2 = sqr(tsx) + sqr(tsy);

    if (ts2 == 0)
        return 1e100;

    const double mul = (dx * tsx + dy * tsy) / ts2;

    const double parx = tsx * mul, pary = tsy * mul;
    const double perpx = dx - parx, perpy = dy - pary;

    const double hitTime = sqrt((sqr(parx) + sqr(pary)) / ts2); // |par| / |ts|, combined under one square root op
    const double perpDist = sqrt(sqr(perpx) + sqr(perpy));

    return perpDist < 3 * PLAYER_RADIUS ? hitTime : 1e100;
}

double Robot::GetHitTeammateTime(double mex, double mey, const GunDirection& dir) const throw () {
    double hitTime = 1e100;
    if (fx.physics.friendly_fire == 0)
        return hitTime;
    for (int i = 0; i < maxplayers; ++i)
        if (fx.player[i].used && fx.player[i].team() == fx.player[me].team() && i != me)
            hitTime = min(hitTime, GetHitTime(mex, mey, dir, i));
    return hitTime;
}

pair<bool, GunDirection> Robot::NeedShootFreeTurning(double mex, double mey, const GunDirection& defaultDir) throw () {
    vector<int> tryOrder;
    for (int i = 0; i < maxplayers; ++i) {
        const ClientPlayer& pl = fx.player[i];
        if (!pl.used || pl.team() == fx.player[me].team() || !pl.onscreen || pl.dead)
            continue;
        if (i != last_seen)
            tryOrder.push_back(i);
    }

    pair<bool, GunDirection> aimLastSeen = last_seen == -1 ? make_pair(false, defaultDir) : TryAimFreeTurning(mex, mey, last_seen);
    if (aimLastSeen.first) {
        if (GetHitTime(mex, mey, aimLastSeen.second, last_seen) <= GetHitTeammateTime(mex, mey, aimLastSeen.second))
            return aimLastSeen;
        aimLastSeen.first = false;
    }
    random_shuffle(tryOrder.begin(), tryOrder.end());
    for (vector<int>::const_iterator ti = tryOrder.begin(); ti != tryOrder.end(); ++ti) {
        const pair<bool, GunDirection> aim = TryAimFreeTurning(mex, mey, *ti);
        if (aim.first && GetHitTime(mex, mey, aim.second, *ti) <= GetHitTeammateTime(mex, mey, aim.second)) {
            last_seen = *ti;
            return aim;
        }
    }
    return aimLastSeen; // aim at last_seen if no one is actually shootable
}

pair<bool, int> Robot::NeedShootTradTurning(double mex, double mey) throw () {
    vector< pair<bool, double> > dirDistances(8, make_pair(false, 0)); // if there's someone to shoot in the direction, first = true, second = time to hit
    for (int i = 0; i < maxplayers; ++i) {
        const ClientPlayer& pl = fx.player[i];
        if (!pl.used || pl.team() == fx.player[me].team() || !pl.onscreen || pl.dead)
            continue;

        const pair<AimLevel, int> aim = TryAimTradTurning(mex, mey, i);
        if (aim.first != AL_Full)
            continue;
        const double hitTime = GetHitTime(mex, mey, GunDirection().from8way(aim.second), i);
        if (hitTime > 1e10) // GetHitTime signals "no hit" with a huge time, and uses a better calculation than TryAim
            continue;
        if (!dirDistances[aim.second].first || hitTime < dirDistances[aim.second].second)
            dirDistances[aim.second] = make_pair(true, hitTime);
    }
    int bestDir = -1;
    double bestValue = 1e99;
    for (int dir = 0; dir < 8; ++dir) {
        if (!dirDistances[dir].first || dirDistances[dir].second > GetHitTeammateTime(mex, mey, GunDirection().from8way(dir)))
            continue;
        int dirDiff = abs(dir - myGundir);
        nAssert(dirDiff < 8);
        if (dirDiff > 4)
            dirDiff = 8 - dirDiff;
        const double value = dirDistances[dir].second * dirDiff; // at or near the current aim is heavily favored (at gives always the best value)
        if (value < bestValue) {
            bestValue = value;
            bestDir = dir;
        }
    }
    return make_pair(bestDir != -1, bestDir);
}

ClientControls Robot::EscapeRocket(double mex, double mey, int mrock) const throw () {
    const Rocket& rocket = fx.rock[mrock];
    const double sdx = rocket.x + averageLag * rocket.sx - mex;
    const double sdy = rocket.y + averageLag * rocket.sy - mey;

    ClientControls ctrl;
    ctrl.setStrafe();
    ctrl.setRun();

    switch (rocket.direction.to8way()) {
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

ClientControls Robot::EscapeExplosion(double mex, double mey) const throw () {
    if (!explosionInRoom(fx.player[me].roomx, fx.player[me].roomy) && !imminentExplosionHere())
        return ClientControls();

    const Area::Neighbor* bestRoom = 0;
    double shortestDistance = 1e99; // always excape to the nearest room regardless of the explosion location (if that's a bad direction, we probably couldn't escape in any)
    const Area* const a = myArea();
    for (vector<Area::Neighbor>::const_iterator ni = a->neighbors().begin(); ni != a->neighbors().end(); ++ni) {
        if (explosionInRoom(ni->area->roomx, ni->area->roomy))
            continue;
        const double dist = distanceFromDoor(*ni, mex, mey);
        if (dist < shortestDistance) {
            bestRoom = &*ni;
            shortestDistance = dist;
        }
    }
    if (bestRoom)
        return MoveToDoor(mex, mey, *bestRoom);
    else
        return ClientControls();
}

ClientControls Robot::Aim(double mex, double mey, int i) const throw () {
    nAssert(!fx.physics.allowFreeTurning);

    const double ttx = fx.player[i].lx + averageLag * fx.player[i].sx;
    const double tty = fx.player[i].ly + averageLag * fx.player[i].sy;

    const double dx = ttx - mex;
    const double dy = tty - mey;

    const pair<AimLevel, int> aim = TryAimTradTurning(mex, mey, i);
    if (aim.first == AL_Full && aim.second == myGundir)
        return ClientControls();
    else if (aim.first == AL_None || aim.second != myGundir)
        return MoveTo(mex, mey, dx, dy, PLAYER_RADIUS + PLAYER_RADIUS);
    nAssert(aim.first == AL_Near && aim.second == myGundir);

    // almost aimed
    ClientControls ctrl;
    ctrl.setRun(); //always run!
    ctrl.setStrafe();
    if (myGundir == 0 || myGundir == 2 || myGundir == 4 || myGundir == 6) {
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

int Robot::FreeDir(double mex, double mey) const throw () {
    int mdir = 0;
    double mdist = 0;

    for (int i = myGundir - 1; i <= myGundir + 1; ++i) {
        const int d = (i + 8) % 8;
        const double dist = ScanDir(mex, mey, GunDirection().from8way(d));

        if (dist > mdist || mdist == 0 || dist >= mdist - ROCKET_RADIUS && i == myGundir) {
            mdist = dist;
            mdir = d;
        }
    }
    if (mdist < 1.5 * PLAYER_RADIUS)
        mdir = inv_dir(mdir);
    return mdir;
}

ClientControls Robot::MoveDirNoAggregate(int dir) const throw () {
    double sdx = 0;
    double sdy = 0;

    int n = 0;

    for (int i = 0; i < maxplayers; ++i) {
        if (!fx.player[i].used || fx.player[i].team() != fx.player[me].team() || fx.player[i].dead || !fx.player[i].onscreen || i == me)
            continue;

        const double dx = fx.player[i].lx - fx.player[me].lx + averageLag * (fx.player[i].sx - fx.player[me].sx);
        const double dy = fx.player[i].ly - fx.player[me].ly + averageLag * (fx.player[i].sy - fx.player[me].sy);
        if (fabs(dx) > PLAYER_RADIUS || fabs(dy) > PLAYER_RADIUS)
            continue;
        sdx += dx;
        sdy += dy;
        n++;
    }

    if (!n)
        return MoveDir(dir);

    if (!sdx && !sdy)
        dir = rand() % 8;

    sdx /= n;
    sdy /= n;

    ClientControls ctrl;
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

ClientControls Robot::MoveDir(int dir) const throw () {
    return ClientControls().fromDirection(dir).setRun();
}

ClientControls Robot::FreeWalk(double mex, double mey) const throw () {
    return MoveDirNoAggregate(FreeDir(mex, mey));
}

ClientControls Robot::MoveToNoAggregate(double mex, double mey, double dx, double dy, double maxDistanceFromTarget) const throw () {
    if (IsBehindWall(mex, mey, dx, dy, PLAYER_RADIUS, maxDistanceFromTarget)) { //walking
        const int mdir = FreeDir(mex, mey);
        return MoveDir(mdir);
    }
    else {
        const int mdir = GetDir(dx, dy).to8way();
        return MoveDirNoAggregate(mdir);
    }
}


ClientControls Robot::MoveTo(double mex, double mey, double dx, double dy, double maxDistanceFromTarget) const throw () {
    int mdir;
    if (IsBehindWall(mex, mey, dx, dy, PLAYER_RADIUS, maxDistanceFromTarget))//walking
        mdir = FreeDir(mex, mey);
    else
        mdir = GetDir(dx, dy).to8way();
    return MoveDir(mdir);
}

ClientControls Robot::GetPowerup(double mex, double mey, bool onImportantMission) const throw () {
    for (int i = 0; i < MAX_POWERUPS; ++i) {
        if (!fx.item[i].real() || area(fx.item[i].position()) != myArea())
            continue;
        const double dx = fx.item[i].x - mex, dy = fx.item[i].y - mey;
        if (onImportantMission) {
            if (IsBehindWall(mex, mey, dx, dy, PLAYER_RADIUS, PLAYER_RADIUS + POWERUP_RADIUS))
                continue;
            if (dx * fx.player[me].sx + dy * fx.player[me].sy < 0 && dx * dx + dy * dy > sqr(5 * PLAYER_RADIUS)) // dot product; don't bother if the powerup is behind
                continue;
        }
        return MoveTo(mex, mey, dx, dy, PLAYER_RADIUS + POWERUP_RADIUS);
    }
    return ClientControls();
}

ClientControls Robot::GetFlag(double mex, double mey) const throw () {
    const int myTeam = fx.player[me].team();

    if (HaveFlag(me)) {
        for (int type = 0; type < 2; ++type) { // 0 for own flags, 1 for wild
            if (!(type == 0 ? capture_on_team_flags_in_effect : capture_on_wild_flags_in_effect))
                continue;
            const int team = type == 0 ? myTeam : 2;
            const vector<Flag>& flags = type == 0 ? fx.teams[team].flags() : fx.wild_flags;
            for (vector<Flag>::const_iterator fi = flags.begin(); fi != flags.end(); ++fi) {
                if (area(fi->position()) != myArea() || fi->carried())
                    continue;
                if (type == 0 || IsFlagAtBase(*fi, team)) // try to capture, or return own flag so that capture is possible; can't return wild flags
                    return MoveTo(mex, mey, fi->position().x - mex, fi->position().y - mey, PLAYER_RADIUS + FLAG_RADIUS);
            }
        }
        return ClientControls(); // can't pick up another flag
    }

    for (int type = 0; type < 3; ++type) { // 0 for own flags, 1 for enemy, 2 for wild
        if (type == 2 ? lock_wild_flags_in_effect : lock_team_flags_in_effect)
            continue;
        const int team = type == 0 ? myTeam : type == 1 ? !myTeam : 2;
        const vector<Flag>& flags = type == 2 ? fx.wild_flags : fx.teams[team].flags();
        for (vector<Flag>::const_iterator fi = flags.begin(); fi != flags.end(); ++fi) {
            if (area(fi->position()) != myArea() || fi->carried())
                continue;
            if (type != 0 || !IsFlagAtBase(*fi, team)) // try to pick up enemy or wild flag, or return own flag; nothing to do with own flags at base
                return MoveTo(mex, mey, fi->position().x - mex, fi->position().y - mey, PLAYER_RADIUS + FLAG_RADIUS);
        }
    }

    return ClientControls();
}

Robot::TeamCounts Robot::Teams(const Area* const a, bool countMe) const throw () {
    TeamCounts c;
    c.enemies = c.friends = 0;
    bool meFound = false;
    for (int i = 0; i < maxplayers; ++i) {
        const ClientPlayer& pl = fx.player[i];
        if (!pl.used || area(pl) != a || pl.dead)
            continue;
        if (pl.team() == fx.player[me].team()) {
            if (i == me)
                meFound = true;
            c.friends++;
        }
    }

    for (int i = 0; i < maxplayers; ++i) {
        const ClientPlayer& pl = fx.player[i];
        if (!pl.used || area(pl) != a || pl.dead)
            continue;
        if (pl.team() != fx.player[me].team()) {
            if (fx.frame - pl.posUpdated > FADEOUT)
                continue;
            if (fx.map.room[pl.roomx][pl.roomy].enemies_seen_frame > pl.posUpdated)
                continue;
            c.enemies++;
        }
    }

    if (meFound && !countMe)
        --c.friends;
    return c;
}

bool Robot::AmILast() const throw () {
    for (int i = 0; i < maxplayers; ++i) {
        const ClientPlayer& pl = fx.player[i];
        if (!pl.used || !pl.onscreen || pl.dead)
            continue;
        if (pl.team() == fx.player[me].team() && i > me && area(pl) == myArea())
            return false; // i am not last one
    }
    return true;
}

ClientControls Robot::Escape(double mex, double mey) const throw () {
    if (!HaveFlag(me))
        return ClientControls();

    const Area* const a = myArea();

    const TeamCounts tc = Teams(a, true);
    if (tc.enemies <= tc.friends)
        return ClientControls();

    // looking for friends
    for (vector<Area::Neighbor>::const_iterator ni = a->neighbors().begin(); ni != a->neighbors().end(); ++ni) {
        const TeamCounts tc = Teams(ni->area, false);
        if (tc.friends + 1 > tc.enemies && tc.friends > 0) {
            if (dangerousExplosionInNeighbor(*ni, mex, mey))
                return ClientControls(); // don't check other neighbors, because this is the one we'll tend to go to when the explosion is over (soon)
            else
                return MoveToDoor(mex, mey, *ni);
        }
    }
    return ClientControls();
}

ClientControls Robot::FollowFlag(double mex, double mey) const throw () {
    double dx = 0;
    double dy = 0;
    double sx = 0;
    double sy = 0;
    int num = 0;
    for (int i = 0; i < maxplayers; ++i) {
        const ClientPlayer& pl = fx.player[i];
        if (!pl.used || pl.team() != fx.player[me].team() || !pl.onscreen || pl.dead || i == me || !HaveFlag(i) || area(pl) != myArea())
            continue;

        dx += pl.lx;
        dy += pl.ly;
        sx += pl.sx;
        sy += pl.sy;
        num++;
    }
    if (!num || (!sx && !sy) || IsHome(myArea()))
        return ClientControls();
    dx = (dx + averageLag * sx) / num - mex;
    dy = (dy + averageLag * sy) / num - mey;
    const double speedMul = 4 * PLAYER_RADIUS / sqrt(sx * sx + sy * sy); // take only the direction, try to lead the carrier by a constant distance
    dx += sx * speedMul;
    dy += sy * speedMul;
    const double dist = sqrt(dx * dx + dy * dy);
    if (dist < 3 * PLAYER_RADIUS)
        return ClientControls();
    return MoveToNoAggregate(mex, mey, dx, dy, PLAYER_RADIUS);
}

void Robot::BuildMap() throw () {
    last_seen = -1;
    myGundir = -1;

    areaMap = sharedDataHandle.acquire(fx.map);

    for (int x = 0; x < fx.map.w; ++x)
        for (int y = 0; y < fx.map.h; ++y)
            fx.map.room[x][y].enemies_seen_frame = 0;

    for (int i = 0; i < Table_Max; i++)
        distanceTable[i].center = 0;

    destinationType = Dest_None;
    destination = 0;
}

void Robot::BuildDistanceTable(Area* startPoint, DistanceTableId num) throw () {
    return BuildDistanceTable(vector<Area*>(1, startPoint), num);
}

void Robot::BuildDistanceTable(const vector<Area*>& startPoints, DistanceTableId num) throw () {
    if (startPoints.size() == 1) {
        if (distanceTable[num].center == startPoints[0])
            return;
        distanceTable[num].center = startPoints[0];
    }
    else
        distanceTable[num].center = 0;

    areaMap.clearDistanceTable(num);

    queue<const Area*> workQueue;
    for (vector<Area*>::const_iterator ai = startPoints.begin(); ai != startPoints.end(); ++ai) {
        (*ai)->distance[num] = 0;
        workQueue.push(*ai);
    }
    while (!workQueue.empty()) {
        const Area* const a = workQueue.front();
        workQueue.pop();
        for (vector<Area::Neighbor>::const_iterator ni = a->neighbors().begin(); ni != a->neighbors().end(); ++ni) {
            if (ni->area->distance[num] != -1) // already labeled
                continue;
            ni->area->distance[num] = a->distance[num] + 1;
            workQueue.push(ni->area);
        }
    }
}

void Robot::setDestination(Area* const target) throw () {
    nAssert(target);
    if (target == destination)
        return;
    destination = target;
    #ifdef BOTDEBUG
    fprintf(stderr, "%d: Set destination: %d %d\n", me, target->roomx, target->roomy);
    #endif

    BuildDistanceTable(target, Table_Destination);
}

ClientControls Robot::MoveToDestination(double melx, double mely) const throw () {
    if (destinationType == Dest_None)
        return ClientControls();

    const Area* const here = myArea();

    if (here->distance[Table_Destination] == -1 || destination == here)
        return ClientControls();

    vector<const Area::Neighbor*> goodNeighbors;
    bool oldDestinationFound = false;
    for (vector<Area::Neighbor>::const_iterator ni = here->neighbors().begin(); ni != here->neighbors().end(); ++ni) {
        const Area* const na = ni->area;
        if (na->distance[Table_Destination] < here->distance[Table_Destination]) {
            if (HaveFlag(me)) {
                const TeamCounts tc = Teams(na, false);
                if (tc.enemies > tc.friends + 1)
                    continue;
            }
            if (&*ni == immediateDestination) {
                oldDestinationFound = true;
                break;
            }
            if (!dangerousExplosionInNeighbor(*ni, melx, mely))
                goodNeighbors.push_back(&*ni);
        }
    }

    if (oldDestinationFound && dangerousExplosionInNeighbor(*immediateDestination, melx, mely)) // we keep the same destination and wait for the explosion to settle because it won't take long
        return ClientControls();

    if (!oldDestinationFound)
        immediateDestination = goodNeighbors.empty() ? 0 : goodNeighbors[rand() % goodNeighbors.size()];

    if (immediateDestination)
        return MoveToDoor(melx, mely, *immediateDestination);
    else
        return ClientControls();
}

Coords Robot::nearestDoor(const Area::Neighbor& neighbor, double lx, double ly) const throw (AlreadyInRoom) {
    int fixedTarget;
    bool xFixed;
    switch (neighbor.direction) {
    /*break;*/ case Area::Neighbor::Up:
            fixedTarget = 0;
            xFixed = false;
            if (ly < 0) // if predicted correctly, we're already in the target room (by the time our controls reach the server)
                throw AlreadyInRoom();
        break; case Area::Neighbor::Down:
            fixedTarget = S_H;
            xFixed = false;
            if (ly > S_H)
                throw AlreadyInRoom();
        break; case Area::Neighbor::Left:
            fixedTarget = 0;
            xFixed = true;
            if (lx < 0)
                throw AlreadyInRoom();
        break; case Area::Neighbor::Right:
            fixedTarget = S_W;
            xFixed = true;
            if (lx > S_W)
                throw AlreadyInRoom();
        break; default:
            nAssert(0);
    }
    const double myFreeCoord = xFixed ? ly : lx;
    vector< pair<double, double> >::const_iterator nearestDoor = neighbor.doors.end();
    double minDist = 1e10;
    for (vector< pair<double, double> >::const_iterator di = neighbor.doors.begin(); di != neighbor.doors.end(); ++di) { // find the nearest door
        if (di->first <= myFreeCoord && di->second >= myFreeCoord) {
            nearestDoor = di;
            break;
        }
        const double dist = di->first <= myFreeCoord ? myFreeCoord - di->second : di->first - myFreeCoord;
        if (dist < minDist) {
            nearestDoor = di;
            minDist = dist;
        }
    }
    nAssert(nearestDoor != neighbor.doors.end());
    const double doorMiddle = (nearestDoor->first + nearestDoor->second) / 2;
    const double niceLow  = min(doorMiddle, nearestDoor->first  + 4 * PLAYER_RADIUS); // if possible, take some headroom, mainly to avoid bumping into walls
    const double niceHigh = max(doorMiddle, nearestDoor->second - 4 * PLAYER_RADIUS);
    const double freeTarget = bound(myFreeCoord, niceLow, niceHigh);
    if (xFixed)
        return Coords(fixedTarget, freeTarget);
    else
        return Coords(freeTarget, fixedTarget);
}

ClientControls Robot::MoveToDoor(double mex, double mey, const Area::Neighbor& neighbor) const throw () {
    const Area::Neighbor::Direction dir = neighbor.direction;
    try {
        const Coords door = nearestDoor(neighbor, mex, mey);
        return MoveToNoAggregate(mex, mey, door.x + PLAYER_RADIUS * xDelta(dir) - mex, door.y + PLAYER_RADIUS * yDelta(dir) - mey, 0);
    } catch (AlreadyInRoom) {
        ClientControls ctrl;
        ctrl.setRun();
        switch (dir) {
        /*break;*/ case Area::Neighbor::Up:    return ctrl.setUp();
            break; case Area::Neighbor::Down:  return ctrl.setDown();
            break; case Area::Neighbor::Left:  return ctrl.setLeft();
            break; case Area::Neighbor::Right: return ctrl.setRight();
        }
        nAssert(0);
        return ctrl;
    }
}

int Robot::GetPlayers(int team) const throw () {
    int npl = 0;
    for (int i = 0; i < maxplayers; ++i) {
        const ClientPlayer& player = fx.player[i];
        if (player.used && player.team() == team)
            ++npl;
    }
    return npl;
}

bool Robot::IsDefender() throw () {
    // get flag base
    const int team = fx.player[me].team();
    const vector<WorldCoords>& tflags = fx.map.tinfo[team].flags;

    if (tflags.empty())
        return false;

    const int npl = GetPlayers(team);
    const int nAllFlags = fx.map.tinfo[0].flags.size() + fx.map.tinfo[1].flags.size() + fx.map.wild_flags.size();
    const int defNum = npl / nAllFlags; // split players evenly between all flags of all teams, rounding down the number of defenders per flag

    const Area* const here = myArea();

    // for all bases
    for (vector<WorldCoords>::const_iterator pi = tflags.begin(); pi != tflags.end(); ++pi) {
        BuildDistanceTable(area(*pi), Table_Def); //#opt: reserve enough distance tables to avoid building them here every frame in case of multiple flags
        const int m_distance = here->distance[Table_Def];
        int nearNum = 0;
        for (int i = 0; i < maxplayers; ++i) {
            const ClientPlayer& player = fx.player[i];
            if (!player.used || player.team() != fx.player[me].team() || player.dead || i == me ||
                player.roomx >= fx.map.w || player.roomy >= fx.map.h)
                    continue;
            const int distance = area(player)->distance[Table_Def];
            if (distance < m_distance || distance == m_distance && (i < me || HaveFlag(i)))
                nearNum++;
        }
        if (nearNum < defNum)
            return true;
    }
    return false;
}

void Robot::ChooseDestination() throw () { // NEED rewrite
    const int flag = HaveFlag(me);
    destinationType = Dest_None;

    if (!flag) {
        const bool at_bases = IsFlagsAtBases(fx.player[me].team()); // are own flags safe?

        const bool sef = !lock_team_flags_in_effect; // try to steal enemy flags?
        const bool swf = !lock_wild_flags_in_effect; // try to steal wild flags?

        bool efc = !IsCarriersDef(1 - fx.player[me].team()); // try to defend carriers of enemy flags?
        bool wfc = !IsCarriersDef(2); // try to defend carriers of wild flags?
        bool mfb = sef && IsDefender(); // try to defend own flags at bases? (!sef means the enemy won't try our flags either, so nothing to defend)

        if (at_bases && !efc && !wfc && !mfb) { // all flags are safe and nothing to support
            TargetNearest(sef, sef,   0,
                            0,   0,   0,
                          swf, swf,   0,   0,
                            0,   0,
                            0,   0,   0);
            if (destinationType == Dest_None) { // we are in control of all flags -> always defend
                efc = wfc = 1;
                mfb = sef; // still no point in defending the base if the flag can't be taken - resources better spent defending carriers
            }
        }
        if (destinationType == Dest_None) {
            TargetNearest(sef, sef, efc,
                          mfb,   1,   1,
                          swf, swf,   1, wfc,
                            0,   0,
                            0,   0,   0);
        }
        if (destinationType == Dest_None) {
            TargetNearest(  0,   0,   0,
                            0,   0,   0,
                            0,   0,   0,   0,
                            1,   0,
                          sef,   0,   0);  // ..., or enemy, or enemy base
        }
        if (destinationType == Dest_None || destinationType == Dest_Base) {
            if (destinationType == Dest_Base) {
                const TeamCounts tc = Teams(destination, false);
                if (tc.friends) { // if we are going to base where is already our forces, forget it
                    if (destination != myArea() || AmILast())
                        TargetFog();
                }
            }
            else
                TargetFog();
        }
    }
    else { // i am flagman ;)
        const bool ctf = capture_on_team_flags_in_effect;
        const bool cwf = capture_on_wild_flags_in_effect;

        if (GetPlayers(fx.player[me].team()) > 1) {
            TargetNearest(  0,   0,   0,
                          ctf,   0,   0,
                          cwf,   0,   0,   0,
                            0,   0,
                            0,   0,   0); // available capture point
        }
        else {
            TargetNearest(  0,   0,   0,
                          ctf,   1,   0,
                          cwf,   0,   0,   0,
                            0,   0,
                            0,   0,   0); // available capture point, or dropped own team flag
        }
        if (destinationType == Dest_None) {
            TargetNearest(  0,   0,   0,
                            0,   0,   0,
                            0,   0,   0,   0,
                            0,   0,
                            0, ctf,   0);  // ok, to capture point, even if unavailable
        }
        if (destinationType == Dest_None) {
            TargetNearest(  0,   0,   0,
                            0,   0,   0,
                            0,   0,   0,   0,
                            0,   0,
                            0,   0, cwf);  // only go wait in an empty wild flag base as a last resort
        }
    }
}

bool Robot::IsMassive() const throw () {
    double dx = 0, dy = 0;
    int n = 0;
    for (int i = 0; i < maxplayers; ++i) {
        if (!fx.player[i].used || fx.player[i].team() != fx.player[me].team() || fx.player[i].dead || !fx.player[i].onscreen) //#fix: should we eliminate players in other areas within the same room from this?
            continue;

        dx += fabs(fx.player[i].lx - fx.player[me].lx + averageLag * (fx.player[i].sx - fx.player[me].sx));
        dy += fabs(fx.player[i].ly - fx.player[me].ly + averageLag * (fx.player[i].sy - fx.player[me].sy));
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

int Robot::HaveFlag(int n) const throw () {
    const int t = 1 - fx.player[n].team();
    nAssert(t == 0 || t == 1);

    // look for enemy flags in team
    for (vector<Flag>::const_iterator fi = fx.teams[t].flags().begin(); fi != fx.teams[t].flags().end(); ++fi)
        if (fi->carried() && fi->carrier() == n)
            return 1;

    // looking for wild flags
    for (vector<Flag>::const_iterator fi = fx.wild_flags.begin(); fi != fx.wild_flags.end(); ++fi)
        if (fi->carried() && fi->carrier() == n)
            return 2;

    return 0;
}

bool Robot::IsFlagAtBase(const Flag& f, int team) const throw () {
    const vector<WorldCoords>& bases = fx.map.tinfo[team].flags;
    for (vector<WorldCoords>::const_iterator bi = bases.begin(); bi != bases.end(); ++bi)
        if (bi->px == f.position().px && bi->py == f.position().py && fabs(bi->x - f.position().x) <= 5. && fabs(bi->y - f.position().y) <= 5.)
            return true;
    return false;
}

void Robot::TargetNearestBase(int& m_distance, Area*& targetArea, int team) throw () {
    const vector<WorldCoords>& tflags = fx.map.tinfo[team].flags;
    int distance = 0;

    for (vector<WorldCoords>::const_iterator pi = tflags.begin(); pi != tflags.end(); ++pi) {
        Area* const a = area(*pi);
        distance = a->distance[Table_Main];
        if (distance == -1)
            continue;
        if (distance < m_distance || m_distance == -1) {
            m_distance = distance;
            targetArea = a;
            destinationType = Dest_Base;
        }
    }
}

void Robot::TargetNearestTeam(int& m_distance, Area*& targetArea, int team) throw () {
    // looking for soldiers
    const bool enemy = (fx.player[me].team() != team);

    for (int i = 0; i < maxplayers; ++i) {
        const ClientPlayer& pl = fx.player[i];
        if (i == me || !pl.used || pl.team() != team || pl.dead || pl.roomx >= fx.map.w || pl.roomy >= fx.map.h)
            continue;

        if (enemy) {
            if (fx.frame - pl.posUpdated > FADEOUT)
                continue;
            if (fx.map.room[pl.roomx][pl.roomy].enemies_seen_frame > pl.posUpdated)
                continue;
        }

        Area* const a = area(pl);
        const int distance = a->distance[Table_Main];
        if (distance == -1)
            continue;

        if (distance < m_distance || m_distance == -1) {
            m_distance = distance;
            targetArea = a;
            destinationType = Dest_Team;
        }
    }
}

bool Robot::IsCarriersDef(int team) throw () {
    const vector<Flag>& flags = (team != 2) ? fx.teams[team].flags() : fx.wild_flags;

    vector<Area*> carrierAreas;

    for (vector<Flag>::const_iterator fi = flags.begin(); fi != flags.end(); ++fi)
        if (fi->carried())
            carrierAreas.push_back(area(fx.player[fi->carrier()]));

    if (carrierAreas.empty()) // nothing to defend
        return true;

    BuildDistanceTable(carrierAreas, Table_Def);

    int teammates = 0, nearer = 0;
    const int myDist = myArea()->distance[Table_Def];
    for (int pi = 0; pi < maxplayers; ++pi) {
        const ClientPlayer& pl = fx.player[pi];
        if (!pl.used || pl.team() != fx.player[me].team())
            continue;
        ++teammates;
        const int dist = area(pl)->distance[Table_Def];
        if (dist < myDist || dist == myDist && (pi < me || HaveFlag(pi)))
            ++nearer;
    }
    return nearer >= teammates / 2;
}

bool Robot::IsHome(const Area* a) const throw () {
    const vector<WorldCoords>& tflags = fx.map.tinfo[fx.player[me].team()].flags;
    // our bases
    for (vector<WorldCoords>::const_iterator pi = tflags.begin(); pi != tflags.end(); ++pi)
        if (area(*pi) == a)
            return true;
    return false;
}

bool Robot::IsFlagsAtBases(int team) const throw () {
    const vector<Flag>& flags = (team != 2) ? fx.teams[team].flags() : fx.wild_flags;
    for (vector<Flag>::const_iterator fi = flags.begin(); fi != flags.end(); ++fi)
        if (fi->carried() || !IsFlagAtBase(*fi, team))
            return false;
    return true;
}

void Robot::TargetNearestFlag(int& m_distance, Area*& targetArea, int team, int state) throw () {
    // state - 0 - at base, 1 - dropped off base, 2 - carried by friends, 3 - carried by enemy

    const bool wantCarried = state == 2 || state == 3;
    const int carrierTeam = state == 2 ? fx.player[me].team() : !fx.player[me].team();

    const vector<Flag>& flags = (team != 2) ? fx.teams[team].flags() : fx.wild_flags;

    for (vector<Flag>::const_iterator fi = flags.begin(); fi != flags.end(); ++fi) {
        if (fi->carried() != wantCarried)
            continue;

        WorldCoords pos;
        if (fi->carried()) {
            const ClientPlayer& pl = fx.player[fi->carrier()];
            if (pl.team() != carrierTeam || !pl.used || pl.roomx >= fx.map.w || pl.roomy >= fx.map.h)
                continue;
            if (state == 3 && !pl.onscreen) { // check if the position is current enough
                if (fx.frame - pl.posUpdated > FADEOUT) // TODO fadeout
                    continue;
                if (fx.map.room[pl.roomx][pl.roomy].enemies_seen_frame > pl.posUpdated)
                    continue;
            }
            pos = pl.position();
        }
        else {
            if (IsFlagAtBase(*fi, team) != (state == 0))
                continue;
            pos = fi->position();
        }

        Area* const a = area(pos);
        const int distance = a->distance[Table_Main];
        if (distance == -1)
            continue;

        if (distance < m_distance || m_distance == -1) {
            m_distance = distance;
            targetArea = a;
            destinationType = Dest_Flag;
        }
    }
}

void Robot::TargetFog() throw () {
    const Area* const here = myArea();
    double max_delta = 0;
    Area* target = 0;
    for (vector<Area::Neighbor>::const_iterator ni = here->neighbors().begin(); ni != here->neighbors().end(); ++ni) {
        Area* const na = ni->area;
        const TeamCounts tc = Teams(na, false);
        if (tc.friends && !tc.enemies) // our sector
            continue;
        const double delta = fabs(fx.frame - fx.map.room[na->roomx][na->roomy].enemies_seen_frame);
        if (delta >= max_delta) {
            max_delta = delta;
            target = na;
        }
    }
    if (!target)
        return;

    destinationType = Dest_Fog;
    setDestination(target);
}

/* TargetNearest(enemy flag {at base, dropped off base, carried},
 *                  my flag {at base, dropped off base, carried},
 *                wild flag {at base, dropped off base, carried by enemy, carried by friend},
 *         {enemy, my} team,
 *   {enemy, my, wild} base)
 *
 * targets first one of the enabled options that yields the minimal distance among enabled options.
 *
 * flag: a flag with desired status and probably known location
 * team: a living non-me player with probably known location
 * base: a base, regardless of where its flag is
 */
void Robot::TargetNearest(int efb, int efd, int efc,
                          int mfb, int mfd, int mfc,
                          int wfb, int wfd, int wfce, int wfcf,
                          int en,  int fr,
                          int eb,  int fb, int wb) throw () {
    int m_distance = -1;
    Area* targetArea = 0;
    const int t = fx.player[me].team();
    const int et = 1 - t;

    BuildDistanceTable(myArea(), Table_Main);

    destinationType = Dest_None;

    if (efb)
        TargetNearestFlag(m_distance, targetArea, et, 0);

    if (efd)
        TargetNearestFlag(m_distance, targetArea, et, 1);

    if (efc)
        TargetNearestFlag(m_distance, targetArea, et, 2);

    if (mfb)
        TargetNearestFlag(m_distance, targetArea, t, 0);

    if (mfd)
        TargetNearestFlag(m_distance, targetArea, t, 1);

    if (mfc)
        TargetNearestFlag(m_distance, targetArea, t, 3);

    if (wfb)
        TargetNearestFlag(m_distance, targetArea, 2, 0);

    if (wfd)
        TargetNearestFlag(m_distance, targetArea, 2, 1);

    if (wfce)
        TargetNearestFlag(m_distance, targetArea, 2, 3);

    if (wfcf)
        TargetNearestFlag(m_distance, targetArea, 2, 2);

    if (en)
        TargetNearestTeam(m_distance, targetArea, et);

    if (fr)
        TargetNearestTeam(m_distance, targetArea, t);

    if (eb)
        TargetNearestBase(m_distance, targetArea, et);
    if (fb)
        TargetNearestBase(m_distance, targetArea, t);
    if (wb)
        TargetNearestBase(m_distance, targetArea, 2);

    #ifdef DEBUGSTRATEGY
    fprintf(stderr, "%d %s: ", static_cast<int>(fx.frame / 10) - map_start_time, fx.player[me].name.c_str());
    fprintf(stderr, "TargetNearest(%d, %d, %d,   %d, %d, %d,   %d, %d, %d, %d,   %d, %d,   %d, %d, %d) -> %d\n",
            efb, efd, efc,
            mfb, mfd, mfc,
            wfb, wfd, wfce, wfcf,
            en,  fr,
            eb,  fb, wb,
            destinationType);
    #endif

    if (destinationType == Dest_None) // nothing todo
        return;

    nAssert(targetArea);

    setDestination(targetArea);
}

bool Robot::IsMission() const throw () {
    const int to_home = IsHome(destination);
    // if we are looking for flag or going to our base for something
    if (destination == myArea())
        return false;
    return HaveFlag(me) || destinationType == Dest_Flag || to_home || !to_home && destinationType == Dest_Base;
}

void Robot::updateUnknownPosition(ClientPlayer& pl) throw () {
    if (pl.posUpdated < fx.frame - FADEOUT) // we don't care about them anymore
        return;
    if (pl.posUpdated >= fx.map.room[pl.roomx][pl.roomy].enemies_seen_frame) // they could still be there
        return;

    // see where they could have gone
    const Area* a = area(pl);
    WorldCoords posGuess;
    double timeGuess = 0; // initialized to please GCC
    bool haveGuess = false;
    for (vector<Area::Neighbor>::const_iterator ni = a->neighbors().begin(); ni != a->neighbors().end(); ++ni) {
        Coords doorPos;
        try {
            doorPos = nearestDoor(*ni, pl.lx, pl.ly);
        } catch (AlreadyInRoom) {
            nAssert(0);
        }
        const double doorDist = sqrt(sqr(doorPos.x - pl.lx) + sqr(doorPos.y - pl.ly));
        const double earliestTimeThere = pl.posUpdated + ceil(doorDist / fx.physics.max_run_speed);
        if (earliestTimeThere > fx.frame)
            continue;

        const Area* n = ni->area;
        const double nSeen = fx.map.room[n->roomx][n->roomy].enemies_seen_frame;
        if (nSeen >= fx.frame - 10)
            continue; // actually, they could have went in and out before nSeen if there was a period of the room not being seen, but such is hard to keep track of

        if (haveGuess) // many possible neighbors, don't try to guess
            return; //#improve: ideally, save the info about rooms that were found impossible, to work better when some current choices are proven impossible

        const double lx = doorPos.x - xDelta(ni->direction) * S_W,
                     ly = doorPos.y - yDelta(ni->direction) * S_H; // move from this-room-coords to relative to the neighbor
        posGuess = WorldCoords(n->roomx, n->roomy, lx, ly);
        timeGuess = max(nSeen + 10, earliestTimeThere);
        haveGuess = true;
    }
    if (haveGuess) {
        #ifdef BOTDEBUG
        fprintf(stderr, "%d %s: ", static_cast<int>(fx.frame / 10) - map_start_time, fx.player[me].name.c_str());
        fprintf(stderr, "Guessing %s moved to %d,%d (%d,%d) %.1f s ago - last seen at %d,%d/%d,%d %.1f s ago, verified away %.1f s ago\n", pl.name.c_str(),
                posGuess.px, posGuess.py, (int)posGuess.x, (int)posGuess.y,
                (fx.frame - timeGuess) / 10.,
                pl.roomx, pl.roomy, (int)pl.lx, (int)pl.ly,
                (fx.frame - pl.posUpdated) / 10.,
                (fx.frame - fx.map.room[pl.roomx][pl.roomy].enemies_seen_frame) / 10.);
        #endif
        pl.setPosition(posGuess, timeGuess); // leaves no mark about the position being a guess, but that isn't terrible
    }
    else
        pl.posUpdated = fx.frame - FADEOUT; // "forget" the position
}

ClientControls Robot::getRobotControls() throw () {
    const double mex = fx.player[me].lx + averageLag * fx.player[me].sx;
    const double mey = fx.player[me].ly + averageLag * fx.player[me].sy;

    fx.map.room[fx.player[me].roomx][fx.player[me].roomy].enemies_seen_frame = fx.frame;

    if (fx.player[me].item_shadow_time > fx.frame / 10.) {
        for (int x = 0; x < fx.map.w; ++x)
            for (int y = 0; y < fx.map.h; ++y) {
                double& esf = fx.map.room[x][y].enemies_seen_frame;
                esf = max(esf, fx.frame - 10); // estimate at most 10 frames to send everyone's position (assumes that we already had shadow 10 frames ago)
            }
    }

    for (int pi = 0; pi < maxplayers; ++pi) {
        const ClientPlayer& p = fx.player[pi];
        if (!p.used || p.dead || p.team() != fx.player[me].team())
            continue;
        if (p.posUpdated == fx.frame && p.fromMinimapUpdate && p.prevMapPosUpdateFrame >= p.posUpdated - 20 && p.prevMapUpdateRoomx == p.roomx && p.prevMapUpdateRoomy == p.roomy) {
            double& esf = fx.map.room[p.roomx][p.roomy].enemies_seen_frame;
            esf = max(esf, p.prevMapPosUpdateFrame); // we'd expect to have received information of any enemies in the room between the two updates of the friend
        }
    }

    for (int pi = 0; pi < maxplayers; ++pi) {
        ClientPlayer& p = fx.player[pi];
        if (p.used && !p.dead && p.team() != fx.player[me].team())
            updateUnknownPosition(p);
    }

    if (last_seen != -1) {
        const ClientPlayer& lsp = fx.player[last_seen];
        if (!lsp.used || lsp.team() == fx.player[me].team() || !lsp.onscreen || lsp.dead) // lost target
            last_seen = -1;
    }

    if (!IsMassive()) {
        const int dangerousRocket = GetDangerousRocket();
        if (dangerousRocket != -1) {
            if (last_seen == -1)
                last_seen = GetDangerousEnemy(mex, mey);
            #ifdef DEBUGSTRATEGY
            fprintf(stderr, "%d %s: ", static_cast<int>(fx.frame / 10) - map_start_time, fx.player[me].name.c_str());
            fprintf(stderr, "Escaping rocket.\n");
            #endif
            return EscapeRocket(mex, mey, dangerousRocket);
        }
    }

    ClientControls ctrl = EscapeExplosion(mex, mey);
    if (!ctrl.idle()) {
        #ifdef DEBUGSTRATEGY
        fprintf(stderr, "%d %s: ", static_cast<int>(fx.frame / 10) - map_start_time, fx.player[me].name.c_str());
        fprintf(stderr, "Escaping explosion.\n");
        #endif
        return ctrl;
    }

    ctrl = GetFlag(mex, mey);
    if (!ctrl.idle()) { // if any
        #ifdef DEBUGSTRATEGY
        fprintf(stderr, "%d %s: ", static_cast<int>(fx.frame / 10) - map_start_time, fx.player[me].name.c_str());
        fprintf(stderr, "Targetting flag.\n");
        #endif
        return ctrl;
    }

    ChooseDestination();

    if (IsMission()) // get only dangerous enemy
        last_seen = GetDangerousEnemy(mex, mey);
    else if (last_seen != -1) { // already locked on someone
        const int easy = GetEasyEnemy(mex, mey); // more easy???
        if (easy != -1)
            last_seen = easy;
    }
    else // get someone
        last_seen = GetNearestEnemy(mex, mey);

    const bool importantMission = HaveFlag(me) && IsMission();

    if (!importantMission && last_seen != -1 && !fx.physics.allowFreeTurning) {
        #ifdef DEBUGSTRATEGY
        fprintf(stderr, "%d %s: ", static_cast<int>(fx.frame / 10) - map_start_time, fx.player[me].name.c_str());
        fprintf(stderr, "Targetting enemy.\n");
        #endif
        return Aim(mex, mey, last_seen);
    }

    // ok, free tour ;)
    ctrl = GetPowerup(mex, mey, importantMission);
    if (!ctrl.idle()) {
        #ifdef DEBUGSTRATEGY
        fprintf(stderr, "%d %s: ", static_cast<int>(fx.frame / 10) - map_start_time, fx.player[me].name.c_str());
        fprintf(stderr, "Targetting powerup.\n");
        #endif
        return ctrl;
    }

    ctrl = Escape(mex, mey);
    if (!ctrl.idle()) {
        #ifdef DEBUGSTRATEGY
        fprintf(stderr, "%d %s: ", static_cast<int>(fx.frame / 10) - map_start_time, fx.player[me].name.c_str());
        fprintf(stderr, "Escaping from room.\n");
        #endif
        return ctrl;
    }

    ctrl = MoveToDestination(mex, mey);
    if (!ctrl.idle()) {
        #ifdef DEBUGSTRATEGY
        fprintf(stderr, "%d %s: ", static_cast<int>(fx.frame / 10) - map_start_time, fx.player[me].name.c_str());
        fprintf(stderr, "Going to %d,%d (target type %d).\n", destination->roomx, destination->roomy, destinationType);
        #endif
        return ctrl;
    }

    ctrl = FollowFlag(mex, mey);
    if (!ctrl.idle()) {
        #ifdef DEBUGSTRATEGY
        fprintf(stderr, "%d %s: ", static_cast<int>(fx.frame / 10) - map_start_time, fx.player[me].name.c_str());
        fprintf(stderr, "Following a carrier.\n");
        #endif
        return ctrl;
    }

    ctrl = FreeWalk(mex, mey);
    if (last_seen == -1)
        ctrl.clearRun();
    #ifdef DEBUGSTRATEGY
    fprintf(stderr, "%d %s: ", static_cast<int>(fx.frame / 10) - map_start_time, fx.player[me].name.c_str());
    if (destinationType == Dest_None)
        fprintf(stderr, "Nothing to do.\n");
    else
        fprintf(stderr, "Nothing to do, already at target [type %d].\n", destinationType);
    #endif
    return ctrl;
}

ClientControls Robot::RobotMain() throw () {
    const bool hide_map = !map_ready || gameover_plaque != NEXTMAP_NONE || fx.skipped || me < 0 || me >= maxplayers;

    if (hide_map || !fx.player[me].used || fx.player[me].dead || fx.player[me].team() != 0 && fx.player[me].team() != 1 ||
                    fx.player[me].roomx >= fx.map.w || fx.player[me].roomy >= fx.map.h) {
        myGundir = -1;
        if (botPrevFire) {
            BinaryBuffer<16> msg;
            msg.U8(data_fire_off);
            client->send_message(msg);
            botPrevFire = false;
        }
        return ClientControls();
    }

    if (myGundir == -1) // was dead, or something like that
        myGundir = fx.player[me].gundir.to8way();

    ClientControls ctrl = getRobotControls();

    const int currentGundir = myGundir;

    if (!ctrl.isStrafe() && ctrl.getDirection() != -1)
        myGundir = ctrl.getDirection();

    const double mex = fx.player[me].lx + averageLag * fx.player[me].sx;
    const double mey = fx.player[me].ly + averageLag * fx.player[me].sy;
    bool shoot;
    if (fx.physics.allowFreeTurning) {
        const pair<bool, GunDirection> shootDir = NeedShootFreeTurning(mex, mey, GunDirection().from8way(myGundir)); // if there's no player to target, aim where we're going
        shoot = shootDir.first;
        // adjust gunDir
        static const double turnCeilingPerFrame = N_PI_2;
        static const double displacementMul = .7;
        static const double shootTreshold = N_PI / 8.; // shoot if aim is within 22° of target
        const double targetDiff = positiveFmod(shootDir.second.toRad() - gunDir.toRad() + N_PI, 2 * N_PI) - N_PI;
        double actualDiff = targetDiff;
        if (fabs(actualDiff) > turnCeilingPerFrame)
            actualDiff = turnCeilingPerFrame * (actualDiff > 0 ? +1. : -1.);
        const double modifier = (rand() % 10001 - 5000) / 5000.;
        actualDiff *= 1. + displacementMul * modifier * modifier * modifier; // actually turn by something between actualDiff * (1 +/- displacementMul), weighted so that values in the middle are more likely than extremes
        gunDir.adjust(actualDiff);
        if (fabs(actualDiff - targetDiff) > shootTreshold)
            shoot = false;
    }
    else {
        const pair<bool, int> shootDir = NeedShootTradTurning(mex, mey);
        shoot = shootDir.first;
        if (shoot && shootDir.second != myGundir) {
            if (shootDir.second == currentGundir) {
                nAssert(!ctrl.isStrafe());
                ctrl.setStrafe();
                myGundir = currentGundir;
            }
            else {
                ctrl.fromDirection(shootDir.second);
                myGundir = shootDir.second;
            }
        }
    }
    if (shoot) {
        if (!botPrevFire) {
            BinaryBuffer<16> msg;
            msg.U8(data_fire_on);
            client->send_message(msg);
            botPrevFire = true;
        }
    }
    else if (botPrevFire) {
        BinaryBuffer<16> msg;
        msg.U8(data_fire_off);
        client->send_message(msg);
        botPrevFire = false;
    }

    return ctrl;
}

Robot::Robot(const ClientExternalSettings& config, Log& clientLog, MemoryLog& externalErrorLog_) throw () :
    ClientBase(config, clientLog, externalErrorLog_),
    sharedDataHandle(g_botSharedDataStorage),
    finished(false),
    botPrevFire(false)
{ }

void Robot::bot_start(const Network::Address& addr, int ping, const string& name, int bot_id) throw () {
    Lock ml(frameMutex);
    #ifndef DEDICATED_SERVER_ONLY
    botmode = true;
    #endif
    botId = bot_id;
    serverIP = addr;

    startBase("_bot" + itoa(botId));

    playername = name;

    botReactedFrame = -1;

    set_ping(ping);

    connect_command();
}

void Robot::set_ping(int ping) throw () {
    while (client->decreasePacketDelay()) { }
    for (int i = 0; i < ping / 10; ++i)
        client->increasePacketDelay();
}

void Robot::client_connected(ConstDataBlockRef data) throw () { // call with frameMutex locked
    ClientBase::client_connected(data);

    bot_send_frame(ClientControls());
}

void Robot::client_disconnected(ConstDataBlockRef data) throw () {
    BinaryDataBlockReader read(data);

    const uint8_t reason = read.U8();
    numAssert2(!read.hasMore() && (reason == server_c::disconnect_client_initiated || reason == server_c::disconnect_server_shutdown
                                   || reason == server_c::disconnect_timeout || reason == disconnect_kick),
               data.size(), reason);

    stop();
}

void Robot::connect_command() throw () {   // call with frameMutex locked
    prepareForConnect();
    connect(serverIP.toString(), bot_password, "");
}

void Robot::bot_send_frame(ClientControls controls) throw () {
    ++clFrameSent;
    controlHistory[clFrameSent] = sentControls = controls;
    svFrameHistory[clFrameSent] = fx.frame + (get_time() - frameReceiveTime) * 10.;
    BinaryBuffer<256> msg;
    msg.U8(clFrameSent);
    msg.U8(sentControls.toNetwork(false));
    if (fx.physics.allowFreeTurning)
        msg.U16(gunDir.toNetworkLongForm());
    client->send_frame(msg);
}

void Robot::bot_loop() throw () {
    Lock ml(frameMutex);

    handlePendingThreadMessages();

    if (!connected || fx.frame == botReactedFrame)
        return;

    #ifdef BOT_TEST_COMPARE_TWO // change team if necessary
    #ifdef ALTERNATE_BOT
    const int desiredTeam = 1;
    #else
    const int desiredTeam = 0;
    #endif
    nAssert(me >= 0 && me < maxplayers);
    if (me / TSIZE != desiredTeam) {
        nAssert(fx.frame < 100);
        if (botReactedFrame == -1) {
            BinaryBuffer<16> msg;
            msg.U8(data_change_team_on);
            client->send_message(msg);
        }
    }
    #endif

    botReactedFrame = fx.frame;

    while (clientReadiesWaiting > 0) {
        send_client_ready();
        --clientReadiesWaiting;
    }

    if (mapChanged) {
        mapChanged = false;
        BuildMap();
    }

    fx.cleanOldDeathbringerExplosions();

    ClientControls controls = RobotMain();
    controls.clearModifiersIfIdle();
    bot_send_frame(controls);
}

void Robot::stop() throw () {
    ClientBase::stop();
    finished = true;
}
