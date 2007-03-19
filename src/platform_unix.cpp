/*
 *  platform_unix.cpp
 *
 *  Copyright (C) 2004, 2006 - Niko Ritari
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

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#include "incalleg.h"
#include "commont.h"
#include "platform.h"
#include "timer.h"

using std::string;

int platStricmp(const char* s1, const char* s2) {
    return strcasecmp(s1, s2);
}

int platVsnprintf(char* buf, size_t count, const char* fmt, va_list arg) {
    return vsnprintf(buf, count, fmt, arg);
}

class LinuxTimer : public SystemTimer {
public:
    double read() {
        timeval tv;
        if (gettimeofday(&tv, 0) != 0)
            nAssert(0);
        return tv.tv_sec + double(tv.tv_usec) * .000001;
    }
};

void platSleep(unsigned ms) {
    /* Here is an option between select and nanosleep. On my Linux system, when idle, both are very predictable in the time they sleep:
     * - nanosleep sleeps about 1.96 ms too long
     * - select sleeps about .05 ms too short
     * - both stop sleeping on a received signal, but with select it is harder to continue sleeping properly so it is not done at all in the code here
     * In Outgun the given time is neither a minimum nor a maximum but just a guideline, so we pick the closer result and don't care about possibly returning prematurely.
     */
    struct timeval t;
    t.tv_sec = ms / 1000;
    t.tv_usec = (ms % 1000) * 1000;
    select(0, 0, 0, 0, &t);
    /*
    struct timespec t;
    t.tv_sec = ms / 1000;
    t.tv_nsec = (ms % 1000) * 1000000;
    while (nanosleep(&t, &t) != 0 && errno == EINTR);
    */
}

class LinuxFileFinder : public FileFinder {
    string path, extension;
    bool directories;

    DIR* dir;
    struct dirent* ent;

    void advance() {
        for (;;) {
            ent = readdir(dir);
            if (!ent)
                return;
            const unsigned len = strlen(ent->d_name);
            if (len < extension.length() || extension != &ent->d_name[len - extension.length()] || !strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
                continue;
            struct stat s;
            if (stat((path + directory_separator + ent->d_name).c_str(), &s) != 0)
                continue;
            if (S_ISDIR(s.st_mode) != directories)
                continue;
            return;
        }
    }

public:
    LinuxFileFinder(const string& path_, const string& extension_, bool directories_) : path(path_), extension(extension_), directories(directories_) {
        dir = opendir(path.c_str());
        if (dir)
            advance();
        else
            ent = 0;
    }

    ~LinuxFileFinder() {
        if (dir)
            closedir(dir);
    }

    bool hasNext() const { return ent; }

    string next() {
        string name = ent->d_name;
        advance();
        return name;
    }
};

FileFinder* platMakeFileFinder(const string& path, const string& extension, bool directories) {
    return new LinuxFileFinder(path, extension, directories);
}

int platMkdir(const string& path) {
    return mkdir(path.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
}

bool platIsFile(const string& name) {
    struct stat s;
    if (stat(name.c_str(), &s) != 0)
        return false;
    return !S_ISDIR(s.st_mode);
}

bool platIsDirectory(const string& name) {
    struct stat s;
    if (stat(name.c_str(), &s) != 0)
        return false;
    return S_ISDIR(s.st_mode);
}

void platInit() {
    directory_separator = '/';
    g_systemTimer = new LinuxTimer();
}

static void closeSignalHandler(int) {
    if (g_exitFlag)
        _exit(EXIT_FAILURE);
    g_exitFlag = true;
}

void platInitAfterAllegro() {
    // initialize wheregamedir
    static const int bufSize = 1000;
    char buf[bufSize];
    int len = readlink("/proc/self/exe", buf, bufSize);
    while (len > 0 && buf[len - 1] != '/')
        --len;
    if (len > 0)
        wheregamedir.assign(buf, len);
    else { // it's not working as expected, give up
        #ifndef DEDICATED_SERVER_ONLY
        // we can use Allegro, which is nice and reliable
        get_executable_name(buf, bufSize);
        replace_filename(buf, buf, "", bufSize);
        wheregamedir = buf;
        #else
        wheregamedir = "./";
        #endif
    }

    // grab basic closing signals from Allegro (this is not very nice but Allegro makes a mess when receiving a signal; besides, we just want to know a signal was received, not terminate immediately)
    signal(SIGHUP, closeSignalHandler);
    signal(SIGINT, closeSignalHandler);
    signal(SIGTERM, closeSignalHandler);

    signal(SIGPIPE, SIG_IGN); // we don't want closed (TCP) sockets to kill us
}

void platUninit() {
    delete g_systemTimer;
    g_systemTimer = 0;
}

#ifndef DEDICATED_SERVER_ONLY

void platMessageBox(const string& caption, const string& msg, bool blocking) {
    // The dialog tools may bug totally when given characters in wrong encoding.
    // At least UTF-8 gdialog can print out "All updates are complete." and completely disregard the given message. (some versions of it did that with normal input like '&' too, so it's no longer used at all)
    // We have no way to know which encoding they expect, so convert texts to 7-bit ASCII.

    char* capBuf = new char[caption.length() + 1];
    char* msgBuf = new char[    msg.length() + 1];

    const char* captionConv = uconvert(caption.c_str(), U_CURRENT, capBuf, U_ASCII_CP, caption.length() + 1);
    const char*     msgConv = uconvert(    msg.c_str(), U_CURRENT, msgBuf, U_ASCII_CP,     msg.length() + 1);

    static const int nFuncs = 2;
    static const char* func[nFuncs] = { "kdialog", "xmessage" };
    static const int iXmessage = 1;
    static int funci = 0;   // updated to whatever works; nFuncs means nothing works
    while (funci != nFuncs) {
        int lFunci = funci; // local copy as a thread safety measure
        pid_t pid = fork();
        if (pid == 0) { // child
            if (lFunci == iXmessage)    // xmessage
                execlp(func[lFunci], func[lFunci], captionConv, ":", msgConv, (const char*)0);
            else
                execlp(func[lFunci], func[lFunci], "--title", captionConv, "--msgbox", msgConv, (const char*)0);
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
            platSleep(200); // this is a total hack - but how else to detect the call failing?
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
    // execution of any dialog failed -> print to console
    fprintf(stderr, "%s: %s\n", captionConv, msgConv);
    delete[] capBuf;
    delete[] msgBuf;
}

#endif // DEDICATED_SERVER_ONLY
