/*
 *  utility.h
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

#ifndef UTILITY_H_INC
#define UTILITY_H_INC

#include <string>
#include <vector>
#include "nassert.h"

// try to keep the includes down: if new includes are needed, consider a separate header

#ifdef __GNUC__
#define PRINTF_FORMAT(a, b) __attribute__ ((format (printf, a, b)))
#else
#define PRINTF_FORMAT(a, b)
#endif

template<class T> T bound(T val, T lb, T hb) { return val <= lb ? lb : val >= hb ? hb : val; }

int atoi(const std::string& str);
std::string itoa(int val);
std::string itoa_w(int val, int width, bool left = false);
std::string fcvt(double val);
std::string fcvt(double val, int precision);
int iround(double value);
int numberWidth(int num);   // how many characters num takes when printed

inline double sqr(double value) {  // the square of the given value (just to keep the code readable)
    return value * value;
}

// Returns the current time in the standard format.
std::string date_and_time();

// Get a verbal approximation of the given time interval
std::string approxTime(int seconds);

// Convert string to uppercase.
std::string toupper(std::string str);

// Strip beginning and trailing whitespaces.
std::string trim(std::string str);

// Replace all occurences of s1 to s2 in text.
std::string replace_all(std::string text, const std::string& s1, const std::string& s2);

// Pad /text/ with /pad/ from the given side until it's length is /size/ characters. Do nothing if length >= /size/.
std::string pad_to_size_left (std::string text, int size, char pad = ' ');
std::string pad_to_size_right(std::string text, int size, char pad = ' ');

bool find_nonprintable_char(const std::string& str);
bool is_nonprintable_char(unsigned char c);

// Replace control characters with their C escape sequences (note: for readability, it doesn't convert \ to \\ so the result might be ambiguous)
std::string formatForLogging(const std::string& str);

// Split string to lines, but only at whitespaces.
std::vector<std::string> split_to_lines(const std::string& source, int lineLength, int indent = 0);

// strspnp: (Watcom definition) find from str the first char not in charset
char* strspnp(char* str, const char* charset);
const char* strspnp(const char* str, const char* charset);

class Log;
class LogSet {
    // the objects aren't owned by this class
    Log* normalLog;
    Log* errorLog;
    Log* securityLog;

public:
    LogSet(Log* normal, Log* error, Log* security) : normalLog(normal), errorLog(error), securityLog(security) { }  // null pointers are allowed
    ~LogSet() { }

    void operator()(const char* fmt, ...) PRINTF_FORMAT(2, 3);
    void error(const std::string);
    void security(const char* fmt, ...) PRINTF_FORMAT(2, 3);

    Log* accessNormal() { return normalLog; }
    Log* accessError() { return errorLog; }
    Log* accessSecurity() { return securityLog; }
};

extern bool g_allowBlockingMessages;    // controls all messageBox calls; disable to suppress all external message boxes (assertions included)

void messageBox(const std::string& heading, const std::string& msg);

class MemoryLog;
void errorMessage(const std::string& heading, MemoryLog& errorLog, const std::string& footer);

class LineReceiver {
protected:
    LineReceiver() { }
    virtual ~LineReceiver() { }

public:
    virtual LineReceiver& operator()(const std::string& str) =0;
};

bool is_keypad(int sc);

void rotate_angle(double& angle, double shift);

double get_time();

template<class DstType> DstType& volatile_ref_cast(volatile DstType& src) { return const_cast<DstType&>(src); }

#endif
