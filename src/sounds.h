#ifndef SOUNDS_H_INC
#define SOUNDS_H_INC

#include "incalleg.h"
#include "commont.h"
#include "log.h"
#include "utility.h"

class Sounds {
public:
	Sounds::Sounds(LogSet logs);
	Sounds::~Sounds();

	bool valid() const;

	void play(int s) const;

	void search_themes(LineReceiver& dst) const;
	void select_theme(const std::string& dir);

	bool setEnable(bool enable);
	void setVolume(int vol) { nAssert(vol >= 0 && vol <= 255); volume = vol; }

private:
	bool try_init();

	void load_samples(const std::string& path);
	SAMPLE* load_outgun_sample(const std::string& path, const std::string& fname, int slot, bool try_redirect = true);
	void unload_samples();

	mutable LogSet log;
	SAMPLE* sample[NUM_OF_SAMPLES];

	std::string themedir;
	std::string themename;

	bool enabled;
	bool allegroSoundInitialized;
	int volume;	// 0 to 255
};

#endif // SOUNDS_H_INC

