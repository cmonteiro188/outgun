/*
 *  utility.h
 *
 *  Copyright (C) 2003, 2004, 2006 - Niko Ritari
 *  Copyright (C) 2003, 2004, 2006 - Jani Rivinoja
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

#include <limits>
#include <string>
#include <vector>

#include "nassert.h" // for __attribute__ for non-GCC as well as nAssert

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
int iround_bound(double value); // if value is out of int range, nearest value is used
int numberWidth(int num);   // how many characters num takes when printed

inline double sqr(double value) {  // the square of the given value (just to keep the code readable)
    return value * value;
}

inline int sqr(int value) {
    return value * value;
}

template<class Int1T, class Int2T> Int2T positiveModulo(Int1T val, Int2T modulus) {
    nAssert(modulus > 0);
    return val >= 0 ? val % modulus : modulus - (-val % modulus);
}

double positiveFmod(double val, double modulus);

template<class UnsignedIntT> UnsignedIntT rotateRight(UnsignedIntT val, int bits) {
    nAssert(std::numeric_limits<UnsignedIntT>::is_integer && !std::numeric_limits<UnsignedIntT>::is_signed);
    const unsigned typew = sizeof(UnsignedIntT) * CHAR_BIT;
    bits = positiveModulo(bits, typew);
    return (val >> bits) | (val << (typew - bits));
}

template<class UnsignedIntT> UnsignedIntT rotateLeft(UnsignedIntT val, int bits) { rotateRight(val, -bits); }

// Returns the current time in the standard format.
std::string date_and_time();

// Get a verbal approximation of the given time interval
std::string approxTime(int seconds);

// UTF-8 mode for the dedicated server
extern bool utf8_mode;
void check_utf8_mode();

// Convert string to upper/lower case.
std::string toupper(std::string str);
std::string tolower(std::string str);

// Convert Latin 1 character to upper/lower case.
unsigned char latin1_toupper(unsigned char c);
unsigned char latin1_tolower(unsigned char c);

std::string latin1_to_utf8(unsigned char c);
std::string latin1_to_utf8(const std::string& str);

int utf8_to_latin1(std::string::const_iterator& si, unsigned bytes);
std::string utf8_to_latin1(const std::string& str);

// Case insensitive string comparison.
bool cmp_case_ins(const std::string& a, const std::string& b);

// Strip beginning and trailing whitespaces.
std::string trim(std::string str);

// Replace all occurences of s1 with s2 in text.
std::string replace_all(std::string text, const std::string& s1, const std::string& s2);

std::string& replace_all_in_place(std::string& text, char c1, char c2);

// Replace characters &<>"' with HTML entities or character references.
std::string escape_for_html(std::string text);

// Pad /text/ with /pad/ from the given side until its length is /size/ characters. Do nothing if length >= /size/.
std::string pad_to_size_left (std::string text, int size, char pad = ' ');
std::string pad_to_size_right(std::string text, int size, char pad = ' ');

bool find_nonprintable_char(const std::string& str);
bool is_nonprintable_char(unsigned char c);

// Replace control characters with their C escape sequences (note: for readability, it doesn't convert \ to \\ so the result might be ambiguous)
std::string formatForLogging(const std::string& str);

// Split string to lines, but only at whitespaces.
std::vector<std::string> split_to_lines(const std::string& source, int lineLength, int indent = 0, bool keep_spaces = false);

// strspnp: (Watcom definition) find from str the first char not in charset
char* strspnp(char* str, const char* charset);
const char* strspnp(const char* str, const char* charset);

class LineReceiver {
protected:
    LineReceiver() { }
    virtual ~LineReceiver() { }

public:
    virtual LineReceiver& operator()(const std::string& str) =0;
};

class Log;
class LogSet : public LineReceiver {
    // the objects aren't owned by this class
    Log* normalLog;
    Log* errorLog;
    Log* securityLog;

public:
    LogSet(Log* normal, Log* error, Log* security) : normalLog(normal), errorLog(error), securityLog(security) { }  // null pointers are allowed
    ~LogSet() { }

    LogSet& operator()(const std::string&);
    LogSet& operator()(const char* fmt, ...) PRINTF_FORMAT(2, 3);
    LogSet& error(const std::string&);
    LogSet& security(const char* fmt, ...) PRINTF_FORMAT(2, 3);

    Log* accessNormal() { return normalLog; }
    Log* accessError() { return errorLog; }
    Log* accessSecurity() { return securityLog; }
};

extern bool g_allowBlockingMessages;    // controls all messageBox calls; disable to suppress all external message boxes (assertions included)

void messageBox(const std::string& heading, const std::string& msg, bool blocking = true); // blocking may not be controllable

void criticalError(const std::string& msg) __attribute__ ((noreturn));

class MemoryLog;
void errorMessage(const std::string& heading, MemoryLog& errorLog, const std::string& footer);

void rotate_angle(double& angle, double shift);

class FileName {
public:
    FileName(const std::string& fullName);

    const std::string& getPath() const { return path; } // without trailing separator
    const std::string& getBaseName() const { return base; }
    const std::string& getExtension() const { return ext; } // with '.'
    std::string getFull() const;

private:
    std::string path, base, ext;
};

class CartesianProductIterator {
    const int N1, N2;
    int val1, val2;

public:
    CartesianProductIterator(int n1, int n2) : N1(n1), N2(n1 ? n2 : 0), val1(n1 - 1), val2(-1) { }
    bool next() {
        if (++val1 == N1) {
            if (++val2 == N2)
                return false;
            val1 = 0;
        }
        return true;
    }
    std::pair<int, int> operator()() const { return std::pair<int, int>(val1, val2); } // only valid after next() returning true
    int i1() const { return val1; }
    int i2() const { return val2; }
};

#endif
