#ifndef NETWORK_H_INC
#define NETWORK_H_INC

#include <iostream>
#include <string>

class LogSet;
class NLSocket;

const char* getNlErrorString();
bool check_private_IP(const char* address);
std::string getPublicIP(LogSet& log);

inline void readStr(const char* buf, int& count, std::string& dst) {
	dst.clear();
	while (buf[count])
		dst += buf[count++];
	++count;
}
inline void writeStr(char* buf, int& count, const std::string& src) {
	memcpy(&buf[count], src.data(), src.length());
	count += src.length();
	buf[count++] = '\0';
}

bool writeToUnblockingTCP(NLsocket& socket, const char* data, int length,
								const volatile bool* abortFlag, int timeout, int roundDelay = 500);
bool saveAllFromUnblockingTCP(NLsocket& socket, std::ostream& out,
								const volatile bool* abortFlag, int timeout, int roundDelay = 500);

std::string url_encode(const std::string& str);
void url_encode(char c, std::ostream& out);
bool is_url_safe(char c);
std::string base64_encode(const std::string& data);

#endif
