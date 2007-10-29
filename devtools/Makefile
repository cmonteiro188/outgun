CFLAGS = -O2 -W -Wall

.PHONY: all

all: stackdump2text translationdb watsonrd

stackdump2text : src/stackdump2text.cpp
	g++ $(CFLAGS) -o $@ $<

translationdb : src/translationdb.cpp
	g++ $(CFLAGS) -o $@ $<

watsonrd : src/watsonrd.cpp
	g++ $(CFLAGS) -o $@ $<
