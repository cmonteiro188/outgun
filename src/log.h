#ifndef LOG_H_INC
#define LOG_H_INC

#include <cstdio>
#include <string>
#include <deque>
#include <cstdarg>
#include <nl.h>
#include "mutex.h"

class Log {	// base class
	mutable MutexHolder m;

	virtual void add(const std::string& str) =0;

protected:
	// note: operator() locks the mutex automatically, so it is already locked on an add() call
	void lock() const { m.lock(); }
	void unlock() const { m.unlock(); }

public:
	Log() { }
	virtual ~Log() { }
	void put(const std::string& str);
	void operator()(const char* fmt, ...);
	void operator()(const char* fmt, va_list args);
};

class NoLog : public Log {
	void add(const std::string&) { }

public:
	NoLog() { }
};

class FileLog : public virtual Log {
	FILE* fp;
	std::string fileName;
	bool printDate;

protected:
	virtual void add(const std::string& str);

public:
	FileLog(const std::string& filename, bool truncate);
	virtual ~FileLog();
};

class MemoryLog : public virtual Log {
	std::deque<std::string> data;

protected:
	virtual void add(const std::string& str);

public:
	MemoryLog() { }
	int size() const { lock(); int sz = data.size(); unlock(); return sz; }
	std::string pop();	// returns empty string when there's nothing more to pop
};

class FileMemLog : public FileLog, public MemoryLog {
protected:
	virtual void add(const std::string& str);

public:
	FileMemLog(const std::string& filename, bool truncate) : FileLog(filename, truncate) { }
};

template<class Base>	// Base can be any Log variant
class SupplementaryLog : public Base {
	Log& host;
	std::string prefix;

protected:
	virtual void add(const std::string& str) {
		host.put(prefix + str);
		Base::add(str);
	}

public:
	SupplementaryLog(Log& hostLog, const std::string& outputPrefix) : host(hostLog), prefix(outputPrefix) { }
	template<class A1> SupplementaryLog(Log& hostLog, const std::string& outputPrefix, A1 a1) : Base(a1), host(hostLog), prefix(outputPrefix) { }
	template<class A1, class A2> SupplementaryLog(Log& hostLog, const std::string& outputPrefix, A1 a1, A2 a2) : Base(a1, a2), host(hostLog), prefix(outputPrefix) { }
};

#endif
