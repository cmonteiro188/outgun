/*
 *  mappic.cpp
 *
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

#include "commont.h"
#include "graphics.h"
#include "language.h"
#include "world.h"

#include "mappic.h"

using std::string;
using std::vector;

void Mappic::run() {
    find_maps();
    save_pictures();
}

void Mappic::find_maps() {
    smaps = load_maps(SERVER_MAPS_DIR);
}

vector<string> Mappic::load_maps(const string& dir) {
    string searchPattern = wheregamedir + dir + directory_separator + "*.txt";

    log("Map picture saver: scanning for maps: '%s'.", searchPattern.c_str());

    al_ffblk mapffblk;  //for al_find*

    int result = al_findfirst(searchPattern.c_str(), &mapffblk, FA_ARCH | FA_RDONLY);
    vector<string> maps;
    while (result == 0) {
        char nameBuf[500];
        replace_extension(nameBuf, mapffblk.name, "", 500);
        nameBuf[strlen(nameBuf) - 1] = '\0';    //take last damn '.' out

        maps.push_back(nameBuf);
        //try next
        result = al_findnext(&mapffblk);
    }
    al_findclose(&mapffblk);
    log("Map picture saver: %d maps found.", maps.size());
    return maps;
}

void Mappic::save_pictures() const {
    set_color_depth(16);
    Graphics graphics(log);
    graphics.setColors();
    const string dir(wheregamedir + "mappic" + directory_separator);
    for (vector<string>::const_iterator name = smaps.begin(); name != smaps.end(); name++) {
        string picture = dir + *name + ".pcx";
        Map mp;
        if (!mp.load(log, SERVER_MAPS_DIR, *name))
            log.error(_("Map picture saver: Map '$1' is not a valid map file.", *name));
        else if (graphics.save_map_picture(picture, mp))
            log("Map picture saver: Saved map picture to '%s'.", picture.c_str());
        else {
            log.error(_("Map picture saver: Can't save map picture to '$1'.", picture));
            throw Save_error();
        }
    }
}

