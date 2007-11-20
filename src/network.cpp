/*
 *  network.cpp
 *
 *  Copyright (C) 2004, 2006 - Niko Ritari
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

#include <iomanip>
#include <sstream>
#include <string>

#include "commont.h"
#include "language.h"
#include "mutex.h"
#include "nassert.h"
#include "protocol.h"
#include "utility.h"
#include "timer.h"
#include "version.h"

#include "network.h"

using std::hex;
using std::ostream;
using std::ostringstream;
using std::map;
using std::setfill;
using std::setw;
using std::string;
using std::vector;

static Mutex nlOpenMutex("network.cpp:nlOpenMutex");

typedef Network::Address Address;
typedef Network::Socket Socket;

Network::Error::Error() :
    nlError(nlGetError())
{
    if (nlError == NL_SYSTEM_ERROR)
        sysError = nlGetSystemError();
}

string Network::Error::basicStr() const {
    nAssert(nlError != NL_NO_ERROR);
    if (nlError == NL_SYSTEM_ERROR)
        return nlGetSystemErrorStr(sysError);
    else
        return nlGetErrorStr(nlError);
}

Network::ResolveError::ResolveError() {
    nlError = NL_NO_ERROR;
}

string Network::Error       ::str() const { return _("Network error: $1", basicStr()); }
string Network::BadIP       ::str() const { return _("\"$1\" is not a valid IP address", ip); }
string Network::ResolveError::str() const { return _("Error resolving hostname \"$1\": $2", name, basicStr()); }
string Network::ConnectError::str() const { return _("Error connecting to \"$1\": $2", addr, basicStr()); }
string Network::ListenError ::str() const { return _("Error setting socket to listen mode: $1", basicStr()); }

string Network::ReadWriteError::str() const {
    if (inRead)
        return _("Error reading from socket: $1", basicStr());
    else
        return _("Error writing to socket: $1", basicStr());
}

bool Network::ConnectError  ::connectionRefused() const { return nlError == NL_CON_REFUSED; }

bool Network::ReadWriteError::connectionRefused() const { return nlError == NL_CON_REFUSED; }
bool Network::ReadWriteError::connectionPending() const { return nlError == NL_CON_PENDING; }
bool Network::ReadWriteError::disconnected()      const { return nlError == NL_MESSAGE_END; }

class Address::HiddenData {
public:
    NLaddress nla;
    // Everything else is in the Address class itself. This class is used only to avoid including nl.h in network.h.

    HiddenData() { }
    HiddenData(const NLaddress& n) : nla(n) { }
};

class Socket::HiddenData {
public:
    NLsocket nls;
    // Everything else is in the Socket class itself. This class is used only to avoid including nl.h in network.h.

    HiddenData(NLsocket n) : nls(n) { }
};

// shortcuts for accessing raw NLaddresses and NLsockets within Address and Socket classes, used throughout this file
#define NLA hidden->nla
#define NLS hidden->nls

Address::Address(HiddenData* h) throw () :
    hidden(h)
{ }

Address::Address() throw () :
    hidden(new HiddenData())
{
    NLA.valid = false;
}

Address::Address(const Address& a) throw () :
    hidden(new HiddenData(*a.hidden))
{ }

Address::Address(const std::string& ip) throw (BadIP) :
    hidden(new HiddenData())
{
    if (!fromIP(ip))
        throw BadIP(ip);
}

Address::~Address() throw () {
    delete hidden;
}

Address& Address::operator=(const Address& a) throw () {
    NLA = a.NLA;
    return *this;
}

void Address::clear() throw () {
    NLA.valid = false;
}

bool Address::tryResolve(const std::string& hostname, ResolveError* errorStore) throw () {
    if (!nlGetAddrFromName(hostname.c_str(), &NLA)) {
        numAssert(nlGetError() == NL_OUT_OF_MEMORY || nlGetError() == NL_SYSTEM_ERROR, nlGetError());
        if (errorStore)
            *errorStore = ResolveError(hostname);
        clear();
        return false;
    }
    nAssert(valid());
    return true;
}

bool Address::fromIP(const std::string& ip) throw () {
    if (!nlStringToAddr(ip.c_str(), &NLA)) {
        numAssert(nlGetError() == NL_BAD_ADDR, nlGetError());
        clear();
    }
    //todo: else { check port here }
    return valid();
}

void Address::fromValidIP(const std::string& ip) throw () {
    if (!fromIP(ip))
        nAssert(0);
}

std::string Address::toString() const throw () {
    nAssert(valid());
    char buf[NL_MAX_STRING_LENGTH];
    if (!nlAddrToString(&NLA, buf))
        nAssert(0);
    return buf;
}

void Address::setPort(uint16_t port) throw () {
    nAssert(valid());
    if (!nlSetAddrPort(&NLA, port))
        nAssert(0);
}

uint16_t Address::getPort() const throw () {
    nAssert(valid());
    /*todo:
    if (!portSet)
        return 0;
    */
    const uint16_t port = nlGetPortFromAddr(&NLA);
    //todo: nAssert(port != 0);
    return port;
}

bool Address::valid() const throw () {
    return NLA.valid;
}

bool Address::operator==(const Address& a) const throw () {
    nAssert(valid() && a.valid());
    return nlAddrCompare(&NLA, &a.NLA);
}

Socket::Socket(bool autoClose_) throw () :
    hidden(new HiddenData(NL_INVALID)),
    connected(false),
    autoClose(autoClose_)
{ }

Socket::Socket(BlockingMode b, SocketType t, uint16_t port, bool autoClose_) throw (OpenError) :
    hidden(new HiddenData(NL_INVALID)),
    connected(false),
    autoClose(autoClose_)
{
    open(b, t, port);
}

Socket::~Socket() throw () {
    if (autoClose)
        closeIfOpen();
    nAssert(!isOpen());
    delete hidden;
}

bool Socket::tryOpen(BlockingMode b, SocketType t, uint16_t port) throw () {
    nAssert(!isOpen());
    Lock ml(nlOpenMutex);
    if (b == Blocking)
        nlEnable(NL_BLOCKING_IO);
    else
        nlDisable(NL_BLOCKING_IO);
    NLS = nlOpen(port, t == UDP ? NL_UNRELIABLE : t == TCP ? NL_RELIABLE : NL_BROADCAST);
    if (!isOpen())
        numAssert(nlGetError() == NL_SYSTEM_ERROR, nlGetError());
    return isOpen();
}

void Socket::open(BlockingMode b, SocketType t, uint16_t port) throw (OpenError) {
    if (!tryOpen(b, t, port))
        throw OpenError(t, port);
}    

void Socket::close() throw () {
    nAssert(isOpen());
    if (!nlClose(NLS))
        nAssert(0);
    NLS = NL_INVALID;
    connected = false;
}

void Socket::closeIfOpen() throw () {
    if (isOpen())
        close();
}

bool Socket::isOpen() const throw () {
    nAssert(NLS != NL_INVALID || !connected);
    return NLS != NL_INVALID;
}

Address Socket::getLocalAddress() const throw (Error) {
    Address a;
    if (!nlGetLocalAddr(NLS, &a.NLA))
        throw Error();
    return a;
}

Address Socket::getRemoteAddress() const throw (Error) {
    Address a;
    if (!nlGetRemoteAddr(NLS, &a.NLA))
        throw Error();
    return a;
}

int Socket::getStat(NLenum type) const throw () {
    return nlGetSocketStat(NLS, type); // can't verify result, because 0 can mean two things and we've no real way to clear what nlGetError returns, either (we could force it to an error value never returned by nlGetSocketStat, but that would be too funny)
}

void Socket::connect(const Address& a) throw (ConnectError) {
    nAssert(isOpen() && !connected);
    if (!nlConnect(NLS, &a.NLA)) {
        numAssert(nlGetError() == NL_SYSTEM_ERROR || nlGetError() == NL_CON_REFUSED, nlGetError());
        throw ConnectError(a.toString());
    }
    connected = true;
}

void Socket::listen() throw (ListenError) {
    nAssert(isOpen() && !connected);
    if (!nlListen(NLS)) {
        numAssert(nlGetError() == NL_SYSTEM_ERROR, nlGetError());
        throw ListenError();
    }
}

bool Socket::acceptConnection(BlockingMode b, Socket& listenerSock) throw () {
    nAssert(!isOpen() && listenerSock.isOpen());
    Lock ml(nlOpenMutex);
    if (b == Blocking)
        nlEnable(NL_BLOCKING_IO);
    else
        nlDisable(NL_BLOCKING_IO);
    const NLsocket newSocket = nlAcceptConnection(listenerSock.NLS);
    if (newSocket != NL_INVALID) {
        NLS = newSocket;
        connected = true;
        return true;
    }
    numAssert(nlGetError() == NL_NO_PENDING, nlGetError());
    return false;
}

void Socket::setRemoteAddress(const Address& a) throw (Error) {
    if (!nlSetRemoteAddr(NLS, &a.NLA))
        throw Error();
}

int Socket::read(void* buffer, int bufSize) throw (ReadWriteError) {
    nAssert(isOpen() /*&& connected (violated at least in Leetnet)*/ && buffer);
    const NLint val = nlRead(NLS, buffer, bufSize);
    if (val != NL_INVALID)
        return val;
    const NLenum err = nlGetError();
    numAssert(err == NL_BUFFER_SIZE || err == NL_CON_REFUSED || err == NL_CON_PENDING || err == NL_SYSTEM_ERROR || err == NL_MESSAGE_END, err);
    throw ReadWriteError(true);
}

void Socket::write(const void* data, int size, int* writtenSize) throw (ReadWriteError) {
    nAssert(isOpen() /*&& connected (violated at least in Leetnet)*/ && data);
    const NLint val = nlWrite(NLS, data, size);
    if (val == NL_INVALID) {
        const NLenum err = nlGetError();
        numAssert(err == NL_CON_REFUSED || err == NL_CON_PENDING || err == NL_SYSTEM_ERROR || err == NL_MESSAGE_END, err);
        throw ReadWriteError(false);
    }
    if (writtenSize)
        *writtenSize = val;
    else
        numAssert2(val == size, val, size);
}

vector<Address> Network::getAllLocalAddresses() {
    NLint count;
    NLaddress* addressList = nlGetAllLocalAddr(&count);
    vector<Address> a;
    if (!addressList) // important because nlGetAllLocalAddr might not set count on an error
        return a;
    a.reserve(count);
    for (int i = 0; i < count; ++i)
        a.push_back(Address(new Address::HiddenData(addressList[i])));
    return a;
}

Address Network::getDefaultLocalAddress() {
    Socket s(NonBlocking, UDP, 0);
    Address addr;
    if (s.isOpen()) {
        addr = s.getLocalAddress();
        s.close();
        addr.setPort(0);
    }
    return addr;
}

bool isValidIP(const string& address, bool allowPort, unsigned int minimumPort, bool requirePort) {
    nAssert(!(!allowPort && requirePort));
    unsigned int i1, i2, i3, i4, port;
    char midChar, endChar;
    const int n = sscanf(address.c_str(), "%u.%u.%u.%u%c%u%c", &i1, &i2, &i3, &i4, &midChar, &port, &endChar);
    if (n == 6) {
        if (!allowPort || midChar != ':' || port == 0 || port > 65535 || port < minimumPort)
            return false;
    }
    else if (n == 4) {
        if (requirePort)
            return false;
    }
    else
        return false;
    return i1 < 256 && i2 < 256 && i3 < 256 && i4 < 256 && i1 + i2 + i3 + i4 != 0;
}

bool check_private_IP(const string& address, bool allowAnyExternal) {
    int i1, i2;
    const int n = sscanf(address.c_str(), "%d.%d.", &i1, &i2);
    nAssert(n == 2);
    if (n != 2)
        return true;
    // private IP ranges:
    //  10.  0.0.0  -   10.255.255.255
    // 172. 16.0.0  -  172. 31.255.255
    // 192.168.0.0  -  192.168.255.255
    // 169.254.0.0  -  169.254.255.255 [used by Microsoft DHCP client]
    // 127.  0.0.0  -  127.255.255.255 [loopback]
    if (i1 == 127)
        return true;
    if (allowAnyExternal)
        return false;
    return (i1 == 10 || (i1 == 172 && i2 >= 16 && i2 <= 31) || (i1 == 192 && i2 == 168) || (i1 == 169 && i2 == 254));
}

string getPublicIP(LineReceiver& output, bool allowAnyExternal) {
    const vector<Network::Address> locals = Network::getAllLocalAddresses();
    for (vector<Network::Address>::const_iterator li = locals.begin(); li != locals.end(); ++li) {
        const string addr = li->toString();
        if (check_private_IP(addr, allowAnyExternal))
            output("Local address " + addr + " ignored");
        else {
            output("Found public address "  + addr);
            return addr;
        }
    }
    output("No public address found");
    return string();
}

bool isLocalIP(Network::Address address) { // local doesn't mean private
    address.setPort(0);
    if (address == Network::Address("127.0.0.1"))
        return true;
    const vector<Network::Address> locals = Network::getAllLocalAddresses();
    for (vector<Network::Address>::const_iterator li = locals.begin(); li != locals.end(); ++li)
        if (address == *li)
            return true;
    return false;
}

NetworkResult writeToUnblockingTCP(Network::Socket& socket, const char* data, int length, const volatile bool* abortFlag, int timeout, int roundDelay) {
    int at = 0;
    int tries = 0;
    while (at < length) {
        if ((abortFlag && *abortFlag) || tries * roundDelay > timeout)
            return NR_timeout;

        int written;
        if (!socket.write(data + at, length - at, &written)) {
            if (nlGetError() != NL_CON_PENDING)
                return NR_nlError;
        }
        else
            at += written;

        platSleep(roundDelay);
        ++tries;
    }
    return NR_ok;
}

NetworkResult saveAllFromUnblockingTCP(Network::Socket& socket, ostream& out, const volatile bool* abortFlag, int timeout, int roundDelay) {
    const int buffer_size = 511;
    char lebuf[buffer_size + 1];

    int tries = 0;
    for (;;) {
        if ((abortFlag && *abortFlag) || tries * roundDelay > timeout)
            return NR_timeout;

        int read = socket.read(lebuf, buffer_size);
        if (read == Network::Error) {
            if (nlGetError() != NL_CON_PENDING)
                return (nlGetError() == NL_MESSAGE_END) ? NR_ok : NR_nlError;
        }
        else {
            lebuf[read] = '\0';
            out << lebuf;
        }

        platSleep(roundDelay);
        ++tries;
    }
}

string format_http_parameters(const map<string, string>& parameters) {
    // URL encode parameter values
    ostringstream param_line;
    for (map<string, string>::const_iterator i = parameters.begin(); i != parameters.end(); i++) {
        if (i != parameters.begin())
            param_line << '&';
        for (string::const_iterator s = i->first.begin(); s != i->first.end(); s++)
            url_encode(*s, param_line);
        if (!i->second.empty()) {
            param_line << '=';
            for (string::const_iterator s = i->second.begin(); s != i->second.end(); s++)
                url_encode(*s, param_line);
        }
    }
    return param_line.str();
}

string build_http_request(bool post, const string& host, const string& script, const string& parameters, const string& auth) {
    ostringstream data;

    data << (post ? "POST" : "GET") << ' ' << script;
    if (!post && !parameters.empty())
        data << '?' << parameters;
    data << " HTTP/1.0\r\n";

    data << "Host: " << host << "\r\n";
    data << "User-Agent: Outgun/" << GAME_BRANCH << '-' << getVersionString(false) << "\r\n";
    if (!auth.empty())
        data << "Authorization: Basic " << base64_encode(auth) << "\r\n";
    data << "Connection: close\r\n";
    if (post) {
        nAssert(!parameters.empty());
        data << "Content-Type: application/x-www-form-urlencoded\r\n";
        data << "Content-Length: " << parameters.length() << "\r\n\r\n";
        data << parameters;
    }
    else
        data << "\r\n";
    return data.str();
}

NetworkResult post_http_data(Network::Socket& socket, const volatile bool* abortFlag, int timeout,
                             const string& host, const string& script, const string& parameters, const string& auth) {
    const string request = build_http_request(true, host, script, parameters, auth);
    return writeToUnblockingTCP(socket, request.data(), request.length(), abortFlag, timeout);
}

NetworkResult save_http_response(Network::Socket& socket, ostream& out, const volatile bool* abortFlag, int timeout) {
    return saveAllFromUnblockingTCP(socket, out, abortFlag, timeout);
}

string url_encode(const string& str) {
    ostringstream ost;
    for (string::const_iterator s = str.begin(); s != str.end(); s++)
        url_encode(*s, ost);
    return ost.str();
}

void url_encode(char c, ostream& out) {
    if (is_url_safe(c)) // send safe characters as they are
        out << c;
    else if (c == ' ')  // spaces to + characters
        out << '+';
    else                // encode unsafe characters to %xx
        out << '%' << hex << setw(2) << setfill('0') << static_cast<int>(static_cast<unsigned char>(c));
}

bool is_url_safe(char c) {
    if (c >= 'a' && c <= 'z')
        return true;
    else if (c >= 'A' && c <= 'Z')
        return true;
    else if (c >= '0' && c <= '9')
        return true;
    const string safe_characters("$-_.+!*'(),");
    return safe_characters.find(c) != string::npos;
}

string base64_encode(const string& data) {
    const string conversion_table("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
    const char padding = '=';
    string result;
    // Convert data to 6-bit sequences. Take characters for every sequence
    // from the conversion table.
    for (string::const_iterator s = data.begin(); s != data.end(); s++) {
        // first encoded byte
        char value = (*s >> 2) & 0x3F;
        result += conversion_table[value];
        // second encoded byte
        value = (*s << 4) & 0x3F;
        s++;
        if (s != data.end())
            value |= (*s >> 4) & 0x0F;
        result += conversion_table[value];
        // third encoded byte
        if (s != data.end()) {
            value = (*s << 2) & 0x3F;
            s++;
            if (s != data.end())
                value |= (*s >> 6) & 0x03;
            result += conversion_table[value];
        }
        else
            result += padding;
        // fourth encoded byte
        if (s != data.end()) {
            value = *s & 0x3F;
            result += conversion_table[value];
        }
        else
            result += padding;
    }
    return result;
}
