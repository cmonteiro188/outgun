#include <cstdarg>
#include <stdio.h>
#include <string>

class EOF_Hit { };

class StrError {
    std::string str;

public:
    StrError(const char* fmt, ...) {
        char buf[10000];
        va_list args;
        va_start(args, fmt);
        vsprintf(buf, fmt, args);
        va_end(args);
        str = buf;
    }
    void print() const {
        fprintf(stderr, "Error: %s\n", str.c_str());
    }
};

inline char getChar(FILE* src) {
    char ch;
    fread(&ch, sizeof(char), 1, src);
    if (feof(src))
        throw EOF_Hit();
    return ch;
}

/** Skip through source file until '\n#include "' is found.
 * @throws EOF_Hit End of file before encountering an #include.
 */
void skipToInclude(FILE* src) {
    static const char matchStr[] = "\n#include \"";
    static const int matchLen = 11;
    // note: the first match character isn't repeated in the match string
    //       that means we don't have to rewind at any point
    for (;;) {
        if (getChar(src) != matchStr[0])
            continue;
        for (int match = 1;; ++match) {
            if (match == matchLen)
                return; // found
            if (getChar(src) == matchStr[match])
                continue;
            else if (getChar(src) == matchStr[0])
                match = 0;
            else
                break;
        }
    }
}

std::string readIncludeFileName(FILE* src) {
    std::string name;
    for (;;) {
        char ch = getChar(src);
        if (ch == '"')
            break;
        name += ch;
    }
    if (name.length() < 3 || name.substr(name.length() - 2) != ".h")
        throw StrError("'#include \"%s\"' - should be \"something.h\"", name.c_str());
    name.erase(name.length() - 2);
    return name;
}

/** Replace all 'from' characters by 'to'.
 */
void replaceChars(std::string& str, char from, char to) {
    for (std::string::size_type i = 0; i < str.size(); ++i)
        if (str[i] == from)
            str[i] = to;
}

void replaceSlashes(std::string& str) { replaceChars(str, '/', '_'); }

/** Translate the source file's #include directives into makefile format. Skip other lines.
 * @param dirbase The relative (to dir of Makefile) directory of the source file, empty if it is the same directory; no slash at the end in any case.
 */
void handleFile(FILE* src, FILE* dst, const std::string& dirbase) {
    try {
        for (;;) {
            skipToInclude(src);
            std::string fileName = readIncludeFileName(src);
            std::string path = dirbase;
            int nameReadPos = 0;
            while (fileName.substr(nameReadPos, 3) == "../") {
                nameReadPos += 3;
                std::string::size_type pathsep = path.find_last_of('/');
                if (pathsep == std::string::npos) {
                    if (path.empty())
                        throw StrError("'#include \"%s\"' - invalid parent reference past base directory", fileName.c_str());
                    pathsep = 0;
                }
                path.erase(pathsep);
            }
            if (!path.empty())
                path += '/';
            path += fileName.substr(nameReadPos);
            replaceSlashes(path);
            fprintf(dst, "\t$(%s_inc)", path.c_str());
        }
    } catch (EOF_Hit) {
        fprintf(dst, "\n");
        return;
    }
}

void handleFile(std::string name, FILE* dst, const std::string& compileCommand) {
    replaceChars(name, '\\', '/');  // for DOS users getting '\'s in their paths
    std::string id = name;
    if (id.length() > 2 && id.substr(id.length() - 2) == ".h") {
        replaceSlashes(id);
        id.erase(id.length() - 2);
        fprintf(dst, "%s_inc =\t%s", id.c_str(), name.c_str());
    }
    else if (id.length() > 4 && id.substr(id.length() - 4) == ".cpp") {
        id.erase(id.length() - 4);
        fprintf(dst, "%s.o:\t%s", id.c_str(), name.c_str());
    }
    else
        throw StrError("'%s' - only .h and .cpp files are handled");
    FILE* src = fopen(name.c_str(), "rb");
    if (!src)
        throw StrError("'%s' - can't open for reading");
    std::string path;
    std::string::size_type pathsep = name.find_last_of('/');
    if (pathsep != std::string::npos)
        path = name.substr(0, pathsep);
    try {
        handleFile(src, dst, path);
    } catch (...) {
        fclose(src);
        throw;
    }
    if (!compileCommand.empty())
        fprintf(dst, "\t%s\n", compileCommand.c_str());
}

int main(int argc, const char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "args: %s [\"Compile command\"] .h-or-.cpp-files\n", argv[0]);
        return 1;
    }
    try {
        for (int i = 2; i < argc; ++i)
            handleFile(argv[i], stdout, argv[1]);
    } catch (const StrError& e) {
        e.print();
        return 1;
    }
    return 0;
}
