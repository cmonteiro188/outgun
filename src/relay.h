/*
 *  relay.h
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

#ifndef RELAY_H_INC
#define RELAY_H_INC

#include <fstream>
#include <sstream>
#include <vector>

#include <nl.h>

class Peer {
public:
    Peer(const NLaddress& addr, const NLsocket& sock): address(addr), socket(sock) { }
    Peer(const Peer& peer);

    Peer& operator=(const Peer& peer);

    NLaddress address;
    NLsocket  socket;
    std::stringstream buffer;
};

class Spectator {
public:
    Spectator(const NLaddress& addr, const NLsocket& sock): address(addr), socket(sock), next_frame(0), bytes_sent(0), first_buffer_sent(false) { }

    NLaddress address;
    NLsocket  socket;
    unsigned  next_frame;
    unsigned  bytes_sent;
    bool first_buffer_sent;
};

class Frame {
public:
    Frame(int l, const std::string& str): len(l), d(str) { }

    void add(const std::string& str) { d.append(str); }

    unsigned length() const { return len; }
    unsigned used() const { return d.length(); }
    unsigned remaining() const { return length() - used(); }
    bool full() const { return used() == length(); }
    const std::string& data() const { return d; }

private:
    unsigned    len;
    std::string d;
};

class Relay {
public:
    Relay(unsigned short port, unsigned spectators);
    ~Relay();

    void run();

private:
    void listen();
    void check_new_connections();

    void get_server_data();
    void add_data(std::istream& in);

    void send_data();
    int send_data(NLsocket& socket, const std::string& data) const;

    std::string frame_data(unsigned frame_nr, unsigned pos) const;

    void load_master_settings();
    void send_master_server();

    void handle_keys();

    NLsocket listen_socket;
    int      listen_port;

    NLaddress server_address;
    NLsocket server_socket;
    std::string hostname;

    unsigned spectator_limit;
    std::vector<Spectator> spectators;

    std::vector<Peer> peers;

    std::stringstream incoming_buffer;
    std::vector<Frame> data_buffer;
    Frame first_buffer;
    unsigned new_game_first_frame;  // frame number of the first frame of the new game
    unsigned buffer_first_frame;    // frame number of data_buffer.front()

    std::string master_name;
    std::string master_submit;
    NLaddress master_socket;
    double master_talk_time;

    std::ofstream log;

    bool quit;
};

#endif // RELAY_H_INC
