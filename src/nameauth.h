#ifndef __NAMEAUTH_H_INCLUDED__
#define __NAMEAUTH_H_INCLUDED__

#include <string>
#include <vector>
#include <ostream>

struct _NLaddress;
typedef struct _NLaddress NLaddress;

class NameAuthorizationDatabase {
	typedef std::string string;

    struct Entry {
        string nameUpr;
        string password;
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

    bool addIP(const string& nameUpr, const string& password, NLaddress addr);  // must be an existing name

	int identifyName(const char* name) const;
    string getName(int idx) const { return db[idx].nameUpr; }
	bool authorize(int idx, NLaddress addr) const;

	bool isBanned(NLaddress addr) const;
	void ban(NLaddress addr);
};

#endif

