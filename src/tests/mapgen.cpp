/*
 *  tests/mapgen.cpp
 *
 *  Copyright (C) 2011 - Jani Rivinoja
 *  Copyright (C) 2012 - Niko Ritari
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

#include <algorithm>
#include <iostream>

#include "../mapgen.h"

using namespace std;

int main() {
    const bool verbose = false, showProgress = false;
    std::srand(1313131313);
    const int baseRepeats = 10, minGoodPerBaseRepeats = 1;
    const int maxSize = 8;
    for (int w = 1; w <= maxSize; w++)
        for (int h = 1; h <= maxSize; h++)
            for (int bitmask = 0; bitmask < 0x10; bitmask++) {
                int b = 0;
                const bool overEdge    = bitmask & (1 << b++);
                const bool respawnArea = bitmask & (1 << b++);
                const bool greenFlag   = bitmask & (1 << b++);
                const bool asymmetric  = bitmask & (1 << b++);
                int nGood = 0;
                const int repeatMul = w == 1 || h == 1 ? 4 : 1;
                const int repeats = baseRepeats * repeatMul;
                for (int i = 0; i < repeats; i++) {
                    MapGenerator generator;
                    if (generator.generate(w, h, overEdge, respawnArea, 0.5, greenFlag, asymmetric))
                        ++nGood;
                    if (showProgress)
                        std::cout << '.' << std::flush;
                }
                nAssert(nGood >= minGoodPerBaseRepeats * repeatMul);
                if (nGood && nGood < repeats / 3 && verbose) {
                    const int s1 = min(w, h), s2 = max(w, h);
                    std::cout << "Rare combo (" << nGood << "): " << s1 << ' ' << s2 << ' ' << greenFlag << asymmetric << overEdge << respawnArea << '\n' << std::flush;
                }
            }
    if (verbose)
        std::cout << '\n';
}
