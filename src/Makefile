CFLAGS = -W -Wall -O2 -Wno-deprecated
#CFLAGS = -W -Wall -Os -fno-default-inline -Wno-deprecated

ifdef STATIC_ALLEGRO
#ifndef DYNAMIC_ALLEGRO
 LIBS = -lalleg_s -lNL -lpthreadGC -lkernel32 -luser32 -lgdi32 -lcomdlg32 -lole32 -ldinput -lddraw -ldxguid -lwinmm -ldsound
 EXTRA_CFLAGS = -DALLEGRO_STATICLINK
else
 LIBS = -lalleg -lNL -lpthreadGC
endif

OBJS = servnet.o server.o graphics.o commont.o client.o main.o world.o names.o nameauth.o effects.o sounds.o
EXTRA_OBJS = icon.res

LEETNET = leetnet/client.o leetnet/ConditionVariable.o leetnet/Mutex.o \
	leetnet/rudp.o leetnet/server.o leetnet/Thread.o leetnet/timefunc.o \
	leetnet/Time.o leetnet/TimerCallback.o leetnet/Timer.o

all: og
og: outgun.exe

outgun.exe: $(OBJS) $(EXTRA_OBJS)
	g++ -mconsole -s $(LEETNET) -o $@ $(OBJS) $(EXTRA_OBJS) $(LIBS)
	cp outgun.exe ../test
	cp outgun.exe ../client.exe

OBJCOMPILE = g++ $(CFLAGS) $(EXTRA_CFLAGS) -c -o $@ $<
SRCTOUCH = touch $@

%.res: %.rc
	windres -i $< -O coff -o $@

#%.o: %.cpp server.h graphics.h commont.h client.h world.h names.h nameauth.h effects.h sounds.h
#	$(OBJCOMPILE)

world_inc =	world.h
nameauth_inc =	nameauth.h
names_inc =	names.h
admshell_inc =	admshell.h
commont_inc =	commont.h	$(names_inc)	$(admshell_inc)
server_inc =	server.h	$(nameauth_inc)	$(world_inc)
client_inc =	client.h	$(graphics_inc)	$(sounds_inc)	$(effects_inc)	$(world_inc)
graphics_inc =	graphics.h	$(world_inc)	$(sounds_inc)
sounds_inc =	sounds.h	$(commont_inc)
effects_inc =	effects.h	$(commont_inc)	$(graphics_inc)	$(sounds_inc)

server.o:	server.cpp	$(commont_inc)	$(world_inc) 	$(server_inc)
	$(OBJCOMPILE)
servnet.o:	servnet.cpp	$(commont_inc)	$(server_inc)
	$(OBJCOMPILE)
graphics.o:	graphics.cpp	$(commont_inc)	$(graphics_inc)
	$(OBJCOMPILE) -DNIX
commont.o:	commont.cpp	$(commont_inc)
	$(OBJCOMPILE)
client.o:	client.cpp	$(commont_inc)	$(world_inc) 	$(client_inc)
	$(OBJCOMPILE)
main.o: 	main.cpp	$(commont_inc)	$(server_inc)	$(client_inc)
	$(OBJCOMPILE)
world.o:	world.cpp	$(commont_inc)	$(world_inc) 	$(server_inc)	$(client_inc)
	$(OBJCOMPILE)
names.o:	names.cpp
	$(OBJCOMPILE)
nameauth.o:	nameauth.cpp	$(nameauth_inc)
	$(OBJCOMPILE)
effects.o:	effects.cpp	$(effects_inc)
	$(OBJCOMPILE)
sounds.o:	sounds.cpp	$(sounds_inc)
	$(OBJCOMPILE)

PHONY: clean

clean:
	rm -f $(OBJS)
