#include <fstream>
#include "commont.h"
#include "language.h"

using std::ifstream;
using std::map;
using std::string;

Language language;

string Language::gettext(const string& text) const {
    const map<string, string>::const_iterator translation = texts.find(text);
    if (translation == texts.end())
        return text;
    return translation->second;
}

bool Language::load(const string& lang) {
    lang_code = lang;
    texts.clear();
    const string dir = string("config") + directory_separator + "languages" + directory_separator;
    ifstream def((dir + "en.txt").c_str());
    ifstream transl((dir + lang_code + ".txt").c_str());
    string key, value;
    while (getline_smart(def, key) && getline_smart(transl, value))
        texts[key] = value;
    return transl;
}

string _(const string& text) {
    return language.gettext(text);
}

