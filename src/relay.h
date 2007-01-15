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

#include "timer.h"

#if defined WIN32 || defined WIN64
class MMSystemTimer : public SystemTimer {
    uint64_t base;
    uint32_t prev;

public:
    MMSystemTimer() {
        base = 0;
        prev = static_cast<uint32_t>(timeGetTime());
    }

    double read() {
        uint32_t val = static_cast<uint32_t>(timeGetTime());
        if (val < prev) // check wrap-around
            base += uint64_t(1) << 32;
        prev = val;
        return double(base + val) * .001; // value from timeGetTime is in ms
    }
};

#else

class LinuxTimer : public SystemTimer {
public:
    double read() {
        struct timeval tv;
        gettimeofday(&tv, 0);
        return tv.tv_sec + double(tv.tv_usec) * .000001;
    }
};
#endif

class Spectator {
public:
    Spectator(const NLaddress& addr): address(addr), next_frame(0) { }

    NLaddress address;
    unsigned  next_frame;
    double    last_ack;
};

class Relay {
public:
    Relay(unsigned short port);

    void run();

private:
    void listen_server();
    void listen_clients();

    void send_data();

    const std::string& frame_data(unsigned frame_nr) const;

    NLaddress server_address;
    NLsocket  server_socket;
    std::vector<Spectator> spectators;

    NLsocket listen_socket;

    std::vector<std::string> data_buffer;
    std::string first_buffer;
    unsigned new_game_first_frame;  // frame number of the first frame of the new game
    unsigned buffer_first_frame;    // frame number of data_buffer.front()

    SystemTimer* timer;

    std::ofstream log;
};

#endif // RELAY_H_INC
