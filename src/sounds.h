#ifndef SOUNDS_H_INC
#define SOUNDS_H_INC
#include "commont.h"
#include "utility.h"

class Sounds {
public:
	Sounds::Sounds(LogSet logs);
	Sounds::~Sounds();

	bool valid() const;
	bool no_sounds() const { return no_theme; }

	const std::string& theme_name() const;

	void play(int s) const;

	void search_themes(LineReceiver& dst) const;
	void select_theme(const std::string& dir);

private:
	std::string make_theme_path(const std::string& dir);
	void load_theme(const std::string& dir);

	void load_samples(const std::string& path);
	void unload_samples();
	SAMPLE *load_outgun_sample(const std::string& path, const std::string& fname, int slot, bool try_redirect = true, bool reverse = false);

	mutable LogSet log;
	SAMPLE* sample[NUM_OF_SAMPLES];
	bool sample_reverse[NUM_OF_SAMPLES];
	std::string themename;
	al_ffblk themeffblk;	// for al_find*
	bool no_theme;
};

#endif // SOUNDS_H_INC

