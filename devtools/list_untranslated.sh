#! /bin/sh
           SEDARGS='s+\\"+@QUOTEDQ@+g'
SEDARGS="$SEDARGS;"'s+\(get_text\|_\)("[^"]*"+\1(@string@+g'
SEDARGS="$SEDARGS;"'s+#include "[^"]*"+#include @string@+g'
SEDARGS="$SEDARGS;"'s+\(log\|logThreadStart\|logThreadExit\)(".*$+\1(@...+g'
SEDARGS="$SEDARGS;"'s+""+@e@+g'
if [ $# == 0 ]; then
    grep '"' *.h *.cpp | sed "$SEDARGS" | grep '"' | sed 's+@QUOTEDQ@+\\"+g'
else
    grep '"' "$@"      | sed "$SEDARGS" | grep '"' | sed 's+@QUOTEDQ@+\\"+g'
fi
