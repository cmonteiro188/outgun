#include <algorithm>
#include "sounds.h"

using std::ifstream;
using std::string;
using std::sort;
using std::vector;

Sounds::Sounds(LogSet logs):
	log(logs),
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

void Sounds::search_themes(LineReceiver& dst) const {
	bool found = false;

	const string searchPattern = wheregamedir + "sound" + directory_separator + "*.*";

	log("Sound theme searching: '%s'", searchPattern.c_str());

	const int attrib = FA_DIREC | FA_ARCH | FA_RDONLY;

 	vector<string> themes;
	struct al_ffblk ffblk;
	for (int error = al_findfirst(searchPattern.c_str(), &ffblk, attrib); !error; error = al_findnext(&ffblk))
		if ((ffblk.attrib & FA_DIREC) && strcmp(ffblk.name, ".") && strcmp(ffblk.name, "..")) {
			themes.push_back(ffblk.name);
			found = true;
		}
	if (!found) {
		dst("<no themes found>");
		return;
	}
	sort(themes.begin(), themes.end());
	for (vector<string>::const_iterator ti = themes.begin(); ti != themes.end(); ++ti)
		dst(*ti);
}

void Sounds::select_theme(const string& dir) {
	load_theme(dir.c_str());
}

void Sounds::load_theme(const string& dir) {
	unload_samples();		//unload old (if any)

	string path = wheregamedir + "sound" + directory_separator + dir + directory_separator;

	load_samples(path);			//load new

	// load sfx theme description
	string des_file = path + "theme.txt";

	ifstream in(des_file.c_str());
	if (!getline_smart(in, themename))
		themename = "(unnamed theme)";
	log("Loaded sound theme '%s'.", dir.c_str());

	//play a sample
	if (!no_theme)
		play(rand() % NUM_OF_SAMPLES);
}

//append the correct path
SAMPLE* Sounds::load_outgun_sample(const string& path, const string& fname, int slot, bool try_redirect, bool reverse) {
	string fileName = path + fname + ".wav";

	//try load
	SAMPLE* ret = sample[slot] = load_sample(fileName.c_str());

	//sample must be played in reverse?
	sample_reverse[slot] = reverse;

	//V0.3.10: if not found, look for .txt redirect
	if (try_redirect && ret == 0) {	// don't go into endless loop
		string textName = path + fname + ".txt";

		ifstream in(textName.c_str());
		if (in) {
			string redir_name;
			getline_smart(in, redir_name);
			in.close();

			bool is_reversed = false;

			//retry once ("false": don't try redirect again if fails)
			return load_outgun_sample(path, redir_name.c_str(), slot, false, is_reversed);
		}
	}

	return ret;
}

//sample try loads
void Sounds::load_samples(const string& path) {
	if (!sound_inited)
		return;
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

