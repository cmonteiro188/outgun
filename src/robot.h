/*
 *  robot.h
 *
 *  Copyright (C) 2006, 2008, 2009, 2010 - Niko Ritari
 *  Copyright (C) 2006, 2008 - Jani Rivinoja
 *  Copyright (C) 2006 - Peter Kosyh
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

#ifndef ROBOT_H_INC
#define ROBOT_H_INC

#include "bot_support.h"
#include "clientbase.h"
#include "client_interface.h"

extern BotSharedDataStorage g_botSharedDataStorage;

class Robot : private ClientBase, public BotInterface {
    enum RouteTargetType {
        Route_None,
        Route_Flag,
        Route_Base,
        Route_Team,
        Route_Fog
    };

    BotSharedDataHandle sharedDataHandle;

    std::string bot_password;
    int botId;
    double botReactedFrame;
    bool finished;

    AreaMap areaMap;
    typedef AreaMap::Area Area;

    struct RouteTableDescriptor { // the actual table is distributed in all Areas
        RouteTargetType type;
        const Area* target;
        const Area* center; // the Area that has distance=0, or 0 if there are multiple centers
    };

    RouteTableDescriptor route[Table_Max];
    bool        botPrevFire;
    int         last_seen;
    int         myGundir;

    struct TeamCounts {
        int enemies, friends;
    };

    void BuildMap() throw ();

          Area* area(int roomx, int roomy, double lx, double ly)       throw () { return areaMap.identifyArea(roomx, roomy, lx, ly); }
    const Area* area(int roomx, int roomy, double lx, double ly) const throw () { return areaMap.identifyArea(roomx, roomy, lx, ly); }
          Area* area(const WorldCoords& c)       throw () { return area(c.px, c.py, c.x, c.y); }
    const Area* area(const WorldCoords& c) const throw () { return area(c.px, c.py, c.x, c.y); }
          Area* area(const ClientPlayer& p)       throw () { return area(p.position()); }
    const Area* area(const ClientPlayer& p) const throw () { return area(p.position()); }
          Area* myArea()       throw () { return area(fx.player[me]); }
    const Area* myArea() const throw () { return area(fx.player[me]); }

    const DeathbringerExplosion* explosionInRoom(int roomx, int roomy) const throw (); // returns the dangerous deathbringer-explosion in the room, if any
    double distanceFromDoor(Area::Neighbor::Direction dir, double lx, double ly) const throw ();

    bool        IsDefender() throw (); // am i defender? (role)
    bool        IsCarriersDef(int team) throw (); // are flags of team that we carry safe?
    bool        IsFlagsAtBases(int team) const throw (); // are flags of team at bases?
    int         GetPlayers(int team) const throw (); // get num of players
    TeamCounts  Teams(const Area* a, bool countMe) const throw (); // get num of en and fr for sector
    bool        IsHome(const Area* a) const throw (); //is it base

    bool        AmILast() const throw ();
    bool        IsMission(RouteTable num) const throw (); // have i mission? (No agression mode)
    int         GetEasyEnemy(double mex, double mey) const throw (); // get easy enemy to kill
    bool        IsMassive() const throw (); // am i berserker? (No rocket avoiding)
    int         HaveFlag(int n) const throw (); // 0 if n isn't carrying a flag, 1 if n carries an enemy flag, 2 if n carries a wild flag
    bool        IsFlagAtBase(const Flag& f, int team) const throw ();
    enum AimLevel { AL_None, AL_Near, AL_Full };
    std::pair<AimLevel, int> TryAimTradTurning(double mex, double mey, int target) const throw (); // returns how near the target is to the aim in the best direction (AL_None if behind a wall), and that direction
    std::pair<bool, GunDirection> TryAimFreeTurning(double mex, double mey, int target) const throw (); // returns (shoot?, direction)
    double      GetHitTime(double mex, double mey, const GunDirection& dir, int iTarget) const throw (); // approximate time until a rocket shot in dir from (mex,mey) would hit player iTarget assuming no walls ("big" if no hit)
    double      GetHitTeammateTime(double mex, double mey, const GunDirection& dir) const throw (); // approximate time until a rocket shot in dir from (mex,mey) would hit first teammate assuming no walls ("big" if no hit, including if friendly fire is off)

    bool        IsBehindWall(double mex, double mey, double dx, double dy, double radius, double maxDistanceFromTarget) const throw ();
    double      ScanDir(double mex, double mey, GunDirection dir) const throw (); // return length to wall
    std::pair<bool, GunDirection> NeedShootFreeTurning(double mex, double mey, const GunDirection& defaultDir) throw (); // to shoot or not to shoot, and the gunDir to aim at (defaultDir if there's no target)
    std::pair<bool, int> NeedShootTradTurning(double mex, double mey) throw (); // to shoot or not to shoot, and the direction if shooting
    GunDirection GetDir(double dx, double dy) const throw (); // 0 - 0, 2 - Pi/2, 3 - Pi...
    int         GetDangerousRocket() const throw (); // get danger rocket index
    int         GetDangerousEnemy(double mex, double mey) const throw (); // same for enemy
    int         GetNearestEnemy(double mex, double mey) const throw (); // get nearest enemy
    int         FreeDir(double mex, double mey) const throw (); // maximum free space at front

    ClientControls Aim(double mex, double mey, int i) const throw ();
    ClientControls EscapeRocket(double mex, double mey, int mrock) const throw ();
    ClientControls EscapeExplosion(double mex, double mey) const throw ();
    ClientControls GetFlag(double mex, double mey) const throw ();
    ClientControls FollowFlag(double mex, double mey) const throw ();
    ClientControls GetPowerup(double mex, double mey, bool onImportantMission) const throw ();
    ClientControls MoveDirNoAggregate(int dir) const throw ();
    ClientControls MoveTo(double mex, double mey, double dx, double dy, double maxDistanceFromTarget) const throw ();
    ClientControls MoveToDoor(double mex, double mey, const Area::Neighbor& n) const throw ();
    ClientControls MoveToNoAggregate(double mex, double mey, double dx, double dy, double maxDistanceFromTarget) const throw ();
    ClientControls MoveDir(int dir) const throw ();
    ClientControls Escape(double mex, double mey) const throw ();
    ClientControls FreeWalk(double mex, double mey) const throw ();
    ClientControls Route(double mex, double mey, RouteTable num) const throw (); // follow route

    void BuildRouteTable(Area* startPoint, RouteTable num) throw (); // build route table from single points
    void BuildRouteTable(const std::vector<Area*>& startPoints, RouteTable num) throw (); // build route table from multiple points
    int  BuildRoute(Area* target, RouteTable num) throw (); // build route, return 0 if not needed, -1 if no path
    bool RouteLogic(RouteTable num) throw (); // build route on route table using AI, -1 if not builded

    // Build Route to nearest enemy flag, enemy flag carry, me flag, .... enemy, friend
    void TargetNearestBase(int& m_distance, Area*& nearestArea, int team, RouteTable num) throw ();
    void TargetNearestTeam(int& m_distance, Area*& nearestArea, int team, RouteTable num) throw ();
    void TargetNearestFlag(int& m_distance, Area*& nearestArea, int team, int state, RouteTable num) throw ();
    int TargetFog(RouteTable num) throw ();

    int TargetRoute(int efb, int efd, int efc,
                    int mfb, int mfd, int mfc,
                    int wfb, int wfd, int wfce, int wfcf,
                    int en,  int fr,
                    int eb,  int fb, int wb,
                    RouteTable num) throw ();

    ClientControls getRobotControls() throw ();

    ClientControls RobotMain() throw ();

    void connect_command() throw ();    // call with frameMutex locked

    void client_connected(ConstDataBlockRef data) throw ();    // call with frameMutex locked
    void client_disconnected(ConstDataBlockRef data) throw ();

    void bot_send_frame(ClientControls controls) throw ();

    std::string getPlayerPassword() const throw () { return std::string(); }

public:
    Robot(const ClientExternalSettings& config, Log& clientLog, MemoryLog& externalErrorLog_) throw ();
    ~Robot() throw () { }

    void bot_start(const Network::Address& addr, int ping, const std::string& name, int botId) throw ();
    void stop() throw ();
    void bot_loop() throw ();
    void set_ping(int ping) throw ();
    bool is_connected() const throw () { return connected; }
    bool bot_finished() const throw () { return finished; }

    int bot_player_id() const throw () { return me; }
    double bot_reacted_frame() const throw () { return botReactedFrame; }
    uint8_t bot_sent_frame() const throw () { return clFrameSent; }

    void set_bot_password(const std::string& pass) throw () { bot_password = pass; }

    int team() const throw () { return me / TSIZE; }
};

#endif
