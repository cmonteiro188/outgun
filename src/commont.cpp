#include <iostream>
#include <string>

#include <cstdlib>

#include "incalleg.h"
#include "commont.h"

using std::cout;
using std::istream;
using std::string;

void ClientControls::fromKeyboard() {
	data = 0;
	if (key[KEY_UP] || key[KEY_8_PAD])
		data |= up;
	if (key[KEY_DOWN] || key[KEY_2_PAD])
		data |= down;
	if (key[KEY_LEFT] || key[KEY_4_PAD])
		data |= left;
	if (key[KEY_RIGHT] || key[KEY_6_PAD])
		data |= right;
	if (key[KEY_7_PAD])
		data |= up | left;
	if (key[KEY_9_PAD])
		data |= up | right;
	if (key[KEY_1_PAD])
		data |= down | left;
	if (key[KEY_3_PAD])
		data |= down | right;
	if (key[KEY_LSHIFT] || key[KEY_RSHIFT])
		data |= run;
	if (key[KEY_ALT] || key[KEY_ALTGR])
		data |= strafe;
}

void ClientControls::fromJoystick() {
	if (poll_joystick())
		return;		// failure
	// Do not reset data because keyboard controls should remain.
	const JOYSTICK_INFO& joystick = joy[0];
	if (joystick.num_sticks >= 1) {
		if (joystick.stick[0].num_axis >= 2) {
			if (joystick.stick[0].axis[0].d1)
				data |= left;
			if (joystick.stick[0].axis[0].d2)
				data |= right;
			if (joystick.stick[0].axis[1].d1)
				data |= up;
			if (joystick.stick[0].axis[1].d2)
				data |= down;
		}
	}
	// The first button is for shooting, which is a bit more important than running.
	if (joystick.num_buttons >= 2) {
		if (joystick.button[1].b)
			data |= run;
		if (joystick.button[2].b)
			data |= strafe;
	}
}

NLaddress master_address;
string wheregamedir;
double svp_fric, svp_accel, svp_maxspeed;
double svp_fric_run, svp_accel_run, svp_maxspeed_run;
double svp_fric_turbo, svp_accel_turbo, svp_maxspeed_turbo;
double svp_fric_turborun, svp_accel_turborun, svp_maxspeed_turborun;
double svp_flag_penalty;
bool   svp_friendly_fire, svp_friendly_db;
bool   svp_player_collisions;

void set_default_physics() {
	svp_fric = 1.5;
	svp_accel = 2.0;
	svp_maxspeed = 12.0;
	svp_fric_run = 1.5;
	svp_accel_run = 2.0;
	svp_maxspeed_run = 22.0;
	svp_fric_turbo = 3.0;
	svp_accel_turbo = 4.0;
	svp_maxspeed_turbo = 18.0;
	svp_fric_turborun = 3.0;
	svp_accel_turborun = 4.0;
	svp_maxspeed_turborun = 33.0;
	svp_flag_penalty = 3.0;
	svp_friendly_fire = false;
	svp_friendly_db = false;
	svp_player_collisions = false;
}

int maxplayers = MAX_PLAYERS;		// the maximum number of players configured for this server (must be <= MAX_PLAYERS and an EVEN NUMBER == NUMERO PAR)

bool dedserver = false;		//dedicated server? -ded
bool textserver = false;		//textmode dedicated server for UNIX/LINUX (V0.5.0) (WON'T WORK ON WINDOWS...)
bool privateserver = false;	//private server? (will not publish)
bool winclient = true;		//windowed client?	-win / -fs
bool trypageflip = false;	//try page flipping? -flip / -dbuf
bool nosound = false;			//disable sound? -nosound
int targetfps = 60;			//target (MAX) frames-per-second
int port = DEFAULT_UDP_PORT;				//the server port
int server_maxplayers = 16;		//default maxplayers of the server
bool force_ip = false;		//force IP?
char force_ip_name[32];		//force IP to what?

volatile bool force_exit = false;	// this is set true when the user tries to close the window

void closeButtonCallback() { force_exit = true; }

void server_status_string(const string& str) {
	#ifndef ALLEGRO_WINDOWS
	if (textserver)
		cout << str << '\n';
	else
	#endif
		set_window_title(str.c_str());
}

istream& getline_smart(istream& in, string& str) {
	str.clear();
	while (1) {
		const char c = in.get();
		if (!in) {
			if (!str.empty())
				in.clear();
			return in;
		}
		if (c == '\n' || c == '\r') {
			if (str.empty())
				continue;
			else
				return in;
		}
		str += c;
	}
}

FileReader::FileReader(const string& filename) {
	file.open(filename.c_str());
}

//#fix: some error handling?
string FileReader::readLine() {	// returns an empty string at end of file
	string s;
	do {
		getline_smart(file, s);
	} while (file && !s.empty() && s[0] == ';');
	return s;
}

