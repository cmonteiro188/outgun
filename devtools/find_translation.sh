#! /bin/sh
if [ $# == 0 -o $# -gt 2 ]; then
    echo "syntax: $0 string-to-find [#-lines-of-context]" >&2
    exit 1
elif [ $# == 1 ]; then
    CONTEXT=
else
    CONTEXT="--context=$2"
fi    

grep $CONTEXT "\(_\|get_text\)(\"$1\"" *.cpp *.h
