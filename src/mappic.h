#ifndef MAPPIC_H_INC
#define MAPPIC_H_INC

#include <vector>
#include <string>

class Map;

class Mappic {
	std::vector<std::string> smaps;	// server maps

	std::vector<std::string> Mappic::load_maps(const std::string& dir);

	void find_maps();
	void save_pictures() const;

public:
	Mappic::Mappic() { }

	void run();
};

#endif // MAPPIC_H_INC

