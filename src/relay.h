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

class Spectator {
public:
    Spectator(const NLaddress& addr, const NLsocket& sock): address(addr), socket(sock), next_frame(0) { }

    NLaddress address;
    NLsocket  socket;
    unsigned  next_frame;
};

class Relay {
public:
    Relay(unsigned short port);
    
    void run();
    
    void listen_server();
    void listen_clients();

    void send_data();

private:
    NLaddress server_address;
    NLsocket  server_socket;
    std::vector<Spectator> spectators;
    
    NLsocket listen_socket;

    std::vector<std::string> data_buffer;
    std::string first_buffer;
    
    std::ofstream log;
};

#endif // RELAY_H_INC
