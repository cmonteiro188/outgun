/*
 *  client_menus.cpp
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

#include <string>
#include <sstream>
#include <vector>

#include "incalleg.h"
#include "menu.h"
#include "utility.h"
#include "graphics.h"
#include "sounds.h"
#include "client_menus.h"

using std::ostringstream;
using std::pair;
using std::string;
using std::vector;

// definition for internal use
class StringSelectInserter : public LineReceiver {
	Select<string>& dst;

public:
	StringSelectInserter(Select<string>& dst_) : dst(dst_) { }
	StringSelectInserter& operator()(const string& str) { dst.addOption(str, str); return *this; }
};

Spacer g_menuSpace(5);

#define ins_space() menu.add_component(&g_menuSpace);

Menu_addServer::Menu_addServer() :
	address		("IP address", "", 21),
	save		("Add to favorite list"),

	menu		("Add server", false)
{
	menu.add_component(&address);
	menu.add_component(&save);
}

Menu_serverList::Menu_serverList() :
	update			("Update server list"),
	refresh			("Refresh servers"),
	refreshStatus	("Refresh status"),

	favorites		("Show favorite servers"),
	addServer		(),
	manualEntry		("Manually enter IP", "", 21),

	keyHelp			("Ins = add to favorites          Del = remove server"),

	caption			("IP address           Ping D Players Vers. Host name"),

	menu			("Server list", true)
{
	reset();
}

void Menu_serverList::add(const NLaddress& address, const string& serverInfo) {
	servers.push_back(pair<NLaddress, Textarea>(address, Textarea(serverInfo)));
}

void Menu_serverList::reset() {
	menu.clear_components();
	servers.clear();
	menu.add_component(&update);
	menu.add_component(&refresh);
	menu.add_component(&refreshStatus);
	ins_space();
	menu.add_component(&favorites);
	menu.add_component(&addServer.menu);
	menu.add_component(&manualEntry);
	ins_space();
//	menu.add_component(&keyHelp);
//	ins_space();
	menu.add_component(&caption);
}

void Menu_serverList::addHooks(MenuHookable<Textarea>::HookFunctionT* hook, KeyHookable<Textarea>::HookFunctionT* keyHook) {
	for (vector<pair<NLaddress, Textarea> >::iterator servi = servers.begin(); servi != servers.end(); ++servi) {
		servi->second.setHook(hook->clone());
		servi->second.setKeyHook(keyHook->clone());
		menu.add_component(&servi->second);
	}
	delete hook;
	delete keyHook;
}

NLaddress Menu_serverList::getAddress(const Textarea& target) {
	for (vector<pair<NLaddress, Textarea> >::iterator servi = servers.begin(); servi != servers.end(); ++servi) {
		if (&servi->second == &target)
			return servi->first;
	}
	nAssert(0);
	return NLaddress();
}

void Menu_serverList::recursiveSetMenuOpener(MenuHookable<Menu>::HookFunctionT* opener) {
	menu.setHook(opener);
	addServer.recursiveSetMenuOpener(opener->clone());
}

Menu_name::Menu_name() :
	name			("Name", "", 15),
	randomName		("Get random name"),

	password		("Tournament password", "", 15, '*'),
	namestatus		("Registration status"),
	tournament		("Take part in the tournament", true),

	removePasswords	("Remove server-specific player passwords"),

	menu			("Name and passwords", true)
{
	menu.add_component(&name);
	menu.add_component(&randomName);
	ins_space();
	menu.add_component(&password);
	menu.add_component(&namestatus);
	menu.add_component(&tournament);
	ins_space();
	menu.add_component(&removePasswords);
}

Menu_game::Menu_game() :
	showNames			("Show player names", false),
	favoriteColors		("Favorite colors"),
	lagPrediction		("Lag prediction", false),
	lagPredictionAmount	("Lag prediction amount", true, 0, 10, 10),

	messageLogging		("Save chat messages", false),
	saveStats			("Save game statistics", false),
	showStats			("Show stats after the round", false),
	showServerInfo		("Show server info when connected", false),
	underlineMasterAuth	("Underline master-authenticated players", true),
	underlineServerAuth	("Underline server-authenticated players", false),

	autoGetServerList	("Get server list at startup", true),

	menu				("Game options", true)
{
	menu.add_component(&showNames);
	menu.add_component(&favoriteColors);
	menu.add_component(&lagPrediction);
	menu.add_component(&lagPredictionAmount);
	ins_space();
	menu.add_component(&messageLogging);
	menu.add_component(&saveStats);
	menu.add_component(&showStats);
	menu.add_component(&showServerInfo);
	menu.add_component(&underlineMasterAuth);
	menu.add_component(&underlineServerAuth);
	ins_space();
	menu.add_component(&autoGetServerList);
}

Menu_controls::Menu_controls() :
	keypadMoving		("Use keypad for moving", true),

	joystick			("Enable joystick control", false),
	joyMove				("Moving stick", false, 0, 5, 1),
	joyText				("Joystick buttons (0 = disabled)"),
	joyShoot			("Shoot ", false, 0, 16, 1),
	joyRun				("Run   ", false, 0, 16, 2),
	joyStrafe			("Strafe", false, 0, 16, 3),

	menu				("Controls", true)
{
	menu.add_component(&keypadMoving);
	ins_space();
	menu.add_component(&joystick);
	menu.add_component(&joyMove);
	menu.add_component(&joyText);
	menu.add_component(&joyShoot);
	menu.add_component(&joyRun);
	menu.add_component(&joyStrafe);
}

void Menu_graphics::reloadChoices(const Graphics& gfx) {
	const vector<ScreenMode> modes = gfx.getResolutions(colorDepth());
	nAssert(!modes.empty());
	resolution.clearOptions();
	for (vector<ScreenMode>::const_iterator mode = modes.begin(); mode != modes.end(); ++mode)
		resolution.addOption(itoa(mode->width) + '×' + itoa(mode->height), *mode);
	theme.clearOptions();
	StringSelectInserter ins(theme);
	gfx.search_themes(ins);
}

Menu_graphics::Menu_graphics() :
	oldMode(-1, -1),	// guarantees anything to be newMode()

	colorDepth	("Color depth"),
	desktopDepth(" desktop", itoa(desktop_color_depth()) + "-bit"),
	resolution	("Screen size"),
	windowed	("Windowed mode", true),
	flipping	("Use page flipping", false),
	refreshRate	("Current refresh rate"),
	apply		("Apply changes"),

	theme		("Theme"),
	antialiasing("Antialiasing", true),
	contTextures("Continuous textures between rooms", false),
	statsBgAlpha("Stats screen alpha", true, 0, 255, 255, 15),

	fpsLimit	("FPS limit", false, 1, 10000, 60, 0),
	mapInfoMode	("Map info mode", false),

	menu		("Graphic options", true)
{
	menu.add_component(&colorDepth);
	menu.add_component(&desktopDepth);
	menu.add_component(&resolution);
	menu.add_component(&windowed);
	menu.add_component(&flipping);
	menu.add_component(&refreshRate);
	menu.add_component(&apply);
	ins_space();
	menu.add_component(&theme);
	menu.add_component(&antialiasing);
	menu.add_component(&contTextures);
	menu.add_component(&statsBgAlpha);
	ins_space();
	menu.add_component(&fpsLimit);
	menu.add_component(&mapInfoMode);
}

void Menu_graphics::init(const Graphics& gfx) {	// call just once, before calling update
	nAssert(colorDepth.size() == 0);
	if (gfx.depthAvailable(16))
		colorDepth.addOption("16-bit", 16);
	if (gfx.depthAvailable(24))
		colorDepth.addOption("24-bit", 24);
	if (gfx.depthAvailable(32))
		colorDepth.addOption("32-bit", 32);
	if (colorDepth.size() == 0)
		colorDepth.addOption("16-bit", 16);	// this will force Graphics to use a hope-this-works resolution
	colorDepth.set(desktop_color_depth());	// may fail (although it's unlikely); ignore
	reloadChoices(gfx);
	resolution.set(ScreenMode(640, 480));	// default resolution
}

void Menu_graphics::update(const Graphics& gfx) {	// tries to keep the selected resolution and theme
	ScreenMode oldmode = resolution();
	string oldtheme = theme();
	reloadChoices(gfx);
	resolution.set(oldmode);	// may fail; ignore
	theme.set(oldtheme);		// may fail; ignore
}

bool Menu_graphics::newMode() {
	if (oldMode == resolution() && oldDepth == colorDepth() && oldWin == windowed() && oldFlip == flipping())
		return false;
	oldMode = resolution();
	oldDepth = colorDepth();
	oldWin = windowed();
	oldFlip = flipping();
	return true;
}


Menu_sounds::Menu_sounds() :
	enabled	("Sounds enabled", true),
	volume	("Volume", true, 0, 255, 128, 15),
	theme	("Theme"),

	menu	("Sound options", true)
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
	const string oldTheme = theme();
	init(snd);
	theme.set(oldTheme);
}

Menu_options::Menu_options() :
	name	(),
	game	(),
	controls(),
	graphics(),
	sounds	(),

	menu	("Options", true)
{
	menu.add_component(&name.menu);
	menu.add_component(&game.menu);
	menu.add_component(&controls.menu);
	menu.add_component(&graphics.menu);
	menu.add_component(&sounds.menu);
}

void Menu_options::recursiveSetMenuOpener(MenuHookable<Menu>::HookFunctionT* opener) {
	menu.setHook(opener);
	name	.recursiveSetMenuOpener(opener->clone());
	game	.recursiveSetMenuOpener(opener->clone());
	controls.recursiveSetMenuOpener(opener->clone());
	graphics.recursiveSetMenuOpener(opener->clone());
	sounds	.recursiveSetMenuOpener(opener->clone());
}

Menu_ownServer::Menu_ownServer() :
	pub		("Add to public serverlist", false),
	port	("Server port", 1, 65535, DEFAULT_UDP_PORT),
	address	("Detected address"),

	start	("Start server"),
	play	("Play on the server"),
	stop	("Stop server"),

	menu	("Local server", true)
{
	menu.add_component(&pub);
	menu.add_component(&port);
	menu.add_component(&address);
	ins_space();
	menu.add_component(&start);
	menu.add_component(&play);
	menu.add_component(&stop);
}

void Menu_ownServer::init(const std::string& externalAddress, bool priv) {
	privateIP = priv;
	if (priv && !externalAddress.empty())
		ip = string("(private) ") + externalAddress;
	else
		ip = externalAddress;	// the case of empty ip is handled specially in the drawing code
}

void Menu_ownServer::refreshCaption(bool serverRunning) {
	if (serverRunning)
		menu.setCaption("Local server (running)");
	else
		menu.setCaption("Local server");
	// this could be a separate function but it really doesn't hurt to update the address field here
	if (ip.empty())
		address.set("(unknown)");
	else {
		ostringstream os;
		os << ip;
		if (port() != DEFAULT_UDP_PORT)
			os << ':' << port();
		address.set(os.str());
	}
}

void Menu_ownServer::refreshEnables(bool serverRunning, bool connected) {
	if (serverRunning) {
		pub.setEnable(false);
		port.setEnable(false);
		start.setEnable(false);
		play.setEnable(!connected);
		stop.setEnable(true);
	}
	else {
		if (privateIP) {
			pub.set(false);
			pub.setEnable(false);
		}
		else
			pub.setEnable(true);
		port.setEnable(true);
		start.setEnable(true);
		play.setEnable(false);
		stop.setEnable(false);
	}
}

void Menu_ownServer::recursiveSetMenuOpener(MenuHookable<Menu>::HookFunctionT* opener) {
	menu.setHook(opener);
}

Menu_main::Menu_main() :
	newVersion	(""),

	connect		(),
	disconnect	("Disconnect"),

	options		(),

	ownServer	(),

	help		(),
	exitOutgun	("Exit Outgun"),

	menu		("Outgun " GAME_VERSION, true)
{
	menu.add_component(&newVersion);
	menu.add_component(&connect.menu);
	menu.add_component(&disconnect);
	ins_space();
	menu.add_component(&options.menu);
	ins_space();
	menu.add_component(&ownServer.menu);
	ins_space();
	menu.add_component(&help.menu);
	menu.add_component(&exitOutgun);
}

void Menu_main::recursiveSetMenuOpener(MenuHookable<Menu>::HookFunctionT* opener) {
	menu.setHook(opener);
	connect.recursiveSetMenuOpener(opener->clone());
	options.recursiveSetMenuOpener(opener->clone());
	ownServer.recursiveSetMenuOpener(opener->clone());
	help.recursiveSetMenuOpener(opener->clone());
}

Menu_text::Menu_text() :
	accept	("OK"),
	cancel	("Cancel"),

	menu	("Outgun " GAME_VERSION, false)
{ }

void Menu_text::addLine(const string& line, bool cancelable) {
	addLine(line, "", cancelable);
}

void Menu_text::addLine(const string& caption, const string& value, bool cancelable) {
	lines.push_back(StaticText(caption, value));

	const int oldSel = menu.selection();
	menu.clear_components();
	for (vector<StaticText>::iterator li = lines.begin(); li != lines.end(); ++li)
		menu.add_component(&*li);
	ins_space();
	if (cancelable)
		menu.add_component(&cancel);
	else
		menu.add_component(&accept);
	menu.setSelection(oldSel);
}

void Menu_text::wrapLine(const string& line, bool cancelable, int wrapPos) {
	vector<string> lines = split_to_lines(line, wrapPos, 5);	// indent continuation lines by 5 characters
	for (vector<string>::const_iterator li = lines.begin(); li != lines.end(); ++li)
		addLine(*li, "", cancelable);
}

void Menu_text::recursiveSetMenuOpener(MenuHookable<Menu>::HookFunctionT* opener) {
	menu.setHook(opener);
}

Menu_playerPassword::Menu_playerPassword() :
	password	("Password", "", 15, '*'),
	save		("Save password"),

	menu		(string(), false)	// caption is set by setup()
{
	menu.add_component(&password);
	menu.add_component(&save);
}

void Menu_playerPassword::setup(const string& plyName, bool saveChecked) {
	menu.setCaption("Player password for " + plyName);
	password.set("");
	save.set(saveChecked);
}

Menu_serverPassword::Menu_serverPassword() :
	password	("Password", "", 15, '*'),

	menu		("Server password", false)
{
	menu.add_component(&password);
}

Menu_help::Menu_help() :
	text	(),

	menu	("Help", false)
{ }

void Menu_help::addLine(const string& line) {
	lines.push_back(line);

	const int oldSel = menu.selection();
	menu.clear_components();
	text = Textobject();
	for (vector<string>::iterator li = lines.begin(); li != lines.end(); ++li)
		text.addLine(*li);
	menu.add_component(&text);
	menu.setSelection(oldSel);
}

void Menu_help::recursiveSetMenuOpener(MenuHookable<Menu>::HookFunctionT* opener) {
	menu.setHook(opener);
}
