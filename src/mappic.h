#ifndef MAPPIC_H_INC
#define MAPPIC_H_INC

#include <vector>
#include <string>
#include "utility.h"

class Map;

class Mappic {
	mutable LogSet log;

	std::vector<std::string> smaps;	// server maps

	std::vector<std::string> load_maps(const std::string& dir);

	void find_maps();
	void save_pictures() const;

public:
	Mappic(LogSet logs) : log(logs) { }

	void run();
	
	class Save_error { };
};

#endif // MAPPIC_H_INC

