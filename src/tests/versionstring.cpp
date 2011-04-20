/*
 *  tests/versionstring.cpp
 *
 *  Copyright (C) 2011 - Niko Ritari
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
#include <iostream>

#include "../version.h"

#include "tests.h"

using namespace std;

struct LengthResult {
    int softLimit;
    const char* result;
};

struct Test {
    const char* release, * relShort, * gitRev, * exportRev;
    bool allowSpaces, tryHardForSoft;
    int hardLimit;
    const char* comment;
    LengthResult result[20];
};

const Test tests[] = {
    { "1.2 beta 3", "1.2b3", "", "$Format:%H-E$", false, true, 0, "uncleanly exported (no Git revision anywhere)",
      { {  1, "1.2b3" },
        {  6, "1.2b3x" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "", "$Format:%H-E$", true, true, 0, "uncleanly exported (no Git revision anywhere) [spaces]",
      { {  1, "1.2b3" },
        {  6, "1.2b3x" },
        { 11, "1.2 beta 3x" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "v1.2b3", "<anything>", false, true, 0, "plain tag from Git",
      { {  1, "1.2b3" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "v1.2b3-M", "<anything>", false, true, 0, "dirty plain tag from Git",
      { {  1, "1.2b3" },
        {  6, "1.2b3M" },
        {  7, "1.2b3-M" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "v1.2b3-4-gfedcba9", "<anything>", false, true, 0, "normal commit from Git",
      { {  1, "1.2b3" },
        {  6, "1.2b3+" },
        {  7, "1.2b3-4" },
        { 10, "1.2b3-4-fe" },
        { 11, "1.2b3-4-fed" },
        { 12, "1.2b3-4-fedc" },
        { 13, "1.2b3-4-fedcb" },
        { 14, "1.2b3-4-fedcba" },
        { 15, "1.2b3-4-fedcba9" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "v1.2b3-45-gfedcba9", "<anything>", false, true, 0, "normal commit from Git",
      { {  1, "1.2b3" },
        {  6, "1.2b3+" },
        {  8, "1.2b3-45" },
        { 11, "1.2b3-45-fe" },
        { 12, "1.2b3-45-fed" },
        { 13, "1.2b3-45-fedc" },
        { 14, "1.2b3-45-fedcb" },
        { 15, "1.2b3-45-fedcba" },
        { 16, "1.2b3-45-fedcba9" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "v1.2b3-45678-gfedcba9", "<anything>", false, true, 0, "normal commit from Git",
      { {  1, "1.2b3" },
        {  6, "1.2b3+" },
        { 11, "1.2b3-45678" },
        { 14, "1.2b3-45678-fe" },
        { 15, "1.2b3-45678-fed" },
        { 16, "1.2b3-45678-fedc" },
        { 17, "1.2b3-45678-fedcb" },
        { 18, "1.2b3-45678-fedcba" },
        { 19, "1.2b3-45678-fedcba9" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "v1.2b3-45-gfedcba9-M", "<anything>", false, true, 0, "dirty normal commit from Git",
      { {  1, "1.2b3" },
        {  6, "1.2b3x" },
        {  7, "1.2b3+M" },
        {  9, "1.2b3-45M" },
        { 10, "1.2b3-45-M" },
        { 12, "1.2b3-45-feM" },
        { 13, "1.2b3-45-fedM" },
        { 14, "1.2b3-45-fed-M" },
        { 15, "1.2b3-45-fedc-M" },
        { 16, "1.2b3-45-fedcb-M" },
        { 17, "1.2b3-45-fedcba-M" },
        { 18, "1.2b3-45-fedcba9-M" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "v1.2b3-45-gfedcba9-M", "<anything>", true, true, 0, "dirty normal commit from Git [spaces]",
      { {  1, "1.2b3" },
        {  6, "1.2b3x" },
        {  7, "1.2b3+M" },
        {  9, "1.2b3-45M" },
        { 10, "1.2b3-45-M" },
        { 12, "1.2b3-45-feM" },
        { 13, "1.2b3-45-fedM" },
        { 14, "1.2b3-45-fed-M" },
        { 15, "1.2b3-45-fedc-M" },
        { 16, "1.2b3-45-fedcb-M" },
        { 17, "1.2b3-45-fedcba-M" },
        { 18, "1.2b3-45-fedcba9-M" },
        { 23, "1.2 beta 3-45-fedcba9-M" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "", "fedcba9876543210123401234567890123456789-E", false, true, 0, "regular export from Git (using git archive)",
      { {  1, "1.2b3" },
        {  6, "1.2b3x" },
        {  7, "1.2b3+E" },
        {  9, "1.2b3-feE" },
        { 10, "1.2b3-fedE" },
        { 11, "1.2b3-fed-E" },
        { 12, "1.2b3-fedc-E" },
        { 13, "1.2b3-fedcb-E" },
        { 14, "1.2b3-fedcba-E" },
        { 15, "1.2b3-fedcba9-E" },
        { 16, "1.2b3-fedcba98-E" },
        { 17, "1.2b3-fedcba987-E" },
        { 18, "1.2b3-fedcba9876-E" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "", "v1.2b3-45-gfedcba9-E", false, true, 0, "revision-added export",
      { {  1, "1.2b3" },
        {  6, "1.2b3x" },
        {  7, "1.2b3+E" },
        {  9, "1.2b3-45E" },
        { 10, "1.2b3-45-E" },
        { 12, "1.2b3-45-feE" },
        { 13, "1.2b3-45-fedE" },
        { 14, "1.2b3-45-fed-E" },
        { 15, "1.2b3-45-fedc-E" },
        { 16, "1.2b3-45-fedcb-E" },
        { 17, "1.2b3-45-fedcba-E" },
        { 18, "1.2b3-45-fedcba9-E" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "", "v1.2b3-45-gfedcba9-M-E", false, true, 0, "revision-added export (dirty)",
      { {  1, "1.2b3" },
        {  6, "1.2b3x" },
        {  7, "1.2b3+M" },
        {  9, "1.2b3-45M" },
        { 10, "1.2b3-45-M" },
        { 12, "1.2b3-45-feM" },
        { 13, "1.2b3-45-fedM" },
        { 14, "1.2b3-45-fed-M" },
        { 15, "1.2b3-45-fedc-M" },
        { 16, "1.2b3-45-fedcb-M" },
        { 17, "1.2b3-45-fedcba-M" },
        { 18, "1.2b3-45-fedcba9-M" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "", "v1.2b3", false, true, 0, "release source archive",
      { {  1, "1.2b3" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "", "v1.2b3", true, true, 0, "release source archive [spaces]",
      { {  1, "1.2b3" },
        { 10, "1.2 beta 3" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "v1.2b3-special", "<anything>", false, true, 0, "plain sub-tag from Git",
      { {  1, "1.2b3" },
        {  6, "1.2b3+" },
        { 13, "1.2b3-special" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "v1.2b3-special-45-gfedcba9", "<anything>", false, true, 0, "normal commit from Git following sub-tag",
      { {  1, "1.2b3" },
        {  6, "1.2b3+" },
        { 14, "1.2b3-special+" },
        { 16, "1.2b3-special-45" },
        { 19, "1.2b3-special-45-fe" },
        { 20, "1.2b3-special-45-fed" },
        { 21, "1.2b3-special-45-fedc" },
        { 22, "1.2b3-special-45-fedcb" },
        { 23, "1.2b3-special-45-fedcba" },
        { 24, "1.2b3-special-45-fedcba9" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "v1.2b3-special-45-gfedcba9", "<anything>", true, true, 0, "normal commit from Git following sub-tag [spaces]",
      { {  1, "1.2b3" },
        {  6, "1.2b3+" },
        { 14, "1.2b3-special+" },
        { 16, "1.2b3-special-45" },
        { 19, "1.2b3-special-45-fe" },
        { 20, "1.2b3-special-45-fed" },
        { 21, "1.2b3-special-45-fedc" },
        { 22, "1.2b3-special-45-fedcb" },
        { 23, "1.2b3-special-45-fedcba" },
        { 24, "1.2b3-special-45-fedcba9" },
        { 29, "1.2 beta 3-special-45-fedcba9" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "", "v1.2b3-special-E", false, true, 0, "revision-added export of sub-tag",
      { {  1, "1.2b3" },
        {  6, "1.2b3x" },
        { 14, "1.2b3-specialE" },
        { 15, "1.2b3-special-E" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "", "v1.2b3-special-45-gfedcba9-E", false, true, 0, "revision-added export following sub-tag",
      { {  1, "1.2b3" },
        {  6, "1.2b3x" },
        { 14, "1.2b3-specialx" },
        { 15, "1.2b3-special+E" },
        { 17, "1.2b3-special-45E" },
        { 18, "1.2b3-special-45-E" },
        { 20, "1.2b3-special-45-feE" },
        { 21, "1.2b3-special-45-fedE" },
        { 22, "1.2b3-special-45-fed-E" },
        { 23, "1.2b3-special-45-fedc-E" },
        { 24, "1.2b3-special-45-fedcb-E" },
        { 25, "1.2b3-special-45-fedcba-E" },
        { 26, "1.2b3-special-45-fedcba9-E" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "v1.2b3-45-gfedcba9", "<anything>", false, true, 8, "normal commit from Git [hard limit]",
      { {  1, "1.2b3" },
        {  6, "1.2b3+" },
        {  8, "1.2b3-45" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "v1.2b3-45-gfedcba9", "<anything>", true, true, 8, "normal commit from Git [hard limit, spaces]",
      { {  1, "1.2b3" },
        {  6, "1.2b3+" },
        {  8, "1.2b3-45" },
        { -1, 0 } } },
    { "1.2 beta 34", "1.2b34", "v1.2b34-56-gfedcba9", "<anything>", false, true, 8, "normal commit from Git [hard limit]",
      { {  1, "1.2b34" },
        {  7, "1.2b34+" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "v1.2b3-45-gfedcba9", "<anything>", false, true, 11, "normal commit from Git [hard limit]",
      { {  1, "1.2b3" },
        {  6, "1.2b3+" },
        {  8, "1.2b3-45" },
        { 11, "1.2b3-45-fe" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "v1.2b3", "<anything>", true, true, 9, "plain tag from Git [hard limit, spaces]",
      { {  1, "1.2b3" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "v1.2b3", "<anything>", true, true, 10, "plain tag from Git [hard limit, spaces]",
      { {  1, "1.2b3" },
        { 10, "1.2 beta 3" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "v1.2b3-45-gfedcba9", "<anything>", true, false, 0, "normal commit from Git [not trying hard for soft, spaces]",
      { {  1, "1.2b3+" },
        {  8, "1.2b3-45" },
        { 11, "1.2b3-45-fe" },
        { 12, "1.2b3-45-fed" },
        { 13, "1.2b3-45-fedc" },
        { 14, "1.2b3-45-fedcb" },
        { 15, "1.2b3-45-fedcba" },
        { 16, "1.2b3-45-fedcba9" },
        { 21, "1.2 beta 3-45-fedcba9" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "v1.2b3-45-gfedcba9", "<anything>", true, false, 8, "normal commit from Git [hard limit, not trying hard for soft, spaces]",
      { {  1, "1.2b3+" },
        {  8, "1.2b3-45" },
        { -1, 0 } } },
    { "1", "1", "v1", "<anything>", false, true, 0, "special: minimal",
      { {  1, "1" },
        { -1, 0 } } },
    { "very long version string", "12345678", "v12345678-special-99-1234-gfedcba9", "<anything>", true, true, 0, "special: ~maximal [spaces]",
      { {  1, "12345678" },
        {  9, "12345678+" },
        { 20, "12345678-special-99+" },
        { 24, "12345678-special-99-1234" },
        { 27, "12345678-special-99-1234-fe" },
        { 28, "12345678-special-99-1234-fed" },
        { 29, "12345678-special-99-1234-fedc" },
        { 30, "12345678-special-99-1234-fedcb" },
        { 31, "12345678-special-99-1234-fedcba" },
        { 32, "12345678-special-99-1234-fedcba9" },
        { 48, "very long version string-special-99-1234-fedcba9" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "v1.2b3-4", "<anything>", false, true, 0, "confusing numerical sub-tag",
      { {  1, "1.2b3" },
        {  6, "1.2b3+" },
        {  7, "1.2b3-4" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "v1.2b3-4-56-gfedcba9", "<anything>", false, true, 0, "confusing numerical sub-tag followed",
      { {  1, "1.2b3" },
        {  6, "1.2b3+" },
        {  8, "1.2b3-4+" },
        { 10, "1.2b3-4-56" },
        { 13, "1.2b3-4-56-fe" },
        { 14, "1.2b3-4-56-fed" },
        { 15, "1.2b3-4-56-fedc" },
        { 16, "1.2b3-4-56-fedcb" },
        { 17, "1.2b3-4-56-fedcba" },
        { 18, "1.2b3-4-56-fedcba9" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "v1.2b3-gfedcba9", "<anything>", false, true, 0, "confusing hash-like sub-tag",
      { {  1, "1.2b3" },
        {  6, "1.2b3+" },
        { 14, "1.2b3-gfedcba9" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "v1.2b3-x45-gfedcba9", "<anything>", false, true, 0, "very confusing sub-tag",
      { {  1, "1.2b3" },
        {  6, "1.2b3+" },
        { 18, "1.2b3-x45-gfedcba9" },
        { -1, 0 } } },
    { "1.2 beta 3", "1.2b3", "v1.2b3-x45-gfedcba9-78-g0123456", "<anything>", false, true, 0, "very confusing sub-tag followed",
      { {  1, "1.2b3" },
        {  6, "1.2b3+" },
        { 19, "1.2b3-x45-gfedcba9+" },
        { 21, "1.2b3-x45-gfedcba9-78" },
        { 24, "1.2b3-x45-gfedcba9-78-01" },
        { 25, "1.2b3-x45-gfedcba9-78-012" },
        { 26, "1.2b3-x45-gfedcba9-78-0123" },
        { 27, "1.2b3-x45-gfedcba9-78-01234" },
        { 28, "1.2b3-x45-gfedcba9-78-012345" },
        { 29, "1.2b3-x45-gfedcba9-78-0123456" },
        { -1, 0 } } },
    { 0, 0, 0, 0, 0, 0, 0, 0, { } }
};

int main() {
    const bool generate = false;

    cout << boolalpha;

    for (int ti = 0; tests[ti].release; ++ti) {
        const Test& t = tests[ti];
        if (generate)
            cout << "    { \"" << t.release << "\", \"" << t.relShort << "\", \"" << t.gitRev << "\", \"" << t.exportRev << "\", " << t.allowSpaces << ", " << t.tryHardForSoft << ", " << t.hardLimit << ", \"" << t.comment << "\",\n      { ";
        string prev;
        int iResult = 0;
        for (int sl = 1; sl <= 80; ++sl) {
            const string v = getVersionString(t.relShort, t.release, t.gitRev, t.exportRev, t.allowSpaces, sl, t.hardLimit, t.tryHardForSoft);
            if (sl == 80)
                nAssert(v == getVersionString(t.relShort, t.release, t.gitRev, t.exportRev, t.allowSpaces,  0, t.hardLimit, t.tryHardForSoft));
            nAssert(!t.hardLimit || (int)v.length() <= t.hardLimit);
            nAssert((int)v.length() <= sl || v == t.relShort || !t.tryHardForSoft && (v == string(t.relShort) + '+' || v == string(t.relShort) + 'x'));
            nAssert(v.length() >= prev.length());
            if (!t.allowSpaces)
                nAssert(v.find_first_of(' ') == string::npos);
            if (v != prev) {
                nAssert((int)v.length() == sl || sl == 1);
                if (generate)
                    cout << "{ " << setw(2) << sl << ", \"" << v << "\" },\n        ";
                else {
                    nAssert(sl == t.result[iResult].softLimit);
                    nAssert(v == t.result[iResult].result);
                    ++iResult;
                }
                prev = v;
            }
        }
        if (generate)
            cout << "{ -1, 0 } } },\n";
        else
            nAssert(!t.result[iResult].result);
    }
    if (generate)
        cout << "    { 0, 0, 0, 0, 0, 0, 0, 0, { } }\n";
    return 0;
}
