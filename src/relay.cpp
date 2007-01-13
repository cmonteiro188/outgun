/*
 *  relay.cpp
 *
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

#include <iostream>
#include <string>

#ifdef __WINDOWS__
#include <windows.h>
#else
#include <sys/time.h>
#endif

#include "commont.h"
#include "network.h"
#include "relay.h"

using std::cout;
using std::ios;
using std::ostringstream;
using std::string;
using std::vector;

int main(/*int argc, const char* argv[]*/) {
    cout << "Starting relay server.\n";
    Relay* relay = new Relay(35000);
    relay->run();
    delete relay;
}

void platSleep(unsigned ms) {
    #ifdef __WINDOWS__
    Sleep(ms);
    #else
    struct timeval t;
    t.tv_sec = ms / 1000;
    t.tv_usec = (ms % 1000) * 1000;
    select(0, 0, 0, 0, &t);
    #endif
}

Relay::Relay(unsigned short port):
    new_game_first_frame(0),
    buffer_first_frame(0)
{
    if (nlInit())
        cout << "NL init successful.\n";
    else
        cout << "NL init failed\n";
    if (nlSelectNetwork(NL_IP))
        cout << "Network init successful.\n";
    else
        cout << "Network init failed.\n";

    nlDisable(NL_BLOCKING_IO);
    server_socket = nlOpen(port, NL_UNRELIABLE);
    listen_socket = nlOpen(port + 1, NL_UNRELIABLE);
    if (server_socket == NL_INVALID || listen_socket == NL_INVALID)
        cout << "Invalid socket.\n";
    log.clear();
    log.open("replay/relay.replay", ios::binary);
}

void Relay::run() {
    while (1) {
        listen_server();
        listen_clients();
        send_data();
        platSleep(5);
    }
}

void Relay::listen_server() {
    while (1) {
        const int max_buffer_size = 20000;
        char buffer[max_buffer_size];
        const int length = nlRead(server_socket, buffer, max_buffer_size);

        if (length == 0)
            break;
        if (length == NL_INVALID) {
            cout << "Socket read error.\n";
            break;
        }

        NLaddress addr;
        nlGetRemoteAddr(server_socket, &addr);
        NLchar addr_str[NL_MAX_STRING_LENGTH];
        nlAddrToString(&addr, addr_str);

        //cout << length << " bytes received from the game server at " << addr_str << ".\n";

        NLbyte first_data; int count = 0;
        readByte(buffer, count, first_data);
        ostringstream data;
        data.write(buffer + count, length - count);
        log.write(buffer + count, length - count);

        if (first_data) {
            // 24 25 26 *27 28 29 30 *
            const unsigned old_buffer_first_frame = buffer_first_frame;
            buffer_first_frame = new_game_first_frame;
            first_buffer = data.str();
            new_game_first_frame = data_buffer.size() + old_buffer_first_frame;
            data_buffer.erase(data_buffer.begin(), data_buffer.begin() + (buffer_first_frame - old_buffer_first_frame));
            cout << "First buffer received: " << length << " bytes.\n";
        }
        else
            data_buffer.push_back(data.str());
        platSleep(2);
    }
}

void Relay::listen_clients() {
    while (1) {
        const int max_buffer_size = 2048;
        char buffer[max_buffer_size];
        const int length = nlRead(listen_socket, buffer, max_buffer_size);

        if (length == 0)
            break;
        if (length == NL_INVALID)
            break;

        NLaddress addr;
        nlGetRemoteAddr(listen_socket, &addr);
        NLchar addr_str[NL_MAX_STRING_LENGTH];
        nlAddrToString(&addr, addr_str);

        cout << length << " bytes received from a client at " << addr_str << ".\n";

        if (length > 1) {
            int count = 0;
            string game, spectator;
            readStr(buffer, count, game);
            if (game != GAME_STRING)
                return;
            readStr(buffer, count, spectator);
            if (spectator != "SPECTATOR")
                return;
            Spectator spec(addr);
            spec.last_ack = timer.read();
            spectators.push_back(spec);
        }
        else
            for (vector<Spectator>::iterator si = spectators.begin(); si != spectators.end(); ++si)
                if (nlAddrCompare(&si->address, &addr)) {
                    si->last_ack = timer.read();
                    break;
                }
        platSleep(2);
    }
}

void Relay::send_data() {
    for (vector<Spectator>::iterator si = spectators.begin(); si != spectators.end();) {
        if (si->last_ack < timer.read() - 30) {
            si = spectators.erase(si);
            continue;
        }
        if (si->next_frame - buffer_first_frame >= data_buffer.size() || !nlSetRemoteAddr(listen_socket, &si->address)) {
            ++si;
            continue;
        }
        if (si->next_frame == 0) {
            cout << "Init data sent to a client.\n";
            nlWrite(listen_socket, first_buffer.data(), first_buffer.length());
            si->next_frame = new_game_first_frame;
        }
        const NLint result = nlWrite(listen_socket, data_buffer[si->next_frame - buffer_first_frame].data(),
                                     data_buffer[si->next_frame - buffer_first_frame].length());
        if (result == NL_INVALID)
            si = spectators.erase(si);
        else {
            //cout << result << " bytes sent to a client.\n";
            si->next_frame++;
            ++si;
        }
    }
}
