#include "commont.h"

void ClientControls::fromKeyboard() {
	data = 0;
	if (key[KEY_UP   ]) data |= 1;
	if (key[KEY_DOWN ]) data |= 2;
	if (key[KEY_LEFT ]) data |= 4;
	if (key[KEY_RIGHT]) data |= 8;
	if (key[KEY_LSHIFT] || key[KEY_RSHIFT])	// run
		data |= 16;
	if (key[KEY_ALT] || key[KEY_ALTGR])	// strafe
		data |= 32;
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
