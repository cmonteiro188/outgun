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
	virtual ~LineReceiver() { }

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

template<class HostClass, class ReturnT, class Arg1T>
class RedirectToMemFun1 {
	HostClass* host;
	ReturnT (HostClass::*function)(Arg1T);

public:
	RedirectToMemFun1(HostClass* h, ReturnT (HostClass::*memFun)(Arg1T)) : host(h), function(memFun) { }
	ReturnT operator()(Arg1T arg) { return (host->*function)(arg); }
};

template<class HostClass, class ReturnT, class Arg1T, class Arg2T>
class RedirectToMemFun2 {
	HostClass* host;
	ReturnT (HostClass::*function)(Arg1T, Arg2T);

public:
	RedirectToMemFun2(HostClass* h, ReturnT (HostClass::*memFun)(Arg1T, Arg2T)) : host(h), function(memFun) { }
	ReturnT operator()(Arg1T arg1, Arg2T arg2) { return (host->*function)(arg1, arg2); }
};

template<class ArgT>
class HookFunctionBase {	// base class for functions used with class Hook
public:
	virtual ~HookFunctionBase() { }
	virtual void operator()(ArgT&) = 0;
	virtual HookFunctionBase* clone() = 0;
};

template<class ArgT>
class Hook {
public:
	typedef HookFunctionBase<ArgT> FunctionT;

	Hook() : hookFn(0) { }
	~Hook() { free(); }
	void set(FunctionT* fn) { free(); hookFn = fn; }	// the ownership is transferred
	bool active() const { return hookFn != 0; }
	void call(ArgT& obj) { if (hookFn) (*hookFn)(obj); }

private:
	void free() { if (hookFn) delete hookFn; }

	FunctionT* hookFn;
};

template<class ArgT>
class Hookable {
public:
	typedef typename Hook<ArgT>::FunctionT HookFunctionT;

	virtual ~Hookable() { }
	void setHook(HookFunctionT* fn) { hook.set(fn); }	// the ownership is transferred
	bool isHooked() const { return hook.active(); }

protected:
	void callHook(ArgT& obj) { hook.call(obj); }

private:
	Hook<ArgT> hook;
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

