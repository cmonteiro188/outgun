#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#include <cctype>
#include <cmath>

#include "incalleg.h"
#include "leetnet/client.h"
#include "leetnet/rudp.h"	// get_self_IP
#include "leetnet/sleep.h"	// sleep util
#include "commont.h"
#include "gameserver_interface.h"
#include "names.h"
#include "nassert.h"
#include "network.h"
#include "protocol.h"	// needed for possible definition of SEND_FRAMEOFFSET, and otherwise
#include "utility.h"
#include "world.h"

#include "client.h"

using std::deque;
using std::endl;
using std::find;
using std::ifstream;
using std::ios;
using std::istringstream;
using std::left;
using std::list;
using std::max;
using std::min;
using std::ofstream;
using std::ostream;
using std::ostringstream;
using std::pair;
using std::right;
using std::setfill;
using std::setw;
using std::stable_sort;
using std::string;
using std::vector;

//#define ROOM_CHANGE_BENCHMARK

const int PASSBUFFER = 32;	//size of password file

#ifdef ROOM_CHANGE_BENCHMARK
int benchmarkRuns = 0;
#endif

Client *gameclient;	//#fix: get rid

class ClientPhysicsCallbacks : public PhysicsCallbacksBase {
	Client& c;

public:
	ClientPhysicsCallbacks(Client& c_) : c(c_) { }

	bool collideToRockets() const { return false; }
	bool gatherMovementDistance() const { return false; }
	bool allowRoomChange() const { return false; }
	void addMovementDistance(int, float) { }
	void playerScreenChange(int) { }
	void rocketHitWall(int rid, bool power, float x, float y, int roomx, int roomy) { c.rocketHitWallCallback(rid, power, x, y, roomx, roomy); }
	bool rocketHitPlayer(int, int) { return false; }
	void playerHitWall(int pid) { c.playerHitWallCallback(pid); }
	void playerHitPlayer(int pid1, int pid2) { c.playerHitPlayerCallback(pid1, pid2); }
	void rocketOutOfBounds(int rid) { c.rocketOutOfBoundsCallback(rid); }
	bool shouldApplyPhysicsToPlayer(int pid) { return c.shouldApplyPhysicsToPlayerCallback(pid); }
};

class TM_DoDisconnect : public ThreadMessage {
public:
	void execute(Client* cl) const { cl->disconnect_command(); }
};

class TM_Text : public ThreadMessage {
	Message_type type;
	string text;

public:
	TM_Text(Message_type type_, const string& text_) : type(type_), text(text_) { }
	void execute(Client* cl) const { cl->print_message(type, text); }
};

class TM_Sound : public ThreadMessage {
	int sample;

public:
	TM_Sound(int sample_) : sample(sample_) { }
	void execute(Client* cl) const { cl->client_sounds.play(sample); }
};

class TM_MapChange : public ThreadMessage {
	string name;
	NLushort crc;

public:
	TM_MapChange(const string& name_, NLushort crc_) : name(name_), crc(crc_) { }
	void execute(Client* cl) const { cl->server_map_command(name, crc); }
};

class TM_NameAuthorizationRequest : public ThreadMessage {
public:
	void execute(Client* cl) const { cl->m_playerPassword.setup(cl->playername, false); cl->showMenu(cl->m_playerPassword); }
};

class TM_GunexploEffect : public ThreadMessage {
	int rokx, roky, px, py;

public:
	TM_GunexploEffect(int rokx_, int roky_, int px_, int py_) : rokx(rokx_), roky(roky_), px(px_), py(py_) { }
	void execute(Client* cl) const { cl->client_graphics.create_gunexplo(rokx, roky, px, py); }
};

class TM_Deathbringer : public ThreadMessage {
	int team, hx, hy, sx, sy;
	double time;

public:
	TM_Deathbringer(int team_, double time_, int hx_, int hy_, int sx_, int sy_) : team(team_), hx(hx_), hy(hy_), sx(sx_), sy(sy_), time(time_) { }
	void execute(Client* cl) const { cl->client_graphics.create_deathbringer(team, time, hx, hy, sx, sy); }
};

class TM_ServerSettings : public ThreadMessage {
	NLubyte caplimit, timelimit, extratime, misc1;

public:
	TM_ServerSettings(NLubyte caplimit_, NLubyte timelimit_, NLubyte extratime_, NLubyte misc1_) : caplimit(caplimit_), timelimit(timelimit_), extratime(extratime_), misc1(misc1_) { }
	void execute(Client* cl) const;
};

class TM_ConnectionUpdate : public ThreadMessage {
	int code;
	char* data;
	int length;

public:
	TM_ConnectionUpdate(int code_, const void* data_, int length_);
	~TM_ConnectionUpdate() { if (data) delete[] data; }
	void execute(Client* cl) const;
};

void ServerThreadOwner::threadFn(const ServerExternalSettings& config) {
	if (LOG_THREAD_IDS)
		log("ServerThreadOwner::threadFn() ID = %d, prio = %d", pthread_self(), threadPriority());

	GameserverInterface gameserver(log, config);
	if (!gameserver.start(config.server_maxplayers)) {
		log.error("Can't start listen server");
		quitFlag = true;
		return;
	}

	log("Listen server running");
	gameserver.loop(&quitFlag, false);
	quitFlag = true;
	gameserver.stop();
	log("Listen server stopped");

	//restore client's windowtitle
	config.statusOutput("Outgun client");	// note: this is the server's statusOutput not client's

	if (LOG_THREAD_IDS)
		log("exiting: ServerThreadOwner::threadFn() ID = %d, prio = %d", pthread_self(), threadPriority());
}

void ServerThreadOwner::start(int port, const ServerExternalSettings& config) {
	nAssert(quitFlag && !threadFlag);
	runPort = port;
	quitFlag = false;
	threadFlag = true;
	RedirectToMemFun1<ServerThreadOwner, void, const ServerExternalSettings&> rmf(this, &ServerThreadOwner::threadFn);
	serverThread.start_assert(rmf, config, config.priority);
}

void ServerThreadOwner::stop() {
	nAssert(threadFlag);
	quitFlag = true;
	threadFlag = false;
	serverThread.join();
}

void TournamentPasswordManager::start() {
	quitThread = false;
	thread.start_assert(RedirectToMemFun0<TournamentPasswordManager, void>(this, &TournamentPasswordManager::threadFn), priority);
}

void TournamentPasswordManager::setToken(const std::string& newToken) {
	if (token.read() != newToken) {
		token = newToken;
		tokenCallback(newToken);
	}
}

TournamentPasswordManager::TournamentPasswordManager(LogSet logs, TokenCallbackT tokenCallbackFunction, int threadPriority) :
	log(logs),
	tokenCallback(tokenCallbackFunction),
	quitThread(true),
	passStatus(PS_noPassword),
	priority(threadPriority),
	servStatus(PS_noPassword)	// no server
{
}

void TournamentPasswordManager::stop() {
	if (!quitThread) {
		log("Joining password-token thread");
		quitThread = true;
		thread.join();
	}
}

void TournamentPasswordManager::changeData(const string& newName, const string& newPass) {
	if (newName == name && newPass == password)
		return;

	stop();
	setToken("");
	if (newPass.empty()) {
		passStatus = PS_noPassword;
		return;
	}
	name = newName;
	password = newPass;
	passStatus = PS_starting;
	start();
}

const char* TournamentPasswordManager::statusAsString() const {
	switch (status()) {
		case PS_noPassword:			return "No password set";
		case PS_starting:			return "Initializing...";
		case PS_socketError:		return "Socket error";
		case PS_sending:			return "Sending login...";
		case PS_sendError:			return "Error sending";
		case PS_receiving:			return "Waiting for response...";
		case PS_recvError:			return "No response";
		case PS_invalidResponse:	return "Invalid response received";
		case PS_unavailable:		return "Master server unavailable";
		case PS_tokenReceived:		return "Logged in";
		case PS_badLogin:			return "Login failed: check password";
		case PS_tokenSent:			return "Logged in; sent to server";
		case PS_tokenAccepted:		return "Logged in; server accepted";
		case PS_tokenRejected:		return "Error: server rejected";
		default: nAssert(0); return 0;
	}
}

void TournamentPasswordManager::threadFn() {
	if (LOG_THREAD_IDS)
		log("TournamentPasswordManager::threadFn() ID = %d, prio = %d", pthread_self(), threadPriority());

	bool newToken = true;
	int delay = 0;	// given a value in MS before each continue: this time will be waited before next round

	while (!quitThread) {
		if (delay > 0) {
			MS_SLEEP(500);
			delay -= 500;
			continue;
		}
		delay = 60000;	// default to one minute

		nlOpenMutex.lock();
		nlDisable(NL_BLOCKING_IO);
		NLsocket sock = nlOpen(0, NL_RELIABLE);
		nlOpenMutex.unlock();
		if (sock == NL_INVALID) {
			log("Password thread: Can't open socket. %s", getNlErrorString());
			passStatus = PS_socketError;
			delay = 10000;
			continue;
		}

		NLaddress tournamentServer;
		if (!nlGetAddrFromName("www.mycgiserver.com", &tournamentServer))
			nlStringToAddr("69.57.148.55", &tournamentServer);

		nlSetAddrPort(&tournamentServer, 80);
		nlConnect(sock, &tournamentServer);

		const string query =
			string() +
			"GET /servlet/fcecin.tk1/index.html?" + url_encode(TK1_VERSION_STRING) +
			'&' + (newToken?"new":"old") +
			"&name=" + url_encode(name) +
			"&password=" + url_encode(password) +
			" HTTP/1.0\r\n\r\n";
		passStatus = PS_sending;
		if (newToken)
			log("Password thread: Sending login");
		NetworkResult result = writeToUnblockingTCP(sock, query.data(), query.length(), &quitThread, 30000);
		if (result != NR_ok) {
			nlClose(sock);
			if (quitThread)
				break;
			log("Password thread: Error sending login: %s", result == NR_timeout ? "Timeout" : getNlErrorString());
			passStatus = PS_sendError;
			continue;
		}

		passStatus = PS_receiving;
		string response;
		{
			ostringstream respStream;
			const NetworkResult result = saveAllFromUnblockingTCP(sock, respStream, &quitThread, 30000);
			nlClose(sock);
			if (result != NR_ok) {
				if (quitThread)
					break;
				log("Password thread: Error receiving response: %s", result == NR_timeout ? "Timeout" : getNlErrorString());
				passStatus = PS_recvError;
				continue;
			}
			string fullResponse = respStream.str();

			// find the start and end of the body: after the last "<html>" and before the last "</html>"
			// the original code uses full case insensivity so response.find_last_of() can't be used
			int startPos, endPos;
			for (startPos = fullResponse.length() - 7; startPos >= 6; --startPos)	// start at length - 7 because "</html>" must fit after that
				if (!stricmp(fullResponse.substr(startPos - 6, 6).c_str(), "<html>"))
					break;
			for (endPos = fullResponse.length() - 7; endPos >= startPos; --endPos)
				if (!stricmp(fullResponse.substr(endPos, 7).c_str(), "</html>"))
					break;
			if (startPos < 6 || endPos < startPos) {
				log("Password thread: Invalid response (no <html>...</html>): \"%s\"", formatForLogging(fullResponse).c_str());
				passStatus = PS_invalidResponse;
				continue;
			}
			response = fullResponse.substr(startPos, endPos - startPos);
		}

		// parse the response
		for (string::size_type i = 0; i < response.length(); ++i) {
			if (response[i] < 32)
				response[i] = '+';	// for readability in the log
			if (!stricmp(response.substr(i, 22).c_str(), "contact servlet runner")) {
				log("Password thread: Service unavailable: \"%s\"", formatForLogging(response).c_str());
				passStatus = PS_unavailable;
				break;
			}
		}
		if (passStatus == PS_unavailable)
			continue;
		log("Password thread: Received response: \"%s\"", formatForLogging(response).c_str());
		string::size_type cPos = response.find_first_of('@');
		if (cPos == string::npos || cPos + 1 >= response.length() || response.find_first_of('@', cPos + 1) != string::npos) {
			log("Password thread: Invalid response (expecting one @-code)");
			passStatus = PS_invalidResponse;
			continue;
		}
		++cPos;	// point to the control character after @
		if (response[cPos] == 'K' && cPos + 1 < response.length()) {	// login ok; token follows
			++cPos;
			const string::size_type tokEnd = response.find_first_of('#', cPos);
			if (tokEnd == string::npos || tokEnd - cPos > 15 || tokEnd == cPos) {
				log("Password thread: Invalid response (invalid token)");
				passStatus = PS_invalidResponse;
			}
			else {
				log("Password thread: Login OK");
				passStatus = PS_tokenReceived;
				setToken(response.substr(cPos, tokEnd - cPos));
				delay = 10*60000;	// refresh token after 10 minutes
			}
		}
		else if (response[cPos] == 'E' || response[cPos] == 'F') {
			log("Password thread: Login failed (wrong name or password)");
			passStatus = PS_badLogin;
			setToken("");
			delay = 10*60000;	// try again after 10 minutes (will probably fail then too)
		}
		else {
			log("Password thread: Invalid response (bad @-code)");
			passStatus = PS_invalidResponse;
		}
	}

	if (LOG_THREAD_IDS)
		log("exiting: TournamentPasswordManager::threadFn() ID = %d, prio = %d", pthread_self(), threadPriority());
}

bool ServerListEntry::setAddress(const string& address) {
	if (!nlStringToAddr(address.c_str(), &addr))
		return false;
	if (nlGetPortFromAddr(&addr) == 0)
		nlSetAddrPort(&addr, DEFAULT_UDP_PORT);
	return true;
}

string ServerListEntry::addressString() const {
	if (nlGetPortFromAddr(&addr) != DEFAULT_UDP_PORT)
		return addressToString(addr);
	else {
		NLaddress cpy = addr;
		nlSetAddrPort(&cpy, 0);
		return addressToString(cpy);
	}
}

FileDownload::FileDownload(const string& type, const string& name, const string& filename) :
	fileType(type),
	shortName(name),
	fullName(filename),
	fp(0)
{ }

FileDownload::~FileDownload() {
	if (fp) {
		fclose(fp);
		remove(fullName.c_str());
	}
}

int FileDownload::progress() const {
	nAssert(fp);
	return ftell(fp);
}

bool FileDownload::start() {
	nAssert(!fp);
	fp = fopen(fullName.c_str(), "wb");
	return (fp != 0);
}

bool FileDownload::save(const char* buf, unsigned len) {
	nAssert(fp);
	return (fwrite(buf, sizeof(char), len, fp) == len);
}

void FileDownload::finish() {
	nAssert(fp);
	fclose(fp);
	fp = 0;
}

void TM_ServerSettings::execute(Client* cl) const {
	cl->m_serverInfo.menu.setCaption(cl->hostname);

	ostringstream caption;
	ostringstream value;
	const int width = 22;

	caption << setw(width) << "Capture limit: ";
	if (caplimit == 0)
		value << "none";
	else
		value << static_cast<int>(caplimit);
	cl->m_serverInfo.addLine(caption.str(), value.str());
	caption.str(""); value.str("");

	caption << setw(width) << "Time limit: ";
	if (timelimit == 0)
		value << "none";
	else
		value << static_cast<int>(timelimit) << " min";
	cl->m_serverInfo.addLine(caption.str(), value.str());
	caption.str(""); value.str("");

	caption << setw(width) << "Extra time: ";
	if (extratime == 0)
		value << "none";
	else
		value << static_cast<int>(extratime) << " min";
	cl->m_serverInfo.addLine(caption.str(), value.str());
	caption.str(""); value.str("");

	caption << setw(width) << "Player collisions: ";
	value << (cl->fx.physics.player_collisions ? "on" : "off");
	cl->m_serverInfo.addLine(caption.str(), value.str());
	caption.str(""); value.str("");

	caption << setw(width) << "Friendly fire: ";
	value << (cl->fx.physics.friendly_fire ? "on" : "off");
	cl->m_serverInfo.addLine(caption.str(), value.str());
	caption.str(""); value.str("");

	const string caps[] = { "Balance teams", "Drop power-ups", "Invisible shadow", "Switch deathbringer" };
	int i;
	for (i = 0; i < 4; i++) {
		caption << setw(width - 2) << caps[i] << ": ";
		value << ((misc1 & (1 << i)) ? "on" : "off");
		cl->m_serverInfo.addLine(caption.str(), value.str());
		caption.str(""); value.str("");
	}
	caption << setw(width) << "Maximum weapon level: ";
	value << (misc1 >> i) + 1;
	cl->m_serverInfo.addLine(caption.str(), value.str());

	if (cl->menu.options.game.showServerInfo())
		cl->showMenu(cl->m_serverInfo);
}

TM_ConnectionUpdate::TM_ConnectionUpdate(int code_, const void* data_, int length_) :
	code(code_),
	length(length_)
{
	if (length) {
		data = new char[length];
		memcpy(data, data_, length);
	}
	else
		data = 0;
}

void TM_ConnectionUpdate::execute(Client* cl) const {
	switch (code) {
		case 0:
			cl->client_connected(data, length);
			break;
		case 1:
			cl->client_disconnected(data, length);
			break;
		case 2:
			cl->connect_failed_denied(data, length);
			break;
		case 3:
			cl->connect_failed_unreachable();
			break;
		case 4: {
			const string msg = "Server is full.";
			cl->connect_failed_denied(msg.data(), msg.length());
			break;
		}
		default:
			nAssert(0);
	}
}

Client::Client(LogSet hostLogs, const ClientExternalSettings& config, const ServerExternalSettings& serverConfig):
	normalLog(wheregamedir + "log" + directory_separator + "clientlog.txt", true),
	errorLog(normalLog, "ERROR: "),
	//securityLog(normalLog, "SECURITY WARNING: ", wheregamedir + "log" + directory_separator + "client_securitylog.txt", false),
	log(&normalLog, &errorLog, 0),
	listenServer(log),
	tournamentPassword(log, new RedirectToMemFun1<Client, void, string>(this, &Client::CB_tournamentToken), config.lowerPriority),
	current_map(-1),
	map_vote(-1),
	player_stats_page(0),
	abortThreads(false),
	refreshStatus(RS_none),
	password_file(wheregamedir + "config" + directory_separator + "passwd"),
	client_graphics(log),
	screenshot(false),
	mapChanged(false),
	predrawNeeded(false),
	client_sounds(log),
	extConfig(config),
	serverExtConfig(serverConfig)
{
	hostLogs("See clientlog.txt for client's log messages");

	//net client
	client = 0;

	setMaxPlayers(MAX_PLAYERS);
	//all the players to show including me
	//player_t player[MAX_PLAYERS];
	for (int p = 0; p < MAX_PLAYERS; p++)
		fx.player[p].used = false;

	//wich player I am
	me = -1;

	//time of last packet received
	lastpackettime = 0;

	initMenus();
	showMenu(menu);
	menusel = menu_none;

	//game showing?
	gameshow = false;

	//frames and seconds for FPS counter
	FPS = 0;
	framecount = 0;
	totalframecount = 0;
	frameCountStartTime = 0;

	//if player wants to changeteams
	want_change_teams = false;

	//connected? (that is, "connection accepted")
	connected = false;

	Thread::setCallerPriority(config.priority);
}

Client::~Client() {
	log("Exiting client: destructor");

	abortThreads = true;
	if (client) {
		delete client;
		client = 0;
	}
	while (refreshStatus != RS_none && refreshStatus != RS_failed)	// wait for a possible refresh thread to abort itself
		MS_SLEEP(50);

	for (deque<ThreadMessage*>::const_iterator mi = messageQueue.begin(); mi != messageQueue.end(); ++mi)
		delete *mi;

	errorMessage("Client had these errors: (see clientlog.txt)", errorLog);

	log("Exiting client: destructor exiting");
}

bool Client::start() {
	extConfig.statusOutput("Outgun client");

	totalframecount = 0;
	framecount = 0;

	clFrameSent = clFrameWorld = 0;
	frameReceiveTime = 0;

	#ifdef SEND_FRAMEOFFSET
	frameOffsetDeltaTotal = 0;
	frameOffsetDeltaNum = 0;
	#endif
	averageLag = 0;

	// default map
	//load_default_map(&map);
	map_ready = false;		// NO map change commands from server yet

	//not showing gameover plaque
	gameover_plaque = NEXTMAP_NONE;

	connected = false;

	client = new_client_c(extConfig.priority);
	client->set_callback(CFUNC_CONNECTION_UPDATE, cfunc_connection_update);
	client->set_callback(CFUNC_SERVER_DATA, cfunc_server_data);

	//try to load the client's password
	string fileName = wheregamedir + "config" + directory_separator + "password.bin";
	FILE *psf = fopen(fileName.c_str(), "rb");
	if (psf) {
		char pas[PASSBUFFER];
		for (int c = 0; c < PASSBUFFER; c++) {
			const int cha = fgetc(psf);	// don't care about EOF yet
			pas[c] = static_cast<char>(255 - cha);
		}
		if (!feof(psf)) {
			pas[8] = 0;
			menu.options.name.password.set(pas);
		}
		fclose(psf);
	}

	//try to load client configuration
	vector<int> fav_colors;
	playername = RandomName();

	fileName = wheregamedir + "config" + directory_separator + "client.cfg";
	ifstream cfg(fileName.c_str());
	for (;;) {
		string line;
		if (!getline_skip_comments(cfg, line))
			break;

		istringstream command(line);

		int settingId;
		string args;
		command >> settingId;
		command.ignore();	// eat separator (space)
		getline(command, args);
		if (!command || settingId < 0) {
			log.error("Invalid syntax in client.cfg (\"%s\")", line.c_str());
			continue;
		}
		if (settingId > CCS_MaxCommand) {
			log.error("Unknown data in client.cfg (\"%s\")", line.c_str());
			continue;
		}
		switch (static_cast<ClientCfgSetting>(settingId)) {
			// name menu
			case CCS_PlayerName:			if (check_name(args)) playername = args; break;
			case CCS_Tournament:			menu.options.name.tournament.set(args == "1"); break;

			// connect menu
			case CCS_Favorites:				menu.connect.favorites.set(args == "1"); break;

			// game menu
			case CCS_ShowNames:				menu.options.game.showNames.set(args == "1"); break;
			case CCS_FavoriteColors: {
				istringstream ist(args);
				int col;
				while (ist >> col)
					if (col >= 0 && col < 16 && find(fav_colors.begin(), fav_colors.end(), col) == fav_colors.end())
						fav_colors.push_back(col);
				break;
			}
			case CCS_LagPrediction:			menu.options.game.lagPrediction.set(args == "1"); break;
			case CCS_LagPredictionAmount:	menu.options.game.lagPredictionAmount.boundSet(atoi(args)); break;
			case CCS_MessageLogging:		menu.options.game.messageLogging.set(args == "1"); break;
			case CCS_SaveStats:				menu.options.game.saveStats.set(args == "1"); break;
			case CCS_ShowStats:				menu.options.game.showStats.set(args == "1"); break;
			case CCS_AutoGetServerList:		menu.options.game.autoGetServerList.set(args == "1"); break;
			case CCS_ShowServerInfo:		menu.options.game.showServerInfo.set(args == "1"); break;

			// controls menu
			case CCS_KeypadMoving:			menu.options.controls.keypadMoving.set(args == "1"); break;
			case CCS_Joystick:				menu.options.controls.joystick.set(args == "1"); break;
			case CCS_JoystickMove:			menu.options.controls.joyMove.boundSet(atoi(args)); break;
			case CCS_JoystickShoot:			menu.options.controls.joyShoot.boundSet(atoi(args)); break;
			case CCS_JoystickRun:			menu.options.controls.joyRun.boundSet(atoi(args)); break;
			case CCS_JoystickStrafe:		menu.options.controls.joyStrafe.boundSet(atoi(args)); break;

			// graphics menu
			case CCS_Windowed:				menu.options.graphics.windowed.set(args == "1"); break;
			case CCS_GFXMode: {
				istringstream is(args);
				int width, height, depth;
				is >> width >> height >> depth;
				bool ok = is;
				char nullc;
				is >> nullc;
				if (!ok || is || width < 640 || height < 400 || (depth != 16 && depth != 24 && depth != 32))
					log("Bad screen mode in client.cfg");
				else {
					nAssert(menu.options.graphics.colorDepth.set(depth));
					menu.options.graphics.update(client_graphics);	// fetch resolutions according to the new depth
					if (!menu.options.graphics.resolution.set(ScreenMode(width, height)))
						log("Previous screen mode not available (%d×%d×%d)", width, height, depth);
				}
				break;
			}
			case CCS_Flipping:				menu.options.graphics.flipping.set(args == "1"); break;
			case CCS_FPSLimit:				menu.options.graphics.fpsLimit.boundSet(atoi(args)); break;
			case CCS_GFXTheme:				menu.options.graphics.theme.set(args); break;	// ignore error
			case CCS_Antialiasing: {
				int modei = atoi(args);
				if (modei < 0 || modei > 2)
					modei = 0;
				Graphics::Antialiasing_mode mode = static_cast<Graphics::Antialiasing_mode>(modei);
				menu.options.graphics.antialiasing.set(mode);
				break;
			}
			case CCS_StatsBgAlpha:			menu.options.graphics.statsBgAlpha.boundSet(atoi(args)); break;

			// sound menu
			case CCS_SoundEnabled:			menu.options.sounds.enabled.set(args == "1"); break;
			case CCS_Volume:				menu.options.sounds.volume.boundSet(atoi(args)); break;
			case CCS_SoundTheme:			menu.options.sounds.theme.set(args); break;	// ignore error

			default:	nAssert(0);	// must handle all values up to the highest known
		}
	}
	cfg.close();

	fileName = wheregamedir + "config" + directory_separator + "favorites.txt";
	ifstream fav(fileName.c_str());
	if (fav) {
		string addr;
		while (getline_skip_comments(fav, addr)) {
			ServerListEntry spy;
			if (spy.setAddress(addr))
				gamespy.push_back(spy);
		}
		fav.close();
	}

	// finalize and apply the settings

	// name
	tournamentPassword.changeData(playername, menu.options.name.password());

	// game
	if (menu.options.game.messageLogging())
		message_log.open((wheregamedir + "log" + directory_separator + "message.log").c_str(), ios::app);
	for (int i = 0; i < 16; i++)
		if (find(fav_colors.begin(), fav_colors.end(), i) == fav_colors.end())
			fav_colors.push_back(i);
	for (vector<int>::const_iterator col = fav_colors.begin(); col != fav_colors.end(); ++col)
		menu.options.game.favoriteColors.addOption(*col);

	// controls
	if (menu.options.controls.joystick())
		install_joystick(JOY_TYPE_AUTODETECT);

	// graphics
	if (extConfig.winclient != -1)
		menu.options.graphics.windowed.set(extConfig.winclient);
	if (extConfig.trypageflip != -1)
		menu.options.graphics.flipping.set(extConfig.trypageflip);
	if (extConfig.targetfps != -1)
		menu.options.graphics.fpsLimit.set(extConfig.targetfps);
	client_graphics.set_antialiasing(menu.options.graphics.antialiasing());
	MCF_statsBgChange();
	client_graphics.select_theme(menu.options.graphics.theme());
	if (!screenModeChange())
		return false;

	// sounds
	if (extConfig.nosound)
		menu.options.sounds.enabled.set(false);
	MCF_sndEnableChange();
	client_sounds.setVolume(menu.options.sounds.volume());
	client_sounds.select_theme(menu.options.sounds.theme());

	if (menu.options.game.autoGetServerList())
		MCF_updateServers();

	return true;
}

//send "client ready" message to server (when map load and/or download completes)
void Client::send_client_ready() {
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, data_client_ready);
	client->send_message(lebuf, count);		// bem curtinha a mensagem mesmo...
}

// incoming chunk of requested file by UDP
void Client::process_udp_download_chunk(const char* buf, int len, bool last) {
	MutexLock ml(downloadMutex);
	if (downloads.empty() || !downloads.front().isActive()) {
		log.error("Server sent a file we aren't expecting");
		addThreadMessage(new TM_DoDisconnect());
		return;
	}
	FileDownload& dl = downloads.front();
	if (!dl.save(buf, len)) {
		log.error("Error writing to %s.", dl.fullName.c_str());
		addThreadMessage(new TM_DoDisconnect());
		return;
	}
	//send the reply
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, data_file_ack);
	client->send_message(lebuf, count);
	if (last) {
		dl.finish();
		log("Download complete: %s '%s' to %s", dl.fileType.c_str(), dl.shortName.c_str(), dl.fullName.c_str());
		if (dl.fileType == "map") {
			if (dl.shortName == servermap) {
				const bool ok = fd.load_map(log, CLIENT_MAPS_DIR, dl.shortName) && fx.load_map(log, CLIENT_MAPS_DIR, dl.shortName);	//#fix
				if (!ok) {
					log.error("After download: map '%s' not found", dl.shortName.c_str());
					addThreadMessage(new TM_DoDisconnect());
					return;
				}
				log("Map '%s' downloaded successfully", dl.shortName.c_str());
				mapChanged = true;
				map_ready = true;
				send_client_ready();
			}
		}
		else
			nAssert(0);
		downloads.pop_front();
		check_download();
	}
}

/* check_download: if there is a download pending, and nothing is downloading, activate it
 * call with downloadMutex locked
 */
void Client::check_download() {	// call with downloadMutex locked
	if (downloads.empty())
		return;
	FileDownload& dl = downloads.front();
	if (dl.isActive())
		return;
	if (!dl.start()) {
		log.error("File download: Can't open '%s' for writing.", dl.fullName.c_str());
		addThreadMessage(new TM_DoDisconnect());
		return;
	}
	// request the file from server
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, data_file_request);
	writeStr(lebuf, count, dl.fileType);
	writeStr(lebuf, count, dl.shortName);
	client->send_message(lebuf, count);
}

void Client::download_server_file(const string& type, const string& name) {
	nAssert(type == "map");
	if (name.find_first_of("./:\\") != string::npos) {
		log.error("Illegal file download request: map \"%s\"", name.c_str());
		addThreadMessage(new TM_DoDisconnect());
		return;
	}

	MutexLock ml(downloadMutex);
	const string fileName = wheregamedir + CLIENT_MAPS_DIR + directory_separator + name + ".txt";
	downloads.push_back(FileDownload(type, name, fileName));
	check_download();
}

//server tells client of current map / map change
// client must attempt to load map from "cmaps" dir
// if map file not there, or the CRC's don't match, ask to download the map from the server
void Client::server_map_command(const string& mapname, NLushort server_crc) {
	log("Received map change: '%s'", mapname.c_str());

	servermap = mapname;

	// try to load the map, in case it's already downloaded
	LogSet noLogSet(0, 0, 0);	// if there's an error with the map, don't log it
	const bool ok = fd.load_map(noLogSet, CLIENT_MAPS_DIR, mapname) && fx.load_map(noLogSet, CLIENT_MAPS_DIR, mapname);	//#fix

	if (!ok)
		log("Map '%s' not found", mapname.c_str());
	else if (fx.map.crc != server_crc)
		log("Map '%s' found but it's CRC %i differs from server map CRC %i", mapname.c_str(), fx.map.crc, server_crc);
	else {
		log("Map '%s' loaded successfully", mapname.c_str());
		mapChanged = true;
		map_ready = true;
		send_client_ready();
		return;
	}

	// start download
	ostringstream msg;
	msg << "Downloading map '" << mapname << "' (CRC " << server_crc << ")...";
	print_message(msg_info, msg.str());
	log("%s", msg.str().c_str());

	download_server_file("map", mapname);
}

void Client::disconnect_command() {	// do not call from a network thread
	//disconnect the client here if was connected, else does nothing
	client->connect(false);
}

void Client::client_connected(const char* data, int length) {	// call with frameMutex locked
	log("Connection successful");

	(void)length;

	//"data" from connection accepted:
	//  BYTE		maxplayers
	//  STRING	hostname
	int count = 0;
	NLubyte maxpl;
	readByte(data, count, maxpl);
	setMaxPlayers(maxpl);

	readStr(data, count, hostname);
	m_serverInfo.clear();

	if (!menu.options.game.favoriteColors.values().empty()) {
		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, data_fav_colors);
		writeByte(lebuf, count, menu.options.game.favoriteColors.values().size());
		// send two colours in a byte
		for (vector<int>::const_iterator col = menu.options.game.favoriteColors.values().begin();
					col != menu.options.game.favoriteColors.values().end(); ++col) {
			NLubyte byte = static_cast<NLubyte>(*col) & 0x0F;
			if (++col != menu.options.game.favoriteColors.values().end())
				byte |= static_cast<NLubyte>(*col) << 4;
			writeByte(lebuf, count, byte);
		}
		client->send_message(lebuf, count);
	}

	show_all_messages = false;
	stats_autoshowing = false;

	//set window title: the hostname
	ostringstream caption;
	caption << "Connected to: " << hostname.substr(0, 32) << " (" << addressToString(serverIP) << ')';
	extConfig.statusOutput(caption.str());

	//don't want to change teams by default
	want_change_teams = false;

	//don't want to exit map by default
	want_map_exit = false;

	//avoid "dropped" plaque
	lastpackettime = get_time() + 1.0;

	averageLag = 0;

	clFrameSent = clFrameWorld = 0;
	frameReceiveTime = 0;

	// reset gamestate?
	connected = true;
	gameshow = true;
	nAssert(openMenus.safeTop() == &m_connectProgress.menu);
	openMenus.clear();
	fx.frame = fd.frame = 0;
	fx.skipped = fd.skipped = true;
	me = -1;	//don't know who am I

	//reset chat buffer
	talkbuffer.clear();
	chatbuffer.clear();

	//reset world data
	// teams
	for (int i = 0; i < 2; i++) {
		fx.teams[i].clear_stats();
		fx.teams[i].remove_flags();
	}
	// players
	for (int i = 0; i < MAX_PLAYERS; i++)
		fx.player[i].clear(false, i, "(name unknown)", i / TSIZE);
	players_sb.clear();
	// powerups
	for (int i = 0; i < MAX_PICKUPS; ++i)
		fx.item[i].kind = Powerup::pup_unused;

	//reset FPS count vars
	framecount = 0;
	frameCountStartTime = get_time();
	FPS = 0;

	//reset map time
	map_time_limit = false;
	map_start_time = 0;
	map_end_time = 0;
	map_vote = -1;

	//send name update request
	issue_change_name_command();
	// send registration token (if any)
	const string s = tournamentPassword.getToken();
	if (!s.empty())
		CB_tournamentToken(s);
	send_tournament_participation();

	map_ready = false;
	servermap.clear();

	mapInfoMutex.lock();
	maps.clear();
	mapInfoMutex.unlock();

	//not showing gameover plaque
	gameover_plaque = NEXTMAP_NONE;

	//clear client side effects
	client_graphics.clear_fx();

	send_frame(true);
}

void Client::send_tournament_participation() {
	char lebuf[8]; int count = 0;
	writeByte(lebuf, count, data_tournament_participation);
	writeByte(lebuf, count, menu.options.name.tournament() ? 1 : 0);
	client->send_message(lebuf, count);
}

void Client::client_disconnected(const char* data, int length) {
	//restore window title
	extConfig.statusOutput("Outgun client");

	// the gamestate?
	connected = false;
	gameshow = false;

	string description;

	if (length == 1)
		switch (data[0]) {
			case server_c::disconnect_client_initiated: break;
			case server_c::disconnect_server_shutdown:	description = "Server was shut down."; break;
			case server_c::disconnect_timeout: 			description = "Connection timed out."; break;
			case disconnect_kick:						description = "You were kicked."; break;
			case disconnect_idlekick:					description = "You were kicked for being idle."; break;
			case disconnect_client_misbehavior:			description = "Internal error (client misbehaved)."; break;
			default:	break;
		}
	m_connectProgress.clear();
	m_connectProgress.addLine("You have been disconnected.");
	if (!description.empty())
		m_connectProgress.addLine(description);
	showMenu(m_connectProgress);
	if (description.empty())
		log("Disconnection successful");
	else
		log("Disconnected: %s", description.c_str());

	tournamentPassword.disconnectedFromServer();
}

void Client::connect_failed_denied(const char* data, int length) {
	string message;
	if (length > 0) {
		int count = 0;
		readStr(data, count, message);
	}
	else
		message = "no reason given.";

	log("Connecting failed: %s", message.c_str());

	if (message == "SERVER PASSWORD") {
		if (openMenus.safeTop() == &m_connectProgress.menu)
			openMenus.close();
		showMenu(m_serverPassword);
	}
	else if (message == "PLAYER PASSWORD") {
		nAssert(openMenus.safeTop() == &m_connectProgress.menu);
		openMenus.close();
		m_playerPassword.setup(playername, false);
		showMenu(m_playerPassword);
	}
	else {
		nAssert(openMenus.safeTop() == &m_connectProgress.menu);
		m_connectProgress.wrapLine(message);
		if (message == "Wrong player password")
			remove_player_password(playername, addressToString(serverIP));
	}
}

void Client::connect_failed_unreachable() {
	nAssert(openMenus.safeTop() == &m_connectProgress.menu);
	m_connectProgress.addLine("No response from server.");
	log("Connecting failed: no response");
}

string Client::load_player_password(const string& name, const string& address) const {
	ifstream in(password_file.c_str());
	while (in) {
		string load_name, load_address, load_password;
		getline_smart(in, load_name);
		getline_smart(in, load_address);
		getline_smart(in, load_password);
		if (load_name == name && load_address == address)
			return load_password;
	}
	return string();
}

vector<vector<string> > Client::load_all_player_passwords() const {
	vector<vector<string> > passwords;
	ifstream in(password_file.c_str());
	while (1) {
		string name, address, password;
		getline_smart(in, name);
		getline_smart(in, address);
		getline_smart(in, password);
		if (in) {
			vector<string> entry;
			entry.push_back(name);
			entry.push_back(address);
			entry.push_back(password);
			passwords.push_back(entry);
		}
		else
			break;
	}
	return passwords;
}

void Client::save_player_password(const string& name, const string& address, const string& password) const {
	nAssert(!name.empty() && !address.empty() && !password.empty());	// empty lines cause trouble
	vector<vector<string> > passwd_list = load_all_player_passwords();
	// check if player already has a password
	string test = load_player_password(name, address);
	if (test.empty()) {
		vector<string> entry;
		entry.push_back(name);
		entry.push_back(address);
		entry.push_back(password);
		passwd_list.push_back(entry);
	}
	ofstream out(password_file.c_str());
	if (!out) {
		log.error("Can't save player password to %s!", password_file.c_str());
		return;
	}
	for (vector<vector<string> >::const_iterator item = passwd_list.begin(); item != passwd_list.end(); ++item) {
		out << (*item)[0] << '\n';
		out << (*item)[1] << '\n';
		if ((*item)[0] == name && (*item)[1] == address) {
			out << password;
			log("New player password saved for %s at %s.", name.c_str(), address.c_str());
		}
		else
			out << (*item)[2];
		out << '\n';
	}
}

void Client::remove_player_password(const string& name, const string& address) const {
	// check if player has a password
	const string test = load_player_password(name, address);
	if (test.empty())
		return;
	const vector<vector<string> > passwd_list = load_all_player_passwords();
	ofstream out(password_file.c_str());
	if (!out)
		return;
	for (vector<vector<string> >::const_iterator item = passwd_list.begin(); item != passwd_list.end(); ++item)
		if ((*item)[0] == name && (*item)[1] == address)
			log("%s's player password at %s removed.", name.c_str(), address.c_str());
		else
			for (int i = 0; i < 3; i++)
				out << (*item)[i] << '\n';
}

int Client::remove_player_passwords(const std::string& name) const {
	vector<vector<string> > passwd_list = load_all_player_passwords();
	ofstream out(password_file.c_str());
	if (!out)
		return 0;
	int removed = 0;
	for (vector<vector<string> >::iterator item = passwd_list.begin(); item != passwd_list.end(); ++item)
		if ((*item)[0] == name)
			removed++;
		else
			for (int i = 0; i < 3; i++)
				out << (*item)[i] << '\n';
	log("%s's player passwords removed.", name.c_str());
	return removed;
}

//connect command
void Client::connect_command(bool loadPassword) {
	// disconnect
	client->connect(false);

	// start connecting to specified IP/port
	// connection results will come through the CFUNC_CONNECTION_UPDATE callback
	const string strAddress = addressToString(serverIP);
	client->set_server_address(strAddress.c_str());

	log("Connecting to %s... passwords: server %s, player %s", strAddress.c_str(), m_serverPassword.password().empty()?"no":"yes", m_playerPassword.password().empty()?"no":"yes");

	//set connect-data (goes in every connect packet): outgun game name and protocol strings
	char lebuf[256]; int count = 0;
	writeString(lebuf, count, GAME_STRING);
	writeString(lebuf, count, GAME_PROTOCOL);
	writeStr(lebuf, count, playername);
	if (!m_serverPassword.password().empty())
		writeStr(lebuf, count, m_serverPassword.password());
	if (loadPassword)
		m_playerPassword.password.set(load_player_password(playername, strAddress)); 
	if (!m_playerPassword.password().empty())
		writeStr(lebuf, count, m_playerPassword.password());

	client->set_connect_data(lebuf, count);

	client->connect(true);

	m_connectProgress.clear();
	m_connectProgress.addLine("Trying to connect...", true);
	if (openMenus.safeTop() != &m_connectProgress.menu)
		showMenu(m_connectProgress);
}

void Client::issue_change_name_command() {
	if (!connected)
		return;
	//regular change name
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, data_name_update);
	nAssert(check_name(playername));
	writeStr(lebuf, count, playername);	// the name
	writeStr(lebuf, count, m_playerPassword.password());	// empty or not, it's needed
	client->send_message(lebuf, count);
}

void Client::change_name_command() {
	//set new name, close menu
	menu.options.name.name.set(trim(menu.options.name.name()));
	const string& newName = menu.options.name.name();
	if (!check_name(newName))
		return;
	if (openMenus.safeTop() == &menu.options.name.menu)
		openMenus.close();

	playername = newName;
	m_playerPassword.password.set(load_player_password(playername, addressToString(serverIP)));
	issue_change_name_command();
	tournamentPassword.changeData(playername, menu.options.name.password());
}

//send the client's frame to server (keypresses)
void Client::send_frame(bool newFrame) {
	char lebuf[256]; int count = 0;

	ClientControls currCtrl;
	if (menusel == menu_none && openMenus.empty()) {	// don't move when menu or similar is open
		currCtrl.fromKeyboard(menu.options.controls.keypadMoving());
		if (menu.options.controls.joystick())
			currCtrl.fromJoystick(menu.options.controls.joyMove() - 1, menu.options.controls.joyRun() - 1, menu.options.controls.joyStrafe() - 1);
	}

	if (newFrame) {
		++clFrameSent;
		controlHistory[clFrameSent] = currCtrl;
		svFrameHistory[clFrameSent] = static_cast<NLulong>(fx.frame);
	}

	writeByte(lebuf, count, clFrameSent);
	writeByte(lebuf, count, currCtrl.toNetwork(false));

	client->send_frame(lebuf, count);
}

void Client::process_incoming_data(const char* data, int length) {
	MutexLock ml(frameMutex);

	(void)length;

	//this is a HACK:
	int whatme = 0;	//#fix: can this be removed?

	// (0) update lastpackettime
	lastpackettime = get_time();

	//(1) process server frame data
	int count = 0;
	NLulong svframe;	//server's frame
	readLong(data, count, svframe);

	if (WATCH_CONNECTION && svframe != fx.frame + 1) {
		ostringstream dstr;
		if (svframe == fx.frame)
			dstr << "S>C packet duplicated: " << svframe;
		else if (svframe < fx.frame)
			dstr << "S>C packet order: prev " << fx.frame << " this " << svframe;
		else
			dstr << "S>C packet lost : prev " << fx.frame << " this " << svframe;
		addThreadMessage(new TM_Text(msg_warning, dstr.str().c_str()));
	}
	//discard older frames
	//overwrite always the newer frames
	// TARGET FRAME: just one
	if (svframe > fx.frame) {
		nAssert(fx.frame == (int)fx.frame);

		ClientPhysicsCallbacks cb(*this);
		fx.rocketFrameAdvance(static_cast<int>(svframe - fx.frame), cb);
		fx.frame = svframe;

		NLulong	players_present;		//LONG players present (32 players max)
		readLong(data, count, players_present);
		for (int i = 0; i < maxplayers; i++) {
			//decode players_present: sets if "player" record is used or not, in clientside
			if (players_present & (1 << i)) {
				if (!fx.player[i].used)
					players_sb.push_back(&fx.player[i]);
				fx.player[i].used = true;
			}
			else {
				if (fx.player[i].used) {
					vector<ClientPlayer*>::iterator rm = find(players_sb.begin(), players_sb.end(), &fx.player[i]);
					if (rm != players_sb.end())
						players_sb.erase(rm);
				}
				fx.player[i].used = false;
			}
		}

		//----- PLAYER SPECIFIC DATA -----

		readByte(data, count, clFrameWorld);
		frameReceiveTime = get_time();
		const int currentLag = bound(svframe - svFrameHistory[clFrameWorld], 0ul, 50ul);	// bound because svFrameHistory has invalid frame# at connect to server
		averageLag = averageLag*.99 + currentLag*.01;

		#ifdef SEND_FRAMEOFFSET
		NLubyte fo;
		readByte(data, count, fo);
		const float offsetDelta = (fo / 256.) - .5;	// the deviation from aim, in frames
		frameOffsetDeltaTotal += offsetDelta;
		if (++frameOffsetDeltaNum == 10) {	// try to fix deviations every 10 frames
			frameOffsetDeltaTotal /= 10.;
			netsendAdjustment = -static_cast<int>(frameOffsetDeltaTotal * 20.);	// the time_counter used for timing netsends is 200 Hz so one frame is 20 ticks
			frameOffsetDeltaTotal = 0;
			frameOffsetDeltaNum = 0;
		}
		#endif

		//extra byte of information
		// BIT 0: extra health
		// BIT 1: extra energy
		// BIT 2 (****VERY IMPORTANT****): NO MORE DATA ON PACKET BECAUSE PLAYER IS NOT READY
		NLubyte xtra;
		readByte(data, count, xtra);

		//moved below: after health assignment
		//if (xtra & 1) player[me].health += 256;
		//if (xtra & 2) player[me].energy += 256;

		const bool empty_frame_cause_not_ready_yet = ((xtra & 4) != 0);

		//read "me" (v0.3.9 tentando espantar bug com tiro de canhao!)
		// BITS 3..8 == what player id
		whatme = xtra >> 3;

		// v0.4.1: ISSO AQUI TAVA FALTANDO. como que tu vai ler um monte de coisa
		//	dependente do "me" sem ter ele definido??????
		//
		me = whatme;

		//EMPTY FRAME? if yes, do something about it, if not, parse it
		if (empty_frame_cause_not_ready_yet)
			fx.skipped = true;
		else {
			//a regular frame
			fx.skipped = false;

			//V 0.3.9 NEW : read screen of "me" player
			NLubyte scr;
			readByte(data, count, scr);		//player.x
			fx.player[me].roomx = scr;
			readByte(data, count, scr);		//player.y
			fx.player[me].roomy = scr;

			//read "players onscreen" vector
			NLulong	players_onscreen;
			readLong(data, count, players_onscreen);

			//decode players_onscreen and update player data
			for (int i = 0; i < maxplayers; i++) {
				//decode players_onscreen: sets if "player" record is there to be read
				if (players_onscreen & (1 << i))
					fx.player[i].onscreen = true;
				else {
					fx.player[i].onscreen = false;
					continue;
				}

				ClientPlayer &h = fx.player[i];

				//V0.3.9: took out screen reading, replacing for the same screen of "me"
				// that is set above
				h.roomx = fx.player[me].roomx;	//same screen since it's on the "players on same screen" vector
				h.roomy = fx.player[me].roomy;

				//coords & speeds
				NLubyte xy;

				NLushort hx, hy;
				readByte(data, count, xy);		//first 8 bits x
				hx = xy;
				readByte(data, count, xy);
				hy = xy;
				readByte(data, count, xy);
				hx += (xy & 0x0F) << 8;
				hy += (xy & 0xF0) << 4;
				h.lx = hx * (plw / float(0xFFF));
				h.ly = hy * (plh / float(0xFFF));

				//V0.3.9 speed em bytes, xinelao mesmo
				NLbyte sxy;
				readByte(data, count, sxy);	//sx
				h.sx = static_cast<double>(sxy) / 2.0;
				readByte(data, count, sxy);	//sy
				h.sy = static_cast<double>(sxy) / 2.0;

				//EXTRA BYTE
				NLubyte byt, extra;
				readByte(data, count, extra);			//extra byte

				//FLAGS BYTE
				h.dead = (extra & 1) != 0;  //DEAD PLAYER = extra bit 0
				h.item_deathbringer = (extra & 2) != 0;		//deathbringer: extra bit 1
				h.deathbringer_affected = (extra & 4) != 0; //deathbringer-affected: extra bit 2
				// ITEMS: movido para este byte
				h.item_shield = (extra & 8) != 0;
				h.item_speed = (extra & 16) != 0;
				h.item_quad = (extra & 32) != 0;

				//verifica se acabou de morrer - play death sound
				if (h.dead && !h.old_dead)
					addThreadMessage(new TM_Sound(SAMPLE_DEATH + rand() % 2));
				h.old_dead = h.dead;

				NLubyte ccb;
				readByte(data, count, ccb);
				h.controls.fromNetwork(ccb, true);

				//bits 5..7 : gundir= 0..7
				h.gundir = ccb >> 5;

				//read helm byte
				readByte(data, count, byt);
				h.visibility = byt;
			}

			//read "enemies on team vislist"
			NLushort eviz;
			readShort(data, count, eviz);
			if (me >= 0)
				fx.player[me].enemyvis = eviz;

			//read who,x,y
			NLubyte who,whox,whoy;
			readByte(data, count, who);
			readByte(data, count, whox);
			readByte(data, count, whoy);

			//update this player's px,py,x,y
			//ignore self and anybody onscreen -- because then I've got better accuracy
			if (who != me && !fx.player[who].onscreen) {
				fx.player[who].roomx = whox / (255/fx.map.w);	//screen = 0..255 / (WXMAX/255)
				fx.player[who].roomy = whoy / (255/fx.map.h);
				fx.player[who].lx = (whox % (255/fx.map.w)) * plw / (255/fx.map.w);	//posicao dentro da tela especifica
				fx.player[who].ly = (whoy % (255/fx.map.h)) * plh / (255/fx.map.h);
			}

			//read player's health and energy
			NLubyte healt, energ;
			readByte(data, count, healt);
			if (me >= 0) {
				fx.player[me].health = healt;
				//EXTRA BIT FROM WAYY ABOVE
				if (xtra & 1) fx.player[me].health += 256;
			}

			readByte(data, count, energ);
			if (me >= 0) {
				fx.player[me].energy = energ;
				//EXTRA BIT FROM WAYY ABOVE
				if (xtra & 2) fx.player[me].energy += 256;
			}

			//read ping of player frame % MAX_PLAYERS
			NLushort daping;
			readShort(data, count, daping);
			fx.player[svframe % maxplayers].ping = daping;
		}//frame not empty
	}

	//(2) process messages (update fx, and add the non frame-related messages to messageQueue)
	for (;;) {
		char* lebuf;
		int msglen;
		lebuf = client->receive_message(&msglen);
		if (lebuf == 0)
			break;

		int count = 0;
		NLubyte code;
		readByte(lebuf, count, code);

		if (LOG_MESSAGE_TRAFFIC)
			log("Message from server, code = %i", code);

		//parse rest of message
		switch (static_cast<Network_data_code>(code)) {
			// name update
			case data_name_update: {
				NLubyte pid;
				string name;
				readByte(lebuf, count, pid);
				readStr(lebuf, count, name);
				if (check_name(name))
					fx.player[pid].name = name;
				else
					log.error("Invalid name for player %d.", pid);
				break;
			}

			case data_text_message: {
				char byte;
				readByte(lebuf, count, byte);
				const Message_type type = static_cast<Message_type>(byte);
				string chatmsg;
				readStr(lebuf, count, chatmsg);
				if (find_nonprintable_char(chatmsg)) {
					log.error("Server sent non-printable characters in a message.");
					addThreadMessage(new TM_DoDisconnect());
				}
				else {
					addThreadMessage(new TM_Text(type, chatmsg));
					if (menu.options.game.messageLogging())
						message_log << date_and_time() << "  " << chatmsg << endl;
				}

				//talk sound
				if (type != msg_info)
					addThreadMessage(new TM_Sound(SAMPLE_TALK));
				break;
			}

			case data_first_packet: {
				NLubyte pid;
				readByte(lebuf, count, pid);	//"who am I"
				me = pid;

				NLubyte color;
				readByte(lebuf, count, color);
				if (color < MAX_PLAYERS / 2)
					fx.player[pid].set_color(color);
				else
					log("Invalid colour (%d) for player %d (me).", color, pid);

				NLubyte map_nr;
				readByte(lebuf, count, map_nr);	//current map number
				current_map = map_nr;

				//reset want-change-teams: this message is send when players are swapped also
				want_change_teams = false;

				NLubyte score;
				readByte(lebuf, count, score);
				fx.teams[0].set_score(score);
				if (fx.teams[0].captures().size() == 0)	// only if just joined the server
					fx.teams[0].set_base_score(score);
				readByte(lebuf, count, score);
				fx.teams[1].set_score(score);
				if (fx.teams[1].captures().size() == 0)	// only if just joined the server
					fx.teams[1].set_base_score(score);

				//server physics parameters
				fx.physics.read(lebuf, count);
				fd.physics = fx.physics;

				// room is probably changed
				fx.player[me].oldx = -1;
				fx.player[me].oldy = -1;

				break;
			}

			case data_frags_update: {
				NLubyte pid;
				NLulong frags;
				readByte(lebuf, count, pid);
				readLong(lebuf, count, frags);
				fx.player[pid].stats().set_frags(frags);
				stable_sort(players_sb.begin(), players_sb.end(), compare_players);
				break;
			}

			case data_flag_update: {
				NLubyte team;
				NLbyte flags;
				readByte(lebuf, count, team);		// team of the flag
				readByte(lebuf, count, flags);	// how many flags
				for (int i = 0; i < flags; i++) {
					if (team == 2) {
						if (i >= static_cast<int>(fx.wild_flags.size()))
							fx.wild_flags.push_back(Flag(WorldCoords()));
					}
					else if (i >= static_cast<int>(fx.teams[team].flags().size()))
						fx.teams[team].add_flag(WorldCoords());
					NLubyte carried;
					readByte(lebuf, count, carried);	// 0==not carried 1==carried
					if (carried == 0) {
						//not carried: update position
						NLubyte px, py;
						NLshort x, y;
						readByte(lebuf, count, px);
						readByte(lebuf, count, py);
						readShort(lebuf, count, x);
						readShort(lebuf, count, y);
						if (team == 2) {
							fx.wild_flags[i].move(WorldCoords(px, py, x, y));
							fx.wild_flags[i].drop();
						}
						else
							fx.teams[team].drop_flag(i, WorldCoords(px, py, x, y));
					}
					else {
						//carried: get carrier
						NLubyte carrier;
						readByte(lebuf, count, carrier);
						if (team == 2)
							fx.wild_flags[i].take(carrier);
						else
							fx.teams[team].steal_flag(i, carrier);
						addThreadMessage(new TM_Sound(SAMPLE_CTF_GOT));
					}
				}
				break;
			}

			case data_rocket_fire: {
				if (!map_ready)
					break;

				NLubyte rpow, rdir;
				NLubyte rids[16];
				NLulong frameno;
				NLubyte rteampower;

				readByte(lebuf, count, rpow);
				readByte(lebuf, count, rdir);
				for (int k = 0; k < rpow; k++)
					readByte(lebuf, count, rids[k]);
				readLong(lebuf, count, frameno);	// frame # of shot
				readByte(lebuf, count, rteampower);	// team (bit 1) and power (bit 0)
				const bool power = ((rteampower & 1) != 0);
				const int team = (rteampower & 2) >> 1;

				NLubyte rpx, rpy;
				NLshort rx, ry;
				readByte(lebuf, count, rpx);
				readByte(lebuf, count, rpy);
				readShort(lebuf, count, rx);
				readShort(lebuf, count, ry);

				ClientPhysicsCallbacks cb(*this);
				fx.shootRockets(cb, 0, rpow, rdir, rids, static_cast<int>(fx.frame - frameno), team, power, rpx, rpy, rx, ry);

				//play sound if rocket on screen
				if (me >= 0 && rpx == fx.player[me].roomx && rpy == fx.player[me].roomy)
					if (power)
						addThreadMessage(new TM_Sound(SAMPLE_QUAD_FIRE));
					else
						addThreadMessage(new TM_Sound(SAMPLE_FIRE));
				break;
			}

			case data_old_rocket_visible: {
				if (!map_ready)
					break;

				NLubyte rockid, rdir;
				NLulong frameno;
				NLubyte rteampower;
				readByte(lebuf, count, rockid);
				readByte(lebuf, count, rdir);
				readLong(lebuf, count, frameno);
				readByte(lebuf, count, rteampower);
				const bool power = ((rteampower & 1) != 0);
				const int team = (rteampower & 2) >> 1;

				NLubyte rpx, rpy;
				readByte(lebuf, count, rpx);
				readByte(lebuf, count, rpy);
				NLshort rx, ry;
				readShort(lebuf, count, rx);
				readShort(lebuf, count, ry);

				ClientPhysicsCallbacks cb(*this);
				fx.shootRockets(cb, 0, 1, rdir, &rockid, static_cast<int>(fx.frame - frameno), team, power, rpx, rpy, rx, ry);
				// no sound
				break;
			}

			case data_rocket_delete: {
				if (!map_ready)
					break;

				NLubyte rockid, target;
				readByte(lebuf, count, rockid);	// rocket object id
				readByte(lebuf, count, target);	// target player
				//hit position
				NLshort rokx, roky;
				readShort(lebuf, count, rokx);
				readShort(lebuf, count, roky);
				fx.rock[rockid].owner = -1;
				if (target != 255) {	// hit player
					if (target < 250)	// blink player if not hit shield (252)
						fx.player[target].hitfx = get_time() + .3;
					addThreadMessage(new TM_GunexploEffect((int)rokx, (int)roky, fx.rock[rockid].px, fx.rock[rockid].py));
					addThreadMessage(new TM_Sound(SAMPLE_HIT));
				}
				break;
			}

			case data_score_update: {
				NLubyte team;
				NLubyte score;
				readByte(lebuf, count, team);		//team
				readByte(lebuf, count, score);		//new score
				fx.teams[team].set_score(score);	// update the score
				break;
			}

			case data_sound: {
				NLubyte sample;
				readByte(lebuf, count, sample);		// sample #
				if (sample < NUM_OF_SAMPLES)
					addThreadMessage(new TM_Sound(sample));
				break;
			}

			case data_pup_visible: {
				NLubyte iid, kind, spos;
				readByte(lebuf, count, iid);		// item id
				readByte(lebuf, count, kind);		// kind
				fx.item[iid].kind = static_cast<Powerup::Pup_type>(kind);
				readByte(lebuf, count, spos);		// screen x
				fx.item[iid].px = spos;
				readByte(lebuf, count, spos);		// screen y
				fx.item[iid].py = spos;
				NLushort coord;
				readShort(lebuf, count, coord);		// pos x
				fx.item[iid].x = coord;
				readShort(lebuf, count, coord);		// pos y
				fx.item[iid].y = coord;
				break;
			}

			case data_pup_picked: {
				NLubyte iid;
				readByte(lebuf, count, iid);
				fx.item[iid].kind = Powerup::pup_unused;		// no more!
				break;
			}

			case data_pup_timer: {
				NLubyte iid;
				NLushort time;
				readByte(lebuf, count, iid);	//kind
				readShort(lebuf, count, time);	//amount of time
				if (me >= 0) {
					if (iid == Powerup::pup_turbo)
						fx.player[me].item_speed_time = get_time() + time;
					else if (iid == Powerup::pup_shadow)
						fx.player[me].item_helm_time = get_time() + time;
					else if (iid == Powerup::pup_power)
						fx.player[me].item_quad_time = get_time() + time;
				}
				break;
			}

			case data_weapon_change: {
				NLubyte level;
				readByte(lebuf, count, level);	// weapon level
				if (me >= 0)
					fx.player[me].weapon = level;
				break;
			}

			case data_map_change: {
				map_ready = false;	// map NOT ready anymore: must load/change
				want_map_exit = false;		// and player does not want to exit the map anymore
				fx.teams[0].remove_flags();
				fx.teams[1].remove_flags();
				fx.wild_flags.clear();
				NLushort crc;
				readShort(lebuf, count, crc);
				string mapname;
				readStr(lebuf, count, mapname);
				NLubyte map_nr;
				readByte(lebuf, count, map_nr);
				current_map = map_nr;
				if (map_vote == current_map)
					map_vote = -1;
				fx.player[me].oldx = -1;
				fx.player[me].oldy = -1;
				old_map = fx.map.title;
				addThreadMessage(new TM_MapChange(mapname, crc));
				break;
			}

			case data_world_reset:
				for (int iid = 0; iid < MAX_PICKUPS; ++iid)
					fx.item[iid].kind = Powerup::pup_unused;
				break;

			case data_gameover_show: {
				NLubyte plaque;
				readByte(lebuf, count, plaque);
				if (plaque == NEXTMAP_CAPTURE_LIMIT || plaque == NEXTMAP_VOTE_EXIT) {
					NLubyte score;
					readByte(lebuf, count, score);	//RED team final score
					red_final_score = score;
					readByte(lebuf, count, score);  //BLUE team final score
					blue_final_score = score;
					for (vector<ClientPlayer>::iterator pi = fx.player.begin(); pi != fx.player.end(); ++pi)
						pi->stats().finish_stats(get_time());
					if (menu.options.game.showStats() && menusel == menu_none && openMenus.empty()) {
						menusel = menu_teams;
						stats_autoshowing = true;
					}
					if (gameover_plaque == NEXTMAP_NONE && menu.options.game.saveStats())
						fx.save_stats("client_stats", old_map);
					gameover_plaque = plaque;
				}
				else {
					gameover_plaque = NEXTMAP_NONE;
					if (stats_autoshowing) {
						menusel = menu_none;
						stats_autoshowing = false;
					}
				}
				break;
			}

			case data_start_game:
				gameover_plaque = NEXTMAP_NONE;		//hide
				fx.teams[0].clear_stats();
				fx.teams[1].clear_stats();
				for (vector<ClientPlayer>::iterator pi = fx.player.begin(); pi != fx.player.end(); ++pi)
					pi->stats().clear();
				if (stats_autoshowing) {
					menusel = menu_none;
					stats_autoshowing = false;
				}
				break;

			case data_deathbringer: {
				NLubyte pid;
				NLulong frameno;
				NLubyte sx, sy;
				readByte(lebuf, count, pid);
				readLong(lebuf, count, frameno);	// creation frame
				readByte(lebuf, count, sx);
				readByte(lebuf, count, sy);
				NLushort hx, hy;
				readShort(lebuf, count, hx);
				readShort(lebuf, count, hy);
				addThreadMessage(new TM_Deathbringer(pid / TSIZE, get_time() + (frameno - fx.frame) * 0.1, hx, hy, sx, sy));
				addThreadMessage(new TM_Sound(SAMPLE_USEDEATHBRINGER));
				break;
			}

			case data_file_download: {
				NLubyte last;
				NLushort chunkSize;
				readShort(lebuf, count, chunkSize);		//chunk size
				readByte(lebuf, count, last);		//"last chunk"?
				process_udp_download_chunk(&lebuf[count], chunkSize, (last != 0));
				break;
			}

			case data_registration_response: {
				NLubyte response;
				readByte(lebuf, count, response);
				if (response == 1)	// success
					tournamentPassword.serverAcceptsToken();
				else
					tournamentPassword.serverRejectsToken();
				break;
			}

			case data_crap_update: {
				NLubyte pid, color, regStatus;
				NLulong prank, pscore, nscore;
				readByte(lebuf, count, pid);
				readByte(lebuf, count, color);
				readByte(lebuf, count, regStatus);
				readLong(lebuf, count, prank);		//ranking#
				readLong(lebuf, count, pscore);		//score
				readLong(lebuf, count, nscore);		//score	NEG v0.4.8
				readLong(lebuf, count, max_world_rank);		//world players count
				if (color < MAX_PLAYERS / 2)
					fx.player[pid].set_color(color);
				else
					log("Invalid colour (%d) for player %d.", color, pid);
				ClientLoginStatus ls;
				ls.fromNetwork(regStatus);
				if (pid == me && fx.player[pid].reg_status != ls) {
					ostringstream msg;
					msg << "Status: ";	// 8
					if (ls.token()) {
						if (ls.masterAuth()) {
							msg << "master authorized, ";
							if (!ls.tournament())
								msg << "not ";
							msg << "recording";
						}
						else {
							msg << "master auth pending, will ";	// + 26 = 34
							if (!ls.tournament())
								msg << "not ";	// + 4 = 38
							msg << "record";	// + 6 = 44
						}
					}
					else
						msg << "no tournament login";
					if (ls.localAuth())
						msg << "; locally authorized";	// + 20 = 64
					if (ls.admin())
						msg << "; administrator";	// + 15 = 79 (maximum length)
					addThreadMessage(new TM_Text(msg_info, msg.str()));
				}
				fx.player[pid].reg_status = ls;
				fx.player[pid].rank = static_cast<int>(prank);
				fx.player[pid].score = static_cast<int>(pscore);
				fx.player[pid].neg_score = static_cast<int>(nscore);
				// update new team powers
				float power[2] = { 0, 0 };
				for (int i = 0; i < fx.maxplayers; i++)
					if (fx.player[i].used)
						power[fx.player[i].team()] += (fx.player[i].score + 1.) / (fx.player[i].neg_score + 1.);
				for (int t = 0; t < 2; t++)
					fx.teams[t].set_power(power[t]);
				break;
			}

			case data_map_time: {
				int current_time, time_left;
				readLong(lebuf, count, current_time);
				readLong(lebuf, count, time_left);
				map_start_time = static_cast<int>(get_time()) - current_time;
				if (time_left > 0) {
					map_end_time = static_cast<int>(get_time()) + time_left;
					map_time_limit = true;
				}
				else
					map_time_limit = false;
				if (LOG_MESSAGE_TRAFFIC)
					log("Map time received. Time left %d seconds.", time_left);
				break;
			}

			case data_map_list: {
				NLubyte width, height, votes;
				MapInfo mapinfo;
				readStr(lebuf, count, mapinfo.title);
				readStr(lebuf, count, mapinfo.author);
				readByte(lebuf, count, width);
				readByte(lebuf, count, height);
				readByte(lebuf, count, votes);
				mapinfo.width = width;
				mapinfo.height = height;
				mapinfo.votes = votes;
				MutexLock ml(mapInfoMutex);
				maps.push_back(mapinfo);
				break;
			}

			case data_map_votes_update: {
				NLbyte total, map_nr, votes;
				readByte(lebuf, count, total);
				MutexLock ml(mapInfoMutex);
				for (int i = 0; i < total; i++) {
					readByte(lebuf, count, map_nr);
					readByte(lebuf, count, votes);
					if (map_nr >= 0 && map_nr < static_cast<int>(maps.size()))
						maps[map_nr].votes = votes;
				}
				break;
			}

			case data_capture: {
				NLubyte pid;
				readByte(lebuf, count, pid);
				fx.player[pid].stats().add_capture(get_time());
				fx.teams[pid / TSIZE].add_score(get_time() - map_start_time, fx.player[pid].name);
				break;
			}

			case data_kill: {
				NLubyte attacker, target;
				readByte(lebuf, count, attacker);
				readByte(lebuf, count, target);
				const bool deathbringer = attacker & 0x80;
				attacker &= ~0x80;
				const bool flag = target & 0x80;
				target &= ~0x80;
				if (fx.player[target].stats().current_cons_kills() >= 10) {
					ostringstream msg;
					msg << fx.player[target].name.c_str() << "'s killing spree was ended by " << fx.player[attacker].name.c_str() << '.';
					addThreadMessage(new TM_Text(msg_info, msg.str()));
				}
				fx.player[attacker].stats().add_kill(deathbringer);
				fx.teams[attacker / TSIZE].add_kill();
				fx.player[target].stats().add_death(deathbringer, static_cast<int>(get_time()));
				fx.teams[target / TSIZE].add_death();
				if (flag) {
					fx.player[attacker].stats().add_carrier_kill();
					fx.player[target].stats().add_flag_drop(get_time());
					fx.teams[target / TSIZE].add_flag_drop();
				}
				if (fx.player[attacker].stats().current_cons_kills() % 10 == 0) {
					if (attacker == me)
						addThreadMessage(new TM_Sound(SAMPLE_KILLING_SPREE));
					ostringstream msg;
					msg << fx.player[attacker].name.c_str() << " is on a killing spree!";
					addThreadMessage(new TM_Text(msg_info, msg.str()));
				}
				break;
			}

			case data_flag_take: {
				NLubyte pid;
				readByte(lebuf, count, pid);
				fx.player[pid].stats().add_flag_take(get_time());
				fx.teams[pid / TSIZE].add_flag_take();
				break;
			}

			case data_flag_return: {
				NLubyte pid;
				readByte(lebuf, count, pid);
				fx.player[pid].stats().add_flag_return();
				fx.teams[pid / TSIZE].add_flag_return();
				break;
			}

			case data_flag_drop: {
				NLubyte pid;
				readByte(lebuf, count, pid);
				fx.player[pid].stats().add_flag_drop(get_time());
				fx.teams[pid / TSIZE].add_flag_drop();
				break;
			}

			case data_suicide: {
				NLubyte pid;
				readByte(lebuf, count, pid);
				const bool flag = pid & 0x80;
				pid &= ~0x80;
				if (fx.player[pid].stats().current_cons_kills() >= 10) {
					ostringstream msg;
					msg << fx.player[pid].name.c_str() << "'s killing spree was ended.";
					addThreadMessage(new TM_Text(msg_info, msg.str()));
				}
				fx.player[pid].stats().add_suicide(static_cast<int>(get_time()));
				fx.teams[pid / TSIZE].add_suicide();
				if (flag) {
					fx.player[pid].stats().add_flag_drop(get_time());
					fx.teams[pid / TSIZE].add_flag_drop();
				}
				break;
			}

			case data_new_player: {
				NLubyte pid;
				readByte(lebuf, count, pid);
				fx.player[pid].stats().clear();
				fx.player[pid].stats().set_start_time(get_time());
				fx.player[pid].stats().set_lifetime(0);
				break;
			}

			case data_spawn: {
				NLubyte pid;
				readByte(lebuf, count, pid);
				fx.player[pid].stats().set_spawn_time(get_time());
				break;
			}

			case data_team_movements_shots: {
				for (int i = 0; i < 2; i++) {
					NLlong movement;
					readLong(lebuf, count, movement);
					fx.teams[i].set_movement(movement);
					NLshort data;
					readShort(lebuf, count, data);
					fx.teams[i].set_shots(data);
					readShort(lebuf, count, data);
					fx.teams[i].set_hits(data);
					readShort(lebuf, count, data);
					fx.teams[i].set_shots_taken(data);
				}
				break;
			}

			case data_team_stats: {
				for (int i = 0; i < 2; i++) {
					NLubyte data;
					readByte(lebuf, count, data);
					fx.teams[i].set_kills(data);
					readByte(lebuf, count, data);
					fx.teams[i].set_deaths(data);
					readByte(lebuf, count, data);
					fx.teams[i].set_suicides(data);
					readByte(lebuf, count, data);
					fx.teams[i].set_flags_taken(data);
					readByte(lebuf, count, data);
					fx.teams[i].set_flags_dropped(data);
					readByte(lebuf, count, data);
					fx.teams[i].set_flags_returned(data);
				}
				break;
			}

			case data_movements_shots: {
				NLubyte id;
				readByte(lebuf, count, id);
				// todo: check id
				NLlong movement;
				readLong(lebuf, count, movement);
				fx.player[id].stats().set_movement(movement);
				fx.player[id].stats().save_speed(get_time());
				NLshort data;
				readShort(lebuf, count, data);
				fx.player[id].stats().set_shots(data);
				readShort(lebuf, count, data);
				fx.player[id].stats().set_hits(data);
				readShort(lebuf, count, data);
				fx.player[id].stats().set_shots_taken(data);
				break;
			}

			case data_stats: {
				NLubyte id;
				readByte(lebuf, count, id);
				const bool flag = (id & 0x80);
				id &= ~0x80;
				// todo: check id
				Statistics& stats = fx.player[id].stats();
				if (flag)
					stats.add_flag_take(get_time());
				NLubyte data;
				readByte(lebuf, count, data);
				stats.set_kills(data);
				readByte(lebuf, count, data);
				stats.set_deaths(data);
				readByte(lebuf, count, data);
				stats.set_cons_kills(data);
				readByte(lebuf, count, data);
				stats.set_current_cons_kills(data);
				readByte(lebuf, count, data);
				stats.set_cons_deaths(data);
				readByte(lebuf, count, data);
				stats.set_current_cons_deaths(data);
				readByte(lebuf, count, data);
				stats.set_suicides(data);
				readByte(lebuf, count, data);
				stats.set_captures(data);
				readByte(lebuf, count, data);
				stats.set_flags_taken(data);
				readByte(lebuf, count, data);
				stats.set_flags_dropped(data);
				readByte(lebuf, count, data);
				stats.set_flags_returned(data);
				readByte(lebuf, count, data);
				stats.set_carriers_killed(data);
				int ldata;
				readLong(lebuf, count, ldata);
				stats.set_start_time(get_time() - ldata);
				readLong(lebuf, count, ldata);
				stats.set_lifetime(ldata);
				readLong(lebuf, count, ldata);
				stats.set_flag_carrying_time(ldata);
				break;
			}

			case data_name_authorization_request: {
				addThreadMessage(new TM_NameAuthorizationRequest());
				break;
			}

			case data_server_settings: {
				NLubyte caplimit, timelimit, extratime, misc1;
				readByte(lebuf, count, caplimit);
				readByte(lebuf, count, timelimit);
				readByte(lebuf, count, extratime);
				readByte(lebuf, count, misc1);
				addThreadMessage(new TM_ServerSettings(caplimit, timelimit, extratime, misc1));
				break;
			}

			default:
				if (code < data_reserved_range_first || code > data_reserved_range_last) {
					log.error("Server sent an unknown message code: %i, length %i", code, msglen);
					addThreadMessage(new TM_DoDisconnect());
					return;	// don't process the rest of the messages
				}
				// just ignore commands in reserved range: they're probably some extension we don't have to care about
				break;
		}
	}

	//detect screen changes / clear powerup fx's
	if (me >= 0 && (fx.player[me].roomx != fx.player[me].oldx) || (fx.player[me].roomy != fx.player[me].oldy)) {
		for (int j = 0; j < MAX_PICKUPS; j++)
			if (fx.item[j].px == fx.player[me].oldx && fx.item[j].py == fx.player[me].oldy)
				fx.item[j].kind = Powerup::pup_unused;	// erase

		fx.player[me].oldx = fx.player[me].roomx;
		fx.player[me].oldy = fx.player[me].roomy;

		predrawNeeded = true;
	}

	me = whatme;	// hack
}

//send chat message
void Client::send_chat(const string& msg) {
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, data_text_message);
	writeStr(lebuf, count, msg);
	client->send_message(lebuf, count);
}

//print message to "console"
void Client::print_message(Message_type type, const string& msg) {
	if (static_cast<int>(chatbuffer.size()) == client_graphics.chat_max_lines())
		chatbuffer.pop_front();
	Message message(type, msg, static_cast<int>(get_time()));
	chatbuffer.push_back(message);
}

void Client::save_screenshot() {
	string filename;
	for (int i = 0; i < 1000; i++) {
		// filename: screens/outgxxx.pcx
		ostringstream fname;
		fname << wheregamedir << "screens" << directory_separator;
		fname << "outg" << setfill('0') << setw(3) << i << ".pcx";
		if (!file_exists(fname.str().c_str(), FA_ARCH | FA_DIREC | FA_HIDDEN | FA_RDONLY | FA_SYSTEM, 0)) {
			filename = fname.str();
			break;
		}
	}

	ostringstream message;
	if (client_graphics.save_screenshot(filename))
		message << "Saved screenshot to " << filename << '.';
	else
		message << "Could not save screenshot to " << filename << '.';
	print_message(msg_warning, message.str().c_str());
}

//toggle help screen
void Client::toggle_help() {
	if (openMenus.safeTop() == &menu.help.menu)
		openMenus.close();
	else
		showMenu(menu.help);
}

const char* Client::refreshStatusAsString() const {
	switch (refreshStatus) {
		case RS_none: 		return "Inactive";
		case RS_running:	return "Running";
		case RS_failed:		return "Failed";
		case RS_contacting:	return "Contacting the servers...";
		case RS_connecting:	return "Getting server list: connecting...";
		case RS_receiving:	return "Getting server list: receiving...";
		default:	nAssert(0);
	}
	nAssert(0); return 0;
}

void Client::getServerListThread() {
	if (LOG_THREAD_IDS)
		log("getServerListThread() ID = %d, prio = %d", pthread_self(), threadPriority());

	nAssert(refreshStatus == RS_running);

	// get server list and refresh
	bool ok = getServerList();
	if (!abortThreads)
		if (!refresh_all_servers())
			ok = false;

	if (LOG_THREAD_IDS)
		log("exiting: getServerListThread() ID = %d, prio = %d", pthread_self(), threadPriority());
	refreshStatus = ok ? RS_none : RS_failed;
}

void Client::refreshThread() {
	if (LOG_THREAD_IDS)
		log("refreshThread() ID = %d, prio = %d", pthread_self(), threadPriority());

	nAssert(refreshStatus == RS_running);
	const bool ok = refresh_all_servers();

	if (LOG_THREAD_IDS)
		log("exiting: refreshThread() ID = %d, prio = %d", pthread_self(), threadPriority());
	refreshStatus = ok ? RS_none : RS_failed;
}

//refresh servers command
bool Client::refresh_all_servers() {
	bool ok = refresh_servers(gamespy);
	if (!abortThreads && !menu.connect.favorites())
		if (!refresh_servers(mgamespy))
			ok = false;
	return ok;
}

class TempPingData {	// internal to Client::refresh_servers
	double st[4];	// send time
	int rc;			// count of received packets
	double rt;		// sum of pings (for averaging)

public:
	TempPingData() : rc(0), rt(0) { }
	void send(int pack) { st[pack] = get_time(); }
	void receive(int pack) { ++rc; rt += get_time() - st[pack]; }
	int received() const { return rc; }
	int ping() const { return static_cast<int>(1000 * rt / rc); }
};

//refresh servers command
bool Client::refresh_servers(vector<ServerListEntry>& gamespy) {
	refreshStatus = RS_contacting;

	nlOpenMutex.lock();
	nlDisable(NL_BLOCKING_IO);
	NLsocket sock = nlOpen(0, NL_UNRELIABLE);
	nlOpenMutex.unlock();

	if (sock == NL_INVALID) {
		log.error("Can't open socket for refreshing servers. %s", getNlErrorString());
		return false;
	}

	char lebuf[512];

	serverListMutex.lock();

	const int nServers = static_cast<int>(gamespy.size());
	vector<TempPingData> tempd(nServers);
	int num_valid = 0;			// count of valid entries still waiting for a response

	for (int i = 0; i < nServers; i++) {
		gamespy[i].refreshed = true;
		gamespy[i].ping = 0;
		num_valid++;
	}

	serverListMutex.unlock();

	for (int round = 0; round < 20; round++) {	// each round takes .1 seconds
		if (abortThreads) {
			log("Refreshing servers aborted: client exiting.");
			return false;
		}

		if (round < 4) {	// on first 4 rounds, packets are sent to each server
			MutexLock ml(serverListMutex);
			for (int i = 0; i < nServers; i++) {
				int count = 0;
				writeLong(lebuf, count, 0);			//special packet
				writeLong(lebuf, count, 200);		//serverinfo request
				writeByte(lebuf, count, (NLubyte)i);		//connect entry (am I lazy or what)
				writeByte(lebuf, count, (NLubyte)round);		//packet number

				nlSetRemoteAddr(sock, &gamespy[i].address());
				nlWrite(sock, lebuf, count);
				tempd[i].send(round);
			}
		}
		else if (num_valid <= 0)	// all done
			break;

		// parse received responses
		for (int subRound = 0; subRound < 20; subRound++) {
			MS_SLEEP(5);

			for (;;) {	// continue while there are new packets
				int len = nlRead(sock, lebuf, 512);
				if (len <= 0)
					break;

				int count = 0;
				NLulong dw1, dw2;
				readLong(lebuf, count, dw1);
				readLong(lebuf, count, dw2);
				if (dw1 != 0 || dw2 != 200)
					continue;

				NLubyte index, pack;
				readByte(lebuf, count, index);	// gamespy entry number echoed by the server
				readByte(lebuf, count, pack);	// packet #

				if (index >= nServers || pack >= 4 || pack > round || len < count)	// don't have to worry about < 0 because they're unsigned
					continue;

				MutexLock ml(serverListMutex);

				NLaddress from;
				nlGetRemoteAddr(sock, &from);
				if (!nlAddrCompare(&from, &gamespy[index].address()))
					continue;

				readStr(lebuf, count, gamespy[index].info);

				if (tempd[index].received() == 0)	// first reply -> server has changed to being valid
					num_valid--;

				tempd[index].receive(pack);
				gamespy[index].ping = tempd[index].ping();

				gamespy[index].noresponse = false;	// set here in advance so that the main thread will already show it
			}
		}
	}

	// mark those that got no responses
	{
		MutexLock ml(serverListMutex);
		for (int i = 0; i < nServers; i++)
			if (tempd[i].received() == 0)
				gamespy[i].noresponse = true;
	}

	nlClose(sock);
	return true;
}

bool Client::getServerList() {
	refreshStatus = RS_connecting;

	//open a nonblocking socket
	nlOpenMutex.lock();
	nlDisable(NL_BLOCKING_IO);
	NLsocket sock = nlOpen(0, NL_RELIABLE);
	nlOpenMutex.unlock();
	if (sock == NL_INVALID) {
		log.error("Client can't open socket to connect to master server. %s", getNlErrorString());
		return false;
	}

	//connect the nonblocking way
	if (nlConnect(sock, &master_address) == NL_FALSE) {
		log.error("Client can't connect to master server. %s", getNlErrorString());
		nlClose(sock);
		sock = NL_INVALID;
		return false;
	}
	
	ifstream in((wheregamedir + "config" + directory_separator + "master.txt").c_str());
	string skip;
	string master_script;
	if (!getline_skip_comments(in, skip) || !getline_skip_comments(in, skip) || !getline_skip_comments(in, master_script))
		master_script = "/outgun/servers/";
	in.close();

	//build query
	ostringstream request;
	request << "GET " << master_script << "?simple&protocol=" << url_encode(GAME_PROTOCOL) << " HTTP/1.0\r\n";
	request << "User-Agent: Outgun " << GAME_VERSION << "\r\n";
	request << "Connection: close\r\n\r\n";

	NetworkResult result = writeToUnblockingTCP(sock, request.str().data(), request.str().length(), &abortThreads, 30000);
	if (result != NR_ok) {
		nlClose(sock);
		if (!abortThreads)
			log("Client can't connect to master server. %s", result == NR_timeout ? "Timeout" : getNlErrorString());
		return false;
	}

	refreshStatus = RS_receiving;

	log("Successfully sent query to master: '%s'", formatForLogging(request.str()).c_str());

	std::stringstream response;
	result = saveAllFromUnblockingTCP(sock, response, &abortThreads, 30000);
	nlClose(sock);
	if (result != NR_ok) {
		if (!abortThreads)
			log("Error receiving server list from master. %s", result == NR_timeout ? "Timeout" : getNlErrorString());
		return false;
	}

	log("Full response: '%s'", formatForLogging(response.str()).c_str());

	MutexLock ml(serverListMutex);

	//clear the old gamespy master screen
	mgamespy.clear();

	string line, empty;

	// Skip HTTP headers.
	while (getline(response, line, '\r') && getline(response, empty, '\n'))
		if (line.empty())
			break;

	// The first line is the newest version.
	getline_smart(response, line);
	if (line.empty()) {
		log.error("Incorrect data received from master server.");
		return false;
	}
	if (line != GAME_VERSION)
		menu.newVersion.set(string() + "New version: " + line);

	// The second line is the total number of servers.
	getline_smart(response, line);
	if (line.empty()) {
		log.error("Incorrect data received from master server.");
		return false;
	}
	const int total_servers = atoi(line);

	// Parse the successful response into the gamespy screen.
	int servers_read;
	for (servers_read = 0; servers_read < total_servers && getline_smart(response, line); servers_read++) {
		ServerListEntry spy;
		if (spy.setAddress(line))
			mgamespy.push_back(spy);
	}

	if (servers_read != total_servers) {
		log.error("Incorrect data received from master server: Server count mismatch.");
		return false;
	}

	return true;
}

void Client::loop(volatile bool* quitFlag) {
	nAssert(quitFlag);
	quitCommand = false;

	openMenus.clear();
	showMenu(menu);
	gameshow = false;

	const unsigned long sendFrameStep = 20;	// 20 steps of 200 Hz clock equals the 10 Hz server clock
	unsigned long nextSendFrame = time_counter;
	unsigned long nextClientFrameI = time_counter;
	double nextClientFrameF = nextClientFrameI;

	bool key_fire = false, key_kill = false, key_swap = false, key_votexit = false, key_drop_flag = false;
	char key_up=0, key_down=0, key_left=0, key_right=0;
	while (!quitCommand && !*quitFlag) {
		// (1) loop doing input/sleep before next simulation/draw time
		for (;;) {
			//quit key Control-F12
			if ((key[KEY_LCONTROL] || key[KEY_RCONTROL]) && key[KEY_F12]) {
				quitCommand = true;
				break;
			}

			// menu keypresses; ESC is dealt with elsewhere
			if (menusel != menu_none || !openMenus.empty()) {
				while (keypressed()) {
					int ch = readkey();
					const int sc = ch >> 8;	// scancode
					ch &= 0xFF;				// character

					if (sc == KEY_F11)
						screenshot = true;

					if (sc == KEY_F1)
						toggle_help();
					else if (sc == KEY_F5) {
						if (openMenus.safeTop() == &m_serverInfo.menu)
							openMenus.close();
						else
							showMenu(m_serverInfo);
						stats_autoshowing = false;
					}
					else if (menusel == menu_maps || menusel == menu_players || menusel == menu_teams) {
						if (sc == KEY_F2) {
							menusel = (menusel == menu_maps ? menu_none : menu_maps);
							stats_autoshowing = false;
						}
						else if (sc == KEY_F3) {
							menusel = (menusel == menu_teams ? menu_none : menu_teams);
							stats_autoshowing = false;
						}
						else if (sc == KEY_F4) {
							menusel = (menusel == menu_players ? menu_none : menu_players);
							stats_autoshowing = false;
						}
					}

					switch (menusel) {
						case menu_maps:
							if (key[KEY_UP])
								client_graphics.map_list_prev();
							if (key[KEY_DOWN])
								client_graphics.map_list_next();
							if (key[KEY_PGUP])
								client_graphics.map_list_prev_page();
							if (key[KEY_PGDN])
								client_graphics.map_list_next_page();
							if (key[KEY_HOME])
								client_graphics.map_list_begin();
							if (key[KEY_END])
								client_graphics.map_list_end();
							if (isdigit(ch) && edit_map_vote.size() < 3)
								edit_map_vote += ch;
							else if (sc == KEY_BACKSPACE) {
								if (!edit_map_vote.empty())
									edit_map_vote.erase(edit_map_vote.end() - 1);
							}
							else if (sc == KEY_ENTER || sc == KEY_ENTER_PAD) {
								int new_vote = atoi(edit_map_vote) - 1;
								edit_map_vote.clear();
								if (new_vote != map_vote && (new_vote >= 0 || map_vote >= 0)) {
									map_vote = new_vote;
									// send map vote
									char lebuf[16];
									int count = 0;
									writeByte(lebuf, count, data_map_vote);
									writeByte(lebuf, count, map_vote);
									client->send_message(lebuf, count);
								}
							}
							break;
						case menu_players:
							if (key[KEY_UP] || key[KEY_LEFT] || key[KEY_PGUP])
								player_stats_page = max(0, player_stats_page - 1);
							if (key[KEY_DOWN] || key[KEY_RIGHT] || key[KEY_PGDN])
								player_stats_page = min(3, player_stats_page + 1);
							break;
						case menu_teams:
							if (key[KEY_UP])
								client_graphics.team_captures_prev();
							if (key[KEY_DOWN])
								client_graphics.team_captures_next();
						case menu_none:
							if (!openMenus.empty())
								openMenus.handleKeypress(sc, ch);
							break;
						default:
							nAssert(0);
					}
				}
			}
			// menu not showing: send keypresses to the game
			else {
				bool sendnow = false;

				// control == fire
				bool fire = key[KEY_LCONTROL] || key[KEY_RCONTROL];
				if (!fire && menu.options.controls.joystick() && !poll_joystick() &&
					joy[0].num_buttons > menu.options.controls.joyShoot() - 1 &&
					joy[0].button[menu.options.controls.joyShoot() - 1].b)
						fire = true;

				if (fire) {
					if (!key_fire) {
						key_fire = true;

						//"fire" message (+ATTACK)
						char lebuf[16]; int count = 0;
						writeByte(lebuf, count, data_fire_on);
						client->send_message(lebuf, count);

						//send early keys packet
						sendnow = true;
					}
				}
				else {
					if (key_fire) {
						key_fire = false;

						//"un-fire" message (-ATTACK)
						char lebuf[16]; int count = 0;
						writeByte(lebuf, count, data_fire_off);
						client->send_message(lebuf, count);

						//send early keys packet
						sendnow = true;
					}
				}

				// del == suicide event
				if (key[KEY_DEL]) {
					if (!key_kill) {
						key_kill = true;

						//"suicide" message
						char lebuf[16]; int count = 0;
						writeByte(lebuf, count, data_suicide);
						client->send_message(lebuf, count);
					}
				}
				else key_kill = false;

				// page down == drop flag
				if (key[KEY_PGDN]) {
					if (!key_drop_flag) {
						key_drop_flag = true;
						char lebuf[16]; int count = 0;
						writeByte(lebuf, count, data_drop_flag);
						client->send_message(lebuf, count);
					}
				}
				else if (key_drop_flag) {
					key_drop_flag = false;
					char lebuf[16]; int count = 0;
					writeByte(lebuf, count, data_stop_drop_flag);
					client->send_message(lebuf, count);
				}

				// end == want/don't want to change team
				if (key[KEY_END]) {
					if (!key_swap) {
						key_swap = true;

						want_change_teams = !want_change_teams;

						char lebuf[16]; int count = 0;
						if (want_change_teams)
							writeByte(lebuf, count, data_change_team_on);
						else
							writeByte(lebuf, count, data_change_team_off);
						client->send_message(lebuf, count);
					}
				}
				else key_swap = false;

				// l,r,u,d,fire game keys
				if ((key[KEY_UP]    != key_up)    ||
					(key[KEY_DOWN]  != key_down)  ||
					(key[KEY_LEFT]  != key_left)  ||
					(key[KEY_RIGHT] != key_right)) {
					//keys changed
					key_up    = key[KEY_UP];
					key_down  = key[KEY_DOWN];
					key_left  = key[KEY_LEFT];
					key_right = key[KEY_RIGHT];
					//send early keys packet
					sendnow = true;
				}

				// send client's input packet now
				if (sendnow)
					send_frame(false);

				static bool alt_sequence = false;

				if (menu.options.controls.keypadMoving()) {
					// Check Alt+keypad sequences
					if (keyboard_needs_poll())
						poll_keyboard();	// ignore return value
					if (key_shifts & KB_INALTSEQ_FLAG)
						alt_sequence = true;
				}

				// keypresses to talk prompt
				while (keypressed()) {
					//get key
					int ch = readkey();
					const int sc = ch >> 8;	// scancode
					ch &= 0xFF;				// character

					// toggle help
					if (sc == KEY_F1)
						toggle_help();
					else if (sc == KEY_F2) {
						menusel = menu_maps;
						stats_autoshowing = false;
					}
					else if (sc == KEY_F3) {
						menusel = menu_teams;
						stats_autoshowing = false;
					}
					else if (sc == KEY_F4) {
						menusel = menu_players;
						stats_autoshowing = false;
					}
					else if (sc == KEY_F5) {
						if (openMenus.safeTop() == &m_serverInfo.menu)
							openMenus.close();
						else
							showMenu(m_serverInfo);
						stats_autoshowing = false;
					}

					// change colours
					if (sc == KEY_HOME) {
						if (key[KEY_LCONTROL] || key[KEY_RCONTROL])
							client_graphics.reset_playground_colors();
						else
							client_graphics.random_playground_colors();
						predrawNeeded = true;
					}

					// Insert: show more messages
					if (sc == KEY_INSERT) {
						show_all_messages = !show_all_messages;
					}

					// F11: screenshot
					else if (sc == KEY_F11) {
						screenshot = true;
					}
					// Backspace: erase one character
					else if (sc == KEY_BACKSPACE) {
						if (!talkbuffer.empty())
							talkbuffer.erase(talkbuffer.end() - 1);
					}
					// Enter: send text
					else if (sc == KEY_ENTER || sc == KEY_ENTER_PAD) {
						if (!talkbuffer.empty()) {
							send_chat(talkbuffer);
							talkbuffer.clear();
						}
					}
					// Add character to text, max text length 60 chars.
					else if (talkbuffer.length() < 60 && !is_nonprintable_char(ch) &&
						(!menu.options.controls.keypadMoving() || (!is_keypad(sc) && !alt_sequence)))
							talkbuffer += static_cast<char>(ch);
				}
				if (!(key_shifts & KB_INALTSEQ_FLAG))
					alt_sequence = false;
			}

			// F8 == want/don't want to exit map
			if (openMenus.empty() && key[KEY_F8]) {
				if (!key_votexit) {
					key_votexit = true;

					want_map_exit = !want_map_exit;

					char lebuf[16]; int count = 0;
					if (want_map_exit)
						writeByte(lebuf, count, data_map_exit_on);
					else
						writeByte(lebuf, count, data_map_exit_off);
					client->send_message(lebuf, count);
				}
			}
			else
				key_votexit = false;

			//ESC = show/hide menu, go back menu (special key)
			static bool kesc = false;
			if (key[KEY_ESC]) {
				if (!kesc) {
					kesc = true;
					if (!talkbuffer.empty()) // cancel chat
						talkbuffer.clear();
					else if (menusel != menu_none) {
						menusel = menu_none;
						stats_autoshowing = false;
					}
					else if (openMenus.empty())
						showMenu(menu);
					else
						MCF_menuCloser();
				}
			}
			else
				kesc = false;

			// process messages from network that have been collected
			{
				MutexLock ml(frameMutex);
				while (!messageQueue.empty()) {
					ThreadMessage* msg = messageQueue.front();
					messageQueue.pop_front();
					msg->execute(this);
					delete msg;
				}
			}

			if (time_counter >= nextSendFrame || time_counter >= nextClientFrameI)
				break;

			if (nextSendFrame - time_counter > 1000 || nextClientFrameI - time_counter > 1000)	// time_counter gone around
				nextClientFrameF = nextClientFrameI = nextSendFrame = time_counter;

			//sleep a bit
			MS_SLEEP(2);
		}

		if (time_counter >= nextSendFrame) {
			nextSendFrame += sendFrameStep;
			#ifdef SEND_FRAMEOFFSET
			nextSendFrame += netsendAdjustment;
			netsendAdjustment = 0;	// losing a value due to concurrency is vaguely possible but affordable
			#endif
			if (time_counter > nextSendFrame)	// don't accumulate lag
				nextSendFrame = time_counter;
			send_frame(true);
		}

		if (time_counter < nextClientFrameI)
			continue;

		// the rest is drawing

		nextClientFrameF += 200. / menu.options.graphics.fpsLimit();
		nextClientFrameI = static_cast<int>(nextClientFrameF);
		if (time_counter > nextClientFrameI)	// don't accumulate lag
			nextClientFrameF = nextClientFrameI = time_counter;

		if (gameshow) {
			MutexLock ml(frameMutex);

			ClientPhysicsCallbacks cb(*this);
			if (menu.options.game.lagPrediction()) {
				const float lagWanted = 2. * (1. - menu.options.game.lagPredictionAmount() / 10.);	// lagPredictionAmount() is in range [0, 10]
				float timeDelta = max<float>(0., averageLag - lagWanted) + (get_time() - frameReceiveTime) * 10.;
				NLubyte firstFrame, lastFrame;
				if (clFrameSent == clFrameWorld)
					firstFrame = lastFrame = clFrameWorld;
				else {
					firstFrame = lastFrame = clFrameWorld + 1;
					while (lastFrame != clFrameSent && timeDelta > 1.) {
						++lastFrame;
						timeDelta -= 1.;
					}
				}
				if (timeDelta > 3.)
					timeDelta = 3.;
				fd.extrapolate(fx, cb, me, controlHistory, firstFrame, lastFrame, timeDelta);
			}
			else
				fd.extrapolate(fx, cb, me, controlHistory, clFrameWorld, clFrameWorld, (get_time() - frameReceiveTime) * 10.);

			if (mapChanged) {
				mapChanged = false;	// once in a million years this will overwrite a true value; this will be fixed when the thread model is reworked
				client_graphics.update_minimap_background(fx.map);
				predrawNeeded = true;
			}
			if (predrawNeeded) {
				predrawNeeded = false;	// once in a million years this will overwrite a true value; this will be fixed when the thread model is reworked
				predraw();
			}

			client_graphics.startDraw();
			draw_game_frame();

			#ifdef ROOM_CHANGE_BENCHMARK
			if (benchmarkRuns >= 500)
				quitCommand = true;
			#endif
		} else {
			client_graphics.startDraw();
			client_graphics.clear();
		}

		const int errors = errorLog.size();
		if (errors) {
			for (int count = 0; count < errors; ++count)
				m_errors.wrapLine(errorLog.pop());
			if (openMenus.safeTop() != &m_errors.menu)
				showMenu(m_errors);
		}

		if (menusel != menu_none || !openMenus.empty())
			draw_game_menu();

		client_graphics.endDraw();
		client_graphics.draw_screen();
		if (screenshot) {
			save_screenshot();
			screenshot = false;
		}
	}

	//client exit cleanup: done at stop wich needs to be called after loop
}

void Client::stop() {
	log("Client exiting: stop() called");

	abortThreads = true;

	//at least disconnect
	disconnect_command();

	tournamentPassword.stop();

	//save configuration file
	string fileName = wheregamedir + "config" + directory_separator + "client.cfg";
	log("Saving client configuration in %s", fileName.c_str());
	ofstream cfg(fileName.c_str());
	if (cfg) {
		// save name menu settings
		cfg << CCS_PlayerName			<< ' ' << playername << '\n';
		cfg << CCS_Tournament			<< ' ' << (menu.options.name.tournament() ? 1 : 0) << '\n';

		// save connect menu settings
		cfg << CCS_Favorites			<< ' ' << (menu.connect.favorites() ? 1 : 0) << '\n';

		// save game menu settings
		cfg << CCS_ShowNames			<< ' ' << (menu.options.game.showNames() ? 1 : 0) << '\n';
		{	// favorite colors
			cfg << CCS_FavoriteColors	<< ' ';
			if (menu.options.game.favoriteColors.values().empty())
				cfg << -1;
			else {
				const vector<int>& colVec = menu.options.game.favoriteColors.values();
				for (vector<int>::const_iterator col = colVec.begin(); col != colVec.end(); ++col)
					cfg << *col << ' ';
			}
			cfg << '\n';
		}
		cfg << CCS_LagPrediction		<< ' ' << (menu.options.game.lagPrediction() ? 1 : 0) << '\n';
		cfg << CCS_LagPredictionAmount	<< ' ' <<  menu.options.game.lagPredictionAmount() << '\n';
		cfg << CCS_MessageLogging		<< ' ' << (menu.options.game.messageLogging() ? 1 : 0) << '\n';
		cfg << CCS_SaveStats			<< ' ' << (menu.options.game.saveStats() ? 1 : 0) << '\n';
		cfg << CCS_ShowStats			<< ' ' << (menu.options.game.showStats() ? 1 : 0) << '\n';
		cfg << CCS_AutoGetServerList	<< ' ' << (menu.options.game.autoGetServerList() ? 1 : 0) << '\n';
		cfg << CCS_ShowServerInfo		<< ' ' << (menu.options.game.showServerInfo() ? 1 : 0) << '\n';

		// save controls menu settings
		cfg << CCS_KeypadMoving			<< ' ' << (menu.options.controls.keypadMoving() ? 1 : 0) << '\n';
		cfg << CCS_Joystick				<< ' ' << (menu.options.controls.joystick() ? 1 : 0) << '\n';
		cfg << CCS_JoystickMove			<< ' ' << menu.options.controls.joyMove() << '\n';
		cfg << CCS_JoystickShoot		<< ' ' << menu.options.controls.joyShoot() << '\n';
		cfg << CCS_JoystickRun			<< ' ' << menu.options.controls.joyRun() << '\n';
		cfg << CCS_JoystickStrafe		<< ' ' << menu.options.controls.joyStrafe() << '\n';

		// save graphics menu settings
		cfg << CCS_Windowed				<< ' ' << (menu.options.graphics.windowed() ? 1 : 0) << '\n';
		ScreenMode mode = menu.options.graphics.resolution();
		cfg << CCS_GFXMode				<< ' ' << mode.width << ' ' << mode.height << ' ' << menu.options.graphics.colorDepth() << '\n';
		cfg << CCS_Flipping				<< ' ' << (menu.options.graphics.flipping() ? 1 : 0) << '\n';
		cfg << CCS_FPSLimit				<< ' ' << menu.options.graphics.fpsLimit() << '\n';
		cfg << CCS_GFXTheme				<< ' ' << menu.options.graphics.theme() << '\n';
		cfg << CCS_Antialiasing			<< ' ' << static_cast<int>(menu.options.graphics.antialiasing()) << '\n';
		cfg << CCS_StatsBgAlpha			<< ' ' << menu.options.graphics.statsBgAlpha() << '\n';

		// save sound menu settings
		cfg << CCS_SoundEnabled			<< ' ' << (menu.options.sounds.enabled() ? 1 : 0) << '\n';
		cfg << CCS_Volume				<< ' ' << menu.options.sounds.volume() << '\n';
		cfg << CCS_SoundTheme			<< ' ' << menu.options.sounds.theme() << '\n';

		cfg.close();
	}
	else
		log("Can't open %s for writing", fileName.c_str());
	log("Client configuration saved");
	fileName = wheregamedir + "config" + directory_separator + "favorites.txt";
	ofstream fav(fileName.c_str());
	if (fav) {
		for (vector<ServerListEntry>::const_iterator spy = gamespy.begin(); spy != gamespy.end(); ++spy)
			fav << spy->addressString() << '\n';
		fav.close();
	}

	//save client's password
	log("Saving password file...");
	fileName = wheregamedir + "config" + directory_separator + "password.bin";
	FILE *psf = fopen(fileName.c_str(), "wb");
	if (psf) {
		const string& password = menu.options.name.password();
		for (int c = 0; c < PASSBUFFER; c++) {
			if (c < (int)password.length())
				fputc(static_cast<unsigned char>(255 - password[c]), psf);
			else
				fputc(255, psf);	// 255 = 0 toggled (important)
		}
		fclose(psf);
	}
	else
		log.error("Can't open %s for writing", fileName.c_str());

	{
		MutexLock ml(downloadMutex);
		downloads.clear();
	}

	if (menu.options.game.messageLogging())
		message_log.close();

	if (listenServer.running())
		listenServer.stop();

	log("Client stop() completed");
}

void Client::rocketHitWallCallback(int rid, bool power, float x, float y, int roomx, int roomy) {
	if (power) {
		client_graphics.create_quadwallexplo(static_cast<int>(x), static_cast<int>(y), roomx, roomy);
		client_sounds.play(SAMPLE_QUADWALLHIT);
	}
	else {
		client_graphics.create_wallexplo(static_cast<int>(x), static_cast<int>(y), roomx, roomy);
		client_sounds.play(SAMPLE_WALLHIT);
	}
	fd.rock[rid].owner = fx.rock[rid].owner = -1;	// erase from clientside simulation
}

void Client::rocketOutOfBoundsCallback(int rid) {
	fd.rock[rid].owner = fx.rock[rid].owner = -1;	// erase from clientside simulation
}

void Client::playerHitWallCallback(int pid) {
	// play bounce sample if minimum time elapsed
	const float currTime = get_time();	//#fix
	if (currTime > fx.player[pid].wall_sound_time) {
		fx.player[pid].wall_sound_time = currTime + 0.2;
		client_sounds.play(SAMPLE_WALLBOUNCE);
	}
}

void Client::playerHitPlayerCallback(int pid1, int pid2) {
	// play bounce sample if minimum time elapsed
	const float currTime = get_time();	//#fix
	if (currTime > fx.player[pid1].player_sound_time || currTime > fx.player[pid2].player_sound_time) {
		fx.player[pid1].player_sound_time = fx.player[pid2].player_sound_time = currTime + 0.2;
		client_sounds.play(SAMPLE_PLAYERBOUNCE);
	}
}

bool Client::shouldApplyPhysicsToPlayerCallback(int pid) {
	return fx.player[pid].onscreen && !fx.player[pid].dead;
}

void Client::predraw() {
	if (fx.player[me].roomx < 0 || fx.player[me].roomx >= fx.map.w ||
			fx.player[me].roomy < 0 || fx.player[me].roomy >= fx.map.h)
		return;	//#fix: this shouldn't be needed, or should be checked from a simple flag
	vector< pair<int, const WorldCoords*> > flags;
	vector< pair<int, const WorldCoords*> > spawns;

	for (int team = 0; team <= 2; team++) {
		const vector<WorldCoords>& tflags = (team == 2 ? fx.map.wild_flags : fx.map.tinfo[team].flags);
		for (vector<WorldCoords>::const_iterator pi = tflags.begin(); pi != tflags.end(); ++pi)		// flags
			if (fx.player[me].roomx == pi->px && fx.player[me].roomy == pi->py)
				flags.push_back(pair<int, const WorldCoords*>(team, &(*pi)));
		if (menu.options.graphics.mapInfoMode() && team < 2) {
			const vector<WorldCoords>& tspawn = fx.map.tinfo[team].spawn;
			for (vector<WorldCoords>::const_iterator pi = tspawn.begin(); pi != tspawn.end(); ++pi)	// spawns
				if (fx.player[me].roomx == pi->px && fx.player[me].roomy == pi->py)
					spawns.push_back(pair<int, const WorldCoords*>(team, &(*pi)));
		}
	}

	client_graphics.predraw(fx.map.room[fx.player[me].roomx][fx.player[me].roomy], flags, spawns, menu.options.graphics.mapInfoMode());
}

//draw the whole game screen
void Client::draw_game_frame() {
	// hide stuff if frame skipped
	const bool hide_game = !map_ready || gameover_plaque != NEXTMAP_NONE || fx.skipped || me < 0 || me >= maxplayers;

	// the playground: border, walls and pits
	if (hide_game) {
		client_graphics.draw_empty_background();

		// game over message
		if (gameover_plaque == NEXTMAP_CAPTURE_LIMIT || gameover_plaque == NEXTMAP_VOTE_EXIT) {
			if (red_final_score > blue_final_score)
				client_graphics.draw_scores("RED TEAM WINS", 0, red_final_score, blue_final_score);
			else if (blue_final_score > red_final_score)
				client_graphics.draw_scores("BLUE TEAM WINS", 1, blue_final_score, red_final_score);
			else
				client_graphics.draw_scores("GAME TIED", -1, blue_final_score, red_final_score);
		}
		else
			client_graphics.draw_one_line_message("Connecting...");	//#fix: if the map download is pending, "Connecting..." is a wrong word

		if (map_ready)
			client_graphics.draw_waiting_map_message("Waiting game start - next map is:", fx.map.title);
		else {
			MutexLock ml(downloadMutex);
			if (!downloads.empty() && downloads.front().isActive()) {
				ostringstream text;
				text << "Loading map: " << downloads.front().progress() << " bytes";
				client_graphics.draw_loading_map_message(text.str());
			}
		}
	}
	else {
		#ifdef ROOM_CHANGE_BENCHMARK
		predraw();
		++benchmarkRuns;
		#endif
		client_graphics.draw_background();
	}
	// frame is valid?
	if (!hide_game && fd.frame >= 0) {
		// draw dead players, except ice creams
		for (int i = 0; i < maxplayers; i++) {
			if (fx.player[i].used && fx.player[i].onscreen && fx.player[i].dead) {
				if (fx.player[i].stats().frags() % 10 == 0 && fx.player[i].stats().frags() >= 10)
					;	// draw later
				else
					client_graphics.draw_player_dead(fx.player[i]);
			}
		}

		// draw any item pickups
		if (me >= 0)
			for (int i = 0; i < MAX_PICKUPS; i++)
				// used power-ups, not respawning, on my screen
				if (fx.item[i].kind != Powerup::pup_unused && fx.item[i].kind != Powerup::pup_respawning &&
						fx.item[i].px == fx.player[me].roomx && fx.item[i].py == fx.player[me].roomy) {
					client_graphics.draw_pup(fx.item[i], get_time());
					if (fx.item[i].kind == Powerup::pup_deathbringer)
						client_graphics.create_smoke(fx.item[i].x + rand() % 30 - 15, fx.item[i].y + rand() % 30 - 5,
							fx.item[i].px, fx.item[i].py);
				}

		// draw speed effect
		client_graphics.draw_speedfx(fx.player[me].roomx, fx.player[me].roomy, get_time());

		// draw any dropped flags (use fx since flags don't move)
		for (int t = 0; t < 2; t++)
			for (vector<Flag>::const_iterator fi = fx.teams[t].flags().begin(); fi != fx.teams[t].flags().end(); ++fi)
				// not carried, on same screen
				if (!fi->carried() && fi->position().px == fx.player[me].roomx && fi->position().py == fx.player[me].roomy)
					client_graphics.draw_flag(t, fi->position().x, fi->position().y);

		for (vector<Flag>::const_iterator fi = fx.wild_flags.begin(); fi != fx.wild_flags.end(); ++fi)
			if (!fi->carried() && fi->position().px == fx.player[me].roomx && fi->position().py == fx.player[me].roomy)
				client_graphics.draw_flag(2, fi->position().x, fi->position().y);

		// draw any rockets
		for (int i = 0; i < MAX_ROCKETS; i++)
			if (fx.rock[i].owner != -1 && fx.rock[i].px == fx.player[me].roomx && fx.rock[i].py == fx.player[me].roomy) {
				fd.rock[i].team = fx.rock[i].team;
				fd.rock[i].power = fx.rock[i].power;
				client_graphics.draw_rocket(fd.rock[i], get_time());
			}

		// the PLAY AREA: the players!
		for (int k = 0; k < maxplayers; k++) {
			const int i = (me / TSIZE == 0 ? k : maxplayers - k - 1);	// own team first

			//HACK REMENDEX: predict item_helm
			if (fd.player[i].item_helm()) {
				const int hspd = static_cast<int>((fd.frame - fx.frame) * 10.);
				fd.player[i].visibility = fx.player[i].visibility - hspd;
				if (fd.player[i].visibility < 0)
					fd.player[i].visibility = 0;
			}

			if (fx.player[i].onscreen && i != me)	// draw only players on my screen
				draw_player(i);
			if (k == maxplayers - 1)				// last draw me
				draw_player(me);
		}

		for (int i = 0; i < maxplayers; i++)
			if (fx.player[i].used && fx.player[i].roomx == fx.player[me].roomx && fx.player[i].roomy == fx.player[me].roomy &&
				fx.player[i].onscreen && fx.player[i].item_deathbringer)
					client_graphics.draw_deathbringer_carrier_effect((int)fd.player[i].lx, (int)fd.player[i].ly);

		client_graphics.draw_effects(fx.player[me].roomx, fx.player[me].roomy, get_time());

		// Draw player names but not for invisible enemies.
		for (int i = 0; i < maxplayers; i++)
			if (menu.options.game.showNames() && fx.player[i].used && !(fx.player[i].visibility < 10 && i / TSIZE != me / TSIZE) &&
					fx.player[i].roomx == fx.player[me].roomx && fx.player[i].roomy == fx.player[me].roomy && fx.player[i].onscreen && !fx.player[i].dead) {
				const int ttx = static_cast<int>(fd.player[i].lx);
				const int tty = static_cast<int>(fd.player[i].ly);
				client_graphics.draw_player_name(fx.player[i].name, ttx, tty, i / TSIZE);
			}
	}

	//do not draw stuff below if map not ready to show
	if (!hide_game) {
		// the MINIMAP
		//client_graphics.draw_minimap_background();

		//draw the miniflags
		// - qualquer flag no chao (na base ou nao, carried == false)
		for (int t = 0; t < 2; t++)
			for (vector<Flag>::const_iterator fi = fx.teams[t].flags().begin(); fi != fx.teams[t].flags().end(); ++fi)
				if (!fi->carried())
					client_graphics.draw_mini_flag(t, *fi, fx.map);

		for (vector<Flag>::const_iterator fi = fx.wild_flags.begin(); fi != fx.wild_flags.end(); ++fi)
			if (!fi->carried())
				client_graphics.draw_mini_flag(2, *fi, fx.map);

		vector<bool> roomvis(fx.map.w * fx.map.h, (me >= 0 && fx.player[me].item_helm()) ? true : false);

		// draw all teammates and enemies on screens where there are teammates
		//draw all the players - put a pixel where they are
		if (me >= 0 && fx.frame >= 0)
			for (int i = 0; i < maxplayers; i++)
				if (fx.player[i].used && fx.player[i].roomx >= 0 && fx.player[i].roomy >= 0 && fx.player[i].roomx < fx.map.w && fx.player[i].roomy < fx.map.h &&
						(i / TSIZE == me / TSIZE || (fx.player[me].enemyvis & (1 << (i % TSIZE))))) {
					roomvis[fx.player[i].roomy * fx.map.w + fx.player[i].roomx] = true;

					//verifica se o jogador a ser desenhado é um carrier de flag inimiga
					const int enemy = 1 - i / TSIZE;
					int f = 0;
					for (vector<Flag>::const_iterator fi = fx.teams[enemy].flags().begin(); fi != fx.teams[enemy].flags().end(); ++fi, ++f)
						if (fi->carrier() == i) {
							// update flag position for draw
							fx.teams[enemy].move_flag(f, WorldCoords(fx.player[i].roomx, fx.player[i].roomy,
								static_cast<int>(fx.player[i].lx), static_cast<int>(fx.player[i].ly)));

							// draw the miniflag here
							client_graphics.draw_mini_flag(enemy, *fi, fx.map);
						}

					for (vector<Flag>::iterator fi = fx.wild_flags.begin(); fi != fx.wild_flags.end(); ++fi)
						if (fi->carrier() == i) {
							// update flag position for draw
							fi->move(WorldCoords(fx.player[i].roomx, fx.player[i].roomy,
								static_cast<int>(fx.player[i].lx), static_cast<int>(fx.player[i].ly)));

							// draw the miniflag here
							client_graphics.draw_mini_flag(2, *fi, fx.map);
						}

					if (i != me) {
						if (fx.player[i].color() >= 0 && fx.player[i].color() < MAX_PLAYERS / 2)	// Check because the server may have sent invalid colour.
							client_graphics.draw_minimap_player(fx.map, fx.player[i]);
					}
					else // myself: draw differently
						client_graphics.draw_minimap_me(fx.map, fx.player[i], get_time());
				}

		// paint fog of war in all invisible rooms
		for (int ry = 0; ry < fx.map.h; ry++)
			for (int rx = 0; rx < fx.map.w; rx++)
				if (!roomvis[ry * fx.map.w + rx])
					client_graphics.draw_minimap_room(fx.map, rx, ry);
	}//!hide_game

	client_graphics.draw_scoreboard(players_sb, fx.teams, maxplayers);

	client_graphics.draw_fps(FPS);

	// Time left if time limit is on and the game is running.
	if (map_time_limit && gameover_plaque == NEXTMAP_NONE)
		if (map_end_time > get_time())
			client_graphics.map_time(map_end_time - static_cast<int>(get_time()));
		else
			client_graphics.map_time(0);

	// player's power-ups
	if (me >= 0) {
		if (fx.player[me].item_quad) {
			double val = fx.player[me].item_quad_time - get_time();
			if (val < 0) val = 0;
			client_graphics.draw_player_power(val);
		}
		if (fx.player[me].item_speed) {
			double val = fx.player[me].item_speed_time - get_time();
			if (val < 0) val = 0;
			client_graphics.draw_player_turbo(val);
		}
		if (fx.player[me].item_helm()) {
			double val = fx.player[me].item_helm_time - get_time();
			if (val < 0) val = 0;
			client_graphics.draw_player_shadow(val);
		}

		client_graphics.draw_player_weapon(fx.player[me].weapon + 1);	// weapon level
	}

	if (want_change_teams)
		client_graphics.draw_change_team_message(get_time());
	if (want_map_exit)
		client_graphics.draw_change_map_message(get_time());

	// the STATUSBAR : health energy, bars ....
	if (me >= 0) {
		client_graphics.draw_player_health(fx.player[me].health);
		client_graphics.draw_player_energy(fx.player[me].energy);
	}

	// the HUD: message output
	const int chat_visible = show_all_messages ? client_graphics.chat_max_lines() : client_graphics.chat_lines();
	int start = static_cast<int>(chatbuffer.size()) - static_cast<int>(chat_visible);
	if (start < 0)
		start = 0;
	list<Message>::const_iterator msg = chatbuffer.begin();
	for (int i = 0; i < start; ++i, ++msg);
	if (!show_all_messages)	// drop old messages
		for (; msg != chatbuffer.end(); ++msg)
			if (get_time() < msg->time() + 80)
				break;
	client_graphics.print_chat_messages(msg, chatbuffer.end(), talkbuffer);

	//"server not responding... connection may have dropped" plaque
	if (get_time() > lastpackettime + 1.0)
		client_graphics.show_not_responding_message();

	// debug panel
	if (key[KEY_F9]) {
		const int bpsin = client->get_socket_stat(NL_AVE_BYTES_RECEIVED);
		const int bpsout = client->get_socket_stat(NL_AVE_BYTES_SENT);

		vector<vector<pair<int, int> > > sticks;
		vector<int> buttons;
		if (menu.options.controls.joystick()) {
			const JOYSTICK_INFO& joystick = joy[0];
			for (int i = 0; i < joystick.num_sticks; i++) {
				vector<pair<int, int> > axes;
				for (int j = 0; j < joystick.stick[i].num_axis; j++)
					axes.push_back(pair<int, int>(joystick.stick[i].axis[j].d1, joystick.stick[i].axis[j].d2));
				sticks.push_back(axes);
			}
			for (int i = 0; i < joystick.num_buttons; i++)
				buttons.push_back(joystick.button[i].b);
		}

		client_graphics.debug_panel(fx.player, me, bpsin, bpsout, sticks, buttons);
	}

	// another frame, calculate FPS
	totalframecount++;
	framecount++;
	const double baixo = get_time() - frameCountStartTime;
	if (baixo > 1.0) {
		FPS = ((double)framecount) / baixo;
		frameCountStartTime = get_time();
		framecount = 0;
	}
}

void Client::draw_player(int pid) {
	ClientPlayer& player = fx.player[pid];
	int alpha = fd.player[pid].visibility;
	const int min_alpha_friends = 128;
	if (player.team() == fx.player[me].team() && alpha < min_alpha_friends)
		alpha = min_alpha_friends;
	else if (alpha < 7)
		alpha = 7;
	// draw flag if player is carrier of a flag
	for (int t = 0; t < 2; t++)
		for (vector<Flag>::const_iterator fi = fx.teams[t].flags().begin(); fi != fx.teams[t].flags().end(); ++fi)
			if (fi->carrier() == pid)
				client_graphics.draw_flag(t, static_cast<int>(fd.player[pid].lx), static_cast<int>(fd.player[pid].ly));
	for (vector<Flag>::const_iterator fi = fx.wild_flags.begin(); fi != fx.wild_flags.end(); ++fi)
		if (fi->carrier() == pid)
			client_graphics.draw_flag(2, static_cast<int>(fd.player[pid].lx), static_cast<int>(fd.player[pid].ly));
	if (player.dead) {	// draw only ice creams
		if (player.stats().frags() % 10 == 0 && player.stats().frags() >= 10)
			client_graphics.draw_virou_sorvete(static_cast<int>(player.lx), static_cast<int>(player.ly));
	}
	else {
		if (player.color() >= 0 && player.color() < MAX_PLAYERS / 2) {	// Check because the server may have sent invalid colour.
			// turbo effect
			if (player.item_speed && player.sx * player.sx + player.sy * player.sy > fx.physics.max_run_speed * fx.physics.max_run_speed &&
						get_time() > player.speed_drop_time) {
				player.speed_drop_time = get_time() + 0.05;
					client_graphics.create_speedfx(static_cast<int>(fd.player[pid].lx), static_cast<int>(fd.player[pid].ly), player.roomx, player.roomy, player.team(), player.color(), player.gundir);
			}

			//draw player
			client_graphics.draw_player(static_cast<int>(fd.player[pid].lx), static_cast<int>(fd.player[pid].ly), player.team(), player.color(), player.gundir, player.hitfx, player.item_quad, alpha, get_time());
		}

		//draw deathbringer carrier effect
		if (player.item_deathbringer && get_time() > player.death_drop_time) {
			player.death_drop_time = get_time() + 0.01;
			for (int i = 0; i < 2; i++)
				client_graphics.create_deathcarrier(static_cast<int>(fd.player[pid].lx) + rand() % 40 - 20, static_cast<int>(fd.player[pid].ly) + rand() % 40, player.roomx, player.roomy);
		}
		// draw deathbringer affected effect
		if (player.deathbringer_affected)
			client_graphics.draw_deathbringer_affected(static_cast<int>(fd.player[pid].lx), static_cast<int>(fd.player[pid].ly), player.team());
		// shield
		if (player.item_shield)
			client_graphics.draw_shield(static_cast<int>(fd.player[pid].lx), static_cast<int>(fd.player[pid].ly), PLAYER_RADIUS + SHIELD_RADIUS_ADD, alpha, player.team(), player.gundir);
	}
}

//draws the game menu
void Client::draw_game_menu() {
	switch (menusel) {
		case menu_maps: {
			MutexLock ml(mapInfoMutex);
			client_graphics.map_list(maps, current_map, map_vote, edit_map_vote);
			break;
		}
		case menu_players:
			client_graphics.draw_statistics(players_sb, player_stats_page, static_cast<int>(get_time()), maxplayers, max_world_rank);
			break;
		case menu_teams:
			client_graphics.team_statistics(fx.teams);
			break;
		case menu_none:
			if (!openMenus.empty())
				openMenus.draw(client_graphics.drawbuffer());
			break;
		default:
			numAssert(0, menusel);
	}
}

void Client::initMenus() {
	typedef MenuCallback<Client> MCB;

	menu.recursiveSetMenuOpener					(new MCB::A<Menu,			&Client::MCF_menuOpener				>(this));

	menu.menu						.setDrawHook(new MCB::N<Menu,			&Client::MCF_prepareMainMenu		>(this));

	menu.disconnect						.setHook(new MCB::N<Textarea,		&Client::MCF_disconnect				>(this));
	menu.startServer					.setHook(new MCB::N<Textarea,		&Client::MCF_startServer			>(this));
	menu.playServer						.setHook(new MCB::N<Textarea,		&Client::MCF_playServer				>(this));
	menu.stopServer						.setHook(new MCB::N<Textarea,		&Client::MCF_stopServer				>(this));
	menu.exitOutgun						.setHook(new MCB::N<Textarea,		&Client::MCF_exitOutgun				>(this));

	menu.connect.menu				.setOpenHook(new MCB::N<Menu,			&Client::MCF_prepareServerMenu		>(this));
	menu.connect.menu				.setDrawHook(new MCB::N<Menu,			&Client::MCF_prepareServerMenu		>(this));	//#fix: inefficient!
	menu.connect.favorites				.setHook(new MCB::N<Checkbox,		&Client::MCF_prepareServerMenu		>(this));
	menu.connect.update					.setHook(new MCB::N<Textarea,		&Client::MCF_updateServers			>(this));
	menu.connect.refresh				.setHook(new MCB::N<Textarea,		&Client::MCF_refreshServers			>(this));

	menu.connect.addServer.menu		.setOpenHook(new MCB::N<Menu,			&Client::MCF_prepareAddServer		>(this));
	menu.connect.addServer.menu		  .setOkHook(new MCB::N<Menu,			&Client::MCF_addServer				>(this));

	menu.options.name.menu			.setOpenHook(new MCB::N<Menu,			&Client::MCF_prepareNameMenu		>(this));
	menu.options.name.menu			.setDrawHook(new MCB::N<Menu,			&Client::MCF_prepareDrawNameMenu	>(this));
	menu.options.name.menu		   .setCloseHook(new MCB::N<Menu,			&Client::MCF_nameMenuClose			>(this));
	menu.options.name.name				.setHook(new MCB::N<Textfield,		&Client::MCF_nameChange				>(this));
	menu.options.name.randomName		.setHook(new MCB::N<Textarea,		&Client::MCF_randomName				>(this));
	menu.options.name.removePasswords	.setHook(new MCB::N<Textarea,		&Client::MCF_removePasswords		>(this));

	menu.options.game.menu			.setOpenHook(new MCB::N<Menu,			&Client::MCF_prepareGameMenu		>(this));
	menu.options.game.messageLogging	.setHook(new MCB::N<Checkbox,		&Client::MCF_messageLogging			>(this));

	menu.options.controls.joystick		.setHook(new MCB::N<Checkbox,		&Client::MCF_joystick				>(this));

	menu.options.graphics.menu		.setOpenHook(new MCB::N<Menu,			&Client::MCF_prepareGfxMenu			>(this));
	menu.options.graphics.menu		.setDrawHook(new MCB::N<Menu,			&Client::MCF_prepareDrawGfxMenu		>(this));
	menu.options.graphics.menu	   .setCloseHook(new MCB::N<Menu,			&Client::MCF_screenModeChange		>(this));
	menu.options.graphics.menu		  .setOkHook(new MCB::N<Menu,			&Client::MCF_screenModeChange		>(this));
	menu.options.graphics.colorDepth	.setHook(new MCB::N<Select<int>,	&Client::MCF_screenDepthChange		>(this));
	menu.options.graphics.apply			.setHook(new MCB::N<Textarea,		&Client::MCF_screenModeChange		>(this));
	menu.options.graphics.theme			.setHook(new MCB::N<Select<string>,	&Client::MCF_gfxThemeChange			>(this));
	typedef Select<Graphics::Antialiasing_mode> aaSelT;
	menu.options.graphics.antialiasing	.setHook(new MCB::N<aaSelT,			&Client::MCF_antialiasChange		>(this));
	menu.options.graphics.statsBgAlpha	.setHook(new MCB::N<Slider,			&Client::MCF_statsBgChange			>(this));
	menu.options.graphics.mapInfoMode	.setHook(new MCB::N<Checkbox,		&Client::predraw					>(this));

	menu.options.sounds.menu		.setOpenHook(new MCB::N<Menu,			&Client::MCF_prepareSndMenu			>(this));
	menu.options.sounds.enabled			.setHook(new MCB::N<Checkbox,		&Client::MCF_sndEnableChange		>(this));
	menu.options.sounds.volume			.setHook(new MCB::N<Slider,			&Client::MCF_sndVolumeChange		>(this));
	menu.options.sounds.theme			.setHook(new MCB::N<Select<string>,	&Client::MCF_sndThemeChange			>(this));

	m_playerPassword.menu			  .setOkHook(new MCB::N<Menu,			&Client::MCF_playerPasswordAccept	>(this));
	m_serverPassword.menu			  .setOkHook(new MCB::N<Menu,			&Client::MCF_serverPasswordAccept	>(this));
	m_connectProgress.accept			.setHook(new MCB::N<Textarea,		&Client::MCF_menuCloser				>(this));
	m_connectProgress.cancel			.setHook(new MCB::N<Textarea,		&Client::MCF_menuCloser				>(this));
	m_connectProgress.menu		   .setCloseHook(new MCB::N<Menu,			&Client::MCF_cancelConnect			>(this));
	m_dialog.accept						.setHook(new MCB::N<Textarea,		&Client::MCF_menuCloser				>(this));	// cancel not used
	m_errors.accept						.setHook(new MCB::N<Textarea,		&Client::MCF_clearErrors			>(this));	// cancel not used
	m_serverInfo.accept					.setHook(new MCB::N<Textarea,		&Client::MCF_menuCloser				>(this));	// cancel not used

	m_errors.menu.setCaption("Errors");

	MCF_loadHelp();

	menu.options.graphics.init(client_graphics);
	menu.options.sounds.init(client_sounds);
}

void Client::MCF_menuOpener(Menu& menu) {
	openMenus.open(&menu);
}

void Client::MCF_menuCloser() {
	openMenus.close();
	if (!gameshow && openMenus.empty())
		showMenu(menu);
}

void Client::MCF_prepareMainMenu() {
	if (listenServer.running()) {
		menu.startServer.setEnable(false);
		menu.playServer.setEnable(!connected);
		menu.stopServer.setEnable(true);
	}
	else {
		menu.startServer.setEnable(true);
		menu.playServer.setEnable(false);
		menu.stopServer.setEnable(false);
	}
	menu.disconnect.setEnable(connected);
}

void Client::MCF_disconnect() {
	disconnect_command();
}

void Client::MCF_startServer() {
	if (!listenServer.running())
		listenServer.start(serverExtConfig.port, serverExtConfig);
}

void Client::MCF_playServer() {
	if (listenServer.running()) {
		nAssert(nlStringToAddr("127.0.0.1", &serverIP));
		nAssert(nlSetAddrPort(&serverIP, listenServer.port()));
		openMenus.clear();
		connect_command(true);
	}
}

void Client::MCF_stopServer() {
	if (listenServer.running())
		listenServer.stop();
}

void Client::MCF_exitOutgun() {
	quitCommand = true;
}

void Client::MCF_cancelConnect() {
	if (!connected)
		disconnect_command();	// will cancel the (probably) ongoing connect attempt
}

void Client::MCF_prepareNameMenu() {
	menu.options.name.name.set(playername);
}

void Client::MCF_prepareDrawNameMenu() {
	menu.options.name.namestatus.set(tournamentPassword.statusAsString());
}

void Client::MCF_nameMenuClose() {
	change_name_command();
	send_tournament_participation();
}

void Client::MCF_nameChange() {	// only function to clear the password
	menu.options.name.password.set("");
	tournamentPassword.changeData(playername, "");
}

void Client::MCF_randomName() {
	menu.options.name.name.set(RandomName());
	MCF_nameChange();
}

void Client::MCF_removePasswords() {
	const int removed = remove_player_passwords(menu.options.name.name());
	ostringstream dialog;
	if (removed > 0)
		dialog << removed << " password" << (removed > 1 ? "s" : "") << " removed.";
	else
		dialog << "No passwords found.";
	m_dialog.clear();
	m_dialog.addLine(dialog.str());
	showMenu(m_dialog);
}

void Client::MCF_prepareGameMenu() {
	menu.options.game.favoriteColors.setGraphicsCallBack(client_graphics);
}

void Client::MCF_joystick() {
	if (menu.options.controls.joystick())
		install_joystick(JOY_TYPE_AUTODETECT);
	else
		remove_joystick();
}

void Client::MCF_messageLogging() {
	if (menu.options.game.messageLogging()) {
		message_log.clear();	// necessary: http://gcc.gnu.org/onlinedocs/libstdc++/faq/index.html#4_4_iostreamclear
		message_log.open((wheregamedir + "log" + directory_separator + "message.log").c_str(), ios::app);
	}
	else
		message_log.close();
}

void Client::MCF_prepareGfxMenu() {
	menu.options.graphics.update(client_graphics);
}

void Client::MCF_prepareDrawGfxMenu() {
	menu.options.graphics.flipping.setEnable(!menu.options.graphics.windowed());
}

void Client::MCF_gfxThemeChange() {
	client_graphics.select_theme(menu.options.graphics.theme());
	predrawNeeded = true;
}

void Client::MCF_screenDepthChange() {
	menu.options.graphics.update(client_graphics);	// fetch resolutions according to the new depth
}

void Client::MCF_screenModeChange() {	// used to lose the return value
	nAssert(screenModeChange());	// it should return true unless it's out of memory, because this function is only used when there is a working mode to revert to
}

bool Client::screenModeChange() {	// returns true whenever Graphics is usable (even when reverted back to current (workingGfxMode) mode)
	if (!menu.options.graphics.newMode())
		return true;

	const ScreenMode res = menu.options.graphics.resolution();
	const int depth = menu.options.graphics.colorDepth();

	Checkbox& win  = menu.options.graphics.windowed;
	Checkbox& flip = menu.options.graphics.flipping;
	const bool owin = win(), oflip = flip();

	for (int nTry = 0;; ++nTry) {
		if (client_graphics.init(res.width, res.height, depth, win(), flip())) {
			if (nTry != 0)
				log.error("Couldn't initialize resolution %d×%d×%d in %s mode; reverted to %s",
						res.width, res.height, depth,
						owin  ? "windowed" : (oflip  ? "flipped fullscreen" : "backbuffered fullscreen"),
						win() ? "windowed" : (flip() ? "flipped fullscreen" : "backbuffered fullscreen"));
			break;
		}
		switch (nTry) {	// try in order: [switch flip], switch windowed, [switch flip]
			case 0:
				if (!win()) {
					flip.set(!flip());
					break;
				}
				nTry = 1;	// no point in changing flipping when windowed, skip round
			case 1:
				win.set(!win());
				break;
			case 2:
				if (!win()) {
					flip.set(!flip());
					break;
				}
				nTry = 3;	// no point in changing flipping when windowed, skip round
			case 3:
				log.error("Couldn't initialize resolution %d×%d×%d in any mode", res.width, res.height, depth);
				if (workingGfxMode.used()) {	// revert to working mode
					const GFXMode& wm = workingGfxMode;
					nAssert(menu.options.graphics.colorDepth.set(wm.depth));
					menu.options.graphics.update(client_graphics);	// fetch resolutions according to the new depth
					menu.options.graphics.resolution.set(ScreenMode(wm.width, wm.height));	// ignore potential error here; we couldn't do anything about it anyway
					win.set(wm.windowed);
					flip.set(wm.flipping);
					return client_graphics.init(wm.width, wm.height, wm.depth, wm.windowed, wm.flipping);
				}
				return false;
		}
	}
	workingGfxMode = GFXMode(res.width, res.height, depth, win(), flip());
	client_graphics.update_minimap_background(fx.map);
	predrawNeeded = true;
	const int rate = get_refresh_rate();
	ostringstream ost;
	if (rate == 0)
		ost << "unknown";
	else
		ost << rate << " Hz";
	menu.options.graphics.refreshRate.set(ost.str());
	return true;
}

void Client::MCF_antialiasChange() {
	client_graphics.set_antialiasing(menu.options.graphics.antialiasing());
	client_graphics.update_minimap_background(fx.map);
	predrawNeeded = true;
}

void Client::MCF_statsBgChange() {
	client_graphics.set_stats_alpha(menu.options.graphics.statsBgAlpha());
}

void Client::MCF_prepareSndMenu() {
	menu.options.sounds.update(client_sounds);
}

void Client::MCF_sndEnableChange() {
	client_sounds.setEnable(menu.options.sounds.enabled());
}

void Client::MCF_sndVolumeChange() {
	client_sounds.setVolume(menu.options.sounds.volume());
	client_sounds.play(SAMPLE_QUAD_FIRE);
}

void Client::MCF_sndThemeChange() {
	client_sounds.select_theme(menu.options.sounds.theme());
}

void Client::MCF_playerPasswordAccept() {
	nAssert(openMenus.safeTop() == &m_playerPassword.menu);
	openMenus.close();
	if (m_playerPassword.save())
		save_player_password(playername, addressToString(serverIP), m_playerPassword.password());
	if (connected)
		issue_change_name_command();
	else
		connect_command(false);
}

void Client::MCF_serverPasswordAccept() {
	nAssert(openMenus.safeTop() == &m_playerPassword.menu && !connected);
	openMenus.close();
	connect_command(false);
}

void Client::MCF_clearErrors() {
	nAssert(openMenus.safeTop() == &m_errors.menu);
	openMenus.close();
	m_errors.clear();
}

void Client::MCF_prepareServerMenu() {
	const int oldSel = menu.connect.menu.selection();

	menu.connect.reset();
	vector<NLaddress> addresses;
	const vector<ServerListEntry>& servers = (menu.connect.favorites() ? gamespy : mgamespy);
	serverListMutex.lock();
	for (vector<ServerListEntry>::const_iterator spy = servers.begin(); spy != servers.end(); ++spy) {
		ostringstream info;
		info << setw(21) << left << spy->addressString() << right;
		info << setw(4);
		if (spy->ping > 0)
			info << spy->ping;
		else
			info << '?';
		if (spy->refreshed) {
			if (spy->noresponse)
				info << " no response";
			else
				info << ' ' << spy->info;
		}
		menu.connect.add(spy->address(), info.str());
		addresses.push_back(spy->address());
	}
	if (!menu.connect.favorites())
		for (vector<ServerListEntry>::const_iterator spy = gamespy.begin(); spy != gamespy.end(); ++spy)
			if (!spy->noresponse && find(addresses.begin(), addresses.end(), spy->address()) == addresses.end()) {
				ostringstream info;
				info << setw(21) << left << spy->addressString() << right;
				info << setw(4);
				if (spy->ping > 0)
					info << spy->ping;
				else
					info << '?';
				if (spy->refreshed) {
					if (spy->noresponse)
						info << " no response";
					else
						info << ' ' << spy->info;
				}
				menu.connect.add(spy->address(), info.str());
				addresses.push_back(spy->address());
			}
	serverListMutex.unlock();

	typedef MenuCallback<Client> MCB;
	typedef MenuKeyCallback<Client> MKC;
	menu.connect.addHooks(new MCB::A<Textarea, &Client::MCF_connect>(this),
						  new MKC::A<Textarea, &Client::MCF_addRemoveServer>(this));
	const bool refreshActive = (refreshStatus != RS_none && refreshStatus != RS_failed);
	menu.connect.update.setEnable(!menu.connect.favorites() && !refreshActive);
	menu.connect.refresh.setEnable(!refreshActive);
	menu.connect.refreshStatus.set(refreshStatusAsString());

	menu.connect.menu.setSelection(oldSel);
}

void Client::MCF_prepareAddServer() {
	menu.connect.addServer.save.set(menu.connect.favorites());
	menu.connect.addServer.address.set("");
}

void Client::MCF_addServer() {
	if (!menu.connect.addServer.address().empty()) {
		ServerListEntry spy;
		if (!spy.setAddress(menu.connect.addServer.address())) {
			m_dialog.clear();
			m_dialog.addLine("Invalid IP address.");
			showMenu(m_dialog);
			return;
		}
		if (menu.connect.favorites())
			gamespy.push_back(spy);
		else {
			mgamespy.push_back(spy);
			if (menu.connect.addServer.save())
				gamespy.push_back(spy);
		}
		MCF_prepareServerMenu();
	}
	MCF_menuCloser();
}

bool Client::MCF_addRemoveServer(Textarea& target, char scan, unsigned char chr) {
	(void)chr;
	if (scan == KEY_DEL) {
		vector<ServerListEntry>& servers = (menu.connect.favorites() ? gamespy : mgamespy);
		const NLaddress address = menu.connect.getAddress(target);
		for (vector<ServerListEntry>::iterator spy = servers.begin(); spy != servers.end(); ++spy)
			if (nlAddrCompare(&address, &spy->address())) {
				servers.erase(spy);
				break;
			}
		return true;
	}
	else if (scan == KEY_INSERT && !menu.connect.favorites()) {
		const NLaddress address = menu.connect.getAddress(target);
		for (vector<ServerListEntry>::const_iterator spy = mgamespy.begin(); spy != mgamespy.end(); ++spy)
			if (nlAddrCompare(&address, &spy->address())) {
				gamespy.push_back(*spy);
				break;
			}
		return true;
	}
	return false;
}

void Client::MCF_connect(Textarea& target) {
	serverIP = menu.connect.getAddress(target);
	connect_command(true);
}

void Client::MCF_updateServers() {
	if (refreshStatus == RS_none || refreshStatus == RS_failed) {
		refreshStatus = RS_running;
		Thread::startDetachedThread_assert(RedirectToMemFun0<Client, void>(this, &Client::getServerListThread), extConfig.lowerPriority);
	}
}

void Client::MCF_refreshServers() {
	if (refreshStatus == RS_none || refreshStatus == RS_failed) {
		refreshStatus = RS_running;
		Thread::startDetachedThread_assert(RedirectToMemFun0<Client, void>(this, &Client::refreshThread), extConfig.lowerPriority);
	}
}

void Client::MCF_loadHelp() {
	menu.help.clear();
	const string configFile = wheregamedir + "config" + directory_separator + "help.txt";
	ifstream in(configFile.c_str());
	if (!in) {
		menu.help.addLine("No help found. It should be in");
		menu.help.addLine(configFile);
		return;
	}
	string line;
	while (getline_smart(in, line))
		menu.help.addLine(line);
}

void Client::CB_tournamentToken(string token) {	// callback called by tournamentPassword from another thread
	if (connected) {
		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, data_registration_token);
		writeStr(lebuf, count, token);
		client->send_message(lebuf, count);
		tournamentPassword.serverProcessingToken();
	}
}

int Client::cfunc_connection_update(client_runes_t *arg) {
	gameclient->connection_update(arg);
	return 0;
}

void Client::connection_update(client_runes_t *arg) {
	if (arg->connect_result < 3)
		addThreadMessage(new TM_ConnectionUpdate(arg->connect_result, arg->data, arg->length));
	else
		addThreadMessage(new TM_ConnectionUpdate(arg->connect_result, 0, 0));
}

int Client::cfunc_server_data(client_runes_t *arg) {
	gameclient->process_incoming_data(arg->data, arg->length);
	return 0;
}

