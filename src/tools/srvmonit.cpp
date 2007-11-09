/*
 *  srvmonit.cpp
 *
 *  Copyright (C) 2003, 2004, 2007 - Niko Ritari
 *  Copyright (C) 2007 - Jani Rivinoja
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

#include <string>
#include <cstdio>
#include <ctime>

#include "../incalleg.h"
#include "../admshell.h"
#include "../nassert.h"
#include "../network.h"
#include "../platform.h"
#include "../utility.h"

// platform dependent code for kbhit, getch and Sleep
#ifdef WIN32

#include <windows.h>    // Sleep
#include <conio.h>  // kbhit, getch
void initKeyboard() { }
void resetKeyboard() { }

#else   // WIN32

#include <unistd.h> // usleep
void Sleep(int msec) { usleep(msec * 1000); }

#include <sys/termios.h>
#include <sys/time.h>
#include <sys/types.h>

struct termios orig_termios;

void initKeyboard() {
    const int fd = STDIN_FILENO;
    termios t;
    nAssert(tcgetattr(fd, &t) >= 0);
    orig_termios = t;
    t.c_lflag &= ~(ICANON | ECHO);
    nAssert(tcsetattr(fd, TCSANOW, &t) >= 0);
    setbuf(stdin, NULL);
}

void resetKeyboard() {
    const int fd = STDIN_FILENO;
    nAssert(tcsetattr(fd, TCSANOW, &orig_termios) >= 0);
}

int getch() { return getc(stdin); }

bool kbhit() {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);
    timeval tv;
    tv.tv_sec = tv.tv_usec = 0;
    const int v = select(STDIN_FILENO + 1, &rfds, 0, 0, &tv);
    nAssert(v != -1);
    return v != 0;
}

#endif  // WIN32

using std::string;

typedef unsigned char uchar;

class ReadFail { };
class SendFail { };
class ConnectFail { };
class UserExit { };

string encode(const string& str) {
    return utf8_mode ? latin1_to_utf8(str) : str;
}

string decode(const string& str) {
    return utf8_mode ? utf8_to_latin1(str) : str;
}

void send(Network::Socket sock, const void* data, int len) {
    if (nlWrite(sock, data, len) != len)
        throw SendFail();
}

class IdleFunction {
public:
    virtual ~IdleFunction() { }
    virtual void operator()() = 0;
};

class DelayHandler : public IdleFunction {
    enum { sayBufLen = 1024 };
    char sayBuf[sayBufLen + 1];
    Network::Socket sock;
    int sayIdx; // -1 when not typing
    bool kick, ban;
    int mute;
    bool* messageBoxSetting;

public:
    DelayHandler(Network::Socket socket, bool* messageBox) : sock(socket), sayIdx(-1), kick(false), ban(false), mute(0), messageBoxSetting(messageBox) { }
    void pauseSay() const {
        if (sayIdx != -1)
            printf("\r%79s\r", "");
    }
    void resumeSay() const {
        if (sayIdx != -1) {
            printf("say:   ");
            fwrite(sayBuf, 1, sayIdx, stdout);
            fflush(stdout);
        }
    }
    void operator()() {
        while (kbhit()) {
            char buf[12 + sayBufLen];
            int idx = 0;
            const int key = getch();
            if (sayIdx > -1) {
                if (key == 27) {
                    printf("(aborted)\n");
                    sayIdx = -1;
                }
                else if (key == '\r' || key == '\n') {
                    if (sayIdx > 0) {
                        writeLong(buf, idx, ATS_SERVER_CHAT);
                        sayBuf[sayIdx] = 0;
                        writeStr(buf, idx, decode(sayBuf));
                        send(sock, buf, idx);
                    }
                    sayIdx = -1;
                    printf("\n");
                }
                else {
                    if (key == '\b' || key == 0x7F) {
                        if (sayIdx > 0) {
                            sayIdx--;
                            printf("\b \b");
                            while (utf8_mode && (sayBuf[sayIdx] & 0xC0) == 0x80)
                                sayIdx--;
                            fflush(stdout);
                        }
                    }
                    else if (sayIdx < sayBufLen && !(key < 32/* || (key >= 128 && key <= 159)*/)) {
                        printf("%c", key);
                        fflush(stdout);
                        sayBuf[sayIdx++] = key;
                    }
                }
            }
            else if (key == 27) {
                printf("Confirm quit? (Y) ");
                fflush(stdout);
                if (getch() == 'Y') {
                    writeLong(buf, idx, ATS_QUIT);
                    send(sock, buf, idx);
                    throw UserExit();
                }
                else
                    printf("aborted\n");
            }
            else if (key >= '0' && key < '9') {
                const int pid = key - '0';
                if (mute || kick || ban) {
                    printf("Confirm: %s player %d? (Y) ", mute == 1 ? "mute" : mute == 2 ? "silently mute" : mute == 3 ? "unmute" : kick ? "kick" : "BAN", pid);
                    fflush(stdout);
                    if (getch() == 'Y') {
                        writeLong(buf, idx, mute ? ATS_MUTE_PLAYER : kick ? ATS_KICK_PLAYER : ATS_BAN_PLAYER);
                        writeLong(buf, idx, pid);
                        if (mute)
                            writeLong(buf, idx, mute == 3 ? 0 : mute);
                        printf("done\n");
                    }
                    else
                        printf("aborted\n");
                }
                else {
                    writeLong(buf, idx, ATS_GET_PLAYER_FRAGS);
                    writeLong(buf, idx, pid);
                    writeLong(buf, idx, ATS_GET_PLAYER_TOTAL_TIME);
                    writeLong(buf, idx, pid);
                    writeLong(buf, idx, ATS_GET_PLAYER_TOTAL_KILLS);
                    writeLong(buf, idx, pid);
                    writeLong(buf, idx, ATS_GET_PLAYER_TOTAL_DEATHS);
                    writeLong(buf, idx, pid);
                    writeLong(buf, idx, ATS_GET_PLAYER_TOTAL_CAPTURES);
                    writeLong(buf, idx, pid);
                }
                send(sock, buf, idx);
            }
            else if (toupper(key) == 'K') {
                kick = !kick;
                if (kick)
                    printf("KICK MODE\n");
                else
                    printf("/kick mode\n");
            }
            else if (toupper(key) == 'B') {
                ban = !ban;
                if (ban)
                    printf("BAN MODE\n");
                else
                    printf("/ban mode\n");
            }
            else if (toupper(key) == 'M') {
                mute = (mute + 1) % 4;
                switch (mute) {
                /*break;*/ case 0:
                        printf("/mute mode\n");
                    break; case 1:
                        printf("mute mode\n");
                    break; case 2:
                        printf("SILENT mute mode\n");
                    break; case 3:
                        printf("unmute mode\n");
                    break; default:
                        nAssert(0);
                }
            }
            else if (toupper(key) == 'S') {
                sayIdx = 0;
                printf("say:   ");
                fflush(stdout);
            }
            else if (toupper(key) == 'P') {
                writeLong(buf, idx, ATS_GET_PINGS);
                send(sock, buf, idx);
            }
            else if (toupper(key) == 'R') {
                printf("Confirm reset settings? (Y) ");
                fflush(stdout);
                if (getch() == 'Y') {
                    writeLong(buf, idx, ATS_RESET_SETTINGS);
                    send(sock, buf, idx);
                    printf("done\n");
                }
                else
                    printf("aborted\n");
            }
            else if (toupper(key) == 'Q') {
                *messageBoxSetting = !*messageBoxSetting;
                printf("Sayadmin message boxes %s\n", *messageBoxSetting ? "enabled" : "disabled");
            }
        }
    }
};

class DelaySocketReader {
    enum { rbufSz = 1024 };
    Network::Socket sock;
    IdleFunction* ifp;
    char rbuf[rbufSz];
    int rbufUsed, rbufRd;

public:
    DelaySocketReader(Network::Socket socket, IdleFunction* handlerFn) : sock(socket), ifp(handlerFn), rbufUsed(0), rbufRd(0) { }
    NLulong getLong() {
        NLulong val = 0;
        val =              getByte();
        val = (val << 8) | getByte();
        val = (val << 8) | getByte();
        val = (val << 8) | getByte();
        return val;
    }
    uchar getByte() {
        for (;;) {
            if (rbufRd < rbufUsed)
                return rbuf[rbufRd++];
            rbufUsed = nlRead(sock, rbuf, rbufSz);
            rbufRd = 0;
            if (rbufUsed == NL_INVALID)
                throw ReadFail();
            if (rbufUsed == 0) {
                (*ifp)();
                Sleep(100);
            }
        }
    }
};

FILE* outfile;

void dualprintf(const char* fmt, ...) PRINTF_FORMAT(1, 2);

void dualprintf(const char* fmt, ...) {
    time_t tt = time(0);
    tm* tmb = localtime(&tt);

    for (int i = 0; i < 2; i++) {
        FILE* const out = i ? outfile : stdout;
        va_list argptr;
        va_start(argptr, fmt);
        fprintf(out, "%02d%02d%02d %02d%02d%02d  ", tmb->tm_year % 100, tmb->tm_mon + 1, tmb->tm_mday, tmb->tm_hour, tmb->tm_min, tmb->tm_sec);
        vfprintf(out, fmt, argptr);
        va_end(argptr);
        fflush(out);
    }
}

string plyNames[32];
const char* plyName(int idx) {
    static char buf[50];
    platSnprintf(buf, 50, "%s (%d)", encode(plyNames[idx]).c_str(), idx);
    return buf;
}

bool runMonitor(int port, bool messageBoxes) {
    Network::Socket sock;

    nlDisable(NL_BLOCKING_IO);
    sock = nlOpen(0, NL_RELIABLE);
    nAssert(sock != NL_INVALID);

    try {
        Network::Address addr("127.0.0.1");
        addr.setPort(port);

        if (nlConnect(sock, addr.NLptr()) != NL_TRUE)
            throw ConnectFail();
        char nul;
        while (nlWrite(sock, &nul, 0) != 0) {
            if (nlGetError() != NL_CON_PENDING)
                throw ConnectFail();
            Sleep(10);
        }
        DelayHandler dh(sock, &messageBoxes);
        printf("Connected!\n");
        DelaySocketReader reader(sock, &dh);
        for (;;) {
            dh.resumeSay();
            const unsigned val = reader.getLong();
            dh.pauseSay();
            if (val >= NUMBER_OF_STA) {
                printf("<Invalid STA code: %u>", val);
                continue;
            }
            static const int ints[NUMBER_OF_STA] = { 0, 1, 1, 1, 1, 1, 1, 0, 2, 2, 2, 2, 2, 0, 0, 2, 1, 1 };
            static const int strs[NUMBER_OF_STA] = { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1 };
            unsigned ival[2];
            const int strBufLen = 1024;
            char strBuf[strBufLen + 1];
            for (int ii = 0; ii < ints[val]; ++ii)
                ival[ii] = reader.getLong();
            if (strs[val]) {
                int si;
                for (si = 0; si < strBufLen; ++si) {
                    strBuf[si] = reader.getByte();
                    if (strBuf[si] == 0)
                        break;
                }
                if (si == strBufLen) {
                    strBuf[si] = 0;
                    printf("Too long string: %s...\n", encode(strBuf).c_str());
                    continue;
                }
            }
            switch (val) {
            /*break;*/ case STA_NOOP:                  dualprintf("<no-op>\n");
                break; case STA_PLAYER_CONNECTED:      dualprintf("| Player %u connected\n", ival[0]);
                break; case STA_PLAYER_DISCONNECTED:   dualprintf("| %s disconnected\n", plyName(ival[0]));
                break; case STA_PLAYER_NAME_UPDATE:
                    plyNames[ival[0]] = strBuf;        dualprintf("| %u changes name to \"%s\"\n", ival[0], encode(strBuf).c_str());
                //break; case STA_PLAYER_KILLS:          dualprintf("| %s gets a kill\n", plyName(ival[0]));
                //break; case STA_PLAYER_DIES:           dualprintf("| %s dies\n", plyName(ival[0]));
                //break; case STA_PLAYER_CAPTURES:       dualprintf("| %s captures\n", plyName(ival[0]));
                break; case STA_GAME_OVER:             dualprintf("| Game over\n");
                break; case STA_PLAYER_FRAGS:          dualprintf("| %s has %u frags\n", plyName(ival[0]), ival[1]);
                break; case STA_PLAYER_TOTAL_TIME:     dualprintf("| %s has been playing for %u seconds\n", plyName(ival[0]), ival[1]);
                break; case STA_PLAYER_TOTAL_KILLS:    dualprintf("| %s has %u kills\n", plyName(ival[0]), ival[1]);
                break; case STA_PLAYER_TOTAL_DEATHS:   dualprintf("| %s has %u deaths\n", plyName(ival[0]), ival[1]);
                break; case STA_PLAYER_TOTAL_CAPTURES: dualprintf("| %s has %u captures\n", plyName(ival[0]), ival[1]);
                break; case STA_GAME_TEXT:             dualprintf("%s\n", encode(strBuf).c_str());
                break; case STA_QUIT:                  dualprintf("| Quit received\n"); nlClose(sock); return true;
                break; case STA_PLAYER_PING:           dualprintf("| %s has ping %u\n", plyName(ival[0]), ival[1]);
                break; case STA_PLAYER_IP:             dualprintf("| %s has IP %s\n", plyName(ival[0]), strBuf);
                break; case STA_ADMIN_MESSAGE: {
                    char cap[strBufLen + 100];
                    platSnprintf(cap, strBufLen + 100, "Sayadmin message from %s", plyNames[ival[0]].c_str());
                    dualprintf("|!| Sayadmin message from %s: %s\n", plyName(ival[0]), encode(strBuf).c_str());
                    if (messageBoxes)
                        messageBox(cap, strBuf, false);
                }
            }
        }
    } catch (UserExit) {
        printf("<User abort>\n");
        nlClose(sock);
        return true;
    } catch (...) {
        try {
            throw;
        } catch (SendFail) {
            printf("<Send failed>\n");
        } catch (ReadFail) {
            printf("<Read failed>\n");
        } catch (ConnectFail) {
            printf("<Connect failed>\n");
        } catch (...) {
            printf("<unknown exception>\n");
        }
        if (nlGetError() == NL_SYSTEM_ERROR)
            printf("sys: %s\n", nlGetSystemErrorStr(nlGetSystemError()));
        else
            printf("nl: %s\n", nlGetErrorStr(nlGetError()));
        nlClose(sock);
        return false;
    }
}

int main(int argc, const char* argv[]) {
    check_utf8_mode();
    initKeyboard();
    int port = 24500;
    bool messageBoxes = true;
    if (argc > 1) {
        if (toupper(argv[1][0]) == 'Q') {
            messageBoxes = false;
            --argc;
            ++argv;
        }
        if (argc > 1) {
            port = atoi(argv[1]);
            if (argc != 2 || port == 0) {
                printf("Arg: [q] [port]\n");
                return 2;
            }
        }
    }
    if (nlInit() == NL_FALSE) {
        printf("ERROR: cannot init HawkNL!\n");
        return 1;
    }
    if (nlSelectNetwork(NL_IP) == NL_FALSE) {
        printf("ERROR: no IP network!\n");
        return 1;
    }
    outfile = fopen("srvmonit.log", "at");
    if (!outfile) {
        printf("Can't open srvmonit.log for append!\n");
        return 1;
    }
    const int r = runMonitor(port, messageBoxes) ? 0 : 1;
    fclose(outfile);
    nlShutdown();
    resetKeyboard();
    return r;
}
#ifdef END_OF_MAIN
 END_OF_MAIN()
#endif
