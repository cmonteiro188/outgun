#ifndef UTILITY_H_INC
#define UTILITY_H_INC

#include <string>
#include <vector>
#include "nassert.h"

// try to keep the includes down: if new includes are needed, consider a separate header

template<class T> T bound(T val, T lb, T hb) { return val<=lb?lb:val>=hb?hb:val; }

int atoi(const std::string& str);
int iround(double value);

// Convert string to uppercase.
std::string toupper(std::string str);

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

public:
	virtual LineReceiver& operator()(const std::string& str) =0;
};

template<class HostClass, class ReturnT>
class RedirectToMemFun {
	HostClass* host;
	ReturnT (HostClass::*function)();

public:
	RedirectToMemFun(HostClass* h, ReturnT (HostClass::*memFun)()) : host(h), function(memFun) { }
	ReturnT operator()() { return (host->*function)(); }
};

inline void readStr(const char* buf, int& count, std::string& dst) {
	dst.clear();
	while (buf[count])
		dst += buf[count++];
	++count;
}
inline void writeStr(char* buf, int& count, const std::string& src) {
	memcpy(&buf[count], src.data(), src.length());
	count += src.length();
	buf[count++] = '\0';
}

bool is_keypad(int sc);

void rotate_angle(float& angle, float shift);

double get_time();

#endif

