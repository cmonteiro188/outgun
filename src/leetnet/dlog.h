/*
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
 *  Copyright (C) 2004 Niko Ritari
 */

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
#include "../mutex.h"
#include "../nassert.h"

#include "Time.h"
#include "Timer.h"

class DLOG_Main {
	FILE* fp;
	MutexHolder writeMutex;

public:
	DLOG_Main() { fp = fopen("lnetdlog.txt", "wb"); nAssert(fp); }
	~DLOG_Main() { fclose(fp); }
	void operator()(const char* str, char mode) {
		int t = GNE::Timer::getCurrentTime().getTotaluSec();
		MutexLock ml(writeMutex);
		fwrite(&t, sizeof(int), 1, fp);
		fwrite(&mode, sizeof(char), 1, fp);
		fwrite(str, sizeof(char), 7, fp);
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

