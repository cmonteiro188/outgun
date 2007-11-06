CFLAGS = -O2 -W -Wall

NLLIBS = -lNL

.PHONY: all

all: buglistener leetdatalogrd mastertxt_crc threadlogrd stackdump2text translationdb watsonrd

buglistener : src/buglistener.cpp
	g++ $(CFLAGS) -o $@ $< $(NLLIBS)

leetdatalogrd : src/leetdatalogrd.cpp
	g++ $(CFLAGS) -o $@ $<

mastertxt_crc : src/mastertxt_crc.cpp
	g++ $(CFLAGS) -o $@ $< $(NLLIBS)

threadlogrd : src/threadlogrd.cpp
	g++ $(CFLAGS) -o $@ $<

stackdump2text : src/stackdump2text.cpp
	g++ $(CFLAGS) -o $@ $<

translationdb : src/translationdb.cpp
	g++ $(CFLAGS) -o $@ $<

watsonrd : src/watsonrd.cpp
	g++ $(CFLAGS) -o $@ $<
