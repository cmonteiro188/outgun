/*
 *  client_menus.h
 *
 *  Copyright (C) 2004 - Niko Ritari
 *  Copyright (C) 2004 - Jani Rivinoja
 *
 *  This file is part of Outgun.
 *
 *  Outgun is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Outgun is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Outgun; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef CLIENT_MENUS_H_INC
#define CLIENT_MENUS_H_INC

#include <string>
#include <vector>

#include "graphics.h"	// for mode fetching functions
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
	Textarea			update;
	Textarea			refresh;
	StaticText			refreshStatus;
	Checkbox			favorites;
	Menu_addServer		addServer;
	Textfield			manualEntry;
	StaticText			keyHelp;
	StaticText			caption;

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
	Checkbox	underlineMasterAuth;
	Checkbox	underlineServerAuth;
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
	Checkbox			antialiasing;
	Checkbox			contTextures;
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

class Menu_ownServer {
public:
	Checkbox		pub;
	NumberEntry		port;
	StaticText		address;
	Textarea		start;
	Textarea		play;
	Textarea		stop;

	Menu menu;

	Menu_ownServer();
	void init(const std::string& externalAddress, bool priv);
	void refreshCaption(bool serverRunning);
	void refreshEnables(bool serverRunning, bool connected);

	void recursiveSetMenuOpener(MenuHookable<Menu>::HookFunctionT* opener);

private:
	std::string ip;
	bool privateIP;
};

class Menu_main {
public:
	StaticText		newVersion;
	Menu_serverList	connect;
	Textarea		disconnect;
	Menu_options	options;
	Menu_ownServer	ownServer;
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

