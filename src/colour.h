/*
 *  colour.h
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

#ifndef COLOUR_H_INC
#define COLOUR_H_INC

#include <string>
#include <vector>

#include "incalleg.h"

class Colour {
public:
    Colour(): r(0), g(0), b(0), col_value(0) { }
    Colour(int r_, int g_, int b_): r(r_), g(g_), b(b_) { update(); }

    void update() { col_value = makecol(r, g, b); }

    operator int() const { return col_value; }

    enum Col_id {
        team_red_basic,
        team_red_light,
        team_red_dark,
        team_red_flash,
        team_blue_basic,
        team_blue_light,
        team_blue_dark,
        team_blue_flash,

        wild_flag,
        wild_flag_flash,

        screen_background,
        flag_pole,
        name,
        name_highlight,
        ground,
        wall,
        map_ground,
        map_wall,
        map_room_border,
        map_pic_room_border,
        map_player_me_1,
        map_player_me_2,
        map_fog,
        map_info_grid,
        map_info_grid_main,

        bar_text,
        bar_0,
        health_100,
        health_200,
        health_300,
        energy_100,
        energy_200,
        energy_300,

        power,
        turbo,
        shadow,
        weapon,
        
        change_message_1,
        change_message_2,
        change_message_delayed,
        fps,
        
        pup_turbo_1,
        pup_turbo_2,
        pup_turbo_3,
        pup_shadow,
        pup_power_1,
        pup_power_2,
        pup_weapon_1,
        pup_weapon_2,
        pup_weapon_3,
        pup_weapon_4,
        pup_health_bg,
        pup_health_border,
        pup_health_cross,
        pup_shield,
        pup_deathbringer,

        deathbringer_smoke,
        deathbringer_carrier_circle,

        power_rocket,
        player_power_team,
        player_power_personal,
        blood,
        rocket_shadow,
        
        ice_cream_crisp,
        ice_cream_ball_1,
        ice_cream_ball_2,
        ice_cream_ball_3,
        ice_cream_text,

        map_time,
        map_loading_1,
        map_loading_2,
        game_draw,
        
        sb_red_bg,
        sb_blue_bg,
        sb_red_line,
        sb_blue_line,
        sb_caption,

        stats_caption_bg,
        stats_text,
        stats_selected,
        stats_highlight,

        text_border,
        message_warning,
        message_team,
        message_info,
        message_header,
        message_server,
        message_normal,
        message_highlight,
        message_input,

        scrollbar,
        scrollbar_bg,

        menu_background,
        menu_border_shadow,
        menu_border_highlight,
        menu_caption,
        menu_caption_bg,
        menu_component_caption,
        menu_active,
        menu_disabled,
        menu_value,
        menu_shortcut_disabled,
        menu_shortcut_enabled,
        
        colours_total
    };

private:
    int r, g, b;
    int col_value;
};

class Colour_manager {
public:
    void init(const std::string& file);
    void update();

    const Colour& operator()(Colour::Col_id key) const;

private:
    std::vector<Colour> colours;
};

#endif // COLOUR_H_INC
