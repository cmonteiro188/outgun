/*
 *  language.cpp
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

#include <fstream>
#include "commont.h"
#include "language.h"

using std::ifstream;
using std::map;
using std::string;

Language language;

string Language::get_text(const string& text) const {
    const map<string, string>::const_iterator translation = texts.find(text);
    if (translation == texts.end())
        return text;
    return translation->second;
}

bool Language::load(const string& lang) {
    lang_code = lang;
    texts.clear();
    const string dir = wheregamedir + "config" + directory_separator + "languages" + directory_separator;
    ifstream def((dir + "en.txt").c_str());
    ifstream transl((dir + lang_code + ".txt").c_str());
    if (!def || !transl)
        return false;
    string key, value;
    while (getline_smart(def, key) && getline_smart(transl, value))
        texts[key] = value;
    return true;
}

string _(const string& text) {
    return language.get_text(text);
}
