CFLAGS = -O2 -Wall -W -Wno-deprecated

OBJS = \
	client.o \
	ConditionVariable.o \
	Mutex.o \
	rudp.o \
	server.o \
	Thread.o \
	Time.o \
	timefunc.o \
	TimerCallback.o \
	Timer.o

PROGNAME = outgun

outgun: $(OBJS)

%.o: %.cpp
	g++ $(CFLAGS) -c -o $@ $<

PHONY: clean

clean:
	rm -f $(OBJS) core
