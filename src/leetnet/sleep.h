/*
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
 *  Copyright (C) 2002 - Fabio Reis Cecin <fcecin@inf.ufrgs.br>
 *  Modified by Niko Ritari 2003
 */

/*

	cross-platform "sleep" - patch for windows

*/

#ifndef _sleep_h_
#define _sleep_h_

// sleep ( secs )
//
#if defined ALLEGRO_WINDOWS || WIN32 || WIN64
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#define MS_SLEEP(x) Sleep(x)
#else
#include "time.h"
#define MS_SLEEP(x) { timespec ts1,ts2; ts1.tv_sec = (x)/1000; ts1.tv_nsec = ((x)%1000)*1000000; nanosleep(&ts1,&ts2); }
#endif


#endif // _sleep_h_

