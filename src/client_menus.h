#ifndef CLIENT_MENUS_H_INC
#define CLIENT_MENUS_H_INC

#include "menu.h"

class Menu_name {
public:
	Textfield name;
	Textfield password;
	Textarea namestatus;
	Textarea removePasswords;

	Menu menu;

	Menu_name() :
		name			("Name", "", 15),
		password		("Password", "", 15, '*'),
		namestatus		("Registration status"),
		removePasswords	("Remove server-specific player passwords"),

		menu			("Name and password")
	{
		menu.add_component(&name);
		menu.add_component(&password);
		menu.add_component(&namestatus);
		menu.add_component(&removePasswords);
	}
	void recursiveSetMenuOpener(Hookable<Menu>::HookFunctionT* opener) { menu.setHook(opener); }
};

class Menu_game {
public:
	Checkbox showNames;
	Textarea favoriteColors;
	Checkbox lagPrediction;
	Slider lagPredictionAmount;
	Checkbox joystick;

	Menu menu;

	Menu_game() :
		showNames			("Show player names"),
		favoriteColors		("Favorite colors"),
		lagPrediction		("Lag prediction"),
		lagPredictionAmount	("Lag prediction amount", 0, 10),
		joystick			("Enable joystick control"),

		menu				("Game options")
	{
		menu.add_component(&showNames);
//defunct		menu.add_component(&favoriteColors);
		menu.add_component(&lagPrediction);
		menu.add_component(&lagPredictionAmount);
//defunct		menu.add_component(&joystick);
	}
	void recursiveSetMenuOpener(Hookable<Menu>::HookFunctionT* opener) { menu.setHook(opener); }
};

class Menu_graphics {
public:
	Checkbox windowed;
	Select resolution;
	Select colorDepth;
	Textarea apply;
	Select theme;
	Select antialiasing;

	Menu menu;

	Menu_graphics() :
		windowed	("Windowed mode"),
		resolution	("Screen size"),
		colorDepth	("Color depth"),
		apply		("Apply changes"),
		theme		("Theme"),
		antialiasing("Antialiasing"),

		menu		("Graphic options")
	{
		antialiasing.addOption("On");
		antialiasing.addOption("Map");
		antialiasing.addOption("Off");
		menu.add_component(&windowed);
		menu.add_component(&resolution);
		menu.add_component(&colorDepth);
		menu.add_component(&apply);
		menu.add_component(&theme);
		menu.add_component(&antialiasing);
	}
	void recursiveSetMenuOpener(Hookable<Menu>::HookFunctionT* opener) { menu.setHook(opener); }
};

class Menu_sounds {
public:
	Checkbox enabled;
	Select theme;

	Menu menu;

	Menu_sounds() :
		enabled	("Sounds enabled"),
		theme	("Theme"),

		menu	("Sound options")
	{
		menu.add_component(&enabled);
		menu.add_component(&theme);
	}
	void recursiveSetMenuOpener(Hookable<Menu>::HookFunctionT* opener) { menu.setHook(opener); }
};

class Menu_options {
public:
	Menu_name name;
	Menu_game game;
	Menu_graphics graphics;
	Menu_sounds sounds;

	Menu menu;

	Menu_options() :
		name	(),
		game	(),
		graphics(),
		sounds	(),

		menu	("Options")
	{
		menu.add_component(&name.menu);
		menu.add_component(&game.menu);
		menu.add_component(&graphics.menu);
		menu.add_component(&sounds.menu);
	}
	void recursiveSetMenuOpener(Hookable<Menu>::HookFunctionT* opener) {
		menu.setHook(opener);
		name	.recursiveSetMenuOpener(opener->clone());
		game	.recursiveSetMenuOpener(opener->clone());
		graphics.recursiveSetMenuOpener(opener->clone());
		sounds	.recursiveSetMenuOpener(opener->clone());
	}
};

class Menu_main {
public:
	Textarea connect;
	Textarea disconnect;
	Menu_options options;
	Textarea startServer;
	Textarea stopServer;

	Menu menu;

	Menu_main() :
		connect		("Connect"),
		disconnect	("Disconnect"),
		options		(),
		startServer	("Start local server"),
		stopServer	("Stop local server"),

		menu		("Outgun")
	{
		menu.add_component(&connect);
		menu.add_component(&disconnect);
		menu.add_component(&options.menu);
		menu.add_component(&startServer);
		menu.add_component(&stopServer);
	}
	void recursiveSetMenuOpener(Hookable<Menu>::HookFunctionT* opener) {
		menu.setHook(opener);
		options.recursiveSetMenuOpener(opener->clone());
	}
};

class Menu_dialog {
public:
	Textarea line1;
	Textarea line2;

	Menu menu;

	Menu_dialog() :
		line1	(std::string()),
		line2	(std::string()),

		menu	("Outgun")
	{
		menu.add_component(&line1);
		menu.add_component(&line2);
	}
	void setup(std::string line1_, std::string line2_) { line1.setCaption(line1_); line2.setCaption(line2_); }
};

class Menu_playerPassword {
public:
	Textfield password;
	Checkbox save;

	Menu menu;

	Menu_playerPassword() :
		password	("Password", "", 15, '*'),
		save		("Save password"),

		menu		(std::string())
	{
		menu.add_component(&password);
		menu.add_component(&save);
	}
	void setup(std::string plyName, bool saveChecked) {
		menu.setCaption("Player password for " + plyName);
		save.set(saveChecked);
	}
};

class Menu_serverPassword {
public:
	Textfield password;

	Menu menu;

	Menu_serverPassword() :
		password	("Password", "", 15, '*'),

		menu		("Server password")
	{
		menu.add_component(&password);
	}
};

#endif	// CLIENT_MENUS_H_INC

