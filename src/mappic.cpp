#include "commont.h"
#include "world.h"
#include "graphics.h"
#include "mappic.h"

using std::vector;
using std::string;

void Mappic::run() {
	find_maps();
	save_pictures();
}

void Mappic::find_maps() {
	smaps = load_maps(SERVER_MAPS_DIR);
}

vector<string> Mappic::load_maps(const string& dir) {
	string searchPattern = wheregamedir + dir + directory_separator + "*.txt";

	log("Map picture saver: scanning for maps: '%s'.", searchPattern.c_str());

	al_ffblk mapffblk;	//for al_find*

	int result = al_findfirst(searchPattern.c_str(), &mapffblk, FA_ARCH | FA_RDONLY);
	vector<string> maps;
	while (result == 0) {
		char nameBuf[500];
		replace_extension(nameBuf, mapffblk.name, "", 500);
		nameBuf[strlen(nameBuf) - 1] = '\0';	//take last damn '.' out

		maps.push_back(nameBuf);
		//try next
		result = al_findnext(&mapffblk);
	}
	log("Map picture saver: %d maps found.", maps.size());
	return maps;
}

void Mappic::save_pictures() const {
	set_color_depth(16);
	Graphics graphics(log);
	graphics.setColors();
	const string dir(wheregamedir + "mappic" + directory_separator);
	for (vector<string>::const_iterator name = smaps.begin(); name != smaps.end(); name++) {
		string picture = dir + *name + ".pcx";
		Map mp;
		if (!mp.load(log, SERVER_MAPS_DIR, *name))
			log.error("Map picture saver: Map '%s' is not a valid map file.", picture.c_str());
		else if (graphics.save_map_picture(picture, mp))
			log("Map picture saver: Saved map picture to '%s'.", picture.c_str());
		else {
			log.error("Map picture saver: Can't save map picture to '%s'.", picture.c_str());
			throw Save_error();
		}
	}
}

