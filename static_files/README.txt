*******************************************************************************
          
    OUTGUN

 A 2D-graphics, 32-player multiplayer, fast-paced capture-the-flag game!

*******************************************************************************
                                                      [ 1.0.0 beta 2004-07-10 ]


-------------------------
Project homepages:
-------------------------

The official Outgun site
   http://outgun.sf.net/en/

Outgun development by Nix and Huntta
   http://koti.mbnet.fi/outgun/

Outgun 1.0.0 beta testing page
   http://koti.mbnet.fi/outgun/1.0.0_beta/


-------------------------
Notes for 1.0.0 beta:
-------------------------

This is a beta version. It contains bugs, and will be outdated fast. Use it at
your own risk! See http://koti.mbnet.fi/outgun/1.0.0_beta/ for more information
about the beta, instructions for reporting bugs, and the newest version.

See the HTML help in the doc directory for some information on running the
server and customizing the game. More information will appear there before the
final release, though.

If you want to configure Outgun to understand your keyboard, edit allegro.cfg.

All available maps aren't used on a server by default. You can select from the
cmaps folder the maps you want to use, and copy them to the maps folder.

If you are an author of one of the maps on this package, and want your map
removed from the distribution or updated, or the credits changed, contact us at
outgun@mbnet.fi. Also feel free to send us new maps to be included with the
game. Especially good ones. ;)

This game is free software under GPL. See the file COPYING for more details.
NOTE: during the initial stages of beta, the sources are available only by
asking us, because we don't see distributing the current sources useful. There
are no up to date compilation instructions and they don't compile on Linux yet.
We ask you to be patient and wait for the final release. But we will send them
if you ask, we have to because they're under GPL and not all our code.


-------------------------
CREDITS:
-------------------------

Programming for 0.5.0 by
   Fabio "Spinal" Cecin <fcecin@inf.ufrgs.br> <frcecin@terra.com.br>
   Random Name Routine(TM) by Renato Hentschke <renato@amok.com.br>

Programming for 1.0.0 by
   Niko Ritari (Nix) <npr1@suomi24.fi>
   Jani Rivinoja (Huntta) <janir@mbnet.fi>

Graphics themes by Jani Rivinoja and Joonas Rivinoja.

Sound theme by Visa-Valtteri Pimiä <visa.pimia@www.fi>

Maps by Fabio Cecin, JoL-PHIN, Flyer-DH, Huntta, Juggis, Jakke, evilKaioh,
Devil, MK-Headshot (unBugger) and Luque.

Beta testers: You! Get your name here by reporting a new bug. :)

This game uses the excellent libraries:
 * Allegro - http://alleg.sourceforge.net/
 * HawkNL - http://www.hawksoft.com/hawknl/
 * GNE - http://www.rit.edu/~jpw9607/gne/
 * Pthreads-win32 - http://sources.redhat.com/pthreads-win32/

The executable and DLLs have been packed to almost 50% of their original size
using UPX - http://upx.sourceforge.net/

The HawkNL NL.dll shipped with Outgun is modified by Nix to avoid problems
(sockets that are left open) with at least Windows 98 SE. Sources for the
modified version are at http://koti.mbnet.fi/outgun/HawkNL168src_Nix.zip