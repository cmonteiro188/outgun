/*
 *  version.cpp
 *
 *  Copyright (C) 2008, 2011 - Niko Ritari
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

#include <string>

#include "nassert.h"
#include "version.h"

#include "generated/gitrevision.inc"

using std::string;

const string GAME_BRANCH = "base";
/* To keep the entry in the server list menu nice, GAME_RELEASED_VERSION_SHORT should be at most 7 characters, 8 is borderline acceptable.
 * If at some point this requirement is relaxed, all uses of getVersionString must be checked for their hard limit.
 * It should fully indentify the release (within the GAME_BRANCH). It must not contain spaces.
 */
const string GAME_RELEASED_VERSION_SHORT = "1.0.3";
const string GAME_RELEASED_VERSION = "1.0.3";
const string GAME_COPYRIGHT_YEAR = "2010";

string getVersionString(bool allowSpaces, string::size_type softLimit, string::size_type hardLimit, bool tryHardForSoft) throw () {
    static const string vShort = GAME_RELEASED_VERSION_SHORT, vFull = GAME_RELEASED_VERSION, rev = GIT_REVISION;

    nAssert(!hardLimit || vShort.length() <= hardLimit);
    if (hardLimit && (!softLimit || softLimit > hardLimit))
        softLimit = hardLimit;

    if (rev.empty())
        return allowSpaces && (!softLimit || vFull.length() <= softLimit) ? vFull : vShort;

    /* parse rev with the regexps 'v(.*)()()(-M)?(-E)?' or 'v(.*)-([0-9]*)-g([0-9a-f]*)(-M)?(-E)?'
     * \1 must begin with vShort, and the rest is left in revRest
     * \2 (if any) is stored in nCommits with a leading '-'
     * \3 (if any) is stored in hash with a leading '-'
     * -M and -E set status to Modified and Exported (-M-E is Modified)
     */

    nAssert(rev.find_first_of(' ') == string::npos);
    string revRest = rev;
    enum { Clean, Exported, Modified } status = Clean;
    if (revRest.size() >= 2 && revRest.substr(revRest.size() - 2) == "-E") {
        revRest.erase(revRest.size() - 2);
        status = Exported;
    }
    if (revRest.size() >= 2 && revRest.substr(revRest.size() - 2) == "-M") {
        revRest.erase(revRest.size() - 2);
        status = Modified;
    }

    nAssert(revRest.substr(0, vShort.length() + 1) == 'v' + vShort);
    revRest.erase(0, vShort.length() + 1);

    string nCommits, hash; // both with leading -
    struct IsPlainTag { }; // exception thrown if nCommits and hash aren't there
    try {
        string::size_type hashPos = revRest.find_last_not_of("0123456789abcdef");
        if (hashPos == string::npos || hashPos == 0 || revRest.substr(hashPos - 1, 2) != "-g")
            throw IsPlainTag();
        hash = '-' + revRest.substr(hashPos + 1);
        const string::size_type nCommitsEnd = hashPos - 1;
        string::size_type nCommitsPos = revRest.find_last_not_of("0123456789", nCommitsEnd - 1);
        if (nCommitsPos == string::npos || revRest[nCommitsPos] != '-')
            throw IsPlainTag();
        nCommits = revRest.substr(nCommitsPos, nCommitsEnd - nCommitsPos);
        if (hash.empty() || nCommits.empty())
            throw IsPlainTag();
        revRest.erase(nCommitsPos);
    } catch (IsPlainTag) {
        nCommits.clear();
        hash.clear();
    }

    // return a maximally informative subset of the information within softLimit

    string flags = status == Modified ? "-M" : status == Exported ? "-E" : "";
    string shortFlags = status == Modified ? "M" : status == Exported ? "E" : "";

    if (allowSpaces) {
        const string ver = vFull + revRest + nCommits + hash + flags;
        if (!softLimit || ver.length() <= softLimit)
            return ver;
    }

    const string base = vShort + revRest + nCommits;
    if (!softLimit || base.length() + hash.length() + flags.length() <= softLimit)
        return base + hash + flags;

    // truncate hash
    if (base.length() + 4 + flags.length() <= softLimit)
        return base + hash.substr(0, softLimit - base.length() - flags.length()) + flags;
    if (base.length() + 3 + shortFlags.length() <= softLimit)
        return base + hash.substr(0, softLimit - base.length() - shortFlags.length()) + shortFlags;

    // drop hash
    if (base.length() + flags.length() <= softLimit)
        return base + flags;
    if (base.length() + shortFlags.length() <= softLimit)
        return base + shortFlags;

    // drop nCommits
    flags = (!nCommits.empty() ? "+" : "") + shortFlags;
    if (!nCommits.empty())
        shortFlags = status == Clean ? '+' : 'x';
    if (vShort.length() + revRest.length() + flags.length() <= softLimit)
        return vShort + revRest + flags;
    if (vShort.length() + revRest.length() + shortFlags.length() <= softLimit)
        return vShort + revRest + shortFlags;

    // drop revRest
    if (!revRest.empty())
        shortFlags = status == Clean ? '+' : 'x';
    const string::size_type limit = tryHardForSoft ? softLimit : hardLimit;
    if (!limit || vShort.length() + shortFlags.length() <= limit)
        return vShort + shortFlags;

    return vShort;
}
