#include "incalleg.h"
#include <cstdarg>
#include <string>
#include <strstream>
#include "log.h"
#include "commont.h"	// for time_counter
#include "utility.h"

using std::string;

int atoi(const string& str) {
	return std::atoi(str.c_str());
}

int iround(double value) {
	return static_cast<int>(value + 0.5);
}

string toupper(string str) {
	for (string::iterator s = str.begin(); s != str.end(); ++s)
		*s = toupper(*s);
	return str;
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

void LogSet::operator()(const char* fmt, ...) { va_list args; va_start(args, fmt); (*  normalLog)(fmt, args); va_end(args); }
void LogSet::error(const char* fmt, ...)      { va_list args; va_start(args, fmt); (*   errorLog)(fmt, args); va_end(args); }
void LogSet::security(const char* fmt, ...)   { va_list args; va_start(args, fmt); (*securityLog)(fmt, args); va_end(args); }

void errorMessage(const string& heading, MemoryLog& errorLog) {
	int errors = errorLog.size();
	if (errors) {
		std::ostringstream msg;
		msg << heading;
		for (int count = min(errors, 10); count > 0; --count)
			msg << '\n' << errorLog.pop();
		errors = errorLog.size();
		if (errors)
			msg << "\n+ " << errors << " more";
		allegro_message(msg.str().c_str());
	}
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

double get_time() {
	return ((double)time_counter) / 200.0;
}

