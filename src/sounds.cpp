#include "sounds.h"

using std::string;

Sounds::Sounds():
	validtheme(false),
	no_theme(false)
{
	//no samples loaded -- important so unload_samples don't crash
	for (int i = 0; i < NUM_OF_SAMPLES; i++)
		sample[i] = 0;
}

Sounds::~Sounds() {
	unload_samples();
}

bool Sounds::valid() const {
	return validtheme;
}

const string& Sounds::theme_name() const {
	return sfxthemename;
}

const string& Sounds::theme_dir() const {
	return sfxthemedir;
}

void Sounds::search_themes() {
	//try the last theme directory first
	char themepath[512];
	make_sfx_theme_path(themepath, sfxthemedir.c_str());

	LOG1("theme searching '%s'\n", themepath);

	if (0==al_findfirst(themepath, &sfxthemeffblk, FA_DIREC|FA_ARCH|FA_RDONLY))
		set_theme_dir(0);	// OK: load ; 0 = no change
	else {
		// sound theme not found. find the first one
		make_sfx_theme_path(themepath, "*.*");

		int result = al_findfirst(themepath, &sfxthemeffblk, FA_DIREC|FA_ARCH|FA_RDONLY);
		for (; result==0; result = al_findnext(&sfxthemeffblk))
			if ((sfxthemeffblk.attrib & FA_DIREC) && strcmp(sfxthemeffblk.name, ".") != 0 && strcmp(sfxthemeffblk.name, "..") != 0) {
				set_theme_dir(sfxthemeffblk.name);
				break;
			}
	}
}

void Sounds::next_sfx_theme() {
	char themepath[512];

	//no valid theme, just give up...
	if (!validtheme)
		return;

	bool round1 = true;

	make_sfx_theme_path(themepath, sfxthemedir.c_str());
	while (1) {
		int result = al_findnext(&sfxthemeffblk);
		if (result) {
			//not found, go back to first ones...
			if (!round1) {
				validtheme = false;
				return;
			}
			else
				no_theme = !no_theme;
			if (no_theme) {
				unload_samples();
				return;
			}
			round1 = false;
			make_sfx_theme_path(themepath, "*.*");
			result = al_findfirst(themepath, &sfxthemeffblk, FA_DIREC|FA_ARCH|FA_RDONLY);
			if (result) {
				validtheme = false;
				return;
			}
		}
		if ((sfxthemeffblk.attrib&FA_DIREC) && strcmp(sfxthemeffblk.name, ".")!=0 && strcmp(sfxthemeffblk.name, "..")!=0) {
			set_theme_dir(sfxthemeffblk.name);
			break;
		}
	}
}

void Sounds::make_sfx_theme_path(char* themepath, const char* themedir) {
	char soundname[1024];

	strcpy(soundname, "sound");  //sound/
	put_backslash(soundname);
	strcat(soundname, themedir);  //theme dir name

	char dest[1024];
	append_filename(dest, wheregamedir, soundname, WHERE_PATH_SIZE);

	strcpy(themepath, dest);

	LOG1("make sfx theme path = '%s'\n", themepath);
}

void Sounds::set_theme_dir(char *dirname) {

	if (dirname)
		sfxthemedir = dirname;

	validtheme = true;

	unload_samples();		//unload old (if any)

	load_samples();			//load new

	// load sfx theme description
	//
	char soundname[256];
	strcpy(soundname, "sound");

	put_backslash(soundname);
	strcat(soundname, sfxthemedir.c_str());

	put_backslash(soundname);
	strcat(soundname, "theme.txt");

	char dest[WHERE_PATH_SIZE];
	append_filename(dest, wheregamedir, soundname, WHERE_PATH_SIZE);

	char sfxthemename[256];
	FILE *theme = fopen(dest, "r");
	if (theme) {
		if (fgets(sfxthemename, 256, theme)) {
			sfxthemename[strlen(sfxthemename)-1] =0;
		}
		else
			strcpy(sfxthemename, "(unnamed theme)");
		fclose(theme);
	}
	this->sfxthemename = sfxthemename;

	//play a sample
	if (!no_theme)
		play(rand() % NUM_OF_SAMPLES);
}

//append the correct path
SAMPLE* Sounds::load_outgun_sample(const char *fname, int slot, bool try_redirect, bool reverse) {
	//soundname: add "sound/" to the filename
	char soundname[256];
	strcpy(soundname, "sound");

	//additional: sfx theme dir name
	put_backslash(soundname);
	strcat(soundname, sfxthemedir.c_str());

	put_backslash(soundname);
	strcat(soundname, fname);
	strcat(soundname, ".wav");

	//add soundname to where game dir
	char dest[WHERE_PATH_SIZE];
	append_filename(dest, wheregamedir, soundname, WHERE_PATH_SIZE);

	//try load
	SAMPLE* ret = sample[slot] = load_sample(dest);

	//sample must be played in reverse?
	sample_reverse[slot] = reverse;

	LOG4("load_sample[%i]: '%s' = %p  rev = %i\n", slot, dest, ret, sample_reverse[slot]);

	//V0.3.10: if not found, look for .txt redirect
	if (try_redirect && ret == 0) {	// don't go into endless loop

		//txt filename
		strcpy(soundname, "sound");
		put_backslash(soundname);
		strcat(soundname, sfxthemedir.c_str());
		put_backslash(soundname);
		strcat(soundname, fname);
		strcat(soundname, ".txt");
		append_filename(dest, wheregamedir, soundname, WHERE_PATH_SIZE);

		FILE *f = fopen(dest, "r");
		if (f) {
			char redirwavname[256];
			fscanf(f, "%s", redirwavname);
			bool is_reversed = false;

			fclose(f);

			//retry once ("false": don't try redirect again if fails)
			return load_outgun_sample(redirwavname, slot, false, is_reversed);
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

void Sounds::set_themedir(const string& dir) {
	sfxthemedir = dir;
	if (dir == "-")
		no_theme = true;
}

