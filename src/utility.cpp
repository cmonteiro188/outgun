/*
 *  utility.cpp
 *
 *  Copyright (C) 2003, 2004 - Niko Ritari
 *  Copyright (C) 2003, 2004 - Jani Rivinoja
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

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <cstdarg>
#include <cstdlib>
#include <ctime>

#include "incalleg.h"
#include "log.h"
#include "commont.h"	// for time_counter
#include "platform.h"
#include "utility.h"

using std::dec;
using std::hex;
using std::min;
using std::ostream;
using std::ostringstream;
using std::setfill;
using std::setw;
using std::string;
using std::vector;

int atoi(const string& str) {
	return std::atoi(str.c_str());
}

string itoa(int val) {
	ostringstream ss;
	ss << val;
	return ss.str();
}

string fcvt(double val) {
	ostringstream ss;
	ss << val;
	return ss.str();
}

int iround(double value) {
	if (value >= 0)
		return static_cast<int>(value + 0.5);
	else
		return static_cast<int>(value - 0.5);
}

string toupper(string str) {
	for (string::iterator s = str.begin(); s != str.end(); ++s)
		*s = toupper(*s);
	return str;
}

string trim(string str) {
	str.erase(0, str.find_first_not_of(" \t\n\r\xA0"));
	const string::size_type lastGood = str.find_last_not_of(" \t\n\r\xA0");
	if (lastGood == string::npos)
		return string();
	if (lastGood + 1 < str.length())
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

string formatForLogging(const string& str) {
	ostringstream result;
	for (string::const_iterator s = str.begin(); s != str.end(); ++s) {
		if (is_nonprintable_char(*s)) {
			result << '\\';
			switch (*s) {
				case '\n': result << 'n'; break;
				case '\r': result << 'r'; break;
				case '\t': result << 't'; break;
				case '\v': result << 'v'; break;
				case '\b': result << 'b'; break;
				case '\f': result << 'f'; break;
				default:
					result << 'x' << setw(2) << setfill('0') << hex << static_cast<unsigned char>(*s) << dec << setfill(' ');
					break;
			}
		}
		else
			result << *s;
	}
	return result.str();
}

// Split string to lines, but only at whitespaces.
vector<string> split_to_lines(const string& source, int lineLength, int indent) {
    vector<string> lines;
    if (source.empty())
        return lines;
    size_t start = 0, end = 0;
    while (end != string::npos) {
        if (source.length() < start + lineLength)
            end = string::npos;
        else
            end = source.find_last_of(" \t", start + lineLength);
        if (end <= start)
            end = source.find_first_of(" \t", start + lineLength);
		string line;
		if (start == 0)	// first line
			lineLength -= indent;	// next lines are shorter because of the indent
		else
			line = string(indent, ' ');	// apply indentation
        line += source.substr(start, end - start);
        lines.push_back(line);
        start = end + 1;
    }
    return lines;
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

void LogSet::operator()(const char* fmt, ...) { if (!  normalLog) return; va_list args; va_start(args, fmt); (*  normalLog)(fmt, args); va_end(args); }
void LogSet::error     (const char* fmt, ...) { if (!   errorLog) return; va_list args; va_start(args, fmt); (*   errorLog)(fmt, args); va_end(args); }
void LogSet::security  (const char* fmt, ...) { if (!securityLog) return; va_list args; va_start(args, fmt); (*securityLog)(fmt, args); va_end(args); }

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

void rotate_angle(double& angle, double shift) {
	angle += shift;
	if (angle < 0)
		angle += 360;
	else if (angle >= 360)
		angle -= 360;
}

double get_time() {
	return time_counter / 200.0;
}

string date_and_time() {
	time_t tt = time(0);
	const tm* tmb = localtime(&tt);
	const int time_w = 20;
	char time_str[time_w + 1];
	strftime(time_str, time_w, "%Y-%m-%d %H:%M:%S", tmb);
	return time_str;
}

string approxTime(int seconds) {
	int time = seconds;
	const char* timeUnit;
	if (time == 0 || (time % 60 != 0 && time <= 200))
		timeUnit = "second";
	else {
		time = (time + 40) / 60;	// 40 chosen because rounding up is favored
		if (time % 60 != 0 && time <= 200)
			timeUnit = "minute";
		else {
			time = (time + 40) / 60;
			if (time % 24 != 0 && time <= 100)
				timeUnit = "hour";
			else {
				time = (time + 16) / 24;
				// because a year isn't an integral amount of weeks, it must be handled separately
				if (time % 365 == 0 || time >= 7 * 100) {	// show more than 100 weeks in years
					time = (time + 250) / 365;
					timeUnit = "year";
				}
				else if (time % 7 != 0 && time <= 50)
					timeUnit = "day";
				else {
					time = (time + 5) / 7;
					timeUnit = "week";
				}
			}
		}
	}
	string str = itoa(time) + ' ' + timeUnit;
	if (time != 1)
		str += 's';
	return str;
}

// definitions for incalleg.h

#if ALLEGRO_VERSION == 4 && ALLEGRO_SUB_VERSION == 0
void textprintf_ex(struct BITMAP* bmp, AL_CONST FONT *f, int x, int y, int color, int bg, AL_CONST char* format, ...) {
	text_mode(bg);
	va_list argptr;
	char xbuf[16384];
	va_start(argptr, format);
	platVsnprintf(xbuf, 16384, format, argptr);
	va_end(argptr);
	textout(bmp, f, xbuf, x, y, color);
}
void textprintf_centre_ex(struct BITMAP* bmp, AL_CONST FONT *f, int x, int y, int color, int bg, AL_CONST char* format, ...) {
	text_mode(bg);
	va_list argptr;
	char xbuf[16384];
	va_start(argptr, format);
	platVsnprintf(xbuf, 16384, format, argptr);
	va_end(argptr);
	textout_centre(bmp, f, xbuf, x, y, color);
}
void textprintf_right_ex(struct BITMAP* bmp, AL_CONST FONT *f, int x, int y, int color, int bg, AL_CONST char* format, ...) {
	text_mode(bg);
	va_list argptr;
	char xbuf[16384];
	va_start(argptr, format);
	platVsnprintf(xbuf, 16384, format, argptr);
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
