#include <allegro.h>
/* XPM */
static const char *allegico_xpm[] = {
/* columns rows colors chars-per-pixel */
"32 32 10 1",
"  c black",
". c navy",
"X c blue",
"o c #800000",
"O c red",
"+ c #808000",
"@ c yellow",
"# c #C0C0C0",
"$ c gray100",
"% c None",
/* pixels */
"%%%%%% %%%%%%%       %%%%   %%%%",
"%%%%%  %%%%   ####### %%  X   %%",
"%%      %% ###$$$$#$##   XXXX  %",
"%  OoOO   ##$$$$$$$##$# XX.XXX  ",
"  OoOooO #$$$#$$$$##$### ....XX ",
" OoooooO $$$$$$##$#$#### X..X..X",
"OOoOooo #$$$$$$#$$#$$$### ...X..",
"OOooOoO ##$$$#$$#$$$$#$$# .....X",
"OoOoOoO #$$#$##$#$#$$$#$# X..X..",
"oOoOoO ##$$#$$#$$$#$$#### XX.XXX",
"oOoOoO #$$#$$$$$$$$$$#     X...X",
"OOoOoOO ###$$####$$$$ ##### .X..",
"OOoOOO   # # ####  #####$## ..XX",
"OOoOO ##$##$#### ####$$$$## ..XX",
"OOoOO ###$$$$###$####$###  X.X.X",
"OoOOoO  ######  ##$$$##   XX.X.X",
"OoOOoOOO #### +  ##### + XXX.XXX",
"OoOOoOOOO    +@ @     @ XXXX.XXX",
"OoOOooOO   ++@+ +@+++@+   XX.XXX",
"OOOOOOO   @+@+@+@+@+@+ %%  XXXX ",
" OOOO   %% @+++@+++@++ %%%   X  ",
"  OO  %%%% +@+@+@+@+@ %%%%%%   %",
"%    %%%%%  +@+++@+++  %%%%%%  %",
"%  %%%%%%   ++@+@+@+    %%%%%%%%",
"%%%%%%%%   % ++@+++@ %   %%%%%%%",
"%%%%%%%%   % +@+@+@ %%   %%%%%%%",
"%%%%%%%   %%% +++@+ %%%   %%%%%%",
"%%%%%%   %%%% @+@+ %%%%%   %%%%%",
"%%%%%%  %%%%%% @++ %%%%%%  %%%%%",
"%%%%%   %%%%%% +@ %%%%%%%   %%%%",
"%%%%%  %%%%%%%% + %%%%%%%%  %%%%",
"%%%%%%%%%%%%%%%  %%%%%%%%%%%%%%%"
};
#if defined ALLEGRO_WITH_XWINDOWS && defined ALLEGRO_USE_CONSTRUCTOR
extern void *allegro_icon;
CONSTRUCTOR_FUNCTION(static void _set_allegro_icon(void));
static void _set_allegro_icon(void)
{
    allegro_icon = allegico_xpm;
}
#endif
