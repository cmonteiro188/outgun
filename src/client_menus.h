#ifndef CLIENT_MENUS_H_INC
#define CLIENT_MENUS_H_INC

#include "incalleg.h"
#include "graphics.h"	// for definition of Graphics::Antialiasing_mode and mode fetching functions
#include "menu.h"
#include "utility.h"

class Menu_name {
public:
	Textfield	name;
	Textarea	randomName;
	Textfield	password;
	Textarea	namestatus;
	Textarea	removePasswords;

	Menu menu;

	Menu_name() :
		name			("Name", "", 15),
		randomName		("Get random name"),
		password		("Tournament password", "", 15, '*'),
		namestatus		("Registration status"),
		removePasswords	("Remove server-specific player passwords"),

		menu			("Name and passwords")
	{
		menu.add_component(&name);
		menu.add_component(&randomName);
		menu.add_component(&password);
		menu.add_component(&namestatus);
		menu.add_component(&removePasswords);
	}
	void recursiveSetMenuOpener(Hookable<Menu>::HookFunctionT* opener) { menu.setHook(opener); }
};

class Menu_game {
public:
	Checkbox	showNames;
	Colorselect	favoriteColors;
	Checkbox	lagPrediction;
	Slider		lagPredictionAmount;
	Checkbox	joystick;

	Menu menu;

	Menu_game() :
		showNames			("Show player names", false),
		favoriteColors		("Favorite colors"),
		lagPrediction		("Lag prediction", false),
		lagPredictionAmount	("Lag prediction amount", 0, 10, 10),
		joystick			("Enable joystick control", false),

		menu				("Game options")
	{
		menu.add_component(&showNames);
		menu.add_component(&favoriteColors);
		menu.add_component(&lagPrediction);
		menu.add_component(&lagPredictionAmount);
		menu.add_component(&joystick);
	}
	void recursiveSetMenuOpener(Hookable<Menu>::HookFunctionT* opener) { menu.setHook(opener); }
};

class StringSelectInserter : public LineReceiver {
	Select<std::string>& dst;

public:
	StringSelectInserter(Select<std::string>& dst_) : dst(dst_) { }
	StringSelectInserter& operator()(const std::string& str) { dst.addOption(str, str); return *this; }
};

class Menu_graphics {
	ScreenMode oldMode;
	int oldDepth;
	bool oldWin;

public:
	Checkbox			windowed;
	Select<int>			colorDepth;
	Textarea			desktopDepth;
	Select<ScreenMode>	resolution;
	Textarea			apply;
	Select<std::string>	theme;
	Select<Graphics::Antialiasing_mode> antialiasing;

	Menu menu;

	Menu_graphics() :
		oldMode(-1, -1),	// guarantees anything to be newMode()

		windowed	("Windowed mode", true),
		colorDepth	("Color depth"),
		desktopDepth(" desktop", itoa(desktop_color_depth()) + "-bit"),
		resolution	("Screen size"),
		apply		("Apply changes"),
		theme		("Theme"),
		antialiasing("Antialiasing"),

		menu		("Graphic options")
	{
		colorDepth.addOption("16-bit", 16);
		colorDepth.addOption("24-bit", 24);
		colorDepth.addOption("32-bit", 32);
		colorDepth.set(desktop_color_depth());
		antialiasing.addOption("On" , Graphics::AA_both);
		antialiasing.addOption("Map", Graphics::AA_map);
		antialiasing.addOption("Off", Graphics::AA_none);
		menu.add_component(&windowed);
		menu.add_component(&colorDepth);
		menu.add_component(&desktopDepth);
		menu.add_component(&resolution);
		menu.add_component(&apply);
		menu.add_component(&theme);
		menu.add_component(&antialiasing);
	}
	void init(const Graphics& gfx) {
		const std::vector<ScreenMode> modes = gfx.getResolutions(colorDepth());
		nAssert(!modes.empty());
		resolution.clearOptions();
		for (std::vector<ScreenMode>::const_iterator mode = modes.begin(); mode != modes.end(); ++mode)
			resolution.addOption(itoa(mode->width) + '×' + itoa(mode->height), *mode);
		theme.clearOptions();
		StringSelectInserter ins(theme);
		gfx.search_themes(ins);
	}
	void update(const Graphics& gfx) {	// tries to keep the selected resolution and theme
		ScreenMode oldmode = resolution();
		std::string oldtheme = theme();
		init(gfx);
		resolution.set(oldmode);	// may fail; ignore
		theme.set(oldtheme);	// may fail; ignore
	}
	bool newMode() {
		if (oldMode == resolution() && oldDepth == colorDepth() && oldWin == windowed())
			return false;
		oldMode = resolution();
		oldDepth = colorDepth();
		oldWin = windowed();
		return true;
	}

	void recursiveSetMenuOpener(Hookable<Menu>::HookFunctionT* opener) { menu.setHook(opener); }
};

class Menu_sounds {
public:
	Checkbox			enabled;
	Select<std::string>	theme;

	Menu menu;

	Menu_sounds() :
		enabled	("Sounds enabled", true),
		theme	("Theme"),

		menu	("Sound options")
	{
		menu.add_component(&enabled);
		menu.add_component(&theme);
	}

	void init(const Sounds& snd) {
		theme.clearOptions();
		StringSelectInserter ins(theme);
		snd.search_themes(ins);
	}
	void update(const Sounds& snd) {	// tries to keep the selected theme
		std::string oldTheme = theme();
		init(snd);
		theme.set(oldTheme);
	}

	void recursiveSetMenuOpener(Hookable<Menu>::HookFunctionT* opener) { menu.setHook(opener); }
};

class Menu_options {
public:
	Menu_name		name;
	Menu_game		game;
	Menu_graphics	graphics;
	Menu_sounds		sounds;

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
	Textarea		connect;
	Textarea		disconnect;
	Menu_options	options;
	Textarea		startServer;
	Textarea		stopServer;

	Menu menu;

	Menu_main() :
		connect		("Connect"),
		disconnect	("Disconnect"),
		options		(),
		startServer	("Start local server"),
		stopServer	("Stop local server"),

		menu		("Outgun " GAME_VERSION)
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
	Textfield	password;
	Checkbox	save;

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

