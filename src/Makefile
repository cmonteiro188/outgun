LIBS = `allegro-config --libs` -lNL
CFLAGS = -O2 
LDFLAGS =  
OBJS = \
	names.o \
	outgun.o \

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


PROGNAME = ../outgun


outgun: $(OBJS)
	g++ $(LDFLAGS) $(LEENET) -o $(PROGNAME) $(OBJS) $(LIBS)

%.o: %.cpp
	g++ $(CFLAGS) -c -o $@ $<


PHONY: clean

clean:
	rm -f $(OBJS) core
