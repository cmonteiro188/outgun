/*
 *  client_interface.h
 *
 *  Copyright (C) 2003, 2004, 2005 - Niko Ritari
 *  Copyright (C) 2003, 2004 - Jani Rivinoja
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

#ifndef CLIENT_INTERFACE_INC
#define CLIENT_INTERFACE_INC

#include <string>

#include <nl.h>

class Client;
class ServerExternalSettings;
class Log;
class MemoryLog;

class ClientExternalSettings {
public:
    int winclient;      // windowed client? ; -1 = undefined, 0 = false, 1 = true (-win / -fs)
    int trypageflip;    // try page flipping? ; -1 = undefined, 0 = false, 1 = true (-flip / -dbuf)
    bool forceDefaultGfxMode;
    bool nosound;       // disable sound? -nosound
    int targetfps;      // target (MAX) frames-per-second ; -1 = undefined
    int lowerPriority, priority, networkPriority;   // lower is used for non-timecritical background threads
    int minLocalPort, maxLocalPort; // set to 0 0 to use any available port

    std::string autoPlay;
    std::string autoReplay;
    std::string autoSpectate;

    typedef void StatusOutputFnT(const std::string& str);
    StatusOutputFnT* statusOutput;

    ClientExternalSettings() : winclient(-1), trypageflip(-1), forceDefaultGfxMode(false), nosound(false), targetfps(-1), minLocalPort(0), maxLocalPort(0) { }
};

class ClientInterface {
protected:
    ClientInterface() { }

public:
    static ClientInterface* newClient(const ClientExternalSettings& config, const ServerExternalSettings& serverConfig, Log& clientLog, MemoryLog& externalErrorLog_);

    virtual ~ClientInterface() { }

    virtual bool start() = 0;
    #ifndef DEDICATED_SERVER_ONLY
    virtual void loop(volatile bool* quitFlag, bool firstTimeSplash) = 0;
    #endif
    virtual void stop() = 0;

    virtual void bot_start(const NLaddress& addr, int ping, const std::string& name_lang, int botId) = 0;
    virtual void bot_loop() = 0;
    virtual void set_ping(int ping) = 0;
    virtual bool is_connected() const = 0;
    virtual bool bot_finished() const = 0;

    virtual void set_bot_password(const std::string& pass) = 0;

    virtual int team() const = 0;
};

#endif
