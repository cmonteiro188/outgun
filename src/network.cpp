#include <iomanip>
#include <sstream>
#include <string>

#include <nl.h>

#include "leetnet/sleep.h"
#include "nassert.h"
#include "utility.h"

#include "network.h"

using std::hex;
using std::ostream;
using std::ostringstream;
using std::setfill;
using std::setw;
using std::string;

const char* getNlErrorString() {
	if (nlGetError() == NL_SYSTEM_ERROR)
		return nlGetSystemErrorStr(nlGetSystemError());
	else
		return nlGetErrorStr(nlGetError());
}

bool check_private_IP(const char *address) {
	int i1, i2;
	int n = sscanf(address, "%d.%d.", &i1, &i2);
	nAssert(n == 2);
	if (n != 2)
		return false;
	// private IP ranges:
	// 10.0.0.0        -   10.255.255.255
	// 172.16.0.0      -   172.31.255.255
	// 192.168.0.0     -   192.168.255.255
	// 169.254.0.0     -   169.254.255.255 [used by Microsoft DHCP client]
	return (i1 == 10 || (i1 == 172 && i2 >= 16 && i2 <= 31) || (i1 == 192 && i2 == 168) || (i1 == 169 && i2 == 254));
}

string getPublicIP(LogSet& log) {
	NLaddress* locals;
	NLint nLocals;
	locals = nlGetAllLocalAddr(&nLocals);
	for (int i = 0; i < nLocals; ++i) {
		char buf[NL_MAX_STRING_LENGTH];
		nlAddrToString(&locals[i], buf);
		bool priv = check_private_IP(buf);
		if (priv)
			log("Local address %s ignored", buf);
		else {
			log("Found public address %s", buf);
			return buf;
		}
	}
	log("No public address found");
	return string();
}

bool writeToUnblockingTCP(NLsocket& socket, const char* data, int length, const volatile bool* abortFlag, int timeout, int roundDelay) {
	int at = 0;
	int tries = 0;
	while (at < length) {
		if ((abortFlag && *abortFlag) || tries * roundDelay > timeout)
			return false;

		NLint written = nlWrite(socket, data + at, length - at);
		if (written == NL_INVALID) {
			if (nlGetError() != NL_CON_PENDING)
				return false;
		}
		else
			at += written;

		MS_SLEEP(roundDelay);
		++tries;
	}
	return true;
}

bool saveAllFromUnblockingTCP(NLsocket& socket, ostream& out, const volatile bool* abortFlag, int timeout, int roundDelay) {
	const int buffer_size = 511;
	char lebuf[buffer_size + 1];

	int tries = 0;
	for (;;) {
		if ((abortFlag && *abortFlag) || tries * roundDelay > timeout)
			return false;

		NLint read = nlRead(socket, lebuf, buffer_size);
		if (read == NL_INVALID) {
			if (nlGetError() != NL_CON_PENDING)
				return nlGetError() == NL_MESSAGE_END;
		}
		else {
			lebuf[read] = '\0';
			out << lebuf;
		}

		MS_SLEEP(roundDelay);
		++tries;
	}
}

string url_encode(const string& str) {
	ostringstream ost;
	for (string::const_iterator s = str.begin(); s != str.end(); s++)
		url_encode(*s, ost);
	return ost.str();
}

void url_encode(char c, ostream& out) {
	if (is_url_safe(c))	// send safe characters as they are
		out << c;
	else if (c == ' ')	// spaces to + characters
		out << '+';
	else				// encode unsafe characters to %xx
		out << '%' << hex << setw(2) << setfill('0') << static_cast<int>(static_cast<unsigned char>(c));
}

bool is_url_safe(char c) {
	if (c >= 'a' && c <= 'z')
		return true;
	else if (c >= 'A' && c <= 'Z')
		return true;
	else if (c >= '0' && c <= '9')
		return true;
	const string safe_characters("$-_.+!*'(),");
	return safe_characters.find(c) != string::npos;
}

string base64_encode(const string& data) {
	const string conversion_table("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
	const char padding = '=';
	string result;
	// Convert data to 6-bit sequences. Take characters for every sequence
	// from the conversion table.
	for (string::const_iterator s = data.begin(); s != data.end(); s++) {
		// first encoded byte
		char value = (*s >> 2) & 0x3F;
		result += conversion_table[value];
		// second encoded byte
		value = (*s << 4) & 0x3F;
		s++;
		if (s != data.end())
			value |= (*s >> 4) & 0x0F;
		result += conversion_table[value];
		// third encoded byte
		if (s != data.end()) {
			value = (*s << 2) & 0x3F;
			s++;
			if (s != data.end())
				value |= (*s >> 6) & 0x03;
			result += conversion_table[value];
		}
		else
			result += padding;
		// fourth encoded byte
		if (s != data.end()) {
			value = *s & 0x3F;
			result += conversion_table[value];
		}
		else
			result += padding;
	}
	return result;
}

