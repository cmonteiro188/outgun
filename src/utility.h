#ifndef UTILITY_H_INC
#define UTILITY_H_INC

#include <string>
#include <vector>
#include "nassert.h"

// try to keep the includes down: if new includes are needed, consider a separate header

template<class T> T bound(T val, T lb, T hb) { return val <= lb ? lb : val >= hb ? hb : val; }

int atoi(const std::string& str);
std::string itoa(int val, int radix = 10);
int iround(double value);

// Returns the current time in the standard format.
std::string date_and_time();

// Get a verbal approximation of the given time interval
std::string approxTime(int seconds);

// Convert string to uppercase.
std::string toupper(std::string str);

// Trim beginning and trailing whitespaces.
std::string trim(std::string str);

bool find_nonprintable_char(const std::string& str);
bool is_nonprintable_char(unsigned char c);

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
	LogSet(Log* normal, Log* error, Log* security) : normalLog(normal), errorLog(error), securityLog(security) { }
	~LogSet() { }

	void operator()(const char* fmt, ...);
	void error(const char* fmt, ...);
	void security(const char* fmt, ...);
};

class MemoryLog;
void errorMessage(const std::string& heading, MemoryLog& errorLog);

class LineReceiver {
protected:
	LineReceiver() { }
	virtual ~LineReceiver() { }

public:
	virtual LineReceiver& operator()(const std::string& str) =0;
};

bool is_keypad(int sc);

void rotate_angle(float& angle, float shift);

double get_time();

template<class DstType> DstType& volatile_ref_cast(volatile DstType& src) { return const_cast<DstType&>(src); }

#endif

