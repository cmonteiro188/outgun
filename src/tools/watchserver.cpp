/*
 *  watchserver.cpp
 *
 *  Copyright (C) 2010 - Niko Ritari
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

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include <cstdlib>
#include <ctime>

#include "../binaryaccess.h"
#include "../commont.h"
#include "../function_utility.h"
#include "../network.h"
#include "../platform.h"
#include "../timer.h"

using std::cerr;
using std::cout;
using std::flush;
using std::istringstream;
using std::string;

class SyntaxError { }; // exception

int argAtoi(const string& s) throw (SyntaxError) {
    istringstream is(s);
    int value;
    is >> value;
    if (!is || !is.eof())
        throw SyntaxError();
    return value;
}

int probe(Network::UDPSocket& sock, const Network::Address& address) {
    const uint8_t sentByte1 = rand() & 0xFF;
    const uint8_t sentByte2 = rand() & 0xFF;

    BinaryBuffer<512> msg;
    msg.U32(0);
    msg.U32(200);
    msg.U8(sentByte1);
    msg.U8(sentByte2);

    sock.write(address, msg);

    platSleep(5000);

    class BadData { }; // internal exception

    char buffer[512];
    Network::UDPSocket::ReadResult result;
    int nInvalid = 0;
    for (;;) {
        result = sock.read(buffer, 512);
        if (result.length == 0) {
            cerr << "No reply.";
            if (nInvalid)
                cout << " (" << nInvalid << " invalid packets)";
            cout << '\n';
            return -1;
        }
        if (result.source != address)
            continue;
        try {
            BinaryDataBlockReader msg(buffer, result.length);

            const uint32_t dw1 = msg.U32();
            const uint32_t dw2 = msg.U32();
            const uint8_t b1 = msg.U8();
            const uint8_t b2 = msg.U8();
            if (dw1 != 0 || dw2 != 200 || b1 != sentByte1 || b2 != sentByte2)
                throw BadData();
            const string text = msg.str();

            const string::size_type slashPos = text.find_first_of('/');
            if (slashPos == string::npos)
                throw BadData();
            const string::size_type lastBefore = text.find_last_not_of("0123456789", slashPos - 1);
            const string::size_type numStart = lastBefore == string::npos ? 0 : lastBefore + 1;
            istringstream numStr(text.substr(numStart, slashPos - numStart));
            int nPlayers;
            numStr >> nPlayers;
            if (!numStr)
                throw BadData();
            nAssert(numStr.eof());
            return nPlayers;
        } catch (BinaryReader::ReadOutside) {
            ++nInvalid;
        } catch (BadData) {
            ++nInvalid;
        }
    }
}

void notify() {
    int status = system("./watchserver-notifier");
    if (status == -1) {
        cerr << "Executing ./watchserver-notifier failed\n";
        abort();
    }
    #ifdef WIFEXITED
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        cerr << "./watchserver-notifier failed (returned " << WEXITSTATUS(status) << ")\n";
    #endif
}

int main(int argc, const char* argv[]) {
    platInit();
    AtScopeExit autoPlatUninit(newFun0(platUninit));

    int minPlayers = 1, minutesBeforeNotify = 1;
    string server;

    try {
        for (int iArg = 1; iArg < argc; ++iArg) {
            const string arg = argv[iArg];
            if (arg == "-p" && iArg + 1 < argc) {
                minPlayers = argAtoi(argv[++iArg]);
                if (minPlayers < 1)
                    throw SyntaxError();
            }
            else if (arg == "-t" && iArg + 1 < argc) {
                minutesBeforeNotify = argAtoi(argv[++iArg]);
                if (minutesBeforeNotify < 0)
                    throw SyntaxError();
            }
            else if (server.empty() && !arg.empty())
                server = arg;
            else
                throw SyntaxError();
        }
        if (server.empty())
            throw SyntaxError();
    } catch (SyntaxError) {
        cerr << "syntax: " << argv[0] << " [-p players] [-t minutes] server-address\n"
             << "Executes ./watchserver-notifier after the server has reported at least the given number of players (default 1, min 1) for the given time (default 1 minute, min 0).\n";
        return 2;
    }

    try {
        Network::init();
    } catch (const Network::Error& e) {
        cerr << e.str() << '\n';
        return 1;
    }
    AtScopeExit autoShutdownNetwork(newFun0(Network::shutdown));

    Network::Address address;
    if (!address.tryResolve(server)) {
        cerr << "Can't resolve IP address for " << server << '\n';
        return 1;
    }
    if (address.getPort() == 0)
        address.setPort(DEFAULT_UDP_PORT);

    Network::UDPSocket sock(true);
    try {
        sock.open(Network::NonBlocking, 0);
    } catch (const Network::Error& e) {
        cerr << "Can't open socket. " << e.str() << '\n';
        return 1;
    }

    srand(time(0));

    int activeInRow = 0;
    time_t lastRoundTime = 0;
    for (;;) {
        for (;;) {
            const time_t now = time(0);
            if (now >= lastRoundTime + 60) {
                lastRoundTime = now;
                break;
            }
            platSleep(1000 * (lastRoundTime + 60 - now));
        }

        try {
            const int players = probe(sock, address);
            if (players >= 0)
                cout << date_and_time() << ' ' << players << " players\n" << std::flush;
            if (players < minPlayers)
                activeInRow = 0;
            else if (activeInRow++ >= minutesBeforeNotify)
                notify();
        } catch (Network::Error& e) {
            cerr << "Network error. " << e.str() << '\n';
            activeInRow = 0;
        }
    }
}
