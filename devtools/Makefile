CFLAGS = -O2 -W -Wall

NLLIBS = -lNL

.PHONY: all

all: buglistener leetdatalogrd threadlogrd stackdump2text translationdb watsonrd

buglistener : src/buglistener.cpp
	g++ $(CFLAGS) -o $@ $< $(NLLIBS)

leetdatalogrd : src/leetdatalogrd.cpp
	g++ $(CFLAGS) -o $@ $<

threadlogrd : src/threadlogrd.cpp
	g++ $(CFLAGS) -o $@ $<

stackdump2text : src/stackdump2text.cpp
	g++ $(CFLAGS) -o $@ $<

translationdb : src/translationdb.cpp
	g++ $(CFLAGS) -o $@ $<

watsonrd : src/watsonrd.cpp
	g++ $(CFLAGS) -o $@ $<
