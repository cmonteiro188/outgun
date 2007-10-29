#! /bin/sh
if [ $# == 2 ]; then
    LANGFILE="$1"
    SOURCEDIR="$2"
else
    echo "syntax: $0 en.txt sourcedir"
    exit 1
fi

WORKDIR="/tmp"
[ -e "$LANGFILE" -a -e "$WORKDIR" ] \
    || { echo "No $LANGFILE or $WORKDIR" >&2; exit 1; }

cat "$LANGFILE" | dos2unix | grep -v '^;.*\|^$\|^English$' | sort > "$WORKDIR/phrases.langfile" \
    || { echo 'Error extracting phrases.langfile' >&2; exit 1; }

sed 's+×+@TIMES@+g;s+\\"+@QUOTEDQ@+g;s+get_text+_+g;s+_("\([^"]*\)"+_("\1"\n_+g' "$SOURCEDIR"/*.h "$SOURCEDIR"/*.cpp \
       | sed -n 's+.*_("\([^"]*\)".*+\1+p' | sed 's+@QUOTEDQ@+"+g;s+@TIMES@+×+g' \
       | sort | uniq > $WORKDIR/phrases.code \
    || { echo "Error extracting phrases.code" >&2; exit 1; }

diff "$WORKDIR/phrases.langfile" "$WORKDIR/phrases.code" || exit 1
exit 0
