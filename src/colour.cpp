/*
 *  colour.cpp
 *
 *  Copyright (C) 2006 - Jani Rivinoja
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
#include <sstream>

#include "commont.h"
#include "nassert.h"

#include "colour.h"

using std::ifstream;
using std::istringstream;
using std::string;
using std::vector;

struct Colour_setting {
    Colour_setting(const std::string& k, const Colour& c): key(k), col(c) { }
    Colour_setting(const std::string& k, int r, int g, int b): key(k), col(r, g, b) { }
    std::string key;
    Colour col;
};

void Colour_manager::init(const string& file) {
    typedef std::auto_ptr<Colour_setting> PT;
    PT hack(0); // avoid GCC bug http://gcc.gnu.org/bugzilla/show_bug.cgi?id=12883
    // Default colours
    PT settings[] = {
        PT(new Colour_setting("team_red_basic"        , 0xFF, 0x00, 0x00)),
        PT(new Colour_setting("team_red_light"        , 0xFF, 0x55, 0x44)),
        PT(new Colour_setting("team_red_dark"         , 0x66, 0x00, 0x00)),
        PT(new Colour_setting("team_red_flash"        , 0xFF, 0xC8, 0xC8)),
        PT(new Colour_setting("team_blue_basic"       , 0x00, 0x00, 0xFF)),
        PT(new Colour_setting("team_blue_light"       , 0x44, 0x55, 0xFF)),
        PT(new Colour_setting("team_blue_dark"        , 0x00, 0x00, 0x66)),
        PT(new Colour_setting("team_blue_flash"       , 0xC8, 0xC8, 0xFF)),

        PT(new Colour_setting("wild_flag"             , 0x00, 0xFF, 0x00)),
        PT(new Colour_setting("wild_flag_flash"       , 0xC8, 0xFF, 0xC8)),

        PT(new Colour_setting("screen_background"     , 0x00, 0x00, 0x00)),
        PT(new Colour_setting("flag_pole"             , 0xFF, 0xFF, 0x00)),
        PT(new Colour_setting("name"                  , 0xFF, 0xFF, 0xFF)),
        PT(new Colour_setting("name_highlight"        , 0xFF, 0xFF, 0x00)),
        PT(new Colour_setting("self_highlight"        , 0xFF, 0xFF, 0x00)),
        PT(new Colour_setting("ground"                , 0x10, 0x40, 0x00)),
        PT(new Colour_setting("wall"                  , 0x30, 0xC0, 0x00)),
        PT(new Colour_setting("map_ground"            , 0x00, 0x00, 0x00)),
        PT(new Colour_setting("map_wall"              , 0x00, 0x77, 0x00)),
        PT(new Colour_setting("map_room_border"       , 0x30, 0x30, 0x30)),
        PT(new Colour_setting("map_pic_room_border"   , 0x68, 0x68, 0x68)),
        PT(new Colour_setting("map_player_me_1"       , 0xFF, 0xFF, 0x00)),
        PT(new Colour_setting("map_player_me_2"       , 0x00, 0x00, 0x00)),
        PT(new Colour_setting("map_fog"               , 0xFF, 0xFF, 0xFF)),
        PT(new Colour_setting("map_info_grid"         , 0xFF, 0xFF, 0xFF)),
        PT(new Colour_setting("map_info_grid_main"    , 0xFF, 0xFF, 0x00)),
        PT(new Colour_setting("map_info_grid_room"    , 0xFF, 0x00, 0x00)),
        PT(new Colour_setting("room_highlight"        , 0xFF, 0xFF, 0x00)),

        PT(new Colour_setting("bar_text"              , 0xFF, 0xFF, 0xFF)),
        PT(new Colour_setting("bar_0"                 , 0x00, 0x00, 0x00)),
        PT(new Colour_setting("health_100"            , 0xFF, 0x00, 0x00)),
        PT(new Colour_setting("health_200"            , 0xFF, 0xFF, 0x00)),
        PT(new Colour_setting("health_300"            , 0xFF, 0x00, 0xFF)),
        PT(new Colour_setting("energy_100"            , 0x00, 0x00, 0xFF)),
        PT(new Colour_setting("energy_200"            , 0x00, 0xFF, 0x00)),
        PT(new Colour_setting("energy_300"            , 0x7D, 0x64, 0xFF)),

        PT(new Colour_setting("power"                 , 0x00, 0xFF, 0xFF)),
        PT(new Colour_setting("turbo"                 , 0xFF, 0xFF, 0x00)),
        PT(new Colour_setting("shadow"                , 0xFF, 0x00, 0xFF)),
        PT(new Colour_setting("weapon"                , 0xFF, 0xFF, 0xFF)),

        PT(new Colour_setting("change_message_1"      , 0xFF, 0x00, 0x00)),
        PT(new Colour_setting("change_message_2"      , 0xFF, 0xFF, 0xFF)),
        PT(new Colour_setting("change_message_delayed", 0x68, 0x68, 0x68)),
        PT(new Colour_setting("fps"                   , 0x68, 0x68, 0x68)),

        PT(new Colour_setting("pup_turbo_1"           , 0xBF, 0x70, 0x00)),
        PT(new Colour_setting("pup_turbo_2"           , 0xFF, 0xA0, 0x00)),
        PT(new Colour_setting("pup_turbo_3"           , 0xFF, 0xFF, 0x00)),
        PT(new Colour_setting("pup_shadow"            , 0xFF, 0x00, 0xFF)),
        PT(new Colour_setting("pup_power_1"           , 0xFF, 0xFF, 0xFF)),
        PT(new Colour_setting("pup_power_2"           , 0x00, 0xFF, 0xFF)),
        PT(new Colour_setting("pup_weapon_1"          , 0x00, 0xFF, 0x00)),
        PT(new Colour_setting("pup_weapon_2"          , 0x00, 0x00, 0xFF)),
        PT(new Colour_setting("pup_weapon_3"          , 0xFF, 0x00, 0x00)),
        PT(new Colour_setting("pup_weapon_4"          , 0xFF, 0xFF, 0x00)),
        PT(new Colour_setting("pup_health_bg"         , 0xFF, 0xFF, 0xFF)),
        PT(new Colour_setting("pup_health_border"     , 0x00, 0x00, 0x00)),
        PT(new Colour_setting("pup_health_cross"      , 0xFF, 0x00, 0x00)),
        PT(new Colour_setting("pup_shield"            , 0x00, 0xFF, 0x00)),
        PT(new Colour_setting("pup_deathbringer"      , 0x22, 0x33, 0x22)),

        PT(new Colour_setting("deathbringer_smoke"    , 0x00, 0x00, 0x00)),
        PT(new Colour_setting("deathbringer_carrier_circle", 0x00, 0x00, 0x00)),

        PT(new Colour_setting("power_rocket"          , 0xFF, 0xFF, 0xFF)),
        PT(new Colour_setting("player_power_team"     , 0xFF, 0xFF, 0xFF)),
        PT(new Colour_setting("player_power_personal" , 0x00, 0xFF, 0xFF)),
        PT(new Colour_setting("blood"                 , 0xFF, 0x00, 0x00)),
        PT(new Colour_setting("rocket_shadow"         , 0x18, 0x18, 0x18)),

        PT(new Colour_setting("ice_cream_crisp"       , 0xFF, 0xA0, 0x00)),
        PT(new Colour_setting("ice_cream_ball_1"      , 0x00, 0x00, 0xFF)),
        PT(new Colour_setting("ice_cream_ball_2"      , 0xFF, 0x00, 0xFF)),
        PT(new Colour_setting("ice_cream_ball_3"      , 0x00, 0xFF, 0x00)),
        PT(new Colour_setting("ice_cream_text"        , 0xFF, 0xFF, 0xFF)),

        PT(new Colour_setting("map_time"              , 0x00, 0xFF, 0x00)),
        PT(new Colour_setting("map_loading_1"         , 0x00, 0xFF, 0x00)),
        PT(new Colour_setting("map_loading_2"         , 0xFF, 0xA0, 0x00)),
        PT(new Colour_setting("game_draw"             , 0x68, 0x68, 0x68)),

        PT(new Colour_setting("sb_red_bg"             , 0x1A, 0x00, 0x00)),
        PT(new Colour_setting("sb_blue_bg"            , 0x00, 0x00, 0x1A)),
        PT(new Colour_setting("sb_red_line"           , 0x3A, 0x3A, 0x3A)),
        PT(new Colour_setting("sb_blue_line"          , 0x3A, 0x3A, 0x3A)),
        PT(new Colour_setting("sb_caption"            , 0xFF, 0xFF, 0xFF)),

        PT(new Colour_setting("stats_caption_bg"      , 0x00, 0x77, 0x00)),
        PT(new Colour_setting("stats_text"            , 0xFF, 0xFF, 0xFF)),
        PT(new Colour_setting("stats_selected"        , 0xFF, 0xFF, 0x00)),
        PT(new Colour_setting("stats_highlight"       , 0x00, 0xFF, 0x00)),

        PT(new Colour_setting("text_border"           , 0x00, 0x00, 0x00)),
        PT(new Colour_setting("message_warning"       , 0xFF, 0x55, 0x44)),
        PT(new Colour_setting("message_team"          , 0xFF, 0xFF, 0x00)),
        PT(new Colour_setting("message_info"          , 0x00, 0xFF, 0x00)),
        PT(new Colour_setting("message_header"        , 0xAA, 0xFF, 0xFF)),
        PT(new Colour_setting("message_server"        , 0x00, 0xFF, 0xFF)),
        PT(new Colour_setting("message_normal"        , 0xFF, 0xA0, 0x00)),
        PT(new Colour_setting("message_highlight"     , 0xFF, 0xFF, 0xFF)),
        PT(new Colour_setting("message_input"         , 0xFF, 0xFF, 0xFF)),

        PT(new Colour_setting("scrollbar"             , 0x00, 0xFF, 0x00)),
        PT(new Colour_setting("scrollbar_bg"          , 0x00, 0x77, 0x00)),

        PT(new Colour_setting("replay_text"           , 0xFF, 0xFF, 0xFF)),
        PT(new Colour_setting("replay_text_border"    , 0x00, 0x00, 0x00)),
        PT(new Colour_setting("replay_symbol"         , 0x00, 0xFF, 0x00)),
        PT(new Colour_setting("replay_bar"            , 0x00, 0xFF, 0x00)),
        PT(new Colour_setting("replay_bar_bg"         , 0x00, 0x77, 0x00)),

        PT(new Colour_setting("menu_background"       , 0x30, 0x40, 0x30)),
        PT(new Colour_setting("menu_border_shadow"    , 0x50, 0x60, 0x50)),
        PT(new Colour_setting("menu_border_highlight" , 0xA0, 0xB0, 0xA0)),
        PT(new Colour_setting("menu_caption"          , 0xFF, 0xFF, 0xFF)),
        PT(new Colour_setting("menu_caption_bg"       , 0x00, 0x77, 0x00)),
        PT(new Colour_setting("menu_component_caption", 0x40, 0xFF, 0x40)),
        PT(new Colour_setting("menu_active"           , 0xFF, 0xFF, 0x00)),
        PT(new Colour_setting("menu_disabled"         , 0x00, 0xAA, 0x00)),
        PT(new Colour_setting("menu_value"            , 0xFF, 0xFF, 0xFF)),
        PT(new Colour_setting("menu_shortcut_disabled", 0x50, 0x60, 0x50)),
        PT(new Colour_setting("menu_shortcut_enabled" , 0xB0, 0xD0, 0xB0)),

        PT(0)
    };

    colours.clear();
    colours.resize(Colour::colours_total);

    for (unsigned i = 0; &*settings[i] && i < colours.size(); ++i)
        colours[i] = settings[i]->col;

    ifstream in(file.c_str());
    string line;
    while (getline_skip_comments(in, line)) {
        string key, rgb;
        istringstream ist(line);
        ist >> key >> rgb;
        if (ist && (rgb.size() == 6 || rgb.size() == 3)) {
            const int len = rgb.size() / 3;
            int triplet[3];
            for (int i = 0; i < 3; ++i) {
                triplet[i] = strtol(rgb.substr(i * len, len).c_str(), 0, 16);
                if (len == 1)
                    triplet[i] *= 0x11;
            }
            for (unsigned i = 0; &*settings[i] && i < colours.size(); ++i)
                if (settings[i]->key == key) {
                    colours[i] = Colour(triplet[0], triplet[1], triplet[2]);
                    break;
                }
        }
    }
}

void Colour_manager::update() {
    for (vector<Colour>::iterator ci = colours.begin(); ci != colours.end(); ++ci)
        ci->update();
}

const Colour& Colour_manager::operator()(Colour::Col_id key) const {
    nAssert(key >= 0 && key < int(colours.size()));
    return colours[key];
}
