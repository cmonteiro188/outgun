#! /bin/bash
echo 'Welcome to a simple build script for Outgun'
if gmake --version >/dev/null 2>&1; then
    MAKE=gmake
else
    MAKE=make
fi
$MAKE -C devtools && $MAKE -C languages && $MAKE -j 3 -C src -f Makefile.common
if [ $? != 0 ]; then
    echo 'Make sure that you have GNU make, G++ and the libraries Allegro and HawkNL installed (-devel packages) and not extremely old versions.'
    exit 1
fi

[ -e build/config ] && { mv build/config{,.bak} || exit 1; }
cp -a --update static_files/* build/
cp -a build/config/gamemod.txt{.default,}
[ -e build/config.bak ] && mv -f build/config{.bak/*,/} && rmdir build/config.bak
cp -a languages/*.txt build/languages/

echo 'Outgun successfully (?) built - try running build/outgun. Have fun!'
