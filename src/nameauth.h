#ifndef __NAMEAUTH_H_INCLUDED__
#define __NAMEAUTH_H_INCLUDED__

#include <string>
#include <vector>
#include <ostream>
#include <nl.h>

#include "utility.h"	// for LogSet

class NameAuthorizationDatabase {
	struct Entry {
		std::string nameUpr;
		std::string password;
		std::vector<NLaddress> addresses;	// currently not used except for ban records

		void save(std::ostream& out) const;
		bool hasAddress(NLaddress addr) const;
	};
	std::vector<Entry> db;
	Entry banned, softBanned;

	mutable LogSet log;

	int size() const { return db.size(); }
	bool addEntry(const Entry& e);

public:
	NameAuthorizationDatabase(LogSet logs) : log(logs) { banned.nameUpr="[BANNED]"; }

	void clear();
	bool load();
	bool save() const;

	int identifyName(const std::string& name) const;
	const std::string& getName(int idx) const { return db[idx].nameUpr; }
	bool authorize(int idx, const std::string& password) const;
	bool NameAuthorizationDatabase::checkNamePassword(const std::string& nameUpr, const std::string& password) const;

	bool isBanned(NLaddress addr) const;
	void ban(NLaddress addr, bool hard);
};

#endif

