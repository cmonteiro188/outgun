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

Server::Server(LogSet& hostLogs) :
	normalLog(wheregamedir + "log" + directory_separator + "serverlog.txt", true),
	errorLog(normalLog, "ERROR: "),
	securityLog(normalLog, "SECURITY WARNING: ", wheregamedir + "log" + directory_separator + "server_securitylog.txt", false),
	log(&normalLog, &errorLog, &securityLog),
	world(this, &network, log),
	network(this, world, log),
	authorizations(log)
{
}

Server::~Server() {
}

void Server::mutePlayer(int pid, int mode, int admin) {	// 0 = unmute, 1 = normal, 2 = mute silently (do not inform the player)
}

void Server::kickPlayer(int pid, int admin, bool ban) {
}

void Server::banPlayer(int pid, int admin, int minutes) {
}

bool Server::check_name_password(const string& name, const string& password) const {
	return false;
}

void Server::ctf_game_restart() {
}

//check if team change requests can be satisfied
void Server::check_team_changes() {
}

//check if a player wants to change teams and if yes, try to fullfill the wish
void Server::check_player_change_teams(int pid) {
}

//move player - move player (f rom) to empty position (t o)
void Server::move_player(int f, int t) {
}

//swap players - both are valid players
void Server::swap_players(int a, int b) {
}

void Server::set_fav_colors(int pid, const vector<char>& colors) {
}

void Server::check_fav_colors(int pid) {
}

void Server::sendMessage(int pid, Message_type type, const std::string& msg) {
}

//refresh team ratings
void Server::refresh_team_score_modifiers() {
}

//score!
void Server::score_frag(int p, int amount) {
}

//score! NEG FRAG (v0.4.8)
void Server::score_neg(int p, int amount) {
}

void Server::load_game_mod() {
}

//load a map from the rotation list
bool Server::load_rotation_map(int pos) {
	return false;
}

bool Server::server_next_map(int reason) {
	return false;
}

//check map exit by vote
void Server::check_map_exit() {
}

//----- THE REST  ----------------

bool Server::reset_settings(bool keepMap) {
	return false;
}

//start server
bool Server::start(int target_maxplayers) {
	return false;
}

int Server::getLessScoredTeam() const {
	return 0;
}

void Server::game_remove_player(int pid) {
}

void Server::disconnectPlayer(int pid, Disconnect_reason reason) {
}

void Server::nameChange(int id, int pid, const string& tempname, const std::string& password) {
}

void Server::chat(int pid, const char* sbuf) {
}

bool Server::changeRegistration(int id, const string& token) {
	return false;
}

void Server::simulate_and_broadcast_frame() {
}

//run something after simulate_and_broadcast
void Server::server_think_after_broadcast() {
}

//loop server
// running_flag: pointer to bool, if this bool goes to false, the loop quits.
void Server::loop(volatile bool *quitFlag, bool acceptEsc) {
}

//stop server
void Server::stop() {
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

