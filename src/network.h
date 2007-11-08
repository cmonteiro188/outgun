/*
 *  network.h
 *
 *  Copyright (C) 2004 - Niko Ritari
 *  Copyright (C) 2004 - Jani Rivinoja
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

#ifndef NETWORK_H_INC
#define NETWORK_H_INC

#include <iostream>
#include <map>
#include <string>

#include "utility.h"

#include <nl.h> //#remove

class LineReceiver;

class Network {
public:
    class Address {
        class HiddenData;
        HiddenData* hidden;

        friend class Network;

    public:
        Address();
        Address(const NLaddress& nla); //#remove
        Address(const Address& a);
        ~Address();
        Address& operator=(const Address& a);

        operator NLaddress&(); //#remove
        operator const NLaddress&() const; //#remove
    };

    class Socket : private NoCopying {
        class HiddenData;
        HiddenData* hidden;

        friend class Network;

    public:
        Socket();
        Socket(const NLsocket& nls); //#remove
        ~Socket();

        operator NLsocket&(); //#remove
        operator const NLsocket&() const; //#remove
    };

    // static members only
};

const char* getNlErrorString();

bool isValidIP(const std::string& address, bool allowPort = false, unsigned int minimumPort = 0, bool requirePort = false);
bool check_private_IP(const std::string& address, bool allowAnyExternal = false);   // with allowAnyExternal only (invalid and) loopback addresses are blocked
std::string getPublicIP(LineReceiver& log, bool allowAnyExternal = false);    // with allowAnyExternal only (invalid and) loopback addresses are blocked
bool isLocalIP(Network::Address address);  // returns true if address points to this machine (nothing to do with the address being private)
std::string addressToString(const Network::Address& address);
inline bool operator==(const Network::Address& a1, const Network::Address& a2) { return nlAddrCompare(&a1, &a2); }

inline void readStr(const char* buf, int& count, std::string& dst) {
    dst.clear();
    while (buf[count])
        dst += buf[count++];
    ++count;
}
inline void writeStr(char* buf, int& count, const std::string& src) {
    memcpy(&buf[count], src.data(), src.length());
    count += src.length();
    buf[count++] = '\0';
}

inline double safeReadFloat(const char* buf, int& count) {
    float val;
    readFloat(buf, count, val);
    return val;
}
inline void safeWriteFloat(char* buf, int& count, float val) {  // this adds type safety: val is ensured to be a float
    writeFloat(buf, count, val);
}

enum NetworkResult { NR_ok, NR_timeout, NR_nlError };   // timeout is also returned when abortFlag triggers the return

NetworkResult writeToUnblockingTCP(Network::Socket& socket, const char* data, int length,
                                const volatile bool* abortFlag, int timeout, int roundDelay = 500);
NetworkResult saveAllFromUnblockingTCP(Network::Socket& socket, std::ostream& out,
                                const volatile bool* abortFlag, int timeout, int roundDelay = 500);

std::string format_http_parameters(const std::map<std::string, std::string>& parameters);

std::string build_http_request(bool post, const std::string& host, const std::string& script, const std::string& parameters = "", const std::string& auth = "");

NetworkResult post_http_data(Network::Socket& socket, const volatile bool* abortFlag, int timeout, const std::string& host,
                             const std::string& script, const std::string& parameters, const std::string& auth = ""); // timeout in ms

NetworkResult save_http_response(Network::Socket& socket, std::ostream& out, const volatile bool* abortFlag, int timeout);   // timeout in ms

std::string url_encode(const std::string& str);
void url_encode(char c, std::ostream& out);
bool is_url_safe(char c);
std::string base64_encode(const std::string& data);

#endif
