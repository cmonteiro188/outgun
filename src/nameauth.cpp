#include <fstream>
#include <sstream>

#include <nl.h>

#include "commont.h"
#include "nameauth.h"
#include "nassert.h"
#include "network.h"

using std::ifstream;
using std::istringstream;
using std::ofstream;
using std::ostream;
using std::string;
using std::vector;

string NameAuthorizationDatabase::makeComparable(const string& name) {
	string nameUpr;
	for (string::size_type i = 0; i < name.length(); ++i) {
		if (isalnum(name[i]))
			nameUpr += char(toupper(name[i]));
		else if (!nameUpr.empty())
			nameUpr += '.';
	}
	string::size_type endi = nameUpr.find_last_not_of('.');
	if (endi == string::npos)
		return string();
	return nameUpr.substr(0, endi + 1);
}

void NameAuthorizationDatabase::clear() {
	names.clear();
	bans.clear();
}

bool NameAuthorizationDatabase::load() {
	clear();
	ifstream in((wheregamedir + "config" + directory_separator + "auth.txt").c_str());
	if (!in) {
		log.error("Can't read auth.txt");
		return false;
	}
	for (;;) {
		string line;
		getline_smart(in, line);
		if (!in)
			return true;
		if (line[0]==';')
			continue;
		istringstream strl(line);
		string command, name, data;
		strl >> command;
		getline(strl, name, '\t');
		if (!strl) {
			log.error("Invalid line \"%s\" in auth.txt", line.c_str());
			continue;
		}
		name = makeComparable(name);
		strl >> data;
		bool dataRead = strl;
		command = toupper(command);
		if (command == "USER") {
			if (!dataRead)
				log.error("Invalid user command (no password) in auth.txt: \"%s\"", line.c_str());
			else
				names.push_back(NameEntry(name, data, false));
		}
		else if (command == "ADMIN") {
			if (dataRead)
				names.push_back(NameEntry(name, data, true));
			else
				names.push_back(NameEntry(name, "", true));
		}
		else if (command == "BAN") {
			NLaddress addr;
			if (!dataRead || !nlStringToAddr(data.c_str(), &addr))
				log.error("Invalid ban command (IP address) in auth.txt: \"%s\"", line.c_str());
			else {
				nlSetAddrPort(&addr, 0);
				time_t endTime;
				strl >> endTime;
				if (!strl)
					bans.push_back(BanEntry(name, addr));
				else if (endTime > time(0))	// if the ban isn't in effect any more, don't bother loading
					bans.push_back(BanEntry(name, addr, endTime));
			}
		}
		else
			log.error("Unrecognized command \"%s\" in auth.txt", command.c_str());
	}
}

bool NameAuthorizationDatabase::save() const {
	ofstream out((wheregamedir + "config" + directory_separator + "auth.txt").c_str());
	if (!out) {
		log.error("Can't write auth.txt");
		return false;
	}
	out << "; This file is automatically rewritten whenever the ban list changes.\n"
		<< "; To reserve a name add a row:\n"
		<< "; user <name> <tab> <password>  or  admin <name> [<tab> <password>]\n"
		<< "; where <tab> is a tabulator character.\n"
		<< "; Passwordless admins need to authenticate by logging in to the tournament\n"
		<< '\n';
	for (vector<BanEntry>::const_iterator bi = bans.begin(); bi != bans.end(); ++bi)
		out << "ban\t" << bi->name << '\t' << addressToString(bi->address) << '\t' << bi->endTime << '\n';
	for (vector<NameEntry>::const_iterator ni = names.begin(); ni != names.end(); ++ni)
		out << (ni->admin ? "admin" : "name") << '\t' << ni->name << '\t' << ni->password << '\n';
	return true;
}

int NameAuthorizationDatabase::identifyName(const string& name) const {
	string nameUpr = makeComparable(name);
	for (int idx = 0; idx < (int)names.size(); ++idx)
		if (nameUpr == names[idx].name)
			return idx;
	return -1;
}

bool NameAuthorizationDatabase::checkNamePassword(const std::string& name, const std::string& password) const {
	int idx = identifyName(name);
	return (idx == -1 || names[idx].password == password);
}

bool NameAuthorizationDatabase::isBanned(NLaddress addr) const {
	nlSetAddrPort(&addr, 0);
	for (vector<BanEntry>::const_iterator bi = bans.begin(); bi != bans.end(); ++bi)
		if (nlAddrCompare(&addr, &bi->address) && bi->endTime > time(0))
			return true;
	return false;
}

void NameAuthorizationDatabase::ban(NLaddress addr, const string& name, int minutes) {
	nlSetAddrPort(&addr, 0);
	bans.push_back(BanEntry(name, addr, time(0) + minutes * 60));
}

