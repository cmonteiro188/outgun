#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <cassert>
#include <cstdio>

using std::cerr;
using std::cout;
using std::dec;
using std::fixed;
using std::hex;
using std::ostringstream;
using std::setfill;
using std::setw;
using std::setprecision;
using std::string;

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;

// copy from protocol.h, except the pseudo-codes
// replace \(data_[^,]*\) with "\1"

const char* codes[] = {
    "data_name_update",
    "data_text_message",
    "data_first_packet",
    "data_frags_update",
    "data_flag_update",
    "data_rocket_fire",
    "data_old_rocket_visible",
    "data_rocket_delete",
    "data_power_collision",
    "data_score_update",
    "data_sound",
    "data_pup_visible",
    "data_pup_picked",
    "data_pup_timer",
    "data_weapon_change",
    "data_map_change",
    "data_world_reset",
    "data_gameover_show",
    "data_start_game",
    "data_deathbringer",
    "data_file_request",
    "data_file_download",
    "data_file_ack",
    "data_registration_token",
    "data_registration_response",
    "data_tournament_participation",
    "data_crap_update",
    "data_map_time",
    "data_fire_on",
    "data_fire_off",
    "data_suicide",
    "data_drop_flag",
    "data_stop_drop_flag",
    "data_change_team_on",
    "data_change_team_off",
    "data_map_exit_on",
    "data_map_exit_off",
    "data_client_ready",
    "data_map_list",
    "data_map_votes_update",
    "data_map_vote",
    "data_stats",
    "data_team_stats",
    "data_capture",
    "data_kill",
    "data_flag_take",
    "data_flag_return",
    "data_flag_drop",
    "data_players_present",
    "data_new_player",
    "data_spawn",
    "data_movements_shots",
    "data_team_movements_shots",
    "data_fav_colors",
    "data_name_authorization_request",
    "data_server_settings",
    "data_reset_map_list",
    "data_stats_ready",
    "data_player_left",
    "data_team_change",
    "data_5_min_left",
    "data_1_min_left",
    "data_30_s_left",
    "data_time_out",
    "data_extra_time_out",
    "data_normal_time_out",
    "data_too_much_talk",
    "data_mute_notification",
    "data_tournament_update_failed",
    "data_player_mute",
    "data_player_kick",
    "data_disconnecting",
    "data_idlekick_warning",
    "data_map_change_info",
    "data_broken_map",
    "data_current_map",
    "data_bot",
    "data_set_extension_level",
    "data_negotiate_third_party_extensions"
    // todo: support gaps in the list, and therefore the negotiated extension messages
};

string getMessageName(byte type) {
    if (type >= sizeof(codes)/sizeof(codes[0])) {
        ostringstream output;
        output << '(' << int(type) << ')';
        return output.str();
    }
    return codes[type];
}

byte  readByte (const byte* data, int& pos) { byte val = *(data + pos); pos += 1; return val; }
word  readShort(const byte* data, int& pos) { return (readByte(data, pos) << 8) | readByte(data, pos); }
dword readLong (const byte* data, int& pos) { return (readShort(data, pos) << 16) | readShort(data, pos); }

void hexDump(const byte* data, int length) {
    for (int i = 0; i < length; ++i)
        cout << (i == 0 ? "" : " ") << hex << setfill('0') << setw(2) << int(data[i]) << dec << setfill(' ');
}

void analyzePacket(const byte* data, int length, bool fromServer) {
    int pos = 0;
    int id = readLong(data, pos);
    if (id == 0) {
        cout << "Special packet: ";
        hexDump(data + pos, length - pos);
        cout << '\n';
        return;
    }
    ostringstream idss;
    idss << (fromServer ? 'S' : 'C') << id << ' ';
    string ids = idss.str();
    // cout << ids << "ID: " << id << '\n';
    cout << ids << "Ack: " << readLong(data, pos);
    cout << '\n';
    int rel = readByte(data, pos);
    cout << ids << rel << " reliables\n";
    for (int ri = 0; ri < rel; ++ri) {
        int id = readLong(data, pos), len = readShort(data, pos);
        cout << ids << " #" << id << " (" << len << " b): ";
        byte type = readByte(data, pos);
        --len;
        cout << getMessageName(type) << ' ';
        if (pos + len > length)
            cerr << pos << '\t' << len << '\t' << length;
        assert(pos + len <= length);
        hexDump(data + pos, len);
        pos += len;
        cout << '\n';
    }
    cout << ids << length - pos << " bytes frame data: ";
    hexDump(data + pos, length - pos);
    pos = length;
    cout << '\n';
}

int main(int argc, const char* argv[]) {
    static const int ALL_CLIENTS = -2;
    int clientFilter = ALL_CLIENTS;
    if (argc == 3) {
        clientFilter = atoi(argv[1]);
        --argc;
        ++argv;
    }
    if (argc != 2 || clientFilter < -1 || clientFilter >= 32) {
        cerr << "syntax: " << argv[0] << " [clientfilter] logfile\n";
        return 9;
    }
    const char* fileName = argv[1];
    FILE* fp = fopen(fileName, "rb");
    if (!fp) {
        cerr << "Can't read '" << fileName << "'\n";
        return 5;
    }
    bool server = (strstr(fileName, "server") != 0);
    for (;;) {
        char mode;
        double time;
        int client; // only if (server)
        int length;
        fread(&mode, sizeof(char), 1, fp);
        if (feof(fp))
            break;
        fread(&time, sizeof(double), 1, fp);
        if (server)
            fread(&client, sizeof(int), 1, fp);
        fread(&length, sizeof(int), 1, fp);
        if (length <= 0 || length >= 1000) {
            cerr << "Bad data: a packet of " << length << " bytes\n";
            return 1;
        }
        byte* buf = new byte[length == 0 ? 1 : length];
        fread(buf, sizeof(byte), length, fp);
        if (feof(fp)) {
            cerr << "Data truncated\n";
            return 1;
        }
        if (server && clientFilter != ALL_CLIENTS && client != clientFilter)
            continue;
        bool fromServer = (mode == 'R') ^ server;
        cout << setprecision(3) << fixed << time << ' ' << length << " bytes from " << (fromServer ? "server":"client");
        if (server) {
            if (fromServer)
                cout << " to client";
            cout << ' ';
            if (client < 0)
                cout << "(unnumbered)";
            else
                cout << client;
        }
        cout << '\n';
        analyzePacket(buf, length, fromServer);
        delete[] buf;
        cout << '\n';
    }
    return 0;
}
