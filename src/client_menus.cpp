#include "incalleg.h"
#include "menu.h"
#include "utility.h"
#include "graphics.h"
#include "sounds.h"
#include "client_menus.h"

// definition for internal use
class StringSelectInserter : public LineReceiver {
	Select<std::string>& dst;

public:
	StringSelectInserter(Select<std::string>& dst_) : dst(dst_) { }
	StringSelectInserter& operator()(const std::string& str) { dst.addOption(str, str); return *this; }
};

Menu_serverList::Menu_serverList() :
	favorites	("Favorite servers only"),
	update		("Update server list"),
	refresh		("Refresh servers"),
	caption		("IP address            Ping Players Version Host name"),

	menu		("Server list")
{
	reset();
}

void Menu_serverList::add(const std::string& address, const std::string& serverInfo) {
	servers.push_back(std::pair<std::string, Textarea>(address, Textarea(serverInfo)));
}

void Menu_serverList::reset() {
	menu.clear_components();
	servers.clear();
	menu.add_component(&favorites);
	menu.add_component(&update);
	menu.add_component(&refresh);
	menu.add_component(&caption);
}

void Menu_serverList::addHooks(Hookable<Textarea>::HookFunctionT* hook) {
	for (std::vector<std::pair<std::string, Textarea> >::iterator servi = servers.begin(); servi != servers.end(); ++servi) {
		servi->second.setHook(hook->clone());
		menu.add_component(&servi->second);
	}
	delete hook;
}

std::string Menu_serverList::getAddress(const Textarea& target) {
	for (std::vector<std::pair<std::string, Textarea> >::iterator servi = servers.begin(); servi != servers.end(); ++servi) {
		if (&servi->second == &target)
			return servi->first;
	}
	return std::string();
}

Menu_name::Menu_name() :
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

Menu_game::Menu_game() :
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

Menu_graphics::Menu_graphics() :
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

void Menu_graphics::init(const Graphics& gfx) {
	const std::vector<ScreenMode> modes = gfx.getResolutions(colorDepth());
	nAssert(!modes.empty());
	resolution.clearOptions();
	for (std::vector<ScreenMode>::const_iterator mode = modes.begin(); mode != modes.end(); ++mode)
		resolution.addOption(itoa(mode->width) + '×' + itoa(mode->height), *mode);
	theme.clearOptions();
	StringSelectInserter ins(theme);
	gfx.search_themes(ins);
}

void Menu_graphics::update(const Graphics& gfx) {	// tries to keep the selected resolution and theme
	ScreenMode oldmode = resolution();
	std::string oldtheme = theme();
	init(gfx);
	resolution.set(oldmode);	// may fail; ignore
	theme.set(oldtheme);		// may fail; ignore
}

bool Menu_graphics::newMode() {
	if (oldMode == resolution() && oldDepth == colorDepth() && oldWin == windowed())
		return false;
	oldMode = resolution();
	oldDepth = colorDepth();
	oldWin = windowed();
	return true;
}

Menu_sounds::Menu_sounds() :
	enabled	("Sounds enabled", true),
	volume	("Volume", 0, 10, 5),
	theme	("Theme"),

	menu	("Sound options")
{
	menu.add_component(&enabled);
	menu.add_component(&volume);
	menu.add_component(&theme);
}

void Menu_sounds::init(const Sounds& snd) {
	theme.clearOptions();
	StringSelectInserter ins(theme);
	snd.search_themes(ins);
}

void Menu_sounds::update(const Sounds& snd) {	// tries to keep the selected theme
	std::string oldTheme = theme();
	init(snd);
	theme.set(oldTheme);
}

Menu_options::Menu_options() :
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

void Menu_options::recursiveSetMenuOpener(Hookable<Menu>::HookFunctionT* opener) {
	menu.setHook(opener);
	name	.recursiveSetMenuOpener(opener->clone());
	game	.recursiveSetMenuOpener(opener->clone());
	graphics.recursiveSetMenuOpener(opener->clone());
	sounds	.recursiveSetMenuOpener(opener->clone());
}

Menu_main::Menu_main() :
	connect		(),
	disconnect	("Disconnect"),
	options		(),
	startServer	("Start local server"),
	stopServer	("Stop local server"),

	menu		("Outgun " GAME_VERSION)
{
	menu.add_component(&connect.menu);
	menu.add_component(&disconnect);
	menu.add_component(&options.menu);
	menu.add_component(&startServer);
	menu.add_component(&stopServer);
}

void Menu_main::recursiveSetMenuOpener(Hookable<Menu>::HookFunctionT* opener) {
	menu.setHook(opener);
	connect.recursiveSetMenuOpener(opener->clone());
	options.recursiveSetMenuOpener(opener->clone());
}

Menu_text::Menu_text() :
	accept	("OK"),
	cancel	("Cancel"),

	menu	("Outgun " GAME_VERSION)
{ }

void Menu_text::addLine(std::string line, bool cancelable) {
	lines.push_back(Textarea(line));
	menu.clear_components();
	for (std::vector<Textarea>::iterator li = lines.begin(); li != lines.end(); ++li)
		menu.add_component(&*li);
	if (cancelable)
		menu.add_component(&cancel);
	else
		menu.add_component(&accept);
}

Menu_playerPassword::Menu_playerPassword() :
	password	("Password", "", 15, '*'),
	save		("Save password"),

	menu		(std::string())	// caption is set by setup()
{
	menu.add_component(&password);
	menu.add_component(&save);
}

void Menu_playerPassword::setup(std::string plyName, bool saveChecked) {
	menu.setCaption("Player password for " + plyName);
	password.set("");
	save.set(saveChecked);
}

Menu_serverPassword::Menu_serverPassword() :
	password	("Password", "", 15, '*'),

	menu		("Server password")
{
	menu.add_component(&password);
}

