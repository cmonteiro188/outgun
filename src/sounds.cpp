#include "sounds.h"

using std::string;

Sounds::Sounds():
	no_theme(false)
{
	//no samples loaded -- important so unload_samples don't crash
	for (int i = 0; i < NUM_OF_SAMPLES; i++)
		sample[i] = 0;
}

Sounds::~Sounds() {
	unload_samples();
}

const string& Sounds::theme_name() const {
	return themename;
}

const string& Sounds::theme_dir() const {
	return themedir;
}

void Sounds::search_themes() {
	if (no_theme)
		return;
	//try the last theme directory first
	const string themepath = make_theme_path("*.*");

	LOG1("theme searching '%s'\n", themepath.c_str());

	int error = al_findfirst(themepath.c_str(), &themeffblk, FA_DIREC | FA_ARCH | FA_RDONLY);

	while (!error) {
		if ((themeffblk.attrib & FA_DIREC) && strcmp(themeffblk.name, ".") &&
		  strcmp(themeffblk.name, "..") && themedir == themeffblk.name) {
			load_theme(themedir);
			return;
		}
		error = al_findnext(&themeffblk);
	}
	no_theme = true;
	LOG("No sound theme selected.\n");
}

void Sounds::next_theme() {
	int error;
	if (no_theme) {
		no_theme = false;
		const string themepath = make_theme_path("*.*");
		error = al_findfirst(themepath.c_str(), &themeffblk, FA_DIREC | FA_ARCH | FA_RDONLY);
	}
	else
		error = al_findnext(&themeffblk);
	if (error) {
		no_theme = true;
		unload_samples();
		LOG("No theme selected.\n");
	}
	else if ((themeffblk.attrib & FA_DIREC) && strcmp(themeffblk.name, ".") && strcmp(themeffblk.name, ".."))
		load_theme(themeffblk.name);
	else
		next_theme();
}

string Sounds::make_theme_path(const string& dir) {
	string sound_name = "sound";
	sound_name += directory_separator;
	sound_name += dir;

	char dest[WHERE_PATH_SIZE];
	append_filename(dest, wheregamedir, sound_name.c_str(), WHERE_PATH_SIZE);

	LOG1("Sound theme path is '%s'.\n", dest);

	return dest;
}

void Sounds::load_theme(const string& dir) {
	if (!dir.empty())
		themedir = dir;

	unload_samples();		//unload old (if any)
	load_samples();			//load new

	// load sfx theme description
	string des_file = "sound";
	des_file += directory_separator;
	des_file += themedir;
	des_file += directory_separator;
	des_file += "theme.txt";

	char dest[WHERE_PATH_SIZE];
	append_filename(dest, wheregamedir, des_file.c_str(), WHERE_PATH_SIZE);

	ifstream in(dest);
	if (!getline_smart(in, themename))
		themename = "(unnamed theme)";
	LOG1("Loaded sound theme from '%s'.\n", des_file.c_str());

	//play a sample
	if (!no_theme)
		play(rand() % NUM_OF_SAMPLES);
}

//append the correct path
SAMPLE* Sounds::load_outgun_sample(const string& fname, int slot, bool try_redirect, bool reverse) {
	//soundname: add "sound/" to the filename
	string sound_name = "sound";
	sound_name += directory_separator;
	sound_name += themedir;
	sound_name += directory_separator;
	sound_name += fname;
	sound_name += ".wav";

	//add soundname to where game dir
	char dest[WHERE_PATH_SIZE];
	append_filename(dest, wheregamedir, sound_name.c_str(), WHERE_PATH_SIZE);

	//try load
	SAMPLE* ret = sample[slot] = load_sample(dest);

	//sample must be played in reverse?
	sample_reverse[slot] = reverse;

	LOG4("load_sample[%i]: '%s' = %p  rev = %i\n", slot, dest, ret, sample_reverse[slot]);

	//V0.3.10: if not found, look for .txt redirect
	if (try_redirect && ret == 0) {	// don't go into endless loop
		//txt filename
		sound_name = "sound";
		sound_name += directory_separator;
		sound_name += themedir;
		sound_name += directory_separator;
		sound_name += fname;
		sound_name += ".txt";
		append_filename(dest, wheregamedir, sound_name.c_str(), WHERE_PATH_SIZE);

		ifstream in(dest);
		if (in) {
			string redir_name;
			getline_smart(in, redir_name);
			in.close();

			bool is_reversed = false;

			//retry once ("false": don't try redirect again if fails)
			return load_outgun_sample(redir_name.c_str(), slot, false, is_reversed);
		}
	}

	return ret;
}

//sample try loads
void Sounds::load_samples() {
	if (!sound_inited)
		return;
	load_outgun_sample("fire", SAMPLE_FIRE);
	load_outgun_sample("hit", SAMPLE_HIT);
	load_outgun_sample("wallhit", SAMPLE_WALLHIT);
	load_outgun_sample("qwallhit", SAMPLE_QUADWALLHIT);

	load_outgun_sample("getdb", SAMPLE_GETDEATHBRINGER);
	load_outgun_sample("usedb", SAMPLE_USEDEATHBRINGER);
	load_outgun_sample("hitdb", SAMPLE_HITDEATHBRINGER);
	load_outgun_sample("diedb", SAMPLE_DIEDEATHBRINGER);

	load_outgun_sample("death1", SAMPLE_DEATH);
	load_outgun_sample("death2", SAMPLE_DEATH_2);

	load_outgun_sample("entergam", SAMPLE_ENTERGAME);
	load_outgun_sample("leftgam", SAMPLE_LEFTGAME);
	load_outgun_sample("chanteam", SAMPLE_CHANGETEAM);
	load_outgun_sample("talk", SAMPLE_TALK);
	load_outgun_sample("wabounce", SAMPLE_WALLBOUNCE);

	load_outgun_sample("weaponup", SAMPLE_WEAPON_UP);
	load_outgun_sample("megaheal", SAMPLE_MEGAHEALTH);
	load_outgun_sample("shieldp", SAMPLE_SHIELD_PICKUP);
	load_outgun_sample("shieldd", SAMPLE_SHIELD_DAMAGE);
	load_outgun_sample("shieldl", SAMPLE_SHIELD_LOST);
	load_outgun_sample("speedon", SAMPLE_BOOTS_ON);
	load_outgun_sample("speedoff", SAMPLE_BOOTS_OFF);
	load_outgun_sample("quadon", SAMPLE_QUAD_ON);
	load_outgun_sample("quadfire", SAMPLE_QUAD_FIRE);
	load_outgun_sample("quadoff", SAMPLE_QUAD_OFF);
	load_outgun_sample("helmon", SAMPLE_HELM_ON);
	load_outgun_sample("helmoff", SAMPLE_HELM_OFF);

	load_outgun_sample("got", SAMPLE_CTF_GOT);
	load_outgun_sample("lost", SAMPLE_CTF_LOST);
	load_outgun_sample("return", SAMPLE_CTF_RETURN);
	load_outgun_sample("capture", SAMPLE_CTF_CAPTURE);
	load_outgun_sample("gameover", SAMPLE_CTF_GAMEOVER);
}

//unload samples
void Sounds::unload_samples() {
	if (!sound_inited)
		return;
	for (int i = 0; i < NUM_OF_SAMPLES; i++)
		if (sample[i]) {
			destroy_sample(sample[i]);
			sample[i] = 0;
		}
}

//play sample
void Sounds::play(int s) const {
	if (sound_inited && sample[s]) {
		//kill any voice playing that sample
		stop_sample(sample[s]);
		play_sample(sample[s], 255, 127, 1000, false);		//regular play
	}
}

void Sounds::set_theme_dir(const string& dir) {
	themedir = dir;
	if (dir == "-")
		no_theme = true;
}

