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

#include "incalleg.h"
#include "platform.h"
#include "leetnet/sleep.h"  // for MS_SLEEP

using std::string;

int platMkdir(const char* path) {
    return mkdir(path, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
}

int platStricmp(const char* s1, const char* s2) {
    return strcasecmp(s1, s2);
}

int platVsnprintf(char* buf, size_t count, const char* fmt, va_list arg) {
    return vsnprintf(buf, count, fmt, arg);
}

void platMessageBox(const string& caption, const string& msg, bool blocking) {
    // The dialog tools may bug totally when given characters in wrong encoding.
    // At least UTF-8 gdialog can print out "All updates are complete." and completely disregard the given message.
    // We have no way to know which encoding they expect, so convert texts to 7-bit ASCII.
    char* capBuf = new char[caption.length() + 1];
    char* msgBuf = new char[    msg.length() + 1];
    #ifndef SDL_DEDICATED_SERVER
    const char* captionConv = uconvert(caption.c_str(), U_CURRENT, capBuf, U_ASCII_CP, caption.length() + 1);
    const char*     msgConv = uconvert(    msg.c_str(), U_CURRENT, msgBuf, U_ASCII_CP,     msg.length() + 1);
    #else
    char* captionConv = capBuf;
    char* msgConv = msgBuf;
    #endif

    #ifdef DEDICATED_SERVER_ONLY
    (void)blocking;
    #else
    static const int nFuncs = 3;
    static const char* func[nFuncs] = { "gdialog", "kdialog", "xmessage" };
    static int funci = 0;   // updated to whatever works; nFuncs means nothing works
    while (funci != nFuncs) {
        int lFunci = funci; // local copy as a thread safety measure
        pid_t pid = fork();
        if (pid == 0) { // child
            if (lFunci == 2)    // xmessage
                execlp(func[lFunci], func[lFunci], captionConv, ":", msgConv, 0);
            else
                execlp(func[lFunci], func[lFunci], "--title", captionConv, "--msgbox", msgConv, 0);
            _exit(EXIT_FAILURE);
        }
        if (pid == -1) {
            funci = nFuncs;
            break;
        }
        // parent
        if (blocking) {
            int status;
            if (waitpid(pid, &status, 0) == -1) {   // shouldn't really happen
                funci = nFuncs;
                break;
            }
            if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS) {
                delete[] capBuf;
                delete[] msgBuf;
                return;
            }
        }
        else {
            MS_SLEEP(200);  // this is a total hack - but how else to detect the call failing?
            int status;
            const int ret = waitpid(pid, &status, WNOHANG);
            if (ret == -1) {
                funci = nFuncs;
                break;
            }
            if (ret == 0 || (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS)) { // ret == 0 means the child is still running, ie. probably showing the message as it should
                delete[] capBuf;
                delete[] msgBuf;
                return;
            }
        }
        ++lFunci;
        funci = lFunci;
    }
    #endif
    // execution of any dialog failed -> print to console
    fprintf(stderr, "%s: %s\n", captionConv, msgConv);
    delete[] capBuf;
    delete[] msgBuf;
}
