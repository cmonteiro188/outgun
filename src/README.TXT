
WHAT IS THIS ?
=========================================================

These are the source files for the game OUTGUN...
 http://www.inf.ufrgs.br/~fcecin/outgun

...and the network engine skeleton L33TNET
 http://www.inf.ufrgs.br/~fcecin/leetnet

Author:
Fabio "Spinal" Cecin   (frcecin@terra.com.br)

The net engine is in the "leetnet" directory of this package.
At the "game" directory are the sources for the outgun game.

This distribution has Makefile's for both Linux and Visual C++.
You can find the non-Linux pre-arranged project files at the
"lib" directory.

We are constantly updating this pack, check the site for
updates.


HOW TO COMPILE / WHAT IS NEEDED ?
=========================================================

You will need to setup the following libraries to compile
and run the game:

Allegro 4.0 - a cross-platform gaming library
http://alleg.sourceforge.net

HawkNL - a cross-platform network library
http://www.hawksoft.com/hawknl

Pthreads-Win32, if you are compiling to windows, wich doesn't
  support Pthreads 
http://sources.redhat.com/pthreads-win32

Hopefully, with these installed and the game sources, you have
everything needed to build except for the compiler's battle
plan :-)


COMPILING - VISUAL C++
=========================================================

Open the project located at "lib/vc++" directory.
Change your settings if needed, and then compile the 
whole thing.
Voila, there you have outgun ready to go.


COMPILING - LINUX
=========================================================

Just type:

] cd <outgun sources root directory>
] ./configure
] make

... at this point, if everything went well, you already
have the outgun executable at the "game" directory.
If you wish to install it inside your /usr/bin, become
root and type:

] make install

... this way you can just type "outgun" to start the game.


CREDITS !!!!
=========================================================

- Thanks to everybody that plays the game and send all the
  valuable feedback!!!!

- Thanks to all people that supports and runs game servers!!!

- This code is GPL'ed! thanks to Rafael Jannone for the GPL
  packaging and linux distribution of the sources. Also thanks
  to Rafael Jannone to fixing a few windowsisms in the code and
  compiling & testing into linux.

- Thanks to Renato Hentschke for his superb "Random Name
  Generator (TM)" routine. If you use it, be sure to give
  him the proper credits!

- Thanks to Shawn Heargreaves and the Allegro community for
  the Allegro game library

- Thanks to Phil Frisbie for the excellent HawkNL library

- Thanks to Pthreads-win32 project for existing



ANYTHING ELSE ?
=========================================================

Go to the site for updates!!!
 http://www.inf.ufrgs.br/~fcecin/outgun

Everything missing here is available at this address:
 Fabio "Spinal" Cecin: <  frcecin@terra.com.br  >
