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

#include "incalleg.h"
#include <fstream>
#include "commont.h"
#include "utility.h"

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
    texts.clear();
    lang_code = "en";
    if (lang == lang_code)  // English - no need to load the same phrases as in the code.
        return true;
    const string dir = wheregamedir + "config" + directory_separator + "languages" + directory_separator;
    const string defname = dir + "en.txt";
    const string translname = dir + lang + ".txt";
    ifstream def(defname.c_str());
	if (!def) {
        allegro_message("%s not found.\nCan't load a language without the English reference.\nContinuing without a translation.", defname.c_str());
        return false;
    }
    ifstream transl(translname.c_str());
    if (!transl) {
        allegro_message("Language file for '%s' not found (%s).\nContinuing without translation.", lang.c_str(), translname.c_str());
        return false;
    }
    for (;;) {
        string key, value;
        getline_skip_comments(def, key);
        getline_skip_comments(transl, value);
        if (def && transl)
            texts[key] = value;
        else
            break;
    }
    if (def || transl) {
        allegro_message("Language file for '%s' is invalid, maybe for another version of Outgun.\n"
                        "%s contains %s phrases than %s.\n"
                        "Continuing without translation.",
                        lang.c_str(), translname.c_str(), transl ? "more" : "less", defname.c_str());
        texts.clear();
        return false;
    }
    lang_code = lang;
    return true;
}

string _(const string& text) {
    return language.get_text(text);
}

string _(const string& text, const string& t1) {
    string translation = _(text);
    translation = replace_all(translation, "$1", t1);
    return translation;
}

string _(const string& text, const string& t1, const string& t2) {
    string translation = _(text, t1);
    translation = replace_all(translation, "$2", t2);
    return translation;
}
