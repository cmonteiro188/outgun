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

#include <iostream>

#include "../version.h"

#include "tests.h"

using namespace std;

int main() {
    string prev;
    for (int as = 1; as >= 0; --as)
        for (int sl = 1; sl <= 80; ++sl) {
            const string v = getVersionString(as, sl, 0, true);
            if (sl == 80)
                nAssert(v == getVersionString(as));
            nAssert((int)v.length() <= sl || v == getVersionString(false, 1, 8, true));
            if (!as)
                nAssert(v.find_first_of(' ') == string::npos);
            if (v != prev) {
                //std::cout << as << ' ' << sl << " '" << v << "'\n";
                prev = v;
            }
        }
    return 0;
}
