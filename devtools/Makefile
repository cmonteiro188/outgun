CFLAGS = -O2 -W -Wall

NLLIBS = -lNL

.PHONY: all

all: buglistener stackdump2text translationdb watsonrd

buglistener : src/buglistener.cpp
	g++ $(CFLAGS) -o $@ $< $(NLLIBS)

stackdump2text : src/stackdump2text.cpp
	g++ $(CFLAGS) -o $@ $<

translationdb : src/translationdb.cpp
	g++ $(CFLAGS) -o $@ $<

watsonrd : src/watsonrd.cpp
	g++ $(CFLAGS) -o $@ $<
