#include <algorithm>
#include "sounds.h"

using std::ifstream;
using std::string;
using std::sort;
using std::vector;

Sounds::Sounds(LogSet logs):
	log(logs),
	enabled(false),
	allegroSoundInitialized(false),
	volume(255)
{
	//no samples loaded -- important so unload_samples don't crash
	for (int i = 0; i < NUM_OF_SAMPLES; i++)
		sample[i] = 0;
}

Sounds::~Sounds() {
	unload_samples();
}

void Sounds::search_themes(LineReceiver& dst) const {
	const string searchPattern = wheregamedir + "sound" + directory_separator + "*.*";

	log("Sound theme searching: '%s'", searchPattern.c_str());

	const int attrib = FA_DIREC | FA_ARCH | FA_RDONLY;

 	vector<string> themes;
	struct al_ffblk ffblk;
	for (int error = al_findfirst(searchPattern.c_str(), &ffblk, attrib); !error; error = al_findnext(&ffblk))
		if ((ffblk.attrib & FA_DIREC) && strcmp(ffblk.name, ".") && strcmp(ffblk.name, ".."))
			themes.push_back(ffblk.name);
	if (themes.empty()) {
		dst("<no themes found>");
		return;
	}
	sort(themes.begin(), themes.end());
	for (vector<string>::const_iterator ti = themes.begin(); ti != themes.end(); ++ti)
		dst(*ti);
}

void Sounds::select_theme(const string& dir) {
	unload_samples();

	themedir = dir;

	if (dir == "<no themes found>") {
		themename.clear();
		return;
	}

	string path = wheregamedir + "sound" + directory_separator + dir + directory_separator;

	ifstream in((path + "theme.txt").c_str());
	if (!getline_smart(in, themename))
		themename = "(unnamed theme)";

	if (enabled) {
		load_samples(path);
		log("Loaded sound theme '%s'.", dir.c_str());
		play(rand() % NUM_OF_SAMPLES);
	}
}

bool Sounds::setEnable(bool enable) {
	if (enable == enabled)
		return true;
	if (enable) {
		if (!try_init())
			return false;
		enabled = true;
		select_theme(themedir);
	}
	else {
		unload_samples();
		enabled = false;
	}
	return true;
}

bool Sounds::try_init() {
	if (allegroSoundInitialized)
		return true;
	if (install_sound(DIGI_AUTODETECT, MIDI_NONE, 0)) {
		log("INSTALL_SOUND failed. no sound.");
		return false;
	}
	else {
		log("INSTALL_SOUND ok.");
		allegroSoundInitialized = true;
		return true;
	}
}

void Sounds::load_samples(const string& path) {
	if (!enabled)
		return;
	nAssert(allegroSoundInitialized);

	load_outgun_sample(path, "fire", SAMPLE_FIRE);
	load_outgun_sample(path, "hit", SAMPLE_HIT);
	load_outgun_sample(path, "wallhit", SAMPLE_WALLHIT);
	load_outgun_sample(path, "qwallhit", SAMPLE_QUADWALLHIT);

	load_outgun_sample(path, "getdb", SAMPLE_GETDEATHBRINGER);
	load_outgun_sample(path, "usedb", SAMPLE_USEDEATHBRINGER);
	load_outgun_sample(path, "hitdb", SAMPLE_HITDEATHBRINGER);
	load_outgun_sample(path, "diedb", SAMPLE_DIEDEATHBRINGER);

	load_outgun_sample(path, "death1", SAMPLE_DEATH);
	load_outgun_sample(path, "death2", SAMPLE_DEATH_2);

	load_outgun_sample(path, "entergam", SAMPLE_ENTERGAME);
	load_outgun_sample(path, "leftgam", SAMPLE_LEFTGAME);
	load_outgun_sample(path, "chanteam", SAMPLE_CHANGETEAM);
	load_outgun_sample(path, "talk", SAMPLE_TALK);
	load_outgun_sample(path, "wabounce", SAMPLE_WALLBOUNCE);
	load_outgun_sample(path, "plbounce", SAMPLE_PLAYERBOUNCE);

	load_outgun_sample(path, "weaponup", SAMPLE_WEAPON_UP);
	load_outgun_sample(path, "megaheal", SAMPLE_MEGAHEALTH);
	load_outgun_sample(path, "shieldp", SAMPLE_SHIELD_PICKUP);
	load_outgun_sample(path, "shieldd", SAMPLE_SHIELD_DAMAGE);
	load_outgun_sample(path, "shieldl", SAMPLE_SHIELD_LOST);
	load_outgun_sample(path, "speedon", SAMPLE_BOOTS_ON);
	load_outgun_sample(path, "speedoff", SAMPLE_BOOTS_OFF);
	load_outgun_sample(path, "quadon", SAMPLE_QUAD_ON);
	load_outgun_sample(path, "quadfire", SAMPLE_QUAD_FIRE);
	load_outgun_sample(path, "quadoff", SAMPLE_QUAD_OFF);
	load_outgun_sample(path, "helmon", SAMPLE_HELM_ON);
	load_outgun_sample(path, "helmoff", SAMPLE_HELM_OFF);

	load_outgun_sample(path, "got", SAMPLE_CTF_GOT);
	load_outgun_sample(path, "lost", SAMPLE_CTF_LOST);
	load_outgun_sample(path, "return", SAMPLE_CTF_RETURN);
	load_outgun_sample(path, "capture", SAMPLE_CTF_CAPTURE);
	load_outgun_sample(path, "gameover", SAMPLE_CTF_GAMEOVER);

	load_outgun_sample(path, "5_min", SAMPLE_5_MIN_LEFT);
	load_outgun_sample(path, "1_min", SAMPLE_1_MIN_LEFT);

	load_outgun_sample(path, "spree", SAMPLE_KILLING_SPREE);
}

SAMPLE* Sounds::load_outgun_sample(const string& path, const string& fname, int slot, bool try_redirect) {
	string fileName = path + fname + ".wav";

	SAMPLE* ret = sample[slot] = load_sample(fileName.c_str());

	if (try_redirect && ret == 0) {	// if not found, look for .txt redirect
		string textName = path + fname + ".txt";

		ifstream in(textName.c_str());
		if (in) {
			string redir_name;
			getline_smart(in, redir_name);
			in.close();

			return load_outgun_sample(path, redir_name.c_str(), slot, false);	// no more redirections (avoid endless loops)
		}
	}

	return ret;
}

void Sounds::unload_samples() {
	if (!allegroSoundInitialized)
		return;
	for (int i = 0; i < NUM_OF_SAMPLES; i++)
		if (sample[i]) {
			destroy_sample(sample[i]);
			sample[i] = 0;
		}
}

void Sounds::play(int s) const {
	if (enabled && sample[s]) {
		nAssert(allegroSoundInitialized);
		stop_sample(sample[s]);	// kill any voice playing that sample
		play_sample(sample[s], volume, 127, 1000, false);	// regular play
	}
}

