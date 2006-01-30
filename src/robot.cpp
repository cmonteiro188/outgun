/*
 *  robot.cpp
 *
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

#ifdef BOTMODE
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <cctype>
#include <cmath>

#include "incalleg.h"
#include "leetnet/client.h"
#include "leetnet/sleep.h"  // sleep util
#include "commont.h"
#include "debug.h"
#include "debugconfig.h"	// for LOG_MESSAGE_TRAFFIC
#include "gameserver_interface.h"
#include "language.h"
#include "names.h"
#include "nassert.h"
#include "network.h"
#include "platform.h"
#include "protocol.h"   // needed for possible definition of SEND_FRAMEOFFSET, and otherwise
#include "utility.h"
#include "world.h"

#include "client.h"

#define fx fx
#define averageLag  (averageLag/2)

#define SCAN_RADIUS (ROCKET_RADIUS/2)

#define S_W (plw)
#define S_H (plh)
#define FADEOUT 100
#define ROUTE_FADEOUT 50

//#define D_W (PLAYER_RADIUS*2)


int Client::IsBehindWall(double mex, double mey, double dx, double dy)
{
    double deg;
    double tx, ty;

    if (!dx)
    {
	deg = N_PI_2;
    }	
    else
	deg = atan(fabs(dy)/fabs(dx));

    if(dx>=0 && dy<=0)
	deg = 2*N_PI - deg;
    else if(dx<=0 && dy>=0)
	deg = N_PI - deg;
    else if(dx<=0 && dy<=0)
	deg = N_PI + deg;

    tx = mex + dx;
    ty = mey + dy;
    double sx = cos(deg) * (SCAN_RADIUS*2);
    double sy = sin(deg) * (SCAN_RADIUS*2);
    Room &room = fx.map.room[fx.player[me].roomx][fx.player[me].roomy]; 

    while(1)
    {
	if(room.fall_on_wall((int)mex, (int)mey, SCAN_RADIUS))
	    return 1;
	mex += sx;
	mey += sy;
	if ((fabs(tx - mex)<PLAYER_RADIUS) && (fabs(ty - mey)<PLAYER_RADIUS))
	    break;    
	if(mex>S_W || mex<0)
	    break;
	if(mex>S_H || mey<0)
	    break;            
    }
    return 0;
}

double Client::ScanDir(double mex, double mey, int dir)
{
    double deg =  N_PI_4 *dir;

    double sx = cos(deg) * (SCAN_RADIUS*2);
    double sy = sin(deg) * (SCAN_RADIUS*2);

    double tx = mex;
    double ty = mey;

    Room &room = fx.map.room[fx.player[me].roomx][fx.player[me].roomy]; 

    while(1)
    {
	if(room.fall_on_wall((int)tx, (int)ty, SCAN_RADIUS))
	    break;
	tx += sx;
	ty += sy;

	if(tx>S_W || tx<0)
	    break;
	if(ty>S_H || ty<0)
	    break;            
    }
    return sqrt((tx -mex)*(tx - mex) + (ty - mey)*(ty-mey));
}

int Client::IsAimed(double mex, double mey, int i) // return 1 if in hit point
{
	double a;
	double dx, dy, tm, dist, ttx, tty;
	int dir;
	// XXX?
	ttx = (fx.player[i].lx)+averageLag *fx.player[i].sx;
        tty = (fx.player[i].ly)+averageLag *fx.player[i].sy;
			
	dx = ttx - mex;
	dy = tty - mey;
	
	dist = sqrt(dx*dx + dy*dy);

	tm= (dist / fx.physics.rocket_speed);
	dx += tm * (fx.player[i].sx);
	dy += tm * (fx.player[i].sy);

	if(dist<=PLAYER_RADIUS)
	    return 2;

	dir = GetDir(dx, dy);
	    
	if (fx.player[me].gundir != dir)
	    return 0;

	if(IsBehindWall(mex, mey, ttx - mex, tty - mey))
	    return 0;
	    
	if (dir ==0 || dir==4) // left or right
	    return (fabs(dy)<1.3*PLAYER_RADIUS)?2:1;
	if (dir ==2 || dir == 6) // up or down
	    return (fabs(dx)<1.3*PLAYER_RADIUS)?2:1; 
	
	a = sqrt(2*PLAYER_RADIUS*PLAYER_RADIUS); // diagonal?

	return ((fabs(dy) <= (fabs(dx) + 1.3*a)) && 
		 (fabs(dy)>= (fabs(dx) - 1.3*a)))?2:1;  		
    

}

int Client::GetDir(double dx, double dy)
{
	int dir = 0;
	double ntg;
	
	if((!dx) && (!dy))
		return 0;
		
	if(fabs(dx)>fabs(dy))
		ntg=fabs(dy)/fabs(dx);
	else
		ntg=fabs(dx)/fabs(dy);
				
	if(ntg<0.41) // h or v
	{
		if((dx>=0)&&(fabs(dy)<fabs(dx)))
			dir = 0;
		else if((dy>=0)&&(fabs(dy)>fabs(dx)))
			dir = 2;
		else if((dx<=0)&&(fabs(dx)>fabs(dy)))
			dir = 4;
		else 
			dir = 6;
	}
	else // diagonals
	{
		if(dx>0 && dy>0)
			dir =1;
		else if(dx<0 && dy>0)
			dir=3;
		else if(dx<0 && dy<0)
			dir=5;
		else dir=7;	
	}
	return dir;
}

int Client::GetDangerousRocket(double mex, double mey)
{
    int i=0;
    int mrock = -1;
    int d;
    
    double rdx, rdy, dist;
    double rdist = 0;
    
    for (i = 0; i < MAX_ROCKETS; ++i)
    {
	if(fx.rock[i].owner == -1)
	    continue;

	if((fx.rock[i].team == fx.player[me].team()) ||
	    (fx.rock[i].px != fx.player[me].roomx || fx.rock[i].py != fx.player[me].roomy)
	   )
	    continue;
	
	rdx = fx.rock[i].x - mex;
	rdy = fx.rock[i].y - mey;

	d = fx.rock[i].direction;
	
	if((d == 0 || d==1 || d == 7) && rdx > 4*PLAYER_RADIUS)
	    continue;
	if((d == 3 || d==4 || d == 5 ) && rdx< -4*PLAYER_RADIUS)
	    continue;
	if((d == 1 || d==2 || d == 3 ) && rdy> 4*PLAYER_RADIUS)
	    continue; 
	if((d == 5 || d==6 || d == 7 ) && rdy< -4*PLAYER_RADIUS)
	    continue;	

	if((d == 0) || (d==4))
	    dist = fabs(rdy);
	else if ((d==2)||(d==6))
	    dist = fabs(rdx);
	else if ((d==1)||(d==5)) 
	    dist = sqrt(((rdy-rdx)*(rdy-rdx))/2);
	else
	    dist = sqrt(((rdy+rdx)*(rdy+rdx))/2);

	if((dist < 6*PLAYER_RADIUS) && ((dist<rdist)|| !rdist)) //was 4
	{
	    if(IsBehindWall(mex, mey, rdx, rdy))
		continue;
	    mrock = i;
	    rdist = dist;
	}
    }
    return mrock;
}

int Client::GetDangerousEnemy(double mex, double mey)
{
    int i=0;
    int snap = -1;
    int d;
    
    double dx, dy, dist, ttx, tty, tm;
    double rdist = 0;
    
    for (i = 0; i < maxplayers; ++i)
    {
        if (!fx.player[i].used || 
	   fx.player[i].roomx != fx.player[me].roomx ||
	   fx.player[i].roomy != fx.player[me].roomy ||
	   fx.player[i].team() == fx.player[me].team() ||
           !fx.player[i].onscreen ||
	   fx.player[i].dead)
	   continue;

	ttx = (fx.player[i].lx)+averageLag *fx.player[i].sx;
        tty = (fx.player[i].ly)+averageLag *fx.player[i].sy;
			
	dx = ttx - mex;
	dy = tty - mey;

	tm = (dist / fx.physics.rocket_speed);
	dx += tm * (fx.player[i].sx);
	dy += tm * (fx.player[i].sy);

	d = GetDir(dx, dy);
	
	if((d == 0) || (d==4))
	    dist = fabs(dy);
	else if ((d==2)||(d==6))
	    dist = fabs(dx);
	else if ((d==1)||(d==5)) 
	    dist = sqrt(((dy-dx)*(dy-dx))/2);
	else
	    dist = sqrt(((dy+dx)*(dy+dx))/2);

	if((dist < 2*PLAYER_RADIUS) && ((dist<rdist)|| !rdist)) //was 4
	{
	    if (IsBehindWall(mex, mey, ttx - mex, tty - mey))
		continue;
	    snap = i;
	    rdist = dist;
	}
    }
    return snap;
}

int Client::GetNearestEnemy(double mex, double mey)
{
    int i;
    double dx, dy, ttx, tty, dist;
    double mdist = 0;
    int snap = -1;
    for (i = 0; i < maxplayers; i++)// find nearest
    {
        if (!fx.player[i].used || 
	   fx.player[i].roomx != fx.player[me].roomx ||
	   fx.player[i].roomy != fx.player[me].roomy ||
	   fx.player[i].team() == fx.player[me].team() ||
           !fx.player[i].onscreen ||
	   fx.player[i].dead)
	   continue;
	
	// XXX?
	ttx = (fx.player[i].lx)+averageLag *fx.player[i].sx;
        tty = (fx.player[i].ly)+averageLag *fx.player[i].sy;
			
	dx = ttx - mex;
	dy = tty - mey;
				
	dist = sqrt(dx * dx + dy * dy);

	if(dist<mdist || !mdist)
	{ //if
//	    if(IsBehindWall(mex, mey, dx, dy))
//		continue;
	    mdist = dist;
	    snap = i;
	}
    }
    return snap;
}

int Client::NeedShoot(double mex, double mey)
{
    int i;

    for (i = 0; i < maxplayers; i++)// find nearest
    {
        if (!fx.player[i].used || 
	   fx.player[i].roomx != fx.player[me].roomx ||
	   fx.player[i].roomy != fx.player[me].roomy ||
	   fx.player[i].team() == fx.player[me].team() ||
           !fx.player[i].onscreen ||
	   fx.player[i].dead)
	   continue;
				
	if(IsAimed(mex, mey, i)==2)
	    return 1;
    }
    return 0;
}

int Client::EscapeRocket(double mex, double mey, int mrock)
{
    int data = 0;
    double sdx, sdy;
    data = 32 | 16; //alt & run

    sdx = fx.rock[mrock].x - mex;
    sdy = fx.rock[mrock].y - mey;
	    
    switch(fx.rock[mrock].direction)
    {
	case 0: // r -> d or up
	case 4: // l -> u or d
	    data |= (sdy>0)?1:2;
	    break;
	case 2: // d - > l or r
	case 6: // u -> l or r
	    data |= (sdx>0)?4:8;	
	    break;
	case 1: // rd -> ru | ld  "\"
	case 5: // lu -> ru | ld
	    data |= (sdy > sdx)?1|8:2|4;
	    break;
	case 3: // ld -> rd | lu "/"
	case 7: // ur -> lu | rd
	    data |= (sdy > -sdx)?1|4:2|8;
	    break;
    }
    return data;
}

int Client::Aim(double mex, double mey, int i)
{
    int data = 16; //always run!
    int dir;
    double dx, dy, ttx, tty;

    ttx = (fx.player[i].lx)+averageLag *fx.player[i].sx;
    tty = (fx.player[i].ly)+averageLag *fx.player[i].sy;
			
    dx = ttx - mex;
    dy = tty - mey;

//    dist = sqrt(dx*dx + dy*dy); 
//
//    if(dist <= 1.5 * PLAYER_RADIUS) // no run
//	data &= ~16;

    dir =  IsAimed(mex, mey, i);
    if(dir == 1) // almost aimed
    {
	data |= 32; //strafe
	if (fx.player[me].gundir == 0 ||
	    fx.player[me].gundir == 2 ||
	    fx.player[me].gundir == 4 ||
	    fx.player[me].gundir == 6)
	{ 
	    if(dx>0) 
		data |= 8;
	    else if(dx<0)
		data |= 4;	
	    if(dy>0)
		data |= 2;
	    if(dy<0)
		data |= 1;	
	}
	else if(fabs(dy) > fabs(dx))
	{
	    if(dy <0)
		data |= 1;
	    else
		data |= 2;
	}		
	else
	{
	    if(dx <0)
		data |= 4;
	    else
		data |= 8;    
	}
	return data;
    }	
    // aimed or nor, go to mdir

    if(dir == 2)
	return data;
	
    return MoveTo(mex, mey, dx, dy);
}


int Client::FreeDir(double mex, double mey)
{
    int dir = fx.player[me].gundir;
    int i;    
    int mdir  =0;
    double mdist = 0;
    double dist;
    int d;
    
    for(i=dir-1; i!=dir+2; i++)
    {
	if(i>7)
	    d = i - 8;
	else if(i<0)
	    d = 8 + i;
	else
	    d = i;	
	dist = ScanDir(mex, mey, d);
	if ((dist>mdist) || (!mdist))
	{
	    mdist = dist;
	    mdir = d;
	}
    }
    return mdir;
}

int Client::MoveDir(int dir)
{
    int data = 16; // run
    switch(dir)
    {
    case 0:
	data |= 8; // right
	break;
    case 1:
	data |= 8; //rd
	data |= 2;
	break;
    case 2:
	data |= 2; //d
	break;
    case 3:
	data |= 2; //dl
	data |= 4;
	break;
    case 4:
	data |= 4; // l
	break;
    case 5:
	data |= 4; // lu
	data |= 1;
	break;
    case 6:
	data |= 1; //u 
	break;
    case 7:
	data |= 8;
	data |= 1;
	break;
    }
    return data;
}

int Client::FreeWalk(double mex, double mey)
{
    return MoveDir(FreeDir(mex, mey));
}

int Client::MoveTo(double mex, double mey, double dx, double dy)
{
    int mdir=-1;
    
    if (IsBehindWall(mex, mey, dx, dy))//walking
    {
	mdir = FreeDir(mex, mey);
    }
    else
	mdir = GetDir(dx, dy);

//    if(sqrt(dx * dx + dy*dy)<PLAYER_RADIUS)
//	data &= ~16;
    return MoveDir(mdir);	
}
#if 0
int Client::CarryFlag(double mex, double mey)
{
    int t = fx.player[me].team(); //
    double x=0, y=0;
    double n =0;
    for (int i =0; i<maxplayers; i++)
    {
	if (i == me ||
	    !fx.player[i].used || 
	    fx.player[i].roomx != fx.player[me].roomx ||
	    fx.player[i].roomy != fx.player[me].roomy ||
	    fx.player[i].team() != t ||
            !fx.player[i].onscreen ||
	    fx.player[i].dead)
		continue;
	x += fx.player[i].lx;
	y += fx.player[i].ly;
	n ++;
    }
    if(!n)
	return 0;
    x = x/n;
    y = y/n;
    if(fabs(x-mex)>PLAYER_RADIUS*3 ||
	fabs(y-mey)>PLAYER_RADIUS*3)	
	return MoveTo(mex, mey, x - mex, y - mey);
    return 0;
}
#endif
int Client::GetPowerup(double mex, double mey)
{
    for (int i = 0; i < MAX_PICKUPS; i++)
    {
	if (fx.item[i].kind == Powerup::pup_unused || 
	     fx.item[i].kind == Powerup::pup_respawning ||
             (fx.item[i].px != fx.player[me].roomx || 
	     fx.item[i].py != fx.player[me].roomy))
	     continue;
	return MoveTo(mex, mey, fx.item[i].x - mex, fx.item[i].y - mey);   	     
    }	     
    return 0;
}

int Client::GetFlag(double mex, double mey)
{
    int data = 0;
        // draw any dropped flags (use fx since flags don't move)
    int carry = 0;
    int t;
    double dx;
    double dy;

    carry = HaveFlag();
        
    t = fx.player[me].team(); //
    
    const std::vector<WorldCoords>& tflags = fx.map.tinfo[t].flags;

    int at_base = 0;    

    for (std::vector<Flag>::const_iterator fi = fx.teams[t].flags().begin(); fi != fx.teams[t].flags().end(); ++fi)
    {
	if (fi->position().px != fx.player[me].roomx ||
	    fi->position().py != fx.player[me].roomy)
		continue;
	// found base of this flag

	for (std::vector<WorldCoords>::const_iterator pi = tflags.begin(); pi != tflags.end(); ++pi)
	{
	    if (fx.player[me].roomx != pi->px || fx.player[me].roomy != pi->py)
		continue;
	    if (fi->position().x != pi->x || fi->position().y != pi->y)
		continue;
	    at_base = 1;	
	}
	
	if((!fi->carried()) && (carry || !at_base)) // my flag and i am carrying or no at base
	{
	    dx = fi->position().x - mex;
	    dy = fi->position().y - mey;
	    return MoveTo(mex, mey, dx, dy); // to my flag    
	}
    }
    
    t = (fx.player[me].team())?0:1; // enemy

    for (std::vector<Flag>::const_iterator fi = fx.teams[t].flags().begin(); fi != fx.teams[t].flags().end(); ++fi)
    {    
	if (fi->position().px != fx.player[me].roomx ||
	    fi->position().py != fx.player[me].roomy)
		continue;
	if (!fi->carried() && !carry) // not my and i am not carry
	{
	    dx = fi->position().x - mex;
	    dy = fi->position().y - mey;
	    return MoveTo(mex, mey, dx, dy);    
	}
    }
//    if(carry && (!fx.player[me].routing || fx.player[me].routing ==-1))
//	return CarryFlag(mex, mey);
    return data;
}

void Client::BuildMap()
{
    botPrevFire = false;
    
    int x,y;
    for(x=0;x<fx.map.w;x++)
    {
	for(y=0;y<fx.map.h;y++)
	{
	    fx.map.room[x][y].pass[0]=
		!fx.map.room[x][y].fall_on_wall(S_W/2, 0, PLAYER_RADIUS) ||
		!fx.map.room[x][y].fall_on_wall(S_W/4, 0, PLAYER_RADIUS) ||
		!fx.map.room[x][y].fall_on_wall(S_W-S_W/4, 0, PLAYER_RADIUS);    
	    
	    fx.map.room[x][y].pass[1]=
		!fx.map.room[x][y].fall_on_wall(S_W/2, S_H, PLAYER_RADIUS) || 
	        !fx.map.room[x][y].fall_on_wall(S_W/4, S_H, PLAYER_RADIUS) || 
	        !fx.map.room[x][y].fall_on_wall(S_W-S_W/4, S_H, PLAYER_RADIUS);    
	    
	    fx.map.room[x][y].pass[2]=
		!fx.map.room[x][y].fall_on_wall(0, S_H/2, PLAYER_RADIUS) ||
		!fx.map.room[x][y].fall_on_wall(0, S_H/4, PLAYER_RADIUS) ||
		!fx.map.room[x][y].fall_on_wall(0, S_H - S_H/4, PLAYER_RADIUS);
		
	    fx.map.room[x][y].pass[3]=
		!fx.map.room[x][y].fall_on_wall(S_W, S_H/2, PLAYER_RADIUS) ||
		!fx.map.room[x][y].fall_on_wall(S_W, S_H/4, PLAYER_RADIUS) ||
		!fx.map.room[x][y].fall_on_wall(S_W, S_H - S_H/4, PLAYER_RADIUS);

	    fx.map.room[x][y].route = false;
	    fx.map.room[x][y].label = -1;
#ifdef BOTDEBUG	    
	    fprintf(stderr,"%d %d: %d %d %d %d\n",x, y,fx.map.room[x][y].pass[0],
		    fx.map.room[x][y].pass[1],
		    fx.map.room[x][y].pass[2],
		    fx.map.room[x][y].pass[3]);
#endif		    
		    
	}
	
    }
}

void Client::next_room(int &x, int &y, int i)
{
    switch(i)
    {
    case 0:
    	y --;
	if(y<0)
	    y = fx.map.h-1;
	break;
    case 1:
	y ++;
	if(y>=fx.map.h)
	    y = 0;
	break;
    case 2:
	x --;
	if(x<0)
	    x = fx.map.w-1;
	break;
    case 3:
	x ++;
	if(x>=fx.map.w)
	    x = fx.map.w-1;
	break;
    }
}

int Client::label_room(int x, int y, int label)
{
    int i;
    int n=0;
    int nx, ny;
    if(fx.map.room[x][y].label != label) // not our label
	return 0;
	
    for(i=0; i<4; i++)
    {
	if(!fx.map.room[x][y].pass[i]) // looking for nearest room
	    continue;
	nx = x;
	ny = y;    
	next_room(nx, ny, i);
	if(fx.map.room[nx][ny].label != -1) // already labeled
	    continue;
#ifdef BOTDEBUG
	fprintf(stderr,"label_room(%d, %d, %d) -> %d %d\n", x, y, label, nx, ny);
#endif	
	fx.map.room[nx][ny].label = label + 1;
	n++;
    }
    return n;
}

int Client::route_room(int &x, int &y)
{
    int i;
    int nx, ny;
    int n = 0;
    int routes[4];
    int route_nr = 0;
    int label = fx.map.room[x][y].label;
    
    if(label == -1) // not labeled
	return 0;

    for(i=0; (i<4) && label; i++)
    {
//	if(!fx.map.room[x][y].pass[i])
//	    continue;
	nx = x;
	ny = y;    
	next_room(nx, ny, i);
	if(fx.map.room[nx][ny].label != label-1)
	    continue;
	if(!fx.map.room[nx][ny].pass[i^1])
	    continue;
	routes[route_nr++] = i;    
//	x = nx;
///	y = ny;
	n ++;
//	break;     
    }
    if(!n)
	return 0;
    next_room(x, y, routes[rand()%route_nr]);
    fx.map.room[x][y].route = true;    
    return n;
}

int Client::BuildRouteTable()
{
    int mex = fx.player[me].roomx;
    int mey = fx.player[me].roomy;
    int w = fx.map.w;
    int h = fx.map.h;
    int label = 0;    
    int x, y;

    for(x=0;x<w;x++)
    {
	for(y=0;y<h;y++)
	{
	    fx.map.room[x][y].label = -1;
	    fx.map.room[x][y].route = false;
	}
    }

    fx.map.room[mex][mey].label = label; // start point
    while(1)
    {
	int i = 0;
	for(x=0;x<w;x++)
	{
	    for(y=0;y<h;y++)
	    {
		i += label_room(x, y, label);
	    }
	}
	if(!i) // all alabeled
	    break;
	label ++;    
    }
#ifdef BOTDEBUG
    fprintf(stderr,"BuildRoute table from %d %d\n", mex, mey);
    for(y=0;y<h;y++)
    {
	for(x=0;x<w;x++)
	{
	    fprintf(stderr,"%02d ", fx.map.room[x][y].label);
	}
	fprintf(stderr,"\n");
    }
#endif    
    return label;
}

int Client::BuildRoute(int tox, int toy)
{
    int i=0;
    int x, y;
    int mex, mey;
#ifdef BOTDEBUG
    static int tox_old=-1, toy_old=-1;
    if((tox != tox_old) || (toy!= toy_old))
    {
	fprintf(stderr,"Buid route to %d %d\n", tox, toy);
	tox_old = tox;
	toy_old = toy;
    }
#endif    
    for(x=0;x<fx.map.w;x++) // clear route
    {
	for(y=0;y<fx.map.h;y++)
	{
	    fx.map.room[x][y].route = false;
	}
    }
    
    if(fx.map.room[tox][toy].label == -1)
	return -1;
	
    mex = fx.player[me].roomx;
    mey = fx.player[me].roomy;
    
    if(fx.map.room[mex][mey].label == -1)
	return -1;
	
    if (fx.map.room[mex][mey].label > fx.map.room[tox][tox].label)
	return -1;

    fx.map.room[tox][toy].route = true;	
    
    while(route_room(tox, toy))
	i++;

    return i;	
}


int Client::DoRoute(double melx, double mely) {
    int mex = fx.player[me].roomx;
    int mey = fx.player[me].roomy;
    int passes[4];
    int n_passes = 0;
    int label;

    double tox, toy;

    label = fx.map.room[mex][mey].label;

    if (label == -1)
        return 0;

    for (int i = 0; i < 4; i++) {
    	if (!fx.map.room[mex][mey].pass[i])
    	    continue;
        int x = mex;
        int y = mey;
        next_room(x, y, i);
        if (fx.map.room[x][y].route && (fx.map.room[x][y].label == (label + 1)))
            passes[n_passes++] = i;
    }

    if (!n_passes)
        return 0; // no need to go
#ifdef BOTDEBUG
//    fprintf(stderr,"i am @ (%d %d) -> (%d %d)\n", fx.player[me].roomx, fx.player[me].roomy, mex, mey);
#endif
    const int dir = passes[rand() % n_passes]; // chose random (from equal) pass
    switch (dir) {
    /*break;*/ case 0:
            tox = S_W/2;
            toy = 0 - PLAYER_RADIUS;
        break; case 1:
            tox = S_W/2;
            toy = S_H + PLAYER_RADIUS;
        break; case 2:
            tox = 0 - PLAYER_RADIUS;
            toy = S_H/2;
        break; case 3:
            tox = S_W + PLAYER_RADIUS;
            toy = S_H/2;
        break; default:
            tox = toy = 0;
    }
    return MoveTo(melx, mely, tox - fx.player[me].lx, toy - fx.player[me].ly);	
}


// en - nearest enemy 
// me - nearest my team
// ef - nearest enemy flag (no carry)
// mf - nearest my flag (no carry)
// efc - nearest enemy flag (carry)
// mfc - nearest my flag (carry) 


// all flags at base
// ef ? en

// some our flags taken -> mfc
// ef ? mfc

// some enemy flags taken ->efc
// ef ? efc 

// some enemy and my flags taken
// ef ? mfc ? efc ?

// if i am carry 
// mf else mfc

// if i am not carry
// ef ? mfc ? efc -> if all fail(no flags) -> en
// if i am carry
// mf ? me -> if all fail -> mfc

//  int Client::TargetRoute(int ef, int efc, int mf, int mfc, int wf, int wfc, int en, int fr)

int Client::RouteLogic() // NEED rewrite
{
    int i = HaveFlag();
    if(!i)
    {
	i = TargetRoute(1, 1, 0,
			0, 1, 1,
			1, 1, 0,
			0, 0,
			0, 0, 0); // try any flags that dropped or at bases
	if (!i)
	    i = TargetRoute(
			0, 0, 1,
			0, 0, 0,
			0, 0, 0,
			1, 0,
			1, 0, 0  // ok enemy flags that we hold, or enemy, or enemy base
			);		
#if 0			
	if (!i)
	    i = TargetRoute(
			0, 0, 0,
			0, 0, 0,
			0, 0, 0,
			0, 0,
			1, 0, 0 // ok, go to enemy base
			);		
#endif			
    }
    else // i am flagman ;)
    {
	i = TargetRoute(0, 0, 0,
			1, 1, 0,
			0, 0, 0,
			0, 0,
			0, 0, 0); // my flag at base or dropped
	if (!i)
	    i = TargetRoute(
			0, 0, 0,	
    		    	0, 0, 0,
			0, 0, 0,
			0, 0,
			0, 1, 0  // ok, to our base
			);		
    }
#ifdef BOTDEBUG
//    fprintf(stderr,"RouteLogic: %d\n", i);
#endif    
    fx.player[me].routing = i;
    return i;		
}

int Client::IsMassive()
{
    int i;
    double x=0,y=0;
    int n=0;
    for (i=0; i<maxplayers;  i++)
    {
	if(!fx.player[i].used || (fx.player[i].team() != fx.player[me].team()) || fx.player[i].dead)
	    continue;
	
	if ((fx.player[i].roomx != fx.player[me].roomx) ||
	    (fx.player[i].roomy != fx.player[me].roomy))
	    continue; //already here
        x = x + fx.player[i].lx;
	y = y + fx.player[i].ly;	        
	n ++;
    }
    
    if (n>1)
    {
	x = x/n;
	y = y/n;
    }
    else
	return 0;
    double dx = x - fx.player[me].lx;
    double dy = y - fx.player[me].ly;	
    double dist = sqrt(dx*dx + dy*dy);	
    if (dist<=(PLAYER_RADIUS *2))
	return 1;
    else
	return 0;	
}	

int Client::Route(double mex, double mey)
{
    int x,y;

    x = fx.player[me].roomx;
    y = fx.player[me].roomy;

    if((fx.map.room[x][y].label != 0)||(!fx.map.room[x][y].route) || ((fx.frame - route_frame)>ROUTE_FADEOUT))
    {
	route_frame = fx.frame;
	if(!BuildRouteTable())
	    return 0;
	if(!RouteLogic())
	{
	    fx.map.room[x][y].route = false;
	    return 0;
	}
    }
//    if(!RouteLogic())
//	return 0;
    return DoRoute(mex, mey);	
}

int Client::HaveFlag()
{
    int t = (fx.player[me].team())?0:1;
    
    // look for enemy flags in team 

    for (std::vector<Flag>::const_iterator fi = fx.teams[t].flags().begin(); fi != fx.teams[t].flags().end(); ++fi)
    {    
	if(fi->carried() && (fi->carrier() == me))
	{
	    return 1;
	}
    }
    // looking for wild flags
    for (std::vector<Flag>::const_iterator fi = fx.wild_flags.begin(); fi != fx.wild_flags.end(); ++fi)
    {
	if(fi->carried() && (fi->carrier() == me))
	{
	    return 1;
	}
    }
    return 0;
}

int Client::TargetNearestBase(int &m_label, int &x, int &y, int team)
{
    const std::vector<WorldCoords>& tflags = fx.map.tinfo[team].flags;
    int label = 0;

    for (std::vector<WorldCoords>::const_iterator pi = tflags.begin();(pi != tflags.end()); ++pi)
    {
	if((pi->px == fx.player[me].roomx) && (pi->py == fx.player[me].roomy))
	    continue; //already here
	label = fx.map.room[pi->px][pi->py].label;
	if(label == -1)
	    continue;
	if((label<m_label) || (m_label == -1) )
	{
	    m_label = label;
	    x = pi->px;
	    y = pi->py;
	}
    }
    if (label == -1)
	return -1;
    if (!label)
	return 0;
    return m_label;     	 	
}

int Client::TargetNearestTeam(int &m_label, int &x, int &y, int team)
{
    int i;
    int label = 0;

    // looking for soldiers
    
    int enemy = (fx.player[me].team() == team)?0:1;
    
    for (i=0; i<maxplayers;  i++)
    {
	if((i == me) || !fx.player[i].used || (fx.player[i].team() != team) || fx.player[i].dead)
	    continue;
	
	if ((fx.player[i].roomx == fx.player[me].roomx) &&
	    (fx.player[i].roomy == fx.player[me].roomy))
	    continue; //already here
	        
	label = fx.map.room[fx.player[i].roomx][fx.player[i].roomy].label;
	if (label == -1)
	    continue;

	if (enemy) // if enemy, check fadeout
	{
	    ClientPlayer &pl = fx.player[i];
	    if (!pl.used || (pl.roomx < 0) || (pl.roomy < 0) || 
		(pl.roomx >= fx.map.w) || (pl.roomy >= fx.map.h) ||
		!(pl.posUpdated > fx.frame - FADEOUT)) // TODO fadeout 
		continue; // old data	
	}
	    
	if((label<m_label) || (m_label == -1)) 
	{
	    m_label = label;
	    x = fx.player[i].roomx;
	    y = fx.player[i].roomy;
	}
    }

    if(label == -1)
	return -1; // not builded
    if (!label)
	return 0; // no such target	
    return m_label; 
}

int Client::TargetNearestFlag(int &m_label, int &x, int &y, int team, int state)
{
    // state - 0 - at base, 1 - no at base/droped, 2 - no at base/carry
    int at_base = 0;
    int on_base = (!state)?1:0;
    int label = 0;
    
    int enemy = (fx.player[me].team() == team)?0:1;
    
    const std::vector<WorldCoords>& tflags = fx.map.tinfo[team].flags;
//    std::vector<Flag>& flags;
    
    const std::vector<Flag>& flags = (team!=2)?fx.teams[team].flags():fx.wild_flags;//.begin();

    for (std::vector<Flag>::const_iterator fi=flags.begin();fi != flags.end(); ++fi)
    {    
	if ((fi->position().px == fx.player[me].roomx) &&
	    (fi->position().py == fx.player[me].roomy))
	    continue;
	    
	if ((state == 2) && !fi->carried())
	    continue;
	if ((state == 1) &&  fi->carried()) // dropped wanted but it is carried
	    continue;    
#if 1
	if (fi->carried() && !enemy) // our flag carried, is there near our forces
	{
	    ClientPlayer &pl = fx.player[fi->carrier()];
	    if (!pl.used || (pl.roomx < 0) || (pl.roomy < 0) || 
		(pl.roomx >= fx.map.w) || (pl.roomy >= fx.map.h) ||
		!(pl.posUpdated > fx.frame - FADEOUT)) // TODO fadeout 
		continue; // old data	
	}
#endif	
#if 1
	at_base = 0;
	// at base or not?    
	for (std::vector<WorldCoords>::const_iterator pi = tflags.begin();(pi != tflags.end()); ++pi)
	{
	    if (fi->carried())
		break;
		
	    if((pi->px != fi->position().px) ||
		(pi->py != fi->position().py))
		continue;
	    
	    at_base = ((pi->x == fi->position().x) &&
		(pi->y == fi->position().y));
	    if (at_base)
		break;
	}

	if (at_base != on_base)
	    continue;
#endif
	// this flag is ok
	label = fx.map.room[fi->position().px][fi->position().py].label;
	if (label == -1)
	    continue;
	if((label<m_label) ||( m_label == -1))
	{
	    m_label = label;

	    if (fi->carried())
	    {
		ClientPlayer &pl = fx.player[fi->carrier()];
		x = pl.roomx;
		y = pl.roomy;
	    }
	    else
	    {    
		x = fi->position().px;
		y = fi->position().py;
	    }
	}
    }
    if(label == -1)
	return label; // not builded
    if (!label)
	return 0; // no such target	
    return m_label; // build route to nearest target
}

int Client::TargetRoute(int efb, int efd, int efc, 
			int mfb, int mfd, int mfc, 
			int wfb, int wfd, int wfc, 
			int en,  int fr, 
			int eb,  int fb, int wb)
{
    int m_label = -1;
    int x, y;
    int t = fx.player[me].team();
    int et = (t)?0:1;
    int n = 0;
    if (efb)
	n+= TargetNearestFlag(m_label, x, y, et, 0);

    if (efd)
	n+= TargetNearestFlag(m_label, x, y, et, 1);

    if (efc)
	n+= TargetNearestFlag(m_label, x, y, et, 2);

    if (mfb)
	n+= TargetNearestFlag(m_label, x, y, t, 0);

    if (mfd)
	n+= TargetNearestFlag(m_label, x, y, t, 1);

    if (mfc)
	n+= TargetNearestFlag(m_label, x, y, t, 2);

    if (wfb)
	n+= TargetNearestFlag(m_label, x, y, 2, 0);

    if (wfd)
	n+= TargetNearestFlag(m_label, x, y, 2, 1);

    if (wfc)
	n+= TargetNearestFlag(m_label, x, y, 2, 2);
		
    if (en)
	n+= TargetNearestTeam(m_label, x, y, et);

    if (fr)
	n+= TargetNearestTeam(m_label, x, y, t);

    if (eb)
	n+= TargetNearestBase(m_label, x, y, et);
    if (fb)
	n+= TargetNearestBase(m_label, x, y, t);
    if (wb)
	n+= TargetNearestBase(m_label, x, y, 2);

    if (!n)
	return 0;
    if (n<0)
	return -1;
		
    return BuildRoute(x, y);	
}

void Client::Robot(ClientControls &ctrl)
{
    char lebuf[16]; int count = 0;
    double mex, mey, dx, dy, ttx, tty;
//    bool send_now = false;
    static int last_seen = -1;
    int i;
//    MutexLock ml(frameMutex);

    const bool hide_map=  !map_ready || gameover_plaque != NEXTMAP_NONE || fx.skipped || me < 0 || me >= maxplayers;

    if(hide_map)
    {
//	fprintf(stderr,"Map not ready\n");	
	return;
    }

//    fprintf(stderr,"Robot logic\n");	
    ctrl.data = 0;
    
    mex = (fx.player[me].lx)+averageLag*fx.player[me].sx;
    mey = (fx.player[me].ly)+averageLag*fx.player[me].sy;
#if 1

    if (NeedShoot(mex, mey))
    {
	if(!botPrevFire)//if not fired
	{
    	    writeByte(lebuf, count, data_fire_on);
    	    client->send_message(lebuf, count);
	    botPrevFire = true;
//	    send_now = true;
	    send_frame(false, true);
	}
    }
    else if(botPrevFire)
    {
	writeByte(lebuf, count, data_fire_off);
        client->send_message(lebuf, count);
	botPrevFire=false;
//	send_now = true;
	send_frame(false, true);
    }
    i = last_seen; // lost target
    if ( (i == -1) || !fx.player[i].used || 
	   fx.player[i].roomx != fx.player[me].roomx ||
	   fx.player[i].roomy != fx.player[me].roomy ||
	   fx.player[i].team() == fx.player[me].team() ||
           !fx.player[i].onscreen ||
	   fx.player[i].dead)
	   last_seen = -1;

    if(!IsMassive())
        i = GetDangerousRocket(mex, mey);
    else
	i = -1;	
    if (i!=-1)
    {
	ctrl.data |= EscapeRocket(mex, mey, i);	
	if ( last_seen == -1)
	    last_seen = fx.rock[i].owner; 
//	send_frame(false, send_now);
	return;
    }

#endif    
    if ( last_seen == -1 )
    {
//	fprintf(stderr,"Get flag\n");
	i = GetFlag(mex, mey);
	if(i) // if any
	{
	    ctrl.data |= i;
//	    send_frame(false, send_now);
	    return;
	}
    }
#if 1    
    if ((last_seen != -1) || HaveFlag()) // already locked on someone or have flag
    {
	i = GetDangerousEnemy(mex, mey); // more dangerous???
	if (i!= -1)
	    last_seen = i;
    }
    else if(!HaveFlag())// get someone
    {
	
	i = GetNearestEnemy(mex, mey);
	last_seen = i;
    }
    
    i = last_seen;

    if (i!= -1)
    {
	ttx = (fx.player[i].lx)+averageLag *fx.player[i].sx;
        tty = (fx.player[i].ly)+averageLag *fx.player[i].sy;
	
	dx = ttx - mex;
	dy = tty - mey;
//	if(IsBehindWall(mex, mey, dx, dy))
//	{
//	    last_seen = -1;    
//	}
//s	else
	{
//	    fprintf(stderr,"Aim\n");
	    ctrl.data |= Aim(mex, mey, i);
//	    send_frame(false, send_now);
	    return;
	}
    }
#endif
    // ok, free tour ;)
    i = GetPowerup(mex, mey);
    if(i)
    { 
	ctrl.data |= i; return;
    }
    i = Route(mex, mey);
    if (i)
    {
	ctrl.data |= i;
	return;
    }
    ctrl.data |= FreeWalk(mex, mey);
    ctrl.data &= ~16;
//    else	
//	ctrl.data |= Route(mex, mey);
//    send_frame(false, send_now);
}

#undef fx
#undef averageLag
//send the client's frame to server (keypresses)
#endif
