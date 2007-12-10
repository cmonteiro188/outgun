#! /bin/sh
set -o pipefail

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

cat "$LANGFILE" | dos2unix | recode latin1.. | egrep -v --line-regexp ';.*||English|locale' | sort > "$WORKDIR/phrases.langfile" \
    || { echo 'Error extracting phrases.langfile' >&2; exit 1; }

cat "$SOURCEDIR"/*.{h,cpp} | recode latin1.. \
       | sed 's+\\"+@QUOTEDQ@+g;s+get_text+_+g;s+_("\([^"]*\)"+_("\1"\n_+g' \
       | sed -n 's+.*_("\([^"]*\)".*+\1+p' | sed 's+@QUOTEDQ@+"+g' \
       | sort | uniq > "$WORKDIR/phrases.code" \
    || { echo "Error extracting phrases.code" >&2; exit 1; }

diff "$WORKDIR/phrases.langfile" "$WORKDIR/phrases.code" || exit 1
exit 0
