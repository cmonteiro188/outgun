LIBS = -lalleg -lNL -lpthreadGC
CFLAGS = -O2 -Wno-deprecated
OBJS = \
	names.o \
	outgun.o \

PUBOBJS = names.o pubsrvog.o

LEENET = \
	leetnet/client.o \
	leetnet/ConditionVariable.o \
	leetnet/Mutex.o \
	leetnet/rudp.o \
	leetnet/server.o \
	leetnet/Thread.o \
	leetnet/timefunc.o \
	leetnet/Time.o \
	leetnet/TimerCallback.o \
	leetnet/Timer.o


all: og pub mon
og: outgun.exe
pub: pubsrvog.exe
mon: srvmonit.exe

outgun.exe: $(OBJS)
	g++ -mconsole $(LEENET) -o $@ $(OBJS) $(LIBS)
	cp outgun.exe ../test

pubsrvog.exe: $(PUBOBJS)
	g++ -mwindows $(LEENET) -o $@ $(PUBOBJS) $(LIBS)
	cp pubsrvog.exe ..

srvmonit.exe: srvmonit.cpp
	g++ -mconsole -o $@ $< -lNL

pubsrvog.o: pubsrvog.cpp
	g++ $(CFLAGS) -DPUBLIC_SERVER -c -o $@ $<

%.o: %.cpp
	g++ $(CFLAGS) -c -o $@ $<


PHONY: clean

clean:
	rm -f $(OBJS) core
