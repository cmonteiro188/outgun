#ifndef DLOG_H_INC
#define DLOG_H_INC

#define DISABLE_DLOG

#ifdef DISABLE_DLOG

class DLOG_Scope {
public:
	DLOG_Scope(const char*) { }
	~DLOG_Scope() { }
};

class DLOG_ScopeNeg {
public:
	DLOG_ScopeNeg(const char*) { }
	~DLOG_ScopeNeg() { }
};

class DLOG_ScopeNegStart {
public:
	DLOG_ScopeNegStart(const char*) { }
	~DLOG_ScopeNegStart() { }
};

class DLOG_Main { };

#else	// DISABLE_DLOG

#include <stdio.h>
#include "/ohjlmnti/outguned/src/nassert.h"

#include "Time.h"
#include "Timer.h"

class DLOG_Main {
	FILE* fp;
	pthread_mutex_t writeMutex;

public:
	DLOG_Main() { pthread_mutex_init(&writeMutex, 0); fp = fopen("lnetdlog.txt", "wb"); nAssert(fp); }
	~DLOG_Main() { pthread_mutex_destroy(&writeMutex); fclose(fp); }
	void operator()(const char* str, char mode) {
		int t = GNE::Timer::getCurrentTime().getTotaluSec();
		pthread_mutex_lock(&writeMutex);
		fwrite(&t, sizeof(int), 1, fp);
		fwrite(&mode, sizeof(char), 1, fp);
		fwrite(str, sizeof(char), 7, fp);
		pthread_mutex_unlock(&writeMutex);
	}
};

extern DLOG_Main G_DLOG;

class DLOG_Scope {
	const char* n;

public:
	DLOG_Scope(const char* scopeName) : n(scopeName) { G_DLOG(n, '+'); }
	~DLOG_Scope() { G_DLOG(n, '-'); }
};

class DLOG_ScopeNeg {
	const char* n;

public:
	DLOG_ScopeNeg(const char* scopeName) : n(scopeName) { G_DLOG(n, '-'); }
	~DLOG_ScopeNeg() { G_DLOG(n, '+'); }
};

class DLOG_ScopeNegStart {
public:
	DLOG_ScopeNegStart(const char* n) { G_DLOG(n, '+'); }
};

#endif	// DISABLE_DLOG

#endif	// DLOG_H_INC

