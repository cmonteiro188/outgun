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
#include <deque>
#include <sstream>
#include <vector>

#include "../network.h"

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
    Spectator(const NLaddress& addr, const NLsocket& sock):
        address(addr),
        socket(sock),
        local(isLocalIP(addr)),
        next_frame(0),
        bytes_sent(0),
        first_buffer_sent(false)
    { }

    NLaddress address;
    NLsocket  socket;
    bool local;
    unsigned  next_frame;
    unsigned  bytes_sent;
    bool first_buffer_sent;
};

class Frame {
public:
    Frame(int l, const std::string& d, double t): len(l), data_(d), time_(t) { }

    void add(const std::string& d, double t) { data_.append(d); time_ = t; }

    unsigned length() const { return len; }
    unsigned used() const { return data_.length(); }
    unsigned remaining() const { return length() - used(); }
    bool full() const { return used() == length(); }
    const std::string& data() const { return data_; }
    double time() const { return time_; }

private:
    unsigned    len;
    std::string data_;
    double      time_;
};

class Game {
public:
    Game(): finished_(false) { }

    void add(const Frame& f) { frames.push_back(f); }
    void finish() { finished_ = true; }

    unsigned size() const { return frames.size(); }

    const Frame& frame(unsigned i) const { return frames[i]; }
    std::vector<Frame>& buffer() { return frames; }

    bool finished() const { return finished_; }

private:
    std::vector<Frame> frames;
    bool finished_;
};

class Relay {
public:
    Relay(unsigned short port, unsigned spectators);
    ~Relay();

    void run();

private:
    /// Listen for new connections
    void listen();

    /// Handle connections just opened
    void check_new_connections();

    /// Read game data from the Outgun server
    void get_server_data();

    /// Add game data to buffer
    bool add_data(std::istream& in);

    /// Send data to every spectator
    void send_data();

    /// Send data to the socket
    int send_data(NLsocket& socket, const std::string& data) const;

    /// Remove the oldest game if it is not needed anymore
    void remove_oldest_game();

    const Frame* get_frame(unsigned frame_nr) const;
    std::string frame_data(unsigned frame_nr, unsigned pos) const;

    void load_master_settings();
    void send_master_server();

    NLsocket listen_socket;     /// Socket for all incoming connections
    unsigned short listen_port; /// Port for all incoming connections

    NLaddress server_address;   /// Game server address
    NLsocket server_socket;     /// Game server socket
    std::string hostname;

    unsigned bandwidth_limit;   /// Total bandwidth limit, bytes per second
    unsigned spectator_limit;   /// Maximum number of spectators for this relay

    std::vector<Spectator> spectators;
    std::vector<Peer> peers;    /// Just connected "things"
    std::deque<Game> games;     /// Game data

    std::string waiting_data;

    Frame first_buffer;         /// Initial buffer that basically has the same data as in the start of the replay
    unsigned buffer_first_frame; /// Frame number of the start frame of the first game in list

    std::string master_name;
    std::string master_submit;
    NLaddress master_socket;
    double master_talk_time;
};

#endif // RELAY_H_INC
