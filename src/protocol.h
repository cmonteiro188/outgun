/*
 *  protocol.h
 *
 *  Copyright (C) 2004 - Niko Ritari
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

#ifndef PROTOCOL_H_INC
#define PROTOCOL_H_INC

#include "leetnet/server.h"

static const bool WATCH_CONNECTION = false;
static const bool LOG_MESSAGE_TRAFFIC = true;
#define SEND_FRAMEOFFSET    // effects some structures -> must be a define

enum Network_data_code {
    data_name_update,
    data_text_message,
    data_first_packet,
    data_frags_update,
    data_flag_update,
    data_rocket_fire,
    data_old_rocket_visible,
    data_rocket_delete,
    data_score_update,
    data_sound,
    data_pup_visible,
    data_pup_picked,
    data_pup_timer,
    data_weapon_change,
    data_map_change,
    data_world_reset,
    data_gameover_show,
    data_start_game,
    data_deathbringer,
    data_file_request,
    data_file_download,
    data_file_ack,
    data_registration_token,
    data_registration_response,
    data_tournament_participation,
    data_crap_update,
    data_map_time,
    data_fire_on,
    data_fire_off,
    data_suicide,
    data_drop_flag,
    data_stop_drop_flag,
    data_change_team_on,
    data_change_team_off,
    data_map_exit_on,
    data_map_exit_off,
    data_client_ready,
    data_map_list,
    data_map_votes_update,
    data_map_vote,
    data_stats,
    data_team_stats,
    data_capture,
    data_kill,
    data_flag_take,
    data_flag_return,
    data_flag_drop,
    data_new_player,
    data_spawn,
    data_movements_shots,
    data_team_movements_shots,
    data_fav_colors,
    data_name_authorization_request,
    data_server_settings,
    data_reset_map_list,
    data_stats_ready,
    data_reserved_range_first,  // reserve some codes for extensions that are otherwise protocol compatible
    data_reserved_range_last = data_stats_ready + 20    // make sure you don't use more!
};

enum Disconnect_reason {
    disconnect_kick = server_c::disconnect_first_user_defined,
    disconnect_idlekick,
    disconnect_client_misbehavior
};

#endif
