#include "commont.h"

using std::cout;
using std::istream;
using std::string;

int atoi (const string& str) {
	return atoi(str.c_str());
}

void ClientControls::fromKeyboard() {
	const int up     =  1;
	const int down   =  2;
	const int left   =  4;
	const int right  =  8;
	const int run    = 16;
	const int strafe = 32;
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

bool is_keypad(int sc) {
	switch (sc) {
		case KEY_1_PAD:
		case KEY_2_PAD:
		case KEY_3_PAD:
		case KEY_4_PAD:
		//case KEY_5_PAD:
		case KEY_6_PAD:
		case KEY_7_PAD:
		case KEY_8_PAD:
		case KEY_9_PAD:
			return true;
		default:
			return false;
	}
}

void rotate_angle(float& angle, float shift) {
	angle += shift;
	if (angle < 0)
		angle += 360;
	else if (angle >= 360)
		angle -= 360;
}

char* strspnp(char* str, const char* charset) {
	for (; *str; ++str)
		if (strchr(charset, *str)==NULL)
			return str;
	return NULL;
}

const char* strspnp(const char* str, const char* charset) {
	return strspnp(const_cast<char*>(str), charset);
}

char directory_separator;

FILE *game_log;
NLaddress		master_address;
char wheregamedir[WHERE_PATH_SIZE];
double svp_fric, svp_accel, svp_maxspeed;
double svp_fric_run, svp_accel_run, svp_maxspeed_run;
double svp_fric_turbo, svp_accel_turbo, svp_maxspeed_turbo;
double svp_fric_turborun, svp_accel_turborun, svp_maxspeed_turborun;
double svp_flag_penalty;
bool   svp_friendly_fire, svp_friendly_db;

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
bool sound_inited = false;		//install_sound succeeded?
bool force_ip = false;		//force IP?
char force_ip_name[32];		//force IP to what?

volatile bool force_exit = false;	// this is set true when the user tries to close the window

void closeButtonCallback() { force_exit = true; }

void server_status_string(const string& str) {
	if (textserver)
		cout << str << '\n';
	else
		set_window_title(str.c_str());
}

//server timer (10Hz)
volatile int server_speed_counter = 0;

//client game timer (60Hz)
volatile int speed_counter = 0;
volatile int client_netsend_counter = 0;		//sub-counter for keypress sending

//this timer will be used to emulate a better clock()
volatile unsigned long time_counter = 0;

double get_time() {
	return ((double)time_counter) / 200.0;
}

char teamname[2][5];

int iround(double value) {
	return static_cast<int>(value + 0.5);
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

string toupper(string str) {
	for (string::iterator s = str.begin(); s != str.end(); ++s)
		*s = toupper(*s);
	return str;
}

