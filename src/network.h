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
    // Socket related enums, declared here to avoid having to use Network::Socket::Something
    enum BlockingMode { Blocking, NonBlocking };
    enum SocketType { UDP, TCP, Broadcast };

    class Address;
    class Socket;

    class Error {
    protected:
        Error() throw () { }

    public:
        virtual ~Error() throw () { }

        virtual std::string str() const throw () = 0;
    };

private:
    class NLError : public Error {
        int sysError;

    protected:
        int nlError;

        NLError() throw ();
        friend class Network;
        friend class Network::Socket;

        std::string basicStr() const throw ();

    public:
        virtual ~NLError() throw () { }

        virtual std::string str() const throw ();
    };

public:
    class InitError : public NLError {
        InitError() throw () { }
        friend class Network;

    public:
        virtual std::string str() const throw ();
    };

    class BadIP : public NLError {
        std::string ip;

        BadIP(const std::string& ip_) throw () : ip(ip_) { }

        friend class Address;

    public:
        ~BadIP() throw () { }
        virtual std::string str() const throw ();
    };

    class ResolveError : public NLError {
        std::string name;

        ResolveError(const std::string& hostname) throw () : name(hostname) { }

        friend class Address;

    public:
        ResolveError() throw ();
        ~ResolveError() throw () { }

        virtual std::string str() const throw ();
    };

    class OpenError : public NLError {
        SocketType type;
        uint16_t port;

        OpenError(SocketType t, uint16_t port_) throw () : type(t), port(port_) { }

        friend class Socket;

    public:
        virtual std::string str() const throw ();
    };

    class ConnectError : public NLError {
        std::string addr;

        ConnectError(const std::string& addr_) throw () : addr(addr_) { }

        friend class Socket;

    public:
        ~ConnectError() throw () { }
        bool connectionRefused() const throw ();
        virtual std::string str() const throw ();
    };

    class ListenError : public NLError {
        ListenError() throw () { }
        friend class Socket;

    public:
        virtual std::string str() const throw ();
    };

    class ReadWriteError : public NLError {
        bool inRead;
        ReadWriteError(bool read) throw () : inRead(read) { }

        friend class Socket;

    public:
        bool connectionRefused() const throw ();
        bool disconnected() const throw ();
        virtual std::string str() const throw ();
    };

    class Timeout : public Error {
        bool inRead;
        Timeout(bool read) throw () : inRead(read) { }

        friend class Socket;

    public:
        virtual std::string str() const throw ();
    };

    class ExternalAbort { };

    class Address {
        class HiddenData;
        HiddenData* hidden;

        Address(HiddenData* h) throw ();

        friend class Network;
        friend class Network::Socket;

    public:
        Address() throw ();
        Address(const Address& a) throw ();
        Address(const std::string& ip) throw (BadIP);
        ~Address() throw ();
        Address& operator=(const Address& a) throw ();

        void clear() throw ();

        bool tryResolve(const std::string& hostname, ResolveError* errorStore = 0) throw (); // if errorStore is set, and an error occurs (false returned), it is saved in *errorStore
        bool fromIP(const std::string& ip) throw ();
        void fromValidIP(const std::string& ip) throw ();

        std::string toString() const throw ();

        void setPort(uint16_t port) throw ();
        uint16_t getPort() const throw ();

        bool valid() const throw ();
        bool operator!() const throw () { return !valid(); }

        bool operator==(const Address& a) const throw ();
        bool operator!=(const Address& a) const throw () { return !(*this == a); }
    };

    class Socket : private NoCopying {
        class HiddenData;
        HiddenData* hidden;
        bool connected;
        bool autoClose;

    public:
        Socket(bool autoClose_ = false) throw ();
        Socket(BlockingMode b, SocketType t, uint16_t port, bool autoClose_ = false) throw (OpenError);
        Socket(TrashableRef<Socket> s) throw (); /// "move constructor": assume the identity of s and clear s
        ~Socket() throw ();

        Socket& operator=(TrashableRef<Socket> s) throw (); /// "move assignment": assume the identity of s and clear s

        bool tryOpen(BlockingMode b, SocketType t, uint16_t port) throw ();
        void open(BlockingMode b, SocketType t, uint16_t port) throw (OpenError);
        void close() throw ();
        void closeIfOpen() throw ();

        bool isOpen() const throw ();
        Address getLocalAddress() const throw (Error);
        Address getRemoteAddress() const throw (Error);
        int getStat(NLenum type) const throw (); //#fix: create an own enum for the type

        void connect(const Address& a) throw (ConnectError);
        bool connectPending() throw (ReadWriteError);
        void listen() throw (ListenError);
        bool acceptConnection(BlockingMode b, Socket& listenerSock) throw ();
        void setRemoteAddress(const Address& a) throw (Error);
        int read(void* buffer, int bufSize) throw (ReadWriteError);
        void write(const void* data, int size, int* writtenSize = 0) throw (ReadWriteError); //#fix: force using writtenSize, then move it to return value

        void writeToUnblockingTCP(const void* data, int length, const volatile bool* abortFlag, int timeout, int roundDelay = 500)
            throw (ReadWriteError, ExternalAbort, Timeout);
        void saveAllFromUnblockingTCP(std::ostream& out, const volatile bool* abortFlag, int timeout, int roundDelay = 500)
            throw (ReadWriteError, ExternalAbort, Timeout);

        void writeToUnblockingTCP(const void* data, int length, int timeout, int roundDelay = 500) throw (ReadWriteError, Timeout);
        void saveAllFromUnblockingTCP(std::ostream& out, int timeout, int roundDelay = 500) throw (ReadWriteError, Timeout);
    };

    // static members only
    static void init() throw (InitError);
    static void shutdown() throw ();
    static std::vector<Address> getAllLocalAddresses() throw ();
    static Address getDefaultLocalAddress() throw (Error);
};

bool isValidIP(const std::string& address, bool allowPort = false, unsigned int minimumPort = 0, bool requirePort = false) throw ();
bool check_private_IP(const std::string& address, bool allowAnyExternal = false) throw ();   // with allowAnyExternal only (invalid and) loopback addresses are blocked
std::string getPublicIP(LineReceiver& log, bool allowAnyExternal = false) throw ();    // with allowAnyExternal only (invalid and) loopback addresses are blocked
bool isLocalIP(Network::Address address) throw ();  // returns true if address points to this machine (nothing to do with the address being private)

inline void readStr(const char* buf, int& count, std::string& dst) throw () {
    dst.clear();
    while (buf[count])
        dst += buf[count++];
    ++count;
}
inline void writeStr(char* buf, int& count, const std::string& src) throw () {
    memcpy(&buf[count], src.data(), src.length());
    count += src.length();
    buf[count++] = '\0';
}

inline double safeReadFloat(const char* buf, int& count) throw () {
    float val;
    readFloat(buf, count, val);
    return val;
}
inline void safeWriteFloat(char* buf, int& count, float val) throw () {  // this adds type safety: val is ensured to be a float
    writeFloat(buf, count, val);
}

std::string format_http_parameters(const std::map<std::string, std::string>& parameters) throw ();

std::string build_http_request(bool post, const std::string& host, const std::string& script, const std::string& parameters = "", const std::string& auth = "") throw ();

void post_http_data(Network::Socket& socket, const volatile bool* abortFlag, int timeout, const std::string& host,
                    const std::string& script, const std::string& parameters, const std::string& auth = "")
    throw (Network::ReadWriteError, Network::ExternalAbort, Network::Timeout); // timeout in ms

void save_http_response(Network::Socket& socket, std::ostream& out, const volatile bool* abortFlag, int timeout)
    throw (Network::ReadWriteError, Network::ExternalAbort, Network::Timeout);   // timeout in ms

void post_http_data(Network::Socket& socket, int timeout, const std::string& host, const std::string& script, const std::string& parameters, const std::string& auth = "")
    throw (Network::ReadWriteError, Network::Timeout); // timeout in ms

void save_http_response(Network::Socket& socket, std::ostream& out, int timeout)
    throw (Network::ReadWriteError, Network::Timeout);   // timeout in ms

std::string url_encode(const std::string& str) throw ();
void url_encode(char c, std::ostream& out) throw ();
bool is_url_safe(char c) throw ();
std::string base64_encode(const std::string& data) throw ();

#endif
