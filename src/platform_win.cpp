/*
 *  platform_win.cpp
 *
 *  Copyright (C) 2004 - Niko Ritari
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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <direct.h>

#include "platform.h"

int platMkdir(const char* path) {
	return mkdir(path);
}

int platStricmp(const char* s1, const char* s2) {
	return stricmp(s1, s2);
}

int platVsnprintf(char* buf, size_t count, const char* fmt, va_list arg) {
	return _vsnprintf(buf, count, fmt, arg);
}
