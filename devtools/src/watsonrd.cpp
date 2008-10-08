#include <stdint.h>
#include <string>
#include <vector>
#include <list>
#include <iostream>
#include <algorithm>

#include <cassert>
#include <cstdio>
#include <cstring>

using namespace std;

const bool portuguese = false;

struct Frame {
    uint64_t ebp;
    uint64_t eip;
    list<uint64_t> args;
    string subName;
};

struct ResolveAddress {
    uint64_t eip;
    string* dst;
    ResolveAddress() { assert(0); }
    ResolveAddress(uint64_t eip_, string* dst_) : eip(eip_), dst(dst_) { }
    bool operator<(const ResolveAddress& o) const { return eip < o.eip; }
};

class Error {
    string desc;

public:
    Error() { }
    Error(const string& text) : desc(text) { }
    const string& getDesc() const { return desc; }
};

void searchFile(FILE* src, const char* search, int slen) {
    int found = 0;
    for (;;) {
        char ch;
        fread(&ch, 1, 1, src);
        if (feof(src))
            throw Error("'" + string(search, slen) + "' not found");
        if (found && ch != search[found])
            found = 0;
        if (ch == search[found]) {
            if (++found == slen)
                return;
        }
        else
            found = 0;
    }
}

void searchFile(FILE* src, const char* search) {
    searchFile(src, search, strlen(search));
}

void searchFile(FILE* src, const string& search) {
    searchFile(src, search.data(), search.length());
}

void searchFile(FILE* src, const char search) {
    for (;;) {
        char ch = fgetc(src);
        if (feof(src))
            throw Error(string() + search + " not found");
        if (ch == search)
            return;
    }
}

vector<Frame> readWatsonLogW98(FILE* src) {
    vector<Frame> frames;
    searchFile(src, "\0Command line: ", 15);
    searchFile(src, "\neip=");
    Frame fr;
    {
        unsigned eip, ebp;
        const int n = fscanf(src, "%X esp=%*X ebp=%X", &eip, &ebp);
        fr.eip = eip;
        fr.ebp = ebp;
        if (n != 2)
            throw Error("Invalid registers line");
    }
    frames.push_back(fr);
    searchFile(src, "\n-- stack dump --");
    uint64_t prevEbp = fr.ebp;
    for (;;) {
        Frame fr;
        char buf[20];
        sprintf(buf, "\n%08x ", (unsigned)prevEbp);
        try {
            searchFile(src, buf);
        } catch (const Error&) {
            break;
        }
        {
            unsigned ebp;
            const int n = fscanf(src, "%X", &ebp);
            fr.ebp = ebp;
            if (n != 1)
                throw Error("Invalid stack dump line");
        }
        sprintf(buf, "\n%08x ", (unsigned)prevEbp + 4);
        searchFile(src, buf);
        {
            unsigned eip;
            const int n = fscanf(src, "%X", &eip);
            fr.eip = eip;
            if (n != 1)
                throw Error("Invalid stack dump line");
        }
        frames.push_back(fr);
        prevEbp = fr.ebp;
    }
    return frames;
}

string unicodeExpand(const string& text) {
    string ret;
    for (unsigned i = 0; i < text.length(); ++i) {
        ret += text[i];
        ret += '\0';
    }
    return ret;
}

string unicodeContract(FILE* fp) {  // read a line
    string ret;
    for (;;) {
        char ch1, ch2;
        fread(&ch1, 1, 1, fp);
        fread(&ch2, 1, 1, fp);
        if (feof(fp))
            throw Error("End of file reading an unicode line");
        if (ch1 == '\n' && ch2 == 0)
            return ret;
        ret += ch1;
    }
}

vector<Frame> readWatsonLogWXP(FILE* src) {
    vector<Frame> frames;
    for (;;) {  // find the last crash in the file that may contain multiple
        uint64_t fPos = ftell(src);
        try {
            searchFile(src, unicodeExpand(portuguese ? "FALHA ->" : "FAULT ->"));
        } catch (const Error&) {
            if (fPos == 0)
                throw;
            fseek(src, fPos, SEEK_SET);
            break;
        }
    }
    Frame fr;
    string ln = unicodeContract(src);
    {
        unsigned eip;
        const int n = sscanf(ln.c_str(), "%X", &eip);
        fr.eip = eip;
        if (n != 1)
            throw Error("Invalid address on FAULT line");
    }
    searchFile(src, unicodeExpand("ChildEBP RetAddr  Args to Child              \r\n"));
    for (;;) {
        string ln = unicodeContract(src);
        if (ln == "\r" || ln.empty())
            break;
        unsigned ebp;
        unsigned newEip;
        unsigned args[3];
        int n = sscanf(ln.c_str(), "%X %X %X %X %X", &ebp, &newEip, &args[0], &args[1], &args[2]);
        fr.ebp = ebp;
        if (n < 2)
            throw Error("Invalid format in stack trace");
        n -= 2; // get number of arguments
        for (int ai = 0; ai < n; ++ai)
            fr.args.push_back(args[ai]);
        frames.push_back(fr);
        fr = Frame();
        fr.eip = newEip;
    }
    return frames;
}

struct StackEntry {
    uint64_t address, value;
    bool read(FILE* src, bool bits64) {
        address = value = 0;
        fread(&address, bits64 ? 8 : 4, 1, src);
        fread(&value, 4, 1, src);
        if (bits64) {
            uint64_t addr2;
            fread(&addr2, 8, 1, src);
            if (feof(src))
                return false;
            if (addr2 != address + 4)
                throw Error("Bad address data or half quadword missing");
            uint32_t value2;
            fread(&value2, 4, 1, src);
            value |= (uint64_t(value2) << 32);
        }
        return !feof(src);
    }
};

uint64_t unsignedAbsDiff(uint64_t val1, uint64_t val2) {
    if (val1 > val2)
        return val1 - val2;
    else
        return val2 - val1;
}

vector<Frame> readStackDump(FILE* src, int offset, bool bits64) {
    vector<Frame> frames;
    Frame fr;
    StackEntry se;
    while (offset >= 0) {
        if (!se.read(src, bits64))
            throw Error("No first EBP found");
        if (unsignedAbsDiff(se.address, se.value) < 20000)  // a likely first stored EBP
            --offset;
    }
    fr.ebp = se.value;
    if (!se.read(src, bits64))
        throw Error("No first EIP found");
    fr.eip = se.value;
    frames.push_back(fr);
    uint64_t prevEbp = fr.ebp;
    list<uint64_t> trashVec;
    for (;;) {
        Frame fr;
        const int argFrame = frames.size() - (bits64 ? 1 : 2);
        list<uint64_t>& args = (argFrame >= 0 ? frames[argFrame].args : trashVec);
        for (;;) {
            if (!se.read(src, bits64))
                break;
            if (se.address == prevEbp)
                break;
            if (args.size() < (bits64 ? 100 : 10)) {
                if (bits64)
                    args.push_front(se.value);
                else
                    args.push_back(se.value);
            }
        }
        fr.ebp = se.value;
        if (!se.read(src, bits64))
            break;
        fr.eip = se.value;
        frames.push_back(fr);
        prevEbp = fr.ebp;
    }
    return frames;
}

void resolveNames(const vector<ResolveAddress>& resolves, FILE* src) {
    string section;
    vector<ResolveAddress>::const_iterator resolve = resolves.begin();
    if (resolve == resolves.end())
        return;
    for (;;) {
        unsigned long long address;
        int n = fscanf(src, "%LX", &address);
        if (feof(src))
            break;
        if (n == 1) {
            char ch = fgetc(src);
            if (ch == ' ') {
                char endch;
                char sectionBuf[200];
                int n = fscanf(src, "<%199[^>]>:%c", sectionBuf, &endch);
                if (n == 2 && endch == '\n') {
                    section = sectionBuf;
                    continue;
                }
            }
            else if (ch == ':' && address >= resolve->eip) {
                if (address == resolve->eip)
                    *resolve->dst = section;
                if (++resolve == resolves.end())
                    return;
            }
        }
        searchFile(src, '\n');
    }
}

string fmt(uint64_t val, bool compress = false) {
    string str;
    bool zeroes = !compress;
    for (int partBase = val > 0xFFFFFFFF ? 48 : 16; ; partBase -= 16) {
        const unsigned part = (val >> partBase) & 0xFFFF;
        if (part == 0 && !zeroes) {
            if (partBase == 0)
                return "0";
            continue;
        }
        if (part == 0 && compress)
            str += '0';
        else {
            static const char digits[] = "0123456789ABCDEF";
            for (int digitBase = 12; digitBase >= 0; digitBase -= 4) {
                const int digit = (part >> digitBase) & 0xF;
                if (!zeroes) {
                    if (digit == 0)
                        continue;
                    zeroes = true;
                }
                str += digits[digit];
            }
        }
        if (partBase == 0)
            break;
        str += ':';
    }
    return str;
}

enum Mode { M_Unset, M_StackDump, M_Watson98, M_WatsonXP, M_Error };

void setMode(Mode& var, Mode value) {
    var = (var == M_Unset || var == value) ? value : M_Error;
}

bool matchRight(const std::string& str, const std::string& match) {
    return str.length() >= match.length() && !str.compare(str.length() - match.length(), match.length(), match);
}

void emitFilePosition(FILE* fp, bool printable) {
    cerr << "At " << ftell(fp) << ": ";
    for (int i = 0; i < 40; ++i) {
        const int c = fgetc(fp);
        if (c == EOF) {
            cerr << "<EOF>";
            break;
        }
        if (printable)
            cerr << (char)c;
        else {
            if (i)
                cerr << ' ';
            cerr << c;
        }
    }
    cerr << '\n';
}

int main(int argc, const char* argv[]) {
    Mode mode = M_Unset;
    int offset = 0;
    bool bits64 = false;
    bool showArgs = true;
    string dumpFile, disasmFile;
    for (int argi = 1; argi < argc; ++argi) {
        if (argv[argi][0] == '@') {
            offset = atoi(&argv[argi][1]);
            setMode(mode, M_StackDump);
        }
        else if (!strcmp(argv[argi], "-64")) {
            bits64 = true;
            setMode(mode, M_StackDump);
        }
        else if (!strcmp(argv[argi], "-brief"))
            showArgs = false;
        else if (!strcmp(argv[argi], "-dump"))
            setMode(mode, M_StackDump);
        else if (!strcmp(argv[argi], "-w98"))
            setMode(mode, M_Watson98);
        else if (!strcmp(argv[argi], "-wxp"))
            setMode(mode, M_WatsonXP);
        else if (dumpFile.empty())
            dumpFile = argv[argi];
        else if (disasmFile.empty())
            disasmFile = argv[argi];
        else
            mode = M_Error;
    }
    if (mode == M_Error || disasmFile.empty()) {
        cerr << "Syntax: " << argv[0] << " [-dump|-w98|-wxp] [-brief] [-64] [@<offset>] (<watson-log>|<stack-dump>) <disassembly>\n"
                "\n"
                "-dump forces stackdump mode.\n"
                "-w98 forces Windows 98 Dr.Watson mode.\n"
                "-wxp forces Windows XP Dr.Watson mode.\n"
                "If none of the above is specified, the mode may be guessed from the log or dump filename (*stackdump.bin, or *drwtsn32.log for XP, else 98) or forced by stackdump-only arguments.\n"
                "\n"
                "-brief omits function argument list from output.\n"
                "(stackdump-only) -64 activates x86_64 mode. Up to 100 qwords from the bottom of stack frame are shown in place of argument list.\n"
                "(stackdump-only) @<offset> skips the first <offset> autodetected starting points.\n";
        return 9;
    }
    if (mode == M_Unset) {
        if (matchRight(dumpFile, "stackdump.bin"))
            mode = M_StackDump;
        else if (matchRight(dumpFile, "drwtsn32.log"))
            mode = M_WatsonXP;
        else
            mode = M_Watson98;
    }
    vector<Frame> frames;
    if (mode == M_StackDump) {
        FILE* fp = fopen(dumpFile.c_str(), "rb");
        if (!fp) {
            cerr << "Can't read " << dumpFile << '\n';
            return 1;
        }
        try {
            frames = readStackDump(fp, offset, bits64);
        } catch (const Error& e) {
            cerr << "Format error reading stack dump " << dumpFile << ": " << e.getDesc() << '\n';
            emitFilePosition(fp, false);
            return 2;
        }
        fclose(fp);
    }
    else {
        FILE* fp = fopen(dumpFile.c_str(), "rb");
        if (!fp) {
            cerr << "Can't read " << dumpFile << '\n';
            return 1;
        }
        try {
            if (mode == M_WatsonXP)
                frames = readWatsonLogWXP(fp);
            else
                frames = readWatsonLogW98(fp);
        } catch (const Error& e) {
            cerr << "Format error reading Win" << (mode == M_WatsonXP ? "XP" : "98") << " Dr.Watson log " << dumpFile << ": " << e.getDesc() << '\n';
            emitFilePosition(fp, true);
            return 2;
        }
        fclose(fp);
    }

    vector<ResolveAddress> resolves;
    resolves.reserve(frames.size());
    for (vector<Frame>::iterator fi = frames.begin(); fi != frames.end(); ++fi)
        resolves.push_back(ResolveAddress(fi->eip, &fi->subName));
    sort(resolves.begin(), resolves.end());

    FILE* fp = fopen(disasmFile.c_str(), "rt");
    if (!fp) {
        cerr << "Can't read " << disasmFile << '\n';
        return 1;
    }
    try {
        resolveNames(resolves, fp);
    } catch (const Error& e) {
        cerr << "Format error reading disassembly " << disasmFile << ": " << e.getDesc() << '\n';
        emitFilePosition(fp, true);
        return 2;
    }
    fclose(fp);

    for (vector<Frame>::const_iterator fi = frames.begin(); fi != frames.end(); ++fi) {
        const Frame& f = *fi;
        cout << "xBP=" << fmt(f.ebp) << " xIP=" << fmt(f.eip) << ' ' << f.subName;
        if (showArgs)
            for (list<uint64_t>::const_iterator ai = f.args.begin(); ai != f.args.end(); ++ai)
                cout << ' ' << fmt(*ai, true);
        cout << '\n';
    }

    return 0;
}
