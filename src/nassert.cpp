#include <cstdio>
#include <cstdarg>
#include <cstdlib>	// exit
#include <ctime>
#include "incalleg.h"

#include "nassert.h"

void stackDump(FILE* dst) {	// makes heavy assumptions about processor architecture wrt stack! Should work fine on any x86 platform.
	unsigned long unused;
	for (unsigned long* stackPtr = (&unused) + 1; *stackPtr != STACK_GUARD; ++stackPtr) {
		unsigned long value = *stackPtr;
		fwrite(&stackPtr, sizeof(stackPtr), 1, dst);
		fwrite(&value, sizeof(value), 1, dst);
	}
}

void nasprintf(const char* expr, ...) {
	// display to console (should be safest, but rarely visible on Windows)
	va_list argptr;
	va_start(argptr, expr);
	vfprintf(stderr, expr, argptr);
	va_end(argptr);
	// save to assert.log
	FILE* asfile = fopen("assert.log", "at");
	if (asfile) {
		time_t tt=time(0);
		struct tm* tmb=localtime(&tt);
		fprintf(asfile, "%d-%02d-%02d %02d:%02d:%02d  ", tmb->tm_year+1900, tmb->tm_mon+1, tmb->tm_mday, tmb->tm_hour, tmb->tm_min, tmb->tm_sec);
		va_start(argptr, expr);
		vfprintf(asfile, expr, argptr);
		va_end(argptr);
		fclose(asfile);
		FILE* stdump = fopen("stackdump.bin", "wb");
		if (stdump) {
			stackDump(stdump);
			fclose(stdump);
		}
	}
	// display using allegro_message
	va_start(argptr, expr);
	char buf[4096];
	vsprintf(buf, expr, argptr);
	va_end(argptr);
	allegro_message("%s\nTo help us fix this, please send assert.log and stackdump.bin and describe what you were doing to npr1@suomi24.fi", buf);
}

#define ARGP(num) const char* name##num, int val##num

void nAssertFail(const char* expr, const char* file, int line) {
	nasprintf("Assertion failed: %s, file %s, line %d\n", expr, file, line);
	exit(-1);
}

void nAssertFail(const char* expr, ARGP(1), const char* file, int line) {
	nasprintf("Assertion failed: %s (%s==%d), file %s, line %d\n", expr, name1, val1, file, line);
	exit(-1);
}

void nAssertFail(const char* expr, ARGP(1), ARGP(2), const char* file, int line) {
	nasprintf("Assertion failed: %s (%s==%d, %s==%d), file %s, line %d\n",
												expr, name1, val1, name2, val2, file, line);
	exit(-1);
}

void nAssertFail(const char* expr, ARGP(1), ARGP(2), ARGP(3), const char* file, int line) {
	nasprintf("Assertion failed: %s (%s==%d, %s==%d, %s==%d), file %s, line %d\n",
									expr, name1, val1, name2, val2, name3, val3, file, line);
	exit(-1);
}

void nAssertFail(const char* expr, ARGP(1), ARGP(2), ARGP(3), ARGP(4), const char* file, int line) {
	nasprintf("Assertion failed: %s (%s==%d, %s==%d, %s==%d, %s==%d), file %s, line %d\n",
									expr, name1, val1, name2, val2, name3, val3, name4, val4, file, line);
	exit(-1);
}

#undef ARGP

