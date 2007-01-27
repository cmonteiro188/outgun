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

#include "commont.h"
#include "network.h"
#include "platform.h"
#include "timer.h"

#include "relay.h"

using std::cin;
using std::cout;
using std::ios;
using std::ifstream;
using std::istream;
using std::istringstream;
using std::ostringstream;
using std::string;
using std::stringstream;
using std::vector;

int main(int argc, const char* argv[]) {
    int port = 0;
    if (argc == 2)
        port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        cout << "Usage: relay port\n";
        return 0;
    }
    if (nlInit())
        cout << "NL init successful.\n";
    else {
        cout << "NL init failed\n";
        return 0;
    }
    if (nlSelectNetwork(NL_IP))
        cout << "Network init successful.\n";
    else {
        cout << "Network init failed.\n";
        return 0;
    }
    cout << "Starting relay server on port " << port << ".\n";
    platInit();

    Relay* relay = new Relay(port);
    relay->run();
    delete relay;

    platUninit();
    nlShutdown();
}

Relay::Relay(unsigned short port):
    listen_port(port),
    server_socket(NL_INVALID),
    first_buffer(-1, string()),
    new_game_first_frame(0),
    buffer_first_frame(0),
    master_talk_time(0),
    quit(false)
{ }

Relay::~Relay() {
    nlClose(listen_socket);
    nlClose(server_socket);
    for (vector<Spectator>::const_iterator si = spectators.begin(); si != spectators.end(); ++si)
        nlClose(si->socket);
}

void Relay::run() {
    nlDisable(NL_BLOCKING_IO);
    listen_socket = nlOpen(listen_port, NL_RELIABLE);

    if (listen_socket == NL_INVALID)
        cout << "Invalid socket.\n";
    if (!nlListen(listen_socket)) {
        cout << "Could not set socket to listen mode.\n";
        return;
    }

    log.clear();
    log.open("replay/relay.replay", ios::binary);

    load_master_settings();

    while (!quit) {
        listen();
        get_server_data();
        send_data();
        handle_keys();
        platSleep(5);
    }
}

void Relay::load_master_settings() {
    #if 0
    ifstream in("config/master.txt");
    string line;
    if (!getline_skip_comments(in, master_name))
        master_name = "koti.mbnet.fi";
    getline_skip_comments(in, line);
    getline_skip_comments(in, line);
    if (!getline_skip_comments(in, master_submit))
        master_submit = "/outgun/servers/submit.php";
    #endif
}

void Relay::listen() {
    while (listen_socket != NL_INVALID) {
        nlDisable(NL_BLOCKING_IO);
        NLsocket new_socket = nlAcceptConnection(listen_socket);

        if (new_socket == NL_INVALID) {
            if (nlGetError() != NL_NO_PENDING)
                cout << "Can't accept incoming connection.\n";
            break;
        }

        NLaddress addr;
        nlGetRemoteAddr(new_socket, &addr);
        NLchar addr_str[NL_MAX_STRING_LENGTH];
        nlAddrToString(&addr, addr_str);

        cout << "Incoming connection from " << addr_str << ".\n";

        const unsigned max_buffer_size = 2000;
        NLbyte buffer[max_buffer_size];
        const int result = nlRead(new_socket, buffer, max_buffer_size);

        if (result == NL_INVALID) {
            cout << "Socket read error. " << getNlErrorString() << '\n';
            break;
        }
        if (result == 0) {
            cout << "Nothing to read.\n";
            break;
        }

        istringstream ist(string(buffer, buffer + result));
        string game;
        read_string(ist, game);
        if (game != GAME_STRING)
            return;
        string type;
        read_string(ist, type);
        if (type == "SPECTATOR") {
            Spectator spec(addr, new_socket);
            spectators.push_back(spec);
            cout << "Spectator connected.\n";
        }
        else if (type == "SERVER") {
            if (server_socket != NL_INVALID) { // if already connected, skip
                cout << "Attempt to connect from another server blocked.\n";
                nlClose(new_socket);
            }
            else {
                unsigned length;
                string identification;
                unsigned version;
                unsigned replay_length;
                unsigned maxplayers;
                string map_name;
                read(ist, length);
                read(ist, identification, REPLAY_IDENTIFICATION.length());
                read(ist, version);
                read(ist, replay_length);
                read_string(ist, hostname);
                read(ist, maxplayers);
                read_string(ist, map_name);

                ostringstream ost;
                ost << identification;
                write(ost, version);
                write(ost, replay_length);
                write_string(ost, hostname);
                write(ost, maxplayers);
                write_string(ost, string());    // This is because the server sent only the map name of the first game.
                first_buffer = Frame(ost.str().length(), ost.str());
                log << ost.str();

                server_socket = new_socket;
                cout << "Server connected: " << hostname << '\n';
            }
        }
        else
            cout << "Refused an unknown program " << type << ".\n";
    }
}

void Relay::get_server_data() {
    stringstream data;
    while (server_socket != NL_INVALID) {
        const unsigned max_buffer_size = 20000;
        NLbyte buffer[max_buffer_size];
        const int result = nlRead(server_socket, buffer, max_buffer_size);

        if (result == NL_INVALID) {
            cout << "Server socket read error. " << getNlErrorString() << '\n';
            nlClose(server_socket);
            server_socket = NL_INVALID;
            return;
        }
        if (result == 0)
            break;
        data.write(buffer, result);
    }

    if (data.str().empty())
        return;

    while (data) {
        add_data(data);
        //cout << "Frame data\n";
    }
}

void Relay::add_data(istream& in) {
    if (!data_buffer.empty() && !data_buffer.back().full()) {
        string temp;
        read(in, temp, data_buffer.back().remaining());
        data_buffer.back().add(temp);
        log << temp;
        //cout << "Frame " << data_buffer.size() - 1 << ", " << temp.length() << " bytes.\n";
    }
    else {
        char game_start;
        if (read(in, game_start) && game_start == 1) {
            const unsigned old_buffer_first_frame = buffer_first_frame;
            buffer_first_frame = new_game_first_frame;
            new_game_first_frame = data_buffer.size() + old_buffer_first_frame;
            data_buffer.erase(data_buffer.begin(), data_buffer.begin() + (buffer_first_frame - old_buffer_first_frame));
            cout << "New game started.\n";
        }
        unsigned length;
        if (read(in, length)) {
            string temp;
            read(in, temp, length);
            data_buffer.push_back(Frame(length, temp));
            write(log, length);
            log << temp;
            //cout << "Frame " << data_buffer.size() - 1 << ", " << temp.length() << " bytes of " << length << ".\n";
        }
    }
}

void Relay::send_data() {
    if (!first_buffer.full())
        return;
    for (vector<Spectator>::iterator si = spectators.begin(); si != spectators.end();) {
        string chunk;
        unsigned protocol_size;
        if (!si->first_buffer_sent) {
            chunk = first_buffer.data().substr(si->bytes_sent);
            protocol_size = 0;
        }
        else {
            chunk = frame_data(si->next_frame, si->bytes_sent);
            protocol_size = sizeof(unsigned);
        }
        if (chunk.empty()) {
            ++si;
            continue;
        }
        const unsigned temp_buffer_size = 10;
        NLbyte temp[temp_buffer_size];
        int result = nlRead(si->socket, temp, temp_buffer_size);
        if (result != NL_INVALID)
            result = send_data(si->socket, chunk);
        if (result == NL_INVALID) {
            cout << "Spectator disconnected: " << getNlErrorString() << '\n';
            nlClose(si->socket);
            si = spectators.erase(si);
            continue;
        }
        si->bytes_sent += result;
        const Frame& frame = si->first_buffer_sent ? data_buffer[si->next_frame - buffer_first_frame] : first_buffer;
        nAssert(si->bytes_sent <= frame.length() + protocol_size);
        if (si->bytes_sent == frame.length() + protocol_size) {
            si->bytes_sent = 0;
            if (!si->first_buffer_sent) {
                cout << "Init data sent to a client.\n";
                si->next_frame = new_game_first_frame;
                si->first_buffer_sent = true;
            }
            else
                si->next_frame++;
        }
        ++si;
    }
}

int Relay::send_data(NLsocket& socket, const string& data) const {
    if (socket == NL_INVALID) {
        cout << "Invalid spectator socket in send_data().\n";
        return NL_INVALID;
    }
    return nlWrite(socket, data.data(), data.length());
}

string Relay::frame_data(unsigned frame_nr, unsigned pos) const {
    if (frame_nr - buffer_first_frame >= data_buffer.size())
        return string();
    const Frame& frame = data_buffer[frame_nr - buffer_first_frame];
    ostringstream ost;
    if (pos == 0)
        write(ost, frame.length());
    ost << frame.data().substr(pos);
    return ost.str();
}

#if 0
void Relay::send_master_server() {
    if (get_time() < master_talk_time)
        return;

    master_talk_time = get_time() + 5 * 60.0;

    nlDisable(NL_BLOCKING_IO);
    NLsocket msock = nlOpen(0, NL_RELIABLE);
    if (msock == NL_INVALID) {
        cout << "Can't open socket to connect to master server.\n";;
        return;
    }

    if (nlConnect(msock, &master_address) == NL_FALSE) {
        cout << "Can't connect to master server.\n";
        nlClose(msock);
        return;
    }

    // build and send data
    map<string, string> parameters;
    parameters["port"] = itoa(listen_port);
    parameters["servers"] = 1;
    parameters["server[]"] = hostname;
    const string data = build_http_data(parameters);
    NetworkResult result = post_http_data(msock, 0, 50, master_name, "/outgun/servers/submit.php", data);
    if (result != NR_ok)
        cout << "Master talker: Error sending info: " << (result == NR_timeout ? "Timeout" : getNlErrorString()) << '\n';
}
#endif

void Relay::handle_keys() {
    //cout << "handle_keys()\n";
    if (cin.rdbuf()->in_avail()) {
        //cout << "hmm\n";
        char c;
        cin.get(c);
        if (toupper(c) == 'Q') {
            quit = true;
            cout << "quit\n";
        }
    }
}
