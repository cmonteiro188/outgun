#ifndef __NAMEAUTH_H_INCLUDED__
#define __NAMEAUTH_H_INCLUDED__

#include <string>
#include <vector>
#include <ostream>

#include "nl.h"

class NameAuthorizationDatabase {
    struct Entry {
        std::string nameUpr;
        std::string password;
        std::vector<NLaddress> addresses;

		void save(std::ostream& out) const;
		bool hasAddress(NLaddress addr) const;
    };
    std::vector<Entry> db;
	Entry banned;

    int size() const { return db.size(); }
	bool addEntry(const Entry& e);

public:
	NameAuthorizationDatabase() { banned.nameUpr="[BANNED]"; }

    bool load();
    bool save() const;

    bool addIP(const std::string& nameUpr, const std::string& password, NLaddress addr);  // must be an existing name
    bool NameAuthorizationDatabase::checkNamePassword(const std::string& nameUpr, const std::string& password) const;

	int identifyName(const std::string& name) const;
    const std::string& getName(int idx) const { return db[idx].nameUpr; }
	bool authorize(int idx, NLaddress addr) const;

	bool isBanned(NLaddress addr) const;
	void ban(NLaddress addr);
};

#endif

