********************************************************************************
          
    OUTGUN

 A 2D-graphics, 32-player multiplayer, fast-paced capture-the-flag game!

********************************************************************************


-------------------------
Project homepages:
-------------------------

 The official Outgun site

	http://www.amok.com.br/outgun/en/

 Outgun development by Nix and Huntta

	http://koti.mbnet.fi/npr/outgun/


-------------------------
Notes for 0.5.0-E:
-------------------------

 This release is based on and compatible with 0.5.0 by Spinal. All code since
   0.5.0 is by Nix (more) and Huntta (less).

 There may be bugs both old and new. Please report them to Nix <npr1@suomi24.fi>
   after checking http://koti.mbnet.fi/npr/outgun/ for newer versions.

 If you want to configure Outgun to understand your keyboard, edit allegro.cfg.

 This version has a new maploader function that detects some map file errors
   that the 0.5.0 doesn't. To encourage correctness, such maps can not be used
   on a server with this version. Playing on erroneus maps in other servers is
   not restricted. Some of the maps distributed with 0.5.0 have these problems,
   so do use those that came with 0.5.0-E. If a map does not load, look at
   out_svr.log (dedicated servers) or out_cli.log (listen servers) to see which
   line is the offending one.

 See the end of this file for a list of most noticable new features in 0.5.0-E.


-------------------------
CREDITS:
-------------------------

 Programming by
	Fabio "Spinal" Cecin <fcecin@inf.ufrgs.br> <frcecin@terra.com.br>
	Niko "Nix" Ritari <npr1@suomi24.fi>
	Jani "Huntta" Rivinoja <janir@mbnet.fi>

 Featuring sound theme by Visa-Valtteri Pimiä <visa.pimia@www.fi>

 GNU/Linux work in 0.5.0 by Rafael Jannone <jannone@inf.ufrgs.br>

 Random Name Routine(TM) by Renato Hentschke <renato@amok.com.br>

 Included maps by Fabio Cecin, JoL-PHIN, Flyer-DH, Huntta, Juggis, Jakke,
   EVILKAIOH and DEVIL

 This game uses the excellent libraries:
  * Allegro - http://alleg.sourceforge.net
  * HawkNL - http://www.hawksoft.com/hawknl 
  * GNE - http://www.rit.edu/~jpw9607/gne/
  * Pthreads-win32 - http://sources.redhat.com/pthreads-win32


-------------------------
SPECIAL THANKS:
-------------------------

 Special thanks to Amok Entertainment (http://www.amok.com.br) for sponsoring
   the first game server.

 Special thanks to hardMOB <http://www.hardmob.com.br> for sponsoring the second
   (but much better) game server! :-) Thanks to Jefferson "JReZIN" Ietto Novo
   <jrezin@hardmob.com.br> for running the hardMOB server!

 Thanks to Paulo Zaffari (Amok Entertainment) for helping in debugging (the
   0.4.0 "win-nt killer" bug!), testing and giving game design suggestions.

 Thanks JoL-PHIN, Flyer-DH and others for the Outgun maps included with the game
   distribution!

 Thanks to Ricky Piller <superpiller@hotmail.com> for the first-ever server
   outside Brazil, in USA (a kick-ass cable connection, by the way!)

 Finally, thanks to ALL players and supporters!


-------------------------
New in 0.5.0-E
-------------------------


 Gameplay affecting server features

 - Selectable capture limit and time limit (optional).
 - The maximum number and effective time of power-ups can be set in gamemod.txt.
 - Number of power-ups can be set in percents of map size (to reduce them in
   smaller maps).
 - Picking up a second deathbringer cancels the effect (optional).
 - Shadow power-up can be set to make the player totally invisible.
 - Power-ups are never inside walls.
 - Power-ups lying on the ground don't change.


 Other server features

 - Server can have more than 32 maps in the list.
 - Map rotation can be fixed or random.
 - Welcome message to players entering the server.
 - Setting to inhibit map changes for some time for each map.
 - Server admin can kick, ban and mute players.
 - Optional player name registering in the server.
 - Empty or invalid player names are not allowed.
 - Pings are measured accurately.
 - Some bugs and vulnerabilities are fixed, and some smaller features added.
 - Console commands:
	/help      List the supported commands.
	/info      Shows info about the server.
	/config    Shows some info about server configuration, e.g. capture
	           limit and time limit.
	/stats     See your stats.
	/mapinfo   Shows info about a map.
	/time      Shows the server uptime, the map time and time left.
	/votemap   Vote a map or see the maplist and votes.
	/sayadmin  Send a message to the server admin.


 Client side features

 - Flag sites are marked on the ground and shown more intelligently in the
   minimap.
 - Option to log messages in a file. Start outgun with the parameter -log
 - Support for high Ascii characters (e.g. ä, é, ñ). Edit allegro.cfg to set up
   your keyboard.
