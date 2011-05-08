/*
 *  tests/mapgen.cpp
 *
 *  Copyright (C) 2011 - Jani Rivinoja
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

#include "../mapgen.h"

int main() {
    const bool verbose = false;
    std::srand(1313131313);
    const int repeats = 10;
    const int maxSize = 8;
    for (int w = 1; w <= maxSize; w++)
        for (int h = 1; h <= maxSize; h++)
            for (int bitmask = 0; bitmask < 0x10; bitmask++)
                for (int i = 0; i < repeats; i++) {
                    int b = 0;
                    const bool overEdge    = bitmask & (1 << b++);
                    const bool respawnArea = bitmask & (1 << b++);
                    const bool greenFlag   = bitmask & (1 << b++);
                    const bool asymmetric  = bitmask & (1 << b++);
                    MapGenerator generator;
                    generator.generate(w, h, overEdge, respawnArea, 0.5, greenFlag, asymmetric);
                    if (verbose)
                        std::cout << '.' << std::flush;
                    bitmask++;
                }
    if (verbose)
        std::cout << '\n';
}
