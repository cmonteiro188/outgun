#include <cstdio>
#include <cstdarg>
#include <string>
#include "platform.h"
#include "utility.h"

#include "log.h"

using std::string;

void Log::put(const string& str) {
	lock();
	add(str);
	unlock();
}

void Log::operator()(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	operator()(fmt, args);
	va_end(args);
}

void Log::operator()(const char* fmt, va_list args) {
	char buf[4000];
	platVsnprintf(buf, 4000, fmt, args);
	put(buf);
}

FileLog::FileLog(const string& filename, bool truncate) {
	fileName = filename;
	printDate = !truncate;
	fp = fopen(fileName.c_str(), (truncate ? "wt" : "at"));
}

FileLog::~FileLog() {
	if (!fp)
		return;
	bool emptyFile = (ftell(fp) == 0);
	fclose(fp);
	if (emptyFile)
		remove(fileName.c_str());
}

void FileLog::add(const string& str) {
	if (!fp)
		return;
	if (printDate) {
		fputs(date_and_time().c_str(), fp);
		fputc(' ', fp);
	}
	fprintf(fp, "%9.2f: %s\n", get_time(), str.c_str());
	fflush(fp);
}

void MemoryLog::add(const string& str) {
	data.push_back(str);
}

string MemoryLog::pop() {
	lock();
	if (data.empty()) {
		unlock();
		return string();
	}
	string s = data.front();
	data.pop_front();
	unlock();
	return s;
}

void FileMemLog::add(const string& str) {
	FileLog::add(str);
	MemoryLog::add(str);
}

