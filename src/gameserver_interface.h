#ifndef GAMESERVER_INTERFACE_INC
#define GAMESERVER_INTERFACE_INC

#include <string>
#include "commont.h"
#include "utility.h"

class Server;
class LogSet;

class ServerExternalSettings {
public:
	int port;			//the server port
	bool dedserver;		// dedicated server? only affects what's told to master and asking players
	bool privateserver;	//private server? (will not publish)
	std::string force_ip_name;	//force IP to what?
	int server_maxplayers;	//maxplayers for the local server, given on the command line (don't use anywhere new)
	int lowerPriority, priority, networkPriority;	// lower is used for non-timecritical background threads

	typedef void StatusOutputFnT(const std::string& str);
	StatusOutputFnT* statusOutput;	// must be set properly (non-null) when used

	ServerExternalSettings() : port(DEFAULT_UDP_PORT), dedserver(false), privateserver(false), server_maxplayers(16), statusOutput(0) { }
};

class GameserverInterface {
	Server* host;

public:
	GameserverInterface(LogSet& hostLog, const ServerExternalSettings& settings);
	~GameserverInterface();
	bool start(int maxplayers);
	void loop(volatile bool *quitFlag, bool quitOnEsc);
	void stop();
};

// implementation is in server.cpp

#endif
