#ifndef PLATFORM_H_INC
#define PLATFORM_H_INC

#include <cstdarg>

int platMkdir(const char* path);
int platStricmp(const char* s1, const char* s2);
int platVsnprintf(char* buf, size_t count, const char* fmt, va_list arg);

inline int platSnprintf(char* buf, size_t count, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int ret = platVsnprintf(buf, count, fmt, args);
	va_end(args);
	return ret;
}

#endif
