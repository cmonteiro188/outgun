#include <nl.h>
#include <fstream>
#include <cassert>

#include "nameauth.h"

using namespace std;


// these are from outgun.cpp, needed for LOG(...) ; argh
#define LOG_EXPR game_log
#define LOG_TIMEFUNC get_time()
#include "leetnet/log.h"
extern FILE *game_log;
extern double get_time();

void NameAuthorizationDatabase::Entry::save(ostream& out) const {
	out << '\n';
	out << "name " << nameUpr << '\n';
	if (password.length())
		out << "password " << password << '\n';
	for (vector<NLaddress>::const_iterator ai=addresses.begin(); ai!=addresses.end(); ++ai) {
		char sbuf[50];
		nlAddrToString(&(*ai), sbuf);
		out << sbuf << '\n';
	}
}

bool NameAuthorizationDatabase::Entry::hasAddress(NLaddress addr) const {
	for (vector<NLaddress>::const_iterator ai=addresses.begin(); ai!=addresses.end(); ++ai)
		if (nlAddrCompare(&addr, &(*ai)))
			return true;
	return false;
}

bool NameAuthorizationDatabase::addEntry(const Entry& e) {
	if (e.nameUpr == "[BANNED]") {
		banned=e;
		banned.password.clear();
	}
	else if (e.nameUpr.length() && e.password.length())
		db.push_back(e);
	else {
		LOG("*** INVALID DATA IN AUTH.TXT\n");
		return false;
	}
	return true;
}

bool NameAuthorizationDatabase::load() {
	ifstream in("auth.txt");
	if (!in) {
		LOG("*** CAN'T READ AUTH.TXT\n");
		return false;
	}
	Entry e;
	bool eActive=false;
	for (;;) {
		string line;
		getline(in, line);
		if (!in) {
			if (eActive)
				return addEntry(e);
			return true;
		}
		if (line.length()==0)
			continue;
		if (line[0]==';')
			continue;
		if (!line.compare(0, 5, "name ")) {
			if (eActive) {
				if (!addEntry(e))
					return false;
				e=Entry();
			}
			eActive=true;
			for (string::size_type i=5; i<line.length(); ++i)
				e.nameUpr+=toupper(line[i]);
			continue;
		}
		if (!line.compare(0, 9, "password ")) {
			if (e.password.length())
				LOG("*** Password redefinition in auth.txt\n");
			eActive=true;
			e.password=line.substr(9, string::npos);
			continue;
		}
		NLaddress addr;
		if (!nlStringToAddr(line.c_str(), &addr))
			LOG1("*** Invalid IP address in auth.txt: %s\n", line.c_str())	// important: no ; because LOG sucks
		else {
			nlSetAddrPort(&addr, 0);
			e.addresses.push_back(addr);
		}
	}
}

bool NameAuthorizationDatabase::save() const {
	ofstream out("auth.txt");
	if (!out) {
		LOG("*** CAN'T WRITE AUTH.TXT\n");
		return false;
	}
	out << "; This file is automatically rewritten whenever valid addresses are added.\n";
	out << "; New reserved names must be added manually: add lines \"name <the name>\"\n";
	out << "; and \"password <the password>\". Then you can add the IP's from the game\n";
	out << "; by typing \"/auth <the name>,<the password>\".\n";
	if (banned.addresses.size())
		banned.save(out);
	for (vector<Entry>::const_iterator dbi=db.begin(); dbi!=db.end(); ++dbi)
		dbi->save(out);
	return true;
}

bool NameAuthorizationDatabase::addIP(const string& nameUpr, const string& password, NLaddress addr) {	// must be an existing name
	for (vector<Entry>::iterator dbi=db.begin(); dbi!=db.end(); ++dbi)
		if (dbi->nameUpr==nameUpr) {
			if (dbi->password!=password)
				return false;
			nlSetAddrPort(&addr, 0);
			dbi->addresses.push_back(addr);
			return true;
		}
	return false;
}

int NameAuthorizationDatabase::identifyName(const string& name) const {
	static const char fromTab[]="!|012357";
	static const char   toTab[]="IIOIZEST";
	string nameUpr, nameAlpha;	// alpha contains only the alphabetical characters
	for (string::size_type i=0; i<name.length(); ++i) {
		char ch = name[i]&127;
		const char* fromp = strchr(fromTab, ch);
		if (fromp)
			ch = toTab[fromp-fromTab];
		if (isalpha(ch)) {
			nameUpr += char(toupper(ch));
			nameAlpha += char(toupper(ch));
		}
		else
			nameUpr += '.';
	}
	for (int idx=0; idx<size(); ++idx) {
		const string& nu=db[idx].nameUpr;
		string::size_type nBegin=nameAlpha.find(nu);
		if (nBegin==string::npos)
			continue;
		// find the same pos in nameUpr which has additional dots in the middle
		string::size_type ui;
		for (ui=0;;) {
			while (nameUpr[ui]=='.')
				++ui;
			if (nBegin==0) break;	// nBegin tells how many real chars more should be deleted
			--nBegin; ++ui;
		}
		assert(nameUpr[ui]==nu[0]);
		if (ui>0 && nameUpr[ui-1]!='.')	// nu is only a part of a word in nameUpr
			continue;
		nBegin=nu.length()-1;	// so that ui will point to the last char in nameUpr
		for (;;) {
			if (nBegin==0) break;	// nBegin tells how many real chars more should be deleted
			--nBegin; ++ui;
			while (nameUpr[ui]=='.')
				++ui;
		}
		assert(nameUpr[ui]==nu[nu.length()-1]);
		if (ui+1<nameUpr.length() && nameUpr[ui+1]!='.')	// nu is only a part of a word in nameUpr
			continue;
		return idx;
	}
	return -1;
}

bool NameAuthorizationDatabase::authorize(int idx, NLaddress addr) const {
	nlSetAddrPort(&addr, 0);
	return db[idx].hasAddress(addr);
}

bool NameAuthorizationDatabase::isBanned(NLaddress addr) const {
	nlSetAddrPort(&addr, 0);
	return banned.hasAddress(addr);
}

void NameAuthorizationDatabase::ban(NLaddress addr) {
	nlSetAddrPort(&addr, 0);
	banned.addresses.push_back(addr);
}

