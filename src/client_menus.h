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
	std::vector<std::pair<std::string, Textarea> > servers;	// address and server info

public:
	Checkbox		favorites;
	Textarea		refreshStatus;
	Textarea		update;
	Textarea		refresh;
	Menu_addServer	addServer;
	Textarea		caption;

	Menu menu;

	Menu_serverList();
	void add(const std::string& address, const std::string& serverInfo);
	void reset();
	void addHooks(MenuHookable<Textarea>::HookFunctionT* hook);
	std::string getAddress(const Textarea& target);

	void recursiveSetMenuOpener(MenuHookable<Menu>::HookFunctionT* opener);
};

class Menu_name {
public:
	Textfield	name;
	Textarea	randomName;
	Textfield	password;
	Textarea	namestatus;
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
	Checkbox	joystick;
	Checkbox	messageLogging;
	Checkbox	saveStats;

	Menu menu;

	Menu_game();
	void recursiveSetMenuOpener(MenuHookable<Menu>::HookFunctionT* opener) { menu.setHook(opener); }
};

class Menu_graphics {
	ScreenMode oldMode;
	int oldDepth;
	bool oldWin, oldFlip;

public:
	Checkbox			windowed;
	Select<int>			colorDepth;
	Textarea			desktopDepth;
	Select<ScreenMode>	resolution;
	Checkbox			flipping;
	Slider				fpsLimit;
	Textarea			refreshRate;
	Textarea			apply;
	Select<std::string>	theme;
	Select<Graphics::Antialiasing_mode> antialiasing;

	Menu menu;

	Menu_graphics();
	void init(const Graphics& gfx);
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
	Menu_graphics	graphics;
	Menu_sounds		sounds;

	Menu menu;

	Menu_options();
	void recursiveSetMenuOpener(MenuHookable<Menu>::HookFunctionT* opener);
};

class Menu_main {
public:
	Menu_serverList	connect;
	Textarea		disconnect;
	Menu_options	options;
	Textarea		startServer;
	Textarea		stopServer;

	Menu menu;

	Menu_main();
	void recursiveSetMenuOpener(MenuHookable<Menu>::HookFunctionT* opener);
};

class Menu_text {
	std::vector<Textarea>	lines;

public:
	Textarea				accept;
	Textarea				cancel;
	// either accept or cancel visible at a time

	Menu menu;

	Menu_text();
	void clear() { lines.clear(); }
	void addLine(std::string line, bool cancelable = false);
};

class Menu_playerPassword {
public:
	Textfield	password;
	Checkbox	save;

	Menu menu;

	Menu_playerPassword();
	void setup(std::string plyName, bool saveChecked);
};

class Menu_serverPassword {
public:
	Textfield password;

	Menu menu;

	Menu_serverPassword();
};

class Menu_serverInfo {
	std::vector<Textarea> info;

public:
	Menu menu;

	Menu_serverInfo();

	void add(const std::string& address, const std::string& serverInfo);
	void reset();
	void finish();

	void recursiveSetMenuOpener(MenuHookable<Menu>::HookFunctionT* opener) { menu.setHook(opener); }
};

#endif	// CLIENT_MENUS_H_INC

