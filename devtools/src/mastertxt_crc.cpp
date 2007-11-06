#include <cstdio>
#include <nl.h>

int main(int argc, const char* argv[]) {
    if (nlInit() == NL_FALSE) {
        fprintf(stderr, "Can't init HawkNL\n");
        return 1;
    }

    for (int argi = 1; argi < argc; ++argi) {
        FILE *fp = fopen(argv[argi], "rb");
        if (!fp) {
            fprintf(stderr, "Can't open %s\n", argv[argi]);
            return 1;
        }
        static const int bufSize = 1024;    // the first kbyte should be enough to distinguish versions, even if the file at some point gets this large
        NLubyte buf[bufSize];
        const int numread = fread(buf, 1, bufSize, fp);
        fclose(fp);
        printf("%s: %u\n", argv[argi], nlGetCRC16(buf, numread));
    }
}
