/*
 *  servnet.cpp
 *
 *  Copyright (C) 2002 - Fabio Reis Cecin
 *  Copyright (C) 2003, 2004, 2005, 2006 - Niko Ritari
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007 - Jani Rivinoja
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
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <cctype>

#include "leetnet/server.h"
#include "leetnet/rudp.h"   // get_self_IP
#include "admshell.h"
#include "debug.h"
#include "debugconfig.h"    // for LOG_MESSAGE_TRAFFIC
#include "function_utility.h"
#include "language.h"
#include "nassert.h"
#include "platform.h"
#include "protocol.h"
#include "server.h"
#include "timer.h"

// Delay for the server contacting the master server, in seconds.
// It is good if this delay is set to a minute or so, since this will
// filter out people opening and closing servers frequently.
const double delay_to_report_server = 30.0;

using std::ifstream;
using std::make_pair;
using std::map;
using std::max;
using std::min;
using std::ofstream;
using std::ostream;
using std::ostringstream;
using std::pair;
using std::setfill;
using std::setw;
using std::string;
using std::stringstream;
using std::vector;

// tournament thread job struct
class MasterQuery {
public:
    string request;
    enum JobType { JT_score, JT_login };
    JobType code;
    int cid;
};

ServerNetworking::ServerNetworking(Server* hostp, const Settings& settings_, ServerWorld& w, LogSet logs, bool threadLock_, MutexHolder& threadLockMutex_) :
    threadLock(threadLock_),
    threadLockMutex(threadLockMutex_),
    host(hostp),
    settings(settings_),
    world(w),
    log(logs),
    player_count(0),
    localPlayers(0),
    newUniqueId(0),
    accelerationModeMask(0),
    maplist_revision(0),
    relay_socket(NL_INVALID),
    playerSlotReservationTime(get_time()),
    reservedPlayerSlots(0)
{
    server = 0;
    frameSentTime = 0;  // no meaning
}

ServerNetworking::~ServerNetworking() {
    nlClose(relay_socket);
    if (server) {
        delete server;
        server = 0;
    }
}

void ServerNetworking::upload_next_file_chunk(int i) {
    const int max_chunksize = 128;      // the max chunk size in bytes

    //actual size sent
    int chunksize = fileTransfer[i].data.size() - fileTransfer[i].dp;     //attempt to send remaining...
    if (chunksize > max_chunksize)                          //...but there is the maximum
        chunksize = max_chunksize;

    const NLubyte islast = fileTransfer[i].dp + chunksize == fileTransfer[i].data.size();

    //send
    char lebuf[256]; int count = 0;
    writeByte(lebuf, count, data_file_download);
    writeShort(lebuf, count, static_cast<NLushort>(chunksize));
    writeByte(lebuf, count, islast);
    writeBlock(lebuf, count, fileTransfer[i].data.data() + fileTransfer[i].dp, chunksize);
    server->send_message(i, lebuf, count);

    //save old dp for the ack
    fileTransfer[i].old_dp = fileTransfer[i].dp;

    //inc dp
    fileTransfer[i].dp += chunksize;
}

string ServerNetworking::get_download_file(const string& ftype, const string& fname) {
    if (ftype == "map") {
        if (fname.find_first_of("./:\\") != string::npos) {
            log("Illegal file download attempt: map \"%s\"", fname.c_str());
            return string();
        }

        const string fileName = wheregamedir + SERVER_MAPS_DIR + directory_separator + fname + ".txt";

        FILE* fmap = fopen(fileName.c_str(), "rb");
        if (!fmap) {
            log("Nonexisting map download attempt: map \"%s\"", fname.c_str());
            return string();
        }
        string data;
        while (!feof(fmap)) {
            static const unsigned bufSize = 16000;
            char buf[bufSize];
            const int amount = fread(buf, 1, bufSize, fmap);
            data.append(buf, amount);
        }
        fclose(fmap);
        log("Uploading map \"%s\"", fname.c_str());
        return data;
    }
    else {
        log("Unknown download type \"%s\"", ftype.c_str());
        return string();
    }
}

void ServerNetworking::record_message(const string& msg) const {
    if (host->recording_active()) {
        ostream& out = host->record_stream();
        write(out, static_cast<unsigned>(msg.length()));
        out << msg;
    }
}

void ServerNetworking::record_message(const char* data, int length) const {
    if (host->recording_active()) {
        ostream& out = host->record_stream();
        write(out, length);
        out.write(data, length);
    }
}

void ServerNetworking::broadcast_message(const char* data, int length) const {
    for (int i = 0; i < maxplayers; ++i)
        if (world.player[i].used)
            server->send_message(world.player[i].cid, data, length);
}

void ServerNetworking::send_simple_message(Network_data_code code, int pid) const {
    int count = 0;
    char lebuf[4];
    writeByte(lebuf, count, code);
    server->send_message(world.player[pid].cid, lebuf, count);
}

void ServerNetworking::broadcast_simple_message(Network_data_code code) const {
    int count = 0;
    char lebuf[4];
    writeByte(lebuf, count, code);
    broadcast_message(lebuf, count);
    record_message(lebuf, count);
}

void ServerNetworking::send_me_packet(int pid) const {
    int count = 0;
    char lebuf[1024];
    writeByte(lebuf, count, data_first_packet);
    writeByte(lebuf, count, ((NLubyte)pid));                    // who am I
    writeByte(lebuf, count, ((NLubyte)world.player[pid].color()));
    writeByte(lebuf, count, ((NLubyte)host->current_map_nr())); // current map
    writeByte(lebuf, count, ((NLubyte)world.teams[0].score())); // team 0 current score
    writeByte(lebuf, count, ((NLubyte)world.teams[1].score())); // team 1 current score
    server->send_message(world.player[pid].cid, lebuf, count);
}

// send a player name update to a client (cid = pid_all: to all clients; pid_record: record only)
void ServerNetworking::send_player_name_update(int cid, int pid) const {
    if (world.player[pid].name.empty())
        return;

    char lebuf[256]; int count = 0;
    writeByte(lebuf, count, data_name_update);
    writeByte(lebuf, count, pid);       // what player id
    writeStr(lebuf, count, world.player[pid].name);

    if (cid == pid_record)
        record_message(lebuf, count);
    else if (cid == pid_all) {
        broadcast_message(lebuf, count);
        record_message(lebuf, count);

        if (shellssock != NL_INVALID) {
            char lebuf[256]; int count = 0;
            writeLong(lebuf, count, STA_PLAYER_NAME_UPDATE);
            writeLong(lebuf, count, world.player[pid].cid);
            writeStr(lebuf, count, world.player[pid].name);
            nlWrite(shellssock, lebuf, count);
        }
    }
    else
        server->send_message(cid, lebuf, count);
}

void ServerNetworking::broadcast_player_name(int pid) const {
    send_player_name_update(pid_all, pid);
}

// cid = to who, pid_all = everybody, pid_record = record only; pid = whose crap
void ServerNetworking::send_player_crap_update(int cid, int pid) {
    const ClientData& clid = host->getClientData(world.player[pid].cid);

    char lebuf[256]; int count = 0;
    writeByte(lebuf, count, data_crap_update);

    // --- RECALC CRAP ---
    ClientLoginStatus st;
    st.setToken(clid.token_have);
    st.setMasterAuth(clid.token_have && clid.token_valid);
    st.setTournament(host->tournament_active() && clid.token_have && clid.current_participation);
    st.setLocalAuth(host->isLocallyAuthorized(pid));
    st.setAdmin(host->isAdmin(pid));
    world.player[pid].reg_status = st;

    writeByte(lebuf, count, (NLubyte)pid);
    writeByte(lebuf, count, (NLubyte)world.player[pid].color());
    writeByte(lebuf, count,          world.player[pid].reg_status.toNetwork());
    writeLong(lebuf, count, (NLulong)clid.rank);
    writeLong(lebuf, count, (NLulong)clid.score);
    writeLong(lebuf, count, (NLulong)clid.neg_score);
    writeLong(lebuf, count, (NLulong)max_world_rank);

    if (cid == pid_record)
        record_message(lebuf, count);
    else if (cid == pid_all) {
        record_message(lebuf, count);
        broadcast_message(lebuf, count);
    }
    else
        server->send_message(cid, lebuf, count);
}

void ServerNetworking::broadcast_player_crap(int pid) {
    send_player_crap_update(pid_all, pid);
}

void ServerNetworking::send_acceleration_modes(int pid) const {
    char lebuf[10]; int count = 0;
    writeByte(lebuf, count, data_acceleration_modes);
    writeLong(lebuf, count, accelerationModeMask);
    if (pid != pid_all)
        server->send_message(world.player[pid].cid, lebuf, count);
    else {
        for (int i = 0; i < maxplayers; ++i)
            if (world.player[i].used && world.player[i].protocolExtensionsLevel >= 0)
                server->send_message(world.player[i].cid, lebuf, count);
        record_message(lebuf, count);
    }
}

void ServerNetworking::warnAboutExtensionAdvantage(int pid) const {
    if (world.player[pid].protocolExtensionsLevel < 0)
        player_message(pid, msg_warning, "Warning: This server has extensions enabled that give an advantage over you to players with a supporting Outgun client.");
    else
        send_simple_message(data_extension_advantage, pid);
}

void ServerNetworking::move_update_player(int a) {
    ctop[world.player[a].cid] = a;
}

void ServerNetworking::broadcast_team_change(int from, int to, bool swap) const {
    char lebuf[64]; int count = 0;
    writeByte(lebuf, count, data_team_change);
    writeByte(lebuf, count, static_cast<NLubyte>(from));
    writeByte(lebuf, count, static_cast<NLubyte>(to));
    writeByte(lebuf, count, static_cast<NLubyte>(world.player[to].color()));
    if (swap)
        writeByte(lebuf, count, static_cast<NLubyte>(world.player[from].color()));
    else
        writeByte(lebuf, count, 255);
    broadcast_message(lebuf, count);
    record_message(lebuf, count);
}

void ServerNetworking::broadcast_sample(int code) const {
    char lebuf[64]; int count = 0;
    writeByte(lebuf, count, data_sound);
    writeByte(lebuf, count, static_cast<NLubyte>(code));
    broadcast_message(lebuf, count);
    record_message(lebuf, count);
}

void ServerNetworking::broadcast_screen_sample(int p, int code) const {
    char lebuf[64]; int count = 0;
    writeByte(lebuf, count, data_sound);
    writeByte(lebuf, count, static_cast<NLubyte>(code));
    broadcast_screen_message(world.player[p].roomx, world.player[p].roomy, (char*)lebuf, count);
}

void ServerNetworking::broadcast_screen_power_collision(int p) const {
    char lebuf[64]; int count = 0;
    writeByte(lebuf, count, data_power_collision);
    writeByte(lebuf, count, p);
    broadcast_screen_message(world.player[p].roomx, world.player[p].roomy, (char*)lebuf, count);
}

//send current flag status (cid == pid_all : broadcast)
void ServerNetworking::ctf_net_flag_status(int cid, int team) const {
    //just resetting server state -- no update needed
    if (!server)
        return;

    char lebuf[256]; int count = 0;
    writeByte(lebuf, count, data_flag_update);

    writeByte(lebuf, count, static_cast<NLubyte>(team));    //what team

    // how many flags
    NLubyte size;
    if (team == 2)
        size = static_cast<NLubyte>(world.wild_flags.size());
    else
        size = static_cast<NLubyte>(world.teams[team].flags().size());
    writeByte(lebuf, count, size);

    const vector<Flag>& flags = team == 2 ? world.wild_flags : world.teams[team].flags();
    for (vector<Flag>::const_iterator fi = flags.begin(); fi != flags.end(); ++fi)
        if (fi->carried())  {
            writeByte(lebuf, count, 1); // carried
            //new flag carrier
            writeByte(lebuf, count, static_cast<NLubyte>(fi->carrier()));   //player who took it
        }
        else {
            writeByte(lebuf, count, 0); // not carried
            //new flag position
            writeByte(lebuf, count, static_cast<NLubyte>(fi->position().px));
            writeByte(lebuf, count, static_cast<NLubyte>(fi->position().py));
            writeShort(lebuf, count, static_cast<NLshort>(fi->position().x));
            writeShort(lebuf, count, static_cast<NLshort>(fi->position().y));
        }

    if (cid == pid_all) {
        broadcast_message(lebuf, count);
        record_message(lebuf, count);
    }
    else
        server->send_message(cid, lebuf, count);
}

//update team scores
void ServerNetworking::ctf_update_teamscore(int t) const {
    char lebuf[64]; int count = 0;
    writeByte(lebuf, count, data_score_update);
    writeByte(lebuf, count, static_cast<NLubyte>(t));       // the team
    writeByte(lebuf, count, static_cast<NLubyte>(world.teams[t].score()));  //the score
    broadcast_message(lebuf, count);
    record_message(lebuf, count);
}

void ServerNetworking::broadcast_reset_map_list() {
    ++maplist_revision;
    broadcast_simple_message(data_reset_map_list);
}

void ServerNetworking::broadcast_current_map(int mapNr) const {
    char lebuf[64];
    int count = 0;
    writeByte(lebuf, count, data_current_map);
    writeByte(lebuf, count, mapNr);
    broadcast_message(lebuf, count);
}

// Tell that stats are ready for saving.
void ServerNetworking::broadcast_stats_ready() const {
    broadcast_simple_message(data_stats_ready);
}

void ServerNetworking::broadcast_5_min_left() const {
    broadcast_simple_message(data_5_min_left);
}

void ServerNetworking::broadcast_1_min_left() const {
    broadcast_simple_message(data_1_min_left);
}

void ServerNetworking::broadcast_30_s_left() const {
    broadcast_simple_message(data_30_s_left);
}

void ServerNetworking::broadcast_time_out() const {
    broadcast_simple_message(data_time_out);
}

void ServerNetworking::broadcast_extra_time_out() const {
    broadcast_simple_message(data_extra_time_out);
}

void ServerNetworking::broadcast_normal_time_out(bool sudden_death) const {
    char lebuf[64];
    int count = 0;
    writeByte(lebuf, count, data_normal_time_out);
    writeByte(lebuf, count, sudden_death ? 0x01 : 0x00);
    broadcast_message(lebuf, count);
    record_message(lebuf, count);
}

void ServerNetworking::broadcast_capture(const ServerPlayer& player, int flag_team) const {
    char lebuf[64];
    int count = 0;
    writeByte(lebuf, count, data_capture);
    writeByte(lebuf, count, static_cast<NLubyte>(player.id) | (flag_team == 2 ? 0x80 : 0x00));
    broadcast_message(lebuf, count);
    record_message(lebuf, count);
    if (shellssock != NL_INVALID) {
        char lebuf[256]; int count = 0;
        writeLong(lebuf, count, STA_PLAYER_CAPTURES);
        writeLong(lebuf, count, player.cid);
        nlWrite(shellssock, lebuf, count);
    }
}

void ServerNetworking::broadcast_flag_take(const ServerPlayer& player, int flag_team) const {
    char lebuf[64];
    int count = 0;
    writeByte(lebuf, count, data_flag_take);
    writeByte(lebuf, count, static_cast<NLubyte>(player.id) | (flag_team == 2 ? 0x80 : 0x00));
    broadcast_message(lebuf, count);
    record_message(lebuf, count);
}

void ServerNetworking::broadcast_flag_return(const ServerPlayer& player) const {
    char lebuf[64];
    int count = 0;
    writeByte(lebuf, count, data_flag_return);
    writeByte(lebuf, count, static_cast<NLubyte>(player.id));
    broadcast_message(lebuf, count);
    record_message(lebuf, count);
}

// player dropped the flag on purpose
void ServerNetworking::broadcast_flag_drop(const ServerPlayer& player, int flag_team) const {
    char lebuf[64];
    int count = 0;
    writeByte(lebuf, count, data_flag_drop);
    writeByte(lebuf, count, static_cast<NLubyte>(player.id) | (flag_team == 2 ? 0x80 : 0x00));
    broadcast_message(lebuf, count);
    record_message(lebuf, count);
}

void ServerNetworking::broadcast_kill(const ServerPlayer& attacker, const ServerPlayer& target,
                                      DamageType cause, bool flag, bool wild_flag, bool carrier_defended, bool flag_defended) const {
    char lebuf[64];
    int count = 0;
    writeByte(lebuf, count, data_kill);
    // first byte: deatbringer bit, carrier defended bit, flag defended bit, and attacker id
    NLubyte attacker_info = attacker.id;
    if (cause == DT_deathbringer)
        attacker_info |= 0x80;
    if (carrier_defended)
        attacker_info |= 0x40;
    if (flag_defended)
        attacker_info |= 0x20;
    // second byte: flag bit, wild flag bit, collision bit, and target id
    NLubyte tar_flag = target.id;
    if (flag)
        tar_flag |= 0x80;
    if (wild_flag)
        tar_flag |= 0x40;
    if (cause == DT_collision)
        tar_flag |= 0x20;
    writeByte(lebuf, count, attacker_info);
    writeByte(lebuf, count, tar_flag);
    broadcast_message(lebuf, count);
    record_message(lebuf, count);
    if (shellssock != NL_INVALID) {
        char lebuf[256]; int count = 0;
        if (attacker.used) {
            writeLong(lebuf, count, STA_PLAYER_KILLS);
            writeLong(lebuf, count, attacker.cid);
        }
        if (target.used) {  // should be
            writeLong(lebuf, count, STA_PLAYER_DIES);
            writeLong(lebuf, count, target.cid);
        }
        nlWrite(shellssock, lebuf, count);
    }
}

void ServerNetworking::broadcast_suicide(const ServerPlayer& player, bool flag, bool wild_flag) const {
    char lebuf[64];
    int count = 0;
    writeByte(lebuf, count, data_suicide);
    NLubyte id_flag = player.id;
    if (flag)
        id_flag |= 0x80;
    if (wild_flag)
        id_flag |= 0x40;
    writeByte(lebuf, count, id_flag);
    broadcast_message(lebuf, count);
    record_message(lebuf, count);
    if (shellssock != NL_INVALID) {
        char lebuf[256]; int count = 0;
        writeLong(lebuf, count, STA_PLAYER_DIES);
        writeLong(lebuf, count, player.cid);
        nlWrite(shellssock, lebuf, count);
    }
}

void ServerNetworking::send_waiting_time(const ServerPlayer& player) const {
    nAssert(player.extra_frames_to_respawn >= 0);
    if (player.protocolExtensionsLevel >= 0 && (player.frames_to_respawn < 100 || player.frames_to_respawn > 65535))
        return;
    char lebuf[64];
    int count = 0;
    writeByte(lebuf, count, data_waiting_time);
    writeShort(lebuf, count, static_cast<NLushort>(player.frames_to_respawn));
    server->send_message(player.cid, lebuf, count);
}

void ServerNetworking::record_players_present() const {
    NLulong players_present = 0;
    for (int i = 0; i < maxplayers; i++)
        if (world.player[i].used)
            players_present |= (1 << i);
    char buffer[32]; int count = 0;
    writeByte(buffer, count, data_players_present);
    writeLong(buffer, count, players_present);
    record_message(buffer, count);
}

void ServerNetworking::broadcast_new_player(const ServerPlayer& player) const {
    char lebuf[64];
    int count = 0;
    writeByte(lebuf, count, data_new_player);
    writeByte(lebuf, count, static_cast<NLubyte>(player.id));
    broadcast_message(lebuf, count);
    record_message(lebuf, count);
}

void ServerNetworking::new_player_to_admin_shell(int pid) const {
    if (shellssock != NL_INVALID) {
        char lebuf[256]; int count = 0;
        writeLong(lebuf, count, STA_PLAYER_IP);
        writeLong(lebuf, count, world.player[pid].cid);
        NLaddress addr = get_client_address(world.player[pid].cid);
        nlSetAddrPort(&addr, 0);
        writeStr(lebuf, count, addressToString(addr));
        nlWrite(shellssock, lebuf, count);
    }
}

void ServerNetworking::broadcast_player_left(const ServerPlayer& player) const {
    char lebuf[256]; int count = 0;
    writeByte(lebuf, count, data_player_left);
    writeByte(lebuf, count, static_cast<NLubyte>(player.id));
    broadcast_message(lebuf, count);
    record_message(lebuf, count);
}

void ServerNetworking::broadcast_spawn(const ServerPlayer& player) const {
    char lebuf[64];
    int count = 0;
    writeByte(lebuf, count, data_spawn);
    writeByte(lebuf, count, static_cast<NLubyte>(player.id));
    broadcast_message(lebuf, count);
    record_message(lebuf, count);
}

// Send player's movement and shots to everyone.
void ServerNetworking::broadcast_movements_and_shots(const ServerPlayer& player) const {
    char lebuf[64];
    int count = 0;
    writeByte(lebuf, count, data_movements_shots);
    writeByte(lebuf, count, static_cast<NLubyte>(player.id));
    const Statistics& stats = player.stats();
    writeLong(lebuf, count, static_cast<NLlong>(stats.movement()));
    writeShort(lebuf, count, static_cast<NLshort>(stats.shots()));
    writeShort(lebuf, count, static_cast<NLshort>(stats.hits()));
    writeShort(lebuf, count, static_cast<NLshort>(stats.shots_taken()));
    broadcast_message(lebuf, count);
    record_message(lebuf, count);
}

// Send player's stats to everyone.
void ServerNetworking::broadcast_stats(const ServerPlayer& player) const {
    for (int i = 0; i < maxplayers; i++)
        if (world.player[i].used)
            send_stats(player, world.player[i].cid);
}

// Send everyone's stats to player.
void ServerNetworking::send_stats(const ServerPlayer& player) const {
    for (int i = 0; i < maxplayers; i++)
        if (world.player[i].used)
            send_stats(world.player[i], player.cid);
}

// Send player's stats to client cid.
void ServerNetworking::send_stats(const ServerPlayer& player, int cid) const {
    char lebuf[64];
    int count = 0;
    writeByte(lebuf, count, data_stats);
    writeByte(lebuf, count, static_cast<NLubyte>(player.id) | (player.stats().has_flag() ? 0x80 : 0x00) | (player.stats().has_wild_flag() ? 0x40 : 0x00) | (player.dead ? 0x20 : 0x00));
    const Statistics& stats = player.stats();
    writeByte(lebuf, count, static_cast<NLubyte>(stats.kills()));
    writeByte(lebuf, count, static_cast<NLubyte>(stats.deaths()));
    writeByte(lebuf, count, static_cast<NLubyte>(stats.cons_kills()));
    writeByte(lebuf, count, static_cast<NLubyte>(stats.current_cons_kills()));
    writeByte(lebuf, count, static_cast<NLubyte>(stats.cons_deaths()));
    writeByte(lebuf, count, static_cast<NLubyte>(stats.current_cons_deaths()));
    writeByte(lebuf, count, static_cast<NLubyte>(stats.suicides()));
    writeByte(lebuf, count, static_cast<NLubyte>(stats.captures()));
    writeByte(lebuf, count, static_cast<NLubyte>(stats.flags_taken()));
    writeByte(lebuf, count, static_cast<NLubyte>(stats.flags_dropped()));
    writeByte(lebuf, count, static_cast<NLubyte>(stats.flags_returned()));
    writeByte(lebuf, count, static_cast<NLubyte>(stats.carriers_killed()));
    writeLong(lebuf, count, static_cast<NLlong>(stats.playtime(get_time())));
    writeLong(lebuf, count, static_cast<NLlong>(stats.lifetime(get_time())));
    writeLong(lebuf, count, static_cast<NLlong>(stats.flag_carrying_time(get_time())));
    if (cid == pid_record)
        record_message(lebuf, count);
    else
        server->send_message(cid, lebuf, count);
}

void ServerNetworking::send_team_movements_and_shots(int cid) const {
    char lebuf[256];
    int count = 0;
    writeByte(lebuf, count, data_team_movements_shots);
    for (int i = 0; i < 2; i++) {
        const Team& team = world.teams[i];
        writeLong(lebuf, count, static_cast<NLlong>(team.movement()));
        writeShort(lebuf, count, static_cast<NLshort>(team.shots()));
        writeShort(lebuf, count, static_cast<NLshort>(team.hits()));
        writeShort(lebuf, count, static_cast<NLshort>(team.shots_taken()));
    }
    if (cid == pid_record)
        record_message(lebuf, count);
    else
        server->send_message(cid, lebuf, count);
}

void ServerNetworking::send_team_stats(const ServerPlayer& player) const {
    char lebuf[256];
    int count = 0;
    writeByte(lebuf, count, data_team_stats);
    for (int i = 0; i < 2; i++) {
        const Team& team = world.teams[i];
        writeByte(lebuf, count, static_cast<NLubyte>(team.kills()));
        writeByte(lebuf, count, static_cast<NLubyte>(team.deaths()));
        writeByte(lebuf, count, static_cast<NLubyte>(team.suicides()));
        writeByte(lebuf, count, static_cast<NLubyte>(team.flags_taken()));
        writeByte(lebuf, count, static_cast<NLubyte>(team.flags_dropped()));
        writeByte(lebuf, count, static_cast<NLubyte>(team.flags_returned()));
    }
    server->send_message(player.cid, lebuf, count);
}

void ServerNetworking::send_map_info(const ServerPlayer& player) const {
    int count = 0;
    char lebuf[256];
    writeByte(lebuf, count, data_map_list);
    const MapInfo& map = host->maplist()[player.current_map_list_item];
    writeStr(lebuf, count, map.title);
    writeStr(lebuf, count, map.author);
    writeByte(lebuf, count, static_cast<NLchar>(map.width));
    writeByte(lebuf, count, static_cast<NLchar>(map.height));
    writeByte(lebuf, count, static_cast<NLchar>(map.votes));
    server->send_message(player.cid, lebuf, count);
}

void ServerNetworking::send_map_vote(const ServerPlayer& player) const {
    int count = 0;
    char lebuf[256];
    writeByte(lebuf, count, data_map_vote);
    writeByte(lebuf, count, static_cast<NLbyte>(player.mapVote));
    server->send_message(player.cid, lebuf, count);
}

void ServerNetworking::broadcast_map_votes_update() {
    // check changed votes
    vector<pair<NLchar, NLchar> > votes;    // map number and votes
    NLchar i = 0;
    for (vector<MapInfo>::iterator mi = host->maplist().begin(); mi != host->maplist().end(); ++mi, ++i)
        if (mi->sentVotes != mi->votes) {
            votes.push_back(pair<NLchar, NLchar>(i, mi->votes));
            mi->sentVotes = mi->votes;
        }

    if (votes.empty())
        return;

    // build packet
    int count = 0;
    char lebuf[256];
    writeByte(lebuf, count, data_map_votes_update);
    writeByte(lebuf, count, static_cast<NLchar>(votes.size()));
    for (vector<pair<NLchar, NLchar> >::const_iterator vi = votes.begin(); vi != votes.end(); ++vi) {
        writeByte(lebuf, count, vi->first);
        writeByte(lebuf, count, vi->second);
    }

    // send packet
    broadcast_message(lebuf, count);
}

//send map time and time left
void ServerNetworking::send_map_time(int cid) const {
    const NLulong current_time = world.getMapTime() / 10;
    NLlong time_left;
    if (world.getTimeLeft() <= 0) {
        time_left = world.getExtraTimeLeft() / 10;
        if (time_left < 0)
            time_left = 0;
    }
    else
        time_left = world.getTimeLeft() / 10;
    char lebuf[64]; int count = 0;
    writeByte(lebuf, count, data_map_time);
    writeLong(lebuf, count, current_time);
    writeLong(lebuf, count, time_left);
    if (cid == pid_all) {
        broadcast_message(lebuf, count);
        record_message(lebuf, count);
    }
    else
        server->send_message(cid, lebuf, count);
}

void ServerNetworking::send_server_settings(const ServerPlayer& player) const {
    send_server_settings(player.cid);
}

void ServerNetworking::send_server_settings(int cid) const {
    int count = 0;
    char lebuf[256];
    const WorldSettings& config = world.getConfig();
    const PowerupSettings& pupConfig = world.getPupConfig();
    writeByte(lebuf, count, data_server_settings);
    writeByte(lebuf, count, static_cast<NLubyte>(config.getCaptureLimit()));
    writeByte(lebuf, count, static_cast<NLubyte>(config.getTimeLimit() / 600)); // note: max time 255 mins ~ 4 hours
    writeByte(lebuf, count, static_cast<NLubyte>(config.getExtraTime() / 600));
    NLushort settings = 0;
    int i = 0;
    if (config.balanceTeams())
        settings |= (1 << i);
    i++;
    if (pupConfig.pups_drop_at_death)
        settings |= (1 << i);
    i++;
    if (config.getShadowMinimum() == 0)
        settings |= (1 << i);
    i++;
    if (pupConfig.pup_deathbringer_switch)
        settings |= (1 << i);
    i++;
    if (pupConfig.pup_shield_hits == 1)
        settings |= (1 << i);
    i++;
    settings |= (pupConfig.pup_weapon_max << i);
    i += 4; // 4 bits are required to transfer pup_weapon_max, in range [1, 9]
    nAssert(i <= 16);
    writeShort(lebuf, count, settings);
    writeShort(lebuf, count, pupConfig.pups_min + (pupConfig.pups_min_percentage ? 100 : 0));
    writeShort(lebuf, count, pupConfig.pups_max + (pupConfig.pups_max_percentage ? 100 : 0));
    writeShort(lebuf, count, pupConfig.pup_add_time);
    writeShort(lebuf, count, pupConfig.pup_max_time);
    world.physics.write(lebuf, count);
    /* TODO: 1.0.4 send more settings
       - flag return delay (in frames, two bytes?)
    */
    writeShort(lebuf, count, static_cast<NLushort>(10 * config.flag_return_delay));
    if (cid == pid_record)
        record_message(lebuf, count);
    else
        server->send_message(cid, lebuf, count);
}

//enqueue a job to the master server to update a client's delta score
void ServerNetworking::client_report_status(int id) {
    ClientData& clid = host->getClientData(id);

    if (!clid.token_have || !clid.token_valid)
        return;
    if (clid.delta_score == 0 && clid.neg_delta_score == 0)
        return;

    //submit-- create job
    MasterQuery* job = new MasterQuery();
    job->cid = id;
    job->code = MasterQuery::JT_score;
    job->request = string() +
        "GET /servlet/fcecin.tk1/index.html?" + url_encode(TK1_VERSION_STRING) +
        "&dscp=" + itoa(clid.delta_score) +
        "&dscn=" + itoa(clid.neg_delta_score) +
        "&name=" + url_encode(world.player[ctop[id]].name) +
        "&token=" + url_encode(clid.token) +
        " HTTP/1.0\r\n"
        "Host: www.mycgiserver.com\r\n"
        "\r\n";

    {
        MutexLock ml(mjob_mutex);
        mjob_count++;
    }
    RedirectToMemFun1<ServerNetworking, void, MasterQuery*> rmf(this, &ServerNetworking::run_masterjob_thread);
    Thread::startDetachedThread_assert(rmf, job, settings.lowerPriority());

    clid.delta_score = 0;
    clid.neg_delta_score = 0;
    clid.fdp = 0.0;
    clid.fdn = 0.0;
}

void ServerNetworking::broadcast_team_message(int team, const string& text) const {
    nAssert(text.length() <= max_chat_message_length + maxPlayerNameLength + 2); // 2 = ": "

    char lebuf[256]; int count = 0;
    writeByte(lebuf, count, data_text_message);
    writeByte(lebuf, count, msg_team);
    writeStr(lebuf, count, text);

    for (int i = 0; i < maxplayers; i++)
        if (world.player[i].used && i / TSIZE == team)  // only to teammates
            server->send_message(world.player[i].cid, lebuf, count);

    record_message(lebuf, count);

    //send to the admin shell
    if (shellssock != NL_INVALID) {
        count = 0;
        writeLong(lebuf, count, STA_GAME_TEXT);
        writeByte(lebuf, count, '.');
        writeStr(lebuf, count, text);
        nlWrite(shellssock, lebuf, count);
    }
}

//broadcast message to all players in one screen
void ServerNetworking::broadcast_screen_message(int px, int py, const char* lebuf, int count) const {
    for (int i = 0; i < maxplayers; i++)
        if (world.player[i].used && world.player[i].roomx == px && world.player[i].roomy == py)
            server->send_message(world.player[i].cid, lebuf, count);

    if (host->recording_active()) {
        ostream& out = host->record_stream();
        write(out, count + 2);
        out.write(lebuf, count);
        write(out, static_cast<unsigned char>(px));
        write(out, static_cast<unsigned char>(py));
    }
}

// broadcast message with varargs
void ServerNetworking::bprintf(Message_type type, const char *fs, ...) const {
    va_list argptr;
    char msg[1000];
    va_start(argptr, fs);
    platVsnprintf(msg, 1000, fs, argptr);
    va_end(argptr);

    broadcast_text(type, msg);
}

void ServerNetworking::plprintf(int pid, Message_type type, const char* fmt, ...) const { // bprintf for a single player
    char buf[1000];
    va_list argptr;
    va_start(argptr, fmt);
    platVsnprintf(buf, 1000, fmt, argptr);
    va_end(argptr);
    player_message(pid, type, buf);
}

//send a single message player-printf
void ServerNetworking::player_message(int pid, Message_type type, const string& text) const {
    if (pid >= 0 && !world.player[pid].used)
        return;
    char lebuf[256];
    if (text.length() <= max_chat_message_length + maxPlayerNameLength + 2) {    // 2 = ": "
        int count = 0;
        writeByte(lebuf, count, data_text_message);
        writeByte(lebuf, count, type);
        writeStr(lebuf, count, text);
        if (pid == pid_record)
            record_message(lebuf, count);
        else if (pid == shell_pid) {
            //send to the admin shell
            if (shellssock != NL_INVALID) {
                count = 0;
                writeLong(lebuf, count, STA_GAME_TEXT);
                writeStr(lebuf, count, text);
                nlWrite(shellssock, lebuf, count);
            }
        }
        else if (pid == pid_all) {
            broadcast_message(lebuf, count);
            record_message(lebuf, count);
        }
        else
            server->send_message(world.player[pid].cid, lebuf, count);
    }
    else {
        vector<string> lines = split_to_lines(text, 79, 4); // this makes more sense than splitting to max_chat_message_length and letting it get split again on the client end
        for (vector<string>::const_iterator li = lines.begin(); li != lines.end(); ++li) {
            int count = 0;
            writeByte(lebuf, count, data_text_message);
            writeByte(lebuf, count, type);
            writeStr(lebuf, count, *li);
            if (pid == pid_record)
                record_message(lebuf, count);
            else if (pid == shell_pid) {
                //send to the admin shell
                if (shellssock != NL_INVALID) {
                    count = 0;
                    writeLong(lebuf, count, STA_GAME_TEXT);
                    writeStr(lebuf, count, *li);
                    nlWrite(shellssock, lebuf, count);
                }
            }
            else if (pid == pid_all) {
                broadcast_message(lebuf, count);
                record_message(lebuf, count);
            }
            else
                server->send_message(world.player[pid].cid, lebuf, count);
        }
    }
}

void ServerNetworking::broadcast_text(Message_type type, const string& text) const {
    player_message(pid_all, type, text);
    //send to the admin shell
    if (shellssock != NL_INVALID) {
        char* lebuf = new char[text.length() + 10];
        int count = 0;
        writeLong(lebuf, count, STA_GAME_TEXT);
        writeStr(lebuf, count, text);
        nlWrite(shellssock, lebuf, count);
        delete[] lebuf;
    }
}

void ServerNetworking::send_map_change_message(int pid, int reason, const char* mapname) const {
    char lebuf[256];
    int count = 0;

    //send a show gameover plaque message, if that is the case
    if (reason != NEXTMAP_NONE) {
        writeByte(lebuf, count, data_gameover_show);
        writeByte(lebuf, count, static_cast<NLubyte>(reason));      //capture limit plaque or vote exit plaque
        if (reason == NEXTMAP_CAPTURE_LIMIT || reason == NEXTMAP_VOTE_EXIT) {
            writeByte(lebuf, count, static_cast<NLubyte>(world.teams[0].score()));  //RED team final score
            writeByte(lebuf, count, static_cast<NLubyte>(world.teams[1].score()));  //BLUE team final score
            writeByte(lebuf, count, static_cast<NLubyte>(world.getConfig().getCaptureLimit()));
            writeByte(lebuf, count, static_cast<NLubyte>(world.getConfig().getTimeLimit() / 600)); // note: max time 255 mins ~ 4 hours
        }
        if (pid == pid_record)
            record_message(lebuf, count);
        else if (pid == pid_all) {
            broadcast_message(lebuf, count);
            record_message(lebuf, count);
        }
        else
            server->send_message(world.player[pid].cid, lebuf, count);
    }

    count = 0;
    writeByte(lebuf, count, data_map_change);

    writeShort(lebuf, count, world.map.crc);
    writeString(lebuf, count, mapname);
    writeStr(lebuf, count, world.map.title);
    writeByte(lebuf, count, static_cast<NLubyte>(host->current_map_nr()));
    writeByte(lebuf, count, static_cast<NLubyte>(host->maplist().size()));

    NLbyte remove_flags = 0;
    remove_flags |= (world.map.tinfo[0].flags.empty() ? 0x01 : 0);
    remove_flags |= (world.map.tinfo[1].flags.empty() ? 0x02 : 0);
    remove_flags |= (world.map.wild_flags    .empty() ? 0x04 : 0);
    writeByte(lebuf, count, remove_flags);

    if (pid == pid_all || pid == pid_record) {
        ostringstream ost;
        ost.write(lebuf, count);
        write(ost, static_cast<unsigned>(host->record_map_data().length()));
        ost << host->record_map_data();
        record_message(ost.str());
        if (pid == pid_record)
            return;
        broadcast_message(lebuf, count);
        if (shellssock != NL_INVALID) {
            char lebuf[256]; int count = 0;
            writeLong(lebuf, count, STA_GAME_OVER);
            nlWrite(shellssock, lebuf, count);
            sendTextToAdminShell("Map is " + host->current_map().title);
        }
    }
    else
        server->send_message(world.player[pid].cid, lebuf, count);

    //VERY IMPORTANT: flags the player as "awaiting map load" - client must confirm map to proceed
    if (pid == pid_all) {
        for (int i = 0; i < maxplayers; ++i)
            ++world.player[i].awaiting_client_readies;
    }
    else
        ++world.player[pid].awaiting_client_readies;
}

void ServerNetworking::broadcast_map_change_message(int reason, const char* mapname) const {
    send_map_change_message(pid_all, reason, mapname);
}

void ServerNetworking::broadcast_map_change_info(int votes, int needed, int vote_block_time) const {
    char lebuf[256];
    int count = 0;
    writeByte(lebuf, count, data_map_change_info);
    writeByte(lebuf, count, static_cast<NLubyte>(votes));
    writeByte(lebuf, count, static_cast<NLubyte>(needed));
    writeShort(lebuf, count, static_cast<NLshort>(vote_block_time));

    broadcast_message(lebuf, count);
}

void ServerNetworking::send_too_much_talk(int pid) const {
    send_simple_message(data_too_much_talk, pid);
}

void ServerNetworking::send_mute_notification(int pid) const {
    send_simple_message(data_mute_notification, pid);
}

void ServerNetworking::send_tournament_update_failed(int pid) const {
    send_simple_message(data_tournament_update_failed, pid);
}

void ServerNetworking::broadcast_mute_message(int pid, int mode, const string& admin, bool inform_target) const {
    char lebuf[256];
    int count = 0;
    writeByte(lebuf, count, data_player_mute);
    writeByte(lebuf, count, static_cast<NLubyte>(pid));
    writeByte(lebuf, count, static_cast<NLubyte>(mode));
    writeStr(lebuf, count, admin);

    for (int i = 0; i < maxplayers; i++)
        if (world.player[i].used && (inform_target || i != pid))
            server->send_message(world.player[i].cid, lebuf, count);
}

void ServerNetworking::broadcast_kick_message(int pid, int minutes, const string& admin) const {
    char lebuf[256];
    int count = 0;
    writeByte(lebuf, count, data_player_kick);
    writeByte(lebuf, count, static_cast<NLubyte>(pid));
    writeLong(lebuf, count, static_cast<NLlong>(minutes));
    writeStr(lebuf, count, admin);

    broadcast_message(lebuf, count);
    record_message(lebuf, count);
}

void ServerNetworking::send_idlekick_warning(int pid, int seconds) const {
    char lebuf[256];
    int count = 0;
    writeByte(lebuf, count, data_idlekick_warning);
    writeByte(lebuf, count, static_cast<NLubyte>(seconds));
    server->send_message(world.player[pid].cid, lebuf, count);
}

void ServerNetworking::send_disconnecting_message(int pid, int seconds) const {
    char lebuf[256];
    int count = 0;
    writeByte(lebuf, count, data_disconnecting);
    writeByte(lebuf, count, static_cast<NLubyte>(seconds));
    server->send_message(world.player[pid].cid, lebuf, count);
}

void ServerNetworking::broadcast_broken_map() const {
    broadcast_simple_message(data_broken_map);
}

void ServerNetworking::set_relay_server(const string& address) {
    if (!nlGetAddrFromName(address.c_str(), &relay_address)) {
        log("Relay address could not be resolved from %s.", address.c_str());
        return;
    }
    const NLushort port = nlGetPortFromAddr(&relay_address);
    if (port == 0)
        log("Invalid or missing relay port in %s.", address.c_str());
    else
        log("Relay server set to %s.", address.c_str());
}

string ServerNetworking::get_relay_server() const {
    char buf[NL_MAX_STRING_LENGTH];
    if (!nlAddrToString(&relay_address, buf))
        return string();
    return string(buf);
}

bool ServerNetworking::is_relay_used() const {
    return nlGetPortFromAddr(&relay_address);
}

bool ServerNetworking::is_relay_active() const {
    return relay_socket != NL_INVALID;
}

void ServerNetworking::send_first_relay_data(const string& data) {
    relay_new_game = true;
    if (is_relay_active()) // already sent
        return;
    nlDisable(NL_BLOCKING_IO);
    relay_socket = nlOpen(0, NL_RELIABLE);
    if (relay_socket == NL_INVALID) {
        log("Could not open relay socket.");
        return;
    }
    if (!nlConnect(relay_socket, &relay_address)) {
        log("Could not connect to relay.");
        nlClose(relay_socket);
        relay_socket = NL_INVALID;
        return;
    }
    ostringstream ost;
    write_string(ost, GAME_STRING);
    write_string(ost, "SERVER");
    write(ost, static_cast<unsigned>(data.length()));
    ost << data;

    const NetworkResult result = writeToUnblockingTCP(relay_socket, ost.str().data(), ost.str().length(), &file_threads_quit, 100, 5);
    if (result != NR_ok) {
        log("Could not send init data to the relay: %s", result == NR_timeout ? "Timeout" : getNlErrorString());
        nlClose(relay_socket);
        relay_socket = NL_INVALID;
    }
    else
        log("Init data sent to the relay (%lu bytes).", static_cast<long unsigned>(ost.str().length()));
}

void ServerNetworking::send_relay_data(const string& data) {
    if (!is_relay_active())  // Try again in the next game.
        return;
    //log("Sending relay data.");
    ostringstream ost;
    write(ost, static_cast<char>(relay_new_game ? 1 : 0));
    ost << data;
    relay_new_game = false;

    const unsigned max_buffer_size = 100;
    NLbyte buffer[max_buffer_size];
    const int receive = nlRead(relay_socket, buffer, max_buffer_size);
    if (receive == NL_INVALID) {
        log("Relay disconnected: %s", getNlErrorString());
        nlClose(relay_socket);
        relay_socket = NL_INVALID;
        return;
    }
    const NetworkResult result = writeToUnblockingTCP(relay_socket, ost.str().data(), ost.str().length(), &file_threads_quit, 50, 1);
    if (result != NR_ok) {
        log("Could not send spectator data to the relay: %s", result == NR_timeout ? "Timeout" : getNlErrorString());
        nlClose(relay_socket);
        relay_socket = NL_INVALID;
    }
    //else
        //log("Spectator data sent to the relay (%lu bytes).", static_cast<long unsigned>(data.length()));
}

bool ServerNetworking::start() {
    file_threads_quit = false;

    for (int i = 0; i < 256; ++i)
        ctop[i] = -1;
    player_count = 0;
    bot_count = 0;

    max_world_rank = 0;

    ping_send_client = 0;
    for (int i = 0; i < MAX_PLAYERS; ++i)
        fileTransfer[i].reset();

    server_identification = itoa(abs(rand()));

    // start server
    server = new_server_c(settings.networkPriority(), settings.minLocalPort(), settings.maxLocalPort());

    server->setHelloCallback(sfunc_client_hello);
    server->setConnectedCallback(sfunc_client_connected);
    server->setDisconnectedCallback(sfunc_client_disconnected);
    server->setDataCallback(sfunc_client_data);
    server->setLagStatusCallback(sfunc_client_lag_status);
    server->setPingResultCallback(sfunc_client_ping_result);

    server->setCallbackCustomPointer(this);

    server->set_client_timeout(5, 10);

    if (!server->start(settings.get_port())) {
        log.error(_("Can't start network server on port $1.", itoa(settings.get_port())));
        return false;
    }

    //v0.4.4 reset master jobs count
    mjob_count = 0;
    mjob_exit = false;              //flag for all pending master jobs to quit now
    mjob_fastretry = false;     //flag for all pending master jobs to stop waiting and retry immediately

    shellssock = NL_INVALID;    // not in use

    //start TCP shell master thread in the port number 500 less than server UDP port
    shellmthread.start_assert(RedirectToMemFun1<ServerNetworking, void, int>(this, &ServerNetworking::run_shellmaster_thread), settings.get_port() - 500, settings.lowerPriority());

    //start TCP thread for talking with master server
    mthread.start_assert(RedirectToMemFun0<ServerNetworking, void>(this, &ServerNetworking::run_mastertalker_thread), settings.lowerPriority());

    //start website thread
    webthread.start_assert(RedirectToMemFun0<ServerNetworking, void>(this, &ServerNetworking::run_website_thread), settings.lowerPriority());

    return true;
}

//update serverinfo
void ServerNetworking::update_serverinfo() {
    //v0.4.8 UGLY FIX : count all players again, check for discrepancy
    int pc = 0;
    for (int i = 0; i < maxplayers; i++)
        if (world.player[i].used)
            pc++;
    nAssert(pc == player_count);

    ostringstream info;
    if (!settings.get_server_password().empty())
        info << "P ";
    else if (settings.dedicated())
        info << "D ";
    else
        info << "  ";
    info << setw(2) << get_human_count() << '/' << setw(2) << std::left << maxplayers << std::right << ' ' << setw(7) << GAME_SHORT_VERSION << ' ' << settings.get_hostname();
    server->set_server_info(info.str().c_str());
}

int ServerNetworking::client_connected(int id) {
    addPlayerMutex.lock();

    //2TEAM: check wich team to put player
    int t1 = 0;     //red team count
    int t2 = 0;     //blue team count
    int red_bots = 0, blue_bots = 0;
    for (int i = 0; i < maxplayers; i++)
        if (world.player[i].used) {
            if (i / TSIZE == 0) {
                t1++;
                if (world.player[i].is_bot())
                    red_bots++;
            }
            else {
                t2++;
                if (world.player[i].is_bot())
                    blue_bots++;
            }
        }

    // put on red team, blue team, or randomize if same # of players in both teams
    int targ;
    if (t1 < t2)
        targ = 0;
    else if (t1 > t2)
        targ = TSIZE;
    else {
        if (red_bots > blue_bots)
            targ = 0;
        else if (blue_bots > red_bots)
            targ = TSIZE;
        else {
            host->refresh_team_score_modifiers();
            targ = TSIZE * host->getLessScoredTeam();
        }
    }

    // alloc new player : scans only slots of the team (up from targ)
    int myself = -1;

    for (int i = targ; i < targ + TSIZE; i++)
        if (!world.player[i].used) {
            myself = i;
            break;
        }

    nAssert(myself != -1);
    const int cid = id;
    ctop[cid] = myself;

    // send players_present before "myself" is present, so new_player can be broadcast to "myself" too
    NLulong players_present = 0;
    for (int i = 0; i < maxplayers; i++)
        if (world.player[i].used)
            players_present |= (1 << i);
    char lebuf[8];
    int count = 0;
    writeByte(lebuf, count, data_players_present);
    writeLong(lebuf, count, players_present);
    server->send_message(cid, lebuf, count);

    unsigned uniqueId;
    if (!freedUniqueIds.empty() && freedUniqueIds.front().second < get_time()) {
        uniqueId = freedUniqueIds.front().first;
        freedUniqueIds.pop();
    }
    else
        uniqueId = ++newUniqueId;
    world.player[myself].clear(true, myself, cid, "", myself / TSIZE, uniqueId);

    addPlayerMutex.unlock();

    ++player_count;
    nAssert(reservedPlayerSlots);
    --reservedPlayerSlots;

    world.player[myself].localIP = isLocalIP(get_client_address(cid));
    if (world.player[myself].localIP)
        ++localPlayers;
    else {
        NLaddress ip = get_client_address(id);
        nlSetAddrPort(&ip, 0);
        vector< pair<NLaddress, int> >::iterator pi;
        for (pi = distinctRemotePlayers.begin(); pi != distinctRemotePlayers.end(); ++pi)
            if (nlAddrCompare(&pi->first, &ip)) {
                ++pi->second;
                break;
            }
        if (pi == distinctRemotePlayers.end())
            distinctRemotePlayers.push_back(pair<NLaddress, int>(ip, 1));
    }

    const vector<string>& welcome_message = host->getWelcomeMessage();
    for (vector<string>::const_iterator line = welcome_message.begin(); line != welcome_message.end(); ++line)
        player_message(myself, msg_server, *line);

    send_map_change_message(myself, NEXTMAP_NONE, host->getCurrentMapFile().c_str());

    broadcast_new_player(world.player[myself]);

    // can't abort from this point on... anything that can abort should be above

    world.player[myself].respawn_to_base = true;
    world.respawnPlayer(myself, true);    // move to a spawn spot to wait for the game
    world.resetPlayer(myself, -1e10);   // kill; negative delay to cancel default delay, so that the player spawns as soon as he is ready; no need to tell clients because of suppressing the spawn message above
    world.player[myself].respawn_to_base = true;    // New players always spawn in the base.
    // don't actually spawn until the client has loaded the map and is in the game

    host->resetClient(id);
    world.player[myself].stats().set_lifetime(0);

    //first update the ADMIN SHELL
    if (shellssock != NL_INVALID) {
        char lebuf[256]; int count = 0;
        writeLong(lebuf, count, STA_PLAYER_CONNECTED);
        writeLong(lebuf, count, world.player[myself].cid);
        nlWrite(shellssock, lebuf, count);
    }

    host->check_fav_colors(myself);

    //update the player with world information

    //  - who is he (player #)
    send_me_packet(myself);

    // - world ctf flags information
    ctf_net_flag_status(id, 0);
    ctf_net_flag_status(id, 1);
    ctf_net_flag_status(id, 2);

    // - all other players' names
    // - all other players' frags

    for (int i = 0; i < maxplayers; i++) {
        if (!world.player[i].used)
            continue;
        if (i == myself)
            continue;

        send_player_name_update(id, i);

        //frags update
        char lebuf[256]; int count = 0;
        writeByte(lebuf, count, data_frags_update);
        writeByte(lebuf, count, static_cast<NLubyte>(i));       // what player id
        writeLong(lebuf, count, world.player[i].stats().frags());
        server->send_message(id, lebuf, count);

        send_player_crap_update(id, i);
    }

    send_server_settings(world.player[myself]);
    send_map_time(id);
    send_stats(world.player[myself]);
    send_team_stats(world.player[myself]);

    if (player_count == 2) {
        host->ctf_game_restart();
        world.reset_time();
        sendStartGame();
    }

    host->check_team_changes();
    update_serverinfo();
    world.check_pickup_creation(false);             // check pickup creation
    host->set_check_bots();

    return myself;
}

void ServerNetworking::client_disconnected(int id) {
    if (ctop[id] == -1)
        return;

    //what player
    const int pid = ctop[id];

    const bool was_bot = world.player[pid].is_bot();

    //first update the ADMIN SHELL
    if (shellssock != NL_INVALID) {
        char lebuf[256]; int count;
        count = 0;
        writeLong(lebuf, count, STA_PLAYER_DISCONNECTED);
        writeLong(lebuf, count, world.player[pid].cid);
        nlWrite(shellssock, lebuf, count);
    }

    //report the latest player achievements to the master server
    client_report_status(id);

    if (world.player[pid].localIP)
        --localPlayers;
    else {
        NLaddress ip = get_client_address(id);
        nlSetAddrPort(&ip, 0);
        vector< pair<NLaddress, int> >::iterator pi;
        for (pi = distinctRemotePlayers.begin(); !nlAddrCompare(&pi->first, &ip); ++pi)
            nAssert(pi != distinctRemotePlayers.end());
        --pi->second;
        if (pi->second == 0)
            distinctRemotePlayers.erase(pi);
    }

    freedUniqueIds.push(make_pair(world.player[pid].uniqueId, get_time() + 5 * 60.));

    fileTransfer[id].reset();
    host->game_remove_player(pid, true);
    --player_count;
    if (was_bot)
        --bot_count;

    broadcast_player_left(world.player[pid]);

    host->check_team_changes();
    host->check_map_exit();

    //update serverinfo
    update_serverinfo();

    host->set_check_bots();
}

//client ping result
void ServerNetworking::ping_result(int client_id, int ping_time) {
    if (ctop[client_id] == -1)
        return;
    world.player[ctop[client_id]].ping = ping_time;
}

void ServerNetworking::forwardSayadminMessage(int cid, const string& message) const {
    if (shellssock == NL_INVALID)
        return;
    char lebuf[256];
    int count = 0;
    writeLong(lebuf, count, STA_ADMIN_MESSAGE);
    writeLong(lebuf, count, cid);
    writeStr(lebuf, count, message);
    nlWrite(shellssock, lebuf, count);
}

void ServerNetworking::sendTextToAdminShell(const string& text) const {
    if (shellssock == NL_INVALID)
        return;
    char buf[512];
    int count = 0;
    writeLong(buf, count, STA_GAME_TEXT);
    buf[count++] = '|';
    buf[count++] = ' ';
    writeStr(buf, count, text);
    nlWrite(shellssock, buf, count);
}

bool ServerNetworking::processMessage(int pid, char* const msg, int msglen) {
    ServerPlayer& sender = world.player[pid];

    int count = 0;
    NLubyte code;
    readByte(msg, count, code);
    if (LOG_MESSAGE_TRAFFIC)
        log("Message from client, code = %i", code);
    switch (code) {
    /*break;*/ case data_name_update: {
        string name, password;
        readStr(msg, count, name);
        readStr(msg, count, password);
        host->nameChange(sender.cid, pid, name, password);
        // not related to name update, but this is a convenient place that's always (with a normal client) entered soon after making the connection but after data_set_extension_level
        sender.protocolExtensionsLevelSet = true; // the point is that if we haven't received a data_set_extensions_level so far, that's because the client actually is unextended
        if (sender.protocolExtensionsLevel < 0)
            sender.needSignalFrameExtensions = false; // unextended clients don't need to know, since we aren't using any extensions with them
    }
    break; case data_text_message:
        if (find_nonprintable_char(msg + 1)) {
            log("Received unprintable characters.");
            return false;
        }
        else if (string(msg + 1).length() > max_chat_message_length) {
            log("Received a too long message (%lu characters).", (unsigned long)string(msg + 1).length());
            return false;
        }
        else
            host->chat(pid, msg + 1);
    break; case data_fire_on:
        sender.attackOnce = sender.attack = true;
        sender.attackGunDir = sender.gundir;
    break; case data_fire_off:
        sender.attack = false;
    break; case data_suicide:
        if (!sender.under_deathbringer_effect(get_time()))
            world.suicide(pid);
    break; case data_change_team_on:
        if (!sender.want_change_teams) {
            sender.want_change_teams = true;
            host->check_team_changes();
        }
    break; case data_change_team_off:
        sender.want_change_teams = false;
    break; case data_client_ready:
        #ifdef EXTRA_DEBUG
        nAssert(sender.awaiting_client_readies); // this is an abnormal condition, but it could be the client's fault so normally we can ignore it
        #endif
        if (sender.awaiting_client_readies)
            --sender.awaiting_client_readies;
    break; case data_map_exit_on:
        if (sender.want_map_exit == false) {
            sender.want_map_exit = true;
            // Make sure that this message matches with the one in client.cpp.
            if (host->specific_map_vote_required() && sender.mapVote == -1)
                player_message(pid, msg_server, "Your vote has no effect until you vote for a specific map.");
            host->check_map_exit();
        }
    break; case data_map_exit_off:
        if (sender.want_map_exit == true) {
            sender.want_map_exit = false;
            host->check_map_exit();
        }
    break; case data_file_request: {
        string ftype, fname;
        readStr(msg, count, ftype);
        readStr(msg, count, fname);
        if (fileTransfer[sender.cid].serving_udp_file) {
            log("Another download already in progress.");
            return false;
        }
        else {
            //alloc to download
            fileTransfer[sender.cid].serving_udp_file = true;
            fileTransfer[sender.cid].data = get_download_file(ftype, fname);
            if (fileTransfer[sender.cid].data.empty()) {
                log("Invalid download attempt");
                return false;
            }
            else {
                fileTransfer[sender.cid].dp = 0;
                upload_next_file_chunk(sender.cid);
            }
        }
    }
    break; case data_file_ack:
        if (fileTransfer[sender.cid].dp >= fileTransfer[sender.cid].data.size()) {
            //no more data, this was the last ack. close stuff
            fileTransfer[sender.cid].reset();   //reset the download data structs
            //the client will carry on from here
        }
        else {
            //send next
            upload_next_file_chunk(sender.cid);
        }
    break; case data_registration_token: {
        string tok;
        readStr(msg, count, tok);
        if (host->changeRegistration(sender.cid, tok)) {
            MasterQuery *job = new MasterQuery();
            job->cid = sender.cid;
            job->code = MasterQuery::JT_login;
            job->request = string() +
                    "GET /servlet/fcecin.tk1/index.html?" + url_encode(TK1_VERSION_STRING) +
                    "&chktk" +
                    "&name=" + url_encode(sender.name) +
                    "&token=" + url_encode(tok) +
                    " HTTP/1.0\r\n"
                    "Host: www.mycgiserver.com\r\n"
                    "\r\n";
            {
                MutexLock ml(mjob_mutex);
                mjob_count++;
            }

            RedirectToMemFun1<ServerNetworking, void, MasterQuery*> rmf(this, &ServerNetworking::run_masterjob_thread);
            Thread::startDetachedThread_assert(rmf, job, settings.lowerPriority());
        }
    }
    break; case data_tournament_participation: {
        NLubyte data;
        readByte(msg, count, data);
        ClientData& clid = host->getClientData(sender.cid);
        clid.next_participation = data;
        if (!clid.participation_info_received) {
            clid.current_participation = clid.next_participation;
            clid.participation_info_received = true;
            broadcast_player_crap(pid);
        }
    }
    break; case data_drop_flag:
        sender.drop_key = true;
        sender.dropped_flag = true;
        world.dropFlagIfAny(pid, true);
    break; case data_stop_drop_flag:
        sender.drop_key = false;
    break; case data_map_vote: {
        NLubyte vote;
        readByte(msg, count, vote);
        if (sender.mapVote != vote) {
            if (vote < 255 && vote < static_cast<int>(host->maplist().size()))
                sender.mapVote = vote;
            else {
                sender.mapVote = -1;
                // Make sure that this message matches with the one in client.cpp.
                if (host->specific_map_vote_required() && sender.want_map_exit)
                    player_message(pid, msg_server, "Your vote has no effect until you vote for a specific map.");
            }
            host->check_map_exit();
        }
    }
    break; case data_fav_colors: {
        NLbyte size;
        readByte(msg, count, size);
        vector<char> fav_colors;
        // two colours in a byte
        for (int i = 0; i < size; i++) {
            NLubyte col;
            readByte(msg, count, col);
            int c = (col & 0x0F);
            if (c >= 0 && c < 16)
                fav_colors.push_back(c);
            c = (col >> 4);
            if (++i < size && c >= 0 && c < 16)
                fav_colors.push_back(c);
        }
        host->set_fav_colors(pid, fav_colors);
        broadcast_player_crap(pid);
    }
    break; case data_bot: {
        NLaddress address = get_client_address(sender.cid);
        nlSetAddrPort(&address, 0);
        char buf[NL_MAX_STRING_LENGTH];
        nlAddrToString(&address, buf);
        if (strcmp(buf, "127.0.0.1"))
            log("Remote bot from %s.", buf);
        if (!sender.is_bot()) {
            ++bot_count;
            sender.set_bot();
            update_serverinfo();
        }
    }
    break; case data_set_extension_level: {
        NLubyte level;
        readByte(msg, count, level);
        if (level > PROTOCOL_EXTENSIONS_VERSION) {
            log("Tried to set unknown extension level %d.", level);
            return false;
        }
        sender.protocolExtensionsLevel = level;
        sender.protocolExtensionsLevelSet = true;
        send_simple_message(data_set_extension_level, pid);
        send_acceleration_modes(pid);
    }
    break; case data_set_minimap_player_bandwidth: {
        NLubyte number;
        readByte(msg, count, number);
        sender.minimapPlayersPerFrame = number;
    }
    break; case data_acknowledge_frame_extensions:
        sender.needSignalFrameExtensions = false;
    break; default:
        if (code < data_reserved_range_first || code > data_reserved_range_last) {
            log("Invalid message code: %i, length %i.", code, msglen);
            return false;
        }
        // just ignore commands in reserved range: they're probably some extension we don't have to care about
    }
    return true;
}

//process incoming client data (callback function)
void ServerNetworking::incoming_client_data(int id, char *data, int length) {
    if (ctop[id] == -1)
        return;

    int pid = ctop[id];

    //1. process client's frame data
    int count = 0;

    NLubyte clFrame;
    readByte(data, count, clFrame);
    ServerPlayer& pl = world.player[pid];
    if (WATCH_CONNECTION && pl.lastClientFrame != clFrame) {
        if (static_cast<NLubyte>(pl.lastClientFrame - clFrame) < 128)
            plprintf(pid, msg_warning, "C>S packet order: prev %d this %d", pl.lastClientFrame, clFrame);
        else if (static_cast<NLubyte>(pl.lastClientFrame + 1) != clFrame)
            plprintf(pid, msg_warning, "C>S packet lost : prev %d this %d", pl.lastClientFrame, clFrame);
    }
    if (static_cast<NLubyte>(clFrame - pl.lastClientFrame) < 128) { // this frame is very likely newer or the same as the previous one
        if (clFrame != pl.lastClientFrame) {
            g_timeCounter.refresh(); // we prefer an exact time here
            pl.frameOffset = 10. * (get_time() - frameSentTime);
        }
        pl.lastClientFrame = clFrame;

        NLubyte ccb;
        readByte(data, count, ccb);
        pl.controls.fromNetwork(ccb, false);
        pl.controls.clearModifiersIfIdle();

        GunDirection newDir;
        bool newDirReceived = false;
        if (count < length) {
            NLushort gd;
            readShort(data, count, gd);
            pl.accelerationMode = (gd & 0x800) != 0 && world.physics.allowFreeTurning ? AM_Gun : AM_World;
            newDir.fromNetworkLongForm(gd & 0x7FF);
            newDirReceived = true;
        }
        else
            pl.accelerationMode = AM_World;
        if (!pl.dead) {
            if (world.physics.allowFreeTurning && newDirReceived)
                pl.gundir = newDir;
            else
                if (!pl.controls.isStrafe())
                    pl.gundir.updateFromControls(pl.controls);
        }
    }

    //2. process messages
    for (;;) {
        int msglen;
        char* const msg = server->receive_message(id, &msglen);
        if (msg == 0)
            break;
        if (!processMessage(pid, msg, msglen)) {
            log("Kicked player %d for client misbehavior.", pid);
            host->disconnectPlayer(pid, disconnect_client_misbehavior);
            break;
        }
        pid = ctop[id]; // the message might have affected the pid
    }
    if (!world.player[pid].attackOnce) // if the player did started holding attack before this frame, he wants to shoot in the new direction, otherwise keep the direction when he started
        world.player[pid].attackGunDir = world.player[pid].gundir;
}

void ServerNetworking::removePlayer(int pid) {  // call only when moving players around; this actually does close to nothing
    ctop[world.player[pid].cid] = -1;
}

void ServerNetworking::disconnect_client(int cid, int timeout, Disconnect_reason reason) {
    server->disconnect_client(cid, timeout, reason);
}

void ServerNetworking::sendWorldReset() const {
    broadcast_simple_message(data_world_reset);
}

void ServerNetworking::sendStartGame() const {
    broadcast_simple_message(data_start_game);
    send_map_time(pid_all);
}

void ServerNetworking::writeMinimapPlayerPosition(char* lebuf, int& lecount, int pid) const {
    nAssert(world.player[pid].used);
    const int xmul = 255 / world.map.w;
    const int ymul = 255 / world.map.h;
    const NLubyte mx = world.player[pid].roomx * xmul + static_cast<NLubyte>(xmul * (world.player[pid].lx - 1e-5) / plw);
    const NLubyte my = world.player[pid].roomy * ymul + static_cast<NLubyte>(ymul * (world.player[pid].ly - 1e-5) / plh);
    writeByte(lebuf, lecount, mx);
    writeByte(lebuf, lecount, my);
}

//simulate and broadcast frame
void ServerNetworking::broadcast_frame(bool gameRunning) {
    // check if player acceleration modes have changed
    NLulong newMask = 0;
    if (world.physics.allowFreeTurning)
        for (int i = 0; i < maxplayers; ++i)
            if (world.player[i].used && world.player[i].accelerationMode == AM_Gun)
                newMask |= NLulong(1) << i;
    if (newMask != accelerationModeMask) {
        accelerationModeMask = newMask;
        send_acceleration_modes(pid_all);
    }

    // ============================
    //   build common data buffer
    // ============================
    char lebuf[4096];       //common frame data
    int count = 0;

    //frame
    writeLong(lebuf, count, world.frame);

    //===============================
    //  build packet for each client
    //      with custom data
    //===============================
    int lecount;    //count after "count"

    static int normalViewI[2] = { 0, 0 };   // each team's normal view player iterator
    static int shadowViewI[2] = { 0, 0 };   // each team's shadow view player iterator
    NLulong normalView[2];  // players shown on minimap to each team, without shadow
    NLulong shadowView[2];  // players shown on minimap to each team, with shadow

    for (int t = 0; t < 2; ++t) {
        normalView[t] = shadowView[t] = 0;
        for (int i = 0; i < maxplayers; ++i) {
            if (!world.player[i].used || world.player[i].dead)  // dead enemies aren't seen, dead teammates don't see
                continue;
            if (i / TSIZE == t) {
                normalView[t] |= 1 << i;    // teammates always visible
                const int oppTeamStart = (1 - i / TSIZE) * TSIZE;
                for (int j = oppTeamStart; j < oppTeamStart + TSIZE; ++j)   // find out who this teammate sees (who are in the same room and visible)
                    if (world.player[j].used && !world.player[j].dead &&
                          world.player[j].roomx == world.player[i].roomx && world.player[j].roomy == world.player[i].roomy &&
                          (world.player[j].visibility > 10 || world.player[j].stats().has_flag()))
                        normalView[t] |= 1 << j;
            }
            else if (!world.player[i].item_shadow() || world.player[i].stats().has_flag())
                shadowView[t] += static_cast<NLulong>(1 << i);
        }
        shadowView[t] |= normalView[t];
    }

    // send 2 players' coordinates each frame; pick those two for each team both with and without shadow
    int normalIters[2][2];  // [team][number]
    int shadowIters[2][2];  // [team][number]
    for (int round = 0; round < 2; ++round) {
        for (int t = 0; t < 2; ++t) {
            if (round >= settings.minimapSendLimit()) {
                normalIters[t][round] = shadowIters[t][round] = -1;
                continue;
            }
            if (normalView[t] == 0) // no visible players
                normalViewI[t] = -1;
            else
                do {
                    if (++normalViewI[t] == maxplayers)
                        normalViewI[t] = 0;
                } while (!(normalView[t] & (1 << normalViewI[t])));
            if (shadowView[t] == 0) // no visible players
                shadowViewI[t] = -1;
            else
                do {
                    if (++shadowViewI[t] == maxplayers)
                        shadowViewI[t] = 0;
                } while (!(shadowView[t] & (1 << shadowViewI[t])));
            normalIters[t][round] = normalViewI[t];
            shadowIters[t][round] = shadowViewI[t];
        }
    }
    for (int t = 0; t < 2; ++t) {
        if (normalIters[t][1] == normalIters[t][0])
            normalIters[t][1] = -1;
        if (shadowIters[t][1] == shadowIters[t][0])
            shadowIters[t][1] = -1;
    }

    // ==================================================================
    //   BUILD AND SEND EVERY DAMN PACKET
    // ==================================================================
    for (int i = 0; i < maxplayers; i++) {
        ServerPlayer& recipient = world.player[i];

        if (!recipient.used)
            continue;

        // start writing at end of common data
        lecount = count;

        // first send client prediction synchronization data
        const NLubyte clFrame = recipient.lastClientFrame;
        writeByte(lebuf, lecount, clFrame);

        NLubyte fo = static_cast<NLubyte>(bound<double>(recipient.frameOffset, 0., .999) * 256.);
        // the frame offset field is now hijacked to signal whether extensions are enabled in this frame, for the first few frames until the client is certain to know that future frames are all extended
        if (recipient.needSignalFrameExtensions) {
            if (recipient.protocolExtensionsLevel < 0)
                fo = 127; // 127 marks unextended frames
            else if (fo == 127)
                fo = 128;
        }
        writeByte(lebuf, lecount, fo);

        const bool skip_frame = recipient.awaiting_client_readies || !gameRunning;

        // first byte: player ID, tob bits of health and energy and a bit telling if the rest of the frame is skipped
        NLubyte xtra = i << 3;
        if (iround(recipient.health) & 256)
            xtra |= 1;
        if (iround(recipient.energy) & 256)
            xtra |= 2;
        if (skip_frame)
            xtra |= 4;
        writeByte(lebuf, lecount, xtra);

        // send almost empty frame if client not ready (leave bandwidth for data transfer) or if server showing gameover plaque
        if (!skip_frame) {
            // 2 bytes with the screen of self
            writeByte(lebuf, lecount, static_cast<NLubyte>(recipient.roomx));
            writeByte(lebuf, lecount, static_cast<NLubyte>(recipient.roomy));

            // player data field to indicate which players are on screen (and therefore sent on the frame)
            NLulong players_onscreen = 0;

            // players_onscreen will be written here in the end
            int p_on_count = lecount;
            writeLong(lebuf, lecount, 0);

            for (int j = 0; j < maxplayers; j++) {
                const ServerPlayer& h = world.player[j];
                // player j exists, in same room, visible or in same team or has a flag
                if (h.used && h.roomx == recipient.roomx && h.roomy == recipient.roomy && (h.visibility > 0 || i / TSIZE == j / TSIZE || h.stats().has_flag())) {
                    players_onscreen |= (1 << j);

                    // position in 3 bytes
                    NLubyte xy;
                    NLushort hx, hy;
                    hx = static_cast<NLushort>(h.lx * (double(0xFFF) / plw) + .5);
                    hy = static_cast<NLushort>(h.ly * (double(0xFFF) / plh) + .5);
                    xy = static_cast<NLubyte>(hx & 0x0FF);
                    writeByte(lebuf, lecount, xy);
                    xy = static_cast<NLubyte>(hy & 0x0FF);
                    writeByte(lebuf, lecount, xy);
                    xy = static_cast<NLubyte>( ((hx & 0xF00) >> 8) | ((hy & 0xF00) >> 4) );
                    writeByte(lebuf, lecount, xy);

                    if (recipient.protocolExtensionsLevel < 0) {
                        // speed in 2 bytes
                        typedef SignedByteFloat<3, -2> SpeedType;   // exponent from -2 to +6, with 4 significant bits -> epsilon = .25, max representable 32 * 31 = enough :)
                        writeByte(lebuf, lecount, SpeedType::toByte(h.sx));
                        writeByte(lebuf, lecount, SpeedType::toByte(h.sy));
                    }

                    // flags in 1 byte : dead, has deathbringer, deathbringer-affected, has shield, has turbo, has power
                    NLubyte extra = 0;
                    if (h.dead)
                        extra |= 1;
                    if (h.item_deathbringer)
                        extra |= 2;
                    if (h.deathbringer_end > get_time())
                        extra |= 4;
                    if (h.item_shield)
                        extra |= 8;
                    if (h.item_turbo)
                        extra |= 16;
                    if (h.item_power)
                        extra |= 32;
                    const bool preciseGundir = recipient.protocolExtensionsLevel >= 0 && world.physics.allowFreeTurning;
                    if (preciseGundir)
                        extra |= 64;
                    writeByte(lebuf, lecount, extra);

                    if (!h.dead && recipient.protocolExtensionsLevel >= 0) { // for unextended clients, speed was sent before the extra byte
                        // speed in 2 bytes
                        typedef SignedByteFloat<3, -2> SpeedType;   // exponent from -2 to +6, with 4 significant bits -> epsilon = .25, max representable 32 * 31 = enough :)
                        writeByte(lebuf, lecount, SpeedType::toByte(h.sx));
                        writeByte(lebuf, lecount, SpeedType::toByte(h.sy));
                    }

                    // controls and gundirection in 1 byte
                    NLubyte ccb;
                    if (!h.dead) // if dead player, don't send keys
                        ccb = h.controls.toNetwork(true);
                    else
                        ccb = ClientControls().toNetwork(true);
                    if (preciseGundir) {
                        const NLushort gundir = h.gundir.toNetworkLongForm();
                        ccb |= (gundir >> 8) << 5;
                        writeByte(lebuf, lecount, ccb);
                        ccb = gundir & 0xFF;
                        writeByte(lebuf, lecount, ccb);
                    }
                    else {
                        ccb |= h.gundir.toNetworkShortForm() << 5;
                        writeByte(lebuf, lecount, ccb);
                    }

                    if (!h.dead || recipient.protocolExtensionsLevel < 0) {
                        // visibility in 1 byte
                        const bool safeAfterSpawn = world.frame < h.start_take_damage_frame;
                        if (safeAfterSpawn)
                            writeByte(lebuf, lecount, world.frame & 2 ? 128 : 220);
                        else
                            writeByte(lebuf, lecount, static_cast<NLubyte>(h.visibility));
                    }
                }
            }

            // write players_onscreen in its place (reserved before the above loop)
            writeLong(lebuf, p_on_count, players_onscreen);

            /* minimap player position protocol:
             * old protocol:
             *  2 * {
             *        byte1 = 255 -> no info
             *        byte1 in [0, 31] ->
             *          byte2,
             *          byte3 = coords of player byte1
             *  }
             * extended protocol:
             *  P = bitmask indicating visible players (32 bits long)
             *  to use the minimum amount of bytes, we use
             *   - 3 bits to indicate which 4-bit boundary the sent data begins from
             *   - 2 bits to tell how many extra bytes of mask are sent (1..4)
             *  this leaves us with free 3 bits in byte1 which we use to start the mask with
             *
             *  byte1 & 0xE0 == 4-bit boundary of P to start from
             *  byte1 & 0x18 == extra byte count - 1
             *  byte1 & 0x07 == first 3 bits of P
             *  extra byte count *
             *    byte == next 8 bits of P
             *  for each sent bit of P that's set {
             *    byte1,
             *    byte2 = coords of the player
             *  }
             */
            if (recipient.protocolExtensionsLevel >= 0) {
                NLulong P = (recipient.item_shadow() ? shadowView : normalView)[i / TSIZE];
                for (int pi = 0; pi < maxplayers; ++pi)
                    if (world.player[pi].roomx == recipient.roomx && world.player[pi].roomy == recipient.roomy)
                        P &= ~(NLulong(1) << pi);
                const unsigned maxPlayers = min(settings.minimapSendLimit(), recipient.minimapPlayersPerFrame);
                if (P == 0 || maxPlayers == 0) {
                    writeByte(lebuf, lecount, 0x00); // start from bit 0 (irrelevant), only 1 (mandatory) extra byte
                    writeByte(lebuf, lecount, 0x00);
                }
                else {
                    int nextPlayer = recipient.nextMinimapPlayer;
                    while ((P & (NLulong(1) << nextPlayer)) == 0)
                        nextPlayer = (nextPlayer + 1) % MAX_PLAYERS;
                    const int sendBoundary = nextPlayer & ~3;
                    NLulong rotP = rotateRight(P, sendBoundary);
                    nextPlayer -= sendBoundary; // now nextPlayer is relative to rotP
                    rotP &= ~NLulong(0) << nextPlayer;
                    vector<int> players;
                    players.reserve(maxPlayers);
                    int bits;
                    for (bits = nextPlayer; bits < 32 && players.size() < maxPlayers; ++bits) {
                        if (rotP >> bits == 0)
                            break;
                        if ((rotP >> bits) & 1)
                            players.push_back((bits + sendBoundary) % 32);
                    }
                    recipient.nextMinimapPlayer = (sendBoundary + bits) % 32;
                    rotP &= ~NLulong(0) >> (32 - bits);
                    const int extraBytes = max(1, (bits - 3 + 7) / 8);
                    writeByte(lebuf, lecount, ((sendBoundary / 4) << 5) | ((extraBytes - 1) << 3) | (rotP & 7));
                    rotP >>= 3;
                    for (int eb = 0; eb < extraBytes; ++eb) {
                        writeByte(lebuf, lecount, rotP & 0xFF);
                        rotP >>= 8;
                    }
                    for (vector<int>::const_iterator pi = players.begin(); pi != players.end(); ++pi)
                        writeMinimapPlayerPosition(lebuf, lecount, *pi);
                }
            }
            else
                for (int round = 0; round < 2; ++round) {
                    const int who = (recipient.item_shadow() ? shadowIters : normalIters)[i / TSIZE][round];
                    if (who == -1)
                        writeByte(lebuf, lecount, 255);
                    else {
                        writeByte(lebuf, lecount, static_cast<NLubyte>(who));
                        writeMinimapPlayerPosition(lebuf, lecount, who);
                    }
                }

            // send 8 bits of player's health
            nAssert(recipient.health >= 0);
            nAssert((recipient.health == 0) == recipient.dead);
            writeByte(lebuf, lecount, static_cast<NLubyte>(iround(recipient.health) & 255));

            // send 8 bits of player's energy
            nAssert(recipient.energy >= 0);
            writeByte(lebuf, lecount, static_cast<NLubyte>(iround(recipient.energy) & 255));

            // ping of player frame# % maxplayers
            if (recipient.protocolExtensionsLevel < 0 || world.player[world.frame % maxplayers].used) {
                const NLushort theping = static_cast<NLushort>(world.player[world.frame % maxplayers].ping);
                writeShort(lebuf, lecount, theping);
            }
        }

        //send the packet
        server->send_frame(recipient.cid, lebuf, lecount);

        //send server map list if not sent yet
        if (recipient.current_map_list_item < host->maplist().size() && world.frame % 2 == 0) {
            send_map_info(recipient);
            ++recipient.current_map_list_item;
        }
    }

    // map votes update
    if (world.frame % 10 == 0)
        broadcast_map_votes_update();

    // stats update
    if (gameRunning && world.frame / MAX_PLAYERS % 5 == 0) {
        const int pid = world.frame % MAX_PLAYERS;
        if (world.player[pid].used) {
            broadcast_movements_and_shots(world.player[pid]);   // player's stats to everyone
            send_team_movements_and_shots(world.player[pid].cid);   // team stats to player
        }
    }
    if (gameRunning && world.frame % 20 == 0)
        send_team_movements_and_shots(pid_record);

    ping_send_client++;
    if (ping_send_client >= maxplayers)
        ping_send_client = 0;
    if (world.player[ping_send_client].used)
        server->ping_client(world.player[ping_send_client].cid);

    g_timeCounter.refresh(); // we prefer an exact time here
    frameSentTime = get_time();
}

double ServerNetworking::getTraffic() const {
    return server->get_socket_stat(NL_AVE_BYTES_RECEIVED) + server->get_socket_stat(NL_AVE_BYTES_SENT);
}

void ServerNetworking::run_masterjob_thread(MasterQuery* job) {
    logThreadStart("run_masterjob_thread", log);

    int delay = 0;  // given a value in MS before each continue: this time will be waited before next round

    while (!mjob_exit) {
        if (delay > 0) {
            platSleep(500);
            delay -= 500;
            if (!mjob_fastretry)
                continue;
        }
        delay = 60000;  // default to one minute

        nlOpenMutex.lock();
        nlDisable(NL_BLOCKING_IO);
        NLsocket sock = nlOpen(0, NL_RELIABLE);
        nlOpenMutex.unlock();
        if (sock == NL_INVALID) {
            log("Tournament thread: Can't open socket. %s", getNlErrorString());
            delay = 10000;
            continue;
        }

        NLaddress tournamentServer;
        if (!nlGetAddrFromName("www.mycgiserver.com", &tournamentServer))
            nlStringToAddr("64.69.35.205", &tournamentServer);

        nlSetAddrPort(&tournamentServer, 80);
        nlConnect(sock, &tournamentServer);

        const NetworkResult result = writeToUnblockingTCP(sock, job->request.data(), job->request.length(), &mjob_exit, 30000);
        if (result != NR_ok) {
            nlClose(sock);
            if (mjob_exit)
                break;
            log("Tournament thread: Error sending info: %s", result == NR_timeout ? "Timeout" : getNlErrorString());
            continue;
        }

        string response;
        {
            ostringstream respStream;
            const NetworkResult result = save_http_response(sock, respStream, &mjob_exit, 30000);
            nlClose(sock);
            if (result != NR_ok) {
                if (mjob_exit)
                    break;
                log("Tournament thread: Error receiving response: %s", result == NR_timeout ? "Timeout" : getNlErrorString());
                continue;
            }
            string fullResponse = respStream.str();

            // find the start and end of the body: after the last "<html>" and before the last "</html>"
            // the original code uses full case insensivity so response.find_last_of() can't be used
            int startPos, endPos;
            for (startPos = fullResponse.length() - 7; startPos >= 6; --startPos)   // start at length - 7 because "</html>" must fit after that
                if (!platStricmp(fullResponse.substr(startPos - 6, 6).c_str(), "<html>"))
                    break;
            for (endPos = fullResponse.length() - 7; endPos >= startPos; --endPos)
                if (!platStricmp(fullResponse.substr(endPos, 7).c_str(), "</html>"))
                    break;
            if (startPos < 6 || endPos < startPos) {
                log("Tournament thread: Invalid response (no <html>...</html>): \"%s\"", formatForLogging(fullResponse).c_str());
                continue;
            }
            response = fullResponse.substr(startPos, endPos - startPos);
        }

        // parse the response
        bool unavailable = false;
        for (string::size_type i = 0; i < response.length(); ++i)
            if (!platStricmp(response.substr(i, 22).c_str(), "contact servlet runner")) {
                log("Tournament thread: Service unavailable: \"%s\"", formatForLogging(response).c_str());
                unavailable = true;
                break;
            }
        if (unavailable)
            continue;
        log("Tournament thread: Received response: \"%s\"", formatForLogging(response).c_str());
        string::size_type cPos = response.find_first_of('@');
        if (cPos == string::npos || cPos + 1 >= response.length() || response.find_first_of('@', cPos + 1) != string::npos) {
            log("Tournament thread: Invalid response (expecting one @-code)");
            continue;
        }
        ++cPos; // point to the control character after @
        if (response[cPos] == 'K' && cPos + 1 < response.length()) {    // success; ranking data follows
            ++cPos;
            double v[4];
            char termChar;
            const int num = sscanf(response.c_str() + cPos, "%lf#%lf#%lf#%*[^#]#%lf%c", &v[0], &v[1], &v[2], &v[3], &termChar); // max_world_score that bugs is ignored
            if (num != 5 || termChar != '#') {
                log("Tournament thread: Invalid response (expecting num#num#num#num#num# after @K)");
                continue;
            }
            const int pid = ctop[job->cid];
            if (pid != -1) {    // the player is still in the game
                ClientData& clid = host->getClientData(job->cid);   //#fix: thread safety
                if (job->code == MasterQuery::JT_login) {
                    log("Tournament thread: Player %s logged in successfully", world.player[pid].name.c_str());
                    char lebuf[128]; int count = 0;
                    writeByte(lebuf, count, data_registration_response);
                    writeByte(lebuf, count, 1); // registration ok
                    server->send_message(job->cid, lebuf, count);
                    clid.token_valid = true;
                }
                else if (job->code == MasterQuery::JT_score)
                    log("Tournament thread: Score for player %s updated successfully", world.player[pid].name.c_str());
                else
                    nAssert(0);
                clid.score      = static_cast<int>(v[0]);
                clid.neg_score  = static_cast<int>(v[1]);
                clid.rank       = static_cast<int>(v[2]);
                max_world_rank  = static_cast<int>(v[3]);
                broadcast_player_crap(pid);
            }
            break;  // request complete
        }
        else if (response[cPos] == 'E' || response[cPos] == 'F') {
            const int pid = ctop[job->cid];
            if (pid != -1) {
                if (job->code == MasterQuery::JT_login) {
                    log.security("Tournament thread: Login failed for player %s (at %s), request: \"%s\"",
                                world.player[pid].name.c_str(), addressToString(get_client_address(job->cid)).c_str(), formatForLogging(job->request).c_str());
                    char lebuf[128]; int count = 0;
                    writeByte(lebuf, count, data_registration_response);
                    writeByte(lebuf, count, 0); // registration failed
                    server->send_message(job->cid, lebuf, count);
                    host->getClientData(job->cid).token_have = false;
                    broadcast_player_crap(pid);
                }
                else if (job->code == MasterQuery::JT_score) {
                    send_tournament_update_failed(pid);
                    log("Tournament thread: Score update for player %s failed!", world.player[pid].name.c_str());
                }
                else
                    nAssert(0);
            }
            else if (job->code == MasterQuery::JT_score)
                log("Tournament thread: Score update lost for a player who has left the server");
            break;  // request complete
        }
        else
            log("Tournament thread: Invalid response (bad @-code)");
    }
    {
        MutexLock ml(mjob_mutex);
        --mjob_count;
    }
    delete job;

    logThreadExit("run_masterjob_thread", log);
}

void ServerNetworking::run_mastertalker_thread() {
    logThreadStart("run_mastertalker_thread", log);

    string localAddress = settings.ip();
    if (!isValidIP(localAddress) || check_private_IP(localAddress)) {
        log("Master talker: No public IP address. Letting the master server decide.");
        localAddress.clear();
    }

    bool master_never_talked = true;
    double master_talk_time = get_time() + delay_to_report_server;  //give it a break

    while (!file_threads_quit) {
        platSleep(500);

        if (get_time() < master_talk_time)
            continue;

        master_talk_time = get_time() + 3 * 60.0 ;      //3 minutes
        if (settings.privateServer()) {
            if (!master_never_talked) {
                send_master_quit(localAddress);
                master_never_talked = true;
            }
            continue;
        }

        // note: most the code from here down is repeated in the quitting phase; make changes there too (//#fixme)

        //open socket
        nlOpenMutex.lock();
        nlDisable(NL_BLOCKING_IO);
        NLsocket msock = nlOpen(0, NL_RELIABLE);
        nlOpenMutex.unlock();
        if (msock == NL_INVALID) {
            log.error(_("Master talker: Can't open socket to connect to master server."));
            continue;
        }

        if (nlConnect(msock, &g_masterSettings.address()) == NL_FALSE) {
            log("Master talker: Can't connect to master server.");
            nlClose(msock);
            continue;
        }

        //now we have talked
        master_never_talked = false;

        // build and send data
        map<string, string> parameters = master_parameters(localAddress);
        const string data = build_http_data(parameters);
        NetworkResult result = post_http_data(msock, &file_threads_quit, 30000, g_masterSettings.host(), g_masterSettings.submit(), data);
        if (result != NR_ok)
            log("Master talker: Error sending info: %s", result == NR_timeout ? "Timeout" : getNlErrorString());
        else {
            stringstream response;
            result = save_http_response(msock, response, &file_threads_quit, 30000);
            if (result == NR_ok) {
                // save transaction to a file
                ofstream out((wheregamedir + "log" + directory_separator + "master.log").c_str());
                out << "This file contains the server's latest successfully completed communications\nwith the server list master server\n\n";
                out << "--- Query ---\n";
                out << data << "\n";
                out << "\n--- Response ---\n";
                out << response.str();
                out.close();
                if (response.str().find("VERSION ERROR") != string::npos) {
                    log.error(_("Master talker: You have a deprecated Outgun version. The server is not accepted on the master list. Please update."));
                    nlClose(msock);
                    return;
                }
                if (response.str().find("[ERROR]") != string::npos) // this means a more permanent problem
                    log.error(_("Master talker: There was an unexpected error while sending information to the master list. See log/master.log. To suppress this error, make the server private by using the -priv argument."));
                else if (response.str().find("[OK]") == string::npos) // this happens when the master server has problems
                    log("Master talker: There was an unexpected error while sending information to the master list. See log/master.log.");
            }
            else {
                log("Master talker: Error while waiting for a response: %s", result == NR_timeout ? "Timeout" : getNlErrorString());
                master_talk_time = get_time() + 30.0;   // faster retry: in 30 seconds
            }
        }

        //close socket
        nlClose(msock);
    }

    log("Master talker: time to say goodbye.");

    if (master_never_talked)
        return;

    send_master_quit(localAddress);
}

// Quitting: Delete my IP from the master so clients won't see it.
void ServerNetworking::send_master_quit(const string& localAddress) const {
    //open socket
    nlOpenMutex.lock();
    nlDisable(NL_BLOCKING_IO);
    NLsocket msock = nlOpen(0, NL_RELIABLE);
    nlOpenMutex.unlock();

    if (msock == NL_INVALID) {
        log.error(_("Master talker: (Quit) Can't open socket to connect to master server."));
        return;
    }

    //connect
    if (nlConnect(msock, &g_masterSettings.address()) == NL_FALSE) {
        log.error(_("Master talker: (Quit) Can't connect to master server."));
        nlClose(msock);
        return;
    }

    // send quit message
    map<string, string> parameters = master_parameters(localAddress, true); // true = quitting
    const string data = build_http_data(parameters);
    NetworkResult result = post_http_data(msock, 0, 5000, g_masterSettings.host(), g_masterSettings.submit(), data); // only 5 seconds allowed; it's not so crucial
    if (result != NR_ok)
        log.error(_("Master talker: (Quit) Error sending info: $1", result == NR_timeout ? "Timeout" : getNlErrorString()));
    else {
        stringstream response;
        result = save_http_response(msock, response, 0, 5000);  // only 5 seconds allowed; it's not so crucial
        if (result == NR_ok) {
            // save transaction to a file
            ofstream out((wheregamedir + "log" + directory_separator + "master.log").c_str());
            out << "This file contains the server's latest successfully completed communications\nwith the server list master server\n\n";
            out << "--- Query ---\n";
            out << data << "\n";
            out << "\n--- Response ---\n";
            out << response.str();
            out.close();
            if (response.str().find("[OK]") == string::npos)
                log.error(_("Master talker: (Quit) There was an unexpected error while sending information to the master list. See log/master.log."));
        }
        else
            log.error(_("Master talker: (Quit) Error while waiting for a response: $1", result == NR_timeout ? "Timeout" : getNlErrorString()));
    }

    nlClose(msock);
}

void ServerNetworking::run_website_thread() {
    logThreadStart("run_website_thread", log);

    if (settings.get_web_servers().empty() || settings.get_web_script().empty())
        return;

    const string& localAddress = settings.ip();
    // use it even if not public

    NLaddress website_address;
    string working_address_string;
    double website_talk_time = 0.0;
    bool first_connection = true;
    int sent_maplist_revision = -1;

    while (!file_threads_quit) {
        platSleep(500);

        if (get_time() < website_talk_time)
            continue;

        website_talk_time = get_time() + settings.get_web_refresh() * 60.0;

        // note: most of the code from here down is repeated in the quitting phase; make changes there too (//#fixme)

        nlOpenMutex.lock();
        nlDisable(NL_BLOCKING_IO);
        NLsocket websock = nlOpen(0, NL_RELIABLE);
        nlOpenMutex.unlock();
        if (websock == NL_INVALID) {
            log.error(_("Website thread: Can't open socket to connect to server website."));
            continue;
        }
        bool success = false;
        for (vector<string>::const_iterator addri = settings.get_web_servers().begin(); addri != settings.get_web_servers().end(); ++addri)
            if (nlGetAddrFromName(addri->c_str(), &website_address)) {
                success = true;
                working_address_string = *addri;
                break;
            }
            else
                log("Website thread: Can't get address from %s. Reason: %s", addri->c_str(), getNlErrorString());
        if (!success) {
            log("Website thread: Can't get any address from the list!");
            continue;
        }
        int web_port = nlGetPortFromAddr(&website_address);
        if (!web_port) {
            web_port = 80;
            nAssert(nlSetAddrPort(&website_address, web_port));
        }
        if (!website_address.valid || nlConnect(websock, &website_address) == NL_FALSE) {       // connect
            log("Website thread: Server can't connect to server website! Reason: %s", getNlErrorString());
            nlClose(websock);
            continue;
        }

        // build and send data
        map<string, string> parameters = website_parameters(localAddress);
        const int sending_maplist_revision = maplist_revision;
        if (first_connection || sending_maplist_revision != sent_maplist_revision) {
            parameters["maplist"] = website_maplist();
            first_connection = false;
        }
        const string data = build_http_data(parameters);
        NetworkResult result = post_http_data(websock, &file_threads_quit, 30000, working_address_string, settings.get_web_script(), data, settings.get_web_auth());
        if (result == NR_ok) {
            // save response to a file
            ofstream out((wheregamedir + "log" + directory_separator + "web.log").c_str());
            result = save_http_response(websock, out, &file_threads_quit, 30000);
        }
        if (result != NR_ok)
            website_talk_time = get_time() + 30.0;  // faster retry: in 30 seconds
        else
            sent_maplist_revision = sending_maplist_revision;

        //close socket
        nlClose(websock);
    }

    log("Website thread: time to say goodbye");

    if (first_connection)   // not send anything
        return;

    //quitting: send server shutdown message to web script
    //open socket
    nlOpenMutex.lock();
    nlDisable(NL_BLOCKING_IO);
    NLsocket websock = nlOpen(0, NL_RELIABLE);
    nlOpenMutex.unlock();

    if (websock == NL_INVALID) {
        log.error(_("Website thread: (Quit) Can't open socket to connect to server website."));
        return;
    }

    //connect
    if (nlConnect(websock, &website_address) == NL_FALSE) {
        log.error(_("Website thread: (Quit) Can't connect to server website."));
        nlClose(websock);
        return;
    }

    // send quit message
    const string quit = "quit=1";
    const NetworkResult result = post_http_data(websock, 0, 5000, working_address_string, settings.get_web_script(), quit, settings.get_web_auth());  // only 5 seconds allowed; it's not so crucial
    log("Website thread: Sent information to server website: \"%s\", result %d", formatForLogging(quit).c_str(), result);

    if (result == NR_ok) {
        // save response to a file
        ofstream out((wheregamedir + "log" + directory_separator + "web.log").c_str());
        save_http_response(websock, out, 0, 5000);  // only 5 seconds allowed; it's not so crucial
    }

    nlClose(websock);
}

map<string, string> ServerNetworking::master_parameters(const string& address, bool quitting) const {
    map<string, string> parameters;
    if (!address.empty())
        parameters["ip"] = address;
    parameters["port"] = itoa(settings.get_port());
    if (quitting)
        parameters["quit"] = "1";
    else {
        parameters["name"] = settings.get_hostname();
        parameters["players"] = itoa(get_human_count());
        parameters["bots"] = itoa(bot_count);
        if (settings.dedicated())
            parameters["dedicated"] = "1";
        parameters["max_players"] = itoa(maxplayers);
        parameters["version"] = GAME_VERSION;
        parameters["protocol"] = GAME_PROTOCOL;
        parameters["uptime"] = itoa(world.frame / 10);
        parameters["map"] = host->current_map().title;
        parameters["link"] = host->server_website();
        if (!settings.get_server_password().empty())
            parameters["password"] = "1";
        if (is_relay_active())
            parameters["spectator"] = "1";
    }
    parameters["id"] = server_identification;
    return parameters;
}

map<string, string> ServerNetworking::website_parameters(const string& address) const {
    map<string, string> parameters;
    parameters["name"] = settings.get_hostname();
    parameters["ip"] = address;
    parameters["port"] = itoa(settings.get_port());
    parameters["players"] = itoa(get_human_count());
    parameters["bots"] = itoa(bot_count);
    if (settings.dedicated())
        parameters["dedicated"] = "1";
    parameters["max_players"] = itoa(maxplayers);
    parameters["version"] = GAME_VERSION;
    parameters["uptime"] = itoa(world.frame / 10);
    parameters["map"] = host->current_map().title;
    parameters["mapfile"] = host->getCurrentMapFile();
    if (!settings.get_server_password().empty())
        parameters["password"] = "1";
    if (is_relay_active())
        parameters["spectator"] = "1";
    string players;
    for (int i = 0; i < maxplayers; i++)
        if (world.player[i].used) {
            if (!players.empty())
                players += '\n';
            players += world.player[i].name + '\t' + itoa(i / TSIZE) + '\t' + itoa(world.player[i].ping);
        }
    parameters["playerlist"] = players;
    return parameters;
}

string ServerNetworking::website_maplist() const {
    ostringstream maps;
    for (vector<MapInfo>::const_iterator m = host->maplist().begin(); m != host->maplist().end(); m++) {
        if (m != host->maplist().begin())
            maps << '\n';
        maps << m->title;
    }
    return maps.str();
}

// read a string from a TCP stream, one char at a time; it doesn't tolerate breaks and is very slow but the admin shell system doesn't need more reliability
bool ServerNetworking::read_string_from_TCP(NLsocket sock, char *buf) {
    for (;;) {
        NLint result = nlRead(sock, buf, 1);
        if (result != 1)    // message not completely received
            return false;
        if (*buf == '\0')
            return true;
        ++buf;
    }
}

//run a admin shell master thread
void ServerNetworking::run_shellmaster_thread(int port) {
    logThreadStart("run_shellmaster_thread", log);

    Thread slaveThread;
    volatile bool slaveRunning = false; // the slave thread will modify this flag when quitting

    log("Admin shell master thread running");

    nlOpenMutex.lock();
    nlDisable(NL_BLOCKING_IO);
    NLsocket shellmsock = nlOpen(port, NL_RELIABLE);
    nlOpenMutex.unlock();
    if (shellmsock == NL_INVALID) {
        log.error(_("Admin shell: Can't open socket on port $1.", itoa(port)));
        return;
    }
    if (!nlListen(shellmsock)) {
        log.error(_("Admin shell: Can't set socket to listen mode."));
        return;
    }

    while (!file_threads_quit) {
        platSleep(1000); // this thread definitely is no priority

        //accept one connection
        nlOpenMutex.lock();
        nlDisable(NL_BLOCKING_IO);
        NLsocket newSock = nlAcceptConnection(shellmsock);
        nlOpenMutex.unlock();

        if (newSock == NL_INVALID) {
            if (nlGetError() == NL_NO_PENDING)
                continue;
            log.error(_("Admin shell: Can't accept connection."));
            return;
        }

        log("Incoming admin shell connection");

        //accept connections only from localhost
        NLaddress addr, c1, c2;
        nlGetRemoteAddr(newSock, &addr);
        nlStringToAddr("127.0.0.1", &c1);
        get_self_IP(&c2);
        nlSetAddrPort(&addr, 0);
        nlSetAddrPort(&c1, 0);
        nlSetAddrPort(&c2, 0);

        if (nlAddrCompare(&addr, &c1) == NL_FALSE && nlAddrCompare(&addr, &c2) == NL_FALSE) {
            log("Attempt to connect a remote admin shell blocked.");
            nlClose(newSock);
            continue;
        }

        if (slaveRunning) { // if already connected, skip
            log("Attempt to connect two simultaneous admin shells blocked.");
            nlClose(newSock);
            continue;
        }

        log("Admin shell connection accepted");

        // tell about the current situation
        char lebuf[4096];
        int count = 0;
        for (int i = 0; i < maxplayers; i++)
            if (world.player[i].used) {
                writeLong(lebuf, count, STA_PLAYER_CONNECTED);
                writeLong(lebuf, count, world.player[i].cid);

                writeLong(lebuf, count, STA_PLAYER_NAME_UPDATE);
                writeLong(lebuf, count, world.player[i].cid);
                writeStr(lebuf, count, world.player[i].name);

                writeLong(lebuf, count, STA_PLAYER_IP);
                writeLong(lebuf, count, world.player[i].cid);
                NLaddress addr = get_client_address(world.player[i].cid);
                nlSetAddrPort(&addr, 0);
                writeStr(lebuf, count, addressToString(addr));

                writeLong(lebuf, count, STA_PLAYER_FRAGS);
                writeLong(lebuf, count, world.player[i].cid);
                writeLong(lebuf, count, world.player[i].stats().frags());
            }
        nlWrite(newSock, lebuf, count);

        if (slaveThread.isRunning())
            slaveThread.join();
        slaveRunning = true;    // slave will set it false when exiting
        shellssock = newSock;
        slaveThread.start(RedirectToMemFun1<ServerNetworking, void, volatile bool*>(this, &ServerNetworking::run_shellslave_thread), &slaveRunning, settings.lowerPriority());
    }
    nlClose(shellmsock);
    log("Admin shell master thread quitting");
    if (slaveThread.isRunning())
        slaveThread.join();

    logThreadExit("run_shellmaster_thread", log);
}

void ServerNetworking::run_shellslave_thread(volatile bool* runningFlag) {  // sets *runningFlag = true when quitting
    logThreadStart("run_shellslave_thread", log);

    while (!file_threads_quit) {
        char rbuf[256];
        int rcount = 0;

        //read request code
        NLint result = nlRead(shellssock, rbuf, 4);

        if (result == NL_INVALID) {
            log.error(_("Admin shell: read failed. Reason: $1", getNlErrorString()));
            break;
        }

        if (result == 0) {
            platSleep(500);  // no need to be more responsive
            continue;
        }

        if (result != 4) {
            log.error("Admin shell: bad data length");
            break;
        }

        NLulong code;
        readLong(rbuf, rcount, code);

        // parse the code
        if (code >= NUMBER_OF_ATS) {
            log.error("Admin shell: invalid command " + itoa(code));
            break;
        }

        if (code == ATS_QUIT) {
            log("Admin shell: received quit command");
            break;
        }

        NLulong cid = 0;
        int pid = 0;    // pid and cid set if argPid[code]
        NLulong dwArg = 0;  // set if argDw[code]
        //                                      noop, get-functions,ch,qu,pi,kckbanmte,reset
        static const int argPid[NUMBER_OF_ATS] = { 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0 };
        static const int argDw [NUMBER_OF_ATS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 };
        const int argsLen = (argPid[code] + argDw[code]) * 4;

        if (argsLen) {
            result = nlRead(shellssock, rbuf, argsLen);
            if (result != argsLen) {
                if (result == NL_INVALID)
                    log.error(_("Admin shell: read failed. Reason: $1", getNlErrorString()));
                else
                    log.error("Admin shell: bad data length (args: " + itoa(result) + '/' + itoa(argsLen) + ')');
                break;
            }
            rcount = 0;
            if (argPid[code]) {
                readLong(rbuf, rcount, cid);
                if (cid > 255) {
                    log.error("Admin shell: bad client id");
                    break;
                }
                pid = ctop[cid];
                if (pid == -1 || !world.player[pid].used)   // player not in the game; just ignore the command
                    continue;
            }
            if (argDw[code])
                readLong(rbuf, rcount, dwArg);
        }

        if (threadLock)
            threadLockMutex.lock();

        char answer[1000];
        int ansLen = 0;
        bool error = false;
        switch (code) {
        /*break;*/ case ATS_GET_PLAYER_FRAGS:
                writeLong(answer, ansLen, STA_PLAYER_FRAGS);
                writeLong(answer, ansLen, cid);
                writeLong(answer, ansLen, world.player[pid].stats().frags());
            break; case ATS_GET_PLAYER_TOTAL_TIME:
                writeLong(answer, ansLen, STA_PLAYER_TOTAL_TIME);
                writeLong(answer, ansLen, cid);
                writeLong(answer, ansLen, static_cast<int>(get_time() - world.player[pid].stats().start_time()));
            break; case ATS_GET_PLAYER_TOTAL_KILLS:
                writeLong(answer, ansLen, STA_PLAYER_TOTAL_KILLS);
                writeLong(answer, ansLen, cid);
                writeLong(answer, ansLen, world.player[pid].stats().kills());
            break; case ATS_GET_PLAYER_TOTAL_DEATHS:
                writeLong(answer, ansLen, STA_PLAYER_TOTAL_DEATHS);
                writeLong(answer, ansLen, cid);
                writeLong(answer, ansLen, world.player[pid].stats().deaths());
            break; case ATS_GET_PLAYER_TOTAL_CAPTURES:
                writeLong(answer, ansLen, STA_PLAYER_TOTAL_CAPTURES);
                writeLong(answer, ansLen, cid);
                writeLong(answer, ansLen, world.player[pid].stats().captures());
            break; case ATS_SERVER_CHAT: {
                char buf[500];
                if (!read_string_from_TCP(shellssock, buf)) {
                    log.error(_("Admin shell: read failed. Reason: $1", getNlErrorString()));
                    error = true;
                }
                else {
                    if (find_nonprintable_char(buf))
                        log.error(_("Admin shell: unprintable characters, message ignored."));
                    else if (buf[0] == '/')
                        host->chat(shell_pid, buf);
                    else
                        bprintf(msg_normal, "ADMIN: %s", buf);
                }
            }
            break; case ATS_GET_PINGS:
                for (int p = 0; p < maxplayers; ++p)
                    if (world.player[p].used) {
                        writeLong(answer, ansLen, STA_PLAYER_PING);
                        writeLong(answer, ansLen, world.player[p].cid);
                        writeLong(answer, ansLen, world.player[p].ping);
                    }
            break; case ATS_MUTE_PLAYER:
                host->mutePlayer(pid, dwArg, shell_pid);
            break; case ATS_KICK_PLAYER:
                host->kickPlayer(pid, shell_pid);
            break; case ATS_BAN_PLAYER:
                host->banPlayer(pid, shell_pid, 60 * 24 * 365);    // ban for a year; this can be later adjusted in auth.txt
            break; case ATS_RESET_SETTINGS:
                host->reset_settings(true);
            break; default:
                nAssert(0);
        }

        if (threadLock)
            threadLockMutex.unlock();

        if (error)
            break;

        if (ansLen) {
            result = nlWrite(shellssock, answer, ansLen);
            if (result != ansLen) {
                log.error(_("Admin shell: sending response failed. Reason: $1", getNlErrorString()));
                break;
            }
        }
    }

    nlClose(shellssock);
    shellssock = NL_INVALID;    // not in use
    *runningFlag = false;
    log("Admin shell slave thread quitting");

    logThreadExit("run_shellslave_thread", log);
}

void ServerNetworking::stop() {
    //submit all pending player reports
    for (int i = 0; i < maxplayers; i++)
        if (world.player[i].used)
            client_report_status(world.player[i].cid);

    //v0.4.4 : flag master job threads to start trying to resolve themselves quickly
    mjob_fastretry = true;
    const double mjmaxtime = get_time() + 30.0;     //timeout : 30 seconds

    settings.statusOutput()(_("Shutdown: net server"));

    if (server)
        server->stop(3);
    else
        nAssert(0);

    //reset client_c struct (closes files...)
    for (int i = 0; i < MAX_PLAYERS; i++)
        fileTransfer[i].reset();

    file_threads_quit = true;   // flag so threads will quit themselves

    //close TCP connection with the server admin shell
    settings.statusOutput()(_("Shutdown: admin shell threads"));
    shellmthread.join();

    //wait for all master jobs to complete nicely
    while (mjob_count > 0 && get_time() < mjmaxtime) {
        settings.statusOutput()(_("Shutdown: waiting for $1 tournament updates", itoa(mjob_count)));
        platSleep(100);
    }

    //clean up jobs
    mjob_exit = true;       //MUST terminate -- abort
    while (mjob_count > 0) {
        settings.statusOutput()(_("Shutdown: ABORTING $1 tournament updates", itoa(mjob_count)));
        platSleep(100);
    }

    settings.statusOutput()(_("Shutdown: master talker thread"));
    mthread.join();

    settings.statusOutput()(_("Shutdown: website thread"));
    webthread.join();

    settings.statusOutput()(_("Shutdown: main thread"));
}

void ServerNetworking::sendWeaponPower(int pid) const {
    char lebuf[256]; int count = 0;
    writeByte(lebuf, count, data_weapon_change);
    writeByte(lebuf, count, static_cast<NLubyte>(world.player[pid].weapon));
    server->send_message(world.player[pid].cid, lebuf, count);
}

void ServerNetworking::sendRocketMessage(int shots, GunDirection gundir, NLubyte* sid, int pid, bool power,
                                         int px, int py, int x, int y, NLulong vislist) const { // sid = shot-id; array of NLubyte[shots]
    for (int iProto = 0; iProto < 2; ++iProto) {
        const bool preciseGundir = iProto == 1 && world.physics.allowFreeTurning;
        char lebuf[256]; int count = 0;
        writeByte(lebuf, count, data_rocket_fire);
        writeByte(lebuf, count, shots);
        if (preciseGundir) {
            const NLushort dirData = gundir.toNetworkLongForm();
            writeByte(lebuf, count, (dirData >> 8) | 0x80); // high bit signals extended form (it's never set in 1-byte directions)
            writeByte(lebuf, count, dirData & 0xFF);
        }
        else
            writeByte(lebuf, count, gundir.toNetworkShortForm());
        for (int i = 0; i < shots; i++)
            writeByte(lebuf, count, sid[i]);    // rocket-object id (needed because client-side rockets can be deleted by the server)
        writeLong(lebuf, count, world.frame);   // time of shot of the rocket: current (last simulated) frame
        NLubyte shotType = power;
        if (iProto == 0)
            shotType |= (pid / TSIZE) << 1;
        else
            shotType |= 2 | (pid << 2); // we're always sending the pid, but this might change in the future
        writeByte(lebuf, count, static_cast<NLubyte>(shotType));    // owner of all rockets
        writeByte(lebuf, count, static_cast<NLubyte>(px));  //coord
        writeByte(lebuf, count, static_cast<NLubyte>(py));
        writeShort(lebuf, count, static_cast<NLshort>(x));
        writeShort(lebuf, count, static_cast<NLshort>(y));

        for (int i = 0; i < maxplayers; i++)
            if ((vislist & (1 << i)) && ((iProto == 0) == (world.player[i].protocolExtensionsLevel == -1)))
                server->send_message(world.player[i].cid, lebuf, count);

        if (iProto == 1)
            record_message(lebuf, count);
    }
}

void ServerNetworking::sendOldRocketVisible(int pid, int rid, const Rocket& rocket) const {
    const bool preciseGundir = world.player[pid].protocolExtensionsLevel >= 0 && world.physics.allowFreeTurning;
    char lebuf[256]; int count = 0;
    const NLubyte shotType = (rocket.team << 1) | rocket.power;
    writeByte(lebuf, count, data_old_rocket_visible);
    writeByte(lebuf, count, static_cast<NLubyte>(rid));
    if (preciseGundir) {
        const NLushort dirData = rocket.direction.toNetworkLongForm();
        writeByte(lebuf, count, (dirData >> 8) | 0x80); // high bit signals extended form (it's never set in 1-byte directions)
        writeByte(lebuf, count, dirData & 0xFF);
    }
    else
        writeByte(lebuf, count, rocket.direction.toNetworkShortForm());
    writeLong(lebuf, count, world.frame);
    writeByte(lebuf, count, shotType);
    writeByte(lebuf, count, rocket.px);
    writeByte(lebuf, count, rocket.py);
    writeShort(lebuf, count, static_cast<NLshort>(rocket.x));
    writeShort(lebuf, count, static_cast<NLshort>(rocket.y));

    server->send_message(world.player[pid].cid, lebuf, count);
}

void ServerNetworking::sendRocketDeletion(NLulong plymask, int rid, NLshort hitx, NLshort hity, int targ) const {
    //assembly rocket delete message
    char lebuf[256]; int count = 0;
    writeByte(lebuf, count, data_rocket_delete);
    writeByte(lebuf, count, static_cast<NLubyte>(rid));     // rocket-object id
    writeByte(lebuf, count, static_cast<NLubyte>(targ));        // player-target. if 255, no player in particular was hit

    writeShort(lebuf, count, hitx);     // HIT X,Y OF ROCKET
    writeShort(lebuf, count, hity);

    //send message to players that received the rocket
    for (int i = 0; i < maxplayers; i++)
        if (world.player[i].used && (plymask & (1 << i)))
            server->send_message(world.player[i].cid, lebuf, count);

    record_message(lebuf, count);
}

void ServerNetworking::sendDeathbringer(int pid, const ServerPlayer& ply) const {
    char lebuf[256]; int count = 0;
    writeByte(lebuf, count, data_deathbringer);
    writeByte(lebuf, count, static_cast<NLubyte>(pid / TSIZE)); // team
    writeLong(lebuf, count, world.frame);                       // frame # of the bringer shot (message can be delayed)
    writeByte(lebuf, count, static_cast<NLubyte>(ply.roomx));
    writeByte(lebuf, count, static_cast<NLubyte>(ply.roomy));
    writeShort(lebuf, count, static_cast<NLushort>(ply.lx));
    writeShort(lebuf, count, static_cast<NLushort>(ply.ly));

    broadcast_message(lebuf, count);
    record_message(lebuf, count);
}

void ServerNetworking::sendPickupVisible(int pid, int pup_id, const Powerup& it) const {
    char lebuf[256]; int count = 0;
    writeByte(lebuf, count, data_pup_visible);
    writeByte(lebuf, count, static_cast<NLubyte>(pup_id));  //what item
    writeByte(lebuf, count, static_cast<NLubyte>(it.kind)); //kind
    writeByte(lebuf, count, static_cast<NLubyte>(it.px));       //screen
    writeByte(lebuf, count, static_cast<NLubyte>(it.py));
    writeShort(lebuf, count, static_cast<NLushort>(it.x));  //pos in screen
    writeShort(lebuf, count, static_cast<NLushort>(it.y));
    if (pid == pid_record)
        record_message(lebuf, count);
    else
        server->send_message(world.player[pid].cid, lebuf, count);
}

void ServerNetworking::broadcastPickupPicked(int roomx, int roomy, int pup_id) const {
    char lebuf[256]; int count = 0;
    writeByte(lebuf, count, data_pup_picked);
    writeByte(lebuf, count, static_cast<NLubyte>(pup_id));
    broadcast_screen_message(roomx, roomy, lebuf, count);
}

void ServerNetworking::sendPupTime(int pid, NLubyte pupType, double timeLeft) const {
    char lebuf[256]; int count = 0;
    writeByte(lebuf, count, data_pup_timer);
    writeByte(lebuf, count, pupType);
    writeShort(lebuf, count, static_cast<NLushort>(timeLeft));
    server->send_message(world.player[pid].cid, lebuf, count);
}

void ServerNetworking::sendFragUpdate(int pid, NLulong frags) const {
    char lebuf[256]; int count = 0;
    writeByte(lebuf, count, data_frags_update);
    writeByte(lebuf, count, pid);       // what player id
    writeLong(lebuf, count, frags);
    broadcast_message(lebuf, count);
    record_message(lebuf, count);
}

void ServerNetworking::sendNameAuthorizationRequest(int pid) const {
    char lebuf[256]; int count = 0;
    writeByte(lebuf, count, data_name_authorization_request);
    server->send_message(world.player[pid].cid, lebuf, count);
}

NLaddress ServerNetworking::get_client_address(int cid) const {
    return server->get_client_address(cid);
}

void ServerNetworking::clientHello(int client_id, char* data, int length, ServerHelloResult* res) {
    res->customDataLength = 0;

    //check versions
    string stri;
    ostringstream temp;
    int count = 0;

    // free reservedPlayerSlots that have been left unused, they might be needed now
    if (playerSlotReservationTime < get_time() - 60.) // in 60 seconds Leetnet should drop the client
        reservedPlayerSlots = 0;

    if (length > 0)
        readStr(data, count, stri); //read gamestring

    if (stri != GAME_STRING) {
        log("Rejected a client because game strings don't match: Server '%s' and player '%s'.", GAME_STRING, stri.c_str());
        res->accepted = false;      // not accepted

        temp << "This game is " << GAME_STRING;
        writeStr(res->customData, res->customDataLength, temp.str());
    }
    else {
        readStr(data, count, stri); //read protocol string
        const time_t tt = time(0);
        const tm* tmb = gmtime(&tt);
        const int seconds = tmb->tm_hour * 3600 + tmb->tm_min * 60 + tmb->tm_sec;
        const int join_start = settings.get_join_start(), join_end = settings.get_join_end();
        if (stri != GAME_PROTOCOL) {
            log("Rejected a client because protocol strings don't match: Server '%s' and player '%s'.", GAME_PROTOCOL, stri.c_str());
            res->accepted = false;

            if (stri.length() > 50)
                stri = "<unknown>";
            temp << "Protocol mismatch: server: " << GAME_PROTOCOL << ", client: " << stri; // this message shouldn't be altered: client detects this exact form and allows translation (it's been the same at least since 0.5.0)
            writeStr(res->customData, res->customDataLength, temp.str());
        }
        else if (get_human_count() == 0 && (join_start < join_end && (seconds < join_start || seconds > join_end) ||
                 join_start > join_end && (seconds < join_start && seconds > join_end))) {
            log("Rejected a client because the server is not open at this time.");
            res->accepted = false;

            temp << "This server is open from ";
            temp << join_start / 3600 << ':' << setfill('0') << setw(2) << join_start / 60 % 60 << " to ";
            temp << join_end   / 3600 << ':' << setfill('0') << setw(2) << join_end   / 60 % 60 << " GMT. ";
            const int wait = (join_start - seconds + 24 * 3600 + 60) % (24 * 3600);
            temp << "Come again in ";
            if (wait >= 3600)
                temp << wait / 3600 << ':' << setfill('0') << setw(2) << wait / 60 % 60 << " hours.";
            else if (wait >= 120)
                temp << wait / 60 << " minutes.";
            else
                temp << "a minute.";
            if (!settings.get_join_limit_message().empty())
                temp << ' ' << settings.get_join_limit_message();
            writeStr(res->customData, res->customDataLength, temp.str());
        }
        else if (player_count + reservedPlayerSlots - bot_count >= maxplayers) {
            log("Rejected a client because the server is full.");
            res->accepted = false;
            writeByte(res->customData, res->customDataLength, reject_server_full);
        }
        else if (host->isBanned(client_id)) {
            log("Rejected a client because their IP is banned (%s).", addressToString(get_client_address(client_id)).c_str());
            res->accepted = false;
            writeByte(res->customData, res->customDataLength, reject_banned);
        }
        else {
            string name;
            readStr(data, count, name);
            string password;
            readStr(data, count, password);
            if (!check_name(name)) {
                res->accepted = false;
                // no need to explain, the client must not allow this
            }
            else if (password == settings.get_server_password()) {
                string player_password;
                readStr(data, count, player_password);
                if (host->check_name_password(name, player_password)) {
                    if (player_count + reservedPlayerSlots >= maxplayers)
                        host->remove_bot();
                    ++reservedPlayerSlots;
                    playerSlotReservationTime = get_time();
                    res->accepted = true;
                    writeByte(res->customData, res->customDataLength, static_cast<NLubyte>(maxplayers));
                    writeStr(res->customData, res->customDataLength, settings.get_hostname());
                    writeByte(res->customData, res->customDataLength, PROTOCOL_EXTENSIONS_VERSION);
                }
                else {
                    res->accepted = false;
                    if (player_password.empty())
                        writeByte(res->customData, res->customDataLength, reject_player_password_needed);
                    else {
                        log.security("Wrong player password. Name \"%s\", password \"%s\" tried from %s.",
                                name.c_str(), player_password.c_str(), addressToString(get_client_address(client_id)).c_str());
                        writeByte(res->customData, res->customDataLength, reject_wrong_player_password);
                    }
                }
            }
            else {
                res->accepted = false;
                if (password.empty())
                    writeByte(res->customData, res->customDataLength, reject_server_password_needed);
                else {
                    log.security("Wrong server password. Password \"%s\" tried from %s, using name \"%s\".",
                            password.c_str(), addressToString(get_client_address(client_id)).c_str(), name.c_str());
                    writeByte(res->customData, res->customDataLength, reject_wrong_server_password);
                }
            }
        }
    }
}

void ServerNetworking::sfunc_client_hello(void* customp, int client_id, char* data, int length, ServerHelloResult* res) {
    ServerNetworking* sn = static_cast<ServerNetworking*>(customp);
    if (sn->threadLock)
        sn->threadLockMutex.lock();
    sn->clientHello(client_id, data, length, res);
    if (sn->threadLock)
        sn->threadLockMutex.unlock();
}

void ServerNetworking::sfunc_client_connected(void* customp, int client_id) {
    ServerNetworking* sn = static_cast<ServerNetworking*>(customp);
    if (sn->threadLock)
        sn->threadLockMutex.lock();
    sn->client_connected(client_id);
    if (sn->threadLock)
        sn->threadLockMutex.unlock();
}

void ServerNetworking::sfunc_client_disconnected(void* customp, int client_id, bool reentrant) {
    ServerNetworking* sn = static_cast<ServerNetworking*>(customp);
    if (sn->threadLock && !reentrant)
        sn->threadLockMutex.lock();
    sn->client_disconnected(client_id);
    if (sn->threadLock && !reentrant)
        sn->threadLockMutex.unlock();
}

void ServerNetworking::sfunc_client_lag_status(void* customp, int client_id, int status) {
    (void)(customp && client_id && status);
}

void ServerNetworking::sfunc_client_data(void* customp, int client_id, char* data, int length) {
    ServerNetworking* sn = static_cast<ServerNetworking*>(customp);
    if (sn->threadLock)
        sn->threadLockMutex.lock();
    sn->incoming_client_data(client_id, data, length);
    if (sn->threadLock)
        sn->threadLockMutex.unlock();
}

void ServerNetworking::sfunc_client_ping_result(void* customp, int client_id, int pingtime) {
    if (pingtime < 0)
        pingtime = 0;
    ServerNetworking* sn = static_cast<ServerNetworking*>(customp);
    if (sn->threadLock)
        sn->threadLockMutex.lock();
    sn->ping_result(client_id, pingtime);
    if (sn->threadLock)
        sn->threadLockMutex.unlock();
}
