#include "../mapgen.h"

using std::cout;

int main(int argc, const char* argv[]) {
    if (argc != 3)
        cout << "Usage: mapgen width height\n";
    else {
        srand(time(0));
        cout << "Width " << atoi(argv[1]) << ", height " << atoi(argv[2]) << '\n';
        MapGenerator generator;
        generator.generate(atoi(argv[1]), atoi(argv[2]), false);
        generator.draw(cout);
    }
}
