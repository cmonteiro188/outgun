#include "commont.h"
#include "world.h"
#include "leetnet/sleep.h"	// sleep util
#include "thread.h"
#include "nassert.h"

// implements:
#include "server.h"
#include "gameserver_interface.h"

using std::find;
using std::ifstream;
using std::max;
using std::ostringstream;
using std::random_shuffle;
using std::setfill;
using std::setprecision;
using std::setw;
using std::string;
using std::swap;
using std::vector;

gameserver_c::gameserver_c(LogSet& hostLogs) :
	normalLog(wheregamedir + "log" + directory_separator + "serverlog.txt", true),
	errorLog(normalLog, "ERROR: "),
	securityLog(normalLog, "SECURITY WARNING: ", wheregamedir + "log" + directory_separator + "server_securitylog.txt", false),
	log(&normalLog, &errorLog, &securityLog),
	world(this, &network, log),
	network(this, world, log),
	authorizations(log)
{
}

gameserver_c::~gameserver_c() {
}

void gameserver_c::mutePlayer(int pid, int mode, int admin) {	// 0 = unmute, 1 = normal, 2 = mute silently (do not inform the player)
}

void gameserver_c::kickPlayer(int pid, int admin, bool ban) {
}

void gameserver_c::banPlayer(int pid, int admin, int minutes) {
}

bool gameserver_c::check_name_password(const string& name, const string& password) const {
	return false;
}

void gameserver_c::ctf_game_restart() {
}

//check if team change requests can be satisfied
void gameserver_c::check_team_changes() {
}

//check if a player wants to change teams and if yes, try to fullfill the wish
void gameserver_c::check_player_change_teams(int pid) {
}

//move player - move player (f rom) to empty position (t o)
void gameserver_c::move_player(int f, int t) {
}

//swap players - both are valid players
void gameserver_c::swap_players(int a, int b) {
}

void gameserver_c::set_fav_colors(int pid, const vector<char>& colors) {
}

void gameserver_c::check_fav_colors(int pid) {
}

void gameserver_c::sendMessage(int pid, Message_type type, const std::string& msg) {
}

//refresh team ratings
void gameserver_c::refresh_team_score_modifiers() {
}

//score!
void gameserver_c::score_frag(int p, int amount) {
}

//score! NEG FRAG (v0.4.8)
void gameserver_c::score_neg(int p, int amount) {
}

void gameserver_c::load_game_mod() {
}

//load a map from the rotation list
bool gameserver_c::load_rotation_map(int pos) {
	return false;
}

bool gameserver_c::server_next_map(int reason) {
	return false;
}

//check map exit by vote
void gameserver_c::check_map_exit() {
}

//----- THE REST  ----------------

bool gameserver_c::reset_settings(bool keepMap) {
	return false;
}

//start server
bool gameserver_c::start(int target_maxplayers) {
	return false;
}

int gameserver_c::getLessScoredTeam() const {
	return 0;
}

void gameserver_c::game_remove_player(int pid) {
}

void gameserver_c::disconnectPlayer(int pid, Disconnect_reason reason) {
}

void gameserver_c::nameChange(int id, int pid, const string& tempname, const std::string& password) {
}

void gameserver_c::chat(int pid, const char* sbuf) {
}

bool gameserver_c::changeRegistration(int id, const string& token) {
	return false;
}

void gameserver_c::simulate_and_broadcast_frame() {
}

//run something after simulate_and_broadcast
void gameserver_c::server_think_after_broadcast() {
}

//loop server
// running_flag: pointer to bool, if this bool goes to false, the loop quits.
void gameserver_c::loop(volatile bool *quitFlag, bool acceptEsc) {
}

//stop server
void gameserver_c::stop() {
}

GameserverInterface::GameserverInterface(LogSet hostLog) {
}

GameserverInterface::~GameserverInterface() {
}

bool GameserverInterface::start(int maxplayers) {
	return false;
}

void GameserverInterface::loop(volatile bool* runFlag) {
}

void GameserverInterface::stop() {
}

