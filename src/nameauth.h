#ifndef __NAMEAUTH_H_INCLUDED__
#define __NAMEAUTH_H_INCLUDED__

#include <ostream>
#include <string>
#include <vector>

#include <ctime>

#include <nl.h>

#include "utility.h"	// for LogSet

class NameAuthorizationDatabase {
	struct NameEntry {
		std::string name;
		std::string password;
		bool admin;

		NameEntry(std::string n, std::string p, bool a) : name(n), password(p), admin(a) { }
	};
	struct BanEntry {
		std::string name;
		NLaddress address;
		time_t endTime;

		BanEntry(std::string n, NLaddress a, time_t e = time(0) + 365 * 24 * 60 * 60) : name(n), address(a), endTime(e) { }
	};

	std::vector<NameEntry> names;
	std::vector<BanEntry> bans;

	mutable LogSet log;

	static std::string makeComparable(const std::string& name);

public:
	NameAuthorizationDatabase(LogSet logs) : log(logs) { }

	void clear();
	bool load();
	bool save() const;

	int identifyName(const std::string& name) const;
	bool NameAuthorizationDatabase::checkNamePassword(const std::string& name, const std::string& password) const;

	bool isBanned(NLaddress addr) const;
	void ban(NLaddress addr, std::string name, int minutes);
};

#endif

