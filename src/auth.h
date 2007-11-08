/*
 *  auth.h
 *
 *  Copyright (C) 2003, 2004 - Niko Ritari
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

#ifndef __AUTH_H_INCLUDED__
#define __AUTH_H_INCLUDED__

#include "map_with_helpers.h"
#include <iostream>
#include <string>
#include <vector>

#include <ctime>

#include "function_utility.h"
#include "network.h"
#include "utility.h"    // for LogSet

class AuthorizationDatabase {
public:
    struct FileError { // exception
        std::string description;
        FileError(const std::string& d) : description(d) { }
    };

    typedef HookFunctionBase1<bool, const std::string&> SettingChecker;

    void load(SettingChecker& validityChecker) throw(FileError);
    void save() const throw(FileError);

    class AccessDescriptor {
    public:
        class GamemodAccessDescriptor {
            struct ControlLine {
                bool allow;
                std::string name;
                ControlLine(bool allow_, const std::string& name_) : allow(allow_), name(name_) { }
            };
            std::vector<ControlLine> lines;
            bool defaultAllow;

        public:
            GamemodAccessDescriptor(bool defaultAllow_) : defaultAllow(defaultAllow_) { }

            void addLine(bool allow, const std::string& name) { lines.push_back(ControlLine(allow, name)); }
            std::ostream& output(std::ostream& os) const;

            bool isAllowed(const std::string& category, const std::string& setting = std::string()) const;
        };

    private:
        friend void AuthorizationDatabase::load(SettingChecker& validityChecker);
        friend void AuthorizationDatabase::save() const;

        GamemodAccessDescriptor gamemod;
        bool resetAccess;

        bool admin, hasPassword;

        void read(std::istream& in, SettingChecker& validityChecker) throw(FileError);

    public:
        AccessDescriptor() : gamemod(false) { }
        AccessDescriptor(bool allowAll, bool admin_) : gamemod(allowAll), resetAccess(allowAll), admin(admin_), hasPassword(false) { }

        bool isAdmin() const { return admin; }
        bool isProtected() const { return hasPassword; }

        bool canReset() const { return resetAccess; }
        const GamemodAccessDescriptor& gamemodAccess() const { return gamemod; }
    };

private:
    struct NameEntry {
        std::string name;
        std::string password;
        AccessDescriptor access;
        std::string classId;

        NameEntry(const std::string& n, const std::string& p, const AccessDescriptor& a, const std::string& c) : name(n), password(p), access(a), classId(c) { }
    };
    struct BanEntry {
        std::string name;
        Network::Address address;
        time_t endTime;

        BanEntry(const std::string& n, const Network::Address& a, time_t e = time(0) + 365 * 24 * 60 * 60) : name(n), address(a), endTime(e) { }
    };

    std::map<std::string, AccessDescriptor> classes;
    std::vector<NameEntry> names;
    std::vector<BanEntry> bans;

    mutable LogSet log;

    void clear();

    int identifyName(const std::string& name) const;
    static std::string makeComparable(const std::string& name);

public:
    AuthorizationDatabase(LogSet logs) : log(logs) { }

    const AccessDescriptor& defaultAccess() const { return map_get(classes, std::string("user")); }
    const AccessDescriptor& shellAccess() const { return map_get(classes, std::string("shell")); }
    const AccessDescriptor& localAccess() const { return map_get(classes, std::string("local")); }
    AccessDescriptor nameAccess(const std::string& name) const;

    bool checkNamePassword(const std::string& name, const std::string& password) const;

    bool isBanned(Network::Address addr) const;
    void ban(Network::Address addr, const std::string& name, int minutes);
};

inline std::ostream& operator<<(std::ostream& os, const AuthorizationDatabase::AccessDescriptor::GamemodAccessDescriptor& gad) { return gad.output(os); }

#endif
