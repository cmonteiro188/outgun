#include <nl.h>
#include <conio.h>
#include <stdio.h>
#include <assert.h>
#include <windows.h>	// for Sleep
#include "admshell.h"

typedef unsigned char uchar;

class ReadFail { };
class SendFail { };
class ConnectFail { };
class UserExit { };

void send(NLsocket sock, void* data, int len) {
	if (nlWrite(sock, data, len)!=len)
		throw SendFail();
}

class IdleFunction {
public:
	virtual void operator()() =0;
};

class DelayHandler : public IdleFunction {
	enum { sayBufLen=1024 };
	char sayBuf[sayBufLen+1];
	int sayIdx;	// -1 when not typing
	NLsocket sock;

public:
	DelayHandler(NLsocket socket) : sock(socket), sayIdx(-1) { }
	void operator()() {
		while (kbhit()) {
			char buf[12+sayBufLen];
			int idx=0;
			int key=getch();
			if (sayIdx>-1) {
				if (key==27) {
					printf("(aborted)\n");
					sayIdx=-1;
				}
				else if (key=='\r') {
					writeLong(buf, idx, ATS_SERVER_CHAT);
					sayBuf[sayIdx]=0;
					writeString(buf, idx, sayBuf);
					send(sock, buf, idx);
					sayIdx=-1;
					printf("\n");
				}
				else {
					if (key=='\b') {
						if (sayIdx>0) {
							sayIdx--;
							printf("\b \b");
						}
					}
					else if (sayIdx<sayBufLen) {
						printf("%c", key);
						sayBuf[sayIdx++]=key;
					}
				}
			}
			else if (key==27) {
				writeLong(buf, idx, ATS_QUIT);
				send(sock, buf, idx);
				throw UserExit();
			} else if (key>='0' && key<'9') {
				int pid=key-'0';
				writeLong(buf, idx, ATS_GET_PLAYER_FRAGS);
				writeLong(buf, idx, pid);
				writeLong(buf, idx, ATS_GET_PLAYER_TOTAL_TIME);
				writeLong(buf, idx, pid);
				writeLong(buf, idx, ATS_GET_PLAYER_TOTAL_KILLS);
				writeLong(buf, idx, pid);
				writeLong(buf, idx, ATS_GET_PLAYER_TOTAL_DEATHS);
				writeLong(buf, idx, pid);
				writeLong(buf, idx, ATS_GET_PLAYER_TOTAL_CAPTURES);
				writeLong(buf, idx, pid);
				send(sock, buf, idx);
			} else if (key=='S' || key=='s') {
				sayIdx=0;
				printf("Say: ");
			} else if (key=='p' || key=='P') {
				writeLong(buf, idx, ATS_GET_PINGS);
				send(sock, buf, idx);
			}
		}
	}
};

class DelaySocketReader {
	enum { rbufSz=1024 };
	NLsocket sock;
	IdleFunction* ifp;
	char rbuf[rbufSz];
	int rbufUsed, rbufRd;

public:
	DelaySocketReader(NLsocket socket, IdleFunction* handlerFn) : sock(socket), ifp(handlerFn), rbufUsed(0), rbufRd(0) { }
	NLulong getLong() {
		NLulong val=0;
		val=         getByte();
		val=(val<<8)|getByte();
		val=(val<<8)|getByte();
		val=(val<<8)|getByte();
		return val;
	}
	uchar getByte() {
		for (;;) {
			if (rbufRd<rbufUsed)
				return rbuf[rbufRd++];
			rbufUsed=nlRead(sock, rbuf, rbufSz);
			rbufRd=0;
			if (rbufUsed==NL_INVALID)
				throw ReadFail();
			if (rbufUsed==0) {
				(*ifp)();
				Sleep(100);
			}
		}
	}
};

bool runMonitor(int port) {
	NLsocket sock;

	nlDisable(NL_BLOCKING_IO);
	sock=nlOpen(0, NL_RELIABLE);
	assert(sock!=NL_INVALID);

	NLaddress addr;
	nlStringToAddr("127.0.0.1", &addr);
	nlSetAddrPort(&addr, port);

	if (NL_TRUE!=nlConnect(sock, &addr))
		throw ConnectFail();
	char nul;
	while (nlWrite(sock, &nul, 0)!=0) {
		if (nlGetError()!=NL_CON_PENDING)
			throw ConnectFail();
		Sleep(10);
	}
	DelayHandler dh(sock);
	printf("Connected!\n");
  try {
	DelaySocketReader reader(sock, &dh);
	for (;;) {
		NLulong val=reader.getLong();
		if (val<0 || val>=NUMBER_OF_STA) {
			printf("<Invalid STA code: %d>", val);
			continue;
		}
		static const int ints[NUMBER_OF_STA]={ 0, 1, 1, 1, 1, 1, 1, 0, 2, 2, 2, 2, 2, 0, 0, 2, 0 };
		static const int strs[NUMBER_OF_STA]={ 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 2 };
		NLulong ival[2];
		const int strBufLen=1024;
		char strBuf[strBufLen+1];
		for (int ii=0; ii<ints[val]; ++ii)
			ival[ii]=reader.getLong();
		if (strs[val]) {
			int si;
			for (si=0; si<strBufLen; ++si) {
				strBuf[si]=reader.getByte();
				if (strBuf[si]==0)
					break;
			}
			if (si==strBufLen) {
				strBuf[si]=0;
				printf("Too long string: %s...\n", strBuf);
				continue;
			}
		}
		switch (val) {
			case STA_NOOP: printf("<no-op>\n"); break;
			case STA_PLAYER_CONNECTED: printf("Player %d connected\n", ival[0]); break;
			case STA_PLAYER_DISCONNECTED: printf("Player %d disconnected\n", ival[0]); break;
			case STA_PLAYER_NAME_UPDATE: printf("Player %d changes name to \"%s\"\n", ival[0], strBuf); break;
			case STA_PLAYER_KILLS: printf("Player %d gets a kill\n", ival[0]); break;
			case STA_PLAYER_DIES: printf("Player %d dies\n", ival[0]); break;
			case STA_PLAYER_CAPTURES: printf("Player %d captures\n", ival[0]); break;
			case STA_GAME_OVER: printf("Game over\n"); break;
			case STA_PLAYER_FRAGS: printf("Player %d has %d frags\n", ival[0], ival[1]); break;
			case STA_PLAYER_TOTAL_TIME: printf("Player %d has been playing for %d seconds\n", ival[0], ival[1]); break;
			case STA_PLAYER_TOTAL_KILLS: printf("Player %d has %d kills\n", ival[0], ival[1]); break;
			case STA_PLAYER_TOTAL_DEATHS: printf("Player %d has %d deaths\n", ival[0], ival[1]); break;
			case STA_PLAYER_TOTAL_CAPTURES: printf("Player %d has %d captures\n", ival[0], ival[1]); break;
			case STA_GAME_TEXT: printf("Broadcast text: \"%s\"\n", strBuf); break;
			case STA_QUIT: printf("Quit received\n"); nlClose(sock); return true;
			case STA_PLAYER_PING: printf("Player %d has ping %d\n", ival[0], ival[1]); break;
			case STA_ADMIN_MESSAGE:
			{
				const char txtBufLen=strBufLen;
				char cap[strBufLen+100], txt[strBufLen];
				sprintf(cap, "Sayadmin message from %s", strBuf);
				int si;
				for (si=0; si<strBufLen; ++si) {
					txt[si]=reader.getByte();
					if (txt[si]==0)
						break;
				}
				assert(si<strBufLen);
				MessageBox(NULL, txt, cap, MB_OK); break;
				break;
			}
		}
	}
  } catch (UserExit) {
	printf("<User abort>\n");
	nlClose(sock);
	return true;
  } catch (...) {
	try {
		throw;
	} catch (SendFail) {
		printf("<Send failed>\n");
	} catch (ReadFail) {
		printf("<Read failed>\n");
	} catch (ConnectFail) {
		printf("<Connect failed>\n");
	}
	printf("%s\n", nlGetErrorStr(nlGetError()));
	nlClose(sock);
	return false;
  }
}

int main(int argc, const char* argv[]) {
	int port=24500;
	if (argc>1) {
		port=atoi(argv[1]);
		if (argc!=2 || port==0) {
			printf("Arg: [port]\n");
			return 2;
		}
	}
    if (nlInit() == NL_FALSE) {
        printf("ERROR: cannot init HawkNL!\n");
        return 0;
    }
    if (nlSelectNetwork(NL_IP) == NL_FALSE) {
        printf("ERROR: no IP network!\n");
        return 0;
    }
	int r=runMonitor(port)?0:1;
	nlShutdown();
	return r;
}
