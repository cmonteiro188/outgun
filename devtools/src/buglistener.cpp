#include <stdint.h>
#include <string>
#include <nl.h>
#include <cstdio>
#include <ctime>
#include <sstream>

using std::string;

inline void readStr(const char* buf, int& count, std::string& dst) {
    dst.clear();
    while (buf[count])
        dst += buf[count++];
    ++count;
}

#undef readLong
void readLong(void* data, int& pos, uint32_t& val) {
    val = 0;
    for (int i = 0; i < 4; ++i)
        val = val << 8 | ((uint8_t*)data)[pos++];
}

string date_and_time(time_t tt) {
    const tm* tmb = localtime(&tt);
    const int time_w = 20;
    char time_str[time_w + 1];
    strftime(time_str, time_w, "%Y%m%d %H%M%S", tmb);
    return time_str;
}

string date_and_time() {
    return date_and_time(time(0));
}

string itoa(int val) {
    std::ostringstream ss;
    ss << val;
    return ss.str();
}

int inMain(FILE* infile, FILE* outfile) {
    const bool listen = !infile;
    NLsocket sock = NL_INVALID;
    if (listen) {
        nlDisable(NL_BLOCKING_IO);
        sock = nlOpen(24900, NL_UNRELIABLE);
        if (sock == NL_INVALID) {
            printf("Can't open socket on UDP port 24900.\n");
            return 1;
        }
        printf("Listening on UDP port 24900...\n");
    }
    int sequence = 1;
    for (;;) {
        static const int BUFSIZE = 10000;
        char buf[BUFSIZE];
        int amount;
        time_t tt;
        NLaddress fromAddr;
        if (listen) {
            amount = nlRead(sock, buf, BUFSIZE);
            if (amount == NL_INVALID) {
                printf("Read from socket failed.\n");
                return 1;
            }
            if (amount == 0) {
                usleep(999999);
                continue;
            }
            if (amount < 0 || amount > BUFSIZE) {
                printf("Bad amount: %d\n", amount);
                return 1;
            }
            nlGetRemoteAddr(sock, &fromAddr);
            tt = time(0);
        }
        else {
            uint32_t tt_f;
            uint32_t amount_f;
            fread(&tt_f, sizeof(uint32_t), 1, infile);
            fread(&fromAddr, sizeof(NLaddress), 1, infile);
            fread(&amount_f, sizeof(uint32_t), 1, infile);
            tt = tt_f;
            amount = amount_f;
            fread(buf, 1, amount, infile);
            if (feof(infile))
                return 0;
        }
        char nameBuf[NL_MAX_STRING_LENGTH];
        nlAddrToString(&fromAddr, nameBuf);
        const string stime = date_and_time(tt);
        printf("%s  %s: ", stime.c_str(), nameBuf);
        if (outfile) {
            const uint32_t tt_f = static_cast<uint32_t>(tt);
            const uint32_t amount_f = amount;
            fwrite(&tt_f, sizeof(uint32_t), 1, outfile);
            fwrite(&fromAddr, sizeof(NLaddress), 1, outfile);
            fwrite(&amount_f, sizeof(uint32_t), 1, outfile);
            fwrite(buf, 1, amount, outfile);
            fflush(outfile);
        }
        int bufp = 0;
        if (!strcmp(buf, "Outgun")) {
            string game, ver, file;
            uint32_t line;
            unsigned char build;
            readStr(buf, bufp, game);
            readStr(buf, bufp, ver);
            if (ver.substr(0, 6) == "1.0.0b")
                build = '?';
            else {
                readByte(buf, bufp, build);
                if (build == 0)
                    build = '?';
            }
            readStr(buf, bufp, file);
            readLong(buf, bufp, line);
            printf("%s v%s/%c @%s:%d", game.c_str(), ver.c_str(), build, file.c_str(), line);
            if (bufp < amount) {
                string msg;
                readStr(buf, bufp, msg);
                printf(" %s", msg.c_str());
            }
            if (bufp != amount)
                printf(" !!! bad data length !!!");
        }
        else {
            bool trace, uncertain;
            string ignore;
            if (!strcmp(buf, "trace")) {
                trace = true;
                uncertain = false;
                readStr(buf, bufp, ignore);
            }
            else if (!strcmp(buf, "dump")) {
                trace = false;
                uncertain = false;
                readStr(buf, bufp, ignore);
            }
            else {
                trace = false;
                uncertain = true; // it can't be a trace, but it could be something sent by an entirely unrelated application
            }
            if (uncertain)
                printf("assumed ");
            printf("stack %s of %d bytes ", trace ? "trace" : "dump", amount - bufp);
            string fileName = string(trace ? "stacktrace" : "stackdump") + itoa(sequence) + ' ' + stime + ' ' + nameBuf + ".bin";
            if (true) { // if (listen) {
                ++sequence;
                FILE* dumpf = fopen(fileName.c_str(), "wb");
                if (dumpf) {
                    if (trace) {
                        fwrite(buf + bufp, 1, amount - bufp, dumpf);
                        fclose(dumpf);
                        if ((amount - bufp) % 4 != 0)
                            printf("!!! bad data length !!! ");
                    }
                    else {
                        uint32_t address;
                        readLong(buf, bufp, address);
                        while (bufp + 3 < amount) {
                            uint32_t value;
                            readLong(buf, bufp, value);
                            fwrite(&address, sizeof(address), 1, dumpf);
                            fwrite(&value, sizeof(value), 1, dumpf);
                            address += 4;
                        }
                        fclose(dumpf);
                        if (bufp != amount)
                            printf("!!! bad data length !!! ");
                    }
                    printf("saved to %s", fileName.c_str());
                }
                else
                    printf("!!! can't write %s !!!", fileName.c_str());
            }
            else
                printf("saved to something like %s", fileName.c_str());
        }
        printf("\n");
    }
}

int main(int argc, const char* argv[]) {
    bool input = true;
    if (argc > 1) {
        if (argc != 2 || strcmp(argv[1], "-read")) {
            printf("Syntax: %s [-read]\n", argv[0]);
            return 1;
        }
        input = false;
    }
    if (nlInit() == NL_FALSE) {
        printf("ERROR: cannot init HawkNL!\n");
        return 1;
    }
    if (nlSelectNetwork(NL_IP) == NL_FALSE) {
        printf("ERROR: no IP network!\n");
        return 1;
    }
    if (input) {
        FILE* outfile = fopen("buglistenerlog.bin", "ab");
        if (!outfile) {
            printf("Can't open buglistenerlog.bin for append!\n");
            return 1;
        }
        int r = inMain(0, outfile);
        fclose(outfile);
        nlShutdown();
        return r;
    }
    else {
        FILE* infile = fopen("buglistenerlog.bin", "rb");
        if (!infile) {
            printf("Can't open buglistenerlog.bin for reading!\n");
            return 1;
        }
        int r = inMain(infile, 0);
        nlShutdown();
        return r;
    }
}
