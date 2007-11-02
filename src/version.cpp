#include <string>

#include "nassert.h"
#include "version.h"

#include "generated/svnrevision.inc"

using std::string;

string getVersionString(bool allowSpaces, string::size_type softLimit, string::size_type hardLimit, bool tryHardForSoft) {
    static const string vShort = GAME_RELEASED_VERSION_SHORT, vFull = GAME_RELEASED_VERSION, rev = SVN_REVISION;
    string ver;

    nAssert(!hardLimit || vShort.length() <= hardLimit);
    if (hardLimit && (!softLimit || softLimit > hardLimit))
        softLimit = hardLimit;

    if (rev.empty())
        return allowSpaces && (!softLimit || vFull.length() <= softLimit) ? vFull : vShort;

    if (allowSpaces) {
        ver = vFull + " r" + rev;
        if (!softLimit || ver.length() <= softLimit)
            return ver;
    }

    ver = vShort + 'r' + rev;
    if (!softLimit || ver.length() <= softLimit)
        return ver;

    ver = vShort + (rev.find_first_of("MS:") == string::npos ? '+' : 'x');
    const string::size_type limit = tryHardForSoft ? softLimit : hardLimit;
    if (!limit || ver.length() <= limit)
        return ver;

    return vShort;
}
