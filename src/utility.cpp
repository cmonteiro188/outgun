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

const char* getNlErrorString() {
	if (nlGetError() == NL_SYSTEM_ERROR)
		return nlGetSystemErrorStr(nlGetSystemError());
	else
		return nlGetErrorStr(nlGetError());
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

string url_encode(const string& str) {
	ostringstream ost;
	for (string::const_iterator s = str.begin(); s != str.end(); s++)
		url_encode(*s, ost);
	return ost.str();
}

void url_encode(char c, ostream& out) {
	if (is_url_safe(c))	// send safe characters as they are
		out << c;
	else if (c == ' ')	// spaces to + characters
		out << '+';
	else				// encode unsafe characters to %xx
		out << '%' << hex << setw(2) << setfill('0') << static_cast<int>(static_cast<unsigned char>(c));
}

bool is_url_safe(char c) {
	if (c >= 'a' && c <= 'z')
		return true;
	else if (c >= 'A' && c <= 'Z')
		return true;
	else if (c >= '0' && c <= '9')
		return true;
	const string safe_characters("$-_.+!*'(),");
	return safe_characters.find(c) != string::npos;
}

string base64_encode(const string& data) {
	const string conversion_table("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
	const char padding = '=';
	string result;
	// Convert data to 6-bit sequences. Take characters for every sequence
	// from the conversion table.
	for (string::const_iterator s = data.begin(); s != data.end(); s++) {
		// first encoded byte
		char value = (*s >> 2) & 0x3F;
		result += conversion_table[value];
		// second encoded byte
		value = (*s << 4) & 0x3F;
		s++;
		if (s != data.end())
			value |= (*s >> 4) & 0x0F;
		result += conversion_table[value];
		// third encoded byte
		if (s != data.end()) {
			value = (*s << 2) & 0x3F;
			s++;
			if (s != data.end())
				value |= (*s >> 6) & 0x03;
			result += conversion_table[value];
		}
		else
			result += padding;
		// fourth encoded byte
		if (s != data.end()) {
			value = *s & 0x3F;
			result += conversion_table[value];
		}
		else
			result += padding;
	}
	return result;
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

