#ifndef NETWORK_H_INC
#define NETWORK_H_INC

//#define WATCH_CONNECTION
//#define SEND_FRAMEOFFSET

enum Network_data_codes {
	data_name_update,
	data_text_message,
	data_first_packet,
	data_frags_update,
	data_flag_update,
	data_rocket_fire,
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
	data_gameover_hide,
	data_deathbringer,
	data_file_request,
	data_file_download,
	data_file_ack,
	data_registration_token,
	data_registration_response,
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
	data_capture,
	data_kill,
	data_flag_take,
	data_flag_return,
	data_flag_drop,
	data_spawn,
	data_movements_shots
};

#endif
