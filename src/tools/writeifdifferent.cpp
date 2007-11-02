#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using std::cerr;
using std::ifstream;
using std::ios;
using std::ofstream;
using std::ostringstream;
using std::string;

int main(int argc, const char* argv[]) {
    if (argc <= 2) {
        cerr << "syntax: " << argv[0] << " <targetfile> <new file content string>\n";
        return 2;
    }
    const string filename = argv[1];
    string newContent;
    for (int argi = 2; argi < argc; ++argi) {
        if (argi > 2)
            newContent += ' ';
        newContent += argv[argi];
    }
    newContent += '\n';

    {
        ifstream fi(filename.c_str());
        ostringstream fileContent;
        std::copy(std::istreambuf_iterator<char>(fi), std::istreambuf_iterator<char>(), std::ostreambuf_iterator<char>(fileContent));
        if (fileContent.str() == newContent)
            return 0;
    }

    ofstream fo(filename.c_str());
    if (!fo) {
        cerr << argv[0] << ": can't open " << filename << " for writing\n";
        return 1;
    }
    std::copy(newContent.begin(), newContent.end(), std::ostreambuf_iterator<char>(fo));
    return 0;
}
