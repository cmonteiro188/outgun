#ifndef SOUNDS_H_INC
#define SOUNDS_H_INC
#include "commont.h"

class Sounds {
	SAMPLE* sample[NUM_OF_SAMPLES];
	bool sample_reverse[NUM_OF_SAMPLES];
	string sfxthemedir;
	string sfxthemename;
	al_ffblk sfxthemeffblk;	//for al_find*
	bool validtheme;		// if sfxthemedir points to valid dir
	bool no_theme;

	void make_sfx_theme_path(char* themepath, const char* themedir);
	void set_theme_dir(char* dirname);

	void load_samples();
	void unload_samples();
	SAMPLE *load_outgun_sample(const char* fname, int slot, bool try_redirect = true, bool reverse = false);

public:
	Sounds::Sounds();
	Sounds::~Sounds();

	bool valid() const;
	bool no_sounds() const { return no_theme; }

	const std::string& theme_name() const;
	const std::string& theme_dir() const;

	void play(int s) const;

	void search_themes();
	void next_sfx_theme();

	void set_themedir(const std::string& dir);
};

#endif // SOUNDS_H_INC

