CFLAGS = -O2 -W -Wall
NLLIBS = -lNL

# this is only needed if nl.h is not already in the include path
HAWKNL_INCLUDE_DIR = /usr/include/hawknl
# the above is from a Fedora RPM install; try 'locate /nl.h'
ifeq ($(wildcard $(HAWKNL_INCLUDE_DIR)),$(HAWKNL_INCLUDE_DIR))
 CFLAGS += -I $(HAWKNL_INCLUDE_DIR)
endif

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
