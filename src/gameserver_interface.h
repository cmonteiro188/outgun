#ifndef GAMESERVER_INTERFACE_INC
#define GAMESERVER_INTERFACE_INC

class gameserver_c;
class LogSet;

class GameserverInterface {
	gameserver_c* host;

public:
	GameserverInterface(LogSet& hostLog);
	~GameserverInterface();
	bool start(int maxplayers);
	void loop(volatile bool* runFlag);
	void stop();
};

// implementation is in server.cpp

#endif
