/*
 *  globals.cpp
 *
 *  Copyright (C) 2002 - Fabio Reis Cecin
 *  Copyright (C) 2003, 2004 - Niko Ritari
 *  Copyright (C) 2003, 2004 - Jani Rivinoja
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

#include "commont.h"
#include "mutex.h"

// put here only those globals that don't have a module they naturally belong to; also keep globals to be eliminated in commont.cpp

volatile unsigned long server_speed_counter = 0;    // 10 Hz (100 ms) server frame counter
volatile unsigned long time_counter = 0;    // 200 Hz (5 ms) counter used by get_time() and for client frame timing

char directory_separator;
std::string wheregamedir;

MutexHolder nlOpenMutex;

bool g_leetnetLog = true;
bool g_leetnetDataLog = true;
