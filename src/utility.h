#ifndef UTILITY_H_INC
#define UTILITY_H_INC

#include <string>
#include <vector>
#include "nassert.h"

// try to keep the includes down: if new includes are needed, consider a separate header

template<class T> T bound(T val, T lb, T hb) { return val<=lb?lb:val>=hb?hb:val; }

int atoi(const std::string& str);
std::string itoa(int val, int radix = 10);
int iround(double value);

// Returns the current time in the standard format.
std::string date_and_time();

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

// HookFunctionBase#, Hook#, Hookable# : generating new versions with different amounts of arguments should be easy when needed

template<class RetT, class Arg1T>
class HookFunctionBase1 {	// base class for functions used with class Hook
public:
	virtual ~HookFunctionBase1() { }
	virtual RetT operator()(Arg1T) = 0;
	virtual HookFunctionBase1* clone() = 0;
};

template<class RetT, class Arg1T>
class Hook1 {
public:
	typedef HookFunctionBase1<RetT, Arg1T> FunctionT;

	Hook1() : hookFn(0) { }
	~Hook1() { free(); }
	void set(FunctionT* fn) { free(); hookFn = fn; }	// the ownership is transferred
	bool active() const { return hookFn != 0; }
	RetT call(Arg1T a1) { if (hookFn) return (*hookFn)(a1); else return RetT(); }

private:
	void free() { if (hookFn) delete hookFn; }

	FunctionT* hookFn;
};

template<class RetT, class Arg1T>
class Hookable1 {
public:
	typedef typename Hook1<RetT, Arg1T>::FunctionT HookFunctionT;

	virtual ~Hookable1() { }
	void setHook(HookFunctionT* fn) { hook.set(fn); }	// the ownership is transferred
	bool isHooked() const { return hook.active(); }

protected:
	RetT callHook(Arg1T a1) { return hook.call(a1); }

private:
	Hook1<RetT, Arg1T> hook;
};

template<class RetT, class Arg1T, class Arg2T, class Arg3T>
class HookFunctionBase3 {
public:
	virtual ~HookFunctionBase3() { }
	virtual RetT operator()(Arg1T, Arg2T, Arg3T) = 0;
	virtual HookFunctionBase3* clone() = 0;
};

template<class RetT, class Arg1T, class Arg2T, class Arg3T>
class Hook3 {
public:
	typedef HookFunctionBase3<RetT, Arg1T, Arg2T, Arg3T> FunctionT;

	Hook3() : hookFn(0) { }
	~Hook3() { free(); }
	void set(FunctionT* fn) { free(); hookFn = fn; }	// the ownership is transferred
	bool active() const { return hookFn != 0; }
	RetT call(Arg1T a1, Arg2T a2, Arg3T a3) { if (hookFn) return (*hookFn)(a1, a2, a3); else return RetT(); }

private:
	void free() { if (hookFn) delete hookFn; }

	FunctionT* hookFn;
};

template<class RetT, class Arg1T, class Arg2T, class Arg3T>
class Hookable3 {
public:
	typedef typename Hook3<RetT, Arg1T, Arg2T, Arg3T>::FunctionT HookFunctionT;

	virtual ~Hookable3() { }
	void setHook(HookFunctionT* fn) { hook.set(fn); }	// the ownership is transferred
	bool isHooked() const { return hook.active(); }

protected:
	RetT callHook(Arg1T a1, Arg2T a2, Arg3T a3) { return hook.call(a1, a2, a3); }

private:
	Hook3<RetT, Arg1T, Arg2T, Arg3T> hook;
};

bool is_keypad(int sc);

void rotate_angle(float& angle, float shift);

double get_time();

#endif

