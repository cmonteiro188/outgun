#ifndef CLIENT_MENUS_H_INC
#define CLIENT_MENUS_H_INC

#include <string>
#include <vector>

#include "graphics.h"	// for definition of Graphics::Antialiasing_mode and mode fetching functions
#include "menu.h"

class Menu_addServer {
public:
	Textfield	address;
	Checkbox	save;

	Menu menu;

	Menu_addServer();

	void recursiveSetMenuOpener(MenuHookable<Menu>::HookFunctionT* opener) { menu.setHook(opener); }
};

class Menu_serverList {
	std::vector<std::pair<NLaddress, Textarea> > servers;	// address and server info

public:
	Textarea		update;
	Textarea		refresh;
	StaticText		refreshStatus;
	Checkbox		favorites;
	Menu_addServer	addServer;
	StaticText		keyHelp;
	StaticText		caption;

	Menu menu;

	Menu_serverList();
	void add(const NLaddress& address, const std::string& serverInfo);
	void reset();
	void addHooks(MenuHookable<Textarea>::HookFunctionT* hook, KeyHookable<Textarea>::HookFunctionT* keyHook);
	NLaddress getAddress(const Textarea& target);

	void recursiveSetMenuOpener(MenuHookable<Menu>::HookFunctionT* opener);
};

class Menu_name {
public:
	Textfield	name;
	Textarea	randomName;
	Textfield	password;
	StaticText	namestatus;
	Checkbox	tournament;
	Textarea	removePasswords;

	Menu menu;

	Menu_name();
	void recursiveSetMenuOpener(MenuHookable<Menu>::HookFunctionT* opener) { menu.setHook(opener); }
};

class Menu_game {
public:
	Checkbox	showNames;
	Colorselect	favoriteColors;
	Checkbox	lagPrediction;
	Slider		lagPredictionAmount;
	Checkbox	messageLogging;
	Checkbox	saveStats;
	Checkbox	showStats;
	Checkbox	showServerInfo;
	Checkbox	autoGetServerList;

	Menu menu;

	Menu_game();
	void recursiveSetMenuOpener(MenuHookable<Menu>::HookFunctionT* opener) { menu.setHook(opener); }
};

class Menu_controls {
public:
	Checkbox	keypadMoving;
	Checkbox	joystick;
	Slider		joyMove;
	StaticText	joyText;
	Slider		joyShoot;
	Slider		joyRun;
	Slider		joyStrafe;

	Menu menu;

	Menu_controls();
	void recursiveSetMenuOpener(MenuHookable<Menu>::HookFunctionT* opener) { menu.setHook(opener); }
};

class Menu_graphics {
	ScreenMode oldMode;
	int oldDepth;
	bool oldWin, oldFlip;

	void reloadChoices(const Graphics& gfx);

public:
	Select<int>			colorDepth;
	StaticText			desktopDepth;
	Select<ScreenMode>	resolution;
	Checkbox			windowed;
	Checkbox			flipping;
	StaticText			refreshRate;
	Textarea			apply;
	Select<std::string>	theme;
	Select<Graphics::Antialiasing_mode> antialiasing;
	Slider				statsBgAlpha;
	Slider				fpsLimit;
	Checkbox			mapInfoMode;

	Menu menu;

	Menu_graphics();
	void init(const Graphics& gfx);	// call just once, before calling update
	void update(const Graphics& gfx);	// tries to keep the selected resolution and theme
	bool newMode();	// returns true if the current selection differs from the one at last call

	void recursiveSetMenuOpener(MenuHookable<Menu>::HookFunctionT* opener) { menu.setHook(opener); }
};

class Menu_sounds {
public:
	Checkbox			enabled;
	Slider				volume;
	Select<std::string>	theme;

	Menu menu;

	Menu_sounds();
	void init(const Sounds& snd);
	void update(const Sounds& snd);	// tries to keep the selected theme

	void recursiveSetMenuOpener(MenuHookable<Menu>::HookFunctionT* opener) { menu.setHook(opener); }
};

class Menu_options {
public:
	Menu_name		name;
	Menu_game		game;
	Menu_controls	controls;
	Menu_graphics	graphics;
	Menu_sounds		sounds;

	Menu menu;

	Menu_options();
	void recursiveSetMenuOpener(MenuHookable<Menu>::HookFunctionT* opener);
};

class Menu_help {
	std::vector<std::string> lines;

public:
	Textobject text;

	Menu menu;

	Menu_help();
	void clear() { lines.clear(); }
	void addLine(const std::string& line);

	void recursiveSetMenuOpener(MenuHookable<Menu>::HookFunctionT* opener);
};

class Menu_main {
public:
	StaticText		newVersion;
	Menu_serverList	connect;
	Textarea		disconnect;
	Menu_options	options;
	Textarea		startServer;
	Textarea		playServer;
	Textarea		stopServer;
	Menu_help		help;
	Textarea		exitOutgun;

	Menu menu;

	Menu_main();
	void recursiveSetMenuOpener(MenuHookable<Menu>::HookFunctionT* opener);
};

class Menu_text {
	std::vector<StaticText>	lines;

public:
	Textarea				accept;
	Textarea				cancel;
	// either accept or cancel visible at a time

	Menu menu;

	Menu_text();
	void clear() { lines.clear(); }
	void addLine(const std::string& line, bool cancelable = false);
	void addLine(const std::string& caption, const std::string& value, bool cancelable = false);
	void wrapLine(const std::string& line, bool cancelable = false, int wrapPos = 68);

	void recursiveSetMenuOpener(MenuHookable<Menu>::HookFunctionT* opener);
};

class Menu_playerPassword {
public:
	Textfield	password;
	Checkbox	save;

	Menu menu;

	Menu_playerPassword();
	void setup(const std::string& plyName, bool saveChecked);
};

class Menu_serverPassword {
public:
	Textfield password;

	Menu menu;

	Menu_serverPassword();
};

#endif	// CLIENT_MENUS_H_INC

