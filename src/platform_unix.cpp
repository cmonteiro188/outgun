/*
 *  platform_unix.cpp
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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "platform.h"

int platMkdir(const char* path) {
    return mkdir(path, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
}

int platStricmp(const char* s1, const char* s2) {
    return strcasecmp(s1, s2);
}

int platVsnprintf(char* buf, size_t count, const char* fmt, va_list arg) {
    return vsnprintf(buf, count, fmt, arg);
}

void messageBox(const char* caption, const char* fmt, ...) {
    const int bufSize = 16384;
    char buf[bufSize];
    va_list argptr;
    va_start(argptr, fmt);
    platVsnprintf(buf, bufSize, fmt, argptr);
    va_end(argptr);
    static const int nFuncs = 4;
    static const char* func[nFuncs] = { "xdialog", "gdialog", "kdialog", "xmessage" };
    static int funci = 0;   // updated to whatever works; nFuncs means nothing works
    while (funci != nFuncs) {
        int lFunci = funci; // local copy as a thread safety measure
        pid_t pid = fork();
        if (pid == 0) { // child
            if (lFunci == 3)    // xmessage
                execlp(func[lFunci], func[lFunci], caption, ":", buf, 0);
            else
                execlp(func[lFunci], func[lFunci], "--title", caption, "--msgbox", buf, 0);
            _exit(EXIT_FAILURE);
        }
        if (pid == -1) {
            funci = nFuncs;
            break;
        }
        // parent
        int status;
        if (waitpid(pid, &status, 0) == -1) {   // shouldn't really happen
            funci = nFuncs;
            break;
        }
        if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS)
            return;
        ++lFunci;
        funci = lFunci;
    }
    // execution of any dialog failed -> print to console
    fprintf(stderr, "%s: %s\n", caption, buf);
}
