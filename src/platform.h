/*
 *  platform.h
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

#ifndef PLATFORM_H_INC
#define PLATFORM_H_INC

#include <cstdarg>
#include <string>

#include "utility.h"

int platMkdir(const char* path);
int platStricmp(const char* s1, const char* s2);
int platVsnprintf(char* buf, size_t count, const char* fmt, va_list arg);
void platMessageBox(const std::string& caption, const std::string& text, bool blocking); // blocking may not be controllable

inline int platSnprintf(char* buf, size_t count, const char* fmt, ...) PRINTF_FORMAT(3, 4);

inline int platSnprintf(char* buf, size_t count, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = platVsnprintf(buf, count, fmt, args);
    va_end(args);
    return ret;
}

#endif
