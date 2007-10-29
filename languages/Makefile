DBFILES=$(wildcard *.db)
TXTFILES=$(patsubst %.db,%.txt,$(DBFILES))

export: $(TXTFILES)

%.txt : %.db en.txt
	../devtools/translationdb export en.txt $< $@
