#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <direct.h>

#include "platform.h"

int platMkdir(const char* path) {
	return mkdir(path);
}

int platStricmp(const char* s1, const char* s2) {
	return stricmp(s1, s2);
}

int platVsnprintf(char* buf, size_t count, const char* fmt, va_list arg) {
	return _vsnprintf(buf, count, fmt, arg);
}
