/*

	Admin Shell
	
	

*/

#ifndef _ADMSHELL_H_
#define _ADMSHELL_H_



//admin terminal-to-server message codes
enum {

	ATS_NOOP = 0,						//0= no-op
	ATS_GET_PLAYER_FRAGS,		//1... request the frags amount of a player <int id>
	ATS_GET_PLAYER_TOTAL_TIME,		//request the frags amount of a player <int id>
	ATS_GET_PLAYER_TOTAL_KILLS,		//request the total kills amount of a player <int id>
	ATS_GET_PLAYER_TOTAL_DEATHS,		//request the total deaths amount of a player <int id>
	ATS_GET_PLAYER_TOTAL_CAPTURES,		//request the total captures amount of a player <int id>
	ATS_SERVER_CHAT,									//server is saying <string chat line>
	ATS_QUIT,							//QUIT flag, very important. on receiving, should close socket and quit
	ATS_GET_PINGS,	//#NR
	ATS_KICK_PLAYER,	//#NR
	ATS_BAN_PLAYER,	//#NR
	ATS_MUTE_PLAYER,	//#NR

	NUMBER_OF_ATS
};

//server-to-admin terminal message codes
enum {

	STA_NOOP = 0,							//0 = no-op
	STA_PLAYER_CONNECTED,			//1 .... player connected <int id>
	STA_PLAYER_DISCONNECTED,	//player disconnected <int id>
	STA_PLAYER_NAME_UPDATE,		//player changes name <int id> <string name>
	STA_PLAYER_KILLS,					//player scores a kill <int id>
	STA_PLAYER_DIES,					//player dies <int id>
	STA_PLAYER_CAPTURES,			//player captures <int id>
	STA_GAME_OVER,						//game is over (no parameters)
	STA_PLAYER_FRAGS,					//player's current frags amount <int id> <int frags>
	STA_PLAYER_TOTAL_TIME,		//player's total time playing since connected <int id> <int time-in-seconds>
	STA_PLAYER_TOTAL_KILLS,		//player's total kills since connected <int id> <int total-kills>
	STA_PLAYER_TOTAL_DEATHS,	//player's total deaths since connected <int id> <int total-deaths>
	STA_PLAYER_TOTAL_CAPTURES,//player's total captures since connected <int id> <int total-captures>
	STA_GAME_TEXT,   					//only one parameter: a <string> of broadcast text, identical to the one that players get
	STA_QUIT,							//QUIT flag, very important. on receiving, should close socket and quit
	STA_PLAYER_PING,	//#NR
	STA_ADMIN_MESSAGE,	//#NR
	STA_PLAYER_IP,	//#NR

	NUMBER_OF_STA
};



#endif

