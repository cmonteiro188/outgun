#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include <cstdarg>
#include <cstdlib>
#include <ctime>

#include "incalleg.h"
#include "log.h"
#include "commont.h"	// for time_counter
#include "utility.h"
#include "thread.h"	// threadRandomize() from there is implemented here

using std::hex;
using std::min;
using std::ostream;
using std::ostringstream;
using std::setfill;
using std::setw;
using std::string;

int atoi(const string& str) {
	return std::atoi(str.c_str());
}

string itoa(int val, int radix) {
	char buf[256];
	itoa(val, buf, radix);
	return buf;
}

int iround(double value) {
	return static_cast<int>(value + 0.5);
}

string toupper(string str) {
	for (string::iterator s = str.begin(); s != str.end(); ++s)
		*s = toupper(*s);
	return str;
}

string trim(string str) {
	str.erase(0, str.find_first_not_of(" \t\n\r\xA0"));
	string::size_type lastGood = str.find_last_not_of(" \t\n\r\xA0");
	if (lastGood == string::npos)
		return string();
	str.erase(lastGood + 1);
	return str;
}

bool find_nonprintable_char(const string& str) {
	for (string::const_iterator s = str.begin(); s != str.end(); ++s)
		if (is_nonprintable_char(*s))
			return true;
	return false;
}

bool is_nonprintable_char(unsigned char c) {
	return c < 32 || (c >= 128 && c <= 159);
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
		ostringstream msg;
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

string date_and_time() {
	time_t tt = time(0);
	const tm* tmb = localtime(&tt);
	const int time_w = 20;
	char time_str[time_w + 1];
	strftime(time_str, time_w, "%Y-%m-%d %H:%M:%S", tmb);
	return time_str;
}

void threadRandomize() {	// declared in thread.h
	srand(time(0));
}

// definitions for incalleg.h

#if ALLEGRO_VERSION == 4 && ALLEGRO_SUB_VERSION == 0
void textprintf_ex(struct BITMAP* bmp, AL_CONST FONT *f, int x, int y, int color, int bg, AL_CONST char* format, ...) {
	text_mode(bg);
	va_list argptr;
	char xbuf[16384];
	va_start(argptr, format);
	vsprintf(xbuf, format, argptr);
	va_end(argptr);
	textout(bmp, f, xbuf, x, y, color);
}
void textprintf_centre_ex(struct BITMAP* bmp, AL_CONST FONT *f, int x, int y, int color, int bg, AL_CONST char* format, ...) {
	text_mode(bg);
	va_list argptr;
	char xbuf[16384];
	va_start(argptr, format);
	vsprintf(xbuf, format, argptr);
	va_end(argptr);
	textout_centre(bmp, f, xbuf, x, y, color);
}
void textprintf_right_ex(struct BITMAP* bmp, AL_CONST FONT *f, int x, int y, int color, int bg, AL_CONST char* format, ...) {
	text_mode(bg);
	va_list argptr;
	char xbuf[16384];
	va_start(argptr, format);
	vsprintf(xbuf, format, argptr);
	va_end(argptr);
	textout_right(bmp, f, xbuf, x, y, color);
}
void textout_ex(struct BITMAP* bmp, AL_CONST FONT *f, AL_CONST char* text, int x, int y, int color, int bg) {
	text_mode(bg);
	textout(bmp, f, text, x, y, color);
}
void textout_centre_ex(struct BITMAP* bmp, AL_CONST FONT *f, AL_CONST char* text, int x, int y, int color, int bg) {
	text_mode(bg);
	textout_centre(bmp, f, text, x, y, color);
}
void textout_right_ex(struct BITMAP* bmp, AL_CONST FONT *f, AL_CONST char* text, int x, int y, int color, int bg) {
	text_mode(bg);
	textout_right(bmp, f, text, x, y, color);
}
#endif	// ALLEGRO_VERSION == 4 && ALLEGRO_SUB_VERSION == 0

