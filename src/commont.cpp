#include <iostream>
#include <string>

#include <cstdlib>

#include "incalleg.h"
#include "utility.h"
#include "commont.h"
#include "nassert.h"

#include <pthread.h>	// must include _after_ incalleg.h

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

istream& getline_skip_comments(istream& in, string& str) {
	while (getline_smart(in, str))
    	if (str[0] != ';')	// str is never empty when getline_smart succeeds
        	return in;
	return in;
}

bool check_name(const std::string& name) {
	if (name.length() > 15)
		return false;
	if (name.find_first_not_of(" \xA0") == string::npos)	// Name with only spaces and no-brake spaces not allowed.
		return false;
	if (find_nonprintable_char(name))
		return false;
	if (name.find_first_of(" \xA0") == 0 || name.find_last_of(" \xA0") == name.length() - 1)
		return false;
	return true;
}

int threadPriority() {
	int policy;
	sched_param param;
	pthread_getschedparam(pthread_self(), &policy, &param);
	return param.sched_priority;
}
