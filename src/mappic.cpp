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
	string mappath = dir;	// maps
	char sep[2] = { 0 };
	put_backslash(sep);
 	mappath += sep;			// maps/
	mappath += "*.txt";		// maps/*.txt
	char nameBuf[512];
	char dest[1024];
	append_filename(dest, wheregamedir, mappath.c_str(), WHERE_PATH_SIZE);	// <FULL-DIR>/maps/*.txt, I hope

	LOG1("Map picture save: scan dir is '%s'\n", dest);

	al_ffblk mapffblk;	//for al_find*

	int result = al_findfirst(dest, &mapffblk, FA_ARCH | FA_RDONLY);
	vector<string> maps;
	while (result == 0) {
		//char *replace_extension(char *dest, const char *filename, const char *ext, int size
		replace_extension(nameBuf, mapffblk.name, "", 500);
		nameBuf[strlen(nameBuf) - 1] = '\0';	//take last damn '.' out

		maps.push_back(nameBuf);
		//try next
		result = al_findnext(&mapffblk);
	}
	LOG1("%d maps found\n", maps.size());
	return maps;
}

void Mappic::save_pictures() const {
	set_color_depth(16);
	Graphics graphics;
	graphics.setcolors();
	string dir("screens");
	dir += directory_separator;
	for (vector<string>::const_iterator name = smaps.begin(); name != smaps.end(); name++) {
		string picture = dir + *name + ".pcx";
		Map mp;
		mp.load(SERVER_MAPS_DIR, *name);
		if (graphics.save_map_picture(picture, mp)) {
			LOG1("Saved map picture to '%s'\n", picture.c_str());
		}
		else {
			LOG1("Can't save map picture to '%s'\n", picture.c_str());
			throw Save_error();
		}
	}
}

