/*
 *  nassert.cpp
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

#include <cstdio>
#include <cstdarg>
#include <cstdlib>  // exit
#include <ctime>
#include "incalleg.h"

#ifndef DISABLE_ENHANCED_NASSERT
#include "commont.h"
#include "platform.h"
#include "utility.h"
#endif

#include "nassert.h"

unsigned long* stackGuardHackPtr;

void stackDump(FILE* dst) { // makes heavy assumptions about processor architecture wrt stack! Should work fine on any x86 platform.
    unsigned long unused;
    for (unsigned long* stackPtr = (&unused) + 1; *stackPtr != STACK_GUARD; ++stackPtr) {
        unsigned long value = *stackPtr;
        fwrite(&stackPtr, sizeof(stackPtr), 1, dst);
        fwrite(&value, sizeof(value), 1, dst);
    }
}

void nasprintf(const char* expr, ...) {
    // display to console (should be safest, but rarely visible on Windows)
    va_list argptr;
    va_start(argptr, expr);
    vfprintf(stderr, expr, argptr);
    va_end(argptr);
    #ifndef DISABLE_ENHANCED_NASSERT
    // save to assert.log
    FILE* asfile = fopen((wheregamedir + "log" + directory_separator + "assert.log").c_str(), "at");
    if (asfile) {
        fprintf(asfile, "%s  %s  ", date_and_time().c_str(), GAME_SHORT_VERSION);
        va_start(argptr, expr);
        vfprintf(asfile, expr, argptr);
        va_end(argptr);
        fclose(asfile);
        FILE* stdump = fopen((wheregamedir + "log" + directory_separator + "stackdump.bin").c_str(), "wb");
        if (stdump) {
            stackDump(stdump);
            fclose(stdump);
        }
    }
    // display using messageBox
    va_start(argptr, expr);
    char buf[10000];
    platVsnprintf(buf, 10000, expr, argptr);
    va_end(argptr);
    messageBox("Internal error", "%s\nThis results from a bug in Outgun. To help us fix it, please send assert.log and stackdump.bin from the log directory and describe what you were doing to outgun@mbnet.fi", buf);
    #endif // DISABLE_ENHANCED_NASSERT
}

#define ARGP(num) const char* name##num, int val##num

void nAssertFail(const char* expr, const char* file, int line) {
    nasprintf("Assertion failed: %s, file %s, line %d\n", expr, file, line);
    exit(-1);
}

void nAssertFail(const char* expr, ARGP(1), const char* file, int line) {
    nasprintf("Assertion failed: %s (%s==%d), file %s, line %d\n", expr, name1, val1, file, line);
    exit(-1);
}

void nAssertFail(const char* expr, ARGP(1), ARGP(2), const char* file, int line) {
    nasprintf("Assertion failed: %s (%s==%d, %s==%d), file %s, line %d\n",
                                                expr, name1, val1, name2, val2, file, line);
    exit(-1);
}

void nAssertFail(const char* expr, ARGP(1), ARGP(2), ARGP(3), const char* file, int line) {
    nasprintf("Assertion failed: %s (%s==%d, %s==%d, %s==%d), file %s, line %d\n",
                                    expr, name1, val1, name2, val2, name3, val3, file, line);
    exit(-1);
}

void nAssertFail(const char* expr, ARGP(1), ARGP(2), ARGP(3), ARGP(4), const char* file, int line) {
    nasprintf("Assertion failed: %s (%s==%d, %s==%d, %s==%d, %s==%d), file %s, line %d\n",
                                    expr, name1, val1, name2, val2, name3, val3, name4, val4, file, line);
    exit(-1);
}

#undef ARGP

