#include <cassert>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

using std::map;
using std::string;
using std::vector;

class FileLineReader {
public:
    FileLineReader(FILE* fp) : src(fp) { line[0] = '\0'; }
    int next() {    // returns line length, or -1 if EOF hit with nothing read
        for (int lp = 0;;) {
            assert(lp < maxLineLength);
            const int ch = fgetc(src);
            if (ch == EOF) {
                if (lp == 0)
                    return -1;
                line[lp] = '\0';
                return lp;
            }
            if (ch == '\n') {
                line[lp] = '\0';
                return lp;
            }
            if (ch == '\r') {
                const int peek = fgetc(src);
                ungetc(peek, src);
                if (peek == '\n')
                    continue;
            }
            line[lp++] = ch;
        }
    }
    const char* read() const { return line; }

private:
    enum { maxLineLength = 500 };
    char line[maxLineLength];
    FILE* src;
};

vector<int> countRefs(const string& s) {
    vector<int> counts;
    counts.resize(10);
    string::size_type pos = 0;
    for (;;) {
        pos = s.find('$', pos);
        if (pos == string::npos || pos + 1 >= s.length())
            return counts;
        if (s[pos + 1] >= '0' && s[pos + 1] <= '9') {
            const int idx = s[pos + 1] - '0';
            ++counts[idx];
        }
        ++pos;
    }
}

struct Translations {
    vector<string> credits;
    map<string, string> translations;
};

bool dbimport(FILE* base, FILE* lang, Translations& db, bool verbose) {
    FileLineReader br(base);
    FileLineReader lr(lang);
    // compare first line first, it should be equal
    if (br.next() < 0 || lr.next() < 0 || strcmp(br.read(), lr.read())) {
        printf("Difference: first line.\n");
        return false;
    }
    if (br.read()[0] != ';' || lr.read()[0] != ';') {
        printf("Error: first line not a comment.\n");
        return false;
    }
    db.credits.clear();
    // initial comment, after the first line, is allowed to vary in length and content
    while (br.next() > 0 && br.read()[0] == ';');
    while (lr.next() > 0 && lr.read()[0] == ';')
        db.credits.push_back(lr.read());
    if (br.read()[0] != '\0' || lr.read()[0] != '\0') {
        printf("Error: credits comment not terminated by an empty line.\n");
        return false;
    }
    // the rest should match line by line: comments and empty lines equal
    for (;;) {
        const bool e1 = (br.next() < 0), e2 = (lr.next() < 0);
        if (e1 || e2) {
            if (!e1 || !e2)
                printf("Difference: end\n");
            return e1 && e2;
        }
        const string bline = br.read(), lline = lr.read();
        const bool fullEq1 = (bline[0] == '\0' || bline[0] == ';');
        const bool fullEq2 = (lline[0] == '\0' || lline[0] == ';');
        if (fullEq1 != fullEq2) {
            printf("Difference:\n%s\n%s\n", bline.c_str(), lline.c_str());
            return false;
        }
        if (fullEq1) {
            if (bline != lline) {
                printf("Difference:\n%s\n%s\n", bline.c_str(), lline.c_str());
                return false;
            }
        }
        else {
            if (!lline.compare(0, 9, "@MISSING@")) {
                if (lline.compare(9, string::npos, bline)) {
                    printf("Tagged missing but modified:\n%s\n%s\n", bline.c_str(), lline.c_str());
                    return false;
                }
            }
            else {
                if (verbose && bline == lline)
                    printf("Unmodified but not tagged missing: '%s'\n", bline.c_str());
                if (countRefs(bline) != countRefs(lline)) {
                    printf("$-difference:\n%s\n%s\n", bline.c_str(), lline.c_str()); // not strictly always has to be an error, but so far never used purposefully
                    return false;
                }
                db.translations[bline] = lline;
            }
        }
    }
}

bool dbexport(FILE* base, const Translations& db, FILE* lang, const string& dbFile, bool verbose) {
    FileLineReader br(base);
    if (br.next() < 0 || br.read()[0] != ';')
        return false;
    fprintf(lang, "%s\n", br.read());
    while (br.next() > 0 && br.read()[0] == ';');
    if (br.read()[0] != '\0')
        return false;
    for (vector<string>::const_iterator ci = db.credits.begin(); ci != db.credits.end(); ++ci)
        fprintf(lang, "%s\n", ci->c_str());
    fputc('\n', lang);
    for (;;) {
        if (br.next() < 0)
            return true;
        if (br.read()[0] == '\0' || br.read()[0] == ';')
            fprintf(lang, "%s\n", br.read());
        else {
            const map<string, string>::const_iterator ti = db.translations.find(br.read());
            if (ti == db.translations.end()) {
                fprintf(lang, "@MISSING@%s\n", br.read());
                if (verbose)
                    printf("Missing in %s: '%s'\n", dbFile.c_str(), br.read());
            }
            else
                fprintf(lang, "%s\n", ti->second.c_str());
        }
    }
}

int main(int argc, const char* argv[]) {
    enum { M_Unset, M_Check, M_Import, M_Export } mode = M_Unset;
    bool verbose = false;
    vector<string> filenames;
    bool error = false;
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            if (!strcmp(argv[i], "-v"))
                verbose = true;
            else
                error = true;
        }
        else if (mode == M_Unset) {
            if (!strcmp(argv[i], "check"))
                mode = M_Check;
            else if (!strcmp(argv[i], "import"))
                mode = M_Import;
            else if (!strcmp(argv[i], "export"))
                mode = M_Export;
            else
                error = true;
        }
        else
            filenames.push_back(argv[i]);
    }
    if (error || mode == M_Unset || filenames.size() != (mode == M_Check ? 2 : 3)) {
        fprintf(stderr,
                "syntax: %s [-v] check en.txt lang.txt\n"
                "        %s [-v] import en.txt lang.txt lang.db\n"
                "        %s [-v] export en.txt lang.db lang.txt\n"
                "\n"
                "-v  verbose mode: list phrases missing from lang.db (export) and unmodified untagged phrases (check/import)\n"
                "\n"
                "If lang.db exists in an import, it is first read and imported phrases added to it.\n",
                argv[0], argv[0], argv[0]);
        return 9;
    }
    const string& baseFile = filenames[0];
    const string& langFile = filenames[mode == M_Export ? 2 : 1];
    const string& databaseFile = filenames[mode == M_Import ? 2 : 1]; // with check, 1 is still valid (of course this var is not used)

    FILE* base = fopen(baseFile.c_str(), "rt");
    if (!base) {
        fprintf(stderr, "Can't open '%s' for reading.\n", baseFile.c_str());
        return 5;
    }
    FILE* lang = fopen(langFile.c_str(), mode == M_Export ? "wt" : "rt");
    if (!lang) {
        fprintf(stderr, "Can't open '%s' for %s.\n", langFile.c_str(), mode == M_Export ? "writing" : "reading");
        return 5;
    }
    Translations db;
    if (mode != M_Check) {
        FILE* dbf = fopen(databaseFile.c_str(), "rt");
        if (!dbf) {
            if (mode == M_Export) {
                fprintf(stderr, "Can't open '%s' for reading.\n", databaseFile.c_str());
                return 5;
            }
            printf("Can't open '%s' for reading. Creating a new database.\n", databaseFile.c_str());
        }
        else {
            FileLineReader dbr(dbf);
            for (;;) { // read credits
                if (dbr.next() < 0) {
                    fprintf(stderr, "Premature EOF in %s\n", databaseFile.c_str());
                    return 1;
                }
                if (dbr.read()[0] == '\0')
                    break;
                if (dbr.read()[0] != ';') {
                    fprintf(stderr, "Credits not terminated by an empty line in %s\n", databaseFile.c_str());
                    return 1;
                }
                db.credits.push_back(dbr.read());
            }
            while (dbr.next() >= 0) { // read translations
                const string line = dbr.read();
                if (line.empty())
                    continue;
                const string::size_type sep = line.find("##");
                if (sep == string::npos) {
                    fprintf(stderr, "Invalid data in %s (no ## on line '%s')\n", databaseFile.c_str(), line.c_str());
                    return 1;
                }
                db.translations[line.substr(0, sep)] = line.substr(sep + 2);
            }
            fclose(dbf);
        }
    }
    bool ok;
    if (mode == M_Export)
        ok = dbexport(base, db, lang, databaseFile, verbose);
    else
        ok = dbimport(base, lang, db, verbose);
    fclose(base);
    fclose(lang);
    if (!ok) {
        if (mode == M_Export)
            printf("Error in '%s'. (produced %s not complete)\n", baseFile.c_str(), langFile.c_str());
        else
            printf("'%s' and '%s' don't match as translations.\n", baseFile.c_str(), langFile.c_str());
    }
    else if (mode == M_Import) {
        FILE* dbf = fopen(databaseFile.c_str(), "wt");
        if (!dbf) {
            fprintf(stderr, "Can't open '%s' for writing.\n", databaseFile.c_str());
            return 5;
        }
        for (vector<string>::const_iterator ci = db.credits.begin(); ci != db.credits.end(); ++ci)
            fprintf(dbf, "%s\n", ci->c_str());
        fputc('\n', dbf);
        for (map<string, string>::const_iterator ti = db.translations.begin(); ti != db.translations.end(); ++ti)
            fprintf(dbf, "%s##%s\n", ti->first.c_str(), ti->second.c_str());
        fclose(dbf);
    }
    return ok ? 0 : 1;
}
