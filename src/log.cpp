#include <cstdio>
#include <cstdarg>
#include <string>
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
	char buf[2000];
	vsprintf(buf, fmt, args);
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
		time_t tt=time(0);
		struct tm* tmb=localtime(&tt);
		fprintf(fp, "%d-%02d-%02d %02d:%02d:%02d ", tmb->tm_year+1900, tmb->tm_mon+1, tmb->tm_mday, tmb->tm_hour, tmb->tm_min, tmb->tm_sec);
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

