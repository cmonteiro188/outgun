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
#include <string>

#include <nl.h>

class LogSet;

const char* getNlErrorString();
bool check_private_IP(const std::string& address);
std::string getPublicIP(LogSet& log);
bool isLocalIP(NLaddress address);	// local doesn't mean private
std::string addressToString(const NLaddress& address);
inline bool operator==(const NLaddress& a1, const NLaddress& a2) { return nlAddrCompare(&a1, &a2); }

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

enum NetworkResult { NR_ok, NR_timeout, NR_nlError };	// timeout is also returned when abortFlag triggers the return

NetworkResult writeToUnblockingTCP(NLsocket& socket, const char* data, int length,
								const volatile bool* abortFlag, int timeout, int roundDelay = 500);
NetworkResult saveAllFromUnblockingTCP(NLsocket& socket, std::ostream& out,
								const volatile bool* abortFlag, int timeout, int roundDelay = 500);

std::string url_encode(const std::string& str);
void url_encode(char c, std::ostream& out);
bool is_url_safe(char c);
std::string base64_encode(const std::string& data);

#endif
