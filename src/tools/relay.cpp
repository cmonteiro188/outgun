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
#include <map>
#include <string>

#include "../commont.h"
#include "../network.h"
#include "../platform.h"
#include "../protocol.h"
#include "../timer.h"
#include "../version.h"

#include "relay.h"

using std::cin;
using std::cout;
using std::deque;
using std::ios;
using std::ifstream;
using std::istream;
using std::istringstream;
using std::map;
using std::noskipws;
using std::ostringstream;
using std::string;
using std::stringstream;
using std::vector;

int main(int argc, const char* argv[]) {
    cout << "Outgun relay " << getVersionString() << '\n';
    int port = 0;
    //int bandwidth_limit = 20000;
    int spectators = 32;

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
    nlEnable(NL_SOCKET_STATS);
    platInit();
    platInitAfterAllegro();
    g_timeCounter.setZero();

    cout << "Starting relay server on port " << port << ".\n";

    Relay* relay = new Relay(port, spectators);
    relay->run();
    delete relay;

    platUninit();
    nlShutdown();
}

string itoa(int val) {
    ostringstream ss;
    ss << val;
    return ss.str();
}

Peer::Peer(const Peer& peer) {
    *this = peer;
}

Peer& Peer::operator=(const Peer& peer) {
    address = peer.address;
    socket = peer.socket;
    buffer.str("");
    buffer << peer.buffer.str();
    return *this;
}

Relay::Relay(unsigned short port, unsigned spectators):
    listen_port(port),
    bandwidth_limit(20000),
    spectator_limit(spectators),
    first_buffer(-1, string(), 0),
    buffer_first_frame(0),
    master_talk_time(0)
{ }

Relay::~Relay() {
    cout << "Closing sockets\n";
    listen_socket.closeIfOpen();
    server_socket.closeIfOpen();
    for (vector<Peer>::iterator pi = peers.begin(); pi != peers.end(); ++pi)
        pi->socket.closeIfOpen();
    for (vector<Spectator>::iterator si = spectators.begin(); si != spectators.end(); ++si)
        si->socket.closeIfOpen();
}

void Relay::run() {
    if (!listen_socket.open(Network::NonBlocking, Network::TCP, listen_port)) {
        cout << "Can't open socket: " << getNlErrorString() << '\n';
        return;
    }
    else if (!listen_socket.listen()) {
        cout << "Could not set socket to listen mode: " << getNlErrorString() << '\n';
        return;
    }

    load_master_settings();

    while (!g_exitFlag) {
        g_timeCounter.refresh();
        listen();
        check_new_connections();
        get_server_data();
        send_data();
        //send_master_server();
        remove_oldest_game();
        platSleep(5);
    }

    cout << "Shutdown\n";
}

// FIX: getline_skip_comments
void Relay::load_master_settings() {
    ifstream in("config/master.txt");
    string line;
    if (!getline(in, master_name))
        master_name = "koti.mbnet.fi";
    //master_name = "127.0.0.1"; // test
    getline(in, line);
    getline(in, line);
    if (!getline(in, master_submit))
        master_submit = "/outgun/servers/submit.php";
}

void Relay::listen() {
    while (listen_socket.isOpen()) {
        Network::Socket new_socket;
        if (!new_socket.acceptConnection(Network::NonBlocking, listen_socket)) {
            if (nlGetError() != NL_NO_PENDING)
                cout << "Can't accept incoming connection.\n";
            break;
        }

        Network::Address addr = new_socket.getRemoteAddress();
        cout << "Incoming connection from " << addr.toString() << ".\n";

        peers.push_back(Peer(addr, new_socket));
    }
}

void Relay::check_new_connections() {
    for (vector<Peer>::iterator pi = peers.begin(); pi != peers.end();) {
        const unsigned max_buffer_size = 2000;
        NLbyte buffer[max_buffer_size];
        const int result = pi->socket.read(buffer, max_buffer_size);

        if (result == Network::Error) {
            cout << "Socket read error. " << getNlErrorString() << '\n';
            pi->socket.close();
            pi = peers.erase(pi);
            continue;
        }

        if (result == 0) {
            ++pi;
            continue;
        }

        pi->buffer.write(buffer, result);

        istream& ist = pi->buffer;
        string game;
        read_string(ist, game);
        if (game != GAME_STRING) {
            cout << "Different game string in a connection attempt.\n";
            pi->socket.close();
            pi = peers.erase(pi);
            continue;
        }
        string type;
        read_string(ist, type);
        if (!ist) {     // Not all data received yet.
            ist.clear();
            ist.seekg(0);
            ++pi;
            continue;
        }
        if (type == "SPECTATOR") {
            if (spectators.size() >= spectator_limit) {
                cout << "New spectator couldn't join because spectator limit already reached.\n";
                pi->socket.close();
                pi = peers.erase(pi);
                continue;
            }
            unsigned replay_version;
            string username, password;
            read(ist, replay_version);
            read_string(ist, username);
            read_string(ist, password);

            if (!ist) {     // Not all data received yet.
                ist.clear();
                ist.seekg(0);
                ++pi;
                continue;
            }

            // TODO: Check username and password.
            #if 0
            if (!check_user()) {
                cout << "New spectator couldn't join because of invalid username or password.\n";
                pi->socket.close();
                pi = peers.erase(pi);
                continue;
            }
            #endif

            spectators.push_back(Spectator(pi->address, pi->socket));
            cout << "Spectator connected.\n";
            pi = peers.erase(pi);
        }
        else if (type == "SERVER") {
            if (server_socket.isOpen()) { // if already connected, skip
                cout << "Attempt to connect from another server blocked.\n";
                pi->socket.close();
                pi = peers.erase(pi);
                continue;
            }
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

            if (!ist) {     // Not all data received yet.
                ist.clear();
                ist.seekg(0);
                ++pi;
                continue;
            }

            ostringstream ost;
            ost << identification;
            write(ost, version);
            write(ost, replay_length);
            write_string(ost, hostname);
            write(ost, maxplayers);
            write_string(ost, string());    // Store empty map name because the server sent only the map name of the first game.
            first_buffer = Frame(ost.str().length(), ost.str(), get_time());

            server_socket = pi->socket;
            cout << "Server connected: " << hostname << '\n';

            pi = peers.erase(pi);
        }
        else if (type == "RELAY") {
            cout << "Subrelay connected. Just dropped it as there is no support for subrelays.\n";
            pi->socket.close();
            pi = peers.erase(pi);
        }
        else {
            cout << "Refused an unknown program.\n";
            pi->socket.close();
            pi = peers.erase(pi);
        }
    }
}

void Relay::get_server_data() {
    stringstream data;
    data << waiting_data;
    waiting_data.clear();
    while (server_socket.isOpen()) {
        const unsigned max_buffer_size = 20000;
        NLbyte buffer[max_buffer_size];
        const int result = server_socket.read(buffer, max_buffer_size);

        if (result == Network::Error) {
            cout << "Server disconnected: " << getNlErrorString() << '\n';
            server_socket.close();
            return;
        }
        if (result == 0)
            break;
        data.write(buffer, result);
    }

    if (data.str().empty())
        return;

    while (data) {
        const bool skipped = !add_data(data);
        if (skipped) {  // not enough data to create frame
            data >> noskipws >> waiting_data;
            break;
        }
        //cout << "Frame data\n";
    }
}

bool Relay::add_data(istream& in) {
    if (games.empty())
        games.push_back(Game());
    vector<Frame>* data_buffer = &games.back().buffer();
    if (!data_buffer->empty() && !data_buffer->back().full()) {
        string temp;
        read(in, temp, data_buffer->back().remaining());
        data_buffer->back().add(temp, get_time());
        //cout << "Frame " << data_buffer->size() - 1 << ", " << temp.length() << " bytes.\n";
    }
    else {
        const istream::pos_type pos = in.tellg();
        unsigned char data_code;
        unsigned length;
        if (read(in, data_code) && read(in, length)) {
            switch (data_code) {
            /*break;*/ case relay_data_game_start:
                    games.back().finish();
                    games.push_back(Game());
                    data_buffer = &games.back().buffer();
                    cout << "New game started.\n";
                break; case relay_data_frame:
                break; default: nAssert(0);
            }
            string temp;
            read(in, temp, length);
            data_buffer->push_back(Frame(length, temp, get_time()));
            //cout << "Frame " << data_buffer->size() - 1 << ", " << temp.length() << " bytes of " << length << ".\n";
        }
        else {
            in.clear();
            in.seekg(pos);
            return false;
        }
    }
    return true;
}

void Relay::send_data() {
    for (vector<Spectator>::iterator si = spectators.begin(); si != spectators.end();) {
        const unsigned temp_buffer_size = 10;
        NLbyte temp[temp_buffer_size];
        // Check connection
        int result = si->socket.read(temp, temp_buffer_size);
        if (result == Network::Error) {
            cout << "Spectator disconnected: " << getNlErrorString() << '\n';
            si->socket.close();
            si = spectators.erase(si);
            continue;
        }
        if (!first_buffer.full()) { // First buffer not ready to send yet
            ++si;
            continue;
        }
        if (!si->local) {           // Limit data sending rate for spectators
            const unsigned bpsout = si->socket.getStat(NL_AVE_BYTES_SENT);
            if (bpsout > bandwidth_limit / spectators.size())
                continue;
        }
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
        if (chunk.empty()) {    // Nothing to send yet
            ++si;
            continue;
        }
        result = send_data(si->socket, chunk);
        if (result == -1) {
            cout << "Spectator disconnected: " << getNlErrorString() << '\n';
            si->socket.close();
            si = spectators.erase(si);
            continue;
        }
        si->bytes_sent += result;
        const Frame& frame = si->first_buffer_sent ? *get_frame(si->next_frame) : first_buffer;
        nAssert(si->bytes_sent <= frame.length() + protocol_size);
        if (si->bytes_sent == frame.length() + protocol_size) { // A frame has entirely been sent
            si->bytes_sent = 0;
            if (!si->first_buffer_sent) {   // Send next the last game available
                cout << "Init data sent to a client.\n";
                unsigned game_start_buffer = buffer_first_frame;
                for (deque<Game>::iterator gi = games.begin(); gi != games.end(); gi++)
                    if (gi->finished())
                        game_start_buffer += gi->size();
                si->next_frame = game_start_buffer;
                si->first_buffer_sent = true;
            }
            else
                si->next_frame++;
        }
        ++si;
    }
}

int Relay::send_data(Network::Socket& socket, const string& data) const {
    if (!socket.isOpen()) {
        cout << "Closed spectator socket in send_data().\n";
        return -1;
    }
    int sent;
    if (!socket.write(data.data(), data.length(), &sent))
        return -1;
    return sent;
}

void Relay::remove_oldest_game() {
    if (!games.front().finished())
        return;
    bool transmissions_needed = false;
    for (vector<Spectator>::iterator si = spectators.begin(); si != spectators.end(); si++)
        if (si->next_frame <= buffer_first_frame + games.front().size()) {
            transmissions_needed = true;
            break;
        }
    if (!transmissions_needed) {
        buffer_first_frame += games.front().size();
        games.pop_front();
    }
}

const Frame* Relay::get_frame(unsigned frame_nr) const {
    const Frame* frame = 0;
    unsigned game_start_buffer = buffer_first_frame;
    for (deque<Game>::const_iterator gi = games.begin(); gi != games.end(); gi++) {
        if (game_start_buffer + gi->size() > frame_nr) {
            frame = &gi->frame(frame_nr - game_start_buffer);
            break;
        }
        game_start_buffer += gi->size();
    }
    return frame;
}

string Relay::frame_data(unsigned frame_nr, unsigned pos) const {
    const Frame* frame = 0;
    unsigned game_start_buffer = buffer_first_frame;
    bool current_game_finished = false;
    for (deque<Game>::const_iterator gi = games.begin(); gi != games.end(); gi++) {
        if (game_start_buffer + gi->size() > frame_nr) {
            frame = &gi->frame(frame_nr - game_start_buffer);
            current_game_finished = gi->finished();
            break;
        }
        game_start_buffer += gi->size();
    }
    if (!frame || !frame->full())
        return string();
    // Do not send too recent frames if the game is still going on.
    if (!current_game_finished && frame->time() + 0 * 60 > get_time())
        return string();
    ostringstream ost;
    if (pos == 0)
        write(ost, frame->length());
    ost << frame->data().substr(pos);
    return ost.str();
}

void Relay::send_master_server() {
    if (get_time() < master_talk_time)
        return;

    master_talk_time = get_time() + 5 * 60.0;

    Network::Socket msock(Network::NonBlocking, Network::TCP, 0);
    if (!msock.isOpen()) {
        cout << "Can't open socket to connect to master server.\n";;
        return;
    }

    Network::Address master_address;
    if (!master_address.resolve(master_name)) {
        cout << "Can't resolve master address for " << master_name << ".\n";
        msock.close();
        return;
    }
    int port = master_address.getPort();
    if (!port) {
        port = 80;
        master_address.setPort(port);
    }

    if (!msock.connect(master_address)) {
        cout << "Can't connect to master server.\n";
        msock.close();
        return;
    }

    // build and send data
    map<string, string> parameters;
    parameters["port"] = itoa(listen_port);
    parameters["server"] = hostname;
    const string data = format_http_parameters(parameters);
    cout << master_name << ": " << data << '\n';
    NetworkResult result = post_http_data(msock, &g_exitFlag, 1000, master_name, master_submit, data);
    if (result != NR_ok)
        cout << "Master talker: Error sending info: " << (result == NR_timeout ? "Timeout" : getNlErrorString()) << '\n';
    else
        save_http_response(msock, cout, &g_exitFlag, 1000);
    msock.close();
}
