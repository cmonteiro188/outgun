#ifndef PUBLIC_SERVER
//#define NR_CLIENT_AFFECTING
#endif


// ---- server side defines

#define NR_CONSOLE	// console commands
#define NR_NAME_AUTHORIZATION

#define NR_NO_PUP_SWITCHING
#define NR_VOTE_ANNOUNCE_INTERVAL 5
//#define NR_SERVER_PHYSICS
//#define NR_PHYS_VECTOR_ACC
#define NR_SUPPORT_OLD_CLIENTS
//#define NR_FIX_BOUNCING	// makes a difference only when nr_server_physics not defined

// ---- client side defines

#define CL_MINIMAP_FLAGPOS
#define CL_SHOW_FLAGPOS
#define FLAGPOS_RAD 30
//#define CL_SHOW_TIME_LEFT
#define SHADOW_MINIMUM_NORMAL 7

// ----

#include <vector>
#include <list>
#include <fstream>
#include <sstream>
#include <iomanip>

#ifdef NR_SERVER_PHYSICS
#define NR_FIX_BOUNCING
#else
#define NR_SUPPORT_OLD_CLIENTS
#endif

#ifdef NR_FIX_BOUNCING
/* NR_SHIFTY is used for bounce checks: 15 aligns with the map, 0 is the buggy default behaviour */
#define NR_SHIFTY 15
#else
#define NR_SHIFTY 0
#define NR_SUPPORT_OLD_CLIENTS
#endif

#ifdef NR_NAME_AUTHORIZATION
#include "nameauth.h"
#endif

template<class T> T bound(T val, T lb, T hb) { return val<=lb?lb:val>=hb?hb:val; }	//#NR

//#NR strspnp: (Watcom definition) find from str the first char not in charset
char* strspnp(char* str, const char* charset) {
	for (; *str; ++str)
		if (strchr(charset, *str)==NULL)
			return str;
	return NULL;
}

const char* strspnp(const char* str, const char* charset) {
	return strspnp(const_cast<char*>(str), charset);
}

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  Copyright (C) 2002 - Fabio Reis Cecin <fcecin@inf.ufrgs.br>
 */

/*

	Outgun

	A simple game with retro-graphics for net coding research purposes.

	by Fábio Reis Cecin (frcecin@terra.com.br)
	http://www.inf.ufrgs.br/~fcecin/outgun

	GPL'ed version and
	Linux packaging by Rafael Jannone (jannone@inf.ufrgs.br)

*/

/*
PROXIMAS VERSOES
========================

 - BUG: scoreboard TAB mostrando scores duplicados/errados
 - KICK do jogador se ele fica inativo (mostrar msg na tela pedindo pra ele se mexer)
   OBS: NAO KICKAR QUEM ESTIVER __TECLANDO__ NO MENU (se ficar so com o menu aberto, kika)
 - animacao do "numero" de bonus que voce ganha quando fraga alguem
 - deletacao automatica da conta se 90 dias de inatividade

se possível, resolver
========================

 - trafego todo na porta 25000, sou muito burro!!
 - BUG: trava o jogo com page flipping, esporadicamente (== ALLEGRO)
 - BUG: resolver o problema da conexao que faz o PING mas nao consegue conectar... (== FIREWALL)
 - BUG: testando com 31 bots + eu, da pau nos clientside FX's... (acho que falta) MAS tambem
   deu pau dai na troca de mapa que nao trocou...!!

se realmente sobrar tempo...
========================

 - mostrar REFERRER STATISTICS do jogador

fica pro outgun II ...
========================

 - BOTS!
 - mostrar na scoreboard TAB o escore acumulado ate agora de cada player/client, que ainda está
   para ser submetido
 - "internacionalizacao": mensagens do jogo em TXT
 - logotipo do HOST DENTRO DO JOGO (formato --> TGA, BMP OU PCX),
   DOWNLOAD AUTOMATICO para o diretorio cmaps quando conecta
 - server MOTD (load/reload & show)
 - tentar tirar todos os diretorios e arquivos, na versao 0.4.6, pra
   ver se nao da pau
 - TESTES: nao consegui conectar nos servidores 45/46 lá em casa, tentando da
   amok com os clientes 45/46. o local/remote address da socket de envio
   (cliente 46 com client_c.log) tava certo:
   143.54.83.238:1029 / 1030 / 1031 (3 tentativas) remote
   200.176.34.176:25000 / 200.176.34.176:25001
 - quando troca de mapa e o cara ganha seu score, aparecer na tela um sumario do score dele no minimo
   SCORE +300 por exemplo (se possivel com os modificadores)
 - suporte completo para registro, administracao e exibição de CLÃS (WEB e INGAME)
 - gerar sumário de resultados do país e do clã no final do dia, junto no dayrank.html
 - incluir um elemento de incentivo para a expansão do clã (mais referrals)
 - CALCULAR A MÉDIA DO SCORE MUNDIAL
 - GRAVAR NA CONTA DO CARA e mostrar na tela de login e public profile O HISCORE DE TODOS OS TEMPOS DO CARA junto com o dia!!
 - TESTAR NA AMOK:
   - se pedir TCP vai ter o timeout e vai pedir via UDP depois mesmo...
   - CLIENTE apos primeiro timeout SEMPRE pede por UDP, os proximos mapas.
 - telas 4x4 que viram uma so. ideia de JoL- .... (?) (MUITO BOA IDEIA)
 - estudar um esquema de "classes", de repente uma opcao de modalidade: com ou sem classes
 - paredes destrutiveis?
 - chao de gelo?
 - chao speedup?
 - chao teleport?
 - weapon pick-up item: classe de itens que, ao pegar, trocam a arma do jogador. por exemplo, laser
   beam e hellblaze(flamethrower), sendo que todas as armas tem um significado diferente para os
   diferentes power levels (1 a 9), e as armas que nao sao o tiro default tem tempo: apos X segundos
   o jogador perde a arma.
 - aumentar o timeout da gameover plaque e fazer que os jogadores podem pedir pra continuar o
   jogo de uma vez teclando TIRO
 - "DESLIGAR" O JOGADOR DO JOGO, isto eh nao desenhar ele em lugar nenhum, enquanto ele estiver
    fazendo DOWNLOAD de mapas. nao spawnar a bolinha de fato no jogo
 - embonitar as telas de map loading e gameover plaque
   - top fragger / most valuable player / dar uma olhada nessas coisas
 - usar aquela janelinha 320x200 do servidor pra exibir status do tipo
   - players overall: numero de jogadores que ja entraram no servidor
    - players today
    - media da sessao de jogo, dos que já saíram: overall
    - meida da sessao de jogo today
    - server utilization overall (tempo com 1 jogador, tempo com 2 jogadores ... tempo com 16
      jogadores dai faz a media)
    - server utilization today
    - server bandwidth mean overall
    - server bandwidth mean today
    - server up time
 - enviar essas informacoes para o master server, codificadas no query de add mesmo
 - mudar todas as sockets TCP BLOCKING para NONBLOCKING, pra nao ter coisa ficando trancada.
 - menus (GUI) legal e muito melhor que a atual (jogar a atual no lixo)
   - GUI no geral muito mais bonita, mudar tudo
 - CONSOLE para o jogo
 - CONFIGURACAO DE TECLAS (tela GUI pra isso)
 - adicionar suporte para joystick : detectar e sair usando (ver allegro)
 - BUG: fazer do map loading uma rotina que nao f*** a memoria carregando info's mal formuladas
   de salas explicacao anterior:
   bug: load_map escreve merda no map.filename carregando o mapa gerado pelo renato por exemplo.
        basicamente o load_map_bla_label(..) inicial q loada o mapa escreve sobre o name (zeros,
        transforma em uma string null). ENTAO: tirar o map.PROTEKT e consertar o map_load

*/

// ***** FORTIFY !!! *****

#include "fortfy22/fortify.h"

// ***** FORTIFY !!! *****

//macros for allegro video mode

//#define WINMODE GFX_GDI		-- can't pageflip

//#define WINMODE GFX_DIRECTX_ACCEL
//#define FULLMODE GFX_DIRECTX_ACCEL

#define WINMODE GFX_AUTODETECT_WINDOWED

#define FULLMODE GFX_AUTODETECT

//DEBUGGING ranking?
//#define DEBUG_RANKING

//TURN OFF BOTS?
#define NO_BOTS

//maximum BOTSIZE variable value, absolute
#define MAX_BOTSIZE 8

//same as PLAYER RADIUS (15) + ROCKET RADIUS (3)
#define	SHOT_DELTAX	17	// V0.4.8 : A HAIR LESS!

//minimum time between flag steal at base and capture, to consider a map to be valid for scoring
#define MINIMUM_GRAB_TO_CAPTURE_TIME 6.0

//RANKING defines
#define DEFAULT_PLAYER_RATE 1.0
#define DEFAULT_BOT_PLAYER_RATE 1.0
#define MINIMUM_POSITIVE_SCORE_FOR_RANKING 100

//#define SWITCH_PAUSE_CLIENT

//#define ALWAYS_FRICTION

#define PI M_PI //3.1416

#define PIOIT M_PI_4 //0.7854 //DOIS PI SOBRE 8 = PI SOBRE 4 = 0.7854

#define PASSBUFFER	32		//size of password file

//quick debugs
//#define MIN_ALPHA_FRIENDS 1			//debug value
#define MIN_ALPHA_FRIENDS 64

#define ROCKET_SPEED 50.0		//in pixels/0.1s

#define MIN_HEALTH_FOR_RUN_PENALTY 40

#define NUMBER_OF_POWERUP_KINDS 7	//quad shield shadow turbo weapon-up megahealth deathbringer

//#define DEBUG_POWERUPS
//#define REALLY_DEBUG_POWERUPS		//define only if DEBUG_POWERUPS defined

// GAME VERSION / GAME STRING
//
#define GAME_STRING "Outgun"
#define GAME_PROTOCOL "14"
#define GAME_VERSION "0.5.0"

#define TK1_VERSION_STRING "v048"

#include "allegro.h"	// Allegro

//patching main / _main / WinMain link errors...
#ifdef ALLEGRO_WINDOWS
#include "winalleg.h"
#include "windows.h"
#endif

#include <stdio.h>			// for -text (v0.5.0)

#include "nl.h"				// HawkNL

#include "leetnet/server.h"		// l33t server
#include "leetnet/client.h"		// l33t client
#include "leetnet/rudp.h"			// get_self_I
#include "leetnet/sleep.h"		// sleep util

#include "string.h"
#include "math.h"

#include "pthread/pthread.h"
#include "pthread/sched.h"

#include <string>
using namespace std;
#include "names.h"		//the COOLEST random-name generator by Renato Hentschke

//admin shell protocol
#include "admshell.h"

//log utils
//#define LOG_NOLOG		// uncomment to disable logging
#define LOG_EXPR game_log
#define LOG_TIMEFUNC get_time()
#include "leetnet/log.h"
FILE *game_log;

#if defined ALLEGRO_WINDOWS || WIN32 || WIN64
#include <conio.h>
#else

// base is always 10 here
char* itoa(const int i, char* buf, const int base)
{
	sprintf(buf,"%i",i);
	return buf;
}

#endif

// ---- client screen layout ----

//resolution
#define RESOL_X 640
#define RESOL_Y 480

//play area offset
#define plx 0
#define ply 90

//play area width/height
#define plw 472
#define plh 354

//minimap offset
#define mmx (plx + plw + 16)	//push 8 to left
#define mmy ply

//scoreboard offset
#define sbx (plx + plw)
#define sby (mmy + 110)			// + XXX = minimap panel height


//************************************************************
//  common stuff
//************************************************************

//the default game port
#define DEFAULT_UDP_PORT 25000

//the master server address (www.mycgiserver.com:80)
NLaddress		master_address;

//directories for save/load maps
#define SERVER_MAPS_DIR "maps"
#define CLIENT_MAPS_DIR "cmaps"

//root path (game executable path)
#define WHERE_PATH_SIZE 256
char wheregamedir[WHERE_PATH_SIZE];

//function that resets the video mode
bool reset_video_mode();

// server game phisics parameters
double svp_fric, svp_accel, svp_maxspeed;
double svp_fric_run, svp_accel_run, svp_maxspeed_run;
double svp_fric_turbo, svp_accel_turbo, svp_maxspeed_turbo;
double svp_fric_turborun, svp_accel_turborun, svp_maxspeed_turborun;
double svp_flag_penalty;

// set the default physics parameters
void set_default_physics() {

	// OLD (and cool) 0.2.3 PHYSICS
	//
	svp_fric = 1.5;
	svp_accel = 2.0;
	svp_maxspeed = 12.0;
	svp_fric_run = 1.5;
	svp_accel_run = 2.0;
	svp_maxspeed_run = 22.0;
	svp_fric_turbo = 3.0;
	svp_accel_turbo = 4.0;
	svp_maxspeed_turbo = 18.0;
	svp_fric_turborun = 3.0;
	svp_accel_turborun = 4.0;
	svp_maxspeed_turborun = 33.0;
	svp_flag_penalty = 3.0;

	// NEW (and lame) PHYSICS 0.3.0 +
	//
	/*
	svp_fric = 1.5;
	svp_accel = 2.0;
	svp_maxspeed = 12.0;
	svp_fric_run = svp_fric  * 1.25;
	svp_accel_run = svp_accel * 1.25;
	svp_maxspeed_run = 20.0;
	svp_fric_turbo = svp_fric * 1.5;
	svp_accel_turbo = svp_accel * 1.5;
	svp_maxspeed_turbo = 24.0;
	svp_fric_turborun = svp_fric * 1.75;
	svp_accel_turborun = svp_accel * 1.75;
	svp_maxspeed_turborun = 32.0;
	svp_flag_penalty = 3.0;
	*/
}

// number-of-players
#define MAX_PLAYERS	32							// the MAXIMUM MAXIMUM number of players EVER
int			maxplayers = MAX_PLAYERS;		// the maximum number of players configured for this server (must be <= MAX_PLAYERS and an EVEN NUMBER == NUMERO PAR)
#define TSIZE (maxplayers/2)				// CTF TEAM SIZE

#define MAX_ROCKETS 256		// maximum number of rockets (nao pode ser mais que 256 pq eh usado um unsigned char p/ passar ids)

#ifdef NR_CLIENT_AFFECTING
#define MAX_PICKUPS 256
#else
#define MAX_PICKUPS MAX_PLAYERS	// the MAXIMUM MAXIMUM number of pickups laying on the ground at one time in the game
#endif

int			maxpickups;							// the maximum number of pickups (function of maxplayers)

//arg switches (+ default values)
bool dedserver = false;		//dedicated server? -ded
bool textserver = false;		//textmode dedicated server for UNIX/LINUX (V0.5.0) (WON'T WORK ON WINDOWS...)
bool privateserver = false;	//private server? (will not publish)
bool winclient = true;		//windowed client?	-win / -fs
bool trypageflip = false;	//try page flipping? -flip / -dbuf
bool nosound = false;			//disable sound? -nosound
int targetfps = 60;			//target (MAX) frames-per-second
int port = DEFAULT_UDP_PORT;				//the server port
bool showinfo = false;		//apenas show info e desliga server
bool defaultprio = false;	//select default server threads priority
#define TARGET_PRIO_UNSPECIFIED -666666
int targetprio = TARGET_PRIO_UNSPECIFIED;	//unspecified
int server_maxplayers = 16;		//default maxplayers of the server
bool sound_inited = false;		//install_sound succeeded?
bool sound_enabled = true;		// player wants sounds?
bool no_tcp_download = true;		// V0.4.7: CHANGED DEFAULT : disable use of the TCP socket for file transfers (use the regular UDP leetnet connection)
bool force_ip = false;		//force IP?
char force_ip_name[32];		//force IP to what?

//v0.5.0 - set_window_title() frontend!
void server_status_string(char *str) {
	if (textserver)
		printf("%s\n",str);
	else
		set_window_title(str);
}

//audio samples : codes
enum {

	SAMPLE_FIRE,			//ok
	SAMPLE_HIT,				//ok

	SAMPLE_WALLHIT,				//new! v0.3.9
	SAMPLE_QUADWALLHIT,		//new! v0.3.9

	SAMPLE_DEATH,			//ok
	SAMPLE_DEATH_2,		//ok
	SAMPLE_ENTERGAME,	//ok
	SAMPLE_LEFTGAME,	//ok
	SAMPLE_CHANGETEAM,
	SAMPLE_TALK,
	SAMPLE_WALLBOUNCE,

	SAMPLE_WEAPON_UP,	// new! v0.1.2

	SAMPLE_MEGAHEALTH, // new! v0.3.0

	SAMPLE_GETDEATHBRINGER, // new! v0.3.9 -- get deathbringer powerup (voz sinistra)
	SAMPLE_USEDEATHBRINGER, // new! v0.3.9 -- use deathbringer powerup (carrier dies) ("GRRRAAWWKKLLLL!!")
	SAMPLE_HITDEATHBRINGER,	// new! v0.3.9 -- target is hit by the deathbringer ("PWRRLLW!")
	SAMPLE_DIEDEATHBRINGER,	// new! v0.3.9 -- target dies by the deathbringer		("HaHaHaHa!")

	SAMPLE_SHIELD_PICKUP,
	SAMPLE_SHIELD_DAMAGE,
	SAMPLE_SHIELD_LOST,

	SAMPLE_BOOTS_ON,
	SAMPLE_BOOTS_OFF,

	SAMPLE_QUAD_ON,
	SAMPLE_QUAD_FIRE,
	SAMPLE_QUAD_OFF,

	SAMPLE_HELM_ON,
	SAMPLE_HELM_OFF,

	SAMPLE_CTF_GOT,			//ok
	SAMPLE_CTF_LOST,		//ok
	SAMPLE_CTF_RETURN,	//ok
	SAMPLE_CTF_CAPTURE,	//ok
	SAMPLE_CTF_GAMEOVER,	//ok

	NUM_OF_SAMPLES
};


//server timer (10Hz)
volatile int server_speed_counter = 0;
void increment_server_speed_counter()
{
	server_speed_counter++;
}
END_OF_FUNCTION(increment_server_speed_counter);

//client game timer (60Hz)
volatile int speed_counter = 0;
volatile int client_netsend_counter = 0;		//sub-counter for keypress sending
void increment_speed_counter()
{
	speed_counter++;
}
END_OF_FUNCTION(increment_speed_counter);

//this timer will be used to emulate a better clock()
volatile unsigned long time_counter = 0;
void increment_time_counter() {
	time_counter++;
}
END_OF_FUNCTION(increment_time_counter);

double get_time() {
	return ((double)time_counter) / 200.0;
}

//colors
enum {
	//player's colors
	COLGREEN,
	COLYELLOW,
	COLWHITE,
	COLMAG,
	COLCYAN,
	COLORA,
	COLLRED,		// light red
	COLLBLUE,		// light blue
	//MORE player colors
	COL9,
	COL10,
	COL11,
	COL12,
	COL13,
	COL14,
	COL15,
	COL16,

	//team colors
	COLRED,			//team 1 (color 0)
	COLBLUE,		//team 2 (color 1)

	//base colors
	COLBRED,			//team 1 (color 0)
	COLBBLUE,		//team 2 (color 1)

	//other
	COLFOGOFWAR,
	COLMENUWHITE,
	COLMENUBLACK,
	COLMENUGRAY,
	COLGROUND,
	COLWALL,
	COLNOLIFE,
	COLDARKGRAY,
	COLSHADOW,
	COLLIMBO,
	COLDARKORA,
	COLINFO,
	COLENER3,
	NUM_OF_COL
};

int teamcol[2];
int teamlcol[2];	//light colours for statusbar
int teamdcol[2];	//dark colours for player name

char teamname[2][5];

int	col[NUM_OF_COL];
void setcolors() {

	//the first 8 colors are player's colors
	col[COLGREEN] = makecol(0,0xff,0);
	col[COLYELLOW] = makecol(0xff,0xff,0);
	col[COLWHITE] = makecol(0xff,0xff,0xff);
	col[COLMAG]	= makecol(0xff, 0, 0xff);
	col[COLCYAN] = makecol(0, 0xff, 0xff);
	col[COLORA]	= makecol(0xff, 0xb0, 0);
	col[COLLRED] = makecol(0xff,0x55,0x44);
	col[COLLBLUE] = makecol(0x44,0x55,0xff);
	//MORE player colors
	col[COL9] = makecol(242, 158, 224);
	col[COL10] = makecol(134, 143, 57);
	col[COL11] = makecol( 14, 148, 87);
	col[COL12] = makecol( 33, 132, 137);
	col[COL13] = makecol(100, 100, 100);
	col[COL14] = makecol(166, 166, 166);
	col[COL15] = makecol(202, 1, 56);	//wine
	col[COL16] = makecol(0xbf, 0x70, 0);	//darkora

	// team solid colors
	col[COLBLUE] = makecol(0,0,0xff);
	col[COLRED] = makecol(0xff,0,0);

	// base minimap background colors
	col[COLBBLUE] = makecol(0,0,0x44);
	col[COLBRED] = makecol(0x44,0,0);

	//other
	col[COLFOGOFWAR] = makecol(0xff, 0xff, 0xff);
	col[COLMENUWHITE] = makecol(0xc0,0xc0,0xc0);
	col[COLMENUGRAY] = makecol(0x68,0x68,0x68);
	col[COLMENUBLACK] = makecol(0x40,0x40,0x40);
	col[COLGROUND] = makecol(0x10, 0x40, 0);
	col[COLWALL] = makecol(0x30, 0xC0, 0);
	col[COLNOLIFE] = makecol(0,0,0);
	col[COLDARKGRAY] = makecol(0x30, 0x30, 0x30);
	col[COLSHADOW] = makecol(0x18,0x18,0x18);
	col[COLLIMBO] = makecol(0x10, 0x10, 0x10);
	col[COLDARKORA]	= makecol(0xbf, 0x70, 0);
	col[COLINFO] = col[COLDARKORA];		//color of statusbar non-game info (hostname, IP, net traffic)
	col[COLENER3] = makecol(125, 100, 255);

	//teams 0 & 1 (playernum(0..15) / 8) colors:
	teamcol[0] = col[COLRED];
	teamcol[1] = col[COLBLUE];

	//light colours for text
	teamlcol[0] = col[COLLRED];
	teamlcol[1] = col[COLLBLUE];

	//light colours for team text bg
	teamdcol[0] = col[COLBRED];
	teamdcol[1] = col[COLBBLUE];
}

//server record
struct gamespy_t {
	NLaddress addr;
	char address[128];	//IP-address typein buffer
	bool invalid;
	bool noresponse;
	bool favs;	//hack
	bool refreshed;	//if data below is valid -------------
	char info[128];
};

struct RectWall {	// rectangular wall
	int a, b, c, d;	// rectangle coords (a,b)->(c,d)
	int tex;	// texture id
	int alpha;

	RectWall() { }
	RectWall(int a_, int b_, int c_, int d_, int tex_, int alpha_)
			: a(a_), b(b_), c(c_), d(d_), tex(tex_), alpha(alpha_) { if (c<a) swap(a, c); if (d<b) swap(b, d); }
	bool intersects_rect(float x1, float y1, float x2, float y2) const { return x1<=c && x2>=a && y1<=d && y2>=b; }
};

struct TriWall {	// triangular wall
	int x1, y1, x2, y2, x3, y3;
	int boundx1, boundy1, boundx2, boundy2;
	int tex, alpha;

	TriWall() { }
	TriWall(int x1_, int y1_, int x2_, int y2_, int x3_, int y3_, int tex_, int alpha_)
			: x1(x1_), y1(y1_), x2(x2_), y2(y2_), x3(x3_), y3(y3_), tex(tex_), alpha(alpha_) {
		if (y2<y1) { swap(x1, x2); swap(y1, y2); }	// 1, 2 sorted
		if (y3<y2) {
			swap(x2, x3); swap(y2, y3);	// 1, 3 and 2, 3 sorted
			if (y2<y1) { swap(x1, x2); swap(y1, y2); }	// all sorted
		}
		boundx1=min(x1, min(x2, x3)), boundy1=min(y1, min(y2, y3));
		boundx2=max(x1, max(x2, x3)), boundy2=max(y1, max(y2, y3));
	}
	bool intersects_rect(float x1, float y1, float x2, float y2) const;
};

/* subIntersection:
 * returns true if the area between lines (lx1,ly1)-(lx2,ly2) and (rx1,ry1)-(rx2,ry2) intersects the rectangle (rectx1,recty1)-(rectx2,recty2)
 * every line must be y-ordered : ly1<=ly2, ry1<=ry2, recty1<=recty2 ; additionally rectx1<=rectx2 and lx(y)<=rx(y) for all applicable y
 *
 * how it works:
 * for the appropriate range of y, if there exists an y so that lx(y)<=rectx2 AND rx(y)>=rectx1 , there is an intersection in that range
 * those ranges are solved with simple linear equasions since lx and rx are linear
 */
bool subIntersection(float lx1, float ly1,  float lx2, float ly2,  float rx1, float ry1,  float rx2, float ry2,
				float rectx1, float recty1, float rectx2, float recty2) {
	assert(ly1<=ly2 && ry1<=ry2);
	float miny = max(max(ly1, ry1), recty1), maxy=min(min(ly2, ry2), recty2);
	if (maxy < miny)
		return false;
	// first narrow the range by lx(y) <= rectx2
	if (lx1 == lx2) {	// can't formulate a value for intersection-y
		if (lx1 > rectx2)	// lx(y) <= rectx2 identically false => no solutions
			return false;
		// lx(y) <= rectx2 identically true => no narrowing from lx
	}
	else {
		// solve lx(y) == rectx2 , where lx(y) = lx1 + (y-ly1)*(lx2-lx1)/(ly2-ly1)
		float intersect_y = (rectx2 - lx1) * (ly2 - ly1) / (lx2 - lx1) + ly1;
		if (lx2 > lx1) {	// the intersection is at y <= intersect_y
			if (maxy > intersect_y)
				maxy = intersect_y;
		}
		else {	// the intersection is at y >= intersect_y
			if (miny < intersect_y)
				miny = intersect_y;
		}
	}
	if (maxy < miny)
		return false;
	// now narrow the range further by rx(y) >= rectx1, similarly
	if (rx1 == rx2)
		return (rx1 >= rectx1);
	else {
		float intersect_y = (rectx1 - rx1) * (ry2 - ry1) / (rx2 - rx1) + ry1;
		if (rx2 > rx1) {	// the intersection is at y >= intersect_y
			if (miny < intersect_y)
				miny = intersect_y;
		}
		else {	// the intersection is at y <= intersect_y
			if (maxy > intersect_y)
				maxy = intersect_y;
		}
	}
	return (maxy >= miny);
}

bool TriWall::intersects_rect(float rx1, float ry1, float rx2, float ry2) const {
	assert(ry1<=ry2 && rx1<=rx2);
	assert(y1<=y2 && y2<=y3);
	if (rx1>boundx2 || rx2<boundx1 || ry1>boundy2 || ry2<boundy1)
		return false;
	/* idea: triangle is split in two triangles: y<=y2 and y>=y2
	 * for both parts, the right and left edge are checked separately
	 * for each edge there may be a region [yi0, yi1] where (for a right side edge) xr(y)>=x1
	 * if those regions overlap with each other and [ry1, ry2], there exists an intersection
	 */
	if (x2 < x1 + (y2-y1) * (x3-x1) / (y3-y1)) {	// p2 is left to the p1-p3 line
		if (subIntersection(x1,y1, x2,y2, x1,y1, x3,y3, rx1, ry1, rx2, ry2))	// part y<=y2 : L,R sides p1-p2, p1-p3
			return true;
		if (subIntersection(x2,y2, x3,y3, x1,y1, x3,y3, rx1, ry1, rx2, ry2))	// part y>=y2 : L,R sides p2-p3, p1-p3
			return true;
	}
	else {
		if (subIntersection(x1,y1, x3,y3, x1,y1, x2,y2, rx1, ry1, rx2, ry2))	// part y<=y2 : L,R sides p1-p3, p1-p2
			return true;
		if (subIntersection(x1,y1, x3,y3, x2,y2, x3,y3, rx1, ry1, rx2, ry2))	// part y>=y2 : L,R sides p1-p3, p2-p3
			return true;
	}
	return false;
}

struct Room {
	vector<RectWall> rwalls, rground;	// ground: optional list of textures for ground [not used]
	vector<TriWall>  twalls, tground;

	void draw(BITMAP* buffer, float x0, float y0, float xScale, float yScale, int color) const {
		for (vector<RectWall>::const_iterator rwi=rwalls.begin(); rwi!=rwalls.end(); ++rwi)
			rectfill(buffer, int(x0+xScale*rwi->a), int(y0+yScale*rwi->b), int(x0+xScale*rwi->c), int(y0+yScale*rwi->d), color);
		for (vector<TriWall>::const_iterator twi=twalls.begin(); twi!=twalls.end(); ++twi)
			triangle(buffer,
					int(x0+xScale*twi->x1), int(y0+yScale*twi->y1),
					int(x0+xScale*twi->x2), int(y0+yScale*twi->y2),
					int(x0+xScale*twi->x3), int(y0+yScale*twi->y3), color);
	}
	bool fall_on_wall(int x1, int y1, int x2, int y2) const {
		for (vector<RectWall>::const_iterator rwi=rwalls.begin(); rwi!=rwalls.end(); ++rwi)
			if (rwi->intersects_rect(x1, y1, x2, y2))
				return true;
		for (vector<TriWall>::const_iterator twi=twalls.begin(); twi!=twalls.end(); ++twi)
			if (twi->intersects_rect(x1, y1, x2, y2))
				return true;
		return false;
	}
};

//entity locale
struct spoint_t {
	int px, py;	//screen (if px == -1, unused)
	int x, y;	//relative (to screen) X,Y position
};

//team info
struct teaminfo_t {
	spoint_t flag;	//flag position
	vector<spoint_t> spawn;	//team spawn points
	unsigned int lastspawn;	//last team spawn point used

	teaminfo_t() : lastspawn(0) { }
};

class Map {
	bool parse_label(FILE *f, const char *label, int crx, int cry);	// crx,cry = "current room pointer"

public:
	bool valid_for_scoring;	//v0.4.7: map is valid for scoring?
	teaminfo_t tinfo[2];	//team information for red=0 and blue=1 teams
	vector< vector<Room> > room;

	string title;	//map title
	int	ver;	// map version
	int w, h;	// width height
	NLushort crc;	//map's 16bit CRC

	Map() : valid_for_scoring(true), ver(-1), w(0), h(0), crc(0) { }

	bool fall_on_wall(int px, int py, int x1, int y1, int x2, int y2) const {
if (px<0 || py<0 || px>=w || py>=h) return false;	//#NR remove this and track why these are given sometimes
		assert(px>=0 && py>=0 && px<w && py<h);
		return room[px][py].fall_on_wall(x1, y1, x2, y2);
	}
	void draw_minimap(BITMAP* buffer) const;
	bool load(FILE* f);
};

bool Map::load(FILE* f) {
	*this=Map();
	NLubyte lebigbuf[65536];
	int numread = fread((void*)lebigbuf, 1, 65536, f);
	crc = nlGetCRC16((NLubyte*)lebigbuf, numread);
	rewind(f);
	return parse_label(f, 0, 0, 0);
}

bool Map::parse_label(FILE *f, const char *scan_label, int crx=0, int cry=0) {	// crx,cry = "current room pointer"
	rewind(f);
	for (;;) {
		char s[1024];
		if (!fgets(s, 1024, f)) {	// end-of-file or error
			if (scan_label) {
				LOG1("Map label %s not found\n", scan_label);
				return false;
			}
			else
				return true;
		}
		s[strlen(s)-1] = '\0';	// erase \n
		if (s[0] == '\0' || s[0]==';')
			continue;
		if (s[0]==':') {	// a label is found
			if (!scan_label)
				return true;
			if (!strcmp(s+1, scan_label))
				scan_label = 0;
			continue;
		}
		if (scan_label)
			continue;
		char nullc;	// to be used at ends of sscanf to make sure there is nothing extra on the line
		if (s[0]=='W' || s[0]=='G') {	// W x1 y1 x2 y2 [tex alpha] : rectangular wall (x1,y1)-(x2,y2) using given texture and alpha ; G : ground texture
			// required: x1<x2, y1<y2
			float x1, y1, x2, y2;
			int texid, alpha;
			int n = sscanf(s+1, " %f %f %f %f %i %i %c", &x1, &y1, &x2, &y2, &texid, &alpha, &nullc);
			if (n == 4) {
				texid = -1;
				alpha = 255;
			}
			if ((n!=4 && n!=6) || crx<0 || cry<0 || crx>=w || cry>=h) {
				LOG1("Invalid map line: %s\n", s);
				return false;
			}
			x1 *= plw; x2 *= plw;
			y1 *= plh; y2 *= plh;
			Room& rm = room[crx][cry];
			vector<RectWall>& wvec = (s[0]=='W') ? rm.rwalls : rm.rground;
			wvec.push_back(RectWall(int(x1), int(y1), int(x2), int(y2), texid, alpha));
			continue;
		}
		if (s[0]=='T') {	// T (W|G) x1 y1 x2 y2 x3 y3 [tex alpha] : triangular wall (W) or ground tex (G) (x1,y1)-(x2,y2)-(x3,y3) using given texture and alpha
			// required: y1<=y2, y2<=y3
			char type;
			float x1, y1, x2, y2, x3, y3;
			int texid, alpha;
			int n = sscanf(s+1, " %c %f %f %f %f %f %f %i %i %c", &type, &x1, &y1, &x2, &y2, &x3, &y3, &texid, &alpha, &nullc);
			if (n == 7) {
				texid = -1;
				alpha = 255;
			}
			if ((n!=7 && n!=9) || (type!='W' && type!='G') || crx<0 || cry<0 || crx>=w || cry>=h) {
				LOG1("Invalid map line: %s\n", s);
				return false;
			}
			x1 *= plw; x2 *= plw; x3 *= plw;
			y1 *= plh; y2 *= plh; y3 *= plh;
			Room& rm = room[crx][cry];
			vector<TriWall>& wvec = (type=='W') ? rm.twalls : rm.tground;
			wvec.push_back(TriWall(int(x1), int(y1), int(x2), int(y2), int(x3), int(y3), texid, alpha));
			continue;
		}
		if (s[0]=='R') {	// R x y : set room pointer to (x,y)
			int n = sscanf(s+1, " %i %i %c", &crx, &cry, &nullc);
			if (n!=2 || crx<0 || crx>=w || cry<0 || cry>=h) {
				LOG1("Invalid map line: %s\n", s);
				return false;
			}
			continue;
		}
		if (s[0]=='X') {	// X label x1 y1 [x2 y2] : add walls from label to the rectangle (x1,y1)-(x2,y2)
			char nextlabel[64];
			int rx1, ry1, rx2, ry2;
			int n = sscanf(s+1, " %64s %i %i %i %i %c", nextlabel, &rx1, &ry1, &rx2, &ry2, &nullc);
			if (n == 3) {	// one room only
				rx2 = rx1;
				ry2 = ry1;
			}
			else if (n!=5 || rx1<0 || rx2>=w || rx2<rx1 || ry1<0 || ry2>=h || ry2<ry1) {
				LOG1("Invalid map line: %s\n", s);
				return false;
			}
			long filepos = ftell(f);
			for (int ry=ry1; ry<=ry2; ry++)
				for (int rx=rx1; rx<=rx2; rx++)
					if (!parse_label(f, nextlabel, rx, ry))
						return false;
			fseek(f, filepos, SEEK_SET);
			crx=rx2; cry=ry2;	// compatibility with original sloppy specs (needed?)
			continue;
		}
		if (!strncmp(s, "P width ", 8)) {	// P width w : set map width to w rooms
			if (w != 0) {
				LOG("Redefined map width\n");
				return false;
			}
			int n = sscanf(s+1, " width %i %c", &w, &nullc);
			if (w<1 || n!=1) {
				LOG1("Invalid map line: %s\n", s);
				return false;
			}
			room.resize(w);
			for (vector< vector<Room> >::iterator ri=room.begin(); ri!=room.end(); ++ri)
				ri->resize(h);
			continue;
		}
		if (!strncmp(s, "P height ", 9)) {	// P height h : set map height to h rooms
			if (h != 0) {
				LOG("Redefined map height\n");
				return false;
			}
			int n = sscanf(s+1, " height %i %c", &h, &nullc);
			if (h<1 || n!=1) {
				LOG1("Invalid map line: %s\n", s);
				return false;
			}
			for (vector< vector<Room> >::iterator ri=room.begin(); ri!=room.end(); ++ri)
				ri->resize(h);
			continue;
		}
		if (!strncmp(s, "P title ", 8)) {	// P title text : set map title to text
			if (title.length()) {
				LOG("Redefined map title\n");
				return false;
			}
			title=string(s+8);
			continue;
		}
		if (!strncmp(s, "spawn ", 6)) {	// spawn t rx ry x y : make a spawn spot for team t at room (rx,ry) at (x,y)
			int team, rx, ry;
			float x, y;
			int n = sscanf(s, "spawn %i %i %i %f %f %c", &team, &rx, &ry, &x, &y, &nullc);
			if (n != 5) {
				LOG1("Invalid map line: %s\n", s);
				return false;
			}
			spoint_t spot;
			spot.px = bound(rx, 0, w-1);
			spot.py = bound(ry, 0, h-1);
			spot.x = (int)(x * (double)plw);
			spot.y = (int)(y * (double)plh);
			tinfo[team].spawn.push_back(spot);
			continue;
		}
		if (!strncmp(s, "flag ", 5)) {	// flag t rx ry x y : set team t's flag position to room (rx,ry) at (x,y)
			int team, rx, ry;
			float x, y;
			int n = sscanf(s, "flag %i %i %i %f %f %c", &team, &rx, &ry, &x, &y, &nullc);
			if (n != 5) {
				LOG1("Invalid map line: %s\n", s);
				return false;
			}
			tinfo[team].flag.px = bound(rx, 0, w-1);
			tinfo[team].flag.py = bound(ry, 0, h-1);
			tinfo[team].flag.x = (int)(x * (double)plw);
			tinfo[team].flag.y = (int)(y * (double)plh);
			continue;
		}
		if (s[0]=='V') {	// V ver : set file format version
			int n = sscanf(s+1, " %i %c", &ver, &nullc);
			if (n != 1) {
				LOG1("Invalid map line: %s\n", s);
				return false;
			}
			continue;
		}
		LOG1("Unrecognized map line: %s\n", s);
	}
}

void Map::draw_minimap(BITMAP* buffer) const {
	//black bg
	clear_to_color(buffer, 0);

	//draw screen boundaries
	int MMSCRW = (int)(98.0/float(w));
	int MMSCRH = (int)(98.0/float(h));
	int j;
	for (j=1;j<w;j++)
		line(buffer, 2+MMSCRW * j, 1, 2+MMSCRW * j, 100, col[COLSHADOW]);
	for (j=1;j<h;j++)
		line(buffer, 1, 2+MMSCRH * j, 100, 2+MMSCRH * j, col[COLSHADOW]);

	double maxx = plw*w;
	double maxy = plh*h;

	//draw bases
	#ifdef CL_MINIMAP_FLAGPOS
	int red_x = tinfo[0].flag.px;
	int red_y = tinfo[0].flag.py;
	int blue_x = tinfo[1].flag.px;
	int blue_y = tinfo[1].flag.py;
	bool same_room = false;
	// Are both maps in the same room?
	if (red_x == blue_x && red_y == blue_y) {
		same_room = true; /*
		float fx = tinfo[0].flag.px * plw + tinfo[0].flag.x;
		float fy = tinfo[0].flag.py * plh + tinfo[0].flag.y;
		circlefill(buffer, int(1 + fx/maxx*98.), int(1 + fy/maxy*98.), 4, col[COLRED]);
		fx = tinfo[1].flag.px * plw + tinfo[1].flag.x;
		fy = tinfo[1].flag.py * plh + tinfo[1].flag.y;
		circlefill(buffer, int(1 + fx/maxx*98.), int(1 + fy/maxy*98.), 4, col[COLBLUE]);*/
	}
	else {
		rectfill(buffer, 2+ MMSCRW * red_x, 2+ MMSCRH * red_y, MMSCRW * (red_x + 1), MMSCRH * (red_y + 1), col[COLBRED]);
		rectfill(buffer, 2+ MMSCRW * blue_x, 2+ MMSCRH * blue_y, MMSCRW * (blue_x + 1), MMSCRH * (blue_y + 1), col[COLBBLUE]);
	}
	#else
	int fx = tinfo[0].flag.px;
	int fy = tinfo[0].flag.py;
	rectfill(buffer, 2+ MMSCRW * fx, 2+ MMSCRH * fy, MMSCRW * (fx + 1), MMSCRH * (fy + 1), col[COLBRED]);
	fx = tinfo[1].flag.px;
	fy = tinfo[1].flag.py;
	rectfill(buffer, 2+ MMSCRW * fx, 2+ MMSCRH * fy, MMSCRW * (fx + 1), MMSCRH * (fy + 1), col[COLBBLUE]);
	#endif

	//draw solid walls
	float xmul=98./maxx, ymul=98./maxy;
	for (int y=0; y<h; y++) {
		float by=1.+y*plh*ymul;
		for (int x=0; x<w; x++) {
			float bx=1.+x*plw*xmul;
			room[x][y].draw(buffer, bx, by, xmul, ymul, makecol(0x00, 0x77, 0x00));
		}
	}

	//green border
	rect(buffer, 0, 0, buffer->w -1, buffer->h -1, col[COLGREEN]);

	#ifdef CL_MINIMAP_FLAGPOS
	if (same_room) {
		float fx = tinfo[0].flag.px * plw + tinfo[0].flag.x;
		float fy = tinfo[0].flag.py * plh + tinfo[0].flag.y;
		floodfill(buffer, int(1 + fx/maxx*98.), int(1 + fy/maxy*98.), col[COLBRED]);
		fx = tinfo[1].flag.px * plw + tinfo[1].flag.x;
		fy = tinfo[1].flag.py * plh + tinfo[1].flag.y;
		floodfill(buffer, int(1 + fx/maxx*98.), int(1 + fy/maxy*98.), col[COLBBLUE]);
	}
	#endif
}

//#NR: new function for use by NR_wallcorrect()
/* calculateDisplacement():
 *
 * calculates how many times the vector (mx,my) can be traveled until wall (dx1,dy1)-(dx2,dy2) is hit by a circle of radius r (max value considered is 1.)
 *
 *
 *        (mx,my)      __--
 *          \    __--^^
 *         __+-^^  + -(dx1,dy1)
 *     +-^^      .>|
 *   (0,0)      /  + -(dx2,dy2)
 *             /
 * p         wall
 * |
 * +--n
 *
 * either
 * A) the circle hits the wall proper with it's center projection on the line
 * B) the circle hits one of the corners where it's center is at distance r from the corner the first time
 *
 * A: | ( t(mx,my)-(dx1,dy1) ) x ( (dx2,dy2)-(dx1,dy1) ) | / | (dx2,dy2)-(dx1,dy1) | = r , taking the smaller solution of t and making sure the point is on the line
 * B: | t(mx,my)-(dx,dy) | = r , taking the smaller solution of t (if any real solution exists)
 *
 * returns: pair( t, pair(collisionn-centern, collisionp-centerp) )
 */
typedef pair<double, double> Coords;
pair<double, Coords> calculateDisplacement(double dx1, double dy1, double dx2, double dy2, double mx, double my, double r) {	// d=distance, m=movement
	// check for solution A (if there is one, there is no need to check B)
	// t * ( mx(dy2-dy1) - my(dx2-dx1) ) = dx1(dy2-dy1) - dy1(dx2-dx1) +-R*|(dx2,dy2)-(dx1,dy1)|
	double diffx = dx2-dx1, diffy = dy2-dy1;
	double div = mx*diffy - my*diffx;
	if (div == 0)	// movement parallel to the line => no collision
		return pair<double, Coords>(999., Coords());
	double rBase = ( dx1*diffy - dy1*diffx ) / div;
	double rAdd = r * sqrt(diffx*diffx+diffy*diffy) / div;
	double t = rBase - fabs(rAdd);	// the collision with smaller t (the other would be going away)
	if (t >= 0) {
		// make sure we are not off an end of the line
		// this can surely be calculated in a simpler way, but this first came to mind
		// collp = p1 + k(p2-p1)	0<=k<=1 if on the line
		// | t*m - collp |  minimum (=r)
		// | t*m - p1 - k(p2-p1) |  minimum (=r)
		// ( t*mx - dx1 - k(dx2-dx1) )^2 + ( t*my - dy1 - k(dy2-dy1) )^2  minimum (=r)
		// (dx2-dx1)*( t*mx - dx1 - k(dx2-dx1) ) + (dy2-dy1)*( t*my - dy1 - k(dy2-dy1) ) = 0  (derivative of the one above)
		// (dx2-dx1)*(t*mx-dx1) + (dy2-dy1)*(t*my-dy1) = k[ (dx2-dx1)(dx2-dx1) + (dy2-dy1)*(dy2-dy1) ]
		double k = ( diffx*(t*mx-dx1) + diffy*(t*my-dy1) ) / (diffx*diffx + diffy*diffy);
		if (k>=0. && k<=1.)
			return pair<double, Coords>(t, Coords(dx1+k*diffx-t*mx, dy1+k*diffy-t*my));
	}

	double dist=1.;
	Coords collisionCoords;
	// check for solution B
	// for dp1:
	double m2=mx*mx+my*my, r2=r*r;	// same for dp2
	double mdotd=mx*dx1+my*dy1;
	double d2=dx1*dx1+dy1*dy1;
	double disc=mdotd*mdotd-m2*(d2-r2);
	if (disc>=0) {	// there are real solutions
		t=(mdotd-sqrt(disc))/m2;	// select smaller t
		if (t<0)
			t=(mdotd+sqrt(disc))/m2;
		if (t>=0 && t<dist) {
			dist=t;
			collisionCoords=Coords(dx1-t*mx, dy1-t*my);
		}
	}
	// for dp2:
	mdotd=mx*dx2+my*dy2;
	d2=dx2*dx2+dy2*dy2;
	disc=mdotd*mdotd-m2*(d2-r2);
	if (disc>=0) {	// there are real solutions
		t=(mdotd-sqrt(disc))/m2;	// select smaller t
		if (t<0)
			t=(mdotd+sqrt(disc))/m2;
		if (t>=0 && t<dist) {
			dist=t;
			collisionCoords=Coords(dx2-t*mx, dy2-t*my);
		}
	}
	return pair<double, Coords>(dist, collisionCoords);
}

bool NR_wallcorrect(const Room& r, double fraction, double *x, double *y, double *sx, double *sy, double *ox, double *oy) {
	static const double plyRadius=15;

	double stx=*ox, sty=*oy-NR_SHIFTY;	// start pos
	double dtx=* x, dty=* y-NR_SHIFTY;	// destination

	double mx=dtx-stx, my=dty-sty;	// movement vector

	bool bounced=false;
	double movementLeft=fraction;

	for (;;) {
		double minMovement=movementLeft;
		Coords bounceVec;
		Coords bbox0(min(stx-plyRadius, dtx-plyRadius), min(sty-plyRadius, dty-plyRadius)),
		       bbox1(max(stx+plyRadius, dtx+plyRadius), max(sty+plyRadius, dty+plyRadius));
		for (vector<RectWall>::const_iterator wi=r.rwalls.begin(); wi!=r.rwalls.end(); ++wi) {	// go through rectangular walls first
			// fast and crude bounding-box style check first
			if (!wi->intersects_rect(bbox0.first, bbox0.second, bbox1.first, bbox1.second))
				continue;
			// check more carefully
			pair<double, Coords> rv;
			rv.first = 1.;
			if (mx>0 && wi->a>stx)	// check vertical wall a
				rv = calculateDisplacement(wi->a - stx, wi->b - sty, wi->a - stx, wi->d - sty, mx, my, plyRadius);
			else if (mx<0 && wi->c<stx)	// check vertical wall c
				rv = calculateDisplacement(wi->c - stx, wi->b - sty, wi->c - stx, wi->d - sty, mx, my, plyRadius);
			if (rv.first < minMovement) {
				minMovement = rv.first;
				bounceVec = rv.second;
			}
			if (my>0 && wi->b>sty)	// check horizontal wall b
				rv = calculateDisplacement(wi->a - stx, wi->b - sty, wi->c - stx, wi->b - sty, mx, my, plyRadius);
			else if (my<0 && wi->d<sty)	// check horizontal wall d
				rv = calculateDisplacement(wi->a - stx, wi->d - sty, wi->c - stx, wi->d - sty, mx, my, plyRadius);
			if (rv.first < minMovement) {
				minMovement = rv.first;
				bounceVec = rv.second;
			}
			#ifndef NDEBUG
			if (minMovement<movementLeft) {
				double dx=bounceVec.first, dy=bounceVec.second, r=plyRadius;
				assert(fabs(dx*dx+dy*dy-r*r)<.0001);
			}
			#endif
		}
		for (vector<TriWall>::const_iterator wi=r.twalls.begin(); wi!=r.twalls.end(); ++wi) {	// go through triangular walls separately
			// fast and crude bounding-box style check first
			if (!wi->intersects_rect(bbox0.first, bbox0.second, bbox1.first, bbox1.second))
				continue;
			// check more carefully
			pair<double, Coords> rv;
			rv = calculateDisplacement(wi->x1 - stx, wi->y1 - sty, wi->x2 - stx, wi->y2 - sty, mx, my, plyRadius);	// wall p1-p2
			if (rv.first < minMovement) {
				minMovement = rv.first;
				bounceVec = rv.second;
			}
			rv = calculateDisplacement(wi->x1 - stx, wi->y1 - sty, wi->x3 - stx, wi->y3 - sty, mx, my, plyRadius);	// wall p1-p3
			if (rv.first < minMovement) {
				minMovement = rv.first;
				bounceVec = rv.second;
			}
			rv = calculateDisplacement(wi->x2 - stx, wi->y2 - sty, wi->x3 - stx, wi->y3 - sty, mx, my, plyRadius);	// wall p2-p3
			if (rv.first < minMovement) {
				minMovement = rv.first;
				bounceVec = rv.second;
			}
			#ifndef NDEBUG
			if (minMovement<movementLeft) {
				double dx=bounceVec.first, dy=bounceVec.second, r=plyRadius;
				assert(fabs(dx*dx+dy*dy-r*r)<.0001);
			}
			#endif
		}
		assert(minMovement>=0. && minMovement<=movementLeft);
		stx+=mx*minMovement*.999;	// make sure we aren't going the least bit inside a wall :)
		sty+=my*minMovement*.999;
		if (stx < 0)    { sty-=     stx *my/mx; stx=  0; break; }
		if (stx >= plw) { sty-=(stx-plw)*my/mx; stx=plw; break; }
		if (sty < 0)    { stx-=     sty *mx/my; sty=  0; break; }
		if (sty >= plh) { stx-=(sty-plh)*mx/my; sty=plh; break; }
		if (minMovement>=movementLeft*.999)	// not bounced
			break;
		bounced=true;
		// bounce: speed component parallel with bounceVec ( (S dot b / |b|) * b / |b| ) is reversed, while perpendicular component is kept
		// : S -= 2* ( (S dot b) * b / |b|^2 )	; |b| is always plyRadius
		double mul=2.*((*sx)*bounceVec.first+(*sy)*bounceVec.second)/(plyRadius*plyRadius);
		*sx -= mul*bounceVec.first;
		*sy -= mul*bounceVec.second;
		// lose some speed too
		*sx *= .95;
		*sy *= .95;
		dtx=stx+(*sx);
		dty=sty+(*sy);
		mx=*sx; my=*sy;
		movementLeft-=minMovement+.01;	// don't bounce over 100 times in any conditions
		if (movementLeft<0)
			break;
	}
	*x=stx;
	*y=sty+NR_SHIFTY;
	*ox=*x;
	*oy=*y;
	return bounced;
}

#ifdef NR_SUPPORT_OLD_CLIENTS

//wall hit?
bool wallhit(double x, double y, const RectWall &w) { return w.intersects_rect(x, y, x, y); }

//wall collision correction
bool wallcorrect(const Room& room, double *x, double *y, double *sx, double *sy, double *ox, double *oy) {
	//delta old to new (ok)
	double tx,ty;
	tx = (*ox) - (*x);
	ty = (*oy) - (*y);

	//deltas for pushing out of walls: normalize
	double dx, dy;
	if (fabs(tx) > fabs(ty)) {
		dx = 2*tx / fabs(tx); // ==1.0
		dy = 2*ty / fabs(tx);		// 0 <= val <= 1
	}
	else {
		dx = 2*tx / fabs(ty);	// 0 <= val <= 1
		dy = 2*ty / fabs(ty);	// ==1.0
	}

	bool ever_had_wall_hit = false;
	bool had_wall_hit; //keep pushing out until no wall hit
	const Room* r = &room;

	int runaway = 10;
	do {

		had_wall_hit = false;
		bool y_solved = false;

		for (int w=0;w<(int)r->rwalls.size();w++) {
			int runaw = 100;
			while (wallhit((*x),(*y)-NR_SHIFTY,r->rwalls[w])) {
				had_wall_hit = true;
				(*x) += dx;
				y_solved = false;
				if (!(wallhit((*x),(*y)-NR_SHIFTY,r->rwalls[w])))
				break;
				(*y) += dy;
				y_solved = true;
				runaw--;
				if (runaw < 0) {
					(*x) = (*ox);
					(*y) = (*oy);
					return false;
				}
			}
		}
		if (had_wall_hit) {
			ever_had_wall_hit = true;
			if (y_solved)
				(*sy) *= -1;
			else
				(*sx) *= -1;
		}
		runaway--;
		if (runaway < 0) {
			(*x) = (*ox);
			(*y) = (*oy);
			return false;
		}
	} while (had_wall_hit);
	if (ever_had_wall_hit) {
		if (((*x) <= 0.01) || ((*x) >= ((double)plw) - 0.01) || ((*y)-NR_SHIFTY <= 0.01) || ((*y)-NR_SHIFTY >= ((double)plh) - 0.01)) {
			(*x) = (*ox);
			(*y) = (*oy);
		}
		(*ox) = (*x);
		(*oy) = (*y);
	}
	return ever_had_wall_hit;
}

#endif	// NR_SUPPORT_OLD_CLIENTS

//draw a wall, solid or nonsolid, texid, lum, in a map
void drawwall_tex(Map *m, bool is_solid, int x, int y, int a, int b, int c, int d, int tex, int alpha) {
	if (is_solid)
		m->room[x][y].rwalls.push_back(RectWall(a, b, c, d, tex, alpha));
	else
		m->room[x][y].rground.push_back(RectWall(a, b, c, d, tex, alpha));
}

//draw a solid wall in a map
void drawwall(Map *m, int x, int y, int a, int b, int c, int d) {
	//draw solid wall with tex = -1 & alpha 255
	drawwall_tex(m, true, x, y, a, b, c, d, -1, 255);
}

int COMPX = plw / 4;
int COMPY = plh / 4;
int LARGX = COMPX / 4;
int LARGY = COMPY / 4;

//closewalls
void closelwall(Map *map, int x, int y) {
	drawwall(map, x, y, 0, 0, LARGX, plh);
}
void closerwall(Map *map, int x, int y) {
	drawwall(map, x, y, plw-LARGX, 0, plw, plh);
}
void closeuwall(Map *map, int x, int y) {
	drawwall(map, x, y, 0, 0, plw, LARGY);
}
void closedwall(Map *map, int x, int y) {
	drawwall(map, x, y, 0, plh-LARGY, plw, plh);
}

//load default map (for testing)
void load_default_map(Map *map) {
	*map = Map();

	//map title
	map->title = "The Blunt, by Fabio Cecin";

	//map size
	map->w = 6;
	map->h = 6;

	int i;
	for (int x=0;x<map->w;x++)
	for (int y=0;y<map->h;y++)	{
		//default borderwalls
		drawwall(map, x, y, 0, 0, COMPX, LARGY);		//0
		drawwall(map, x, y, 0, 0, LARGX, COMPY);		//1
		drawwall(map, x, y, plw-COMPX, 0, plw, LARGY);		//2
		drawwall(map, x, y, plw-LARGX, 0, plw, COMPY);		//3
		drawwall(map, x, y, 0, plh-LARGY, COMPX, plh);		//4
		drawwall(map, x, y, 0, plh-COMPY, LARGX, plh);		//5
		drawwall(map, x, y, plw-COMPX, plh-LARGY, plw, plh);		//6
		drawwall(map, x, y, plw-LARGX, plh-COMPY, plw, plh);		//7
	}

	//closewalls
	closelwall(map, 0, 0);	closelwall(map, 0, 1);	closelwall(map, 0, 4);	closelwall(map, 0, 5);	closelwall(map, 1, 1);	closelwall(map, 1, 2);
	closelwall(map, 1, 3);	closelwall(map, 1, 4);	closelwall(map, 3, 1);	closelwall(map, 3, 2);	closelwall(map, 3, 3);	closelwall(map, 3, 4);
	closelwall(map, 5, 1);	closelwall(map, 5, 2);	closelwall(map, 5, 3);	closelwall(map, 5, 4);

	closerwall(map, 0, 1);	closerwall(map, 0, 2);	closerwall(map, 0, 3);	closerwall(map, 0, 4);	closerwall(map, 2, 1);	closerwall(map, 2, 2);
	closerwall(map, 2, 3);	closerwall(map, 2, 4);	closerwall(map, 4, 1);	closerwall(map, 4, 2);	closerwall(map, 4, 3);	closerwall(map, 4, 4);
	closerwall(map, 5, 0);	closerwall(map, 5, 1);	closerwall(map, 5, 4);	closerwall(map, 5, 5);

	closeuwall(map, 0, 0);	closeuwall(map, 1, 0);	closeuwall(map, 2, 0);	closeuwall(map, 3, 0);	closeuwall(map, 4, 0);	closeuwall(map, 5, 0);
	closeuwall(map, 3, 1);	closeuwall(map, 2, 5);

	closedwall(map, 0, 5);	closedwall(map, 1, 5);	closedwall(map, 2, 5);	closedwall(map, 3, 5);	closedwall(map, 4, 5);	closedwall(map, 5, 5);
	closedwall(map, 3, 0);	closedwall(map, 2, 4);

	// BUILD RED TEAM INFO
	//
	map->tinfo[0].flag.px = 1;		// flag pos
	map->tinfo[0].flag.py = 1;
	map->tinfo[0].flag.x = plw / 2;
	map->tinfo[0].flag.y = plh / 2;
	map->tinfo[0].lastspawn = 0;		// lastspawn
	for (i=0;i<8;i++) {	// teamspawns
		spoint_t spot;
		spot.px = 1;
		spot.py = 1;
		spot.x = 80 + 30 * i;
		spot.y = 3 * plh / 4;
		map->tinfo[0].spawn.push_back(spot);
	}

	// BUILD BLUE TEAM INFO
	//
	map->tinfo[1].flag.px = map->w-2;		// flag pos
	map->tinfo[1].flag.py = map->h-2;
	map->tinfo[1].flag.x = plw / 2;
	map->tinfo[1].flag.y = plh / 2;
	map->tinfo[1].lastspawn = 0;		// lastspawn
	for (i=0;i<8;i++) {	// teamspawns
		spoint_t spot;
		spot.px = map->w-2;
		spot.py = map->h-2;
		spot.x = 80 + 30 * i;
		spot.y = plh / 4;
		map->tinfo[1].spawn.push_back(spot);
	}
}

//************************************************************
//  the game frame common stuff
//************************************************************

// a player's record.
struct player_t {

	bool			used;		// player record valid?

	bool			isbot;	// BOTZ: TRUE when this player is a serverside bot

	//server-side flag: waiting for this client to say that it has all the resources needed to play
	bool			awaiting_client_ready;

	//player registration status
	char			reg_status;
	int				score, rank;
	int				neg_score;			// V0.4.8 NEW VAR

	//client-to-server bot configuration preferences:
	NLubyte		botsize;		//target team sizes
	NLubyte		botmode;		//wanted bot mode  0=default   1=use botsize   2=fill holes

	int				weapon;	// poder da arma atual (nivel) 0,1,2,3,4 (...?)

	bool			want_map_exit; //server side - player quer sair p/ proximo mapa na rotacao

	//#NR
	#ifdef NR_CONSOLE
	int mapVote;
	typedef list< pair<int, string> > DMQueueT;
	DMQueueT delayedMessages;	// int is the # of server frames the message has delay after the previous one
	#endif
	int kickTimer;
	int muted;	// 0 = no, 1 = yes, 2 = silently

	bool	want_change_teams;		//server-side: player wants do change teams?
	double	team_change_time;		//server-side time to allow next team change
	bool	team_change_pending;	//server-side hack

	double	speed_drop_time;		//speed powerup FX aux var
	double	wall_sound_time;		// min time to play sound again

	bool	onscreen;	//player onscreen? used only in clientside

	NLulong	enemyvis;	//enemies being viewed . clientside only

	//DEATHBRINGER
	bool	item_deathbringer;	// DEATHBRINGER -- if player carries it
	long	item_deathbringer_time;	//time of start explosion (server simulation)
	double	deathbringer_end;	// end time of deathbringer effect on this player
	bool	deathbringer_affected;		//CLIENT-SIDE: draw/spawn affected deathbringer effect
	double	death_drop_time;
	int		deathbringer_attacker;		//the attacker

	bool	item_shield;		// SHIELD: bit sent always: shield fx / shield present
	bool	item_quad;			// QUAD damage
	bool	item_speed;			// SPEED BOOTS
	int		item_helm;			// HELM 0== no   1+ == yes, alpha  (-1)

	double	item_quad_time;		// time of expiring (to print on clientside screen)
	double	item_speed_time;
	double	item_helm_time;

	double	quad_sound_finished;	// to avoid too much quad sound

	double	hitfx;		// player-hit fx (relative to time): clientside only

	bool	attack;		// if player is holding attack button

	int		id;			// player's id (position on the vectr)

	int		cid;		// client id (network identity)

	char	name[64]; // player's name

	double	waitnametime;		// protect from name change flooding

	int		x, y;			// position in world (screen coordinates)

	int		oldx, oldy;		// old positions (to detect screen changing)

	int		drawptr;		// HACK: id of the player to draw (depth sorting for drawing order)
	int		drawused;		// HACK: id of the player to draw (depth sorting for drawing order)

	int		ping;				// the ping time

	int		frags;			// integer number displayed on the scoreboard ("frags")
	int		oldfrags;		// last value informed to the client

	int		health;			// current health (sent always 2-byte)

	int		energy;			// player's energy (run/shoot)

	int		megabonus;	// bonus left de health e energy

	bool	dead;		//dead? zframe byte == 255  clientside only
	bool	old_dead;		// to detect time to play death sound

	bool	dropped_flag;   // Has player dropped the flag?

	double	next_shoot_time;		// minimum time for next shoot

	double	respawn_time;				// time for respawn

	bool	respawn_to_base;		// force respawning to base

	//talk flood control
	double	talk_temp;			//talk temperature
	double	talk_hotness;		//hotness of talk action

	//admin shell stats
	int		total_kills;
	int		total_deaths;
	int		total_suicides;
	int		total_captures;
	int		total_flags_taken;
	int		total_flags_dropped;
	int		total_flags_returned;
	int		total_flag_carriers_killed;
	int		total_shots;
	int		total_hits;
	int		total_shots_taken;
	int		start_time;

	void reset_message_queue_timing() {	// make messages already on queue appear instantly
		for (DMQueueT::iterator m=delayedMessages.begin(); m!=delayedMessages.end(); ++m)
			m->first=0;
	}
	void add_to_queue(const string& str) {
		int time;	// in server frames (1/10 sec)
		if (delayedMessages.size()<=5)
			time=0;
		else
			time=30;
		delayedMessages.push_back(pair<int, string>(time, str));
	}
	void queue_printf(const char* fmt, ...) {
		char buf[16385];
		va_list argptr;
		va_start(argptr, fmt);
		vsprintf(buf, fmt, argptr);
		va_end(argptr);
		add_to_queue(string(buf));
	}
	void clear(bool enable, bool is_bot, int _pid, int _cid, const char* _name) {
		ping = 0;
		frags = 0;	//reset score ?
		oldfrags = -666;
		want_map_exit = false;		//by default don't want change maps
		#ifdef NR_CONSOLE
		mapVote=-1;
		delayedMessages.clear();
		#endif
		kickTimer=0;
		muted=0;
		want_change_teams = false;	// don't want to change teams yet
		team_change_time = 0;
		team_change_pending = false;
		next_shoot_time = 0;
		attack = false;
		reg_status = ' ';
		talk_temp = 0.0;
		talk_hotness = 1.0;

		cid=_cid;
		id = _pid;
		strcpy(name, _name);	//the default name
		waitnametime = get_time() - 666.0;	//can change name right now
			//default stats
		total_kills = 0;
		total_deaths = 0;
		total_suicides = 0;
		total_captures = 0;
		total_flags_taken = 0;
		total_flags_dropped = 0;
		total_flags_returned = 0;
		total_flag_carriers_killed = 0;
		total_shots = 0;
		total_hits = 0;
		total_shots_taken = 0;
		start_time = (int)get_time();

		// BOTZ: bot ou nao?
		isbot = is_bot;
			//default bot preferences
		botmode = 0;
		botsize = 0;
			//score...  (V0.4.8 -- was missing!)
		score = 0;
		neg_score = 0;
		rank = 0;

		//#NR added to simulate memset(player_t, 0) :
		awaiting_client_ready=false;
		weapon=0;
		speed_drop_time=wall_sound_time=0;
		onscreen=false;
		enemyvis=0;
		item_deathbringer=deathbringer_affected=false;
		item_deathbringer_time=0;
		deathbringer_end=death_drop_time=0;
		deathbringer_attacker=0;
		item_shield=item_quad=item_speed=false;
		item_helm=0;
		item_quad_time=item_speed_time=item_helm_time=0;
		quad_sound_finished=hitfx=0;
		x=y=oldx=oldy=0;
		drawptr=drawused=0;
		health=energy=megabonus=0;
		dead=old_dead=false;
		dropped_flag=false;
		respawn_time=0;
		respawn_to_base=false;

		if (enable) {
			reg_status='-';
			used=true;
		}
		else
			used=false;
	}

};


//per-client struct (statically allocated to a single client)
class oneclient_c {
public:

	//v0.4.4 UDP FILE transfer
	bool		serving_udp_file;			//if TRUE, already serving a file
	char		*data;					//the file data
	NLulong		dp,old_dp,fsize;				//the file pointer and the total size

	//v0.4.4 PLAYER REGISTRATION STATUS
	bool		token_have;					//player claims to be registered with (name,token)
	bool		token_valid;		//player (name,token) is validated
	char		token[64];					//the player's token
	int			intoken;						//integer version of token

	//v0.4.4 client statistics
	int			delta_score;		//the player's score accumulator
	int			neg_delta_score;		//NEG score accum 0.4.8

	double fdp;		//DOUBLE delta accums. os acima sao apenas o "trunc atual"
	double fdn;

	int			rank;						//current ranking position
	int			score;					//current score POS -- SOMATORIO (né?!?!?)
	int			neg_score;					//current score NEG 0.4.8 -- SOMATORIO (né?!?!?)

	oneclient_c() {
		serving_udp_file = false;
		data = 0;
		reset();
	}

	//chamado no fim do UDP!
	void download_reset() {
		if ((serving_udp_file) && (data)) {
			delete data;
			data = 0;
		}
		serving_udp_file = false;
	}

	void reset() {
		delta_score = 0;
		neg_delta_score = 0;
		fdp = 0.0;
		fdn = 0.0;
		score = 0;
		neg_score = 0;
		rank = 0;

		token_have = false;
		token_valid = false;
		token[0]=0;
		intoken=666;

		download_reset();
	}

	~oneclient_c() {
		download_reset();
	}
};

// a player's sprite state
struct hero_t {
	int tx, ty;		//tela X,Y
	double x, y, sx, sy;	// position and speed
	double ox, oy;	// old coords: garantidamente NAO em paredes
	bool l, r, u, d;	// left, right, up, down keypresses (player acceleration vectrs)
	int gundir;	// gun direction 0-7 (0 = right 1 = right-down 2 = down ...... 7 = right-up
	bool strafe;	// strafing?
	bool run;	//running

	//keypresses (net format)
	// 0,1,2,3 : lrud
	// 4 : strafe?
	// 5,6,7 : gun direction

	//CLIENT writes: l,r,u,d,strafe,shift,unused(2)

	//SERVER writes: l,r,u,d,shift,gundir(3)
	NLubyte		keys;
};

// a rocket-shot
class rocket_c {
public:

	//owning player-id (-1 == unused)
	int	owner;

	//don't draw flag & remove schedule (CLIENT-SIDE): se dontdraw==true, nao desenha em client side e remove quando tempo >= clremove
	bool dontdraw;
	double clremove;

	//team/color
	int team;

	//power	(na verdade a partir da versao 0.1.2, cada rocket pode ser um multi-rocket!
	//NLubyte		power;

	//notification list (bitfield, bit0=player0, bit1=player1... etc.)
	NLulong		vislist;

	//hit position
	NLshort hitx, hity;

	//screen coords
	int px, py;

	//start position or current position
	double x, y;

	//v0.1.2 - how long it moved (in pixels) since creation
	double d;

	//v0.1.2 - em graus : direcao
	double deg;

	//speed
	double sx, sy;

	//time of shot or current time
	NLulong time;

	//time for effective calculation on clientside (not always integer)
	double cl_time;

	//time-of-hit do rocket clientside
	double hit_time;

	//hit_target. se ==255, ninguem em particular.  se ==254 hit wall
	int hit_target;

	rocket_c() { owner = -1; }
};

struct ctflag_t {

	//carried? else dropped somewhere
	bool			carried;

	//if not carried, dropped at base?
	bool			atbase;

	//who owns it if carried
	int				carrier;

	//score of the "flag" (team score)
	int				score;

	//0.4.7 tempo em que adversario pegou a flag do estande na base
	double		grab_time;

	//where is it if dropped
	spoint_t	pos;
};

//pickups
class pickup_c {
public:

	NLubyte kind;		// type of powerup  0==unused     255=valid, but respawning

	double		respawn_time;		// time to respawn

	int				px;	//screen
	int				py;
	int				x;	//position
	int				y;

	pickup_c() { kind=0; }
};

// a game frame, or game state
class frame_t {
public:

	// frame is invalid -- when frame is skipped in the broadcast
	bool skipped;

	// frame number  (simulation time)
	double		frame;

	// real time (clientside) of the frame
	double		time;

	// the player's sprite positions
	hero_t		hero[MAX_PLAYERS];

	// flag objects - one for each team
	ctflag_t		flag[2];

	// rockets
	rocket_c	rock[MAX_ROCKETS];

	// pickups
	pickup_c	item[MAX_PICKUPS];

	//ctor
	frame_t() {
		frame = 0;
		time = 0;
		for (int i=0;i<MAX_PLAYERS;i++)
			memset(&hero[i], 0, sizeof(hero_t));
	}

	//dtor
	virtual ~frame_t() {
	}
};

// tenta carregar um mapa do arquivo OUTGUN_DIR/mapdir/mapname.TXT
// retorna FALSE se nao conseguiu abrir ou se tava corrompido etc
// retorna o mapa (map_c) atualizado no parametro *map

bool load_map(char *mapdir, char *mapname, Map *map) {
	//clear
	char lebuffer[1024];
	char dest[WHERE_PATH_SIZE];

	// MAPDIR + / + MAPNAME + .TXT
	strcpy(lebuffer, mapdir);
	put_backslash(lebuffer);
	strcat(lebuffer, mapname);
	strcat(lebuffer, ".txt");

	//append all that to the root dir of the game
	append_filename(dest, wheregamedir, lebuffer, WHERE_PATH_SIZE);
	FILE *fmap = fopen(dest, "r");	// FIXME: r or rb ??
	if (fmap) {
		LOG1("LOAD_MAP MAP FILENAME IS '%s'\n", mapname);
		if (!map->load(fmap)) {
			LOG1("Error loading map '%s'\n", mapname);
			return false;
		}
		fclose(fmap);
		return true;
	}
	else {
		LOG1("can't load mapfile from '%s'!\n", dest);
		return false;
	}
}

bool NR_applyPhysics(hero_t* h, const Room& room, float fraction, bool turbo, bool carryFlag, bool deathbringer_affected) {
	//select effective physics vars for the player
	//
	float player_accel;
	float player_friction;
	float player_maxspeed;
	if (h->run) {
		if (turbo) {
			player_accel    = svp_accel_turborun;
			player_friction = svp_fric_turborun;
			player_maxspeed = svp_maxspeed_turborun;
		}
		else {
			player_accel    = svp_accel_run;
			player_friction = svp_fric_run;
			player_maxspeed = svp_maxspeed_run;
		}
	}
	else {
		if (turbo) {
			player_accel    = svp_accel_turbo;
			player_friction = svp_fric_turbo;
			player_maxspeed = svp_maxspeed_turbo;
		}
		else {
			player_accel    = svp_accel;
			player_friction = svp_fric;
			player_maxspeed = svp_maxspeed;
		}
	}
	//flag carrier disadvantage when running
	if (h->run && carryFlag)
		player_maxspeed -= svp_flag_penalty;

	int xAcc = (h->r?1:0) - (h->l?1:0), yAcc = (h->d?1:0) - (h->u?1:0);

	#ifdef NR_PHYS_VECTOR_ACC

	// scale these up so the acceleration is near the average of original (either 1 or sqrt(2) times)
	player_accel *= 1.2;
	player_friction *= 1.2;
	player_maxspeed *= 1.2;

	// friction
	float spd = sqrt( h->sx*h->sx + h->sy*h->sy );
	float mul;
	if (spd < player_friction)
		mul = 0.;
	else
		mul = 1. - player_friction/spd;
	spd *= mul;
	h->sx *= mul;
	h->sy *= mul;

	// acceleration
	if (!deathbringer_affected && spd<player_maxspeed) {
		float mul = player_accel+player_friction;
		if (xAcc!=0 && yAcc!=0)	// normalize the total acceleration vector
			mul /= sqrt(2.);

		h->sx += float(xAcc)*mul;
		h->sy += float(yAcc)*mul;

		spd = sqrt( h->sx*h->sx + h->sy*h->sy );
		if (spd > player_maxspeed) {
			float mul = player_maxspeed/spd;
			h->sx *= mul;
			h->sy *= mul;
		}
	}

	#else	// NR_PHYS_VECTOR_ACC

	// friction
	if (!xAcc) {
		if (fabs(h->sx) > player_friction)
			h->sx *= 1. - player_friction/fabs(h->sx);
		else
			h->sx = 0.;
	}
	if (!yAcc) {
		if (fabs(h->sy) > player_friction)
			h->sy *= 1. - player_friction/fabs(h->sy);
		else
			h->sy = 0.;
	}

	// acceleration
	if (!deathbringer_affected) {
		if (xAcc!=0) {
			h->sx += float(xAcc)*player_accel;
			if (fabs(h->sx) > player_maxspeed)
				h->sx *= player_maxspeed/fabs(h->sx);
		}
		if (yAcc!=0) {
			h->sy += float(yAcc)*player_accel;
			if (fabs(h->sy) > player_maxspeed)
				h->sy *= player_maxspeed/fabs(h->sy);
		}
	}

	#endif	// NR_PHYS_VECTOR_ACC else

	h->ox=h->x; h->oy=h->y;
	h->x+=h->sx;
	h->y+=h->sy;

	//wall collision correction
	return NR_wallcorrect(room, fraction, &h->x, &h->y, &h->sx, &h->sy, &h->ox, &h->oy);
}

#ifdef NR_SUPPORT_OLD_CLIENTS
bool applyDefaultPhysics(hero_t* h, const Room& room, float fraction, bool turbo, bool carryFlag, bool deathbringer_affected, bool fixBouncing) {
	//select effective physics vars for the player
	//
	float player_accel;
	float player_friction;
	float player_maxspeed;
	if (h->run) {
		if (turbo) {
			player_accel    = svp_accel_turborun;
			player_friction = svp_fric_turborun;
			player_maxspeed = svp_maxspeed_turborun;
		}
		else {
			player_accel    = svp_accel_run;
			player_friction = svp_fric_run;
			player_maxspeed = svp_maxspeed_run;
		}
	}
	else {
		if (turbo) {
			player_accel    = svp_accel_turbo;
			player_friction = svp_fric_turbo;
			player_maxspeed = svp_maxspeed_turbo;
		}
		else {
			player_accel    = svp_accel;
			player_friction = svp_fric;
			player_maxspeed = svp_maxspeed;
		}
	}
	//flag carrier disadvantage when running
	if (h->run && carryFlag)
		player_maxspeed -= svp_flag_penalty;

	//friction x - apply if l xor r
	#ifndef ALWAYS_FRICTION
	if ( ((int)h->l + (int)h->r != 1) || (fabs(h->sx) > player_maxspeed) ) {
	#endif
		if (h->sx > 0) {
			//h->sx -= sv_frictionx * boots_accel_bonus;
			h->sx -= player_friction * fraction;
			if (h->sx < 0) h->sx = 0;
		}
		else if (h->sx < 0) {
			//h->sx += sv_frictionx * boots_accel_bonus;
			h->sx += player_friction * fraction;
			if (h->sx > 0) h->sx = 0;
		}
	#ifndef ALWAYS_FRICTION
	}
	#endif

	//friction y
	#ifndef ALWAYS_FRICTION
	if ( ((int)h->u + (int)h->d != 1) || (fabs(h->sy) > player_maxspeed) ){
	#endif
		if (h->sy > 0) {
			//h->sy -= sv_frictiony * boots_accel_bonus;
			h->sy -= player_friction * fraction;
			if (h->sy < 0) h->sy = 0;
		}
		else if (h->sy < 0) {
			//h->sy += sv_frictiony * boots_accel_bonus;
			h->sy += player_friction * fraction;
			if (h->sy > 0) h->sy = 0;
		}
	#ifndef ALWAYS_FRICTION
	}
	#endif

	//deathbringer penalty : no movement. move only if not in effect
	if (!deathbringer_affected) {

		//accelerate x if not over maximum speed
		if ((h->l) && (h->sx > -player_maxspeed))
			h->sx -= player_accel * fraction;
		if ((h->r) && (h->sx < +player_maxspeed))
			h->sx += player_accel * fraction;
		//accelerate y if not over maximum speed
		if ((h->u) && (h->sy > -player_maxspeed))
			h->sy -= player_accel * fraction;
		if ((h->d) && (h->sy < +player_maxspeed))
			h->sy += player_accel * fraction;
	}

	//DEBUG
	//if (h->sx > +player_maxspeed) h->sx = +player_maxspeed;
	//if (h->sx < -player_maxspeed) h->sx = -player_maxspeed;
	//if (h->sy > +player_maxspeed) h->sy = +player_maxspeed;
	//if (h->sy < -player_maxspeed) h->sy = -player_maxspeed;

	//save ox,oy
	h->ox = h->x;
	h->oy = h->y;

	//wall collision correction
	if (fixBouncing) {
		h->x += h->sx;
		h->y += h->sy;
		return NR_wallcorrect(room, fraction, &h->x, &h->y, &h->sx, &h->sy, &h->ox, &h->oy);
	}
	else {
		//move x
		h->x += h->sx * fraction;
		if (h->x < 0) h->x = 0;
		else if (h->x > plw) h->x = plw;

		//move y
		h->y += h->sy * fraction;
		if (h->y-NR_SHIFTY < 0) h->y = 0+NR_SHIFTY;
		else if (h->y-NR_SHIFTY > plh) h->y = plh+NR_SHIFTY;

		return wallcorrect(room, &h->x, &h->y, &h->sx, &h->sy, &h->ox, &h->oy);
	}
}
#endif

//************************************************************
//  server stuff
//************************************************************

//delay for the server contacting the master server, in seconds
// it is good if this delay is set to a minute or so, since this will
// filter out people opening and closing servers frequently
#define	DELAY_TO_REPORT_SERVER	10.0

//bot frame extrapolation depth (numero de frames que o bot extrapola pra frente)
#define BOT_XDEPTH 10

//server_next_map() reasons
enum {
	NEXTMAP_NONE,
	NEXTMAP_CAPTURE_LIMIT,
	NEXTMAP_VOTE_EXIT,
	NUM_OF_NEXTMAP
};

#define SERVER_DEFAULT_PLAYER_NAME "**DEFAULT**"

#define MAPFILENAMESIZE 32			// e tah mais que bom!
//#define MAPROTSIZE 32			//tambem mais do que bom
#define MAPROTSIZE 200			//tambem mais do que bom //#NR

// client count
int	player_count;

// serverside bot count BOTZ
int bot_count;

// server callbacks
int sfunc_client_hello(runes_t *arg);
int sfunc_client_connected(runes_t *arg);
int sfunc_client_disconnected(runes_t *arg);
int sfunc_client_lag_status(runes_t *arg);
int sfunc_client_data(runes_t *arg);
int sfunc_client_ping_result(runes_t *arg);

//threads for tcp file transfer
void *thread_filemaster_f(void *arg);
void *thread_fileslave_f(void *arg);

//thread for master server interaction (LIST SERVER)
void *thread_mastertalker_f(void *arg);

//thread for server shell connections
void *thread_shellmaster_f(void *arg);
void *thread_shellslave_f(void *arg);

//master job (ACCOUNT/STATS SERVER)
void *thread_masterjob_f(void *arg);

//master job struct
class masterjob_c {
public:

	char								request[512];		//http request to be sent

	bool		html_end;			//received a response(request fullfilled)

	char				lebuf[65536];		//lebuf for collecting response
	int					n;			//lebuf length

	int			code;			//job code

	//VARS FOR EACH SPECIFIC JOB CODE
	int			cid;		//code 1 - client id

	//return values of the callback
	bool		retry;		//if true, wait a bit and retry

	masterjob_c() {
		lebuf[0]=0;
		html_end = false;
		request[0]=0;
	}
};

// the game server state
class gameserver_c {
public:

	// the current worldmap
	Map		map;

	// net server
	server_c	*server;

	//host ad
	char hostadname[128];

	//master server interaction thread and socket
	NLsocket		msock;
	pthread_t		mthread;
	double			master_talk_time;	//time to talk?
	bool				master_pre_exiting_ok;		// if no need to kill the master socket...
	bool				master_exiting_ok;		// if no need to kill the master socket...
	bool				master_never_talked;		// if never talked to master, then no need to unregister the server when qutting (optimization)

	//master job thread count
	bool							mjob_exit;				//flag for all pending master jobs to quit now
	bool							mjob_fastretry;		//flag for all pending master jobs to stop waiting and retry immediately
	int								mjob_count;
	pthread_mutex_t		mjob_mutex;  //mutex for socket list

	// socket for connection acceptance
	bool							file_threads_quit;		//terminate all file server threads/sockets now
	NLsocket					filesock;
	pthread_t					server_filemaster_thread;  // thread for server filemaster
	pthread_mutex_t		fslavesock_mutex;  //mutex for socket list
	NLsocket					fslavesock[MAX_PLAYERS];
	pthread_t					fslavethr[MAX_PLAYERS];

	//shell socket
	NLsocket		shellmsock;
	pthread_t		shellmthread;
	NLsocket		shellssock;
	pthread_t		shellsthread;

	//hostname
	char		hostname[256];

	vector<string> welcome_message;	// welcome message line by line
	vector<string> info_message;	// the message /info shows, line by line

	// the players
	player_t	player[MAX_PLAYERS];
	int				ctop[256];			// client id-to-player id index. ctop[255] reservado para lixo area de bots

	//the CLIENTS (v0.4.4  -- this should have been in here before)
	oneclient_c	client[MAX_PLAYERS];

	//outgun network stats
	int max_world_score, max_world_rank;

	//V0.4.8: the TEAM's score modifiers
	double team_smul[2];

	//the current frame (game world simulation state)
	frame_t		world;

	//frames extrapoladas tempo zero: world  tempo 1 em diante:		xw[0] em diante
	frame_t		xw[BOT_XDEPTH];

	//bot target configuration
	int  botmode;		//1= fill-in (default)  2= target team size
	int  botsize;

	// current frame count
	NLulong		frame;

	//total server traffic in kbytes/sec
	double server_kbps_traffic;

	// ping send counter
	int ping_send_counter, ping_send_client;

	// server map rotation list
	int		maprots;		//quantos mapas em maprots
	int		currmap;		//mapa atual do maprot
	bool	builtin;		//using the builtin map
	char	maprot[MAPROTSIZE][MAPFILENAMESIZE];
	#ifdef NR_CONSOLE
	struct MapInfo {
		string title, file;
		int width, height;
		int votes;
		MapInfo() : votes(0) { }
	};
	MapInfo mapinfo[MAPROTSIZE];
	#endif
	#ifdef NR_NAME_AUTHORIZATION
	NameAuthorizationDatabase authorizations;
	#endif
	bool random_maprot;

	// vote announce timer
	NLulong next_vote_announce_frame;
	int last_vote_announce_votes, last_vote_announce_needed;

	NLulong map_start_time;	// frame #

	//server showing gameover plaque?
	bool	gameover;
	double		gameover_time;		//timeout for gameover plaque

	NLulong time_limit;
	int capture_limit;

	int vote_block_time;	// how long a mapchange can't be voted (except unanimously), in frames (in gamemod, it is minutes)

	//server parameters: powerups
	int pups_min, pups_max, pups_respawn_time, pup_chance_shield, pup_chance_turbo, pup_chance_shadow,
			pup_chance_power, pup_chance_weapon, pup_chance_megahealth, pup_chance_deathbringer;
	bool pups_min_percentage, pups_max_percentage;
	int pup_add_time, pup_max_time;
	bool pup_deathbringer_switch;
	int shadow_minimum;	// smallest alpha value allowed; 1 is when even the coordinates are not sent
	double respawn_time, waiting_time_deathbringer;

	//ctor
	gameserver_c() {
		server = 0;
		hostname[0]=0;	//hostname
		//memset(&world, 0, sizeof(frame_t));		//the current frame (game world simulation state)
		frame = 0;		// current frame count
		next_vote_announce_frame = 0;
		last_vote_announce_votes = last_vote_announce_needed = 0;

		server_kbps_traffic = 0;		//total server traffic in kbytes/sec
		ping_send_counter = 0;		// ping send counter
		ping_send_client = 0;

		for (int i=0; i<256; ++i)
			ctop[i]=-1;

		pthread_mutex_init(&fslavesock_mutex, 0);

		pthread_mutex_init(&mjob_mutex, 0);
	}

	//dtor
	virtual ~gameserver_c() {

		pthread_mutex_destroy(&fslavesock_mutex);

		pthread_mutex_destroy(&mjob_mutex);

		LOG("GAMESERVER_C() DESTRUCTOR");
		if (server) {
			delete server;
			server = 0;
		}
	}

	//#NR
	void mutePlayer(int pid, int mode) {	// 0 = unmute, 1 = normal, 2 = mute silently (do not inform the player)
		if (mode==0 && player[pid].muted!=2)
			plprintf(pid, "@WYou have been unmuted (you can send messages again)");
		else if (mode == 1)
			plprintf(pid, "@WYou have been muted (you can't send messages)");
		for (int i=0; i<MAX_PLAYERS; ++i)
			if (player[i].used && !player[i].isbot && i!=pid) {
				if (mode == 0)
					plprintf(i, "@IThe admin has unmuted %s", player[pid].name);
				else
					plprintf(i, "@IThe admin has muted %s", player[pid].name);
			}
		player[pid].muted = mode;
	}
	void kickPlayer(int pid, bool ban=false) {
		player[pid].delayedMessages.clear();
		if (ban)
			plprintf(pid, "@WYou are now BANNED from this server! Have a nice life...");
		else {
			plprintf(pid, "@WYou are being kicked from this server!");
			plprintf(pid, "@WWarning: you can get permanently banned for behaving badly!");
		}
		for (int i=0; i<MAX_PLAYERS; ++i)
			if (player[i].used && !player[i].isbot && i!=pid)
				plprintf(i, "@IThe admin has %s %s (disconnect in 10 seconds)", ban?"banned":"kicked", player[pid].name);
		player[pid].kickTimer = 10*10;
	}

	#ifdef NR_NAME_AUTHORIZATION
	void banPlayer(int pid) {
		authorizations.ban(server->get_client_address(player[pid].cid));
		authorizations.save();
		kickPlayer(pid, true);
	}
	#endif

	//v0.4.4 choose a kind from all chances
	int choose_powerup_kind() {

		int max = pup_chance_shield + pup_chance_turbo + pup_chance_shadow + pup_chance_power
							+ pup_chance_weapon + pup_chance_megahealth + pup_chance_deathbringer;

		int chance = 1 + rand() % max;		//1..100 por exemplo se max = 100

		chance -= pup_chance_shield;
		if (chance <= 0) return 1;
		chance -= pup_chance_turbo;
		if (chance <= 0) return 2;
		chance -= pup_chance_shadow;
		if (chance <= 0) return 3;
		chance -= pup_chance_power;
		if (chance <= 0) return 4;
		chance -= pup_chance_weapon;
		if (chance <= 0) return 5;
		chance -= pup_chance_megahealth;
		if (chance <= 0) return 6;
		//chance -= pup_chance_deathbringer;
		return 7;
	}

	// ---- FILE DOWNLOADING TO CLIENTS VIA UDP -------

	//upload next chunk of a file to a client_c
	void upload_next_file_chunk(int i) {

		int CHUNKSIZE = 128;		// the max chunk size in bytes

		//actual size sent
		int chunksize = client[i].fsize - client[i].dp;		//attempt to send remaining...
		if (chunksize > CHUNKSIZE)							//...but there is the maximum
			chunksize = CHUNKSIZE;

		//check if will be last
		NLubyte islast = 0;	//default:no
		if (client[i].dp + chunksize == client[i].fsize) //maybe yes?
			islast = 1;		//yes.

		//send
		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 28);		//28 = next file chunk 4 u....
		writeByte(lebuf, count, islast);
		writeLong(lebuf, count, client[i].dp );
		writeShort(lebuf, count, ((NLushort)chunksize) );
		writeBlock(lebuf, count, &(client[i].data[ client[i].dp ] ), chunksize);
		server->send_message(i, lebuf, count);

		//save old dp for the ack
		client[i].old_dp = client[i].dp;

		//inc dp
		client[i].dp += chunksize;
	}

	//---- FILE DOWNLOADING TO CLIENTS VIA TCP --------

	//load file from disk. puts file of type/name into the given buffer, returns filesize
	int get_download_file(char *lebuf, char *ftype, char *fname) {

		//map file type
		if (!strcmp(ftype, "map")) {
			//#NR
			if (strpbrk(fname, "./:\\")!=NULL) {
				LOG1("*!*!*!* ILLEGAL FILE DOWNLOAD ATTEMPT: MAP \"%s\"\n", fname);
				return -1;	//#should also kick their butt for that
			}

			char lebuffer[1024];
			char dest[WHERE_PATH_SIZE];

			// MAPDIR + / + MAPNAME + .TXT
			strcpy(lebuffer, SERVER_MAPS_DIR);
			put_backslash(lebuffer);
			strcat(lebuffer, fname);
			strcat(lebuffer, ".txt");

			//append all that to the root dir of the game
			append_filename(dest, wheregamedir, lebuffer, WHERE_PATH_SIZE);
			FILE *fmap = fopen(dest, "rb");
			if (fmap) {
				int amount = fread(lebuf, 1, 65536, fmap);
				fclose(fmap);
				LOG1("UPLOADING MAP \"%s\" (SV)\n", fname);
				return amount;	//size read!
			}
			else {
				LOG1("FAILED MAP DOWNLOAD ATTEMPT \"%s\" (SV)\n", fname);
				return -1;	//can't read!
			}
		}

		// don't know type!
		return -1;
	}

	//run a file master thread
	void run_filemaster_thread(void *arg) {

		while (1) {
			//accept one connection
			nlEnable(NL_BLOCKING_IO);
			NLsocket pidaosock = nlAcceptConnection(filesock);
			nlDisable(NL_BLOCKING_IO);

			//valid socket?
			if (pidaosock != NL_INVALID) {

				// FIXME: deny connections from client IPs that are not connected to the server already!

				//create thread for serving this client
				// & update list
				int i;
				pthread_mutex_lock( &fslavesock_mutex );

				for (i=0;i<MAX_PLAYERS;i++)
					if (fslavesock[i] == NL_INVALID) {
						pthread_create(&(fslavethr[i]), 0, thread_fileslave_f, (void *)i);	//keep thread so we can join
						fslavesock[i] = pidaosock;		//keep socket so it can be closed
						break;
					}

				pthread_mutex_unlock( &fslavesock_mutex );
			}
			else {
				if (file_threads_quit) {		//quitting!
					LOG("FILEMASTER THREAD IS QUITTING\n");
					nlClose(filesock);
					return;
				}
			}

			//sleep a bit
			MS_SLEEP(500);
		}
	}

	//run a file slave thread
	void run_fileslave_thread(void *arg) {

		//NLsocket sock = (NLsocket)arg;

		int k = (int)arg;

		pthread_mutex_lock( &fslavesock_mutex );
		NLsocket sock = fslavesock[k];
		pthread_mutex_unlock( &fslavesock_mutex );

		char lebuf[65536]; int count = 0;

		char ftype[256];
		char fname[256];

		while (1) {

			//read what client wants
			NLint result = nlRead(sock, lebuf, 1);
			if (file_threads_quit) break;
			//FIXME: check result
			if (result == 0) {
				MS_SLEEP(100);	//this should never be happening
				continue;
			}
			if (result != 1) {
				//not read a byte, NL_INVALID!
				//FIXME: client misbehaved, should also disconnect him
				LOG("ERROR: CLIENT MISBEHAVED HIS FILE SOCKET, QUITTING THREAD!");
				break;
			}

			NLubyte req;
			count = 0;
			readByte(lebuf, count, req);

			//what you want?
			if (req == 1) {		// gimme file!
				LOG("FILE SLAVE THREAD: CLIENT WANTS FILE!\n");

				int i;	//index
				bool bad_error;

				//READ ONE STRING ON LEBUF
				i = 0;
				bad_error = false;
				do {
					result = nlRead(sock, &(lebuf[i]), 1);
					if (file_threads_quit) break;
					//FIXME: check result
					if (result == 0) {
						MS_SLEEP(100);	//this should never be happening
						continue;
					}
					if (result != 1) {
						//not read a byte, NL_INVALID!
						//FIXME: client misbehaved, should also disconnect him
						bad_error = true;
						break;
					}

					if (lebuf[i] == 0)
						break;
					i++;
				} while (1);
				if (file_threads_quit) break;

				//can't break from inside the loop
				if (bad_error) {
					LOG("ERROR: CLIENT MISBEHAVED HIS FILE SOCKET, QUITTING THREAD! (2)");
					break;
				}

				//copy to ftype
				strcpy(ftype, lebuf);

				LOG1("FILE SLAVE THREAD: TYPE IS '%s'!\n", ftype);

				//READ ONE STRING ON LEBUF
				i = 0;
				bad_error = false;
				do {
					result = nlRead(sock, &(lebuf[i]), 1);
					if (file_threads_quit) break;
					//FIXME: check result
					if (result == 0) {
						MS_SLEEP(100);	//this should never be happening
						continue;
					}
					if (result != 1) {
						//not read a byte, NL_INVALID!
						//FIXME: client misbehaved, should also disconnect him
						bad_error = true;
						break;
					}
					if (lebuf[i] == 0)
						break;
					i++;
				} while (1);
				if (file_threads_quit) break;

				//can't break from inside the loop
				if (bad_error) {
					LOG("ERROR: CLIENT MISBEHAVED HIS FILE SOCKET, QUITTING THREAD! (3)");
					break;
				}

				//copy to fname
				strcpy(fname, lebuf);

				LOG1("FILE SLAVE THREAD: NAME IS '%s'!\n", fname);

				//load file from disk. puts file of type/name into the given buffer, returns filesize
				int filesize = get_download_file((char *)lebuf, ftype, fname);
				if (file_threads_quit) break;

				// FIXME: write 4 or something to signal THAT THE FILE WAS NOT FOUND then quit
				if (filesize == -1) {
					//FIXME: also client misbehaved -- asked for bad file -- disconnect him!
					LOG("ERROR: CLIENT MISBEHAVED ASKING FOR BAD FILE, QUITTING THREAD!");
					break;
				}

				//write 2 (sending file baby!)
				char	leheader[256];
				count = 0;
				writeByte(leheader, count, 2);	//ok baby, sending!
				writeShort(leheader, count, 0);		//FIXME: send file CRC
				writeLong(leheader, count, (NLulong)filesize);	// file size
				result = nlWrite(sock, leheader, count);
				if (file_threads_quit) break;
				//FIXME: check result
				if (result != count) {
					//not read a byte, NL_INVALID!
					//FIXME: client misbehaved, should also disconnect him
					LOG("ERROR: CLIENT MISBEHAVED HIS FILE SOCKET, QUITTING THREAD! (4)");
					break;
				}

				LOG1("FILE SLAVE THREAD: FILESIZE UPLOADED IS %i\n", filesize);

				//send file to client as a big chunk)
				// FIXME: make this send in smaller pieces
				result = nlWrite(sock, lebuf, filesize);
				if (file_threads_quit) break;
				// FIXME: check result
				if (result != filesize) {
					//not read a byte, NL_INVALID!
					//FIXME: client misbehaved, should also disconnect him
					LOG("ERROR: CLIENT MISBEHAVED HIS FILE SOCKET, QUITTING THREAD! (5)");
					break;
				}

				LOG1("FILE SLAVE THREAD: RESULT OF UPLOAD IS %i\n", result);
			}
			else if (req == 3) {	// bye!
				LOG("FILE SLAVE THREAD GIVING BYE...\n");
				//ok..
				//nlClose(sock);
				break;
			}
		}

		//thread is no more - will quit by itself, no need to notify
		nlClose(sock);
		LOG("FILESLAVE THREAD IS QUITTING...");
		pthread_mutex_lock( &fslavesock_mutex );
		fslavethr[k] = (pthread_t)-1;		//"invalid"?
		fslavesock[k] = NL_INVALID;
		pthread_mutex_unlock( &fslavesock_mutex );
		LOG("QUIT!\n");
	}

	//---- CTF/GAME ------------------

	//also called the "first packet"
	// na verdade eh um packet de proposito geral. manda sempre que:
	//  - fisica mudar
	//  - "myself" de um client mudar
	void send_me_packet(int pid) {
		int count = 0;
		char lebuf[1024];
		writeByte(lebuf, count, 3); // "3" = first-packet information
		writeByte(lebuf, count, ((NLubyte)pid) );		// who am I
		writeByte(lebuf, count, ((NLubyte)world.flag[0].score) );		//team 0 current score
		writeByte(lebuf, count, ((NLubyte)world.flag[1].score) );		//team 1 current score
		//server physics parameters
		writeFloat(lebuf, count, ((NLfloat)svp_fric) );
		writeFloat(lebuf, count, ((NLfloat)svp_accel) );
		writeFloat(lebuf, count, ((NLfloat)svp_maxspeed) );
		writeFloat(lebuf, count, ((NLfloat)svp_fric_run) );
		writeFloat(lebuf, count, ((NLfloat)svp_accel_run) );
		writeFloat(lebuf, count, ((NLfloat)svp_maxspeed_run) );
		writeFloat(lebuf, count, ((NLfloat)svp_fric_turbo) );
		writeFloat(lebuf, count, ((NLfloat)svp_accel_turbo) );
		writeFloat(lebuf, count, ((NLfloat)svp_maxspeed_turbo) );
		writeFloat(lebuf, count, ((NLfloat)svp_fric_turborun) );
		writeFloat(lebuf, count, ((NLfloat)svp_accel_turborun) );
		writeFloat(lebuf, count, ((NLfloat)svp_maxspeed_turborun) );
		writeFloat(lebuf, count, ((NLfloat)svp_flag_penalty) );
		server->send_message(player[pid].cid, lebuf, count);
	}

	//send a player name update to a client
	void send_player_name_update(int cid, int pid) {

		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 1);	// "1" = player name update
		writeByte(lebuf, count, pid);		// what player id
		player[pid].name[15]=0;		// force trunc name at 15 chars (paranoia)
		writeString(lebuf, count, player[pid].name);
		server->send_message(cid, lebuf, count);
	}

	//broadcast new player name
	void broadcast_player_name(int pid) {

		for (int i=0;i<maxplayers;i++)
		if (player[i].used)
		if (!player[i].isbot)
			send_player_name_update(player[i].cid, pid);

		//server->broadcast_message(lebuf, count);
	}

	//send a player crap update to a client
	void send_player_crap_update(int cid, int pid) {

		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 32);	// "32" = player CRAP update

		// --- RECALC CRAP ---
		//reg_status char:
		if (client[ player[pid].cid ].token_have) {
			if (client[ player[pid].cid ].token_valid)
				player[pid].reg_status = '*';
			else
				player[pid].reg_status = '?';
		}
		else
			player[pid].reg_status = ' ';

		writeByte(lebuf, count, ((NLubyte)pid));
		writeByte(lebuf, count, ((NLubyte)player[pid].reg_status));					//regstatus
		writeLong(lebuf, count, ((NLulong)client[player[pid].cid].rank));		//ranking#
		writeLong(lebuf, count, ((NLulong)client[player[pid].cid].score));		//score POS
		writeLong(lebuf, count, ((NLulong)client[player[pid].cid].neg_score));		//score NEG v0.4.8
		writeLong(lebuf, count, ((NLulong)max_world_rank));		//MAX WORLD ranking#
		writeLong(lebuf, count, ((NLulong)max_world_score));		//MAX WORLD score

		//LOG5("CRAPZ SENT TO CID %i of PID %i %c r:%i s:%i\n", cid, pid, player[pid].reg_status
		//	,client[player[pid].cid].rank
		//	,client[player[pid].cid].score);

		server->send_message(cid, lebuf, count);
	}

	//v0.4.5: broadcast player crap
	void broadcast_player_crap(int pid) {

		for (int i=0;i<maxplayers;i++)
		if (player[i].used)
		if (!player[i].isbot)
			send_player_crap_update(player[i].cid, pid);
	}

	//ugly hack
	int check[MAX_PLAYERS];
	int checount;

	//check if team change requests can be satisfied
	void check_team_changes() {

		// check players in random order
		//
		for (int i=0;i<maxplayers;i++) check[i]=0;
		checount = maxplayers;
		while (checount > 0) {

			int p = rand() % maxplayers;
			if (!check[p]) {
				check[p] = 1;
				checount--;
				check_player_change_teams(p);
			}
		}
	}

	//check if a player wants to change teams and if yes, try to fullfill the wish
	void check_player_change_teams(int pid) {

		//valid players that want to change teams only
		if (!player[pid].used) return;
		if (!player[pid].want_change_teams) return;
		if (get_time() < player[pid].team_change_time) {
			player[pid].team_change_pending = true;	//vai continuar tentando
			return;	//v0.3.3 : intervalos minimos para troca de times
		}

		//count players in each team - IGNORE BOTS
		int tc[2];tc[0]=0;tc[1]=0;
		for (int i=0;i<maxplayers;i++)
			if (player[i].used)
			if (!player[i].isbot)		// IGNORE BOTS
				tc[i/TSIZE]++;

		//check if team changing happens: calculate delta TARGET TEAM - MY TEAM
		int teamdelta = tc[1-(pid/TSIZE)] - tc[pid/TSIZE];

		//case 0: target team with MORE players: do not move
		if (teamdelta > 0) {
		}
		//case 1: target team with 2 players less:  move player without trades
		else if (teamdelta <= -2) {
			// MOVE W/O TRADE
			for (int i=0;i<maxplayers;i++)
			if (i/TSIZE != pid/TSIZE)			// no time oposto
			{
				if (!player[i].used)		// player vago
				{
					move_player(pid, i);	// move pid to free slot
					break;
				}
				else if (player[i].isbot)  // bot
				{
					bot_disconnect(i); // DESCONECTA BOT primeiro
					move_player(pid, i);	// move pid to free slot
					break;
				}
			}
		}
		//case 2: target team with 0 player less: check for trade, else do nothing
		//case 3: target team with 1 player less: check for trade, else go anyways
		else {
			// FIND A TRADE
			bool found = false;

			for (int i=0;i<maxplayers;i++)
			if (player[i].used)	// um player
			if (!player[i].isbot)		// player que NAO EH BOT
			if (i/TSIZE != pid/TSIZE)		// do outro time
			if (player[i].want_change_teams)		// que quer trocar de time
			{
				found = true;
				swap_players(pid, i);		// make trade
				break;
			}

			// IF TRADE NOT FOUND AND TEAMDELTA == 1, MOVE W/O TRADE
			if ((teamdelta == -1) && (!found)) {

				//comentando isso fora conserta o bug q eh o server entrar em loop

				for (int i=0;i<maxplayers;i++)
				if (i/TSIZE != pid/TSIZE)			// no time oposto
				{
					if (!player[i].used)		// player vago
					{
						move_player(pid, i);	// move pid to free slot
						break;
					}
					else if (player[i].isbot)  // bot
					{
						bot_disconnect(i); // DESCONECTA BOT primeiro
						move_player(pid, i);	// move pid to free slot
						break;
					}
				}
				/*
				for (int i=0;i<MAX_PLAYERS;i++)
				if (!player[i].used)		// player vago
				if (i/TSIZE != pid/TSIZE)			// no time oposto
				{
					move_player(pid, i);	// move pid to free slot
					break;
				}
				*/
			}
		}
	}

	// messages to update moved players (players/clients with new clients/players)
	//
	void move_update_player(int a) {

		if (player[a].used) {
			broadcast_player_name(a);

			if (!player[a].isbot) {

				send_me_packet(a);
				/*
				count = 0;
				writeByte(lebuf, count, 3); // "3" = first-packet information
				writeByte(lebuf, count, ((NLubyte)a) );		// who am I
				writeByte(lebuf, count, ((NLubyte)world.flag[0].score) );		//team 0 current score
				writeByte(lebuf, count, ((NLubyte)world.flag[1].score) );		//team 1 current score
				server->send_message(player[a].cid, lebuf, count);
				*/

				//v0.4.4 : tentativa de conserto
				//broadcast frags update
				char lebuf[256]; int count = 0;
				writeByte(lebuf, count, 4);	//"4" = frags update
				writeByte(lebuf, count, a);		// what player id
				writeLong(lebuf, count, player[a].frags);
				server->broadcast_message(lebuf, count);

				//v0.4.5 : atualiza registration char / score / rank
				broadcast_player_crap( a );

				//name (NEEDED? FIXME - ja tem la em cima!)
				//broadcast_player_name( a );
			}

			//message
			bprintf("@I%s moved to %s team", player[a].name, teamname[a/TSIZE]);
		}
	}

	//move player - move player (f rom) to empty position (t o)
	//
	void move_player(int f, int t) {

		//broadcast sound
		broadcast_sample(SAMPLE_CHANGETEAM);

		//UGLY HACK
		if (!check[t]) {
			check[t] = 1;
			checount--;
		}

		//char lixao[200];
		//sprintf(lixao, "move player pl=%i cl=%i %s to pl=%i", f, player[f].cid, player[f].name, t);
		//broadcast_message(lixao);

		//copy to t
		//player[t] = player[f];
//		memcpy(&(player[t]), &(player[f]), sizeof(player_t));
		player[t] = player[f];	//#NR

		//copy hero
		world.hero[t] = world.hero[f];

		//remove f
		game_remove_player(f);

		//update ctop
		if (!player[t].isbot)
			ctop[ player[t].cid ] = t;

		//I really dont want to change teams no more..
		player[t].want_change_teams = false;
		player[t].team_change_time = get_time() + 10.0;		//10 secs interval

		//kill t
		if (player[t].health > 0)	game_damage_player(t, t, 333333);

		//update t
		move_update_player(t);
	}

	//swap players - both are valid players
	//
	void swap_players(int a, int b) {

		//broadcast sound
		broadcast_sample(SAMPLE_CHANGETEAM);

		//mata quem nao tiver morto
		if (player[a].health > 0)	game_damage_player(a, a, 333333);
		if (player[b].health > 0)	game_damage_player(b, b, 333333);

		//chutando: troca players inteiros;
		swap(player[a], player[b]);

		//swap client id's
		if (!player[a].isbot)
			ctop[ player[a].cid ] = a;
		if (!player[b].isbot)
			ctop[ player[b].cid ] = b;

		//either don't want to change teams anymore
		player[a].want_change_teams = false;
		player[a].team_change_time = get_time() + 10.0;		//10 secs interval
		player[b].want_change_teams = false;
		player[b].team_change_time = get_time() + 10.0;		//10 secs interval

		//send updates
		move_update_player(a);
		move_update_player(b);
	}

	//broadcast a sample
	void broadcast_sample(int code) {

		char lebuf[64]; int count = 0;
		writeByte(lebuf, count, 14);		// sample play
		writeByte(lebuf, count, (NLubyte)code);		// the sample code
		server->broadcast_message(lebuf, count);
	}

	//play a sample to a player's screen audience
	void broadcast_screen_sample(int p, int code) {
		char lebuf[64]; int count = 0;
		writeByte(lebuf, count, 14);		// sample play
		writeByte(lebuf, count, (NLubyte)code);		// the sample code
		broadcast_screen_message(player[p].x, player[p].y, (char*)lebuf, count);
	}

	//send current flag status (cid == -1 : broadcast)
	void ctf_net_flag_status(int cid, int team) {

		//just resetting server state -- no update needed
		if (!server) return;

		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 6);	// 6 = flag status update

		NLubyte te = (NLubyte)team;
		writeByte(lebuf, count, te);	//what team

		if (world.flag[team].carried)	{ //carried?

			writeByte(lebuf, count, 1);	//TRUE

			//new flag carrier
			NLubyte thecarrier = ((NLubyte)world.flag[team].carrier);
			writeByte(lebuf, count, thecarrier);	//player who took it
		}
		else {
			NLubyte	p; NLshort sh;

			writeByte(lebuf, count, 0);	//FALSE

			//new flag position
			p = (NLubyte)world.flag[team].pos.px;		//px
			writeByte(lebuf, count, p);

			p = (NLubyte)world.flag[team].pos.py;		//py
			writeByte(lebuf, count, p);

			sh = (NLshort)world.flag[team].pos.x;		//x  FIXED v0.5.0
			writeShort(lebuf, count, sh);

			sh = (NLshort)world.flag[team].pos.y;		//y  FIXED v0.5.0
			writeShort(lebuf, count, sh);
		}

		if (cid == -1)
			server->broadcast_message(lebuf, count);
		else
			server->send_message(cid, lebuf, count);
	}

	//return flag to base position
	void ctf_return_flag(int team) {

		world.flag[team].carried = false;			// not carried anymore
		world.flag[team].pos = map.tinfo[team].flag;		// return to original position
		world.flag[team].atbase = true;		// yes, at base

		ctf_net_flag_status(-1, team);	// broadcast new status
	}

	//drop flag on ground
	void ctf_drop_flag(int team, int px, int py, int x, int y) {

		world.flag[team].carried = false;		// not carried
		world.flag[team].pos.px = px;		// dropped somewhere
		world.flag[team].pos.py = py;
		world.flag[team].pos.x = x;
		world.flag[team].pos.y = y;
		world.flag[team].atbase = false;		// not at base, team must touch to return (or it can be stolen)

		ctf_net_flag_status(-1, team);	// broadcast new status
	}

	//steal flag
	void ctf_steal_flag(int team, int carrier) {

		world.flag[team].carried = true;		// carried
		world.flag[team].carrier = carrier;	// who stole it
		world.flag[team].atbase = false;		// not at base (not needed / paranoia)

		ctf_net_flag_status(-1, team); // broadcast new status
	}

	//update team scores
	void ctf_update_teamscore(int t) {

		if (world.flag[t].score == capture_limit) {

			//change map!
			server_next_map(NEXTMAP_CAPTURE_LIMIT);

			//maximum score reached -- restart game (reposiciona jogadores no novo mapa)
			ctf_game_restart();

			return;
		}

		char lebuf[64]; int count = 0;
		writeByte(lebuf, count, 9);		// CTF teamscore update
		writeByte(lebuf, count, ((NLubyte)t));		// the team
		writeByte(lebuf, count, ((NLubyte)world.flag[t].score));	//the score
		server->broadcast_message(lebuf, count);
	}

	//respawn player. killed==true when player was killed, if joining game or it's
	//		a fresh game start, it's set to false
	//  if killed==true, use team spawn points and world spawn points
	//	if killed==false, use team spawn points only
	//
	void game_respawn_player(bool killed, int pid) {

		// not time to respawn anymore
		player[pid].respawn_time = -1;

		//the player's team
		int t = pid/TSIZE;

		//choose a team spawn point
		if (++map.tinfo[t].lastspawn >= map.tinfo[t].spawn.size())
			map.tinfo[t].lastspawn = 0;

		spoint_t pos;
		if (!killed) {
			int sp = map.tinfo[t].lastspawn;		//team spawn point #
			pos = map.tinfo[t].spawn[sp];	// the point
		}

		//if was killed or map spawn point places player over a wall
		if (killed || map.fall_on_wall(pos.px, pos.py, pos.x-20, pos.y-NR_SHIFTY-20, pos.x+20, pos.y-NR_SHIFTY+20)) {
			//if killed, generate a random spot for respawn:
			// - unnocupied screen
			// - away from walls

			//calculate room touch matrix
			vector<bool> roompop;
			roompop.resize(map.w*map.h, false);
			for (int i=0;i<maxplayers;i++)
				if (player[i].used && player[i].x >= 0 && player[i].y >= 0 && player[i].x < map.w && player[i].y < map.h)
					roompop[player[i].y * map.w + player[i].x] = true;

			int runaway = 400;
			do {
				//find screen
				int ridx;
				do {
					ridx = rand() % (map.w*map.h);
				} while ((runaway-- > 200) && (roompop[ridx] == true));	//keep trying until unnocupied (==false)
				pos.px = ridx%map.w;
				pos.py = ridx/map.w;

				//find a suitable coordinate -- middle square
				pos.x = plw / 8 + rand() % (3 * plw / 4);
				pos.y = plh / 8 + rand() % (3 * plh / 4) +NR_SHIFTY;

				//do a check for walls, maybe retrying another screen if hits a wall
				if (!map.fall_on_wall(pos.px, pos.py, pos.x-20, pos.y-NR_SHIFTY-20, pos.x+20, pos.y-NR_SHIFTY+20))
					break;	//success!

				//fall on wall true, keep trying...

			} while (runaway-- > 0);

			if (runaway <= 0)
				broadcast_message("PLAYER SPAWN RUNAWAY");
		}

		//put player there
		//LOG("SPAWN %i %i  %i %i\n", pos.px, pos.py, pos.x, pos.y);
		player[pid].x = pos.px;	//screen
		player[pid].y = pos.py;
		world.hero[pid].x = pos.x;	//screen position
		world.hero[pid].y = pos.y;

		//reset speeds / z
		world.hero[pid].sx = 0;
		world.hero[pid].sy = 0;

		//reset player attributes
		player[pid].health = 100;
		player[pid].energy = 100;
		player[pid].megabonus = 0;  //balaca megahealth

		player[pid].weapon = 0;		//default weapon

		//notify player weapon power change
		if (!player[pid].isbot) {
			char lebuf[256]; int count = 0;
			writeByte(lebuf, count, 18);		//player power change
			writeByte(lebuf, count, ((NLubyte)player[pid].weapon) );
			server->send_message(player[pid].cid, lebuf, count);
		}

		player[pid].item_shield = false;			// no items
		player[pid].item_quad = false;
		player[pid].item_speed = false;
		player[pid].item_helm = 0;
		player[pid].item_deathbringer = false;//
		player[pid].deathbringer_end = 0;		//not hit by deathbringer yet

		// clear pup-list (the client won't do it!?)	//#NR
		for (int iid=0; iid<MAX_PICKUPS; ++iid) {
			char lebuf[256]; int count=0;
			writeByte(lebuf, count, 16);	//	item removed
			writeByte(lebuf, count, iid);
			server->send_message(player[pid].cid, lebuf, count);
		}

		//for all effects, player screen changed
		game_player_screen_change(pid);

		//FIXME: add teleport visual effect and sound
	}

	//delete a rocket
	void game_delete_rocket(int r, NLshort hitx, NLshort hity, int targ) {

		rocket_c *rock = &(world.rock[r]);

		//assembly rocket delete message
		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 8);		// 8 = rocket deletion
		NLubyte byt = (NLubyte)r;
		writeByte(lebuf, count, byt);		// rocket-object id
		byt = (NLubyte)targ;
		writeByte(lebuf, count, byt);		// player-target. if 255, no player in particular was hit

		//byt = (NLubyte)framesleft;
		//writeByte(lebuf, count, byt);		// 10-msecs' left to the client to animate
		writeShort(lebuf, count, hitx);		// HIT X,Y OF ROCKET
		writeShort(lebuf, count, hity);

		//send message to players that received the rocket
		for (int p=0;p<maxplayers;p++)
		if (player[p].used)								//still valid player? (nao custa checar..)
		if (!player[p].isbot)							// not a bot !!!!
		if (rock->vislist & (1 << p))			//verifica se o bit de "conhece o rocket" ta ligado
		{
			// send the message to this player
			server->send_message(player[p].cid, lebuf, count);

			//LOG2("...sent to pl=%i rock=%i\n", p, byt);
		}

		//server-side invalidate
		rock->owner = -1;
	}

	//make damn rocket v0.4.7 remendo chute brabo pra tentar consertar 1 bug
	void make_damn_rocket(int i, int playernum, int px, int py, int x, int y, double deg, int xdelta) {
		//alloc
		rocket_c *rock = &(world.rock[i]);
		rock->owner = playernum;
		rock->team = playernum/TSIZE;
		rock->px = px;
		rock->py = py;
		rock->x = x;
		rock->y = y;
		rock->deg = deg;	//direcao em RADIANOS
		rock->hit_time = 0;
		//speed nos eixos: constante depende da direcao
		rock->sx = cos(rock->deg) * (ROCKET_SPEED);
		rock->sy = sin(rock->deg) * (ROCKET_SPEED);

		//deslocamento a 90graus
		rock->x += xdelta * cos(deg + PI/2);
		rock->y += xdelta * sin(deg + PI/2);

		//REMENDAO: avanca 0,5 frame  (5 vezes 1 decimo da velo (/2)
		rock->x += rock->sx * 5.0 / 10.0;
		rock->y += rock->sy * 5.0 / 10.0;
	}

	//shoot rocket to a certain direction
	//deg: em radianos
	//retorno: id do rocket alocado
	//XDELTA: deslocamento positivo para a direita ou negativo para a esquerda
	NLubyte game_do_shoot_rocket(int playernum, int px, int py, int x, int y, double deg, int xdelta) {

		for (NLubyte i=0;i<MAX_ROCKETS;i++)
			if (world.rock[i].owner == -1) { //unused
				make_damn_rocket(i,playernum,px,py,x,y,deg,xdelta);
				return i;
			}

		//whoops!
		LOG("WHOOPS!\n");
		int wtf = rand() % MAX_ROCKETS;
		make_damn_rocket(wtf,playernum,px,py,x,y,deg,xdelta);
		return (NLubyte)wtf;
	}

	//versao 0.1.2
	void game_shoot_rocket(int playernum, int shots, int px, int py, int x, int y, int gundir) {

		player[playernum].total_shots++;

		//ids alocados pra shots
		NLubyte		sid[16];

		// center degree
		double cdeg = gundir * PIOIT;

		//allocate a new rocket server-side for each shot
		// shots = qual arma (1-9 tiros!)
		switch (shots) {
		case 1:
			sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, 0);
			break;
		case 2:
			sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, - SHOT_DELTAX);
			sid[1] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, + SHOT_DELTAX);
			break;
		case 3:
			//V0.4.8 : NEW TRIPLE SHOT!
			sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, 0);
			sid[1] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, - SHOT_DELTAX * 2);
			sid[2] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, + SHOT_DELTAX * 2);
			//sid[1] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT, 0);
			//sid[2] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT, 0);
			break;
		case 4:
			sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, - SHOT_DELTAX);
			sid[1] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, + SHOT_DELTAX);
			sid[2] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT, 0);
			sid[3] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT, 0);
			break;
		case 5:
			sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, 0);
			sid[1] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, - SHOT_DELTAX * 2);
			sid[2] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, + SHOT_DELTAX * 2);
			sid[3] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT * 2, 0);
			sid[4] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT * 2, 0);
			break;
		case 6:
			sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, - SHOT_DELTAX);
			sid[1] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, + SHOT_DELTAX);
			sid[2] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT, 0);
			sid[3] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT, 0);
			sid[4] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT * 2, 0);
			sid[5] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT * 2, 0);
			break;
		case 7:
			sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, 0);
			sid[1] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, - SHOT_DELTAX * 2);
			sid[2] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, + SHOT_DELTAX * 2);
			sid[3] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT * 2, 0);
			sid[4] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT * 2, 0);
			sid[5] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT * 3, 0);
			sid[6] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT * 3, 0);
			break;
		case 8:
			sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, - SHOT_DELTAX);
			sid[1] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, + SHOT_DELTAX);
			sid[2] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT, 0);
			sid[3] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT, 0);
			sid[4] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT * 2, 0);
			sid[5] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT * 2, 0);
			sid[6] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT * 3, 0);
			sid[7] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT * 3, 0);
			break;
		case 9:
			sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, 0);
			sid[1] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, - SHOT_DELTAX * 2);
			sid[2] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, + SHOT_DELTAX * 2);
			sid[3] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT, 0);
			sid[4] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT, 0);
			sid[5] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT * 2, 0);
			sid[6] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT * 2, 0);
			sid[7] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT * 3, 0);
			sid[8] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT * 3, 0);
			break;
		}

#ifdef DEBUG_WEAPON
		char lix[2000];
		sprintf(lix, "pnum %i sids %i %i %i", playernum, sid[0], sid[1], sid[2]);
		broadcast_message(lix);
#endif

		//assembly multi-rocket message
		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 7);		// 7 = MULTI rocket fire
		NLubyte  powerdir;		//bits 0..4 = power bits 5..8=dir
		powerdir = (NLubyte)shots;			// shots

		//powerdir += (NLubyte)(gundir * 16);	//16,32,64,128
		// AWW FUCK IT
		int fuck = powerdir;
		fuck += gundir * 16;
		powerdir = (NLubyte)fuck;

		//writeByte(lebuf, count, powerdir);		// power and dir
		writeByte(lebuf, count, shots);		// power and dir
		writeByte(lebuf, count, gundir);		// power and dir
		for (int i=0;i<shots;i++) //MULTI ROCKETS!
			writeByte(lebuf, count, sid[i]);		// rocket-object id (needed because client-side rockets can be deleted by the server)
		writeLong(lebuf, count, this->frame);	// time of shot of the rocket: current (last simulated) frame
		writeByte(lebuf, count, (NLubyte)playernum);	// owner of all rockets
		writeByte(lebuf, count, (NLubyte)px);	//coord
		writeByte(lebuf, count, (NLubyte)py);
		writeShort(lebuf, count, (NLshort)x);
		writeShort(lebuf, count, (NLshort)y);

		//send to all people, build people-that-know DOUBLE WORD (32bits == 32players max)
		//send message to players on the same screen
		NLulong  vislist = 0;
		for (int p=0;p<maxplayers;p++)
		if (player[p].used)
		if (!player[p].isbot)	//not bot!!
		if (player[p].x == px)
		if (player[p].y == py) {
			vislist += (1 << p);	// mark as sent
			server->send_message(player[p].cid, lebuf, count);	// send the message to this player
		}

		//mark all created rockets with the vislist
		for (int k=0;k<shots;k++)
			world.rock[ sid[k] ].vislist = vislist;
	}

	//ctf player drops flag if carrying any
	bool ctf_drop_flag_if_any(int pid) {

		int enemyteam = 1 - (pid/TSIZE);

		//if is carrier of enemy flag, drop it, extra frag for fragging carrier
		if (world.flag[enemyteam].carried)		// attacker team's flag carried
		if (world.flag[enemyteam].carrier == pid) {	//...by the target

			//message
			bprintf("@I%s LOST THE %s FLAG!", player[pid].name, teamname[enemyteam]);

			//sound broadcast
			broadcast_sample(SAMPLE_CTF_LOST);

			//drop the flag
			ctf_drop_flag(enemyteam, player[pid].x, player[pid].y, (int)world.hero[pid].x, (int)world.hero[pid].y);

			player[pid].dropped_flag = true;
			player[pid].total_flags_dropped++;

			return true;
		}

		return false;
	}


	//refresh team ratings
	void refresh_team_score_modifiers() {

		double raw[2];
		raw[0]=0.0;
		raw[1]=0.0;

		//somatorio raw ratings
		for (int p=0;p<maxplayers;p++)
		if (player[p].used)
			if (!player[p].isbot) {

				// use "1.0" rating for anybody with less than 100 positive points
				if (client[player[p].cid].score < MINIMUM_POSITIVE_SCORE_FOR_RANKING)
					raw[p/TSIZE] += DEFAULT_PLAYER_RATE;		// default player rate
				else
					raw[p/TSIZE] += ( ((double)client[player[p].cid].score) + 1.0) / ( ((double)client[player[p].cid].neg_score) + 1.0);
			}
			else
				raw[p/TSIZE] += DEFAULT_BOT_PLAYER_RATE;		//soma 1.0 rating para cada bot

		//modifiers
		team_smul[0] = raw[1] / raw[0];
		team_smul[1] = raw[0] / raw[1];

		//ceil,floor (1/3 & 3/1)
		for (int i=0;i<2;i++) {
			if (team_smul[i] < 0.3333)
				team_smul[i] = 0.3333;
			if (team_smul[i] > 3.0)
				team_smul[i] = 3.0;
		}
	}


	//score!
	void score_frag(int p, int amount) {

		//add regular frags amount
		player[p].frags += amount;

		//v0.4.4 -- add score to the player's score accumulator
		//v0.4.7: DO NOT add score if map is not valid for scoring
		if (map.valid_for_scoring)
		if (player_count >= 2) { //v0.4.7.1 : skip the scoring if only one player present

			//refresh team ratings
			refresh_team_score_modifiers();

			int cid = player[p].cid;

			double parcela = ((double)amount) * team_smul[p/TSIZE];

			//add normalizado
			client[cid].fdp += parcela;

			//refresh "inteiro version"
			client[cid].delta_score = (int)(client[cid].fdp);

			//DEBUGz
			//char lix[256];
			//sprintf(lix, "%s scores +%.4f for %.4f +delta", player[p].name, parcela, client[cid].fdp);
			//broadcast_message(lix);

			//client[cid].delta_score += amount;		//just add the frags for now
		}
	}

	//score! NEG FRAG (v0.4.8)
	void score_neg(int p, int amount) {

		//add regular frags amount
		//player[p].frags += amount;

		//v0.4.4 -- add score to the player's score accumulator
		//v0.4.7: DO NOT add score if map is not valid for scoring
		if (map.valid_for_scoring)
		if (player_count >= 2) { //v0.4.7.1 : skip the scoring if only one player present

			//refresh team ratings
			refresh_team_score_modifiers();

			int cid = player[p].cid;

			double parcela = ((double)amount);		// NAO multiplica....

			//add normalizado
			client[cid].fdn += parcela;

			//refresh "inteiro version"
			client[cid].neg_delta_score = (int)(client[cid].fdn);

			//DEBUGz
			//char lix[256];
			//sprintf(lix, "%s scores -%.4f for %.4f -delta", player[p].name, parcela, client[cid].fdn);
			//broadcast_message(lix);

			//client[cid].neg_delta_score += amount;		//just add the frags for now V0.4.8: NEG SCORE!
		}
	}

	//enqueue a job to the master server to update a client's delta score
	void client_report_status(int id) {

		if (client[id].token_have)			// told token
		if (client[id].token_valid)			// validated token
		if ((client[id].delta_score != 0) || (client[id].neg_delta_score != 0)) { //if zero, NOP

			//submit-- create job
			masterjob_c *job = new masterjob_c();
			job->cid = id;
			job->code = 2;

			//V0.4.8: envia POS e NEG deltascore
			sprintf(job->request, "GET /servlet/fcecin.tk1/index.html?%s&dscp=%i&dscn=%i&name=%s&token=%s\n\n", TK1_VERSION_STRING, client[id].delta_score, client[id].neg_delta_score, player[ ctop [id] ].name, client[id].token);

			//LOG2("== MJOB for REPORT STATUS : %i '''%s'''\n", id, job->request);

			pthread_t mjob_thread;
			pthread_create (&mjob_thread, 0, thread_masterjob_f, (void *)job);

			//assume new score computed
			//client[id].score += client[id].delta_score;
			// NOT! new score will come...

			//reset the delta
			client[id].delta_score = 0;
			client[id].neg_delta_score = 0;
			client[id].fdp = 0.0;
			client[id].fdn = 0.0;
		}
	}

	//damage/kill player
	void game_damage_player(int target, int attacker, int damage) {

		bool no_death_penalty = false;

		//no score penalty
		if (damage == 666666)
			no_death_penalty = true;
		// take deathbringer out if move/swap players
		else if (damage == 333333) {
			player[target].item_deathbringer = false;
			no_death_penalty = true;	//no score penalty
		}
		else if (target != attacker) {	// no shots for suicides
			player[attacker].total_hits++;
			player[target].total_shots_taken++;
		}

		//HELM powerup: show player
		if (player[target].item_helm > 0)
			player[target].item_helm = 255;

		//damage?
		//check too much damage = must kill
		if (damage >= 10000)
			player[target].health -= damage;	//do damage
		else {

			//QUAD!!!!!
			// v0.2.2 : quad is just that: a quad damage
			if (player[attacker].item_quad)
			if (attacker != target)		// do not apply quaddamage into self
			{
				damage *= 2;
			}
			// if no attacker quad, shield absorbs at least 1 shot
			//else

			//v0.2.2: shield always absorbs
			if (player[target].item_shield) {

				player[target].energy -= damage;
				if (player[target].energy <= 0) {
					player[target].energy = 0;
					player[target].item_shield = false;
					if (target != attacker)
						broadcast_screen_sample(target, SAMPLE_SHIELD_LOST);
				}
				else {
					if (target != attacker)
						broadcast_screen_sample(target, SAMPLE_SHIELD_DAMAGE);
				}
			}
			//else do the regular body damage
			else {
				player[target].health -= damage;

				//freeze target's gun
				player[target].next_shoot_time = get_time() + 1.0;
			}
		}

		//died?
		if (player[target].health <= 0) {

			player[target].health = 0;

			//powerups: no more, if had any
			player[target].item_helm = 0;
			player[target].item_quad = false;
			player[target].item_speed = false;

			//stop all speed
			world.hero[target].sx = 0;
			world.hero[target].sy = 0;

			int tateam = target/TSIZE;
			int atteam = attacker/TSIZE;

			// check defends
			//
			if (attacker != target)		// self kills don't count
			{
				//check if the enemy flag is carried in this screen(target's) by somebody that is not me
				//
				if (world.flag[tateam].carried) {
					int p = world.flag[tateam].carrier;

					if (player[p].used)		//  carrier valid (paranoia)
					if (p != attacker)		// carrier not attacker
					if (player[p].x == player[target].x)		// carrier on dead target's screen
					if (player[p].y == player[target].y)
					{
						//defends the flag carrier!
						bprintf("@I%s DEFENDS THE %s CARRIER", player[attacker].name, teamname[atteam]);
						score_frag(attacker, 1);
					}
				}

				//check if my flag is atbase in target's screen
				//if (world.flag[atteam].atbase)
				//V0.4.6: doesn't matter if the flag is returned or not. killing an enemy near
				//  it is to defend the flag.
				//
				if (!world.flag[atteam].carried)		//not carried?
				if (world.flag[atteam].pos.px == player[target].x)	// flag in target's screen?
				if (world.flag[atteam].pos.py == player[target].y)
				{
					//defends the flag!
					bprintf("@I%s DEFENDS THE %s FLAG", player[attacker].name, teamname[atteam]);
					score_frag(attacker, 1);
				}
			}

			//drop flag if any
			if (ctf_drop_flag_if_any(target)) {
				//extra frag for fragging a carrier
				if (attacker != target) {
					score_frag(attacker, 1);
					player[attacker].total_flag_carriers_killed++;
				}

				//V0.4.8 SCORE NEG POINTS because of losing the flag
				if (!no_death_penalty)
					score_neg(target, 1);
			}

			//frag to attacker for the kill
			if (attacker != target)
				score_frag(attacker, 1);

			//V0.4.8 SCORE NEG POINTS because of death
			if (!no_death_penalty)
				score_neg(target, 1);

			//broadcast obituary
			if (attacker != target) {
				char lix[256];
				sprintf(lix, "@I%s was nailed by %s", player[target].name, player[attacker].name);
				broadcast_message(lix);

				//add to total kills/deaths
				player[attacker].total_kills++;
				player[target].total_deaths++;

				//update the ADMIN SHELL
				if (shellssock) {
					char lebuf[256]; int count; NLint result;
					if (!player[attacker].isbot) {
						count = 0;
						writeLong(lebuf, count, STA_PLAYER_KILLS);
						writeLong(lebuf, count, player[attacker].cid);
						result = nlWrite(shellssock, lebuf, count);
					}
					if (!player[target].isbot) {
						count = 0;
						writeLong(lebuf, count, STA_PLAYER_DIES);
						writeLong(lebuf, count, player[target].cid);
						result = nlWrite(shellssock, lebuf, count);
					}
				}
			}

			//respawn target in a few seconds
			player[target].respawn_time = get_time() + respawn_time;
			if (damage == 666666)
				player[target].respawn_to_base = true;	//special respawn-to-base damage
			else
				player[target].respawn_to_base = false;

			//!!DEATHBRINGER!!
			if (player[target].item_deathbringer) {

				//record time to simulate the deathbringer explosion
				player[target].item_deathbringer_time = frame;

				//delay respawn, so you can watch the PAIN! THE HORROR! HAHAHAHAHAHHAHA!
				player[target].respawn_time += waiting_time_deathbringer;

				//deathbringer message
				char lebuf[256]; int count = 0;
				writeByte(lebuf, count, 26);	//26==deathbringer
				writeByte(lebuf, count, ((NLubyte)target));	//team/target player
				writeLong(lebuf, count, frame);		//frame # of the bringer shot (message can be delayed)
				//LIE: x,y can be taken from the player since it's rock-dead on the bringer's position
				//v0.4.6: somehow the graphical effect is playing on the wrong screen of the clients
				//  trying sending screen x,y and player x y
				writeByte(lebuf, count, ((NLubyte)player[target].x));
				writeByte(lebuf, count, ((NLubyte)player[target].y));
				writeShort(lebuf, count, ((NLushort)world.hero[target].x));
				writeShort(lebuf, count, ((NLushort)world.hero[target].y));

				server->broadcast_message(lebuf, count);
			}
		}
	}

	//remove player from the game
	void game_remove_player(int pid) {

		//remove all shots from this player
		for (int r=0;r<MAX_ROCKETS;r++)
		if (world.rock[r].owner == pid)
			game_delete_rocket(r, 0, 0, 255);

		//if player carrying flag, drop it
		ctf_drop_flag_if_any(pid);

		//erase player
		player[pid].delayedMessages.clear();
		player[pid].used = false;

		//bot prefs changed?
		bot_prefs_change();
	}

	//restart ctf game
	void ctf_game_restart() {

		int i,cid;

		//submit all pending reports
		for (i=0;i<maxplayers;i++)
		if (player[i].used)
		if (!player[i].isbot)
		{
			cid = player[i].cid;
			client_report_status(cid);
		}

		//final score
		char lix[256];
		sprintf(lix, "@ICTF GAME RESTARTED - FINAL SCORE:   %i RED x %i BLUE !", world.flag[0].score, world.flag[1].score);
		broadcast_message(lix);

		//tell players the capture and time limit
		if (time_limit == 0)
			sprintf(lix, "@ICAPTURE %i FLAGS TO WIN THE GAME", capture_limit);
		else
			sprintf(lix, "@ICAPTURE %i FLAGS TO WIN THE GAME - TIME LIMIT IS %lu MINUTES", capture_limit, time_limit / 10 / 60);
		broadcast_message(lix);

		//sound
		broadcast_sample(SAMPLE_CTF_GAMEOVER);

		// zero teamscores
		world.flag[0].score = 0;
		world.flag[1].score = 0;
		ctf_update_teamscore(0);
		ctf_update_teamscore(1);

		// return all flags
		ctf_return_flag(0);
		ctf_return_flag(1);

		// reset map start time
		map_start_time = frame;

		// zero all player frags and kill them
		for (i=0;i<maxplayers;i++)
		if (player[i].used)
		{
			//kill - to respawn
			game_damage_player(i, i, 666666);	//666666==special damage - spawn on base
			//zero score
			player[i].frags = 0;
		}

		//zero all rockets
		for (i=0;i<MAX_ROCKETS;i++)
			world.rock[i].owner = -1;

		//#NR: remove and regenerate powerups
		for (i=0;i<MAX_PICKUPS;i++)
			world.item[i].kind = 0;
		check_pickup_creation(true);

		//update the ADMIN SHELL
		if (shellssock) {
			char lebuf[256]; int count = 0;
			writeLong(lebuf, count, STA_GAME_OVER);
			nlWrite(shellssock, lebuf, count);
		}
	}

	//respawn a powerup
	// put in a screen where there are NO players and NO other powerups
	void respawn_pickup(int p) {

		//char lixox[200];
		//broadcast_message("pickup respawned %s %s\n", itoa(p, lixox, 10), "hoo!");

		//nullify
		world.item[p].kind = 0;

		//find a screen with no players and no other powerups
		int px, py, itemx, itemy, i;
		for (int runaway=300;; --runaway) {
			bool hit = false;
			px = rand() % map.w;
			py = rand() % map.h;

			//check for players if not tried a 100 times yet

			//check players
			if (runaway>200)
				for (i=0; i<maxplayers; i++)
					if (player[i].used && player[i].x==px && player[i].y==py) {
						hit = true;
						break;
					}
			if (hit)
				continue;

			//check for items if not tried 200 times yet

			//check items if no players found
			if (runaway>100)
				for (i=0;i<MAX_PICKUPS;i++)
					if (world.item[i].kind!=0 && world.item[i].px==px && world.item[i].py==py) {
						hit = true;
						break;
					}
			if (hit)
				continue;

			//find a suitable coordinate -- middle square
			itemx = plw / 8 + rand() % (3 * plw / 4);
			itemy = plh / 8 + rand() % (3 * plh / 4);

			//do a check for walls, maybe retrying another screen if hits a wall
			hit = map.fall_on_wall(px, py, itemx - 20, itemy - 20, itemx + 20, itemy + 20);
			if (!hit)
				break;
			if (--runaway < 0) {
				broadcast_message("ITEM SPAWN RUNAWAY");
				return;
			}
		}
		//choose a powerup kind
		//v0.4.4 : roulette kind
		int kind = choose_powerup_kind(); //1 + (rand() % NUMBER_OF_POWERUP_KINDS);  //  % x   = x different items

		//v0.4.0 chance to set deathbringer to something else
		//if (kind == 7)
		//if (rand() % 100 <= 50)
		//kind = 1 + (rand() % NUMBER_OF_POWERUP_KINDS);  //  % x   = x different items

		//alloc powerup
		world.item[p].kind = (NLubyte)kind;

		//world.item[p].respawning = false;
		world.item[p].px = px;
		world.item[p].py = py;
		world.item[p].x = itemx;	//copy from randomized position
		world.item[p].y = itemy;
		//screen-change message of players in the screen the powerup arrived
		//fixes "invisible powerup" problem, I hope
		for (i=0;i<maxplayers;i++)
		if (player[i].used)		//valid
		if (!player[i].isbot)		//nonbot
		if (player[i].x == px)	//on the screen of the item
		if (player[i].y == py)
		{
			pickup_c *it = &(world.item[p]);
			char lebuf[256]; int count = 0;
			writeByte(lebuf, count, 15);		//"item update"
			writeByte(lebuf, count, (NLubyte)p);	//what item
			writeByte(lebuf, count, (NLubyte)it->kind);	//kind
			writeByte(lebuf, count, (NLubyte)it->px);		//screen
			writeByte(lebuf, count, (NLubyte)it->py);
			writeShort(lebuf, count, (NLushort)it->x);	//pos in screen
			writeShort(lebuf, count, (NLushort)it->y);
			server->send_message(player[i].cid, lebuf, count);
		}
	}

	int pups_by_percent(int percentage) const {
		int result = (map.w*map.h*percentage+50) / 100;	// +50 to round properly
		if (result==0 && percentage>0)
			return 1;
		if (result>MAX_PICKUPS)
			return MAX_PICKUPS;
		return result;
	}

	// verifica powerups unused por jogadores presentes
	void check_pickup_creation(bool instant) {
		int i, pc, ic;

		//count number of players
		pc = 0;
		for (i=0;i<maxplayers;i++)
			if (player[i].used)
				pc++;

		//count number of items
		// TEST SERVER FUK : change "if" to :    if (player[i].used)
		ic = 0;
		for (i=0;i<MAX_PICKUPS;i++)
			if (world.item[i].kind != 0)	//0=unused 255=respawning 1..6(?)=spawned/kind
				ic++;

		int real_min = pups_min_percentage?pups_by_percent(pups_min):pups_min;
		int real_max = pups_max_percentage?pups_by_percent(pups_max):pups_max;
		if (pc > real_min)
			real_min = pc;
		if (real_min > real_max)
			real_min = real_max;
		if (ic >= real_min)
			return;
		//while number of players > number of pickups: create a pickup and ic++
		for (i=0; i<MAX_PICKUPS; i++)
			if (world.item[i].kind == 0) {
				world.item[i].kind = 255;
				if (instant)
					respawn_pickup(i);
				else
					world.item[i].respawn_time = get_time() + pups_respawn_time;
				if (++ic>=real_min)
					break;
			}
	}

	// player i touches a pickup p!
	void game_touch_pickup(int p, int pk) {

		pickup_c *it = &world.item[pk];

		//send "item removed" message to all players on the current screen
		//
		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 16);		//"item removed"
		writeByte(lebuf, count, (NLubyte)pk);	//what item id
		//server->send_message(player[j].cid, lebuf, count);
		broadcast_screen_message(it->px, it->py, lebuf, count);


#ifdef REALLY_DEBUG_POWERUPS
		char lixx[2000];
		sprintf(lixx, "pickup %i snatched!\n", pk);
		broadcast_message(lixx);
#endif


		//player picked it! make fx
		//shield
		if (it->kind == 1) {

			player[p].item_shield = true;

			//increase health to minimum of 100
			if (player[p].health < 100)
				player[p].health = 100;		//full health

			//increase energy +100
			if (player[p].energy < 200) {
				player[p].energy += 100;
				if (player[p].energy > 200)
					player[p].energy = 200;
			}

			broadcast_screen_sample(p, SAMPLE_SHIELD_PICKUP);
		}
		//boots
		else if (it->kind == 2) {

			double itemTime=player[p].item_speed_time-get_time();
			if (!player[p].item_speed || itemTime<0)
				itemTime = 0;
			itemTime += pup_add_time;
			if (itemTime > pup_max_time)
				itemTime = pup_max_time;

			player[p].item_speed = true;
			player[p].item_speed_time = get_time() + itemTime;

			if (!player[p].isbot) {
				char lebuf[256]; int count = 0;
				writeByte(lebuf, count, 17);		//powerup time indicator
				writeByte(lebuf, count, it->kind);		//what kind
				writeShort(lebuf, count, (NLushort)itemTime);		//time
				server->send_message(player[p].cid, lebuf, count);
			}

			broadcast_screen_sample(p, SAMPLE_BOOTS_ON);
		}
		//helm
		else if (it->kind == 3) {

			double itemTime=player[p].item_helm_time-get_time();
			if (!player[p].item_helm || itemTime<0)
				itemTime = 0;
			itemTime += pup_add_time;
			if (itemTime > pup_max_time)
				itemTime = pup_max_time;

			player[p].item_helm = 1;		//invis maximo de inicio
			player[p].item_helm_time = get_time() + itemTime;

			if (!player[p].isbot) {
				char lebuf[256]; int count = 0;
				writeByte(lebuf, count, 17);		//powerup time indicator
				writeByte(lebuf, count, it->kind);		//what kind
				writeShort(lebuf, count, (NLushort)itemTime);		//time
				server->send_message(player[p].cid, lebuf, count);
			}

			broadcast_screen_sample(p, SAMPLE_HELM_ON);
		}
		//quad
		else if (it->kind == 4) {

			double itemTime=player[p].item_quad_time-get_time();
			if (!player[p].item_quad || itemTime<0)
				itemTime = 0;
			itemTime += pup_add_time;
			if (itemTime > pup_max_time)
				itemTime = pup_max_time;

			player[p].item_quad = true;
			player[p].item_quad_time = get_time() + itemTime;

			if (!player[p].isbot) {
				char lebuf[256]; int count = 0;
				writeByte(lebuf, count, 17);		//powerup time indicator
				writeByte(lebuf, count, it->kind);		//what kind
				writeShort(lebuf, count, (NLushort)itemTime);		//time
				server->send_message(player[p].cid, lebuf, count);
			}

			broadcast_screen_sample(p, SAMPLE_QUAD_ON);
		}
		//weapon
		else if (it->kind == 5) {

			if (player[p].weapon < 8)	// test for max (shots=weapon+1, entao p/ shots max 9, weapon max = 8)
				player[p].weapon++;	//increase weapon power

			//increase energy +100
			if (player[p].energy < 200) {
				player[p].energy += 100;
				if (player[p].energy > 200)
					player[p].energy = 200;
			}

			//notify player weapon power change
			if (!player[p].isbot) {
				char lebuf[256]; int count = 0;
				writeByte(lebuf, count, 18);		//player power change
				writeByte(lebuf, count, ((NLubyte)player[p].weapon) );
				server->send_message(player[p].cid, lebuf, count);
			}

			broadcast_screen_sample(p, SAMPLE_WEAPON_UP);
		}
		//megahealth
		else if (it->kind == 6) {

			//megabonus!
			player[p].megabonus += 160;

			//increase energy +100, upto 300
			//player[p].energy += 100;
			//if (player[p].energy > 300)
			//	player[p].energy = 300;

			//increase health +100, upto 300
			//player[p].health += 100;
			//if (player[p].health > 300)
			//	player[p].health = 300;

			broadcast_screen_sample(p, SAMPLE_MEGAHEALTH);
		}
		//deathbringer
		else if (it->kind == 7) {
			if (pup_deathbringer_switch)
				player[p].item_deathbringer = !player[p].item_deathbringer;
			else
				player[p].item_deathbringer = true;

			broadcast_screen_sample(p, SAMPLE_GETDEATHBRINGER);
		}

		// unused item
		it->kind = 0;

		// check pickup creation
		check_pickup_creation(false);
	}

	//game player screen changed
	// --> send any pickups on screen
	void game_player_screen_change(int p) {

		//check for new pickups visible
		for (int i=0;i<MAX_PICKUPS;i++) {
			pickup_c *it = &world.item[i];
			if (it->kind)		// item exists
			if (it->kind != 255)		// item not respawning
			if (it->px == player[p].x) // item on screen that player is entering
			if (it->py == player[p].y) {

				#ifndef NR_NO_PUP_SWITCHING
				//broadcast_message("sending powerup update\n");

				//v0.1.2: PRIMEIRO verifica se tem mais alguem nessa tela. se nao
				//  tiver, verifica se nao seria interessante mudar o "kind" do item
				//muda WHILE item alvo eh powerup cujo time do jogador eh > 30
				bool temjog = false;
				for (int j=0;j<maxplayers;j++)
				if (j != p)
				if (player[j].used)	//valido
				if (player[j].x == player[p].x)	// mesma tela
				if (player[j].y == player[p].y) {
					temjog = true;
					break;
				}

				int original = it->kind;

				if (!temjog) {
					bool non_satisfactory;
					do {
						non_satisfactory = false;

						if ((it->kind == 1) && (player[p].health >= 80) && (player[p].energy >= 30) && (player[p].item_shield))//hide if just using as extra battery or not seriously injured
							non_satisfactory = true;
						else if ((it->kind == 2) && (player[p].item_speed) && (player[p].item_speed_time - get_time() > 40.0))
							non_satisfactory = true;
						else if ((it->kind == 3) && (player[p].item_helm) && (player[p].item_helm_time - get_time() > 40.0))
							non_satisfactory = true;
						else if ((it->kind == 4) && (player[p].item_quad) && (player[p].item_quad_time - get_time() > 40.0))
							non_satisfactory = true;
						else if ((it->kind == 6) && (player[p].health + (rand() % 70) >= 300))//if 300 non-satisf. but if >200, less chance of seeing another one
							non_satisfactory = true;
						else if ((it->kind == 7) && (player[p].item_deathbringer))
							non_satisfactory = true;

						//re-choose item type
						if (non_satisfactory)
							it->kind = (NLubyte)choose_powerup_kind();

					} while (non_satisfactory);

					//if loop choosed "weapon" powerup (item 5) but you are at maximum, then keep the original choice
					if ((it->kind == 5) && (player[p].weapon >= 8))
						it->kind = (NLubyte)original;
				}
				#endif	// NR_NO_PUP_SWITCHING

				//send a "item on the screen" message
				if (!player[p].isbot) {
					char lebuf[256]; int count = 0;
					writeByte(lebuf, count, 15);		//"item update"
					writeByte(lebuf, count, (NLubyte)i);	//what item
					writeByte(lebuf, count, (NLubyte)it->kind);	//kind
					writeByte(lebuf, count, (NLubyte)it->px);		//screen
					writeByte(lebuf, count, (NLubyte)it->py);
					writeShort(lebuf, count, (NLushort)it->x);	//pos in screen
					writeShort(lebuf, count, (NLushort)it->y);
					server->send_message(player[p].cid, lebuf, count);
				}
			}
		}
	}


	//broadcast team message
	void broadcast_team_message(int team, char *text) {

		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 2);
		writeString(lebuf, count, text);

		// send message only to teammates
		for (int i=0;i<maxplayers;i++)
		if (player[i].used)
		if (!player[i].isbot)	// nao para bots!!
		if ((i/TSIZE) == team) {
			server->send_message(player[i].cid, lebuf, count);
		}

		//send to the admin shell
		if (shellssock) {
			count = 0;
			writeLong(lebuf, count, STA_GAME_TEXT);
			writeString(lebuf, count, text);
			nlWrite(shellssock, lebuf, count);
		}
	}

	//broadcast message to all players in one screen
	void broadcast_screen_message(int px, int py, char *lebuf, int count) {

		for (int j=0;j<maxplayers;j++)
		if (player[j].used)
		if (!player[j].isbot)		// nao para bots!!
		if (player[j].x == px)
		if (player[j].y == py)
			server->send_message(player[j].cid, lebuf, count); //send the message
	}

	/*
	//broadcast message w/ 2 string parms (lixao)
	void broadcast_message(char *text, char *t1, char *t2) {
		char lix[256];
		sprintf(lix, text, t1, t2);
		broadcast_message(lix);
	}
	//broadcast message w/ 1 string parms (lixao)
	void broadcast_message(char *text, char *t1) {
		char lix[256];
		sprintf(lix, text, t1);
		broadcast_message(lix);
	}
	*/

	// V0.4.9 : broadcast message with varargs
	void bprintf(const char *fs, ...) {
		//vsprintf...
		va_list argptr;
		char msg[16384];
		va_start(argptr, fs);
		vsprintf(msg, fs, argptr);
		va_end (argptr);

		//broadcast it
		broadcast_message(msg);
	}
	void plprintf(int pid, const char* fmt, ...) {	//#NR: bprintf for a single player
		char buf[16385];
		buf[0]=2;	// server text
		va_list argptr;
		va_start(argptr, fmt);
		vsprintf(buf+1, fmt, argptr);
		va_end(argptr);
		server->send_message(player[pid].cid, buf, strlen(buf)+1);
	}

	//send a single message player-printf
	void player_message(int pid, const char *text) {
		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 2);
		writeString(lebuf, count, text);
		if (player[pid].used)
		if (!player[pid].isbot)	// nao para bots!
			server->send_message(player[pid].cid, lebuf, count);
	}

	//broadcast message to all
	void broadcast_message(const char *text) {
		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 2);
		writeString(lebuf, count, text);
		for (int i=0;i<maxplayers;i++)
		if (player[i].used)
		if (!player[i].isbot)	// nao para bots!
			server->send_message(player[i].cid, lebuf, count);

		//send to the admin shell
		if (shellssock) {
			count = 0;
			writeLong(lebuf, count, STA_GAME_TEXT);
			writeString(lebuf, count, text);
			nlWrite(shellssock, lebuf, count);
		}
	}

	// ---- GAME MOD -------

	void load_game_mod() {

		char dest[WHERE_PATH_SIZE];
		append_filename(dest, wheregamedir, "gamemod.txt", WHERE_PATH_SIZE);
		FILE *fmod = fopen(dest, "r");
		if (fmod) {

			char s[1024];

			bool command = true;
			int cmd = 0;

			LOG1("loading game mod from '%s'...\n", dest);

			while (1) {
				//char* fgets(char* s, int n, FILE* stream);
				//Copies characters from (input) stream stream to s,
				// stopping when n-1 characters copied, newline copied, end-of-file reached or error occurs.
				//If no error, s is NUL-terminated. Returns NULL on end-of-file or error, s otherwise.

				if (fgets((char*)s, 1024, fmod) == 0)
					break;

				s[ strlen(s) - 1] = 0;	//erase \n

				//LOG1("modline '%s'\n", s);

				if (s[0] == '\0')	//skip blank
					continue;

				if (s[0] == ';') // skip comment
					continue;

				if (command) {
					if (!strcmp(s, "friction")) cmd = 1;
					else if (!strcmp(s, "accel")) cmd = 2;
					else if (!strcmp(s, "maxspeed")) cmd = 3;
					else if (!strcmp(s, "friction_run")) cmd = 4;
					else if (!strcmp(s, "accel_run")) cmd = 5;
					else if (!strcmp(s, "maxspeed_run")) cmd = 6;
					else if (!strcmp(s, "friction_turbo")) cmd = 7;
					else if (!strcmp(s, "accel_turbo")) cmd = 8;
					else if (!strcmp(s, "maxspeed_turbo")) cmd = 9;
					else if (!strcmp(s, "friction_turborun")) cmd = 10;
					else if (!strcmp(s, "accel_turborun")) cmd = 11;
					else if (!strcmp(s, "maxspeed_turborun")) cmd = 12;
					else if (!strcmp(s, "flag_penalty")) cmd = 13;
					else if (!strcmp(s, "map")) cmd = 14;
					else if (!strcmp(s, "pups_min")) cmd = 15;
					else if (!strcmp(s, "pups_max")) cmd = 28;
					else if (!strcmp(s, "pups_respawn_time")) cmd = 16;
					else if (!strcmp(s, "pup_chance_shield")) cmd = 17;
					else if (!strcmp(s, "pup_chance_turbo")) cmd = 18;
					else if (!strcmp(s, "pup_chance_shadow")) cmd = 19;
					else if (!strcmp(s, "pup_chance_power")) cmd = 20;
					else if (!strcmp(s, "pup_chance_weapon")) cmd = 21;
					else if (!strcmp(s, "pup_chance_megahealth")) cmd = 22;
					else if (!strcmp(s, "pup_chance_deathbringer")) cmd = 23;
					else if (!strcmp(s, "time_limit")) cmd = 24;
					else if (!strcmp(s, "capture_limit")) cmd = 25;
					else if (!strcmp(s, "welcome_message")) cmd = 26;
					else if (!strcmp(s, "info_message")) cmd = 27;
					// 28 = pups_max
					else if (!strcmp(s, "pup_add_time")) cmd = 29;
					else if (!strcmp(s, "pup_max_time")) cmd = 30;
					else if (!strcmp(s, "pup_deathbringer_switch")) cmd = 31;
					else if (!strcmp(s, "random_maprot")) cmd = 32;
					else if (!strcmp(s, "vote_block_time")) cmd = 33;
					else if (!strcmp(s, "respawn_time")) cmd = 34;
					else if (!strcmp(s, "waiting_time_deathbringer")) cmd = 35;
					else if (!strcmp(s, "pup_shadow_invisibility")) cmd = 36;
					else {
						LOG1("*** Bad command in gamemod: %s\n", s);
						cmd = 0;
					}

					//LOG1("is command %i\n", cmd);
				}
				else {
					double val = 1.0;
					int ival = 1;

					sscanf(s, "%lf", &val);
					sscanf(s, "%i", &ival);

					//LOG3("set cmd %i value to %f from '%s'\n", cmd, val, s);
					//LOG3("set cmd %i value to %i from '%s'\n", cmd, ival, s);

					if (cmd == 1) {
						svp_fric = val;
					}
					else if (cmd == 2) {
						svp_accel = val;
					}
					else if (cmd == 3) {
						svp_maxspeed = val;
					}
					else if (cmd == 4) {
						svp_fric_run = val;
					}
					else if (cmd == 5) {
						svp_accel_run = val;
					}
					else if (cmd == 6) {
						svp_maxspeed_run = val;
					}
					else if (cmd == 7) {
						svp_fric_turbo = val;
					}
					else if (cmd == 8) {
						svp_accel_turbo = val;
					}
					else if (cmd == 9) {
						svp_maxspeed_turbo = val;
					}
					else if (cmd == 10) {
						svp_fric_turborun = val;
					}
					else if (cmd == 11) {
						svp_accel_turborun = val;
					}
					else if (cmd == 12) {
						svp_maxspeed_turborun = val;
					}
					else if (cmd == 13) {
						svp_flag_penalty = val;
					}
					else if (cmd == 14) {
						//	int		maprots;		//quantos mapas em maprots
						//	int		currmap;		//mapa atual do maprot
						//	char	maprot[MAPROTSIZE][MAPFILENAMESIZE];
						strcpy(maprot[maprots], s);
						maprots++;
						//LOG("++++++ MAPROTS++ +++++++\n");
					}
					else if (cmd == 15) {
						if (strchr(s, '%')) {
							if (ival >= 0) {
								pups_min = ival;
								pups_min_percentage = true;
							}
							else LOG1("Can't set pups_min to %d%%\n", ival);
						}
						else if (ival >= 0 && ival <= MAX_PICKUPS) {
							pups_min = ival;
							pups_min_percentage = false;
						}
						else LOG1("Can't set pups_min to %d\n", ival);
					}
					else if (cmd == 28) {
						if (strchr(s, '%')) {
							if (ival >= 0) {
								pups_max = ival;
								pups_max_percentage = true;
							}
							else LOG1("Can't set pups_max to %d%%\n", ival);
						}
						else if (ival>=0 && ival<=MAX_PICKUPS) {
							pups_max = ival;
							pups_max_percentage = false;
						}
						else LOG1("Can't set pups_max to %d\n", ival);
					}
					else if (cmd == 16) {
						if (ival >= 0 && ival <= 60)
							pups_respawn_time = ival;
						else LOG1("Can't set pups_respawn_time to %d\n", ival);
					}
					else if (cmd == 17) {
						pup_chance_shield = ival;
					}
					else if (cmd == 18) {
						pup_chance_turbo = ival;
					}
					else if (cmd == 19) {
						pup_chance_shadow = ival;
					}
					else if (cmd == 20) {
						pup_chance_power = ival;
					}
					else if (cmd == 21) {
						pup_chance_weapon = ival;
					}
					else if (cmd == 22) {
						pup_chance_megahealth = ival;
					}
					else if (cmd == 23) {
						pup_chance_deathbringer = ival;
					}
					else if (cmd == 24) {
						if (ival >= 0)
							time_limit = 60 * 10 * ival; // convert minutes to frames
					}
					else if (cmd == 25) {
						if (ival > 0)
							capture_limit = ival;
					}
					else if (cmd == 26) {
						welcome_message.push_back(s);
					}
					else if (cmd == 27) {
						info_message.push_back(s);
					}
					else if (cmd == 29) {
						if (ival > 0 && ival<1000)
							pup_add_time = ival;
						else LOG1("Can't set pup_add_time to %d\n", ival);
					}
					else if (cmd == 30) {
						if (ival > 0 && ival<1000)
							pup_max_time = ival;
						else LOG1("Can't set pup_max_time to %d\n", ival);
					}
					else if (cmd == 31) {
						if (ival == 0 || ival == 1)
							pup_deathbringer_switch = ival==1?true:false;
						else LOG1("Can't set pup_deathbringer_switch to %d\n", ival);
					}
					else if (cmd == 32) {
						if (ival == 0 || ival == 1)
							random_maprot = ival==1?true:false;
						else LOG1("Can't set random_maprot to %d\n", ival);
					}
					else if (cmd == 33) {
						if (ival >= 0)
							vote_block_time = 60*10*ival;	// minutes to frames
					}
					else if (cmd == 34) {
						if (val >= 0)
							respawn_time = val;
					}
					else if (cmd == 35) {
						if (val >= 0)
							waiting_time_deathbringer = val;
					}
					else if (cmd == 36) {
						if (ival == 0 || ival == 1)
							shadow_minimum = ival==1?1:SHADOW_MINIMUM_NORMAL;
						else LOG1("Can't set pup_shadow_invisibility to %d\n", ival);
					}
				}

				//parameter
				command = !command;
			}

			LOG("END OF MOD FILE.\n");

			fclose(fmod);
		}
	}

	//load a map from the rotation list
	void load_rotation_map(int pos) {
		bool ok = load_map(SERVER_MAPS_DIR, maprot[pos], &map);
		assert(ok);
		LOG2("load_rotation_map() maprot[%i] = '%s'\n", pos, maprot[pos]);
	}

	//send map change message to a player
	//reason: NEXTMAP_CAPTURE_LIMIT ou NEXTMAP_VOTE_EXIT
	void send_map_change_message(int pid, int reason, const char* mapname) {

		char lebuf[256];
		int count = 0;
		writeByte(lebuf, count, 20);	// 20 = map change

		if (builtin) {
			writeByte(lebuf, count, 1);		// 1 = built-in map message
			writeByte(lebuf, count, 1);		//1= the first (and probably only) built-in map
		}
		else {
			writeByte(lebuf, count, 2);		// 2 = custom map message
			writeShort(lebuf, count, map.crc);
			//LOG2("SERVER: send mapchange to %i mapfile = '%s'\n", pid, map.filename);
			writeString(lebuf, count, mapname);
		}
		server->send_message(player[pid].cid, lebuf, count);

		//VERY IMPORTANT: flags the player as "awaiting map load" - client must confirm map to proceed
		player[pid].awaiting_client_ready = true;

		//send a show gameover plaque message, if that is the case
		if (reason != NEXTMAP_NONE) {

			count = 0;
			writeByte(lebuf, count, 24);	// 24 = gameover plaque
			writeByte(lebuf, count, ((NLubyte)reason));		//capture limit plaque or vote exit plaque
			if ((reason == NEXTMAP_CAPTURE_LIMIT) || (reason == NEXTMAP_VOTE_EXIT)) {
				//informacoes para mostrar apos o jogo (time vencedor, most valuable player, etc.)

				writeByte(lebuf, count, (NLubyte)world.flag[0].score);	//RED team final score
				writeByte(lebuf, count, (NLubyte)world.flag[1].score);	//BLUE team final score
			}
			server->send_message(player[pid].cid, lebuf, count);

			//important: server is showing gameover plaque. nobody should move or receive world frames
			gameover = true;
			gameover_time = get_time() + 5.0;		//5 secods timeout for gameover plaque
		}
	}

	//go to server_next_map();
	void server_next_map(int reason) {

		//(re)load hostname
		reload_hostname();

		char lix[256];lix[0]=0;

		if (maprots == 0) {
			//no map rotation - change to next built-in
			load_default_map(&map);		// default map
			builtin = true;
			sprintf(lix, "Server changed map to: DEFAULT (0)");
		}
		else {
			#ifdef NR_CONSOLE
			vector<int> winners;
			int maxVotes=0;
			for (int m=0; m<maprots; ++m) {
				if (mapinfo[m].votes<maxVotes)
					continue;
				if (mapinfo[m].votes>maxVotes) {
					maxVotes=mapinfo[m].votes;
					winners.clear();
				}
				winners.push_back(m);
			}
			if (maxVotes==0)
				currmap=(currmap+1)%maprots;
			else {
				if (winners.size()>1)
					winners.erase(find(winners.begin(), winners.end(), currmap));
				currmap=winners[rand()%winners.size()];
			}
			// clear votes for the current map
			for (int p=0; p<maxplayers; ++p) {
				player[p].want_map_exit=false;
				if (player[p].mapVote==currmap)
					player[p].mapVote=-1;
			}
			mapinfo[currmap].votes=0;
			#else
			//next map on rotation
			currmap++;
			if (currmap >= maprots) {
				currmap = 0;
			}
			for (int p=0; p<maxplayers; ++p)
				player[p].want_map_exit=false;
			#endif

			// attempts to load map from current position of rotation list
			load_rotation_map(currmap);
			sprintf(lix, "Server changed map to: %s (%i of %i)", maprot[currmap], currmap+1, maprots);
//			sprintf(lix, "Server changed map to: DEFAULT (%i of %i)", currmap+1, maprots);
		}

		next_vote_announce_frame = 0;	// let new announce be made as soon as someone votes
		last_vote_announce_votes = last_vote_announce_needed = 0;

		// notify all players
		for (int i=0;i<maxplayers;i++)
			if (player[i].used && !player[i].isbot)
				send_map_change_message(i, reason, maprot[currmap]);

		broadcast_message(lix);

		// reset map start time
		map_start_time = frame;
	}

	//check map exit by vote
	void check_map_exit() {
		int num_for = 0, num_against = 0;
		for (int i=0; i<maxplayers; i++)
			if (player[i].used && !player[i].isbot) {
				if (player[i].want_map_exit)
					num_for++;
				else
					num_against++;
			}

		#ifdef NR_CONSOLE
		// this could be done elsewhere, but this function is called whenever votes change
		for (int m=0; m<maprots; ++m)
			mapinfo[m].votes=0;
		for (int p=0; p<maxplayers; ++p)
			if (player[p].used && !player[p].isbot && player[p].mapVote!=-1)
				++mapinfo[player[p].mapVote].votes;
		#endif

		if ((map_start_time+vote_block_time<frame && num_for>num_against) || num_against==0) {
			server_next_map(NEXTMAP_VOTE_EXIT);
			ctf_game_restart();
		}
	}

	//----- THE REST  ----------------

	//start server
	bool start(int target_maxplayers) {

		#ifdef NR_NAME_AUTHORIZATION
		authorizations.load();
		#endif

		int i;

		//check if maxplayers is valid
		if (target_maxplayers < 2)				//menos de dois
			return false;
		if (target_maxplayers > MAX_PLAYERS)		//mais que o maximo
			return false;
		if (target_maxplayers % 2 == 1)	//numero impar de jogadores
			return false;

		//set maxplayers
		maxplayers = target_maxplayers;

		//reset client_c struct (closes files...)
		for (i=0;i<MAX_PLAYERS;i++)
			client[i].reset();

		//not showing gameover plaque to clients
		gameover = false;

		ping_send_counter = 0;
		ping_send_client = 0;

		//reset fslavesocks
		for (int ss=0;ss<MAX_PLAYERS;ss++)
			fslavesock[ss] = NL_INVALID;			//inicializa
		file_threads_quit = false;	//not yet

		// server game phisics parameters
		// DEFAULT VALUES...
		//
		set_default_physics();

		// reset players
		//players_present = 0;
		player_count = 0;
		for (i=0;i<MAX_PLAYERS;i++) {
			player[i].used = false;
			player[i].id = i;
			player[i].name[0]=0;
		}

		// reset bots
		bot_count = 0;
		bot_prefs_change();

		//default configuration
		//DEFAULT POWERUP CONFIG
		pup_add_time = 60;
		pup_max_time = 180;

		pups_min = 6;
		pups_min_percentage = false;
		pups_max = MAX_PICKUPS;
		pups_max_percentage = false;
		pups_respawn_time = 25;

		pup_chance_shield = 16;
		pup_chance_turbo = 14;
		pup_chance_shadow = 14;
		pup_chance_power = 14;
		pup_chance_weapon = 18;
		pup_chance_megahealth = 13;
		pup_chance_deathbringer = 11;

		pup_deathbringer_switch = true;
		shadow_minimum = SHADOW_MINIMUM_NORMAL;

		respawn_time = 2.0;
		waiting_time_deathbringer = 4.0;

		// default time and capture limits
		time_limit = 0;	// no time limit
		capture_limit = 8;

		vote_block_time = 0;	// no limit

		random_maprot = false;
		// reset server rotation list
		maprots = 0;
		currmap = 0;
		builtin = false;

		// load server configuration from gamemod.txt
		load_game_mod();

		// did not specify maps, scan "maps/" folder for .txt map files
		if (maprots == 0) {

			char mappath[256];
			strcpy(mappath, SERVER_MAPS_DIR);  // maps
			put_backslash(mappath);					// maps/
			strcat(mappath, "*.txt");				// maps/*.txt
			char dest[1024];
			append_filename(dest, wheregamedir, mappath, WHERE_PATH_SIZE);	// <FULL-DIR>/maps/*.txt, I hope

			LOG1("MAP SCAN DIR IS = '%s'\n", dest);

			al_ffblk mapffblk;	//for al_find*

			int result = al_findfirst(dest, &mapffblk, FA_ARCH);
			while (result == 0) {

				//work with result
				//Replaces the specified filename+extension with a new extension tail, storing at most size bytes
				//into the dest buffer. Returns a copy of the dest parameter.
				//char *replace_extension(char *dest, const char *filename, const char *ext, int size
				//strcpy(maprot[maprots], mapffblk.name);

				replace_extension(maprot[maprots], mapffblk.name, "", MAPFILENAMESIZE-1);
				maprot[maprots][strlen(maprot[maprots])-1] = 0;	//take last damn '.' out

				LOG2("COPYING TO MAPROT %i = '%s'\n", maprots, maprot[maprots]);

				maprots++;
				if (maprots == MAPROTSIZE)	//can't store any more maps in the rotation....
					break;

				//try next
				result = al_findnext(&mapffblk);
			}
		}

		if (random_maprot)
			for (int p=0; p<maprots; ++p) {
				char buf[MAPFILENAMESIZE];
				int i=p+rand()%(maprots-p);
				strcpy(buf, maprot[p]);
				strcpy(maprot[p], maprot[i]);
				strcpy(maprot[i], buf);
			}
		#ifdef NR_CONSOLE
		for (int p=0; p<maprots; ++p) {
			load_rotation_map(p);
			mapinfo[p].title=map.title;
			mapinfo[p].file=maprot[p];
			mapinfo[p].width=map.w;
			mapinfo[p].height=map.h;
		}
		#endif

		if (maprots == 0) {
			LOG("No maps for rotation\n");
			return false;
		}

		load_rotation_map(currmap);

		// start server
		server = new_server_c();

		server->set_callback(SFUNC_CLIENT_HELLO, sfunc_client_hello);
		server->set_callback(SFUNC_CLIENT_CONNECTED, sfunc_client_connected);
		server->set_callback(SFUNC_CLIENT_DISCONNECTED, sfunc_client_disconnected);
		server->set_callback(SFUNC_CLIENT_LAG_STATUS, sfunc_client_lag_status);
		server->set_callback(SFUNC_CLIENT_DATA, sfunc_client_data);
		server->set_callback(SFUNC_CLIENT_PING_RESULT, sfunc_client_ping_result);
		server->set_client_timeout(5, 10);

		if (!server->start(port))
			return false;

		//v0.4.4 reset master jobs count
		mjob_count = 0;
		mjob_exit = false;				//flag for all pending master jobs to quit now
		mjob_fastretry = false;		//flag for all pending master jobs to stop waiting and retry immediately

		//v0.4.2 : calc TCP PORT

		NLboolean ok;

		//v0.4.4 : DO NOT open tcp download port if (-notcp) enabled
		//
		if (!no_tcp_download) {

			int tcp_port = 24999 - (port - 25000);

			//open socket for accepting connections to TCP file download requests
			//nlEnable(NL_BLOCKING_IO);
			filesock = nlOpen((NLushort)tcp_port, NL_RELIABLE);		//v0.4.2 : custom TCP port
			//nlDisable(NL_BLOCKING_IO);
			if (filesock == NL_INVALID) {
				LOG1("CAN'T OPEN THE TCP SOCKET ON PORT %i\n", tcp_port);
				return false; //oh no
			}
			ok = nlListen(filesock);
			if (!ok) {
				LOG("CAN'T SET SOCKET TO LISTEN MODE\n");
				return false; //oh no
			}

			//start TCP file server master thread
			pthread_create(&server_filemaster_thread, 0, thread_filemaster_f, (void *)0);
		}

		//start TCP thread for talking with master server
		if (!privateserver) {
			master_talk_time = get_time();
			msock = NL_INVALID;		//not opened
			master_talk_time = get_time() + DELAY_TO_REPORT_SERVER;	//give it a break
			master_never_talked = true;		//not talked yet
			pthread_create(&mthread, 0, thread_mastertalker_f, (void *)0);
		}

		//shell socket
		//v0.4.2 : new port
		int tcp_shell_port = 24500 + (port - 25000);

		shellmsock = nlOpen((NLushort)tcp_shell_port, NL_RELIABLE);
		if (shellmsock == NL_INVALID) {
			LOG1("CAN'T OPEN THE SHELL SOCKET ON PORT %i\n", tcp_shell_port);
			return false; //oh no
		}
		ok = nlListen(shellmsock);
		if (!ok) {
			LOG("CAN'T SET SHELL SOCKET TO LISTEN MODE\n");
			return false; //oh no
		}
		shellssock = NL_INVALID;

		//start TCP shell master and slave threads
		pthread_create(&shellmthread, 0, thread_shellmaster_f, (void *)0);
		pthread_create(&shellsthread, 0, thread_shellslave_f, (void *)0);

		// reset game
		ctf_game_restart();

		//default serverinfo with reload hostname
		reload_hostname();

		return true;
	}

	//reload hostname
	void reload_hostname() {
		char dest[WHERE_PATH_SIZE];
		append_filename(dest, wheregamedir, "hostname.txt", WHERE_PATH_SIZE);
		FILE *cfg = fopen(dest, "r");
		if (cfg) {
			fgets(hostname, 256, cfg);
			hostname[ strlen(hostname) - 1 ] = 0;  //replace newline with \0
			LOG1("HOSTNAME IS = '%s'\n", hostname);

			//v0.4.7: ad name (FIXME MAKEIT)
			//fgets(hostadname, 256, cfg);
			//hostadname[ strlen(hostadname) - 1 ] = 0;  //replace newline with \0
			//LOG1("HOST AD NAME IS = '%s'\n", hostadname);

			fclose(cfg);
		}
		else
			strcpy(hostname, "ANNONYMOUS HOST");

		//update serverinfo
		update_serverinfo();
	}

	//update serverinfo
	void update_serverinfo() {

		//v0.4.8 UGLY FIX : count all players again, check for discrepancy
		int pc = 0;
		for (int i=0;i<maxplayers;i++) {
			if (player[i].used == true) {
				if (player[i].isbot == false) {
					pc++;
				}
			}
		}
		if (pc != player_count) { //debug
			LOG2("** update_serverinfo() BUG FOUND: PC=%i player_count=%i !\n", pc, player_count);
		}
		//force player_count
		player_count = pc;

		char sinfo[1024];
		sprintf(sinfo, "%2i %7s/%s", player_count, GAME_VERSION, hostname);
		server->set_server_info(sinfo);
	}

	// ---------- SERVER BOT API START -------------------------------------------

	// INSERE (tenta inserir) UM BOT PLAYER NO JOGO
	//  retorna: player id do bot criado ou -1 se o server ja ta full (tratar isso (-1) porque
	//     pode acontecer de um player de carne e osso conectar no meio tempo)
	//
	// essa funcao aqui se chama de algum outro método aqui do gameserver_c mesmo. quer dizer,
	// é a lógica do server é quem vai inserir ou remover bots do jogo. os clientes (players)
	// até podem fazer parte do processo (pedir mais bots ou menos bots), mas eles entram so
	// com requisicoes pro gameserver, que eh quem vai efetivamente colocar ou remover bots
	// do jogo
	int bot_connect() {

		if (bot_count + player_count >= maxplayers)		// nao deixa extrapolar o maximo, nao custa nada checar isso
			return -1;

		// consuma a conexao pelas vias naturais
		int bot_id = client_connected(-1, true);

		// -1? erro (acho que e normal)
		if (bot_id == -1)
			return -1;

		// mais um bots...
		//bot_count++;
		//char leex[333];
		//sprintf(leex, "bot %i CONNECT new count = %i\n", bot_id, bot_count);
		//broadcast_message(leex);

		// trocando o nome do bot
		//
		//static int fakek_counter =0;
		//sprintf(player[bot_id].name, "[BOT] P%i N%i", bot_id, fakek_counter++);
		sprintf(player[bot_id].name, "[BOT] %s", RandomName().c_str() );
		player[bot_id].name[15] = 0;	//max 15 chars

		// send entered-game message
		//
		//char dummy[1] = { 0 };
		//broadcast_message("@I%s added to the game%s", player[bot_id].name, dummy);
		//broadcast_sample(SAMPLE_ENTERGAME);

		// broadcast the new player's name
		//
		broadcast_player_name(bot_id);

		//		aqui:		gameserver_c::player[bot_id]  <-- player_t que representa o bot

		return bot_id;
	}

	// REMOVE UM BOT PLAYER DO JOGO
	// valem as mesmas observacoes do bot_connect acima
	// passa como parametro o id (player[id]) que o bot_connect te passou
	//
	void bot_disconnect(int bot_id) {

		//if player not used, do not disconnect
		if (!player[bot_id].used)
			return;

		// paranoia total
		if (player[bot_id].isbot == false) {
			LOG("\nERROR!!!!! ARGH QUE COISA ESTRANHA BOT_DISCONNECT() TENTANDO DESCONECTAR ALGUEM QUE NAO EH FLAGGED COMO BOT !!!\n");
			throw 6789;
			return;
		}

		// menos um bots...
		//bot_count--;
		//char leex[333];
		//sprintf(leex, "bot %i disconnect new count = %i\n", bot_id, bot_count);
		//broadcast_message(leex);

		// consuma a desconexão por vias naturais
		client_disconnected(bot_id, true);
	}

	//funcao invocada quando ha mudanca nas preferencias individuais de cada player sobre como
	// cada um deles acha que os bots no server devem se comportar (numero de bots, modo de preenchimento
	// com bots, etc.)
	void bot_prefs_change() {

		int mode[3];		//votos para modo[0] (sem opiniao), modo[1] (NO BOTS) e modo[2] (bot fill teams + extra)
		mode[0]=mode[1]=mode[2]=0;
		int sizeacum = 0;
		int i;

		// conta votos para modos e team sizes
		for (i=0;i<maxplayers;i++)
		if (player[i].used)
		if (!player[i].isbot)
		{
			switch (player[i].botmode) {
			case 0: mode[0]++; break;
			case 1: mode[1]++; break;
			case 2:
				mode[2]++;
				sizeacum += player[i].botsize;
				break;
			}
		}

		//se tem mais gente votando pra modo 2 (SIM BOTS) entao tem bots
		if (mode[2] > mode[1]) {
			botmode = 2;	//fit-to-size
			botsize = sizeacum / mode[2];
		}
		//senao, nao poe bots
		else
			botmode = 1;	//NO BOTS new v0.4.7
	}

	// bot_before_frame
	// aqui adicionar ou remover bots conforme necessidade
	void bot_before_frame() {

		//========================================================
		// 1. check entrada e saida de bots
		//========================================================

		//V0.4.7: novos botmodes
		int i;

		//botmode 1: disconnect any and all bots
		if (botmode == 1) {
			for (i=0;i<maxplayers;i++)
			if (player[i].used)
			if (player[i].isbot)
				bot_disconnect(i);
		}
		//botmode 2: fill teams and add some extra bots
		//
		// 1o) for the team with MOST HUMAN PLAYERS (any will do if equal #) compare number of bots
		//     with botsize
		//     IF EQUAL : ok
		//     WHILE #BOTS GREATER : disconnect a bot
		//     WHILE #BOTS LESSER : add a bot
		// 2o) compare the TOTAL number of players of both teams
		//     IF EQUAL : ok
		//     WHILE #OTHER > #FIRST : disconnect a bot from OTHER
		//     WHILE #OTHER < #FIRST : connect a bot to OTHER
		//
		else if (botmode == 2) {

			//count HUMAN players, BOT players and TOTAL players in each team
			int thc[2];thc[0]=0;thc[1]=0;
			int tbc[2];tbc[0]=0;tbc[1]=0;
			int ttc[2];ttc[0]=0;ttc[1]=0;
			int i;
			for (i=0;i<maxplayers;i++) {
				if (player[i].used) {
					if (player[i].isbot) {
						tbc[i/TSIZE]++;
						ttc[i/TSIZE]++;
					}
					else {
						thc[i/TSIZE]++;
						ttc[i/TSIZE]++;
					}
				}
			}

			//t is the team with most human players, or any of them
			int t = rand() % 2;
			if (thc[0] > thc[1])
				t = 0;
			else if (thc[1] > thc[0])
				t = 1;

			//ot is the other team
			int ot = 1 - t;

			//debug
			static int derun;

			//compare botsize with number of bots in team t
			if (tbc[t] == botsize) {
				//OK!
			}
			else if (tbc[t] > botsize) {
				derun = 0;
				//disconnect bots until equal
				while (tbc[t] > botsize) {
					if (derun++ > 1000)
						throw 36661;
					i = rand() % maxplayers;
					if (i/TSIZE == t)
					if (player[i].used)
					if (player[i].isbot) {
						//less one
						bot_disconnect(i);
						tbc[t]--;
						ttc[t]--;
					}
				}
			}
			else if (tbc[t] < botsize) {
				derun = 0;
				//connect bots
				while (tbc[t] < botsize) {
					if (derun++ > 1000)
						throw 36662;
					int p = bot_connect();
					if (p == -1)
						break; //enough...
					//another one...
					tbc[t]++;
					ttc[t]++;
				}
			}

			//compare total number of players of other team with the t team
			if (ttc[ot] == ttc[t]) {
				//OK!
			}
			else if (ttc[ot] > ttc[t]) {
				derun = 0;
				//disconnect bots from other
				while (ttc[ot] > ttc[t]) {
					if (derun++ > 1000)
						throw 36663;
					i = rand() % maxplayers;
					if (i/TSIZE == ot)
					if (player[i].used)
					if (player[i].isbot) {
						//less one
						bot_disconnect(i);
						tbc[ot]--;
						ttc[ot]--;
					}
				}
			}
			else if (ttc[ot] < ttc[t]) {
				derun = 0;
				//connect bots
				while (ttc[ot] < ttc[t]) {
					if (derun++ > 1000)
						throw 36664;
					int p = bot_connect();
					if (p == -1)
						break; //enough...
					//another one...
					tbc[ot]++;
					ttc[ot]++;
				}
			}

		}

		/*
		//fill-in
		if (botmode == 1) {

			//count HUMAN players in each team
			int tc[2];tc[0]=0;tc[1]=0;
			int i;
			for (i=0;i<maxplayers;i++)
				if (player[i].used)
				if (!player[i].isbot)
					tc[i/TSIZE]++;

			// 0. disconnect all bots from team that has the most human players

			int t = -1;
			if (tc[0] > tc[1])
				t = 0;
			else if (tc[1] > tc[0])
				t = 1;

			//same number of humans in both teams
			if (t == -1) {

				//simply disconnect any and all bots from the game
				for (i=0;i<maxplayers;i++)
				if (player[i].used)
				if (player[i].isbot)
					bot_disconnect(i);

			}
			//team t has the most human players
			else {

				// 0. disconnect all bots from team t
				for (i=0;i<maxplayers;i++)
				if (i/TSIZE == t)	//that team
				if (player[i].used)		//valid
				if (player[i].isbot)	//bot
					bot_disconnect(i);	//bye

				// 0,5 recalc number of players in each team
				//count players in each team
				tc[0]=tc[1]=0;
				for (i=0;i<maxplayers;i++)
					if (player[i].used)
						tc[i/TSIZE]++;

				// 1. disconnect bots from team that has most players, until number of players is the same
				// or the team with most players runs out of bots

				int t = -1;
				if (tc[0] > tc[1])
					t = 0;
				else if (tc[1] > tc[0])
					t = 1;

				//is there a team that has most players?
				if (t != -1) {

					for (i=0;i<maxplayers;i++)
					if (i/TSIZE == t) // player of that team
					if (player[i].used)	//valid
					if (player[i].isbot)	//bot
					if (tc[t] > tc[1-t])	//time continua com mais gente
					{
						tc[t]--;	//menos um
						bot_disconnect(i);	//tira o bot
					}
				}

				// 2. connect bots until number of players is the same

				while (tc[0] != tc[1])	//um dos timem continua com menos gente
				{
					int pid = bot_connect();	//coloca um bot - ja vai pro time com menos gente
					tc[pid/TSIZE]++;							//mais um pra esse time
				}

			}//t != -1
		}
		//set to target team size
		else if (botmode == 2) {

			//count players in each team
			int tc[2];tc[0]=0;tc[1]=0;
			int i;
			for (i=0;i<maxplayers;i++)
				if (player[i].used)
					tc[i/TSIZE]++;

			//for both teams
			for (int t=0;t<2;t++) {

				//keep disconnecting bots until no bots left or tc[t] == botsize
				if (tc[t] > botsize) {

					for (i=0;i<maxplayers;i++)
					if (i/TSIZE == t)					//do time
					if (player[i].used)		//player used
					if (player[i].isbot)	//eh bot
					if (tc[t] > botsize)	//time ainda ta grande demais
					{
						tc[t]--;		//menos um
						bot_disconnect(i);	//remove o bot
					}

				}
			}

			//connect bots until same number of players on both teams
			while ((tc[0] < botsize) || (tc[1] < botsize)) {

				//add a bot
				int pid = bot_connect();

				//update counter of the team the bot went
				tc[pid/TSIZE]++;
			}

		}
*/
		//========================================================
		// 2. calcula extrapolated frames
		//========================================================

		int fwd;	//forward frame count. 0= +100ms 1= +200ms ...
		for (fwd=0;fwd<BOT_XDEPTH;fwd++) {

			if (fwd == 0) {
				//world to xw[0]

				// start by copying state
				memcpy(&xw[0], &world, sizeof(frame_t));

				// run physics frame for all players
				for (int p=0;p<maxplayers;p++)
				if (player[p].used)
					run_server_player_physics(p, &world, &xw[0]);	//player id, frame source, frame dest
			}
			else {
				//xw[fwd-1] to xw[fwd]

				// start by copying state
				memcpy(&xw[fwd], &xw[fwd-1], sizeof(frame_t));

				// run physics frame for all players
				for (int p=0;p<maxplayers;p++)
				if (player[p].used)
					run_server_player_physics(p, &xw[fwd-1], &xw[fwd]);	//player id, frame source, frame dest
			}

			// update rocket coordinates
			for (int r=0;r<MAX_ROCKETS;r++) {

				rocket_c *rock = &xw[fwd].rock[r];
				rocket_c *oldrock;
				if (fwd == 0)
					oldrock = &world.rock[r];
				else
					oldrock = &xw[fwd-1].rock[r];

				if (rock->owner != -1)
				{
					//move-se
					rock->x = oldrock->x + rock->sx;
					rock->y = oldrock->y + rock->sy;

					//out of bounds
					if ((rock->x < -20) || (rock->y < -20) || (rock->x > plw + 20) || (rock->y > plh + 20)) {
						rock->owner = -1;	//just remove it. clients will figure out the same
					}
				}
			}
		}

		//========================================================
		// 3. FIXME MAKE-ME : calcula alguma coisa pros bots, baseado
		//    no extrapolated frame acima
		//========================================================

		// **** FIXME ****

	}

	// ******************************************************************************************
	// ******************************************************************************************
	//BOTZ!!!
	// !!!! NESSA FUNÇÃO É ONDE VAI A INTELIGENCIA ARTIFICIAL DO SUPER HIPER BOT DO JANNONE !!!!!
	//
	//  bot_frame_think()
	//  entradas:
	//     1. int bi: player id (indice do array "player" que deve estar no escopo da funcao)
	//     2. uma porrada de variaveis do gameserver_c, caso voce nao tenha notado (perfeitamente
	//        compreensivel devido ao tamanho dessa porcaria) essa funcao eh um metodo de
	//        gameserver_c, entao o bot enxerga tudo que o server enxerga (maps, jogadores, etc.)
	//        o bot pode ate roubar e ir correndo atras de powerups e pessoas que ele nao enxerga!!
	//        --> PERGUNTAR PRO FABIO PARA QUE VARIAVEIS PERGUNTAR QUAIS INFORMACOES O BOT PRECISA
	//
	//    essa funcao vai ser chamada de 100 em 100 milisegundos, ou seja, sempre que o server
	//    realizar um step de simulacao (aquela coisa suave clientside eh pura extrapolacao de
	//    movimento). o objetivo dessa funcao eh o bot dizer, para essa frame, que teclas ele
	//    estah pressionando (o input dele)
	//
	//  saida:
	//     a saida do bot escreve direto nessas vars: (eu sei, tah podre, fazer o que:)
	//     world.hero[bi].l  :  se esta pressionando a tecla de "move left"
	//     world.hero[bi].r  :  se esta pressionando a tecla de "move right"
	//     world.hero[bi].u  :  se esta pressionando a tecla de "move up"
	//     world.hero[bi].d  :  se esta pressionando a tecla de "move down"
	//     world.hero[bi].run : se esta pressionando a tecla de "run"
	//    (*** ATENCAO ACHO QUE BOT NAO PRECISA DO STRAFE -- USAR "GUNDIR" DIRETO ***)
	//     world.hero[bi].strafe :  se esta pressionando a tecla de "strafe"
	//     world.hero[bi].gundir :  gun direction 0-7 (0 = right 1 = right-down 2 = down ...... 7 = right-up
	//                              gundir=0 eh pra direita, e vai aumentando 1 por 1 girando em sentido horario ate 7 (sao 8 angulos diferentes)
	//     player[bi].attack : true se esta pressionando a tecla de ataque (nao significa que
	//                         vai sair um tiro na proxima frame, pois tem o delay do tiro, que
	//											da' pra ser lido de player[bi].next_shoot_time)
	//
	// ******************************************************************************************
	// ******************************************************************************************
	void bot_frame_think(int bi) {

		// bot mutcho louco!
		//
		world.hero[bi].l = (rand() % 2) == 0;
		world.hero[bi].r = (rand() % 2) == 0;
		world.hero[bi].u = (rand() % 2) == 0;
		world.hero[bi].d = (rand() % 2) == 0;
		world.hero[bi].run    = (rand() % 2) == 0;
		world.hero[bi].gundir = rand() % 8;		//0..7
		player[bi].attack     = (rand() % 2) == 0;
	}

	// ---------- SERVER BOT API END ---------------------------------------------

	//client connected (callback function)
	// se is_bot, significa que é um BOT! essa funcao eh chamada pelo callback do leet server
	//  quando eh um client de carne e osso, e tambem chamada pelo BOT_CONNECT para conectar um bot
	//  pelas vias naturais
	//
	//NEW! a funcao retorna o player id alocado, bom para bots
	//
	int client_connected(int id, bool is_bot) {

		//2TEAM: check wich team to put player
		int t1, t2, targ;
		t1 = 0;		//red team count
		t2 = 0;		//blue team count
		int i;
		for (i=0;i<maxplayers;i++) {
			if (player[i].used)
			if ((!is_bot) && (player[i].isbot))	//NAO CONTA BOTS SE NAO FOR BOT!
			{
				//nop
			}
			else
				if (i/TSIZE == 0)
					t1++;
				else
					t2++;
		}

		//put on red team, blue team, or randomize if same # of players in both teams
		if (t1 < t2)
			targ = 0;
		else if (t1 > t2)
			targ = TSIZE;
		else {
			//refresh team modifiers
			refresh_team_score_modifiers();

			//this means team 0 is LACKING
			if (team_smul[0] > team_smul[1])
				targ = 0;
			//this means team 1 is LACKING
			else if (team_smul[1] > team_smul[0])
				targ = 8;
			// 0 or 8 -- choose a random one
			else
				targ = (rand()%2) * TSIZE;
		}

		//alloc new player : scans only slots of the team (targ...targ + 7)
		int myself = -1;

		for (i=targ;i<(targ+TSIZE);i++)
		{
			//UPDATE: sobrescreve bots SE NAO EH BOT
			if ((!player[i].used) || ((player[i].isbot) && (!is_bot)) )
			{
				//se bot, tira
				if (player[i].isbot)
					bot_disconnect(i);

				// add player to players_present
				//players_present = players_present | (1 << i);

				// init player
				//#NR
				int cid;
				if (is_bot)
					cid=222+i;
				else
					cid=id;
				ctop[cid]=i;

				player[i].clear(true, is_bot, i, cid, SERVER_DEFAULT_PLAYER_NAME);

				myself = i;

				// spawn player
				game_respawn_player(false, i);

				//reset keypresses
				world.hero[i].l = 0;
				world.hero[i].r = 0;
				world.hero[i].u = 0;
				world.hero[i].d = 0;

				//check pickup creation
				check_pickup_creation(false);

				break;
			}
		}

		// internal error can be detected if no player is free (used == false)
		//0.4.7: normal behavior (bots...)
		if (myself == -1) {
			//LOG("ERROR: BAD BAD BAD INTERNAL GAMESERVER ERROR myself == -1 !!! client_connected()...\n");
			return myself;	//-1...
		}

		//CONNECT OK: another one...
		if (!is_bot)
			player_count++;
		else
			bot_count++;

		// se o player_count ficou == 2, reseta partida
		//
		if (player_count + bot_count == 2)
			ctf_game_restart();

		//char lelix[222];
		//sprintf(lelix, "PCOUNT = %i BCOUNT = %i\n", player_count, bot_count);
		//broadcast_message(lelix);

		// "if not a bot..." inits
		//
		if (!is_bot) {

			char lebuf[256]; int count;

			//**** V0.4.4 : init oneclient_c ****
			//		v0.4.7: IF NOT A BOT !!!
			client[id].reset();
			client[id].token_have = false;		// no token

			//first update the ADMIN SHELL
			if (shellssock) {
				count = 0;
				writeLong(lebuf, count, STA_PLAYER_CONNECTED);
				writeLong(lebuf, count, player[myself].cid);
				nlWrite(shellssock, lebuf, count);
			}

			//update the player with world information
			//	- who is he (player #)

			send_me_packet(myself);

			// - world ctf flags information
			ctf_net_flag_status(id, 0);
			ctf_net_flag_status(id, 1);

			// MAP NAME+CRC !!! VERY IMPORTANT
			//
			send_map_change_message(myself, NEXTMAP_NONE, maprot[currmap]);

			// - all other player's names
			// - all other player's frags

			for (i=0;i<maxplayers;i++)
			if (player[i].used)
			if (i != myself) {

				//player name update
				send_player_name_update(id, i);
				//count = 0;
				//writeByte(lebuf, count, 1);	// "1" = player name update
				//writeByte(lebuf, count, i);		// what player id
				//player[i].name[15]=0;		// force trunc name at 15 chars (paranoia)
				//writeString(lebuf, count, player[i].name);
				//server->send_message(id, lebuf, count);

				//frags update
				count = 0;
				writeByte(lebuf, count, 4);	//"4" = frags update
				writeByte(lebuf, count, i);		// what player id
				writeLong(lebuf, count, player[i].frags);
				server->send_message(id, lebuf, count);

				//crap update
				send_player_crap_update(id, i);
			}

			//#NR
			for (vector<string>::const_iterator line=welcome_message.begin(); line!=welcome_message.end(); line++)
				player[myself].add_to_queue(*line);
		}

		//check for team changes
		//
		check_team_changes();

		//update serverinfo
		update_serverinfo();

		//ok!
		//
		return myself;
	}

	//client disconnected (callback function)
	//
	// mesmas observacoes para client_connect. adicionalmente:
	//  se is_bot == true, entao id eh um PLAYER ID
	//  se is_bot == false, entao id eh um CLIENT ID
	//
	void client_disconnected(int id, bool is_bot) {
		if (!is_bot && ctop[id]==-1)
			return;

		int pid;

		if (!is_bot) {

			//less one...
			player_count--;

			//what player
			pid = ctop[id];

			//first update the ADMIN SHELL
			if (shellssock) {
				char lebuf[256]; int count;
				count = 0;
				writeLong(lebuf, count, STA_PLAYER_DISCONNECTED);
				writeLong(lebuf, count, player[pid].cid);
				nlWrite(shellssock, lebuf, count);
			}

			//broadcast a textual message "Player BLABLA left the game"
			bprintf("@I%s left the game with %i frags", player[pid].name, player[pid].frags);

			//sound
			broadcast_sample(SAMPLE_LEFTGAME);

			//report the latest player achievements to the master server
			client_report_status(id);

			//clear oneclient_c
			client[id].reset();
		}
		else {

			//less one...
			bot_count--;

			//se bot, pid eh o proprio id
			pid = id;
		}

		//remove player from the game
		game_remove_player(pid);
		ctop[id]=-1;

		//check for team changes - APENAS se nao era bot. bots saindo/entrando nao devem afetar isso!
		if (!is_bot) {
			check_team_changes();
			check_map_exit();	//#NR
		}

		//update serverinfo
		update_serverinfo();
	}

	//client ping result
	void ping_result(int client_id, int ping_time) {
		if (ctop[client_id]==-1)
			return;

		//save result
		player[ ctop[client_id] ].ping = ping_time;
	}

	//process incoming client data (callback function)
	void incoming_client_data(int id, char *data, int length) {
		if (ctop[id]==-1)
			return;

		//player id
		int pid = ctop[id];

		//for talk msgs
		char talkmsg[256];

		//1. process client's frame data
		NLubyte keys;
		int count = 0;
		readByte(data, count, keys);
		//update the player's direction keys (accel.vectrs) (and other keys too if needed later)
		hero_t *h;
		h = &(world.hero[pid]);

		h->l = (keys & 1) != 0;
		h->r = (keys & 2) != 0;
		h->u = (keys & 4) != 0;
		h->d = (keys & 8) != 0;
		h->strafe = (keys & 16) != 0;
		h->run = (keys & 32) != 0;

		//if not strafing, update direction
		if (!h->strafe) {
			// left
			if ((h->l) && (!h->r)) {
				if ((h->u) && (!h->d))	// + up
					h->gundir = 5;
				else if ((!h->u) && (h->d)) // + down
					h->gundir = 3;
				else if ((!h->u) && (!h->d)) // !up !down
					h->gundir = 4;
			}
			// right
			else if ((!h->l) && (h->r)) {
				if ((h->u) && (!h->d))	// + up
					h->gundir = 7;
				else if ((!h->u) && (h->d)) // + down
					h->gundir = 1;
				else if ((!h->u) && (!h->d)) // !up !down
					h->gundir = 0;
			}
			//!right !left
			else if ((!h->l) && (!h->r)) {
				if ((h->u) && (!h->d))	// + up
					h->gundir = 6;
				else if ((!h->u) && (h->d)) // + down
					h->gundir = 2;
			}
		}

		//2. process messages
		char *msg;
		int msglen;
		do {
			msg = server->receive_message(id, &msglen);
			if (msg != 0) {
				// process a client's message
				//
				int count = 0;
				NLubyte code;
				readByte(msg, count, code);
				if (code == 1) {

					//read the name
					char tempname[256];
					readString(msg, count, tempname); //name update request

					//v0.4.9: IF SAME NAME: ignore it. if not, keep going
					if (strcmp(tempname, player[pid].name))
					//name change flooding protection
					if (get_time() >= player[pid].waitnametime) {

						//FLUSH PENDING REPORTS TO MASTER IF token_have/token_valid !!!
						client_report_status(id);

						//name changed -- this means that the player is NOT REGISTERED
						//  anymore for recording statistics
						client[id].token_have = false;

						//need to broadcast the player's crap to remove eventual '*' and stuff
						broadcast_player_crap( pid );

						//check if it's the first name information from client. then it
						// must have just entered the game
						bool entered_game = !strcmp(SERVER_DEFAULT_PLAYER_NAME, player[pid].name);

						// log
						//LOG3("client %i player %i '%s' renamed to", id, pid, player[pid].name);

						//readString(msg, count, player[pid].name); //name update request
						strcpy(player[pid].name, "(invalid name)");
						if (strpbrk(tempname, "%@")!=NULL)
							player[pid].add_to_queue("@WSorry, this server doesn't accept %% or @ in a name");
						else if (strspnp(tempname, " ")==NULL)
							player[pid].add_to_queue("@WPlease enter a name");
						else {
							#ifdef NR_NAME_AUTHORIZATION
							int nid=authorizations.identifyName(tempname);
							if (nid==-1 || authorizations.authorize(nid, server->get_client_address(id)))
								strcpy(player[pid].name, tempname);
							else {
								player[pid].queue_printf("@WThe name %s is reserved on this server.", authorizations.getName(nid).c_str());
								player[pid].queue_printf("@ITo authorize, type /auth %s,pass where pass is your password.", authorizations.getName(nid).c_str());
							}
							#else
							strcpy(player[pid].name, tempname);
							#endif
						}

						//LOG1("'%s'\n", player[pid].name);

						//send entered-game message
						if (entered_game) {
							bprintf("@I%s entered the game", player[pid].name);
							//sound
							broadcast_sample(SAMPLE_ENTERGAME);
						}
						// next time allowed to change name
						player[pid].waitnametime = get_time() + 1.0;

						//broadcast the new player's name
						broadcast_player_name(pid);

						//update the ADMIN SHELL
						if (shellssock) {
							char lebuf[256]; int count = 0;
							writeLong(lebuf, count, STA_PLAYER_NAME_UPDATE);
							writeLong(lebuf, count, player[pid].cid);
							writeString(lebuf, count, player[pid].name);
							if (entered_game) {
								writeLong(lebuf, count, STA_PLAYER_IP);
								writeLong(lebuf, count, player[pid].cid);
								char addrBuf[50];
								NLaddress addr=server->get_client_address(player[pid].cid);
								nlSetAddrPort(&addr, 0);
								nlAddrToString(&addr, addrBuf);
								writeString(lebuf, count, addrBuf);
							}
							nlWrite(shellssock, lebuf, count);
						}
					}
				}
				//chat!
				else if (code == 2) {

					//#NR remove single %'s
					char sbuf[strlen(msg+1)+1];
					int si=0, mi=1;
					do {
						if (msg[mi]!='%')
							sbuf[si++]=msg[mi++];
						else if (msg[mi+1]=='%') {	// allow pairs of '%' only
							sbuf[si++]='%';
							sbuf[si++]='%';
							mi+=2;
						}
						else
							++mi;
					} while (msg[mi-1]!='\0');
					//#NR handle 'console' commands
					#ifdef NR_CONSOLE
					if (player[pid].delayedMessages.size()>2) {
						player[pid].delayedMessages.clear();
						plprintf(pid, "@I(rest of message cancelled)");
					}
					player[pid].reset_message_queue_timing();
					if (sbuf[0]=='/') {
						char* pCommand=sbuf+1;
						char cbuf[30];
						int ci;
						for (ci=0;; ++ci, ++pCommand) {
							if (ci==29) {
								cbuf[29]='\0';
								break;
							}
							if (*pCommand==' ') {
								cbuf[ci]='\0';
								++pCommand;
								break;
							}
							cbuf[ci]=*pCommand;
							if (*pCommand=='\0')
								break;
						}
						if (!strcmp(cbuf, "help")) {
//							                          123456789*123456789*123456789*123456789*123456789*123456789*123456789*123456789*
							player[pid].queue_printf("@TConsole commands available on this server:");
							player[pid].queue_printf("/help       this screen");
							if (!info_message.empty())
								player[pid].queue_printf("/info       information about this server");
							player[pid].queue_printf("/config     current server configuration");
							player[pid].queue_printf("/stats      see your stats");
							player[pid].queue_printf("/mapinfo n  information about map n (default: current map)");
							player[pid].queue_printf("/votemap n  vote for the next map to be n (default: list maps and votes)");
							player[pid].queue_printf("/time       check server uptime, current map time and time left on the map");
							player[pid].queue_printf("/sayadmin   send a message to the server admin (in English or Finnish, please)");
						}
						else if (!strcmp(cbuf, "info") && !info_message.empty()) {
							for (vector<string>::const_iterator line=info_message.begin(); line!=info_message.end(); line++)
								player[pid].add_to_queue(*line);
							player[pid].add_to_queue("type /config to see current server settings");
						}
						else if (!strcmp(cbuf, "config")) {
							player[pid].queue_printf("@TCurrent server settings:");
							player[pid].queue_printf("Flag capture limit: %i", capture_limit);
							if (time_limit == 0)
								player[pid].queue_printf("No map time limit.");
							else
								player[pid].queue_printf("Map time limit: %i min", time_limit / 10 / 60);
							if (pup_max_time > pup_add_time)
								player[pid].queue_printf("Power-ups add %d seconds to what's left, with a maximum of %d seconds", pup_add_time, pup_max_time);
							else
								player[pid].queue_printf("Power-up time is %d seconds", pup_max_time);
							if (pup_deathbringer_switch)
								player[pid].queue_printf("Picking up a second deathbringer power-up cancels the effect");
							if (shadow_minimum == 1)
								player[pid].queue_printf("A player using the shadow power-up gets totally invisible");
							ostringstream pupstr;
							pupstr << "Base number of power-ups is " << pups_min; if (pups_min_percentage) pupstr << "%%";
							pupstr << " and upper limit " << pups_max; if (pups_max_percentage) pupstr << "%%";
							if (pups_min_percentage || pups_max_percentage)
								pupstr << " (%% of map size)";
							player[pid].add_to_queue(pupstr.str());
							#ifdef NR_SERVER_PHYSICS
							player[pid].queue_printf("The physics model is different (looks funny with a standard 0.5.0 client)");
							#endif
						}
						else if (!strcmp(cbuf, "sayadmin")) {
							if (strspnp(pCommand, " ")!=NULL) {
								FILE* logp=fopen("msglog.txt", "at+");
								time_t tt=time(0);
								struct tm* tmb=localtime(&tt);
								fprintf(logp, "%d-%02d-%02d %02d:%02d:%02d  %s: %s\n", tmb->tm_year+1900, tmb->tm_mon+1, tmb->tm_mday,
										tmb->tm_hour, tmb->tm_min, tmb->tm_sec, player[pid].name, pCommand);
								fclose(logp);
								if (shellssock) {
									char lebuf[256];
									int count=0;
									writeLong(lebuf, count, STA_ADMIN_MESSAGE);
									writeLong(lebuf, count, player[pid].cid);
									writeString(lebuf, count, pCommand);
									nlWrite(shellssock, lebuf, count);
								}
								player[pid].queue_printf("@IYour message has been logged. Thank you for your feedback!");
							}
							else
								player[pid].queue_printf("@IFor example to send \"Hello!\", type /sayadmin Hello!");
						}
						else if (!strcmp(cbuf, "map") || !strcmp(cbuf, "mapinfo")) {
							if (*pCommand!='\0') {
								int mid=atoi(pCommand)-1;
								if (mid>=0 && mid<maprots && pCommand[strspn(pCommand, "0123456789")]=='\0') {
									player[pid].queue_printf("@IMap %d is %s", mid+1, mapinfo[mid].title.c_str());
									player[pid].queue_printf("@I%s.txt, size %dx%d", mapinfo[mid].file.c_str(), mapinfo[mid].width, mapinfo[mid].height);
								}
								else
									player[pid].queue_printf("@WValid map id's are 1 to %d", maprots);
							}
							else {
								player[pid].queue_printf("@IThis map is %s", mapinfo[currmap].title.c_str());
								player[pid].queue_printf("@I%s.txt, size %dx%d", mapinfo[currmap].file.c_str(), mapinfo[currmap].width, mapinfo[currmap].height);
							}
							player[pid].queue_printf("@IType /votemap to see a list of all maps");
						}
						else if (!strcmp(cbuf, "votemap")) {
							string status;
							bool err=false;
							if (*pCommand!='\0') {
								int mid=atoi(pCommand)-1;
								if (mid>=-1 && mid<maprots && pCommand[strspn(pCommand, "0123456789")]=='\0') {
									if (player[pid].mapVote==mid)
										status="no changes";
									else {
										if (player[pid].mapVote==-1)
											status="vote added";
										else if (mid==-1)
											status="vote removed";
										else
											status="vote updated";
										player[pid].mapVote=mid;
										check_map_exit();
									}
									if (!player[pid].want_map_exit)
										player[pid].queue_printf("@TPress F4 to actually vote for a mapchange");
								}
								else {
									player[pid].queue_printf("@W\"%s\" is not a valid map id (1 to %d)", pCommand, maprots);
									err=true;
								}
							}
							else
								player[pid].queue_printf("@TFor example to vote for map 1, type /votemap 1");
							if (!err) {
								if (status.length())
									player[pid].queue_printf("@T(%s) Maps on this server: ID, votes, description", status.c_str());
								else
									player[pid].queue_printf("@TMaps on this server: ID, votes, description");
								// 26 chars usable for entry, to fit three on a line
								char buf[200]; int bufi=0;
								int rows=(maprots+2)/3;
								for (int row=0; row<rows; ++row) {
									for (int col=0; col<3; ++col) {
										int mid=col*rows+row;
										if (mid>=maprots)
											continue;
										sprintf(buf+bufi, "%2d %2d %-18s", mid+1, mapinfo[mid].votes, mapinfo[mid].title.c_str());
										if (strlen(buf+bufi)>24)
											strcpy(buf+bufi+23, ".. ");
										else
											strcpy(buf+bufi+24, "  ");
										bufi+=26;
									}
									player[pid].queue_printf("%s", buf);
									bufi=0;
								}
							}
						}
						else if (!strcmp(cbuf, "time")) {
							// server uptime
							unsigned long uptime = frame/10/60;	// minutes
							int days = uptime / 60 / 24;
							ostringstream server_time;
							server_time << "@IThe server has been up for ";
							if (days > 0)
								server_time << ' ' << days << " day" << (days > 1 ? "s " : " ");
							server_time << uptime / 60 % 24 << ':' << setfill('0') << setw(2) << uptime % 60;
							if (days == 0)
								server_time << " hours";
							server_time << '.';
							player[pid].add_to_queue(server_time.str());
							// map time
							int seconds = (frame - map_start_time) / 10;
							int remaining_seconds = (time_limit / 10 - seconds);
							ostringstream map_time;
							map_time << "@IMap time: " << seconds / 60 << ':' << setfill('0') << setw(2) << seconds % 60 << '.';
							if (time_limit == 0)
								map_time << " There is no time limit.";
							else {
								// time limit not very useful when only one player
								int players = 0;
								for (int i = 0; i < maxplayers; i++)
									if (player[i].used)
										players++;
								if (players == 1)
									map_time << " No time limit at the moment as you are the only player.";
								else if (remaining_seconds < 0) // if time is out and game continues, it must be sudden death
									map_time << " Sudden death.";
								else {
									map_time << " Time left: " << remaining_seconds / 60 << ':';
									map_time << setfill('0') << setw(2) << remaining_seconds % 60 << '.';
								}
							}
							player[pid].add_to_queue(map_time.str());
						}
						else if (!strcmp(cbuf, "stats")) {
							int playing_time = (int)get_time() - player[pid].start_time;  // seconds
							int lifetime = playing_time / (player[pid].total_deaths + 1);
							player[pid].queue_printf("Your stats: %d captures, %d kills, %d deaths, %d suicides",
								player[pid].total_captures,
								player[pid].total_kills,
								player[pid].total_deaths,
								player[pid].total_suicides);
							player[pid].queue_printf("Enemy flags: %d taken, %d dropped",
								player[pid].total_flags_taken,
								player[pid].total_flags_dropped);
							player[pid].queue_printf("Own flags: %d returned, %d carriers killed",
								player[pid].total_flags_returned,
								player[pid].total_flag_carriers_killed);
							int accuracy = 0;
							if (player[pid].total_shots > 0)
								accuracy = int((100. * player[pid].total_hits) / player[pid].total_shots + 0.5);
							player[pid].queue_printf("Shots: %d shot, accuracy %d %%%%, %d taken",
								player[pid].total_shots,
								accuracy,
								player[pid].total_shots_taken);
							player[pid].queue_printf("You have played %d minutes. Your average lifetime is %d:%02d.",
								playing_time / 60,
								lifetime / 60,
								lifetime % 60);
							// Add more stats: flags taken and dropped, flag carrying time, travelled distance,
							// shots, shot accuracy, shots taken, flag carriers killed, suicides, etc.
						}
						#ifdef NR_NAME_AUTHORIZATION
						else if (!strcmp(cbuf, "auth")) {
							string nameUpr;
							for (; *pCommand && *pCommand!=','; ++pCommand)
								nameUpr+=toupper(*pCommand);
							if (*pCommand==',') {
								string pwd(pCommand+1);
								if (authorizations.addIP(nameUpr, pwd, server->get_client_address(id))) {
									authorizations.save();
									player[pid].queue_printf("@WOK: authorized your IP address to use %s", nameUpr.c_str());
									player[pid].queue_printf("@WYou may change your name now");
								}
								else
									player[pid].queue_printf("@WAuthorization failed");
							}
							else
								player[pid].queue_printf("@WInvalid auth command");
						}
						#endif
						else
							player[pid].queue_printf("@WUnknown command %s. Type /help for a list.", cbuf);
					}
					else
					#endif
					if (strspnp(sbuf, " ")!=NULL) {	// ignore messages that are all spaces
						//talk flood protection
						player[pid].talk_temp += player[pid].talk_hotness;
						player[pid].talk_hotness += 3.0;
						if (player[pid].talk_temp > 18.0)
							player[pid].talk_temp = 18.0;
						if (player[pid].talk_hotness > 6.0)
							player[pid].talk_hotness = 6.0;

						if (player[pid].talk_temp > 10.0) {

							//esquenta o cara
							player[pid].talk_temp = 18.0;

							char elbuf[200]; int elcnt = 0;
							writeByte(elbuf, elcnt, 2);
							writeString(elbuf, elcnt, "@WToo much talk. Chill...");
							server->send_message(player[pid].cid, elbuf, elcnt);
						}
						else if (player[pid].muted==1) {
							plprintf(pid, "@WYou are muted. You can't send messages.");
						}
						else {
							// check for team message:
							if (msg[1] == '.') {
								sprintf(talkmsg, "@T%s: %s", player[pid].name, sbuf+1);
								if (player[pid].muted==2)
									player_message(pid, talkmsg);
								else
									broadcast_team_message(pid/TSIZE, talkmsg);
							}
							//regular msg
							else {
								sprintf(talkmsg, "%s: %s", player[pid].name, sbuf);
								if (player[pid].muted==2)
									player_message(pid, talkmsg);
								else
									broadcast_message(talkmsg);
							}
							// log
							//LOG4("client %i player %i name %s says: '%s'\n", id, pid, player[pid].name, &(msg[1]));
						}
					}
				}
				//+attack
				else if (code == 5) {

					//if attack was false, it's a fire event (fire false-->true)
					bool fire_event = (player[pid].attack == false);

					player[pid].attack = true;

					if (fire_event) { // firing code moved to simulate/broadcast frame (player[id].attack is tested there)
					}

				}
				//SUICIDE!!
				else if (code == 10) {

					//only if alive still
					if (player[pid].health > 0) {
						game_damage_player(pid, pid, 30000);
						player[pid].total_deaths++;
						player[pid].total_suicides++;
						//frag penalty
						player[pid].frags--;
						//delay respawn if player does not have deathbringer (there is another delay for that)
						if (!player[pid].item_deathbringer)
							player[pid].respawn_time += waiting_time_deathbringer;
					}
				}
				//-attack
				else if (code == 11) {

					//if attack was true, it's a fire event (fire false-->true)
					bool unfire_event = (player[pid].attack == true);

					player[pid].attack = false;

					if (unfire_event) {
					}
				}
				// want changeteams on
				else if (code == 12) {
					if (!player[pid].want_change_teams) {
						//set
						player[pid].want_change_teams = true;
						//show message
						//broadcast_message("@I%s player '%s' wants to change teams", teamname[pid/TSIZE], player[pid].name);
						//check for team changes
						check_team_changes();
					}
				}
				// want changeteams off
				else if (code == 13) {
					// show message if command had effect
					if (player[pid].want_change_teams) {
						player[pid].want_change_teams = false;
						player[pid].team_change_pending = false; //so pra garantir
						//broadcast_message("@I%s player '%s' don't want to change teams", teamname[pid/TSIZE], player[pid].name);
					}
				}
				// game preferences update
				else if (code == 19) {
					NLubyte prefcode;
					readByte(msg, count, prefcode);
					if (prefcode == 1) {
						//bot preferences
						readByte(msg, count, player[pid].botmode);
						readByte(msg, count, player[pid].botsize);
						bot_prefs_change();
					}
					else {
						//unknown preferences --- just discard message
					}
				}
				// "client ready" message
				else if (code == 21) {
					//client is ready to play now
					player[pid].awaiting_client_ready = false;
				}
				// want change teams
				else if (code == 22) {
					if (player[pid].want_map_exit == false) {
						player[pid].want_map_exit = true;
						check_map_exit();		//check map exit
					}
				}
				// dont' want change teams
				else if (code == 23) {
					if (player[pid].want_map_exit == true) {
						player[pid].want_map_exit = false;
						check_map_exit();		//check map exit
					}
				}
				// v0.4.4: UDP DOWNLOAD of files - request
				else if (code == 27) {
					char ftype[64];
					char fname[256];
					readString(msg, count, ftype);
					readString(msg, count, fname);
					if (client[id].serving_udp_file) {
						// FIXME: ERROR: this client is already downloading a file
						LOG1("ERROR: CLIENT %i ALREADY DOWNLOADING A FILE!\n", id);
					}
					else {
						//alloc to download
						client[id].serving_udp_file = true;
						char buffy[65536];		//buffy is our friend buffer
						int fsize = get_download_file((char *)buffy, ftype, fname);
						//error: can't read file FIXME: DISCONNECT THE CLIENT
						if (fsize == -1) {
							LOG("ERROR: CAN'T READ THE FILE CLIENT IS ASKING FOR. FIXME: MUST DISCONNECT HIM...\n");
						}
						else {
							client[id].data = new char[fsize];	//allocated to fit!
							memcpy(client[id].data, buffy, fsize);	//copy from buffy to the client's buffer
							client[id].dp = 0;	//RESET FILE POINTER: important
							client[id].fsize = fsize;
							//send a chunk
							upload_next_file_chunk(id);
						}
					}
				}
				// v0.4.4: UDP DOWNLOAD of files - the ack
				else if (code == 29) {

					//check expected ack
					NLulong ackpos;
					readLong(msg, count, ackpos);
					if (client[id].old_dp == ackpos) {

						//check upload successful
						if (client[id].dp >= client[id].fsize) {
							//no more data, this was the last ack. close stuff
							client[id].download_reset();	//reset the download data structs
											//the client will carry on from here
						}
						else {
							//send next
							upload_next_file_chunk(id);
						}
					}
					else {
						//unexpected ack pos. should never happen and if it does ,just discard...
					}
				}
				// v0.4.4 : client is telling his newest token
				else if (code == 30) {

					//get the token from the client
					readString(msg, count, client[id].token);	//read the token
					int intoken = atoi(client[id].token); //atoi it

					//se TOKEN SET + TOKEN VALID + TOKEN IDENTICO : ignora
					if ((client[id].token_have) && (client[id].token_valid) && (intoken == client[id].intoken)) {
#ifdef DEBUG_RANKING
						bprintf("%s %i %i token identico.", player[ctop[id]].name, client[id].intoken, intoken);
#endif
					}
					//senao, faz a rotina de dispatch old results + set new invalid token to be validated
					else {

#ifdef DEBUG_RANKING
						//debug message
						if ((client[id].token_have) && (client[id].token_valid))
							bprintf("%s %i %i changes registration, submitting results...", player[ctop[id]].name, client[id].intoken, intoken);
						else if ((client[id].token_have) && (!client[id].token_valid))
							bprintf("%s %i %i changes registration ('?')", player[ctop[id]].name, client[id].intoken, intoken);
						else if (!client[id].token_have)
							bprintf("%s %i %i sends registration.", player[ctop[id]].name, client[id].intoken, intoken);
#endif

						// v0.4.9 FIX : IF HAD previous token have/valid, then FLUSH his stats
						client_report_status(id);

						// new token
						client[id].intoken = intoken;

						// NEW (or first) REGISTRATION -- reset player report / stop reporting his old ID
						client[id].neg_delta_score = 0;
						client[id].delta_score = 0; //V0.4.8
						client[id].fdp = 0.0; //V0.4.8
						client[id].fdn = 0.0; //V0.4.8
						client[id].score = 0;
						client[id].neg_score = 0;		//V0.4.8
						client[id].rank = 0;

						client[id].token_have = true;			//token set
						client[id].token_valid = false;		//BUT not validated yet

						//v0.4.5 : atualiza registration char / score / rank
						broadcast_player_crap( ctop[id] );

						// ENQUEUE TOKEN VALIDATION IN A QUEUE THAT TALKS TO THE MASTER SERVERS

						//create job
						masterjob_c *job = new masterjob_c();
						job->cid = id;
						job->code = 1;
						sprintf(job->request, "GET /servlet/fcecin.tk1/index.html?%s&chktk&name=%s&token=%s\n\n", TK1_VERSION_STRING, player[ ctop [id] ].name, client[id].token);

						//LOG2("== MJOB CREATED : %i '''%s'''\n", id, job->request);

						pthread_t mjob_thread;
						pthread_create (&mjob_thread, 0, thread_masterjob_f, (void *)job);
					}
				}
				//V0.4.9: DEBUGGING the TAB ranking stuff
				else if (code == 33) {
						player_message( ctop[id], "Refreshed your crap...");
						//broadcast his crap
						broadcast_player_crap( ctop[id] );
				}
				// drop flag
				else if (code == 34)
					ctf_drop_flag_if_any(pid);
				else {
					//ERROR: unknown message from client
					LOG3("ERROR: UNKNOWN MESSAGE FROM CLIENT %i CODE=%i LENGTH=%i\n", id, code, msglen);
				}
			}
		} while (msg != 0);
	}

	//team t's flag touched by player #i?
	bool check_flag_touch(int i, int px, int py, int x, int y, int t) {
		if (world.flag[t].carried) return false;	//carried can't touch
		if (world.flag[t].pos.px != px) return false;	//screen x mismatch
		if (world.flag[t].pos.py != py) return false;	//screen y mismatch

		int fx = world.flag[t].pos.x;
		int fy = world.flag[t].pos.y;

		if (fx > x - 30)
		if (fx < x + 30)
		if (fy > y - 30)
		if (fy < y + 30)
			return true;	//touch

		return false;
	}

	//run a physics frame simulation step for a player
	void run_server_player_physics(int i, frame_t *src, frame_t *dest) {	//player id, frame source, frame dest
		if (dest != src)
			dest->hero[i] = src->hero[i];
		hero_t* hd = &dest->hero[i];

		if (hd->tx<0 || hd->ty<0 || hd->tx>=map.w || hd->ty>=map.h) return;	//#NR remove this and track why these are given sometimes
		const Room& room = map.room[hd->tx][hd->ty];

		bool carryFlag = src->flag[1-(i/TSIZE)].carried && src->flag[1-(i/TSIZE)].carrier == i;
		bool deathbringerAffected = player[i].deathbringer_end >= get_time();

		#ifdef NR_SERVER_PHYSICS
			bool realBounce = NR_applyPhysics(hd, room, 1., player[i].item_speed, carryFlag, deathbringerAffected);
		#else
			#ifdef NR_FIX_BOUNCING
				bool realBounce = applyDefaultPhysics(hd, room, 1., player[i].item_speed, carryFlag, deathbringerAffected, true);
			#else
				bool realBounce = applyDefaultPhysics(hd, room, 1., player[i].item_speed, carryFlag, deathbringerAffected, false);
				#define DEFAULT_DONE
			#endif
		#endif
		#ifdef NR_SUPPORT_OLD_CLIENTS
			#ifndef DEFAULT_DONE
				if (realBounce) {
					hero_t testHero = src->hero[i];
					bool clientBounce = applyDefaultPhysics(&testHero, room, 1., player[i].item_speed, carryFlag, deathbringerAffected, false);
					//player bounced: play bounce sample if minimum time elapsed
					if (!clientBounce && get_time() > player[i].wall_sound_time || get_time() + 0.2 < player[i].wall_sound_time) {	// second test means w_s_t is invalid (since it is not initialized, ever)
						player[i].wall_sound_time = get_time() + 0.2;
						broadcast_screen_sample(i, SAMPLE_WALLBOUNCE);
					}
					else if (clientBounce)
						player[i].wall_sound_time = get_time() + 0.2;
				}
			#else
				#undef DEFAULT_DONE
				(void)realBounce;
			#endif
		#else
			(void)realBounce;
		#endif

		//check room change x
		if (int(hd->x) == plw) {
			hd->x = 1;
			if (++hd->tx >= map.w)
				hd->tx = 0;
		}
		else if (int(hd->x) == 0) {
			hd->x = plw - 1;
			if (--hd->tx < 0)
				hd->tx = map.w - 1;
		}

		//check room change y
		if (int(hd->y)-NR_SHIFTY == plh) {
			hd->y = 1 +NR_SHIFTY;
			if (++hd->ty >= map.h)
				hd->ty = 0;
		}
		else if (int(hd->y)-NR_SHIFTY == 0) {
			hd->y = plh - 1 +NR_SHIFTY;
			if (--hd->ty < 0)
				hd->ty = map.h - 1;
		}
	}

	//simulate and broadcast frame
	void simulate_and_broadcast_frame() {

		int i;

		//hack
		static unsigned long ticker = 0;
		ticker++;

		// (-1) check powerup respawn
		//
		double thetime = get_time();
		for (i=0;i<MAX_PICKUPS;i++)
		if (world.item[i].kind == 255)					//valid & respawning
		//if (world.item[i].respawning)		//respawning
		if (thetime > world.item[i].respawn_time) {

#ifdef REALLY_DEBUG_POWERUPS
			bprintf("time to respawn item %i\n", i);
#endif

			respawn_pickup(i);
		}

		// (0) do stuff for every player
		//
		if (!gameover)		//skip if game over
		for (i=0;i<maxplayers;i++)
		if (player[i].used) {

			//dec talk flood protect counter
			player[i].talk_temp -= 0.1;
			if (player[i].talk_temp < 0.0)
				player[i].talk_temp = 0.0;
			player[i].talk_hotness -= 0.1;
			if (player[i].talk_hotness < 1.0)
				player[i].talk_hotness = 1.0;

			//check frags changed
			if (player[i].oldfrags != player[i].frags) {
				//updated
				player[i].oldfrags = player[i].frags;

				//send frag update
				char lebuf[256]; int count = 0;
				writeByte(lebuf, count, 4);	//"4" = frags update
				writeByte(lebuf, count, i);		// what player id
				writeLong(lebuf, count, player[i].frags);
				server->broadcast_message(lebuf, count);
			}

			// check powerups expired
			//
			if (player[i].item_speed)
			if (get_time() > player[i].item_speed_time) {
				player[i].item_speed = false;
				broadcast_screen_sample(i, SAMPLE_BOOTS_OFF);
			}
			if (player[i].item_quad)
			if (get_time() > player[i].item_quad_time) {
				player[i].item_quad = false;
				broadcast_screen_sample(i, SAMPLE_QUAD_OFF);
			}
			if (player[i].item_helm)
			if (get_time() > player[i].item_helm_time) {
				player[i].item_helm = 0;
				broadcast_screen_sample(i, SAMPLE_HELM_OFF);
			}

			// helm alpha down
			//
			if (player[i].item_helm > 0) {
				player[i].item_helm -= 10;		//slowly fades....
				if (player[i].item_helm < shadow_minimum)	// minimum
					player[i].item_helm = shadow_minimum;
			}

			// check deathbringer effect
			//
			if (player[i].deathbringer_end > get_time()) {
				//check if still alive
				if (player[i].health > 0) {
					//has shield: do big damage to it, in order to remove the shield
					if (player[i].item_shield)
						game_damage_player(i, i, 12);	//12x0.1 = 120/sec = 600 5sec
					else
						game_damage_player(i, i, 3); // 3x0.1 = 30/sec = 150 5sec

					//dead? play "HA HA HA HA HA"
					if (player[i].health <= 0) {
						broadcast_screen_sample(i, SAMPLE_DIEDEATHBRINGER);

						//check if attacker still in game
						int a = player[i].deathbringer_attacker;

						if (player[a].used) {
							//give +1 frag
							score_frag(a, 1);
							player[a].total_kills++;
							player[i].total_deaths++;
							//reason broadcast
							bprintf("@I%s was choked by %s", player[i].name, player[a].name);
						}
					}
				}
			}

			// check for a player's deathbringer to bring death
			//
			if (player[i].health <= 0) //dead
			if (player[i].item_deathbringer)		//with deathbringer
			{
				//delta time since shoot
				double delta = (frame - player[i].item_deathbringer_time) * 0.1;
				//figure out new radius
				int rad;
				if (delta < 1.0)
					rad = (int)(delta * 100);
				else
					rad = 100 + (int)((delta - 1.0) * (delta - 1.0) * 800);

				//check enemy players onscreen that are not hit by it yet and are inside
				// the donut radius...radius-50
				for (int v=0;v<maxplayers;v++)
				if (v/TSIZE != i/TSIZE)		//enemy players only
				if (player[v].used)	//used
				if (player[v].health > 0)	//alive
				if (player[v].x == player[i].x)	// in the same screen of the deathbringer
				if (player[v].y == player[i].y)
				if (player[v].deathbringer_end < get_time())		// deathbringer fx end time -- not already hit?
				{
					//calculate player distance to the deathbringer core
					double ex = world.hero[i].x;
					double ey = world.hero[i].y - 15;
					double rx = world.hero[v].x;
					double ry = world.hero[v].y - 15;
					double dt = sqrt( (ex - rx)*(ex - rx) + (ey - ry)*(ey - ry) );

					// hit distance: if dt == rad, hit, if rad
					if ((rad <= dt + 20) && (rad >= dt - 60)) {

						//take his deathbringer off
						player[v].item_deathbringer = false;

						//hit sample
						broadcast_screen_sample(v, SAMPLE_HITDEATHBRINGER);

						//record the attacker
						player[v].deathbringer_attacker = i;

						//time of effect
						//also freeze his gun for this same amount of time
						player[v].deathbringer_end = player[v].next_shoot_time = get_time() + 4.5 + ((double)(rand() % 1000) / 1000.0);

						// calc recoil:
						double tx = world.hero[v].x - world.hero[i].x;		//triangle size x
						double ty = world.hero[v].y - world.hero[i].y;		//triangle size y

						// for lack of trigonometry knowledge, using this ugly hack
						if ((fabs(tx) >= 40.0) || (fabs(ty) >= 40.0)) { //too much
							//while one of them abs's is greater than 50, divide by some amount
							while (  ((fabs(tx) >= 40.0) || (fabs(ty) >= 40.0))  ) {
								tx *= 0.90;		//reduce
								ty *= 0.90;
							}
						}
						else { //too little
							//while both abses are smaller than 50, multiply by some amount
							while (  ((fabs(tx) <= 40.0) && (fabs(ty) <= 40.0))  ) {
								tx *= 1.10;		//increase
								ty *= 1.10;
							}
						}

						//set the new speed
						world.hero[v].sx = tx;
						world.hero[v].sy = ty;
					}

				}

			}

			// check for player weapons fire time
			//
			if (player[i].attack)	// player holding attack button
			if (player[i].health > 0)		// check if player alive
			if (get_time() > player[i].next_shoot_time)  // check if time allowed to fire again
			{
				//gasta 7 + 2 * tiros energy, se tem energy
				int numshots = 1;
				player[i].energy -= 7;			//gasta normal
				if (player[i].energy < 0)	//se ficou menor que zero, atira 1 so
					player[i].energy = 0;
				else {
					for (int k=1;k<player[i].weapon+1;k++) {
						//try add one shot
						player[i].energy -= 1;		//v0.4.7: diminuí METADE do gasto per shot!
						if (player[i].energy < 0)
							player[i].energy = 0;
						else
							numshots++;
					}
				}

				player[i].next_shoot_time = get_time() + 0.5;		// add minimum interval (in secs)

				//show helm
				if (player[i].item_helm > 0)
					player[i].item_helm = 255;

				//v0.1.2 shoot rocket
				game_shoot_rocket(
					i,						//player
					numshots,			//quantos tiros
					player[i].x,	//px
					player[i].y,	//py
					(int)world.hero[i].x,		//x
					(int)world.hero[i].y,		//y
					world.hero[i].gundir);	//direction
			}

		}


		// (1)  simulate (calculate) the next frame
		//

		// for each ROCKET, update position
		//
		if (!gameover)		//skip if game over
		for (i=0;i<MAX_ROCKETS;i++)
		if (world.rock[i].owner != -1)	//exists
		{
			rocket_c *rock = &(world.rock[i]);

			#if defined(NR_SERVER_PHYSICS) && defined(NR_SUPPORT_OLD_CLIENTS)
			bool NR_hit=false;
			#endif
			//run ten times for better collision accuracy (UGLY UGLY UGLY HACK)
			int t;
			for (t=0;t<10;t++)
			{
				//move-se
				rock->x += rock->sx / 10.0;
				rock->y += rock->sy / 10.0;

				//out of bounds
				if ((rock->x < -20) || (rock->y < -20) || (rock->x > plw + 20) || (rock->y > plh + 20)) {
					rock->owner = -1;	//just remove it. clients will figure out the same

					//broadcast_message("SE FOI");

					//2-loop break
					t=999;break;
				}

				//wall hit - remove
				#if !defined(NR_SERVER_PHYSICS) || defined(NR_SUPPORT_OLD_CLIENTS)
				if (map.fall_on_wall(rock->px, rock->py, (int)rock->x, (int)rock->y, (int)rock->x, (int)rock->y)) {
					rock->owner=-1;
					t=999;break;
				}
				#endif
				#ifdef NR_SERVER_PHYSICS
				if (map.fall_on_wall(rock->px, rock->py, (int)rock->x-2, (int)rock->y-NR_SHIFTY-2, (int)rock->x+2, (int)rock->y-NR_SHIFTY+2)) {
					#ifdef NR_SUPPORT_OLD_CLIENTS
					NR_hit=true;
					#else
					rock->owner=-1;
					t=999;
					break;
					#endif
				}
				#endif	// NR_SERVER_PHYSICS

				// check if a player (alive) is hit by this rocket now
				//
				//sqrt( (ex - x)*(ex - x) + (ey - y)*(ey - y) ). Acho que é isto...

				for (int p=0;p<maxplayers;p++)
				if (player[p].used)
				if (player[p].health > 0)		// alive
				if (rock->team != (p/TSIZE)) // shot is from opposing team
				if (rock->px == player[p].x) // in same screen
				if (rock->py == player[p].y)
				{
					//calculate distance rocket<->target center
					double ex = world.hero[p].x;
					double ey = world.hero[p].y - 15.0;
					double rx = rock->x;
					double ry = rock->y - 15.0;
					double dt = sqrt( (ex - rx)*(ex - rx) + (ey - ry)*(ey - ry) );

					//the number is the sum of the two balls bounding boxes radiuses (15 player + 3 rocket's)
					if (dt <= 18.0)
					{
						//record wether the player had shield, if yes, will not blink him
						bool had_shield = player[p].item_shield;

						//default damage to the target: 70
						int damage = 70;

						//v0.4.0: dano 50 se esta com o deathbringer
						if (player[rock->owner].item_deathbringer)
							damage = 50;

						//do damage
						game_damage_player(p, rock->owner, damage);

						//if player not dead, push him
						if (player[p].health > 0) {

							//se indo no sentido contrario, primeiro zera
							//(emulando comportamento v0.1.0)
							if (((world.hero[p].sx > 0) && (rock->sx < 0)) || ((world.hero[p].sx < 0) && (rock->sx > 0)))
								world.hero[p].sx = 0;
							if (((world.hero[p].sy > 0) && (rock->sy < 0)) || ((world.hero[p].sy < 0) && (rock->sy > 0)))
								world.hero[p].sy = 0;

							//adiciona velocidade do rocket/3
							world.hero[p].sx += rock->sx / 3.0;
							world.hero[p].sy += rock->sy / 3.0;
						}

						//delete shot
						//game_delete_rocket(i, t, p);
						if (had_shield)
							game_delete_rocket(i, (NLshort)rock->x, (NLshort)rock->y, 252);		//do not blink
						else
							game_delete_rocket(i, (NLshort)rock->x, (NLshort)rock->y, p);			//blink

						//2-loop break
						t=999;break;
					}
				}

			}
			#if defined(NR_SERVER_PHYSICS) && defined(NR_SUPPORT_OLD_CLIENTS)
			if (t==10 && NR_hit)	//# ugly fix to remove rockets inside walls under new physics, only when clients won't
				game_delete_rocket(i, (int)rock->x, (int)rock->y, 255);
			#endif
		}

		// for each player, update positions & speeds
		//
		hero_t  *h;

		if (!gameover)		//skip if game over
		for (i=0;i<maxplayers;i++)
		if (player[i].used) {
			h = &(world.hero[i]);

			//check if dead/respawn
			if (player[i].health <= 0) {

				if (player[i].respawn_time < get_time()) {
					game_respawn_player( !player[i].respawn_to_base , i);		//time to respawn player
				}

			}
			// player alive: do stuff for alive players
			else {

				// IN : copia player screen p/ hero screen
				h->tx = player[i].x;
				h->ty = player[i].y;

				// run server physics frame
				run_server_player_physics(i, &world, &world);	//player id, frame source, frame dest

				//OUT : copy screen information from hero back to player
				// (for bots)
				if ((player[i].x != h->tx) || (player[i].y != h->ty))
				{
					player[i].x = h->tx;
					player[i].y = h->ty;

					//player screen changed check
					game_player_screen_change(i);
				}


				//---------------------------------
				// player every-frame stuff
				//---------------------------------

				// check don't regen because of deathbringer
				//v0.4.0: do not regen if has deathbringer and both health/energy are at no less than 100
				bool deathbringer_penalty =
						((player[i].item_deathbringer) && (player[i].health >= 100) && (player[i].energy >= 100))			//rand() % 100 < 50
						||
						(player[i].deathbringer_end > get_time());

				// regen?
				if (!deathbringer_penalty) {

					// regenerate +1 health or +1 energy
					if (player[i].health < 100)
						player[i].health++;
					else {

						//caso energy > 100, regenera mais devagar (-33%)
						if (player[i].energy < 100)
							player[i].energy++;
						else if (player[i].energy < 200) {
							// 0.3.0: MAIS devagar
							//if (frame % 3)
							if (frame % 2)
								player[i].energy++;
						}
						//MEGA health vagarosamente...
						else if ((player[i].health < 200) && (frame %10 == 0))
							player[i].health++;
					}
				}

				//lose health & energy if running
				if (h->run) {

					if (player[i].energy <= 0) {

						//if (!player[i].item_speed)	// se ta com SPEED, faz nao hurt
						if (player[i].health > MIN_HEALTH_FOR_RUN_PENALTY) {	// se health > 30, desconta

							if (ticker % 2)
								player[i].health -= 2;	//desconta 2 (o normal)
							else
								player[i].health -= 1;	//desconta 1 (menos)

							if (player[i].health < MIN_HEALTH_FOR_RUN_PENALTY)		// garante minimo 30
								player[i].health = MIN_HEALTH_FOR_RUN_PENALTY;
						}

					} else {

						if (ticker % 2)
							player[i].energy -= 2; //desconta 2 (o normal)
						else
							player[i].energy -= 1; //desconta 1 (menos)

						if (player[i].energy == -1) { // special case

							player[i].energy++;

							if (player[i].health > MIN_HEALTH_FOR_RUN_PENALTY) {	// se health > 30, desconta

								player[i].health--;

								if (player[i].health < MIN_HEALTH_FOR_RUN_PENALTY)		// garante minimo 30
									player[i].health = MIN_HEALTH_FOR_RUN_PENALTY;
							}
						}
					}
				}

				//rot health to 100 if has deathbringer
				if ((player[i].item_deathbringer) && (player[i].health > 100) && (frame % 4 == 0))
					player[i].health--;

				//rot energy to 100 if has deathbringer
				if ((player[i].item_deathbringer) && (player[i].energy > 100) && (frame % 4 == 0))
					player[i].energy--;

				//megahealth bonus:
				if (player[i].megabonus > 0)
				if ((player[i].health == 300) && (player[i].energy == 300))
					player[i].megabonus--;	// realiza um certo "guardamento" de energia mas nao muito...
				else
				for (int mh=0;mh<5;mh++) {

						//+health
						if (player[i].megabonus > 0)
						if (player[i].health < 300) {
								player[i].health++;
								player[i].megabonus--;
						}

						//+energy
						if (player[i].megabonus > 0)
						if (player[i].energy < 300) {
								player[i].energy++;
								player[i].megabonus--;
						}
				}


				//limit health 0 .. 300
				if (player[i].health < 0)
					player[i].health = 0;
				else if (player[i].health > 300)
					player[i].health = 300;

				//limit energy 0 .. 300
				if (player[i].energy < 0)
					player[i].energy = 0;
				else if (player[i].energy > 300)
					player[i].energy = 300;

				//---------------------------------
				// check game object collisions
				//---------------------------------

				int myteam = i/TSIZE;
				int enemyteam = 1 - myteam;

				// --> ITEM PICKUP
				//
				int prad = 10;	//pickup item radius

				for (int k=0;k<MAX_PICKUPS;k++)
				if (world.item[k].kind > 0)	//valid item
				if (world.item[k].kind != 255) // not respawning
				if (world.item[k].px == player[i].x)		// player's screen
				if (world.item[k].py == player[i].y)
				//x,y == center of powerup!
				if (world.item[k].x + prad > world.hero[i].x - 20)
				if (world.item[k].x - prad < world.hero[i].x + 20)
				if (world.item[k].y + prad > world.hero[i].y - 20 - 10)
				if (world.item[k].y - prad < world.hero[i].y + 20 - 10)
				{
					//pick pickup
					game_touch_pickup(i, k);		//COOL!
				}

				// --> CTF FLAG STEAL touch other team's flag
				//
				if (!world.flag[enemyteam].carried &&	// enemy flag dropped (at base or somewhere)
					check_flag_touch(i, player[i].x, player[i].y, (int)h->x, (int)h->y, enemyteam))  // and I touch it
				{
					// Has player just dropped the flag or not?
					if (!player[i].dropped_flag) {
						//v0.4.7: update grab time (to detect degenerated maps) if flag was at base
						if (world.flag[enemyteam].atbase)
							world.flag[enemyteam].grab_time = get_time();

						//FLAG STOLEN!
						score_frag(i, 1);	// just add some frags

						player[i].total_flags_taken++;

						bprintf("@I%s GOT THE %s FLAG!", player[i].name, teamname[enemyteam]);

						ctf_steal_flag(enemyteam, i);  //flag stolen!

						//HELM powerup: show player
						if (player[i].item_helm > 0)
							player[i].item_helm = 255;
					}
				}
				else	// Player has removed away from the flag.
					player[i].dropped_flag = false;

				// --> CTF FLAG RETURN
				//
				if (!world.flag[myteam].carried)	// my flag dropped
				if (!world.flag[myteam].atbase)	// not at base
				if (check_flag_touch(i, player[i].x, player[i].y, (int)h->x, (int)h->y, myteam))  // and I touch it
				{
					//FLAG RETURNED!
					score_frag(i, 1);	// just add some frags
					player[i].total_flags_returned++;

					bprintf("@I%s RETURNED THE %s FLAG!", player[i].name, teamname[myteam]);

					ctf_return_flag(myteam);  //flag returned

					//sound broadcast
					broadcast_sample(SAMPLE_CTF_RETURN);
				}

				// --> CTF FLAG CAPTURE
				//
				if (world.flag[enemyteam].carried)		// enemy flag carried
				if (world.flag[enemyteam].carrier == i)	// by me
				if (!world.flag[myteam].carried)	// my flag dropped
				if (world.flag[myteam].atbase)	// at my base
				if (check_flag_touch(i, player[i].x, player[i].y, (int)h->x, (int)h->y, myteam))		// I touch my flag
				{
					//v0.4.7: detect degenerated maps
					if (map.valid_for_scoring)		//still valid?
					if (get_time() - world.flag[enemyteam].grab_time <= MINIMUM_GRAB_TO_CAPTURE_TIME) {

						//this map is bogus, ignore all scoring for it.
						map.valid_for_scoring = false;
						//tell people...
						broadcast_message("@WThis map is too small. Scoring for World Ranking disabled.");
						//zero all delta scores so far
						for (int p=0;p<MAX_PLAYERS;p++) {
							client[p].delta_score = 0;
							client[p].neg_delta_score = 0;		//V0.4.8
							client[p].fdp = 0.0;
							client[p].fdn = 0.0;		//V0.4.8
						}
					}

					//add frags to all players of the team

					// V0.4.8: PENALIZE every player of the other team

					for (int h=0;h<MAX_PLAYERS;h++)
					if (player[h].used)
					if ((h/TSIZE) == myteam)
						score_frag(h, 2);				//small two-frag bonus
					else
						score_neg(h, 1);		//v0.4.8 : small NEG POINT penalty for YOUR FLAG BEING CAPTURED

					//CHANGED 0.4.8: add +3 extra frags to the capturer (for a total of 5)
					score_frag(i, 3);

					//return enemy flag to their base
					ctf_return_flag(enemyteam);

					//message
					string one_more;
					if (world.flag[myteam].score == capture_limit - 2) // points update later
						one_more = " One more to win!";
					bprintf("@I%s CAPTURED THE %s FLAG!%s", player[i].name, teamname[enemyteam], one_more.c_str());

					//count
					player[i].total_captures++;

					//update the ADMIN SHELL
					if (!player[i].isbot) {
						char lebuf[256]; int count; NLint result;
						count = 0;
						writeLong(lebuf, count, STA_PLAYER_CAPTURES);
						writeLong(lebuf, count, player[i].cid);
						result = nlWrite(shellssock, lebuf, count);
					}

					//CAPTURE (team count ++)
					world.flag[myteam].score++;
					ctf_update_teamscore(myteam);		//this function can decide to restart the game . (?)

					//sound broadcast
					broadcast_sample(SAMPLE_CTF_CAPTURE);
				}

			} // player.health > 0

		}

		// announce voting status
		if (frame >= next_vote_announce_frame) {
			int votes=0, players=0;
			for (int i=0; i<maxplayers; ++i)
				if (player[i].used && !player[i].isbot) {
					++players;
					if (player[i].want_map_exit)
						++votes;
				}
			players=players/2+1;
			if (votes!=last_vote_announce_votes || (players!=last_vote_announce_needed && votes!=0)) {
				last_vote_announce_votes=votes;
				last_vote_announce_needed=players;
				next_vote_announce_frame=frame+NR_VOTE_ANNOUNCE_INTERVAL*10;
				ostringstream voteinfo;
				voteinfo << "@I*** " << votes << '/' << players << " votes for mapchange";
				if (map_start_time+vote_block_time >= frame)
					voteinfo << " (all players needed for " << (map_start_time+vote_block_time-frame+5)/10 << " more seconds)";
				broadcast_message(voteinfo.str().c_str());
			}
		}

		// player maintenance (check delayed messages etc)
		int players = 0;
		for (int i=0; i<maxplayers; ++i)
			if (player[i].used) {
				++players;
				if (!player[i].isbot) {
					if (player[i].kickTimer) {
						--player[i].kickTimer;
						if (player[i].kickTimer==0)
							server->disconnect_client(player[i].cid, 1);	// 1 second timeout
						else if (player[i].kickTimer%10 == 0 && player[i].kickTimer<=50)
							plprintf(i, "@WDisconnecting in %d...", player[i].kickTimer/10);
					}
					else {
						player_t::DMQueueT& dm=player[i].delayedMessages;
						while (dm.size() && --dm.begin()->first<0) {
							player_message(i, dm.begin()->second.c_str());
							dm.erase(dm.begin());
						}
					}
				}
			}

		// check timelimit
		if (players > 1 && time_limit > 0) {
			if (time_limit >= 10*60 * 10 && frame - map_start_time == time_limit - 5*60 * 10)
				bprintf("@I*** Five minutes left in the game");
			if (time_limit >= 2*60 * 10 && frame - map_start_time == time_limit - 60 * 10)
				bprintf("@I*** One minute left in the game");
			else if (time_limit >= 60 * 10 && frame - map_start_time == time_limit - 30 * 10)
				bprintf("@I*** 30 seconds left in the game");
			else if (frame - map_start_time > time_limit) {
				bprintf("@I*** Time out - CTF game over");
				server_next_map(NEXTMAP_CAPTURE_LIMIT);
				ctf_game_restart();
			}
		}

		// (2)  broadcast the frame
		//
		//		o pacote nao eh o mesmo pra todo mundo, entao nao eh broadcast
		//		m uma parte que depende do player (tipo, qual o health do cara)
		//
		// server frame format:  (protocolo v0.4.1)
		//
		//  --- PRIMEIRA PARTE : igual pra todo mundo ----
		//
		//    LONG  frame
		//		LONG  players present (bits 0..31 dizendo quais players[] sao validos)
		//
		// --- SEGUNDA PARTE : varia p/ cada cliente -----
		//
		//		BYTE xtra   (bitfield)
		//       0  health extra bit (+256)
		//       1  energy extra bit (+256)
		//       2  SKIP FRAME : no more frame data (depois desse byte)
		//       3..7   "me" (0..31)
		//    BYTE player screen(room) x
		//    BYTE player screen(room) y
		//    LONG players onscreen (bits 0..31 dizendo quais players[] estao na mesma room que eu)
		//
		//    ** E PARA CADA "PLAYER ONSCREEN", na ordem do bitfield, de 0 a 31:
		//       3 BYTES   x e y
		//       2 BYTES   sx e sy
		//       BYTE   extra (bitfield)
		//         0   player dead?
		//         1   has deathbringer?
		//         2   affected by deathbringer?
		//         3   has shield?
		//         4   has speed?
		//         5   has quad?
		//         6..7   FREE BITS
		//       BYTE   keys (aceleracao/bitfield)
		//           0   left?
		//           1   right?
		//           2   up?
		//           3   down?
		//           4   running? (SHIFT)
		//           5..7   gundir  (direcao em que esta mirando)
		//       BYTE   shadow alpha level
		//
		//    SHORT inimigos visiveis (0..15)  (bitfield)
		//    BYTE  indice "V" do jogador que eu vou ficar sabendo agora (0-31)
		//    BYTE  minimap x do player V
		//    BYTE  minimap y do player V
		//    BYTE  health base do jogador (primeiros 8 bits)
		//    BYTE  energy base do jogador (primeiros 8 bits)
		//    SHORT ping do jogador : player[frame % maxplayers].ping;
		//

		//RECALC PLAYERS PRESENT EVERY TIME
		NLulong players_present = 0;
		for (int pp=0;pp<maxplayers;pp++)
		if (player[pp].used)
			players_present += (1 << pp);


		// ============================
		//   build common data buffer
		// ============================
		char lebuf[4096];		//common frame data
		int count = 0;

		//frame
		writeLong(lebuf, count, frame);

		//players present NEW: LONG
		writeLong(lebuf, count, players_present);

		//===============================
		//  build packet for each client
		//		with custom data
		//===============================
		int lecount;	//count after "count"

		//stuff for minimap update: my team's enemy team view
		static int tviter[2] = { 0 , 0 };
		static int helmiter = 0;		// HELM ITERATOR : manda todo mundo!
		int tview[2][MAX_PLAYERS/2];		//[time][inimigo# visto? 1-8]
		NLushort	tview_bits[2];	//enemy view SHORT (bitfield for the 8 enemies of each team(0,1))

		//HELM PATCH: the "helm view" bytes for both teams - if somebody has helm, he will se
		//  helmview[] for his team
		NLushort helmview[2];

		//atualiza HELM ITERATOR - para em um player valido ou entao qualquer um
		int runaway = maxplayers + 1;
		do {
			helmiter++;
			if (helmiter > maxplayers - 1)
				helmiter = 0;
			if (player[helmiter].used)
				//fix: helm nao enxerga outros helms, a nao ser com a flag
				//ou seja: so mostra (break) se:  NAO TEM HELM   ou   TEM FLAG
				if ((player[helmiter].item_helm == 0) || ((world.flag[1-helmiter/TSIZE].carried) && (world.flag[1-helmiter/TSIZE].carrier == helmiter)))
					break;
		} while (runaway-- > 0);

		int t;

		//atualiza tview E HELMVIEW
		for (t=0;t<2;t++) {		// p/ cada time

			tview_bits[t] = 0;

			helmview[t] = 0;		//default zero

			for (int i=0;i<maxplayers;i++)			// p/ cada inimigo desse time
			if (i/TSIZE == 1-t)
			if (player[i].used)
			{
				// ---- helmview -----
				// mostra se NAO TEM HELM ou SE TA COM FLAG
				if ((player[i].item_helm == 0) || ((world.flag[1-i/TSIZE].carried) && (world.flag[1-i/TSIZE].carrier == i))) {
					//adiciona bit
					//helmview[t] += ((NLushort) (1 << (i%TSIZE)));
					// FUCKING TYPE CONVERSION HELL
					int fuck = helmview[t];
					fuck += (1 << (i%TSIZE));
					helmview[t] = (NLushort)fuck;
				}

				// ---- tview -----
				tview[t][i] = 0;		// default = nao visto

				for (int j=0;j<maxplayers;j++)			// verifica se ele esta no campo de visao (tela) de alguem do meu time
				if (j/TSIZE == t)
				if (player[j].used)
				{
					if ((player[j].x == player[i].x) && (player[j].y == player[i].y))	{

						//se o cara tem helm E NAO TEM FLAG, nao aparece!!
						if ((player[i].item_helm > 0) && ((world.flag[1-i/TSIZE].carried == false) || (world.flag[1-i/TSIZE].carrier != i))) {
							//nao visto!
						}
						else {
							//visto!
							tview[t][i%TSIZE] = 1;	//visto!
							//tview_bits[t] += ((NLushort) (1 << (i%TSIZE)));		//seta bit de "visto"
							// FUCKING TYPE CONVERSION HELL
							int fuck = tview_bits[t];
							fuck += (1 << (i%TSIZE));
							tview_bits[t] = (NLushort)fuck;
							break;
						}
					}
				}
			}

			//avanca tviter do time p/ escolher alguem
			int runaway = maxplayers + 3;
			do {
				//avanca proximo candidato a envio
				tviter[t]++;
				if (tviter[t] < 0)
					tviter[t] = 0;
				if (tviter[t] >= maxplayers)
					tviter[t] = 0;

				//testa se o candidato se aplica ao visor minimap do time
				//testa apenas used players
				if (player[tviter[t]].used) {

					//do meu time? envia, tenho q saber todos do meu time
					if (tviter[t]/TSIZE == t)
						break;

					//inimigo? so se estiver na visao do time
					if (tviter[t]/TSIZE == 1-t)
					if (tview[t][ tviter[t]%TSIZE] == 1)
						break;
				}

			} while (runaway-- > 0);
		}

		// ==================================================================
		//   BUILD AND SEND EVERY DAMN PACKET
		// ==================================================================
		for (i=0;i<maxplayers;i++)
		if (player[i].used)			// player valido (used)
		if (!player[i].isbot)		// BOTZ: para bots, nao faz o "BROADCAST" do simulate_and_broadcast...
		{
			//rewrite past common data
			lecount = count;

			//extra byte of information
			// BIT 0: extra health
			// BIT 1: extra energy
			// BIT 2  ( VERY IMPORTANT ) : player not ready bit (envia frame vazio!) OU server exibindo placa "game over"
			NLubyte xtra = 0;
			if (player[i].health & 256) xtra += 1;
			if (player[i].energy & 256) xtra += 2;
			// BITS 3..8 == what player id
			if (i & 1) xtra += 8;
			if (i & 2) xtra += 16;
			if (i & 4) xtra += 32;
			if (i & 8) xtra += 64;
			if (i & 16) xtra += 128;

			bool skip_frame = (player[i].awaiting_client_ready) || ((gameover) && (gameover_time > get_time()));

			if (skip_frame) xtra += 4;		// VERY IMPORTANT
			writeByte(lebuf, lecount, xtra);

			//****** VERY IMPORTANT ******
			// send almost empty frame if client not ready (leave bandwidth for data transfer) OR IF
			// server showing gameover plaque
			if (!skip_frame) {

				// NEW: 0.3.9 : send before players_onscreen 2 bytes with the screen of self
				//
				NLubyte scr;
				scr = ((NLubyte)player[i].x);
				writeByte(lebuf, lecount, scr);	//player.x (screen)
				scr = ((NLubyte)player[i].y);
				writeByte(lebuf, lecount, scr);	//player.y (screen)

				//player data field for each player ON SCREEN
				NLulong		players_onscreen = 0;

				//keep place for players_onscreen
				int p_on_count = lecount;
				writeLong(lebuf, lecount, 0);

				NLubyte keys;
				for (int j=0;j<maxplayers;j++)
				if ((players_present & (1 << j)) != 0)		//player j exists
				// j is on same screen than i (the viewer)
				// AND
				//   ((j helm != 1)  ||  (j/TSIZE == i/TSIZE))  // nao-totalmente invisivel OU e do mesmo time OU j com flag
				if (
						(player[j].x == player[i].x)
						&&
						(player[j].y == player[i].y)
						&&
						(player[j].item_helm != 1 || i/TSIZE == j/TSIZE ||
								(world.flag[1-j/TSIZE].carried && world.flag[1-j/TSIZE].carrier == j)) ) {
					//add to players_onscreen
					players_onscreen += (1 << j);

					h = &(world.hero[j]);

//					NLshort sho;

					//V0.3.9: took out screen from here

					//V0.3.9 : transmissao x,y de 4 bytes para 3
					NLubyte xy;
					NLushort hx,hy;
					hx = ((NLushort)h->x);
					hy = ((NLushort)h->y);

					xy = (NLubyte) (hx & 255);
					writeByte(lebuf, lecount, xy);		//first 8 bits x
					xy = (NLubyte) (hy & 255);
					writeByte(lebuf, lecount, xy);		//first 8 bits y
					//256+512+1024+2048 = 3840    last 4 bits mask
					xy = (NLubyte) ( ((hx & 3840) >> 8) + ((hy & 3840) >> 4) ); //x: bit 8-11 to 0-3  y: bit 8-11 to 4-7
					writeByte(lebuf, lecount, xy);   //last 4 bits x + last 4 bits y

					//sho = ((NLshort)h->x);
					//writeShort(lebuf, lecount, sho);	//x
					//sho = ((NLshort)h->y);
					//writeShort(lebuf, lecount, sho);	//y

					//speed em bytes - xinelao mesmo
					NLbyte sxy;
					sxy = ((NLbyte)(h->sx * 2));
					writeByte(lebuf, lecount, sxy);
					sxy = ((NLbyte)(h->sy * 2));
					writeByte(lebuf, lecount, sxy);

					//sho = ((NLshort)(h->sx * 100));
					//writeShort(lebuf, lecount, sho );	//sx  30.283482345634... = 30283 = 30.283(depois)
					//sho = ((NLshort)(h->sy * 100));
					//writeShort(lebuf, lecount, sho );	//sy

					/*
					EXTRA BYTE (ex- zframe)

					bit 0 : player dead
					bit 1 : has deathbringer
					bit 2 : deathbringer-affected
					*/
					NLubyte extra = 0;
					if (player[j].health <= 0) extra += 1; //deadflag
					if (player[j].item_deathbringer) extra += 2; //has deathbringer
					if (player[j].deathbringer_end > get_time()) extra += 4;		//deathbringer-affected
					// ITEMS: moved to this byte
					if (player[j].item_shield)		extra += 8;
					if (player[j].item_speed)			extra += 16;
					if (player[j].item_quad)			extra += 32;

					//write extra byte
					writeByte(lebuf, lecount, extra);

					//keys
					keys = 0;

					//set bits 0..3
					// if dead player, don't send keys
					if (player[j].health > 0) {
						if (h->l) keys += 1;
						if (h->r) keys += 2;
						if (h->u) keys += 4;
						if (h->d) keys += 8;
						if (h->run) keys += 16;		//bit 4 is "running" flag (max-speed modifier for the clientside movement extrapolator)
					}

					//set bits 5..7
					//keys = keys + ((NLubyte)(h->gundir * 32));
					// FUCK NONIMPLICIT TYPE CONVERSION AND FUCK ANSI-C
					int fuck = keys;
					fuck += h->gundir * 32;
					keys = (NLubyte)fuck;

					//write keys
					writeByte(lebuf, lecount, keys);

					//write SHIELD  BOOTS / QUAD
					//NLubyte		itemflags = 0;
					//if (player[j].item_shield)		itemflags += 1;
					//if (player[j].item_speed)			itemflags += 2;
					//if (player[j].item_quad)			itemflags += 4;
					//writeByte(lebuf, lecount, itemflags);

					//write HELM alpha level
					writeByte(lebuf, lecount, ((NLubyte)player[j].item_helm));
				}

				//update players_onscreen (it's before the players on screen data (above))
				writeLong(lebuf, p_on_count, players_onscreen);

				//ELMO: visao alem do alcance!!
				NLubyte who;
				if (player[i].item_helm > 0) {

					//team "viewed enemies" do meu time (i/TSIZE)
					//writeByte(lebuf, lecount, 255);		// todos!!!
					//FIX: helm nao enxerga todo mundo nao
					//team "viewed enemies" do meu time (i/TSIZE)
					//writeByte(lebuf, lecount, tview_bits[i/TSIZE]);
					//FIX: helm tambem nao eh no viewed enemies. o helm de um time é 255 (todos)
					//     menos quem tem helm
					writeShort(lebuf, lecount, helmview[i/TSIZE]);

					//"quem eu vou ficar sabendo no minimap agora?" -- do time
					who = (NLubyte)helmiter;
					writeByte(lebuf, lecount, who);
				}
				//sem elmo: visao normal
				else {

					//team "viewed enemies" do meu time (i/TSIZE)
					writeShort(lebuf, lecount, tview_bits[i/TSIZE]);

					//"quem eu vou ficar sabendo no minimap agora?" -- do time
					who = (NLubyte)tviter[i/TSIZE];
					writeByte(lebuf, lecount, who);
				}

				//x do cara, 0..255 (%) do mundo
				NLubyte mx = (NLubyte)(((world.hero[who].x + ((double)(player[who].x * plw))) / (map.w*plw)) * 255.0);
				writeByte(lebuf, lecount, mx);

				//y do cara, 0..255 (%) do mundo
				NLubyte my = (NLubyte)(((world.hero[who].y + ((double)(player[who].y * plh))) / (map.h*plh)) * 255.0);
				writeByte(lebuf, lecount, my);

				//send player's BASE health (first 8 bits)
				if (player[i].health < 0) player[i].health = 0;
				writeByte(lebuf, lecount, ((NLubyte)(player[i].health & 255)));

				//send player's BASE energy (first 8 bits)
				if (player[i].energy < 0) player[i].energy = 0;
				writeByte(lebuf, lecount, ((NLubyte)(player[i].energy & 255)));

				//ping of player frame# % MAXPLAYERS
				NLushort theping = (NLushort) player[frame % maxplayers].ping;
				writeShort(lebuf, lecount, theping);

			}//!player[i].awaiting_client_ready

			//send the packet
			server->send_frame(player[i].cid, lebuf, lecount);	//use client id of the player, and LEcount
		}

		// PING: v0.4.1
		// envia um ping a cada frame, faz revezamento entre todos os players
		ping_send_client++;		//next player
		if (ping_send_client >= maxplayers)
			ping_send_client = 0;
		if (player[ping_send_client].used) // valid player?
		if (!player[ping_send_client].isbot)	//NOT a bot?
			server->ping_client(player[ping_send_client].cid); //ping
	}

	//run something after simulate_and_broadcast
	void server_think_after_broadcast() {
		int i;

		//check players with pending team changes
		for (i=0;i<maxplayers;i++)
		if (player[i].used)
		if (!player[i].isbot)
		if (player[i].team_change_pending)
		if (player[i].want_change_teams)
		if (player[i].team_change_time < get_time())
			check_player_change_teams(i);

		//check end of gameover plaque
		if (gameover)
		if (gameover_time < get_time()) {
			gameover = false;
			char lebuf[256]; int count = 0;
			writeByte(lebuf, count, 25);		//end of gameover plaque
			server->broadcast_message(lebuf, count);
		}
	}

	//loop server
	// running_flag: pointer to bool, if this bool goes to false, the loop quits.
	void loop(volatile bool *running_flag) {

		LOG("GAMESERVER::LOOP()\n");

		frame = 0;	//frame to generate next

		//sync with speed counter until it's time to generate one frame (== 1)
		server_speed_counter = -3;
		while (server_speed_counter < 1)
			MS_SLEEP(1);		//*** NO CPU PROBLEM HERE ***

		// no flag specified: esc quits
		bool keep_running = true;
		if (!running_flag)
			running_flag = &keep_running;

		static int fubie = 0;

		LOG("GAMESERVER::LOOP() (2)\n");

		while ( (*running_flag) == true )	{

			// generate and send frame
			simulate_and_broadcast_frame();

			//dec counter - another 100ms must pass before next send
			server_speed_counter--;

			// next frame
			frame++;

			//update dedserver wintitle
			if (fubie++ > 10) {

				server_kbps_traffic =
					server->get_socket_stat(NL_AVE_BYTES_RECEIVED) +
					server->get_socket_stat(NL_AVE_BYTES_SENT);
				server_kbps_traffic /= 1024.0;

				//update bar
				fubie = 0;
				char elbuf[128];
				sprintf(elbuf, "%i/%ip %.1fk/s v%s port:%i ESC:quit", player_count, maxplayers, server_kbps_traffic, GAME_VERSION, port);

				//V0.5.0 : -text  flag
				server_status_string(elbuf);
			}

			// executa algo para todos os players
			server_think_after_broadcast();

			// BOTZ: apos a simulacao da fisica da frame e envio de frame update pra todos os jogadores de
			//   carne e osso, segue-se um periodo de dormencia do server, em torno de 100 ms (menos, porque
			//   o simulate/broadcast frame tem seu custo)
			//
			//   ANTES desta sonolencia, ha um tempo para que os bots "pensem" que teclas vao apertar.
			//   fabuloso, nao?
			//
			//   todos os bots tem que atualizar seus keypresses lá no bot_frame_think()
			//
#ifndef	NO_BOTS

			bot_before_frame();		// roda algo sobre bots, só que antes de todos os bots pensarem

			int bi;
			for (bi=0;bi<maxplayers;bi++)
				if (player[bi].used)		// bá, muito importante! quase esqueco....
				if (player[bi].isbot)		// é bot? nao avacalhar teclas dos outros...
					bot_frame_think(bi);

#endif

			// sleep while not time to send again
			while (server_speed_counter <= 0) {

				// sleep a bit
				MS_SLEEP(2);			// *** OPTIMIZE THIS ***
			}

			// quit? if no running-flag specified
			if (key[KEY_ESC])
				keep_running = false;
		}

		LOG("GAMESERVER::LOOP() (EXITING!)\n");
	}

	//a master job response is obtained: parse it
	void master_job_response(masterjob_c *j) {

		//LOG4("== MJOB RESPONSE : %i %i %i '''%s'''\n", j->code, j->cid, j->html_end, j->lebuf);

		int i;

		//1 -- player token check
		// RETORNO ESPERADO : @K<SCORE>#<RANKPOS>#

		bool got_a_final_response = false;		//if got a final response (@F/@E/@K)
																					//if not, will retry (e.g. getting "can't contact servlet runner"
																					//or other pages describing temporary problems in the master server)

		if (j->code == 1) {

			//parse response
			for (i=0;i<j->n;i++) {

				if (j->lebuf[i] == '@') {
					i++;

					if ((j->lebuf[i] == 'F') || (j->lebuf[i] == 'E')) {

						//FIXME: this is bad news -- deal better with it
						got_a_final_response = true;

					}
					if (j->lebuf[i] == 'K') {

						got_a_final_response = true;

						//OK!
						char lebuf[128]; int count = 0;
						writeByte(lebuf, count, 31);		//31 = registration NAME,TOKEN response
						writeByte(lebuf, count, 1);		// OK!
						server->send_message(j->cid, lebuf, count);

						//set this player as being recorded as of now
						client[j->cid].token_valid = true;		//validated his token

						//PARSE STUFF
						char pb[256];
						int  pc;
						//PARSE: current score
						pb[0]=0;
						pc=0;
						i++;
						while (j->lebuf[i] != '#') {
							pb[pc] = j->lebuf[i];
							pb[pc+1] = 0;
							pc++;
							i++;
							if (pc > 15) break;	//improbable length
						}
						client[j->cid].score = atoi(pb);
						//PARSE: current NEG score   // V0.4.8 ===
						pb[0]=0;
						pc=0;
						i++;
						while (j->lebuf[i] != '#') {
							pb[pc] = j->lebuf[i];
							pb[pc+1] = 0;
							pc++;
							i++;
							if (pc > 15) break;	//improbable length
						}
						client[j->cid].neg_score = atoi(pb);
						//LOG1("=== parsed SCORE '%s'\n", pb);
						//PARSE: current ranking pos
						pb[0]=0;
						pc=0;
						i++;
						while (j->lebuf[i] != '#') {
							pb[pc] = j->lebuf[i];
							pb[pc+1] = 0;
							pc++;
							i++;
							if (pc > 15) break;	//improbable length
						}
						client[j->cid].rank = atoi(pb);
						//LOG1("=== parsed RANKPOS '%s'\n", pb);
						//PARSE: max score
						pb[0]=0;
						pc=0;
						i++;
						while (j->lebuf[i] != '#') {
							pb[pc] = j->lebuf[i];
							pb[pc+1] = 0;
							pc++;
							i++;
							if (pc > 15) break;	//improbable length
						}
						max_world_score = atoi(pb);
						//PARSE: max rank pos
						pb[0]=0;
						pc=0;
						i++;
						while (j->lebuf[i] != '#') {
							pb[pc] = j->lebuf[i];
							pb[pc+1] = 0;
							pc++;
							i++;
							if (pc > 15) break;	//improbable length
						}
						max_world_rank = atoi(pb);

						//v0.4.5 : atualiza registration char / score / rank
						broadcast_player_crap( ctop [j->cid] );

						//done
						break;
					}
					else if (j->lebuf[i] == 'F') {
						//FAILED!
						char lebuf[128]; int count = 0;
						writeByte(lebuf, count, 31);		//31 = registration NAME,TOKEN response
						writeByte(lebuf, count, 0);		// FAILED!
						server->send_message(j->cid, lebuf, count);

						//done
						break;
					}
				}
			}
		}
		//2 -- submit a player's report
		// RETORNO ESPERADO : @K<SCORE>#<RANKPOS>#  (atualizados...)
		else if (j->code == 2) {
			//parse response
			for (i=0;i<j->n;i++) {

				if (j->lebuf[i] == '@') {
					i++;
					if ((j->lebuf[i] == 'F') || (j->lebuf[i] == 'E')) {

						//FIXME: this is bad news -- deal better with it
						got_a_final_response = true;

					}
					if (j->lebuf[i] == 'K') {

						got_a_final_response = true;

						//deltascore report: just update score/ranking

						//PARSE STUFF
						char pb[256];
						int  pc;
						//PARSE: current score
						pb[0]=0;
						pc=0;
						i++;
						while (j->lebuf[i] != '#') {
							pb[pc] = j->lebuf[i];
							pb[pc+1] = 0;
							pc++;
							i++;
							if (pc > 15) break;	//improbable length
						}
						client[j->cid].score = atoi(pb);
						//PARSE: current NEG score  v0.4.8 !! ======
						pb[0]=0;
						pc=0;
						i++;
						while (j->lebuf[i] != '#') {
							pb[pc] = j->lebuf[i];
							pb[pc+1] = 0;
							pc++;
							i++;
							if (pc > 15) break;	//improbable length
						}
						client[j->cid].neg_score = atoi(pb);
						//LOG1("=== parsed SCORE '%s'\n", pb);
						//PARSE: current ranking pos
						pb[0]=0;
						pc=0;
						i++;
						while (j->lebuf[i] != '#') {
							pb[pc] = j->lebuf[i];
							pb[pc+1] = 0;
							pc++;
							i++;
							if (pc > 15) break;	//improbable length
						}
						client[j->cid].rank = atoi(pb);
						//LOG1("=== parsed RANKPOS '%s'\n", pb);
						//PARSE: max score
						pb[0]=0;
						pc=0;
						i++;
						while (j->lebuf[i] != '#') {
							pb[pc] = j->lebuf[i];
							pb[pc+1] = 0;
							pc++;
							i++;
							if (pc > 15) break;	//improbable length
						}
						max_world_score = atoi(pb);
						//PARSE: max rank pos
						pb[0]=0;
						pc=0;
						i++;
						while (j->lebuf[i] != '#') {
							pb[pc] = j->lebuf[i];
							pb[pc+1] = 0;
							pc++;
							i++;
							if (pc > 15) break;	//improbable length
						}
						max_world_rank = atoi(pb);

						//v0.4.5 : atualiza registration char / score / rank
						broadcast_player_crap( ctop [j->cid] );

						//done
						break;
					}
					else if (j->lebuf[i] == 'F') {

						//FIXME: something went wrong, DELTA SCORE LOST !!!
						//  log this, must check reasons later
						LOG("ERROR : A PLAYER DELTA SCORE UPDATE HAS BEEN LOST! (@F returned)!\n");

						break;
					}
				}
			}
		}

		//no definitive response: should retry
		if (got_a_final_response == false)
			j->retry = true;
	}

	//master job -- handle a single request
	void run_masterjob_thread(void *arg) {

		//increment job count FIXME: this shouldn't be here really... must place it BEFORE
		//creating the thread
		pthread_mutex_lock ( &mjob_mutex );
		mjob_count++;
		pthread_mutex_unlock ( &mjob_mutex );

		//the rune
		masterjob_c *job = (masterjob_c*)arg;

		int w; //wait

		NLsocket sock = NL_INVALID;

		while (mjob_exit == false) {

			//open a nonblocking socket
			nlDisable(NL_BLOCKING_IO);
			sock = nlOpen(0, NL_RELIABLE);
			if (sock == NL_INVALID) {
				//FIXME show "cant open socket to master" error
				for (w=0;w<60*2*2;w++) { if (mjob_exit) break; if (mjob_fastretry) break; MS_SLEEP(500); }
				continue;				//again...
			}

			//connect the nonblocking way
			nlConnect(sock, &master_address);

			//build query
			char querybuf[1024]; int qcount = 0;
			writeString(querybuf, qcount, job->request);
			qcount--;	//take the zero out

			//FIXME: LOG PROGRESS

			//keep trying to write the query.
			NLint result;
			do {
				result = nlWrite(sock, querybuf, qcount);
				MS_SLEEP(50);

				//qutting?
				if (mjob_exit) break;

			} while ((result == NL_INVALID) && (nlGetError() == NL_CON_PENDING));

			//qutting?
			if (mjob_exit) continue;

			//FIXME: LOG PROGRESS

			//try to read the reply
			//parse the response (should be <HTML><BODY> etc... with "@I @I @I ... @K" on it
			int nostuffcound = 0;
			int n = 0;
			char *lebuf = &(job->lebuf[0]);
			do {

				//read
				result = nlRead(sock, &(lebuf[n]), 1);

				//quitting?
				if (mjob_exit) break;

				//no byte
				if (result == 0) {

					if (nostuffcound > 0) {
						nostuffcound++;
						//200 (4000*50/1000) seconds after it came some stuff but now without coming more stuff
						// THEN: retry in a while
						if (nostuffcound > 4000) {
							//retry
							lebuf[0] = 0;
							nlClose(sock);
							sock = NL_INVALID;
							//FIXME: LOG PROGRESS strcpy(namestatus, "NO RESPONSE. RETRYING...");
							for (w=0;w<3*2;w++) { if (mjob_exit) break; if (mjob_fastretry) break; MS_SLEEP(500); }
							break;
						}
					}

					MS_SLEEP(50);
				}

				//error occured
				if (result == NL_INVALID) {

					//if already got html_end, no error
					//if (html_end)  // *** FIXME: parsing the result?
					//break;

					//error: try again
					nlClose(sock);
					sock = NL_INVALID;
					//FIXME: LOG PROGRESS strcpy(namestatus, "ERROR. RETRYING...");
					for (w=0;w<3*2;w++) { if (mjob_exit) break; if (mjob_fastretry) break; MS_SLEEP(500); }
					break;
				}

				//received anything below 32: turn them into "+" signals...
				if (lebuf[n] < 32)
					lebuf[n] = '+';

				//check for received </HTML>
				if (n >= 6) {
					if (
						(lebuf[n-6] == '<') &&
						(lebuf[n-5] == '/') &&
						((lebuf[n-4] == 'h') || (lebuf[n-4] == 'H')) &&
						((lebuf[n-3] == 't') || (lebuf[n-3] == 'T')) &&
						((lebuf[n-2] == 'm') || (lebuf[n-2] == 'M')) &&
						((lebuf[n-1] == 'l') || (lebuf[n-1] == 'L')) &&
						(lebuf[n-0] == '>')
					)
					{
						//LOG1("CLIENT MASTER QUERY RECEIVED </HTML>! SUCCESS!! n=%i\n", n);
						job->html_end = true;
						lebuf[n+1] = 0;
						//LOG1("---- Full response START ----\n%s\n---- Full response END ----\n", lebuf);
						break;
					}
				}

				//check for received another <HTML> : reset all stuff
				if (n >= 5) {
					if (
						(lebuf[n-5] == '<') &&
						((lebuf[n-4] == 'h') || (lebuf[n-4] == 'H')) &&
						((lebuf[n-3] == 't') || (lebuf[n-3] == 'T')) &&
						((lebuf[n-2] == 'm') || (lebuf[n-2] == 'M')) &&
						((lebuf[n-1] == 'l') || (lebuf[n-1] == 'L')) &&
						(lebuf[n-0] == '>')
					)
					{
						lebuf[n+1]=0;
						//LOG1("\n** READ <HTML>, DISCARDING BUFFER '%s' **\n", lebuf);
						n = -1;
					}
				}

				//read next
				n++;

			} while (1);

			//save n
			job->n = n;

			//quitting?
			if (mjob_exit) break;

			//found it?
			if (job->html_end) {

				//FIRST THINGS FIRST: close the socket
				nlClose(sock);
				sock = NL_INVALID;

				//job completed: who to call?
				job->retry = false;
				master_job_response(job);

				//check for retry -- 2 minutes wait before doing so
				if (job->retry)
					for (w=0;w<60*2*2;w++) { if (mjob_exit) break; if (mjob_fastretry) break; MS_SLEEP(500); }
				else
					break;	// ALL DONE !!
			}
			else {
				//failed, just retry (go on with the loop)
			}

		}//WHILE(password set)

		// =====
		//CLOSE SOCKET AND DECREMENT JOB COUNT
		// =====
		if (sock != NL_INVALID)
			nlClose(sock);
		pthread_mutex_lock ( &mjob_mutex );
		mjob_count--;
		pthread_mutex_unlock ( &mjob_mutex );

		//job completed -- nuke it
		delete job;
	}

	//check for private IP
	bool check_private_IP(char *address) {
		int i1, i2;
		int n=sscanf(address, "%d.%d.", &i1, &i2);
		assert(n==2);
		if (n != 2)
			return false;
		// private IP ranges:
		// 10.0.0.0        -   10.255.255.255
		// 172.16.0.0      -   172.31.255.255
		// 192.168.0.0     -   192.168.255.255
		// 169.254.0.0     -   169.254.255.255 [used by Microsoft DHCP client]
		return (i1==10 || (i1==172 && i2>=16 && i2<=31) || (i1==192 && i2==168) || (i1==169 && i2==254));
	}

	//master server talker thread
	void run_mastertalker_thread(void *arg) {

		//FIXME: generate a decent password here

		LOG("run_mastertalker_thread()\n");

		//get my IP
		//V0.4.4: -ip : force IP to something else
		char address[256];
		if (force_ip) {
			strcpy(address, force_ip_name);		//force IP

			LOG1("FORCING IP TO VALUE %s\n", force_ip_name);
		}
		else {
			//don't force: resolve
			NLaddress myadr;
			get_self_IP(&myadr);
			nlAddrToString(&myadr, address);

			//strcpy(address, "192.168.1.1");  //DEBUG private ip

			//v0.4.7: check for "private class IPs":
			bool privateip = check_private_IP(address);

			//LOG2("CHECKED DEFAULT IP %s RESULT = %i\n", address, privateip);

			//private ip?
			if (privateip) {

				//don't despair! check for all IPs
				NLaddress *locals;
				NLint     locsize;
				locals = nlGetAllLocalAddr(&locsize);

				for (int z=0;z<locsize;z++) {
					nlAddrToString( &(locals[z]) , address );
					LOG1("CHECKING LOCAL : %s ... ", address);
					privateip = check_private_IP(address);
					if (!privateip)	{
						LOG("NOT PRIVATE! this is good\n");
						break;	//success! "address" now has non-private address string
					}
					else
						LOG("PRIVATE! sucks... trying next...\n");
				}

				//still??
				if (privateip) {
					LOG1("PRIVATE IP: %s, (and all others): not talking to master server...\n", address);
					msock = NL_INVALID;	//???
					master_exiting_ok = true;
					return;
				}
			}
		}

		//v0.4.2: add port
		char sport[200];
		sprintf(sport, ":%i", port);
		strcat(address, sport);

		//while not time to quit
		while (!file_threads_quit) {

			//time to update with master server?
			if (get_time() > master_talk_time) {

				//LOG("MASTER TALKER: time to talk\n");

				//schedule next
				master_talk_time = get_time() + 3 * 60.0 ;		//3 minutes

				//open socket
				nlEnable(NL_BLOCKING_IO);
				msock = nlOpen(0, NL_RELIABLE);
				nlDisable(NL_BLOCKING_IO);
				if (msock == NL_INVALID) {
					LOG("SERVER CAN'T OPEN SOCKET TO CONNECT TO MASTER SERVER!!\n");
					continue;
				}

				//LOG("MASTER TALKER: socket open\n");

				//connect
				//NLaddress masadr;
				//nlGetAddrFromName("www.mycgiserver.com", &masadr);	//www.mycgiserver.com
				//nlStringToAddr("212.69.162.53", &masadr);
				//nlSetAddrPort(&masadr, 80);													//port 80
				if (nlConnect(msock, &master_address) == NL_FALSE) {		//connect
					LOG("SERVER CAN'T CONNECT TO WWW.MYCGISERVER.COM:80 !!!\n");
					nlClose(msock);
					msock = NL_INVALID;
					continue;
				}

				//LOG("MASTER TALKER: socket connected\n");

				//built the state
				char state[1024];
				sprintf(state, "%i/%i players - %s - v%s", player_count, maxplayers, hostname, GAME_VERSION);
				for (int h=0; state[h]; h++)
					if (state[h] == ' ')	//switch spaces to plus'es
						state[h] = '+';

				//build the GET request
				char query[1024];
				sprintf(query, "GET /servlet/fcecin.m3/index.html?add=%s&pass=1111&st=%s\n\n", address, state);
				char lebuf[65536]; int count = 0;
				writeString(lebuf, count, query);
				//erase the 0
				count--;

				//chance to give up
				if (file_threads_quit)
					break;

				//now we have talked
				master_never_talked = false;

				//send it
				NLint result = nlWrite(msock, lebuf, count);
				//LOG3("WROTE TO MASTER '%s', result = %i, count = %i\n", query, result, count);
				//LOG2("MASTER TALKER: wrote to master %i,%i", result, count);

				//parse the response (should be <HTML><BODY> etc... with "@K" on it
				int n=0;
				double timeout = get_time() + 60.0;
				do {

					//read
					result = nlRead(msock, &(lebuf[n]), 1);
					if (result != 1) {
						LOG1("MASTER TALKER: ERROR r=%i\n", result);
						break;
					}

					//check for received </HTML>
					if (n > 8) {
						if (
							(lebuf[n-6] == '<') &&
							(lebuf[n-5] == '/') &&
							((lebuf[n-4] == 'h') || (lebuf[n-4] == 'H')) &&
							((lebuf[n-3] == 't') || (lebuf[n-3] == 'T')) &&
							((lebuf[n-2] == 'm') || (lebuf[n-2] == 'M')) &&
							((lebuf[n-1] == 'l') || (lebuf[n-1] == 'L')) &&
							(lebuf[n-0] == '>')
						)
						{

							//LOG("MASTER TALKER: </HTML>\n");
							lebuf[n+1] = 0;
							//LOG1("---- Full response START ----\n%s\n---- Full response END ----\n", lebuf);
							break;
						}
					}

					//read next
					n++;

					//quit if timeout
					if (get_time() > timeout)
						break;

					//quit if told to
					if (file_threads_quit)
						break;

				} while (1);

				//close socket
				nlClose(msock);
				msock = NL_INVALID;
			}

			//no: sleep a bit
			MS_SLEEP(500);
		}

		LOG("MASTER TALKER: time to say goodbye\n");

		//master is pre-exiting, no need to do the first socket closure
		master_pre_exiting_ok = true;

		//qutting: close the socket
		if (msock != NL_INVALID) {
			LOG("MASTER TALKER: bye 1\n");
			nlClose(msock);
			msock = NL_INVALID;
		}

		//never talked? then just quit
		if (master_never_talked) {
			//master exited OK!
			master_exiting_ok = true;
			return;
		}

		LOG("MASTER TALKER: bye 2\n");

		//quitting: delete my IP from master so clients won't see it
		//open socket
		nlDisable(NL_BLOCKING_IO);			//nonblocking socket, let's make this simple...
		msock = nlOpen(0, NL_RELIABLE);

		if (msock == NL_INVALID) {
			LOG("(QUIT) SERVER CAN'T OPEN SOCKET TO CONNECT TO MASTER SERVER!!\n");
			return;
		}

		//connect
		//NLaddress masadr;
		//nlGetAddrFromName("www.mycgiserver.com", &masadr);	//www.mycgiserver.com
		//nlStringToAddr("212.69.162.53", &masadr);
		//nlSetAddrPort(&masadr, 80);													//port 80
		if (nlConnect(msock, &master_address) == NL_FALSE) {		//connect
			LOG("(QUIT) SERVER CAN'T CONNECT TO WWW.MYCGISERVER.COM:80 !!!\n");
			nlClose(msock);
			msock = NL_INVALID;
			return;
		}

		//build the GET request
		char query[1024];
		sprintf(query, "GET /servlet/fcecin.m3/index.html?del=%s&pass=1111\n\n", address);
		char lebuf[65536]; int count = 0;
		writeString(lebuf, count, query);
		//erase the 0
		count--;

		//send it

		NLint result;
		double querytimeout = get_time() + 5.0;

		do {
			MS_SLEEP(50);
			result = nlWrite(msock, lebuf, count);
		} while ((result == NL_INVALID) && (get_time() < querytimeout));

		if (get_time() >= querytimeout) {

			LOG("(QUIT) query timeout\n");

			//master exited OK!
			master_exiting_ok = true;

			//close socket
			nlClose(msock);
			msock = NL_INVALID;
			return;
		}

		LOG3("(QUIT) WROTE TO MASTER '%s', result = %i, count = %i\n", query, result, count);

		//parse the response (should be <HTML><BODY> etc... with "@K" on it
		double timeout = get_time() + 5.0;
		int n=0;
		do {

			//read
			result = nlRead(msock, &(lebuf[n]), 1);
			if (result == NL_INVALID) {
				LOG1("(QUIT) ERROR READING RESPONSE result = %i\n", result);
				break;
			}
			//timeout?
			if (get_time() > timeout)
				break;
			//nothing to read
			if (result == 0) {
				MS_SLEEP(10);
				continue;
			}

			//check for received </HTML>
			if (n > 8) {
				if (
					(lebuf[n-6] == '<') &&
					(lebuf[n-5] == '/') &&
					((lebuf[n-4] == 'h') || (lebuf[n-4] == 'H')) &&
					((lebuf[n-3] == 't') || (lebuf[n-3] == 'T')) &&
					((lebuf[n-2] == 'm') || (lebuf[n-2] == 'M')) &&
					((lebuf[n-1] == 'l') || (lebuf[n-1] == 'L')) &&
					(lebuf[n-0] == '>')
				)
				{
					LOG("(QUIT) RECEIVED </HTML>! SUCCESS!!\n");
					lebuf[n+1] = 0;
					//LOG1("(QUIT) ---- Full response START ----\n%s\n(QUIT) ---- Full response END ----\n", lebuf);
					break;
				}
			}

			//read next
			n++;

		} while (1);

		//master exited OK!
		master_exiting_ok = true;

		//close socket
		nlClose(msock);
		msock = NL_INVALID;
	}

	//read a string from a blocking TCP stream, one char at a time
	bool read_string_from_TCP(NLsocket sock, char *buf) {

		int n =0;

		do {

			NLint result = nlRead(sock, &(buf[n]), 1);
			if (result != 1)	//interrupted
				return false;

//			if (buf[n] == '0')	//string was terminated
			if (buf[n] == '\0')	//#NR //string was terminated
				return true;

			n++;

		} while (1);
	}

	//run a admin shell master thread
	void run_shellmaster_thread(void *arg) {

		LOG("\nrun_shellmaster_thread() STARTED\n");

		while (1) {
			//accept one connection
			nlEnable(NL_BLOCKING_IO);
			NLsocket pidaosock = nlAcceptConnection(shellmsock);
			nlDisable(NL_BLOCKING_IO);

			//valid socket?
			if (pidaosock != NL_INVALID) {

				LOG("\npidaosock NOT INVALID! incoming SHELL CONNECTION!\n");

				//accept connections only from localhost
				NLaddress addr, c1, c2;
				nlGetRemoteAddr(pidaosock, &addr);
				nlStringToAddr("127.0.0.1", &c1);
				get_self_IP(&c2);
				nlSetAddrPort(&addr, 0);
				nlSetAddrPort(&c1, 0);
				nlSetAddrPort(&c2, 0);

				if ((nlAddrCompare(&addr, &c1) == NL_FALSE) && (nlAddrCompare(&addr, &c2) == NL_FALSE)) {
					LOG("\nWARNING: attempt to connect a remote admin shell blocked!\n");
					nlClose(pidaosock);
					continue;
				}

				//if already connected, skip
				if (shellssock != NL_INVALID)	{ //skip
					LOG("\nWARNING: attempt to connect two simultaneous admin shells blocked!\n");
					nlClose(pidaosock);
					continue;
				}

				LOG("**** ADMIN SHELL SOCKET CONNECTED ON LOOPBACK PORT! ******\n");

				//ADMIN SHELL just connected: tell about the current situation!
				//
				char lebuf[4096]; int count = 0;
				for (int i=0;i<maxplayers;i++)
				if (player[i].used)
				if (!player[i].isbot)
				{
					writeLong(lebuf, count, STA_PLAYER_CONNECTED);////1 .... player connected <int id>
					writeLong(lebuf, count, player[i].cid);
					writeLong(lebuf, count, STA_PLAYER_FRAGS);
					writeLong(lebuf, count, player[i].cid);
					writeLong(lebuf, count, player[i].frags);
					writeLong(lebuf, count, STA_PLAYER_NAME_UPDATE);
					writeLong(lebuf, count, player[i].cid);
					writeString(lebuf, count, player[i].name);
					writeLong(lebuf, count, STA_PLAYER_IP);
					writeLong(lebuf, count, player[i].cid);
					char addrBuf[50];
					NLaddress addr=server->get_client_address(player[i].cid);
					nlSetAddrPort(&addr, 0);
					nlAddrToString(&addr, addrBuf);
					writeString(lebuf, count, addrBuf);
				}
				nlWrite(pidaosock, lebuf, count);

				//keep socket so it can be closed. this assignment also will reflect
				// on the execution of run_shellslave_thread()
				shellssock = pidaosock;
			}
			else {
				if (file_threads_quit) {		//quitting!
					LOG("SHELLMASTER THREAD IS QUITTING\n");
					nlClose(shellmsock);
					return;
				}
				//else
				//LOG("(SH)");
			}

			//sleep a bit
			MS_SLEEP(500);
		}
	}

	//run an admin shell slave thread
	void run_shellslave_thread(void *arg) {

		LOG("run_shellslave_thread() STARTED\n");

		while (1) {

			// valid socket?
			if (shellssock != NL_INVALID) {

				//LOG("SLAVE SHELLSOCK READING MESSAGE...\n");

				//read request code
				NLint code, pid, clid, arg;
				char rbuf[256]; int rcount = 0;
				NLint result = nlRead(shellssock, rbuf, 4);
				rcount = 0; readLong(rbuf, rcount, code);

				//a zero result may be connection not ready yet (?)
				if (result == 0) {
					MS_SLEEP(10);
					continue;
				}

				LOG2("READ RESULT = %i VALUE = %i\n", result, code);

				if (result == NL_INVALID) {
					LOG4("RESULT IS NL_INVALID. errors are %i %s %i %s\n", nlGetError(), nlGetErrorStr(nlGetError()), nlGetSystemError(), nlGetSystemErrorStr(nlGetSystemError()));
					LOG("SLAVE CONNECTION RESET (1)\n");
					if (shellssock != NL_INVALID) {
						nlClose(shellssock);
						shellssock = NL_INVALID;
					}
					continue;
				}

				if (file_threads_quit) {
					LOG("SLAVE QUIT (2)\n");
					break; //quitting...
				}

				if (result != 4) {
					LOG("SLAVE CONNECTION RESET (2)\n");
					if (shellssock != NL_INVALID) {
						nlClose(shellssock);
						shellssock = NL_INVALID;
					}
					continue;
				}//error occured: end here

				bool should_quit = false;
				bool answer = false;
				char lebuf[1024];
				char chat[2048];
				char lechat[2048];
				int count = 0, delta;

				//parse it
				switch (code) {
				case ATS_NOOP:						//0= no-op
					break;
				case ATS_GET_PLAYER_FRAGS:		//1... request the frags amount of a player <int id>
					result = nlRead(shellssock, rbuf, 4);
					rcount = 0; readLong(rbuf, rcount, clid); pid = ctop[clid];
					if (result == 4) {

						if ((!player[pid].used) || (player[pid].isbot)) {
						}
						else {
							answer = true; //ADMIN SHELL
							writeLong(lebuf, count, STA_PLAYER_FRAGS);
							writeLong(lebuf, count, player[pid].cid);
							writeLong(lebuf, count, player[pid].frags);
						}
					}
					break;
				case ATS_GET_PLAYER_TOTAL_TIME:		//request the frags amount of a player <int id>
					result = nlRead(shellssock, rbuf, 4);
					rcount = 0; readLong(rbuf, rcount, clid); pid = ctop[clid];
					if (result == 4) {

						if ((!player[pid].used) || (player[pid].isbot)) {
						}
						else { //ADMIN SHELL
							answer = true;
							writeLong(lebuf, count, STA_PLAYER_TOTAL_TIME);
							writeLong(lebuf, count, player[pid].cid);
							delta = (int)(get_time() - player[pid].start_time);
							writeLong(lebuf, count, delta);
						}
					}
					break;
				case ATS_GET_PLAYER_TOTAL_KILLS:		//request the total kills amount of a player <int id>
					result = nlRead(shellssock, rbuf, 4);
					rcount = 0; readLong(rbuf, rcount, clid); pid = ctop[clid];
					if (result == 4) {

						if ((!player[pid].used) || (player[pid].isbot)) {
						}
						else {
							answer = true;//ADMIN SHELL
							writeLong(lebuf, count, STA_PLAYER_TOTAL_KILLS);
							writeLong(lebuf, count, player[pid].cid);
							writeLong(lebuf, count, player[pid].total_kills);
						}
					}
					break;
				case ATS_GET_PLAYER_TOTAL_DEATHS:		//request the total deaths amount of a player <int id>
					result = nlRead(shellssock, rbuf, 4);
					rcount = 0; readLong(rbuf, rcount, clid); pid = ctop[clid];
					if (result == 4) {
						if ((!player[pid].used) || (player[pid].isbot)) {
						}
						else {
							answer = true;//ADMIN SHELL
							writeLong(lebuf, count, STA_PLAYER_TOTAL_DEATHS);
							writeLong(lebuf, count, player[pid].cid);
							writeLong(lebuf, count, player[pid].total_deaths);
						}
					}
					break;
				case ATS_GET_PLAYER_TOTAL_CAPTURES:		//request the total captures amount of a player <int id>				}
					result = nlRead(shellssock, rbuf, 4);
					rcount = 0; readLong(rbuf, rcount, clid); pid = ctop[clid];
					if (result == 4) {

						if ((!player[pid].used) || (player[pid].isbot)) {
						}
						else {
							answer = true;//ADMIN SHELL
							writeLong(lebuf, count, STA_PLAYER_TOTAL_CAPTURES);
							writeLong(lebuf, count, player[pid].cid);
							writeLong(lebuf, count, player[pid].total_captures);
						}
					}
					break;
				case ATS_SERVER_CHAT:									//server is saying <string chat line>
					read_string_from_TCP(shellssock, (char *)chat);
					sprintf(lechat, "ADMIN: %s", chat);
					broadcast_message(lechat);
					break;
				case ATS_GET_PINGS:	//#NR
					for (int p=0; p<maxplayers; ++p)
						if (player[p].used && !player[p].isbot) {
							answer=true;
							writeLong(lebuf, count, STA_PLAYER_PING);
							writeLong(lebuf, count, player[p].cid);
							writeLong(lebuf, count, player[p].ping);
						}
					break;
				case ATS_MUTE_PLAYER:
					result = nlRead(shellssock, rbuf, 8);
					rcount = 0; readLong(rbuf, rcount, clid); pid = ctop[clid];
					readLong(rbuf, rcount, arg);
					if (result == 8)
						mutePlayer(pid, arg);
					break;
				case ATS_KICK_PLAYER:
					result = nlRead(shellssock, rbuf, 4);
					rcount = 0; readLong(rbuf, rcount, clid); pid = ctop[clid];
					if (result == 4)
						kickPlayer(pid);
					break;
				case ATS_BAN_PLAYER:
					result = nlRead(shellssock, rbuf, 4);
					rcount = 0; readLong(rbuf, rcount, clid); pid = ctop[clid];
					if (result == 4)
						banPlayer(pid);
					break;
				case ATS_QUIT:
					should_quit = true;
					break;
				}

				//quitting?
				if (should_quit) {
					LOG("SLAVE ATS_QUIT (connction reset)\n");
					if (shellssock != NL_INVALID) {
						nlClose(shellssock);
						shellssock = NL_INVALID;
					}
					continue;
				}

				//quitting...
				if (file_threads_quit) {
					LOG("SLAVE QUIT (4)\n");
					break;
				}

				//send answer to the shell
				if (answer) {
					result = nlWrite(shellssock, lebuf, count);
				}
			}
			//not valid
			else {
				//sleep a bit
				MS_SLEEP(1000);
			}

			//quitting...
			if (file_threads_quit) {
				LOG("SLAVE QUIT (5)\n");
				break;
			}
		}

		//quitting
		LOG("ADMIN SHELL SLAVE THREAD QUITTING... CLOSING SOCKET\n");
		if (shellssock != NL_INVALID) {
			nlClose(shellssock);
			shellssock = NL_INVALID;
		}
	}

	//stop server
	void stop() {

		//submit all pending player reports
		int i,cid;
		for (i=0;i<maxplayers;i++)
		if (player[i].used)
		if (!player[i].isbot)
		{
			cid = player[i].cid;
			client_report_status(cid);
		}

		//v0.4.4 : flag master job threads to start trying to resolve themselves quickly
		mjob_fastretry = true;
		double mjmaxtime = get_time() + 30.0;		//timeout : 30 seconds

		server_status_string("Shutdown: Net Server");

		if (server)
			server->stop(3);
		else
			throw 8384;

		//reset client_c struct (closes files...)
		for (i=0;i<MAX_PLAYERS;i++)
			client[i].reset();

		// flag so threads will quit themselves
		master_pre_exiting_ok = false;
		master_exiting_ok = false;
		file_threads_quit = true;	//quit stuff now

		//close TCP connection with the server admin shell
		//
		server_status_string("Shutdown: MSHELL Thread");

		LOG("GAMESERVER JOINING SHELL-MASTER THREAD...\n");
		pthread_join( shellmthread, 0 );

		server_status_string("Shutdown: SSHELL Socket");

		LOG("GAMESERVER CLOSING SHELL-SLAVE SOCKET...\n");
		if (shellssock != NL_INVALID)
			nlClose( shellssock );

		server_status_string("Shutdown: SSHELL Thread");

		LOG("GAMESERVER JOINING SHELL-SLAVE THREAD...\n");
		pthread_join( shellsthread, 0 );

		//file downlaod to clients threads..
		//

		//v0.4.4 : DO NOT open tcp download port if (-notcp) enabled
		//
		if (no_tcp_download) {
			LOG("SKIPPING TCP FILE SOCKETS/THREADS (-notcp....)");
		}
		else {

			LOG("GAMESERVER CLOSING FILEMASTER'S SOCKET...\n");

			server_status_string("Shutdown: MFILE Socket");

			nlClose(filesock);
			LOG("GAMESERVER STOP JOIN FILEMASTER...\n");

			server_status_string("Shutdown: MFILE Thread");

			pthread_join( server_filemaster_thread , 0 );
			LOG("OK!\n");

			pthread_mutex_lock( &fslavesock_mutex );

			for (i=0;i<MAX_PLAYERS;i++)
				if (fslavesock[i] != NL_INVALID) {
					server_status_string("Shutdown: SFILE Socket");
					nlClose(fslavesock[i]);
					LOG2("GAMESERVER STOP JOIN FILESLAVE %i %i...", i, (int)fslavesock[i]);
					server_status_string("Shutdown: SFILE Thread");
					if (fslavethr[i] != (pthread_t)-1)
						pthread_join ( fslavethr[i] , 0 );
					LOG("OK!\n");
				}

			pthread_mutex_unlock( &fslavesock_mutex );
		}

		//thread for TCP connection that server uses to register it's IP on the master-server
		//
		if (!privateserver) {

			//MS_SLEEP(1000);	//wait a bit for the slave thread to catch up

			if (!master_pre_exiting_ok) {
				if (msock != NL_INVALID) {
					LOG("GAMESERVER CLOSING MASTER SERVER SOCKET...\n");
					server_status_string("Shutdown: MASTER Socket 1");
					nlClose(msock);
					msock = NL_INVALID;
				}
				else
					LOG("GAMESERVER CLOSING MASTER SERVER SOCKET (BUT IT HAS ALREADY BEEN DISCONNECTED AND SENDING DISCONNECT TO MASTER)...\n");
			}

			//MS_SLEEP(1000);	//wait a bit for the slave thread to catch up

			LOG("GAMESERVER JOINING MASTER SERVER THREAD...\n");

			server_status_string("Shutdown: MASTER Thread");

			int waitcount = 0;
			do {
				MS_SLEEP(100);
				if (master_exiting_ok) {	//done!
					LOG("IT EXITED...\n");
					break;
				}

				waitcount++;
				if (waitcount > 30) {		// 30 * 100ms = 3 seconds
					LOG("TIRED OF WAITING...\n");
					//kill the socket
					server_status_string("Shutdown: MASTER Socket 2");
					nlClose(msock);		//close AGAIN (it's a different one)
					msock = NL_INVALID;
					break;
				}

			} while (1);

			//wait for all master jobs to complete nicely
			while ( (mjob_count > 0) && ( get_time() < mjmaxtime )) {
				char lix[200];
				sprintf(lix, "Shutdown: MJOBS %i left", mjob_count);
				server_status_string(lix);
				MS_SLEEP(100);
			}

			//clean up jobs
			mjob_exit = true;		//MUST terminate -- abort
			while (mjob_count > 0) {
				char lix[200];
				sprintf(lix, "Shutdown: MJOBS ABORT %i left", mjob_count);
				server_status_string(lix);
				MS_SLEEP(100);
			}

			//NOW one can join with the thread without fear
			LOG("DE FACTO JOIN...\n");

			server_status_string("(Kill this window if not closing)");

			pthread_join( mthread , 0 );
		}


	}

	//get hostname: for hello
	char *get_hostname() {
		return hostname;
	}
};

// gameserver instance
gameserver_c *gameserver;


int sfunc_client_hello(runes_t *arg) {

	//FIXME: this is returned by the function! should be using one per client
	static char lebuf[128];
	lebuf[127]=0;	//paranoia

	//LOG1("hello client %i!\n", arg->client_id);
	static runes_t result;

	//check versions
	char stri[256];
	char temp[256];
	int count = 0;

	/*LOG("args bytevalues : ");
	for (int i=0;i<arg->length;i++) {
		LOG1("%i.\n", arg->data[i] );
	}
	LOG("args charvalues : ");
	for (i=0;i<arg->length;i++) {
		LOG1("%c.\n", arg->data[i] );
	}*/

	readString(arg->data, count, stri);	//read gamestring

	if (strcmp(stri, GAME_STRING)) {

		LOG2("GAME STRINGS DONT MATCH: %s and %s\n", GAME_STRING, stri);
		result.client_id = -1;		// not accepted

		sprintf(temp, "Different game: '%s'", stri);
		count = 0;
		writeString(lebuf, count, temp);

		result.data = &(lebuf[0]);			//custom deny data
		result.length = count;
		//result.data = 0;		//no custom data
		//result.length = 0;	//no custom data
	}
	else {

		readString(arg->data, count, stri);	//read protocol string

		if (strcmp(stri, GAME_PROTOCOL)) {

			LOG2("GAME PROTOCOL STRINGS DONT MATCH: %s and %s\n", GAME_PROTOCOL, stri);
			result.client_id = -1;		// not accepted
			//result.data = 0;		//no custom data
			//result.length = 0;	//no custom data

			sprintf(temp, "Protocol mismatch: server: %s, client: %s", GAME_PROTOCOL, stri);
			count = 0;
			writeString(lebuf, count, temp);

			result.data = &(lebuf[0]);			//custom deny data
			result.length = count;
		}
		else if (player_count >= maxplayers) {		//server full!

			LOG("....unfortunatelly the server is FULL! hello rejected\n");
			result.client_id = -1;		// not accepted
			//result.data = 0;		//no custom data
			//result.length = 0;	//no custom data

			sprintf(temp, "Server is full. (%i players)", player_count);
			count = 0;
			writeString(lebuf, count, temp);

			result.data = &(lebuf[0]);			//custom deny data
			result.length = count;
		}
		#ifdef NR_NAME_AUTHORIZATION
		else if (gameserver->authorizations.isBanned(gameserver->server->get_client_address(arg->client_id))) {
			result.client_id = -1;	// not accepted
			count=0;
			writeString(lebuf, count, "You are banned");
			result.data = lebuf;
			result.length = count;
		}
		#endif
		else {

			result.client_id = arg->client_id;		//this means "accept the connection"
			//custom data:
			//   BYTE MAXPLAYERS
			//   STRING hostname
			//
			int count = 0;
			writeByte(lebuf, count, ((NLubyte)maxplayers));
			writeString(lebuf, count, gameserver->get_hostname());
			if (no_tcp_download)
				writeByte(lebuf, count, 1);							//V0.4.4 NEW: server's NOTCP flag value. 0=off 1=on
			else
				writeByte(lebuf, count, 0);							//V0.4.4 NEW: server's NOTCP flag value. 0=off 1=on

			//strcpy(lebuf, gameserver->get_hostname());
			//result.data = &(lebuf[0]);
			//result.length = strlen(lebuf)+1;	//inclui o zero
			result.data = &(lebuf[0]);
			result.length = count;	//inclui o zero
		}
	}

	return (int)(&result);
}

int sfunc_client_connected(runes_t *arg) {

	//LOG1("client connected %i\n", arg->client_id);

	gameserver->client_connected(arg->client_id, false); //false == nao eh serverside bot

	return 0;
}

int sfunc_client_disconnected(runes_t *arg) {

	//LOG1("client disconnected %i\n", arg->client_id);

	gameserver->client_disconnected(arg->client_id, false);	//false == nao eh serverside bot

	return 0;
}

int sfunc_client_lag_status(runes_t *arg) {

	//LOG2("client %i lagstatus %i\n", arg->client_id, arg->status);

	return 0;
}

int sfunc_client_data(runes_t *arg) {

	//LOG2("client %i data=%i\n", arg->client_id, arg->length);

	//process it
	gameserver->incoming_client_data(arg->client_id, arg->data, arg->length);

	return 0;
}

int sfunc_client_ping_result(runes_t *arg) {
	gameserver->ping_result(arg->client_id, arg->pingtime);

	return 0;
}

//============================================================
//  TCP server admin shell interaction thread
//============================================================

//thread for server shell connections
void *thread_shellmaster_f(void *arg) {

	gameserver->run_shellmaster_thread(arg);

	return 0;
}

void *thread_shellslave_f(void *arg) {

	gameserver->run_shellslave_thread(arg);

	return 0;
}

//============================================================
//  TCP master server (web servlet) interaction thread -- LIST SERVER
//============================================================

//thread for master server interaction
void *thread_mastertalker_f(void *arg) {

	gameserver->run_mastertalker_thread(arg);

	return 0;
}

//============================================================
//  a single independent job to the master server
//============================================================

void *thread_masterjob_f(void *arg) {

	gameserver->run_masterjob_thread(arg);

	return 0;
}

//============================================================
//  TCP file server threads
//============================================================

void *thread_filemaster_f(void *arg) {

	gameserver->run_filemaster_thread(arg);

	return 0;
}

void *thread_fileslave_f(void *arg) {

	gameserver->run_fileslave_thread(arg);

	return 0;
}

//============================================================
//  listen server thread
//============================================================

int listen_port_running;
volatile bool	listen_server_running = false;
pthread_t	listen_server_thread;

void *thread_listenserver_f(void *arg) {
	srand(time(0));	//#NR

	//save for display
	listen_port_running = port;		//port selectr

	//(1) start the localserver
	//
	gameserver = new gameserver_c();
	if (!gameserver->start(server_maxplayers)) {
		LOG("ERROR: cannot start LISTEN GAME SERVER!!!\n");
		listen_server_running = false;
		return 0;
	}

	//(2) loop the server until not quitting
	//
	gameserver->loop( &listen_server_running );

	//(3) shutdown the localserver
	//
	LOG("GAMESERVER STOPPING");
	gameserver->stop();
	LOG("GAMESERVER DELETING");
	delete gameserver;
	LOG("GAMESERVER DELETED");
	gameserver = 0;

	//restore client's windowtitle
	server_status_string("Outgun client - CTRL+F12 to quit");

	return 0;
}

void listen_start() {

	if (listen_server_running) return;
	listen_server_running = true;

	LOG("listen_start()\n");

	pthread_create(&listen_server_thread, 0, thread_listenserver_f, (void *)0);
}

void listen_stop() {

	if (!listen_server_running) return;
	listen_server_running = false;

	LOG("listen_stop()\n");

	pthread_join(listen_server_thread, 0);
}

//************************************************************
//  client stuff
//************************************************************

// number of chat messages in the buffer
#define CHAT_SIZE 8

// size of clientside visual fx array
#define MAX_CLIENTFX 128

// size of connect screen
#define MAX_GAMESPY 24

// size of udp download queue (only 1 should be needed but...)
#define MAX_UDPDQ 16

// drawing screens
BITMAP *drawbuf, *vidpage1=0, *vidpage2=0, *backbuf=0;
bool		page_flipping;

//pageflip panic
//BITMAP *vidarr[4096];

// client callbacks
int cfunc_connection_update(client_runes_t *arg);
int cfunc_server_data(client_runes_t *arg);

//explosion clientside fx
struct clientfx_t {

	bool		used;		//used record?

	int			type;		// type of fx	0==gun explosion
	int			px,py;	// screen where it spawned. if changed when time to redraw, delete it
	double time;		// start time

	//fx specific vars
	int x;					// screen x  of fx
	int y;					// screen y  of fx

	//speed fx
	int col1, col2, gundir;

	//deathbringer owner
	int owner;
};

struct download_runes_t {

	int did;		//download id

	char type[64];	//type of file to download
	char name[256];	//name of file to download
	char dest[512];	//full destination path+name for downloaded file

};

//client downloader thread prototype
void *thread_clientdownloader_f(void *arg);

//client password-to-token retrieval thread
void *thread_clientpassword_f(void *arg);


// the game client state
// this includes graphics, simulation & networking (what a mess)
class gameclient_c {
public:

	// the current worldmap
	Map		map;

	//all the players to show including me
	player_t player[MAX_PLAYERS];

	//explosion fx's
	clientfx_t		cfx[MAX_CLIENTFX];

	//wich player I am
	int	me;

	//game option!
	bool option_show_names;

	//password stuff
	bool player_password_set;	//flag for the thread
	bool player_token_new;		//TRUE if first call to token servled
	bool player_token_set;		//TRUE if player now holds a valid token
	char player_token[64];		// the token
	char player_password[16];
	pthread_t	passthread;

	//the frame to be drawn , frame received from server (extrapolation basis)
	frame_t		fd, fx;

	//incoming framedata mutex
	pthread_mutex_t		frame_mutex;

	//UDP download queue
	pthread_mutex_t		udpdq_mutex;
	int udpdq_size;		//size
	download_runes_t		*udpdq[MAX_UDPDQ];		//the udp download queue
	int udpdq_ptr;		//current download. if -1, no current downloads
	int ud_fp;			//file pointer for read/write
	FILE *ud_fout;	//input or output file

	//progress bar of file download
	NLulong fdp, fdp_max;

	//outgun world network status
	NLulong max_world_score, max_world_rank;

	//time of last packet received
	double lastpackettime;

	//net client
	client_c *client;

	//audio samples
	SAMPLE *sample[NUM_OF_SAMPLES];
	//reverse-play flags
	bool sample_reverse[NUM_OF_SAMPLES];

	//sound themes setting
	char sfxthemedir[256];
	char sfxthemename[256];
	al_ffblk	sfxthemeffblk;	//for al_find*
	bool	validtheme;		// if sfxthemedir points to valid dir

	//menu showing?
	bool menushow;
	int menu;		//menu screen #

	//game showing?
	bool gameshow;

	//help screen showing
	bool helpshow;

	//frames and seconds for FPS counter
	double FPS;
	int framecount, totalframecount;
	double starttime;

	//if player wants to changeteams
	bool want_change_teams;

	//if player wants to exit the map
	bool want_map_exit;

	//client-to-server bot configuration preferences:
	NLubyte		botsize;		//target team sizes
	NLubyte		botmode;		//wanted bot mode  0=fill holes  1=use botsize
	double		bot_pref_time;		//otimizando rede: caso jogador fique trocando toda hora de cfg de bot
	bool			bot_pref_change;	//otimizando rede: caso jogador fique trocando toda hora de cfg de bot

	//the scoreboard, maps entry # to display into player-id
	int scoreboard[MAX_PLAYERS];

	//trying connection? if true, ESC cancels it
	volatile bool trying_connection;

	//connected? (that is, "connection accepted")
	volatile bool connected;

	//hostname, retirado do connect packet
	char	hostname[256];
	int		strlen_hostname;	//strlen precalculado

	//connect screen, my "mini-gamespy"
	bool				showmaster;		//showing master screen (opposite: showing favourites screen)
	bool				first_fav_refresh;		//first refresh of favorites page already done?
	gamespy_t		gamespy[MAX_GAMESPY];
	int					gi;	//what game entry
	gamespy_t		mgamespy[MAX_GAMESPY];		//gamespy of masterserver

	char playername[256]; //the player's name (max name len = 16)
	char namestatus[64];		// v0.4.4: NAME STATUS (unregistered, registering..., registered!)
	char editplayername[256]; //the player's name edit buffer
	char address[256];		//server IP address
	char dialogmessage[256];	//dialog message
	char dialogmessage2[256];	//dialog message line 2
	char talkbuffer[256];			// chat input buffer
	char chatbuffer[CHAT_SIZE][256];		// last chat messages list
	double chaterasetime;				// time to erase a chat message from the list
	//more edit stuff 0.4.4
	char editplayerpass[64]; //the player's password edit buffer
	char namecursor[2];
	char passcursor[2];

	//IMPORTANT: player name registration status
	int		namestatus_code;		//0==NONE  1==LOGGED w/ token  2==LOGIN FAILED by last attempt  3==LOGGED+RECORDING

	//bitmap for minimap background (level walls, screen grid)
	BITMAP *minibg;

	// bitmap for flag position marks
	BITMAP* flagpos_buf[2];
	bool flagpos_ready;

	//flag if map valid or not ready yet
	bool map_ready;
	char servermap[64];	//last map command from server

	//flag for gameover plaque showing
	int gameover_plaque;
	int red_final_score, blue_final_score;		//final scores for showing in the gameover plaque

	//server is using -notcp flag?
	bool server_no_tcp;

	//host's banner image
	BITMAP *hostad;
	char    hostadname[128];

	ofstream message_log;

	void check_flagpos_marks();

	//start
	bool start() {

		//host ad
		hostad = 0;
		hostadname[0]=0;

		// open message log file
		message_log.open("message.log", ios::app);

		//default physics parameters
		//set_default_physics();
		//LOG3("\nNORMAL   fri %.1f acc %.1f mxs %.1f\n", svp_fric, svp_accel, svp_maxspeed);
		//LOG3("RUN      fri %.1f acc %.1f mxs %.1f\n", svp_fric_run, svp_accel_run, svp_maxspeed_run);
		//LOG3("TURBO    fri %.1f acc %.1f mxs %.1f\n", svp_fric_turbo, svp_accel_turbo, svp_maxspeed_turbo);
		//LOG3("TURBORUN fri %.1f acc %.1f mxs %.1f\n", svp_fric_turborun, svp_accel_turborun, svp_maxspeed_turborun);

		//cursors: default at name
		strcpy(namecursor, "_");
		strcpy(passcursor, "");

		//clear UDPDQ
		for (int uq=0;uq<MAX_UDPDQ;uq++) udpdq[uq] = 0;
		udpdq_ptr = -1;
		udpdq_size = 0;

		//hide helpscreen
		helpshow = false;

		totalframecount = 0;
		framecount = 0;

		// default map
		//load_default_map(&map);
		map_ready = false;		// NO map change commands from server yet
		servermap[0]=0;

		flagpos_buf[0] = 0;
		flagpos_buf[1] = 0;
		flagpos_ready = false;

		//not showing gameover plaque
		gameover_plaque = NEXTMAP_NONE;

		//update minimap background
		minibg = create_bitmap(100, 100);
		//update_minimap_background();

		//clear fx
		clear_fx();

		trying_connection = false;
		connected = false;

		client = new_client_c();
		client->set_callback(CFUNC_CONNECTION_UPDATE, cfunc_connection_update);
		client->set_callback(CFUNC_SERVER_DATA, cfunc_server_data);

		//init gamespy/adresses
		first_fav_refresh = false;
		showmaster = true;
		address[0]=0;
		gi = 0;
		for (int i=0;i<MAX_GAMESPY;i++) {
			gamespy[i].address[0]=0;
			gamespy[i].refreshed = false;
			gamespy[i].invalid = false;	//don't know the status yet
			//master gamespy:
			mgamespy[i].address[0]=0;
			mgamespy[i].refreshed = false;
			mgamespy[i].invalid = false;	//don't know the status yet
		}

		//assume no password
		player_token_set = false;			//no token
		player_password_set = false;	//no password
		player_password[0]=0;
		editplayerpass[0]=0;
		strcpy(namestatus, "NO PASSWORD SET");
		namestatus_code = 0;

		//try to load the client's password
		char dest[WHERE_PATH_SIZE];
		int c;
		append_filename(dest, wheregamedir, "password.bin", WHERE_PATH_SIZE);
		FILE *psf = fopen(dest, "rb");
		if (psf) {

			char pas[PASSBUFFER];
			for (c=0;c<PASSBUFFER;c++) {
				int cha = fgetc(psf);
				if (cha == EOF) break;
				pas[c] = (char)cha;
			}

			//read all?
			if (c == PASSBUFFER) {
				//toggle bits
				int rot;
				for (int d=0;d<PASSBUFFER;d++) {
					rot = pas[d];
					rot = 255 - rot;
					pas[d] = (char)rot;
				}
				//get the password
				pas[8]=0;
				strcpy(editplayerpass, pas);
				//copy to editpass and simulate pressing ENTER on the name/pass screen...
				check_change_pass_command();
			}

			fclose(psf);
		}

		//try to load client configuration
		bool randomname = true; // give random name
		append_filename(dest, wheregamedir, "clconfig.txt", WHERE_PATH_SIZE);
		LOG1("dest for clconfig.txt = %s\n", dest);

		FILE *cfg = fopen(dest, "r");
		if (cfg) {
			char lebuf[4096];

			//read starting directory name
			sfxthemedir[0]=0;
			if (fgets(sfxthemedir, 256, cfg)) { // load sucessful
				//ok
				sfxthemedir[strlen(sfxthemedir) - 1] = 0;
				LOG1("sfxthemedir default = %s\n", sfxthemedir);
			}

			//read name
			if (fscanf(cfg, "%s", lebuf) == 1) { //if load sucessful
				lebuf[15]=0;		// max needed for name=15!

				//if not an asterisk, load name
				if (strcmp(lebuf, "*"))	{
					randomname=false;
					strcpy(playername, lebuf);
				}
			}

			//read addresses
			for (int i=0;i<MAX_GAMESPY;i++) {

				if (fscanf(cfg, "%s", lebuf) == 1) {
					lebuf[21]=0;		// max needed for IP=15!
					strcpy(gamespy[i].address, lebuf);
					strcpy(mgamespy[i].address, lebuf); //copy to master list too!
				}
			}

			fclose(cfg);
		}

		//give a random name
		if (randomname) {
			string nome_tri_legal = RandomName();
			strcpy(playername, nome_tri_legal.c_str() );
			FreeName(nome_tri_legal);
		}

		//no themes set yet
		validtheme = false;

		//no samples loaded -- important so unload_samples don't crash
		for (int n=0;n<NUM_OF_SAMPLES;n++)
			sample[n] = 0;

		//attempt to position search at the last theme directory
		char themepath[512];
		make_sfx_theme_path(themepath, sfxthemedir);

		LOG1("\ntheme searching '%s'\n", themepath);

		if (al_findfirst(themepath, &sfxthemeffblk, FA_DIREC)) {

			//sound theme not found. find the first one
			// skip "previous" and "current" DOS dirs
			make_sfx_theme_path(themepath, "*.*");

			int result;
			result = al_findfirst(themepath, &sfxthemeffblk, FA_DIREC);
			LOG2("res = %i   name = %s", result, sfxthemeffblk.name);
			if ((!strcmp(sfxthemeffblk.name, ".")) || (!strcmp(sfxthemeffblk.name, "..")))
				result = al_findnext(&sfxthemeffblk);
			LOG2("res = %i   name = %s", result, sfxthemeffblk.name);
			if ((!strcmp(sfxthemeffblk.name, ".")) || (!strcmp(sfxthemeffblk.name, "..")))
				result = al_findnext(&sfxthemeffblk);
			LOG2("res = %i   name = %s", result, sfxthemeffblk.name);

			if (result)
			{
				//no themes at all
				//do nothing
			}
			else {
				//found one
				set_theme_dir(sfxthemeffblk.name);
			}
		}
		else {
			//found, so load it.
			set_theme_dir(0);		//0=sfxthemedir already set
		}

		//refresh master!
		get_servers_from_master();

		return true;
	}

	//send "client ready" message to server (when map load and/or download completes)
	void send_client_ready() {
		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 21);		// 21 = "client ready"
		client->send_message(lebuf, count);		// bem curtinha a mensagem mesmo...
	}

	// check if password has changed.
	// if NO, do nothing
	// if YES: send a new request to the master server
	// V0.4.6 : always re-log client if ENTER pressed! (if player doesn't want to log again, should press ESC instead)
	void check_change_pass_command() {

		//password changed
		// V0.4.6: check removed
		//if (strcmp(editplayerpass, player_password))

		//player_password_set = false is a flag for the token retrieve thread
		//join with it
		if (player_password_set) {
			player_password_set = false;
			pthread_join( passthread, 0 );
		}

		//NO TOKEN, not anymore...
		player_token_set = false;

		//empty?
		if (editplayerpass[0] == 0) {

			strcpy(namestatus, "NO PASSWORD SET");
			namestatus_code = 0;
			player_password[0]=0;

		}
		else {
			//non-empty! copy stuff
			strcpy(namestatus, "STARTING LOGIN...");
			strcpy(player_password, editplayerpass);

			//setup stuff for the new thread
			player_password_set = true;	//don't quit the thread
			player_token_new = true;	//getting a NEW token, not refreshing the token

			//*** request new token for this password --- a new, independent thread ***
			pthread_create(&passthread, 0, thread_clientpassword_f, (void *)this);	//will call function below (sucks but works)
		}
	}

	//THREAD for getting a token from a password. nonblocking TCP operations, if
	// player_password_set == false, then quit immediately
	void client_password_thread(void *arg) {

		NLsocket sock = NL_INVALID;

		while (player_password_set == true) {

			//open a nonblocking socket
			nlDisable(NL_BLOCKING_IO);
			sock = nlOpen(0, NL_RELIABLE);
			if (sock == NL_INVALID) {
				//show "cant open socket to master" error
				strcpy(namestatus, "SOCKET ERROR. RETRYING...");
				MS_SLEEP(3000);	//five secs
				continue;				//again...
			}

			//connect the nonblocking way
			nlConnect(sock, &master_address);

			//build query
			char blux[1024];
			if (player_token_new)
				sprintf(blux, "GET /servlet/fcecin.tk1/index.html?%s&new&name=%s&password=%s\n\n", TK1_VERSION_STRING, playername, player_password);
			else
				sprintf(blux, "GET /servlet/fcecin.tk1/index.html?%s&old&name=%s&password=%s\n\n", TK1_VERSION_STRING, playername, player_password);

			char querybuf[1024]; int qcount = 0;
			writeString(querybuf, qcount, blux);
			qcount--;	//take the zero out

			strcpy(namestatus, "SENDING LOGIN...");

			//keep trying to write the query.
			NLint result;
			do {
				result = nlWrite(sock, querybuf, qcount);
				MS_SLEEP(50);

				//qutting?
				if (player_password_set == false)
					break;

			} while ((result == NL_INVALID) && (nlGetError() == NL_CON_PENDING));

			//qutting?
			if (player_password_set == false)
				continue;

			strcpy(namestatus, "WAITING RESPONSE...");

			//try to read the reply
			//parse the response (should be <HTML><BODY> etc... with "@I @I @I ... @K" on it
			bool html_end = false;
			int nostuffcound = 0;
			char lebuf[65536];
			int n = 0;
			do {

				//read
				result = nlRead(sock, &(lebuf[n]), 1);

				//quitting?
				if (player_password_set == false)
					break;

				//no byte
				if (result == 0) {

					if (nostuffcound > 0) {
						nostuffcound++;
						//200 (4000*50/1000) seconds after it came some stuff but now without coming more stuff
						// THEN: retry in a while
						if (nostuffcound > 4000) {
							//retry
							lebuf[0] = 0;
							nlClose(sock);
							sock = NL_INVALID;
							strcpy(namestatus, "NO RESPONSE. RETRYING...");
							MS_SLEEP(3000);
							break;
						}
					}

					MS_SLEEP(50);
				}

				//error occured
				if (result == NL_INVALID) {

					//if already got html_end, no error
					//if (html_end)  // *** FIXME: parsing the result?
					//break;

					//error: try again
					nlClose(sock);
					sock = NL_INVALID;
					strcpy(namestatus, "ERROR. RETRYING...");
					MS_SLEEP(3000);
					break;
				}

				//received anything below 32: turn them into "+" signals...
				if (lebuf[n] < 32)
					lebuf[n] = '+';

				//check for received </HTML>
				if (n >= 6) {
					if (
						(lebuf[n-6] == '<') &&
						(lebuf[n-5] == '/') &&
						((lebuf[n-4] == 'h') || (lebuf[n-4] == 'H')) &&
						((lebuf[n-3] == 't') || (lebuf[n-3] == 'T')) &&
						((lebuf[n-2] == 'm') || (lebuf[n-2] == 'M')) &&
						((lebuf[n-1] == 'l') || (lebuf[n-1] == 'L')) &&
						(lebuf[n-0] == '>')
					)
					{
						//LOG1("CLIENT MASTER QUERY RECEIVED </HTML>! SUCCESS!! n=%i\n", n);
						html_end = true;
						lebuf[n+1] = 0;
						//LOG1("---- Full response START ----\n%s\n---- Full response END ----\n", lebuf);
						break;
					}
				}

				//check for received another <HTML> : reset all stuff
				if (n >= 5) {
					if (
						(lebuf[n-5] == '<') &&
						((lebuf[n-4] == 'h') || (lebuf[n-4] == 'H')) &&
						((lebuf[n-3] == 't') || (lebuf[n-3] == 'T')) &&
						((lebuf[n-2] == 'm') || (lebuf[n-2] == 'M')) &&
						((lebuf[n-1] == 'l') || (lebuf[n-1] == 'L')) &&
						(lebuf[n-0] == '>')
					)
					{
						lebuf[n+1]=0;
						//LOG1("\n** READ <HTML>, DISCARDING BUFFER '%s' **\n", lebuf);
						n = -1;
					}
				}

				//read next
				n++;

			} while (1);

			//found it?
			if (html_end) {

				//FIRST THINGS FIRST: close the socket
				nlClose(sock);
				sock = NL_INVALID;

				//parse the result (FIXME)
				//if SUCCESS (got token) then semi-busy-wait until told to quit or time
				//  to send a new token request
				//if FAILED then update the status and quit

				strcpy(namestatus, "RECEIVED RESPONSE!");

				bool ok = false, wrongid = false, unavailable = false;
				int i;
				LOG1("RECV RESPONSE n = %i\n", n);
				for (i=0;i<n;i++) {

					//0.4.7: "can't contact servlet runner.." > service unavailable
					if ((i>21)
						&&
						(
						((lebuf[i-21] == 'C') || (lebuf[i-21] == 'c')) &&
						((lebuf[i-20] == 'O') || (lebuf[i-20] == 'o')) &&
						((lebuf[i-19] == 'N') || (lebuf[i-19] == 'n')) &&
						((lebuf[i-18] == 'T') || (lebuf[i-18] == 't')) &&
						((lebuf[i-17] == 'A') || (lebuf[i-17] == 'a')) &&
						((lebuf[i-16] == 'C') || (lebuf[i-16] == 'c')) &&
						((lebuf[i-15] == 'T') || (lebuf[i-15] == 't')) &&
						((lebuf[i-14] == ' ') || (lebuf[i-14] == ' ')) &&
						((lebuf[i-13] == 'S') || (lebuf[i-13] == 's')) &&
						((lebuf[i-12] == 'E') || (lebuf[i-12] == 'e')) &&
						((lebuf[i-11] == 'R') || (lebuf[i-11] == 'r')) &&
						((lebuf[i-10] == 'V') || (lebuf[i-10] == 'v')) &&
						((lebuf[i-9] == 'L') || (lebuf[i-9] == 'l')) &&
						((lebuf[i-8] == 'E') || (lebuf[i-8] == 'e')) &&
						((lebuf[i-7] == 'T') || (lebuf[i-7] == 't')) &&
						((lebuf[i-6] == ' ') || (lebuf[i-6] == ' ')) &&
						((lebuf[i-5] == 'R') || (lebuf[i-5] == 'r')) &&
						((lebuf[i-4] == 'U') || (lebuf[i-4] == 'u')) &&
						((lebuf[i-3] == 'N') || (lebuf[i-3] == 'n')) &&
						((lebuf[i-2] == 'N') || (lebuf[i-2] == 'n')) &&
						((lebuf[i-1] == 'E') || (lebuf[i-1] == 'e')) &&
						((lebuf[i] == 'R') || (lebuf[i] == 'r')))) {
						unavailable = true;
					}
					//control char
					else if (lebuf[i] == '@') {

						//LOG("(( @ ))\n");
						i++;

						if (lebuf[i] == 'K') {

							//LOG("(( @K ))\n");
							//scan the token....
							i++;
							player_token[0]=0;
							int pt=0;
							while (lebuf[i] != '#') {
								//LOG("(( TOK ))\n");
								player_token[pt]=lebuf[i];		//cat char
								player_token[pt+1]=0;
								if (pt > 15) {
									//error... give up... tokens aren't so long...
									break;
								}
								i++;	//next.
								pt++;
							}
							//okeydokey...?
							if (pt <= 15) {
								ok = true;
							}
						}
						else if ((lebuf[i] == 'E') || (lebuf[i] == 'F')) //query Error / Failed login (wrong name/pass)
						{
							//LOG("(( @E || @F ))\n");
							wrongid = true;
						}
						else {
							//UNKNOWN code... yuck.
						}
					}
				}

				//OK?
				if (ok) {
					strcpy(namestatus, "LOGGED (");
					strcat(namestatus, player_token);
					strcat(namestatus, ")");
					namestatus_code = 1;

					//OK!
					player_token_set = true;
					player_token_new = false;

					//--- if connected, update token ---
					if (connected)
						send_player_token();

					//wait xxx minutes to send again	//*2 == halfsecond
#ifdef DEBUG_RANKING
					for (int busy=0;busy<20;busy++) {		//10 SECONDz
#else
					for (int busy=0;busy<60*10*2;busy++) {		//10 MINUTES
#endif
						MS_SLEEP(500);
						if (player_password_set == false)
							break;
					}
				}
				//WRONG ID?
				else if (wrongid) {
					strcpy(namestatus, "ERROR: WRONG ID!");
					namestatus_code = 2;
					LOG2("ERROR WRONG ID. QUERY='''%s''' LEBUF='''%s'''\n", blux, lebuf);
					//wait xxx minutes to send again	//*2 == halfsecond
					for (int busy=0;busy<60*10*2;busy++) {		//10 MINUTES
						MS_SLEEP(500);
						if (player_password_set == false)
							break;
					}
				}
				//UNAVAILABLE?
				else if (unavailable) {
					strcpy(namestatus, "SERVER UNAVAILABLE");
					namestatus_code = 2;
					LOG2("ERROR UNKNOWN!!! QUERY='''%s''' LEBUF='''%s'''\n", blux, lebuf);
					//wait xxx minutes to send again	//*2 == halfsecond
					for (int busy=0;busy<60*1*2;busy++) {		//1 MINUTE
						MS_SLEEP(500);
						if (player_password_set == false)
							break;
					}
				}
				//WHAT???
				else {
					strcpy(namestatus, "UNKNOWN ERROR!");
					namestatus_code = 2;
					LOG2("ERROR UNKNOWN!!! QUERY='''%s''' LEBUF='''%s'''\n", blux, lebuf);
					//wait xxx minutes to send again	//*2 == halfsecond
					for (int busy=0;busy<60*10*2;busy++) {		//10 MINUTES
						MS_SLEEP(500);
						if (player_password_set == false)
							break;
					}
				}
			}
			else {
				//failed, just retry (go on with the loop)
			}

		}//WHILE(password set)

		// =====
		//CLOSE SOCKET AND WAIT UNTIL SETPASS==FALSE
		// =====
		if (sock != NL_INVALID)
			nlClose(sock);
		while (player_password_set == true) {
			MS_SLEEP(500);
		}
	}

	// incoming chunk of requested file by UDP
	void process_udp_download_chunk(int last, NLulong pos, int len, char* buf) {

		//progress...
		fdp = pos + len;

		//write it
		fseek(ud_fout, pos, 0);		//0 == "SEEK_SET" ??
		int amount = fwrite(buf, 1, len, ud_fout);

		//this is bad! but will never happen.
		if (amount != len) {
			LOG2("BAD BAD ERROR! process_udp_download_chunk can't fwrite len %i amount %i !!!", len, amount);
			// FIXME: better handling
		}

		//send the reply
		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 29);		//29 = file chunk ACK
		writeLong(lebuf, count, pos);		// acked pos (just to be sure...)
		client->send_message(lebuf, count);

		//if done, then we're done!
		// - take out of queue
		// - notify app
		if (last) {
			pthread_mutex_lock ( &udpdq_mutex );

			//close the file
			fclose(ud_fout);

			// THANK GOD I DON'T HAVE TO REWRITE THIS!!
			download_file_complete(udpdq[udpdq_ptr]);

			// remove from queue - record deleted by download_file_complete() call above
			udpdq[udpdq_ptr] = 0;
			udpdq_size--;		//less one

			// no download
			udpdq_ptr = -1;

			// readily check for next download, if any on the queue
			//
			for (int i=0;i<MAX_UDPDQ;i++)
				if (udpdq[i] != 0) {
					//found: start the new download right now
					udpdq_ptr = i;
					client_udp_setup_download();
					break;
				}

			pthread_mutex_unlock ( &udpdq_mutex );
		}
	}

	//do the download setup
	//must be called with udpdq_mutex locked
	void client_udp_setup_download() {

		//to simplify things...
		download_runes_t  *r = udpdq[udpdq_ptr];

		//open dest file for output (ud_fout)
		ud_fout = fopen(r->dest, "wb");
		if (ud_fout) {
			LOG1("UDP client_download_thread() file '%s' opened\n", r->dest);
		}
		else {
			//do something if can't write to the file (disconnect player/whatever)
			LOG1("UDP client_download_thread() can't write output file!! (%s)", r->dest);
			disconnect_command();		//FIXME make it better
			return;
		}

		//reset ud_fp
		ud_fp = 0;			//file pointer (progress...)

		//request the file and wait...
		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 27);		//27= request file
		writeString(lebuf, count, r->type);
		writeString(lebuf, count, r->name);
		client->send_message(lebuf, count);
	}

	//add to UDP DOWNLOAD QUEUE
	void client_udp_download(download_runes_t  *rune) {

		pthread_mutex_lock ( &udpdq_mutex );

		for (int i=0;i<MAX_UDPDQ;i++)
			if (udpdq[i] == 0) {

				//add to empty pos on queue
				udpdq[i] = rune;		//copy to "queue"
				udpdq_size++;				//another one

				//setup ptr to download, if ptr free
				if (udpdq_ptr == -1) {
					udpdq_ptr = i;
					//check download
					client_udp_setup_download();
				}

				//anyway, we're done
				pthread_mutex_unlock ( &udpdq_mutex );
				return;
			}

		//error that will never happen even in a million years
		LOG("BAD BAD **ERROR** : UDPDQ IS FULL");
		throw 66640;	//BAD ERROR

		pthread_mutex_unlock ( &udpdq_mutex );
	}

	//file download complete
	void download_file_complete(download_runes_t  *r) {

		LOG3("download_file_complete '%s' '%s' '%s'\n", r->type, r->name, r->dest);

		//map complete
		if (!strcmp(r->type, "map")) {

			//if expected map, change now
			if (!strcmp(r->name, servermap)) {
				bool ok = load_map(CLIENT_MAPS_DIR, r->name, &map);
				if (!ok)
					LOG1("AFTER DOWNLOAD: MAP '%s' NOT FOUND\n", r->name)
				else {
					LOG1("AFTER DOWNLOAD: MAP '%s' LOADED SUCESSFULLY!\n", r->name);

					//load ok!  (FIXME: tell server)
					//
					update_minimap_background();		// recalc minimap
					map_ready = true;								// map ready to show
					send_client_ready();				//send "client ready" to server
				}
			}
		}

		//delete the download record
		delete r; r = 0;
	}

	//start downloading a server file
	void download_server_file(char *type, char *name, char *dest) {

		//new download request
		download_runes_t	*rune = new download_runes_t();
		strcpy(rune->type, type);
		strcpy(rune->name, name);
		strcpy(rune->dest, dest);

		// v0.4.4 if not using TCP socket, add to the download queue
		if (server_no_tcp || no_tcp_download) {

			//ADD TO QUEUE
			client_udp_download(rune);
		}
		else {

			//start new thread wich will call client_download_thread below (yes sucks but works)
			pthread_t			downloader_thread;
			pthread_create(&downloader_thread, 0, thread_clientdownloader_f, (void *)(rune));
		}
	}

	//downloads a server file
	void client_download_thread(void *arg) {

		//get pointer to runes
		download_runes_t  *r = (download_runes_t*)arg;

		//open blocking, thank god, TCP connection
		nlEnable(NL_BLOCKING_IO);
		NLsocket sok = nlOpen(0, NL_RELIABLE);
		nlDisable(NL_BLOCKING_IO);

		if (sok == NL_INVALID) {		//d'oh!
			LOG("ERROR client_download_thread() can't open socket!");

			//disconnect client here
			//disconnect_command();

			//V0.4.4: add to UDP download queue and set to not use TCP anymore
			no_tcp_download = true;
			client_udp_download(r);

			return;
		}

		//connect to server IP :

		//v0.4.2 : custom TCP port
		int tcp_port = 24999 - (port - 25000);

		char addr_n_port[256];
		sprintf(addr_n_port, "%s:%i", address, tcp_port);	//v0.4.2 custom TCP port
		NLaddress addr;
		nlStringToAddr(addr_n_port, &addr);

		NLboolean ok = nlConnect(sok, &addr);
		if (ok == NL_FALSE) {

			LOG1("ERROR client_download_thread() can't connect to %s!", addr_n_port);

			//disconnect socket
			nlClose(sok);

			//disconnect_command();

			//V0.4.4: add to UDP download queue and set to not use TCP anymore
			no_tcp_download = true;
			client_udp_download(r);

			return;
		}

		//request file
		char lebuf[65536];
		int count = 0;
		writeByte(lebuf, count, 1);		//1 = request file download
		writeString(lebuf, count, r->type);	//filetype
		writeString(lebuf, count, r->name);	//filename
		NLint result = nlWrite(sok, lebuf, count);
		// check result of write
		if (result != count) {

			LOG("ERROR client_download_thread() send error (1)!");

			nlClose(sok);
			disconnect_command();		//FIXME make it better
			return;
		}

		//download file
		result = nlRead(sok, lebuf, 1);		// read response byte
		//check result
		if (result != 1) {

			LOG1("ERROR client_download_thread() error reading response byte; result = %i", result);

			nlClose(sok);
			disconnect_command();		//FIXME make it better
			return;
		}

		NLubyte ans;
		count = 0;
		readByte(lebuf, count, ans);

		//FIXME: deal with other answers
		if (ans == 2)    // 2 = file request ok, sending file
		{
			//read file CRC
			result = nlRead(sok, lebuf, 2);
			if (result != 2) {

				LOG1("ERROR client_download_thread() error reading crc; result = %i", result);

				nlClose(sok);
				disconnect_command();		//FIXME make it better
				return;
			}
			NLushort incrc;
			count = 0;
			readShort(lebuf, count, incrc);

			//read file SIZE
			result = nlRead(sok, lebuf, 4);
			if (result != 4) {

				LOG1("ERROR client_download_thread() error reading filesize; result = %i", result);

				nlClose(sok);
				disconnect_command();		//FIXME make it better
				return;
			}
			NLulong filesize;
			count = 0;
			readLong(lebuf, count, filesize);

			//read the file in 1 big chunk
			result = nlRead(sok, lebuf, filesize);
			if (result != (int)filesize) {

				LOG2("ERROR client_download_thread() error reading file; result = %i filesize = %lu", result, filesize);

				nlClose(sok);
				disconnect_command();		//FIXME make it better
				return;
			}

			//write to the file
			FILE *fw = fopen(r->dest, "wb");
//size_t fwrite(const void* ptr, size_t size, size_t nobj, FILE* stream);
//Writes to stream stream, nobj objects of size size from array ptr. Returns number of objects written.
			if (fw) {
				int amount = fwrite(lebuf, 1, filesize, fw);

				fclose(fw);

				LOG3("client_download_thread() file '%s' written %i of %lu\n", r->dest, amount, filesize);

				//file download complete
				download_file_complete(r);
			}
			else {
				//do something if can't write to the file (disconnect player/whatever)

				LOG1("ERROR client_download_thread() can't write output file!! (%s)", r->dest);

				nlClose(sok);
				disconnect_command();		//FIXME make it better
				return;
			}

			//disconnect
			int count = 0;
			writeByte(lebuf, count, 3);		//3 = bye
			nlWrite(sok, lebuf, count);
			//FIXME: check result

			//wait a bit
			MS_SLEEP(3000);

			//drop connection
			nlClose(sok);
		}
		else {
			//unknown answer code

			LOG1("ERROR client_download_thread() answer not 2, it's %i", ans);

			nlClose(sok);
			disconnect_command();		//FIXME make it better
			return;
		}

		//close connection

		//do stuff in the event of download complete

		// FIXME: if error with connection, quit.

	}

	//server tells client of map change to builtin map
	//  num: number of builtin map (deve ser "1" que eh o unico)
	void server_builtin_map_command(NLubyte num) {

		if (num == 1) {
			load_default_map(&map);
		}
		else {
			//FIXME: "error": unknown builtin map number
			return;
		}

		//load ok!  (FIXME: tell server)
		//
		update_minimap_background();  // recalc minimap
		map_ready = true;
		send_client_ready();				//send "client ready" to server
	}

	//server tells client of current map / map change
	// client must attempt to load map from "cmaps" dir
	// if map file not there, or the CRC's don't match, ask to download the map from the server
	void server_map_command(char *mapname, NLushort server_crc) {

		LOG1("CLIENT: server_map_command : '%s'", mapname);

		//try to load the map. will fail if not found
		bool ok = load_map(CLIENT_MAPS_DIR, mapname, &map);

		if (!ok)
			LOG1("MAP '%s' NOT FOUND\n", mapname)
		else if (map.crc != server_crc)
			LOG3("MAP '%s' FOUND BUT IT'S CRC %i DIFFERS FROM SERVER MAP CRC %i\n", mapname, map.crc, server_crc)
		else {
			LOG1("MAP '%s' LOADED SUCESSFULLY!\n", mapname);

			//load ok!  (FIXME: tell server)
			//
			update_minimap_background();  // recalc minimap
			map_ready = true;  // map ready to show
			send_client_ready();				//send "client ready" to server
		}

		// download map from server (ask file)
		if (!ok || map.crc != server_crc) {

			char lix[256];
			sprintf(lix, "Client: downloading map '%s' (CRC %i)...", mapname, server_crc);
			print_message(lix);

			LOG(lix);

			// MAKE DOWNLOAD -- ASK FILE

			char fname[256];
			strcpy(fname, CLIENT_MAPS_DIR);
			put_backslash(fname);
			strcat(fname, mapname);
			strcat(fname, ".txt");

			char dest[1024];	//full destination path for file
			append_filename(dest, wheregamedir, fname, WHERE_PATH_SIZE);

			//copy to name of map waiting -- for when download_server_file completes
			strcpy(servermap, mapname);

			//download server file -- opens new thread and TCP conection
			download_server_file("map", mapname, dest);
		}
	}

	void next_sfx_theme() {

		char themepath[512];
		int result;

		//no valid theme, just give up...
		if (!validtheme) {
			return;
		}

		make_sfx_theme_path(themepath, sfxthemedir);
		result = al_findnext(&sfxthemeffblk);

		if (result) {
			//not found, go back to first ones...
			//sound theme not found. find the first one
			// skip "previous" and "current" DOS dirs
			make_sfx_theme_path(themepath, "*.*");

			LOG1("\ntheme searching '%s'\n", themepath);

			result = al_findfirst(themepath, &sfxthemeffblk, FA_DIREC);
			LOG2("res = %i   name = %s", result, sfxthemeffblk.name);
			if ((!strcmp(sfxthemeffblk.name, ".")) || (!strcmp(sfxthemeffblk.name, "..")))
				result = al_findnext(&sfxthemeffblk);
			LOG2("res = %i   name = %s", result, sfxthemeffblk.name);
			if ((!strcmp(sfxthemeffblk.name, ".")) || (!strcmp(sfxthemeffblk.name, "..")))
				result = al_findnext(&sfxthemeffblk);
			LOG2("res = %i   name = %s", result, sfxthemeffblk.name);

			if (result)
			{
				//no themes at all
				validtheme = false;

			}
			else {
				set_theme_dir(sfxthemeffblk.name);
			}
		}
		else
			//found
			set_theme_dir(sfxthemeffblk.name);

	}

	void make_sfx_theme_path(char *themepath, char *themedir) {

		char soundname[1024];

		strcpy(soundname, "sound");  //sound/
		put_backslash(soundname);
		strcat(soundname, themedir);  //theme dir name

		char dest[1024];
		append_filename(dest, wheregamedir, soundname, WHERE_PATH_SIZE);

		strcpy(themepath, dest);

		LOG1("make sfx theme path = '%s'\n", themepath);
	}

	void set_theme_dir(char *dirname) {

		if (dirname)
			strcpy(sfxthemedir, dirname);

		validtheme = true;

		unload_samples();		//unload old (if any)

		load_samples();			//load new

		// load sfx theme description
		//
		char soundname[256];
		strcpy(soundname, "sound");

		put_backslash(soundname);
		strcat(soundname, sfxthemedir);

		put_backslash(soundname);
		strcat(soundname, "theme.txt");

		char dest[WHERE_PATH_SIZE];
		append_filename(dest, wheregamedir, soundname, WHERE_PATH_SIZE);

		FILE *theme = fopen(dest, "r");
		if (theme) {
			if (fgets(sfxthemename, 256, theme)) {
				sfxthemename[strlen(sfxthemename)-1] =0;
			}
			else
				strcpy(sfxthemename, "(unnamed theme)");
			fclose(theme);
		}

		//play a sample
		sound( rand() % NUM_OF_SAMPLES );

	}

	//append the correct path
	SAMPLE *load_outgun_sample(char *fname, int slot, bool try_redirect = true, bool reverse = false) {

		//soundname: add "sound/" to the filename
		char soundname[256];
		strcpy(soundname, "sound");

		//additional: sfx theme dir name
		put_backslash(soundname);
		strcat(soundname, sfxthemedir);

		put_backslash(soundname);
		strcat(soundname, fname);
		strcat(soundname, ".wav");

		//add soundname to where game dir
		char dest[WHERE_PATH_SIZE];
		append_filename(dest, wheregamedir, soundname, WHERE_PATH_SIZE);

		//try load
		SAMPLE* ret = sample[slot] = load_sample(dest);

		//sample must be played in reverse?
		sample_reverse[slot] = reverse;

		LOG4("load_sample[%i]: '%s' = %p  rev = %i\n", slot, dest, ret, sample_reverse[slot]);

		//V0.3.10: if not found, look for .txt redirect
		if (try_redirect)	// don't go into endless loop
		if (ret == 0) {

			//txt filename
			strcpy(soundname, "sound");
			put_backslash(soundname);
			strcat(soundname, sfxthemedir);
			put_backslash(soundname);
			strcat(soundname, fname);
			strcat(soundname, ".txt");
			append_filename(dest, wheregamedir, soundname, WHERE_PATH_SIZE);

			FILE *f = fopen(dest, "r");
			if (f) {
				char redirwavname[256];
				fscanf(f, "%s", redirwavname);
				bool is_reversed = false;

				// versao 0.4.1 : EARLY OPTIMIZATION IS THE ROOT OF ALL EVIL
				// versao 0.4.1 : EARLY OPTIMIZATION IS THE ROOT OF ALL EVIL
				// versao 0.4.1 : EARLY OPTIMIZATION IS THE ROOT OF ALL EVIL
				//if (!strcmp("REVERSE", redirwavname)) {
				//	is_reversed = true;	//want reversed
				//	fscanf(f, "%s", redirwavname);		//scan again the name of the wav
				//}

				fclose(f);

				//retry once ("false": don't try redirect again if fails)
				return load_outgun_sample(redirwavname, slot, false, is_reversed);
			}
		}

		return ret;
	}

	//sample try loads
	void load_samples() {

		if (!sound_inited) return;
		load_outgun_sample("fire", SAMPLE_FIRE);
		load_outgun_sample("hit", SAMPLE_HIT);
		load_outgun_sample("wallhit", SAMPLE_WALLHIT);	//new 0.3.9
		load_outgun_sample("qwallhit", SAMPLE_QUADWALLHIT);	//new 0.3.9

		load_outgun_sample("getdb", SAMPLE_GETDEATHBRINGER);	// new! v0.3.9 -- get deathbringer powerup (voz sinistra)
		load_outgun_sample("usedb", SAMPLE_USEDEATHBRINGER);	// new! v0.3.9 -- use deathbringer powerup (carrier dies) ("GRRRAAWWKKLLLL!!")
		load_outgun_sample("hitdb", SAMPLE_HITDEATHBRINGER);	// new! v0.3.9 -- target is hit by the deathbringer ("PWRRLLW!")
		load_outgun_sample("diedb", SAMPLE_DIEDEATHBRINGER);	// new! v0.3.9 -- target dies by the deathbringer		("HaHaHaHa!")

		load_outgun_sample("death1", SAMPLE_DEATH);
		load_outgun_sample("death2", SAMPLE_DEATH_2);
		//sample[SAMPLE_RESPAWN] = load_outgun_sample("respawn");
		load_outgun_sample("entergam", SAMPLE_ENTERGAME);
		load_outgun_sample("leftgam", SAMPLE_LEFTGAME);
		load_outgun_sample("chanteam", SAMPLE_CHANGETEAM);
		load_outgun_sample("talk", SAMPLE_TALK);
		load_outgun_sample("wabounce", SAMPLE_WALLBOUNCE);

		load_outgun_sample("weaponup", SAMPLE_WEAPON_UP);  //new
		load_outgun_sample("megaheal", SAMPLE_MEGAHEALTH); // new
		load_outgun_sample("shieldp", SAMPLE_SHIELD_PICKUP);
		load_outgun_sample("shieldd", SAMPLE_SHIELD_DAMAGE);
		load_outgun_sample("shieldl", SAMPLE_SHIELD_LOST);
		load_outgun_sample("speedon", SAMPLE_BOOTS_ON);
		load_outgun_sample("speedoff", SAMPLE_BOOTS_OFF);
		load_outgun_sample("quadon", SAMPLE_QUAD_ON);
		load_outgun_sample("quadfire", SAMPLE_QUAD_FIRE);
		load_outgun_sample("quadoff", SAMPLE_QUAD_OFF);
		load_outgun_sample("helmon", SAMPLE_HELM_ON);
		load_outgun_sample("helmoff", SAMPLE_HELM_OFF);

		load_outgun_sample("got", SAMPLE_CTF_GOT);
		load_outgun_sample("lost", SAMPLE_CTF_LOST);
		load_outgun_sample("return", SAMPLE_CTF_RETURN);
		load_outgun_sample("capture", SAMPLE_CTF_CAPTURE);
		load_outgun_sample("gameover", SAMPLE_CTF_GAMEOVER);
	}

	//unload samples
	void unload_samples() {
		if (!sound_inited) return;
		for (int i=0;i<NUM_OF_SAMPLES;i++)
			if (sample[i])
				destroy_sample(sample[i]);
	}

	//play sample
	void sound(int s) {
		if (sound_enabled)
		if (sample[s]) {

			//kill any voice playing that sample
			stop_sample(sample[s]);

			// versao 0.4.1 : EARLY OPTIMIZATION IS THE ROOT OF ALL EVIL
			// versao 0.4.1 : EARLY OPTIMIZATION IS THE ROOT OF ALL EVIL
			// versao 0.4.1 : EARLY OPTIMIZATION IS THE ROOT OF ALL EVIL
			/*
			if (sample_reverse[s]) {

				//play_sample(sample[s], 255, 127, 1000, false);		//reversed play

				//allocate new voice
				int v = allocate_voice(sample[s]);

				//set up to play backwards
				voice_set_playmode(v, PLAYMODE_BACKWARD);


				//go!
				voice_start(v);
			}
			else
			*/
				//regular play
				play_sample(sample[s], 255, 127, 1000, false);		//regular play
		}
	}

	//clear clientside fx's
	void clear_fx() {
		for (int i=0;i<MAX_CLIENTFX;i++)
			cfx[i].used = false;
	}

	//find new clientside fx
	int get_new_cfx() {
		for (int i=0;i<MAX_CLIENTFX;i++)
		if (!cfx[i].used)
			return i;
		//print_message("overflow");
		return rand() % MAX_CLIENTFX;	//overwrite algum sorteado....
	}

	//create wall explosion fx
	void cfx_create_wallexplo(int x, int y, int px, int py) {

		int f = get_new_cfx();

		cfx[f].used = true;
		cfx[f].type = 2;		// WALL EXPLOSION
		cfx[f].x = x;
		cfx[f].y = y;
		cfx[f].time = get_time();
		cfx[f].px = px;
		cfx[f].py = py;

		//sound
		sound(SAMPLE_WALLHIT);
	}

	//create quad wall explosion fx
	void cfx_create_quadwallexplo(int x, int y, int px, int py) {

		int f = get_new_cfx();

		cfx[f].used = true;
		cfx[f].type = 3;		// QUAD WALL EXPLOSION
		cfx[f].x = x;
		cfx[f].y = y;
		cfx[f].time = get_time();
		cfx[f].px = px;
		cfx[f].py = py;

		//sound
		sound(SAMPLE_QUADWALLHIT);
	}

	//create deathbringer explosion fx
	void cfx_create_deathbringer(int owner, double start_time, int x, int y, int px, int py) {

		int f = get_new_cfx();

		cfx[f].used = true;
		cfx[f].owner = owner;		//deathbringer owner
		cfx[f].type = 4;		// DEATHBRINGER EXPLOSION
		cfx[f].x = x;
		cfx[f].y = y;
		cfx[f].time = start_time;
		cfx[f].px = px;
		cfx[f].py = py;

		//sound
		sound(SAMPLE_USEDEATHBRINGER);
	}

	//create deathbringer carrier trail fx
	void cfx_create_deathcarrier(int x, int y, int px, int py, int team) {

		int f = get_new_cfx();

		cfx[f].used = true;
		cfx[f].type = 5;	//death carrier cloud fx
		cfx[f].x = x;
		cfx[f].y = y;
		cfx[f].px = px;
		cfx[f].py = py;
		cfx[f].time = get_time();

		//owner: set color
		int r = rand() %100;
		if (team) {
			if (r < 50)
				cfx[f].col1 = makecol(0,0,0xff);
			else if (r < 75)
				cfx[f].col1 = makecol(0,0xff,0);
			else
				cfx[f].col1 = 0;
		} else {
			if (r < 50)
				cfx[f].col1 = makecol(0xff,0,0);
			else if (r < 75)
				cfx[f].col1 = makecol(0,0xff,0);
			else
				cfx[f].col1 = 0;
		}

		//JUST BLACK
		cfx[f].col1 = 0;
	}

	//create explosion fx
	void cfx_create_gunexplo(int x, int y, int px, int py) {

		int f = get_new_cfx();

		cfx[f].used = true;
		cfx[f].type = 0;		// GUN EXPLOSION
		cfx[f].x = x;
		cfx[f].y = y;
		cfx[f].time = get_time();
		cfx[f].px = px;
		cfx[f].py = py;

		//sound
		sound(SAMPLE_HIT);
	}

	//create speed bolinha fx
	void cfx_create_speedfx(int x, int y, int px, int py, int col1, int col2, int gundir) {

		int f = get_new_cfx();

		cfx[f].used = true;
		cfx[f].type = 1;	//speed fx
		cfx[f].x = x;
		cfx[f].y = y;
		cfx[f].px = px;
		cfx[f].py = py;
		cfx[f].time = get_time();

		cfx[f].col1 = col1;
		cfx[f].col2 = col2;
		cfx[f].gundir = gundir;
	}

	//update the scoreboard
	void update_scoreboard() {

		//reset used players/used scoreboard entries
		bool scoreused[MAX_PLAYERS];
		for (int f=0;f<MAX_PLAYERS;f++) {
			scoreused[f] = false;
			scoreboard[f] = -1;
		}

		// fill each team
		for (int t=0;t<2;t++)	{

			//team delta
			int td = t * TSIZE;

			//itera do 1o ao 8o slot
			for (int s=td;s<TSIZE+td;s++) {

				//itera do 1o jogador ao 8o jogador do time
				//busca o maior que ainda nao foi usado
				int maxfrag = -666;
				int maxwho = -1;
				for (int i=td;i<TSIZE+td;i++)
				if (player[i].used)
				if (!scoreused[i])		// ainda nao usado
				if (player[i].frags > maxfrag) {
					//achou maior
					maxfrag = player[i].frags;
					maxwho = i;
				}

				//aloca se achou
				if (maxwho != -1) {
					scoreboard[s] = maxwho;
					scoreused[maxwho] = true;
				}

			}//itera slots
		}//itera times
	}

	//calc the game frame
	void calc_game_frame() {

		int i;

		pthread_mutex_lock( &frame_mutex );

		//frame was skipped
		if (fx.skipped) {
			fd.skipped = true;
			pthread_mutex_unlock( &frame_mutex );
			return;
		}

		//make fancy extrapolation from the most recent frame from the server

		if (fx.frame > 0)	// valid?
		{
			//calcula framedelta (d)
			fd.time = get_time();
			double d = (fd.time - fx.time) / 0.1;

			//just to draw
			fd.frame = fx.frame + d;

			//player extrapolation
			//
			hero_t  *h;
			for (i=0;i<maxplayers;i++)

			if (player[i].onscreen) {	// nao eh suficiente usar platyer[i].USED !!!
				//copy all to fill in holes
				memcpy(&fd.hero[i], &fx.hero[i], sizeof(hero_t));

				if (player[i].x<0 || player[i].y<0 || player[i].x>=map.w || player[i].y>=map.h) continue;	//#NR remove this and track why these are given sometimes
				const Room& room = map.room[player[i].x][player[i].y];
				bool carryFlag = fx.flag[1-(i/TSIZE)].carried && fx.flag[1-(i/TSIZE)].carrier == i;

				h = &fd.hero[i];

				//delta counter
				double dc, f;
				dc = d;

				while (dc > 0) {
					//calc amount of movement
					f = dc;
					if (f > 1.0)
						f = 1.0;

					//dec dc
					dc -= 1.0;

					//run physics

					#ifdef NR_SERVER_PHYSICS
					if (NR_applyPhysics(&fd.hero[i], room, f, player[i].item_speed, carryFlag, player[i].deathbringer_affected)) {
						//player bounced: play bounce sample if minimum time elapsed
						if (get_time() > player[i].wall_sound_time) {
							player[i].wall_sound_time = get_time() + 0.2;
							sound(SAMPLE_WALLBOUNCE);
						}
					}
					#else	// NR_SERVER_PHYSICS
					#ifdef NR_FIX_BOUNCING
					bool fixBouncing = true;
					#else
					bool fixBouncing = false;
					#endif
					if (applyDefaultPhysics(&fd.hero[i], room, f, player[i].item_speed, carryFlag, player[i].deathbringer_affected, fixBouncing)) {
						//player bounced: play bounce sample if minimum time elapsed
						if (get_time() > player[i].wall_sound_time) {
							player[i].wall_sound_time = get_time() + 0.2;
							sound(SAMPLE_WALLBOUNCE);
						}
					}
					#endif	// NR_SERVER_PHYSICS else
				}
			}

			//rocket "interpolation"?
			//
			for (i=0;i<MAX_ROCKETS;i++)
			if (fx.rock[i].owner != -1)
			{
				rocket_c *rd = &(fd.rock[i]);
				rocket_c *rx = &(fx.rock[i]);

				//still drawing only - update pos
				if (!rx->dontdraw) {
					//find pos for draw
					// pos = startpos + sin/cos deg * timetravel * speed
					rd->x = (int)( rx->x + (fd.frame - rx->cl_time) * cos(rx->deg) * ROCKET_SPEED );
					rd->y = (int)( rx->y + (fd.frame - rx->cl_time) * sin(rx->deg) * ROCKET_SPEED );
				}

				//SPECIAL CASE: check if rocket just died
				if ((rx->hit_time > 0) && (get_time() > rx->hit_time))
				{
					rx->owner = -1;		// nao rola mais
					rd->x = rx->hitx;	// hit coords
					rd->y = rx->hity;

					//quad-hit wall?
					/*
					FIXED 0.3.9 (2) : wall collision+explosion is totally clientside

					if (rx->hit_target == 253) {

						//spawn clientside fx
						cfx_create_quadwallexplo((int)rd->x, ((int)rd->y) - 10, rx->px, rx->py);

					}
					//hit wall?
					else if (rx->hit_target == 254) {

						//spawn clientside fx
						cfx_create_wallexplo((int)rd->x, ((int)rd->y) - 10, rx->px, rx->py);

					}
					*/
					//just removing rocket == NOP
					//else
					if (rx->hit_target == 255) {
					}
					//hit player
					else {

						// blink player if not hit shield (252)
						if (rx->hit_target < 250)
							player[rx->hit_target].hitfx = get_time() + 0.3;

						//spawn clientside fx
						cfx_create_gunexplo((int)rd->x, ((int)rd->y) - 10, rx->px, rx->py);
					}
				}
				//else if still drawing check collisions/out of screen
				else if (!rx->dontdraw) {

					//0.3.9: check rocket hit a wall (clientside) if not vanished already
					if (map.fall_on_wall(rx->px, rx->py, (int)rd->x-2, (int)rd->y-NR_SHIFTY-2, (int)rd->x+2, (int)rd->y-NR_SHIFTY+2)) {
						//probably hit wall
						rx->dontdraw = true;
						rx->clremove = get_time() + 5.0;
						// IF the rocket is in the same room of "me" player
						if (rx->px == player[me].x)
						if (rx->py == player[me].y) {
							//then SPAWN a client-side hit-wall FX for the rocket
							if (player[rx->owner].item_quad)
								cfx_create_quadwallexplo((int)rd->x, ((int)rd->y) - 10, rx->px, rx->py);	//quad hit wall
							else
								cfx_create_wallexplo((int)rd->x, ((int)rd->y) - 10, rx->px, rx->py);		//normal hit wall
						}
					}
					// check out of screen (erase)
					else if ((rd->x < 0) || (rd->y < 5) || (rd->x > plw) || (rd->y > plh)) {
						rx->dontdraw = true;
						rx->clremove = get_time() + 1.0;	//enough...
					}
				}

				// check rocket expired
				if (rx->dontdraw)
				if (get_time() <= rx->clremove)
					rx->owner = -1;	// erase from clientside simulation
			}
		}

		pthread_mutex_unlock( &frame_mutex );
	}

	//draw a flag  t=team 0/1   x,y: coord relative to playarea
	void draw_flag_at(BITMAP *drawbuf, int t, int x, int y) {
		//draw shadow
		ellipsefill(drawbuf,
			plx + x,
			ply + y,
			12, 3, col[COLSHADOW]
		);
		//draw mastro
		rectfill(drawbuf,
			plx + x - 3,
			ply + y - 40,
			plx + x + 3,
			ply + y,
			col[COLYELLOW]
		);
		//draw bandeira
		rectfill(drawbuf,
			plx + x,
			ply + y - 38,
			plx + x + 20,
			ply + y - 20,
			teamcol[t]
		);
	}

	//draw minimap flag
	void draw_mini_flag(BITMAP *drawbuf, int whatteam) {

		int f = whatteam;

			double px, py;
			px = ((double)fx.flag[f].pos.px * (double)plw + fx.flag[f].pos.x) / ((double)plw * map.w);
			py = ((double)fx.flag[f].pos.py * (double)plh + fx.flag[f].pos.y) / ((double)plh * map.h);
			int pix = mmx + 21 + ((int)(px*98));
			int piy = mmy + 01 + ((int)(py*98));

			//draw mastro
			rectfill(drawbuf, pix, piy-5, pix, piy, col[COLYELLOW]);
			//draw bandeira
			rectfill(drawbuf, pix+1,piy-5,pix+5,piy-2,teamcol[f]);
	}

	//update the minimap background
	void update_minimap_background() {
		LOG2("update_minimap map.w = %i map.h = %i\n", map.w, map.h);
		map.draw_minimap(minibg);
	}

	//draws a player object
	void draw_player(BITMAP *drawbuf, int x, int y, int gundir, int pc1, int pc2, int alpha) {

		//draw the gun (direction facing)
		int xg,yg;
		switch (gundir) {
		case 0: xg=40;yg=0;
			break;
		case 1: xg=28;yg=28;
			break;
		case 2: xg=0;yg=40;
			break;
		case 3: xg=-28;yg=28;
			break;
		case 4: xg=-40;yg=0;
			break;
		case 5: xg=-28;yg=-28;
			break;
		case 6: xg=0;yg=-40;
			break;
		case 7: xg=28;yg=-28;
			break;
		default: xg=0;yg=0;
			break;
		}
		xg = (int)( ((double)xg) * 0.7);
		yg = (int)( ((double)yg) * 0.7);

		xg += x;
		yg += y - 15;

		if (alpha < 255) {
			set_trans_blender(0,0,0,alpha);
			drawing_mode(DRAW_MODE_TRANS, 0,0,0);
		}

		//desenha arma antes se dir 5,6,7
		if (gundir >= 5) {
			line(drawbuf, 0+plx + x, 0+ply + y - 15, 0+plx + xg, 0+ply + yg, pc1);
			line(drawbuf, 1+plx + x, 0+ply + y - 15, 1+plx + xg, 0+ply + yg, pc1);
			line(drawbuf, 1+plx + x, 1+ply + y - 15, 1+plx + xg, 1+ply + yg, pc1);
		}

		// the player is an ugly thin ellipse! 30=player's height x2 (60)
		// REMENDO: nao vai ter eixo Z mesmo....
		// outer color: team color
		circlefill(drawbuf, plx + x, ply + y - 15 - 0, 15, pc1);
		// inner color: self color
		circlefill(drawbuf, plx + x, ply + y - 15 - 0, 10, pc2);

		//desenha arma depois se dir 0,1,2,3,4
		if (gundir < 5) {
			line(drawbuf, 0+plx + x, 0+ply + y - 15, 0+plx + xg, 0+ply + yg, pc1);
			line(drawbuf, 1+plx + x, 0+ply + y - 15, 1+plx + xg, 0+ply + yg, pc1);
			line(drawbuf, 1+plx + x, 1+ply + y - 15, 1+plx + xg, 1+ply + yg, pc1);
		}

		if (alpha < 255)
			solid_mode();
	}

	//draw the whole game screen
	void draw_game_frame(BITMAP *drawbuf) {

		//static int HOWMANY = 0;
		//rectfill(drawbuf, 20, 20, 100, 40, makecol(rand(),rand(),rand()));
		//textprintf(drawbuf,font, 25,25,makecol(rand(),rand(),rand()),"HOW=%i",++HOWMANY);

		// erase old chat messages (this shouldn't be here really but wtf..)
		//
		if (chaterasetime < get_time())
			print_message("");

		//do not draw anything if "me" not known
		if (me < 0)
		if (me >= maxplayers) {
			clear_to_color(drawbuf, col[COLYELLOW]);
			return;
		}

		//lock frame mutex
		//LOG1("locking HOW=%i",HOWMANY);
		pthread_mutex_lock( &frame_mutex );
		//LOG1("locked! HOW=%i",HOWMANY);

		//SUPER USELESS MODE   //if (rand() % 10 < 2) SEEP(30);
		//clear_to_color(drawbuf, makecol(rand(),0,0));	// clear buffer

		// game screen background
		//
		clear_to_color(drawbuf, col[COLSHADOW]);

		// hiding stuff?
		// v0.4.1 : hide stuff if frame skipped
		bool hide_game = ((!map_ready) || (gameover_plaque != NEXTMAP_NONE) || (fx.skipped));

		// the PLAY AREA: border, walls and pits
		//
		if (hide_game) {

			//draw THE PLAQUE
			rectfill(drawbuf, plx, ply, plx + plw, ply + plh, 0);

			//DEBUG FIXME REMOVEME:
			//if (fx.skipped)
			//	textprintf_centre(drawbuf, font, plx+plw/2, ply+plh/2 - 80, col[COLDARKGRAY], "fx.skipped");

			//qual mensagem
			if ((gameover_plaque == NEXTMAP_CAPTURE_LIMIT) || (gameover_plaque == NEXTMAP_VOTE_EXIT)) {
				if (red_final_score > blue_final_score) {
					textprintf_centre(drawbuf, font, plx+plw/2, ply+plh/2 - 40, col[COLLRED], "RED TEAM WINS");
					textprintf_centre(drawbuf, font, plx+plw/2, ply+plh/2 - 20, col[COLLRED], "SCORE: %i x %i", red_final_score, blue_final_score);
				}
				else if (blue_final_score > red_final_score) {
					textprintf_centre(drawbuf, font, plx+plw/2, ply+plh/2 - 40, col[COLLBLUE], "BLUE TEAM WINS");
					textprintf_centre(drawbuf, font, plx+plw/2, ply+plh/2 - 20, col[COLLBLUE], "SCORE: %i x %i", blue_final_score, red_final_score);
				}
				else {
					textprintf_centre(drawbuf, font, plx+plw/2, ply+plh/2 - 40, col[COLMENUGRAY], "GAME TIED");
					textprintf_centre(drawbuf, font, plx+plw/2, ply+plh/2 - 20, col[COLMENUGRAY], "SCORE: %i x %i", blue_final_score, red_final_score);
				}
			}
			else {
				textprintf_centre(drawbuf, font, plx+plw/2, ply+plh/2, col[COLGREEN], "Connecting...");
			}

			if (map_ready) {
				textprintf_centre(drawbuf, font, plx+plw/2, ply+plh/2 + 20, col[COLGREEN], "Waiting game start - next map is:");
				textprintf_centre(drawbuf, font, plx+plw/2, ply+plh/2 + 50, col[COLORA], "%s", map.title.c_str());
			}
			else
				textprintf_centre(drawbuf, font, plx+plw/2, ply+plh/2 + 20, col[COLGREEN], "Loading map: %lu bytes", fdp);
		}
		else {

			// map ground
			//
			//0.4.0 check if base ground, then paint differently
			/*
			if ((player[me].x == map.tinfo[0].flag.px) && (player[me].y == map.tinfo[0].flag.py)) {
				rectfill(drawbuf, plx, ply, plx + plw, ply + plh, col[COLBRED]);
			}
			else if ((player[me].x == map.tinfo[1].flag.px) && (player[me].y == map.tinfo[1].flag.py)) {
				rectfill(drawbuf, plx, ply, plx + plw, ply + plh, col[COLBBLUE]);
			}
			//regular ground
			else
			*/
				rectfill(drawbuf, plx, ply, plx + plw, ply + plh, col[COLGROUND]);

			// place of flag
			for (int team = 0; team < 2; team++)
				if (player[me].x == map.tinfo[team].flag.px && player[me].y == map.tinfo[team].flag.py) {
					check_flagpos_marks();
					int flag_x = map.tinfo[team].flag.x;
					int flag_y = map.tinfo[team].flag.y;
					int x1 = max(0, FLAGPOS_RAD - flag_x);
					int y1 = max(0, FLAGPOS_RAD - flag_y);
					int x2 = min(2 * FLAGPOS_RAD, plw - flag_x + FLAGPOS_RAD + 1);
					int y2 = min(2 * FLAGPOS_RAD, plh - flag_y + FLAGPOS_RAD + 1);
					blit(flagpos_buf[team], drawbuf, x1, y1,
						plx + flag_x - FLAGPOS_RAD + x1, ply + flag_y - FLAGPOS_RAD + y1, x2 - x1, y2 - y1);
			}

			// map walls
			if (player[me].x >= 0 && player[me].y >= 0 && player[me].x < map.w && player[me].y < map.h)
				map.room[player[me].x][player[me].y].draw(drawbuf, plx, ply, 1., 1., col[COLWALL]);
		}

		// frame is valid?
		if (!hide_game)		// do not draw if map not set yet
		if (fd.frame >= 0) {

			int i;

			// FIXME: y-ordering of draw not maintained
			// draw any item pickups
			//
			if (me >= 0)
			for (i=0;i<MAX_PICKUPS;i++)
			if (fx.item[i].kind > 0)		// used pickups
			if (fx.item[i].kind != 255)	//not respawning
			if (fx.item[i].px == player[me].x)	// on my screen
			if (fx.item[i].py == player[me].y)
			{
				pickup_c *it = &fx.item[i];

				ellipsefill(drawbuf, plx + it->x, ply + it->y + 12, 12, 3, col[COLSHADOW]);

				//shield
				if (it->kind == 1) {

					ellipse(drawbuf, plx + it->x, ply + it->y, 14+rand()%3, 14+rand()%3, makecol(rand(),rand(),rand()));
					ellipse(drawbuf, plx + it->x, ply + it->y, 14+rand()%5, 14+rand()%5, makecol(rand(),rand(),rand()));
					ellipse(drawbuf, plx + it->x, ply + it->y, 14+rand()%9, 14+rand()%9, makecol(rand(),rand(),rand()));

					circlefill(drawbuf, plx + it->x, ply + it->y, 12, col[COLGREEN]);

				}
				//boots
				else if (it->kind == 2) {

					circlefill(drawbuf, plx + it->x + rand()%6-3, ply + it->y + rand()%6-3, 12, col[COLDARKORA]);
					circlefill(drawbuf, plx + it->x + rand()%8-4, ply + it->y + rand()%8-4, 12, col[COLORA]);
					circlefill(drawbuf, plx + it->x + rand()%12-6, ply + it->y + rand()%12-6, 12, col[COLYELLOW]);
				}
				//helm
				else if (it->kind == 3) {

					drawing_mode(DRAW_MODE_TRANS, 0,0,0);
					int alpha = ((int)(get_time() * 600.0)) % 400;
					if (alpha > 200)
						alpha = 400 - alpha;
					set_trans_blender(0,0,0,55+alpha);
					circlefill(drawbuf, plx + it->x, ply + it->y, 12, col[COLMAG]);
					solid_mode();
				}
				//instant kill shots
				else if (it->kind == 4) {

					if (((int)(get_time() * 30)) % 2)
						circlefill(drawbuf, plx + it->x, ply + it->y, 13, col[COLWHITE]);
					else
						circlefill(drawbuf, plx + it->x, ply + it->y, 11, col[COLCYAN]);
				}
				//weapon upgrade
				else if (it->kind == 5) {

					//graus de rotacao
					for (int b=0;b<4;b++) {
						//deg: 0..360
						double deg = ((int) (get_time() * 1000.0)) % 1000;		//thousand ticker
						deg = deg / 1000.0;		//0..1 inteiro
						deg = deg * 6.2832;		// 0..1 vezes 2 pi (1 volta)
						deg = deg + 1.5708 * b; //mais b's (90 grauses)

						//pos
						double dx = 10 * cos(deg);
						double dy = 10 * sin(deg);

						//draw a ball
						switch (b) {
						case 0: circlefill(drawbuf, plx + it->x + (int)dx, ply + it->y + (int)dy, 4, col[COLGREEN]); break;
						case 1: circlefill(drawbuf, plx + it->x + (int)dx, ply + it->y + (int)dy, 4, col[COLBLUE]); break;
						case 2: circlefill(drawbuf, plx + it->x + (int)dx, ply + it->y + (int)dy, 4, col[COLRED]); break;
						case 3: circlefill(drawbuf, plx + it->x + (int)dx, ply + it->y + (int)dy, 4, col[COLYELLOW]); break;
						}
					}
				}
				//megahealth
				else if (it->kind == 6) {

					//caixa de saude pulsante
					int varia = ((int)(get_time() * 15)) % 10;

					if (varia > 5)
						varia = 10 - varia;

					int itemsize = 11 + varia;
					int crossize = 8 + varia;
					int crosslar = 3;//aria/2;

					// health box black border
					rectfill(drawbuf, plx + it->x - itemsize - 2, ply + it->y - itemsize - 2, plx + it->x + itemsize + 2, ply + it->y + itemsize + 2, 0);

					// health box
					rectfill(drawbuf, plx + it->x - itemsize, ply + it->y - itemsize, plx + it->x + itemsize, ply + it->y + itemsize, col[COLWHITE]);

					// red cross
					rectfill(drawbuf, plx + it->x - crossize, ply + it->y - crosslar, plx + it->x + crossize, ply + it->y + crosslar, col[COLRED]);
					rectfill(drawbuf, plx + it->x - crosslar, ply + it->y - crossize, plx + it->x + crosslar, ply + it->y + crossize, col[COLRED]);
				}
				//deathbringer
				else if (it->kind == 7) {

					//bola preta
					circlefill(drawbuf, plx + it->x, ply + it->y, 12, makecol(0x22,0x33,0x22));

					//smoke da bola preta
					cfx_create_deathcarrier(it->x + rand()%30-15, it->y + rand()%30-5, it->px, it->py, 0);
				}
			}

			// draw clientside fx -- efeitos ATRAS das coisas
			//
			if (me >= 0)	// where am I?
			for (i=0;i<MAX_CLIENTFX;i++)
			if (cfx[i].used)	//fx used?
			if (cfx[i].px == player[me].x)	//on same screen?
			if (cfx[i].py == player[me].y)
			{
				double tim = get_time();

				//speed rastro
				if (cfx[i].type == 1) {

					double delta = tim - cfx[i].time;
					if (delta > 0.3) {
						cfx[i].used = false;
					}
					else {
						int alpha = 90 - ((int)(delta * 300.0));
						draw_player(drawbuf, cfx[i].x, cfx[i].y, cfx[i].gundir, cfx[i].col1, cfx[i].col2, alpha);
					}
				}

			}

			// FIXME: y-ordering of draw not maintained
			// draw any dropped flags (use fx since flags don't move)
			//
			for (int t=0;t<2;t++)
				if (me >= 0)
				if (fx.flag[t].carried == false)	// not carried == dropped
				if (fx.flag[t].pos.px == player[me].x)  // on same screen than me
				if (fx.flag[t].pos.py == player[me].y)  // on same screen than me
				{
					draw_flag_at(drawbuf, t, fx.flag[t].pos.x, fx.flag[t].pos.y);
				}

			// FIXME: y-ordering of draw not maintained
			// draw any rockets
			if (me >= 0)
			for (i=0;i<MAX_ROCKETS;i++)
			if (fx.rock[i].owner != -1)		// valid
			if (fx.rock[i].dontdraw == false)	// DO draw?
			if (fx.rock[i].px == player[me].x)	//in same screen
			if (fx.rock[i].py == player[me].y)
			{
				rocket_c *r = &(fd.rock[i]);

				//QUAD ROCKET!
				if (player[ fx.rock[i].owner ].item_quad) {
					//draw rocket shadow
					ellipsefill(drawbuf, plx + (int)r->x, ply + (int)r->y, 6, 3, col[COLSHADOW]);
					//draw the rocket
					if (((int)(get_time() * 30)) % 2)
						circlefill(drawbuf, plx + (int)r->x, ply + (int)r->y - 15, 6, col[COLWHITE]);	//y-12?
					else
						circlefill(drawbuf, plx + (int)r->x, ply + (int)r->y - 15, 4, teamlcol[fx.rock[i].team]); //y-12??
				}
				else {
					//draw rocket shadow
					ellipsefill(drawbuf, plx + (int)r->x, ply + (int)r->y, 4, 2, col[COLSHADOW]);
					//draw the rocket
					circlefill(drawbuf, plx + (int)r->x, ply + (int)r->y - 15, 4, teamcol[fx.rock[i].team]); //y-10??
				}
			}

			// sort order of drawing of the players
			//
			for (i=0;i<maxplayers;i++) {
				player[i].drawused = 0;
				player[i].drawptr = -1;
			}

			double miny;
			int minyid;

			for (i=0;i<maxplayers;i++) {
				minyid = -1;
				miny = 999999;

				for (int j=0;j<maxplayers;j++)
				if (player[j].used)	{
					if (player[j].drawused == 0)
					if (fd.hero[j].y < miny) {
						miny = fd.hero[j].y;
						minyid = j;
					}
				}

				if (minyid == -1)
					break;

				player[minyid].drawused = 1;
				player[i].drawptr = minyid;
			}

			// the PLAY AREA: the players!
			//
			for (int k=0;k<maxplayers;k++) {

				//HACK REMENDEX: predict item_helm
				if (player[i].item_helm > 0) {
					int hspd = (int) ((fd.time - fx.time) * 100.0);
					player[i].item_helm = player[i].item_helm - hspd;
					if (player[i].item_helm < 1)
						player[i].item_helm = 1;
				}

				//indirection: draw in y-order
				int i;
				i = player[k].drawptr;

				if (i >= 0)
				if (player[i].onscreen)		// draw only players on my screen
				{
					//calcula alfa do player
					//
					int alpha = 255;
					if (player[i].item_helm > 0) {
						drawing_mode(DRAW_MODE_TRANS, 0,0,0);
						alpha = player[i].item_helm;
						if (me >= 0)
						if (i/TSIZE == me/TSIZE)	// teammate or myself
						if (alpha < MIN_ALPHA_FRIENDS) alpha = MIN_ALPHA_FRIENDS;
						set_trans_blender(0,0,0,alpha);
					}

					// the player's shadow: showing last valid position
					ellipsefill(drawbuf, plx + (int)fx.hero[i].x, ply + (int)fx.hero[i].y, 15, 3, col[COLSHADOW]);

					if (player[i].item_helm > 0)
						solid_mode();

					// DRAW FLAG IF PLAYER IS CARRIER OF A FLAG
					for (int t=0; t<2; t++)
						if (fx.flag[t].carried == true)
						if (fx.flag[t].carrier == i)
						{
							draw_flag_at(drawbuf, t, (int)fd.hero[i].x, (int)fd.hero[i].y);
						}

					// se player morto, desenha poca de sangue
					if (player[i].dead) {

						// FIXME: draw different deathbringer death (green stuff?)

						// virou sorvete!
						if ((player[i].frags >= 10) && (player[i].frags % 10 == 0)) {
							ellipsefill(drawbuf, plx + (int)fx.hero[i].x, ply + (int)fx.hero[i].y - 15, 6, 15, col[COLORA]);
							circlefill(drawbuf, plx + (int)fx.hero[i].x - 8, ply + (int)fx.hero[i].y - 10-15, 8, col[COLBLUE]);
							circlefill(drawbuf, plx + (int)fx.hero[i].x + 8, ply + (int)fx.hero[i].y - 10-15, 8, col[COLMAG]);
							circlefill(drawbuf, plx + (int)fx.hero[i].x + 0, ply + (int)fx.hero[i].y - 20-15, 8, col[COLGREEN]);
							textprintf_centre(drawbuf, font, plx + (int)fx.hero[i].x + 0, ply + (int)fx.hero[i].y - 20-43, col[COLWHITE], "VIROU");
							textprintf_centre(drawbuf, font, plx + (int)fx.hero[i].x + 0, ply + (int)fx.hero[i].y - 20-33, col[COLWHITE], "SORVETE!");
						}
						else {
							ellipsefill(drawbuf, plx + (int)fx.hero[i].x, ply + (int)fx.hero[i].y, 20, 6, col[COLRED]);
							circlefill(drawbuf, plx + (int)fx.hero[i].x, ply + (int)fx.hero[i].y - 10, 12, col[COLRED]);
						}
					}
					// desenha player vivo
					else {

						// verifica turbo drop efeitos
						//
						if (player[i].item_speed)		// tem speed
						if (
								(fabs(fx.hero[i].sx) > svp_maxspeed)		// ta rapído
								||
								(fabs(fx.hero[i].sy) > svp_maxspeed)
							)
						if (get_time() > player[i].speed_drop_time)		// intervalo entre drop de efeito bolinha
						{
							//tempo minimo pra soltar outra bolinha fade
							player[i].speed_drop_time = get_time() + 0.05;

							//solta a bolinha
							cfx_create_speedfx((int)fx.hero[i].x, (int)fx.hero[i].y, player[i].x, player[i].y, teamcol[i/TSIZE], col[i%TSIZE], fx.hero[i].gundir);
						}

						//blink player when hit
						//
						int pc1,pc2;
						pc1 = teamcol[i/TSIZE];
						pc2 = col[i%TSIZE];
						double deltafx = player[i].hitfx - get_time();
						if (deltafx > 0) {
							NLubyte rgb = (NLubyte) ( 70.0 + deltafx * 600.0 );  // var 180
							pc1 = pc2 = makecol(rgb,rgb,rgb);
						}
						else if (player[i].item_quad) {
							//pisca branco
							if ( ((int)(get_time() * 10)) % 2 ) {
								pc1 = col[COLWHITE];
								pc2 = col[COLCYAN];
							}
						}

						//draw player
						draw_player(drawbuf, (int)fd.hero[i].x, (int)fd.hero[i].y, fd.hero[i].gundir, pc1, pc2, alpha);

						//draw deathbringer carrier effect
						if (player[i].item_deathbringer) {
							// intervalo entre drop de efeito
							if (get_time() > player[i].death_drop_time)
							{
								//tempo p/ proximo efeito
								player[i].death_drop_time = get_time() + 0.01;
								//drop it
								cfx_create_deathcarrier((int)fd.hero[i].x + rand()%40-20, (int)fd.hero[i].y + rand()%40-10, player[i].x, player[i].y, i/TSIZE);
								cfx_create_deathcarrier((int)fd.hero[i].x + rand()%40-20, (int)fd.hero[i].y + rand()%40-10, player[i].x, player[i].y, i/TSIZE);
							}
						}

						//draw deathbringer affected effect
						if (player[i].deathbringer_affected) {
							drawing_mode(DRAW_MODE_TRANS, 0,0,0);
							set_trans_blender(0,0,0,128);
							int q,co;
							if (i/TSIZE)
								co = makecol(0xA0,0,0);
							else
								co = makecol(0,0,0xff);
							for (q=0;q<5;q++)
								circlefill(drawbuf, plx + (int)fd.hero[i].x+rand()%40-20, ply + (int)fd.hero[i].y+rand()%40-20 - 15, 15, co);
							for (q=0;q<5;q++)
								circlefill(drawbuf, plx + (int)fd.hero[i].x+rand()%40-20, ply + (int)fd.hero[i].y+rand()%40-20 - 15, 15, 0);
							solid_mode();
						}

						// SHIELD FX!!
						if (player[i].item_shield) {
							ellipse(drawbuf, plx + (int)fd.hero[i].x, ply + (int)fd.hero[i].y - 15, 24+rand()%3, 24+rand()%3, makecol(rand(),rand(),rand()));
							ellipse(drawbuf, plx + (int)fd.hero[i].x, ply + (int)fd.hero[i].y - 15, 24+rand()%5, 24+rand()%5, makecol(rand(),rand(),rand()));
							ellipse(drawbuf, plx + (int)fd.hero[i].x, ply + (int)fd.hero[i].y - 15, 24+rand()%9, 24+rand()%9, makecol(rand(),rand(),rand()));
						}
					}

					//draw player's name -- nao interessa se vivo ou morto
					if (option_show_names)
					if (me >= 0)
					if (me < maxplayers)
					//NOT an invisible enemy
					if (!((player[i].item_helm) && (i/TSIZE != me/TSIZE))) {
						int ttx = (int)fd.hero[i].x + plx;
						int tty = (int)fd.hero[i].y + ply - 40;
						int supercol = teamdcol[i/TSIZE];
						int midcol = col[COLWHITE];

						textprintf_centre(drawbuf, font, ttx-1, tty-1, supercol, "%s", player[i].name);
						textprintf_centre(drawbuf, font, ttx+1, tty-1, supercol, "%s", player[i].name);
						textprintf_centre(drawbuf, font, ttx-1, tty+1, supercol, "%s", player[i].name);
						textprintf_centre(drawbuf, font, ttx+1, tty+1, supercol, "%s", player[i].name);
						textprintf_centre(drawbuf, font, ttx-1, tty , supercol, "%s", player[i].name);
						textprintf_centre(drawbuf, font, ttx+1, tty , supercol, "%s", player[i].name);
						textprintf_centre(drawbuf, font, ttx, tty-1, supercol, "%s", player[i].name);
						textprintf_centre(drawbuf, font, ttx, tty+1, supercol, "%s", player[i].name);

						textprintf_centre(drawbuf, font, ttx, tty, midcol, "%s", player[i].name);
					}
				}
			}

			// DEATHBRINGER: darken ground for carriers
			//
			for (int o=0;o<maxplayers;o++)
			if (player[o].used)	//valid
			if (player[o].x == player[me].x)	//in visible screen
			if (player[o].y == player[me].y)
			if (player[o].onscreen)						//REALLY in visible screen??
			if (player[o].item_deathbringer)		// has deathbringer
			{
				set_clip(drawbuf, plx, ply, plx + plw, ply + plh);
				//darken ground
				drawing_mode(DRAW_MODE_TRANS, 0,0,0);
				for (int i=50;i>0;i -= 5) {
					set_trans_blender(0,0,0,50-i);
					circlefill(drawbuf, plx + (int)fd.hero[o].x, ply + (int)fd.hero[o].y - 10, i, 0);
				}
				solid_mode();
				set_clip(drawbuf, 0, 0, drawbuf->w - 1, drawbuf->h - 1);
			}

			// draw clientside fx apos players
			//
			if (me >= 0)	// where am I?
			for (i=0;i<MAX_CLIENTFX;i++)
			if (cfx[i].used)	//fx used?
			if (cfx[i].px == player[me].x)	//on same screen?
			if (cfx[i].py == player[me].y)
			{
				double tim = get_time();

				//gun explosion
				if (cfx[i].type == 0) {

					double delta = tim - cfx[i].time;
					if (delta > 0.4)
						cfx[i].used = false;
					else {
						for (int e=0;e<3;e++) {
							int rad = 4 + e + (int)(delta * 40);
							circle(drawbuf, plx + cfx[i].x, ply + cfx[i].y, rad, makecol(rand(),rand(),rand()));
						}
					}
				}
				//wall explosion
				else if (cfx[i].type == 2) {

					double delta = tim - cfx[i].time;
					if (delta > 0.2)
						cfx[i].used = false;
					else {
						for (int e=0;e<2;e++) {
							int rad = 4 + e + (int)(delta * 40);
							circle(drawbuf, plx + cfx[i].x, ply + cfx[i].y, rad, makecol(rand(),rand(),rand()));
						}
					}
				}
				//quad wall explosion
				else if (cfx[i].type == 3) {

					double delta = tim - cfx[i].time;
					if (delta > 0.2)
						cfx[i].used = false;
					else {
						for (int e=0;e<3;e++) {
							int rad = 4 + e + (int)(delta * 60);
							circle(drawbuf, plx + cfx[i].x, ply + cfx[i].y, rad, makecol(rand(),rand(),rand()));
						}
					}
				}
				// deathcarrier rastro
				else if (cfx[i].type == 5) {

					double delta = tim - cfx[i].time;
					if (delta > 0.6) {
						cfx[i].used = false;
					}
					else {
						int alpha = 120 - ((int)(delta * 200.0));
						drawing_mode(DRAW_MODE_TRANS, 0,0,0);
						set_trans_blender(0,0,0,alpha);
						double drad = (3.0 + 9.0 * (0.6 - delta));
						int rad = (int)drad;
						int subdist = (int)( (96.0 - drad * 8.0) );
						circlefill(drawbuf, plx + cfx[i].x, ply + cfx[i].y - subdist, rad, cfx[i].col1);
						solid_mode();
					}
				}
				//the deathbringer
				else if (cfx[i].type == 4) {

					double delta = tim - cfx[i].time;
					if (delta > 3.0) {
						cfx[i].used = false;
					}
					else {
						set_clip(drawbuf, plx, ply, plx + plw, ply + plh);
						//radius
						int e,rad,co;
						if (delta < 1.0)
							rad = (int)(delta * 100);
						else
							rad = 100 + (int)((delta - 1.0) * (delta - 1.0) * 800);
						//brightening ring
						for (e=0;e<30;e++) {
							if (cfx[i].owner/TSIZE)
								co = makecol(0,0,14+8*e);
							else
								co = makecol(14+8*e,0,0);
							circle(drawbuf, plx + cfx[i].x, ply + cfx[i].y, rad, co);
							//ellipse(drawbuf, plx + cfx[i].x+1, ply + cfx[i].y, rad, rad, co);
							//ellipse(drawbuf, plx + cfx[i].x, ply + cfx[i].y+1, rad, rad, co);
							rad++;
						}
						//darkening ring
						for (e=0;e<10;e++) {
							if (cfx[i].owner/TSIZE)
								co = makecol(0,0,255-14*e);
							else
								co = makecol(255-14*e,0,0);
							circle(drawbuf, plx + cfx[i].x, ply + cfx[i].y, rad, co);
							circle(drawbuf, plx + cfx[i].x+1, ply + cfx[i].y, rad, co);
							circle(drawbuf, plx + cfx[i].x, ply + cfx[i].y+1, rad, co);
							rad++;
						}
						set_clip(drawbuf, 0, 0, drawbuf->w - 1, drawbuf->h - 1);
					}
				}

			}

		}

		//do not draw stuff below if map not ready to show
		if (!hide_game) {

			// the MINIMAP
			//
			blit(minibg, drawbuf, 0, 0, mmx + 20, mmy, minibg->w, minibg->h);	// minimap bg

			//draw the miniflags
			// - qualquer flag no chao (na base ou nao, carried == false)
			for (int f=0;f<2;f++)
			if (fx.flag[f].carried == false)
				draw_mini_flag(drawbuf, f);

			vector<bool> roomvis;
			roomvis.resize(map.w*map.h, (me>=0 && player[me].item_helm>0)?true:false);

			// draw all teammates and enemies on screens where there are teammates
			//draw all the players - put a pixel where they are
			if (me>=0 && fx.frame>=0)
				for (int i=0;i<maxplayers;i++)
					if (player[i].used && player[i].x>=0 && player[i].y>=0 && player[i].x<map.w && player[i].y<map.h &&
							(i/TSIZE == me/TSIZE || (player[me].enemyvis & (1<<(i%TSIZE)) ))) {
						roomvis[player[i].y*map.w+player[i].x] = true;

						// coord on minimap
						double px, py;
						px = ((double)player[i].x * (double)plw + fx.hero[i].x) / ((double)plw * map.w);
						py = ((double)player[i].y * (double)plh + fx.hero[i].y) / ((double)plh * map.h);
						int pix = mmx + 21 + ((int)(px*98));
						int piy = mmy +  1 + ((int)(py*98));

						//verifica se o jogador a ser desenhado é um carrier de flag inimiga
						int enemyteam = 1-i/TSIZE;
						if (fx.flag[enemyteam].carried)
						if (fx.flag[enemyteam].carrier == i) {

							// update flag position for draw
							fx.flag[enemyteam].pos.px = player[i].x;
							fx.flag[enemyteam].pos.py = player[i].y;
							fx.flag[enemyteam].pos.x = (int)fx.hero[i].x;
							fx.flag[enemyteam].pos.y = (int)fx.hero[i].y;

							// draw the miniflag here
							draw_mini_flag(drawbuf, enemyteam);
						}

						if (i != me) {
							putpixel(drawbuf, pix+0, piy+0, teamcol[i/TSIZE]);	//3 pixel teamcol
							putpixel(drawbuf, pix+1, piy+0, teamcol[i/TSIZE]);	//3 pixel teamcol
							putpixel(drawbuf, pix+0, piy+1, teamcol[i/TSIZE]);	//3 pixel teamcol
							putpixel(drawbuf, pix+1, piy+1, col[i%TSIZE]);		// 1 pixel personal-color
						}
						else {
							//myself: draw differently
							if ( (int(get_time() * 15)) % 3 > 0 ) {
								circlefill(drawbuf, pix, piy, 2, col[COLYELLOW]);
								circlefill(drawbuf, pix, piy, 1, teamlcol[i/TSIZE]);
							}
							else
								circlefill(drawbuf, pix, piy, 2, 0);
						}
					}

			// paint fog of war in all invisible rooms
			//
			for (int ry=0; ry<map.h; ry++)
				for (int rx=0; rx<map.w; rx++)
					if (!roomvis[ry*map.w+rx]) {
						drawing_mode(DRAW_MODE_TRANS, 0,0,0);
						set_trans_blender(0,0,0,0x38);
						int a,b,c,d;
						a = mmx+21 +  rx   *98/map.w  ;
						b = mmy+ 1 +  ry   *98/map.h  ;
						c = mmx+21 + (rx+1)*98/map.w-1;
						d = mmy+ 1 + (ry+1)*98/map.h-1;
						rectfill(drawbuf, a,b,c,d, col[COLFOGOFWAR]);
					}
			solid_mode();
		}//!hide_game

		//
		// the SCOREBOARD
		//
		#define NAMEYDELTA_MIN 8
		int NAMEYDELTA = 12;	//default value

		if (key[KEY_TAB]) {
			textprintf(drawbuf, font, sbx + 4, sby-4, teamlcol[0], "Red Team:   (PINGS)");
			textprintf(drawbuf, font, sbx + 4, sby-4 + 18*NAMEYDELTA_MIN, teamlcol[1], "Blue Team:  (PINGS)");
		}
		else {
			textprintf(drawbuf, font, sbx + 4, sby-4, teamlcol[0], "Red Team:    %2i capt", fx.flag[0].score);
			textprintf(drawbuf, font, sbx + 4, sby-4 + 18*NAMEYDELTA_MIN, teamlcol[1], "Blue Team:   %2i capt", fx.flag[1].score);
		}
		int i;
		int pix[2]; pix[0]=pix[1]=0;
		for (int fw=0;fw<2;fw++)		//first count, then draw
		{
			if (fw == 1) {
				//most players in a team
				int thepix;
				if (pix[0] > pix[1])
					thepix = pix[1];
				else
					thepix = pix[0];
				//calc the new NAMEYDELTA
				while ( (thepix * NAMEYDELTA) > 16 * 8 - 8 - 2) {
					if (NAMEYDELTA == NAMEYDELTA_MIN)
						break;	//the minimum is the minimum...
					NAMEYDELTA--;
				}
			}

			for (int dp=0;dp<maxplayers;dp++)
			{
				// i = player #   dp = draw slot (0-7 = red team's  8-15 = blue team's)
				i = scoreboard[dp];

				//i = dp;

				//draw it
				if (i != -1)
				if (player[i].used)
				{
					// print player name
					//textprintf(drawbuf, font, sbx + 4, sby + 2 +13 + dp * NAMEYDELTA + (dp/TSIZE)*(26-2), col[i%TSIZE], "%s", player[i].name);
					int what_y;
					if (i < TSIZE) {
						what_y = sby + 8 + dp * NAMEYDELTA;
					}
					else {
						what_y = sby + 19*NAMEYDELTA_MIN + (dp-TSIZE) * NAMEYDELTA;
					}

					//just count
					if (fw == 0) {
//						if (pix[i/TSIZE] < i%TSIZE)
//							pix[i/TSIZE] = i%TSIZE;
						if (pix[i/TSIZE] < dp%TSIZE)
							pix[i/TSIZE] = dp%TSIZE;
					}
					//draw
					else {

						// show name
						textprintf(drawbuf, font, sbx + 4, what_y, col[i%TSIZE], "%c%s", player[i].reg_status, player[i].name);

						// show ping or frags
						if (key[KEY_TAB]) {
							if (player[i].ping > 9999) player[i].ping = 9999;			//fix ping if too big
							textprintf(drawbuf, font, sbx + 4 + 16*8, what_y, teamlcol[i/TSIZE], "%4i", player[i].ping);
						}
						else
							textprintf(drawbuf, font, sbx + 4 + 16*8, what_y, teamlcol[i/TSIZE], "%4i", player[i].frags);
					}
				}
			}
		}

		// the STATUSBAR : traffic
		//
		//int bpsin = client->get_socket_stat(NL_AVE_BYTES_RECEIVED);
		//int bpsout = client->get_socket_stat(NL_AVE_BYTES_SENT);
		//int bpstraffic = bpsin + bpsout;
		//textprintf(drawbuf, font, 72*8-2, ply+plh+  5, col[COLINFO], "BPS:%4i", bpstraffic);
		//textprintf(drawbuf, font, 71*8-2, ply+plh+ 15, col[COLINFO], "%4i:%4i", bpsin, bpsout);

		//FPS
		textprintf(drawbuf, font, plx+10, ply+plh-14, 0, "FPS:%3.0f", FPS);

#ifdef CL_SHOW_TIME_LEFT
		// Time left if time limit is on.
		if (time_limit > 0)
			textprintf(drawbuf, font, plx+10, ply+6, 0, "TIME: %3d:%02d", seconds / 60, seconds % 60);
#endif

		// QUAD DAMAGE
		if (me >= 0) {

			double val;
			if (player[me].item_quad) {
				val = player[me].item_quad_time - get_time();
				if (val < 0) val = 0;
				textprintf(drawbuf, font, plx+244, ply+plh+5, col[COLCYAN], "POWER:  %2.0f", val);
			}
			if (player[me].item_speed) {
				val = player[me].item_speed_time - get_time();
				if (val < 0) val = 0;
				textprintf(drawbuf, font, plx+244, ply+plh+15, col[COLYELLOW], "TURBO:  %2.0f", val);
			}
			if (player[me].item_helm) {
				val = player[me].item_helm_time - get_time();
				if (val < 0) val = 0;
				textprintf(drawbuf, font, plx+244, ply+plh+25, col[COLMAG], "SHADOW: %2.0f", val);
			}

			//WEAPON LEVEL
			textprintf(drawbuf, font, plx+340, ply+plh+5, col[COLWHITE], "WEAPON: %i", player[me].weapon + 1);

			//BOT CFG
#ifndef	NO_BOTS

			if (botmode == 0)
				textprintf(drawbuf, font, plx+340, ply+plh+25, col[COLWHITE], "Bots: Default");
			else if (botmode == 1)
				textprintf(drawbuf, font, plx+340, ply+plh+25, col[COLWHITE], "Bots: None"); // NEW
			else if (botsize > 0)
				textprintf(drawbuf, font, plx+340, ply+plh+25, col[COLWHITE], "Bots: Fill+%i", botsize);
			else
				textprintf(drawbuf, font, plx+340, ply+plh+25, col[COLWHITE], "Bots: Fill"); // NEW

#endif  // NO_BOTS
		}

		//server hostname
		//textprintf(drawbuf, font, plx+6*8+334+(32-strlen_hostname)*8, ply+plh+25, col[COLINFO], "%s", hostname);

		//show "want change teams" flag if active
		if (want_change_teams)
		{
			int c = col[COLWHITE];
			if ( ( (int) (get_time() * 2.0) ) % 2 )	// blink!
				c = col[COLRED];
			textprintf(drawbuf, font, +0 + plx + plw - 6*8 - 10,     ply + plh - 18, c, "CHANGE");
			textprintf(drawbuf, font, +0 + plx + plw - 6*8 - 10 + 4, ply + plh -  9, c, "TEAMS");
		}
		if (want_map_exit) {
			int c = col[COLWHITE];
			if ( ( (int) (get_time() * 2.0) ) % 2 )	// blink!
				c = col[COLRED];
			textprintf(drawbuf, font, -40 + plx + plw - 6*8 - 10,     ply + plh - 18, c, "EXIT");
			textprintf(drawbuf, font, -40 + plx + plw - 6*8 - 10 + 4, ply + plh -  9, c, "MAP");
		}

		// the STATUSBAR : health energy, bars ....
		//
		if (me >= 0)
			textprintf(drawbuf, font, 10, ply+plh+ 5, col[COLWHITE],  "Health: %4i  Energy: %4i", player[me].health, player[me].energy);

		rectfill(drawbuf, 10, ply+plh+18, 10 + 100, ply+plh+18+10, col[COLNOLIFE]); //lifebar bg
		rectfill(drawbuf, 10+14*8, ply+plh+18, 10+14*8 + 100, ply+plh+18+10, col[COLNOLIFE]); // enerbar bg

		if (me >= 0) {
			if (player[me].health > 0) {

				//barra vermelha 0..100
				int redtarg = player[me].health;
				if (redtarg > 100) redtarg = 100;
				rectfill(drawbuf, 10, ply+plh+18, 10 + redtarg, ply+plh+18+10, col[COLRED]); //lifebar

				//barra magenta 100..200
				int magtarg = player[me].health - 100;
				if (magtarg > 100) magtarg = 100;
				if (magtarg > 0)
					rectfill(drawbuf, 10, ply+plh+18, 10 + magtarg, ply+plh+18+10, col[COLYELLOW]);

				//barra 3o nivel
				int targ3 = player[me].health - 200;
				if (targ3 > 100) targ3 = 100;
				if (targ3 > 0)
					rectfill(drawbuf, 10, ply+plh+18, 10 + targ3, ply+plh+18+10, col[COLMAG]);
			}
			if (player[me].energy > 0) {

				//barra azul 0..100
				int bluetarg = player[me].energy;
				if (bluetarg > 100) bluetarg = 100;
				rectfill(drawbuf, 10+14*8, ply+plh+18, 10+14*8 + bluetarg, ply+plh+18+10, col[COLBLUE]); // enerbar

				//barra verde 100..200
				int magtarg = player[me].energy - 100;
				if (magtarg > 100) magtarg = 100;
				if (magtarg > 0)
					rectfill(drawbuf, 10+14*8, ply+plh+18, 10+14*8 + magtarg, ply+plh+18+10, col[COLGREEN]); // enerbar

				//barra 3o nivel
				int targ3 = player[me].energy - 200;
				if (targ3 > 100) targ3 = 100;
				if (targ3 > 0)
					rectfill(drawbuf, 10+14*8, ply+plh+18, 10+14*8 + targ3, ply+plh+18+10, col[COLENER3]);
			}
		}

		// the HUD: message output
		//
		char lix[16];
		char *themsg;
		int top = 0;
		for (i=0;i<CHAT_SIZE;i++) {
			if (chatbuffer[i][0] != 0) {
				top = i+1;

				//default text color (normal chat)
				int tcol = col[COLORA];

				//change color if team chat
				strncpy(lix, chatbuffer[i], 2);	//2 primeiros chars da string
				lix[2]=0;
				themsg = &chatbuffer[i][2];
				if (!strcmp(lix, "@T"))		// T eam message
					tcol = col[COLYELLOW];
				else if (!strcmp(lix, "@I")) // I nformation
					tcol = col[COLGREEN];
				else if (!strcmp(lix, "@W")) // W warning
					tcol = col[COLLRED];
				else
					themsg = chatbuffer[i];	//don't discard 2 chars because there's no "@x" rpefix

				//colorful text
				textprintf(drawbuf, font, 3, 3+i*11, tcol, "%s", themsg);
			}
		}

		// the HUD: input text on "top" of message output
		//
		if (talkbuffer[0] != 0) {

			static char themsg[128];
			sprintf(themsg, "Say: %s_", talkbuffer);

			//nice border
			textprintf(drawbuf, font, +1+3, +0+3+top*11, 0, "%s", themsg);
			textprintf(drawbuf, font, +1+3, +1+3+top*11, 0, "%s", themsg);
			textprintf(drawbuf, font, +0+3, +1+3+top*11, 0, "%s", themsg);
			textprintf(drawbuf, font, -1+3, +1+3+top*11, 0, "%s", themsg);
			textprintf(drawbuf, font, -1+3, +0+3+top*11, 0, "%s", themsg);
			textprintf(drawbuf, font, -1+3, -1+3+top*11, 0, "%s", themsg);
			textprintf(drawbuf, font, +0+3, -1+3+top*11, 0, "%s", themsg);
			textprintf(drawbuf, font, +1+3, -1+3+top*11, 0, "%s", themsg);

			// the prompt text
			textprintf(drawbuf, font, 3, 3+top*11, col[COLWHITE], "%s", themsg);
		}

		//"server not responding... connection may have dropped" plaque
		if (get_time() > lastpackettime + 1.0) {
			rect(drawbuf,  194,  199, 444, 279, col[COLMENUWHITE]);
			rect(drawbuf, 196, 201, 446, 281, col[COLMENUBLACK]);
			rectfill(drawbuf, 195, 200, 445, 280, col[COLMENUGRAY]);
			textprintf(drawbuf, font, 220, 220, col[COLWHITE], "SERVER NOT RESPONDING...");
			textprintf(drawbuf, font, 220, 240, col[COLWHITE], "May be heavy packet loss,");
			textprintf(drawbuf, font, 220, 255, col[COLWHITE], "or the server disconnected");
		}

		// V0.4.4 : player scores overlay
		if (key[KEY_TAB]) {
			drawing_mode(DRAW_MODE_TRANS, 0,0,0);
			set_trans_blender(0,0,0,150);

			int w = 440;
			int h = 420;
			int mx = SCREEN_W / 2;
			int my = SCREEN_H / 2;
			int x1 = mx - w/2;
			int y1 = my - h/2;
			int x2 = mx + w/2;
			int y2 = my + h/2;
			int xc = (x1+x2)/2;

			rectfill(drawbuf, x1,y1,x2,y2, 0);

			solid_mode();

			int XLEFTPAD = x1+40;
			int YDEL;
			char sorry[256];
			int p, redt = 0, bluet = 0;
			double redpow = 0.0, bluepow = 0.0;

			// FIXME: "max world score"? "max world rating"?
			textprintf_centre(drawbuf, font, xc, y1+10, col[COLWHITE], "Ranking - %lu players", max_world_rank); //, max_world_score);

			textprintf(drawbuf, font, XLEFTPAD, y1+45, col[COLWHITE], "Rank Power Score Name            Frags Ping");

			YDEL = 60;

			for (p=0;p<TSIZE;p++)
			if (scoreboard[p] >= 0)
			{
				i = scoreboard[p];
				redt++;

				sorry[0]=0;
				if (player[i].isbot)
					strcpy(sorry, " ");
				else if (player[i].reg_status == ' ')
					strcpy(sorry, " ");
				else if (player[i].reg_status == '?')
					strcpy(sorry, " ");

				if (sorry[0]==0) {
					textprintf(drawbuf,font,XLEFTPAD,y1+YDEL, col[COLLRED], "%4i %5.2f %5i %-15s %5i %4i",
						player[i].rank,
						( ( ((double)player[i].score) + 1.0) / ( ((double)player[i].neg_score) + 1.0) ),
						player[i].score - player[i].neg_score,
						player[i].name,
						player[i].frags,
						player[i].ping
					);
					//V0.4.8
					redpow += ((double)(player[i].score+1)) / ((double)(player[i].neg_score+1));
				}
				else {
					textprintf(drawbuf,font,XLEFTPAD,y1+YDEL, col[COLLRED], "%16s %-15s %5i %4i",
						sorry,
						player[i].name,
						player[i].frags,
						player[i].ping
					);
					redpow += DEFAULT_PLAYER_RATE;//V0.4.8
				}

				//next
				YDEL += 9;
			}

			textprintf(drawbuf, font, XLEFTPAD, y1+240, col[COLWHITE], "Rank Power Score Name            Frags Ping");

			YDEL = 255;

			for (p=TSIZE;p<maxplayers;p++)
			if (scoreboard[p] >= 0)
			{
				bluet++;
				i = scoreboard[p];

				sorry[0]=0;
				if (player[i].isbot)
					strcpy(sorry, " ");
				else if (player[i].reg_status == ' ')
					strcpy(sorry, " ");
				else if (player[i].reg_status == '?')
					strcpy(sorry, " ");

				if (sorry[0]==0) {
					textprintf(drawbuf,font,XLEFTPAD,y1+YDEL, col[COLLBLUE], "%4i %5.2f %5i %-15s %5i %4i",
						player[i].rank,
						( ( ((double)player[i].score) + 1.0) / ( ((double)player[i].neg_score) + 1.0) ),
						player[i].score - player[i].neg_score,
						player[i].name,
						player[i].frags,
						player[i].ping
					);
					bluepow += ((double)(player[i].score+1)) / ((double)(player[i].neg_score+1)); //V0.4.8
				}
				else {
					textprintf(drawbuf,font,XLEFTPAD,y1+YDEL, col[COLLBLUE], "%16s %-15s %5i %4i",
						sorry,
						player[i].name,
						player[i].frags,
						player[i].ping
					);
					bluepow += DEFAULT_PLAYER_RATE;	//V0.4.8
				}

				//next
				YDEL += 9;
			}

			//calc powers
			/*
			double rtp, btp, rtpmean, btpmean;

			if ((redt == 0) || (redt == redu))
				rtp = 0;			//can't know...
			else {
				rtpmean = redpow / (redt - redu);
				rtp = (rtpmean * redt) / 3000.0;
			}

			if ((bluet == 0) || (bluet == blueu))
				btp = 0;			//can't know...
			else {
				btpmean = bluepow / (bluet - blueu);
				btp = (btpmean * bluet) / 3000.0;
			}
			*/

			//V0.4.8
			textprintf_centre(drawbuf, font, xc, y1+30, col[COLLRED], "Red Team - Power %.2f", redpow);
			textprintf_centre(drawbuf, font, xc, y1+225, col[COLLBLUE], "Blue Team - Power %.2f", bluepow);
		}

		// debug panel
		if (key[KEY_F9]) {
			clear_to_color(drawbuf, col[COLSHADOW]);

			textprintf(drawbuf,font,0,0,col[COLWHITE], "me=%i ", me);

			int p;
			for (p=0;p<maxplayers;p++) {
				textprintf(drawbuf,font,0,10+p*10,col[COLWHITE], "p.%i u=%i ons=%i evs=%lu sxy=%i,%i HR:p=%.1f,%.1f s=%.1f,%.1f o=%.1f,%.1f",
					p, player[p].used, player[p].onscreen, player[p].enemyvis, player[p].x, player[p].y,

					//					fx.hero[p].x, fx.hero[p].y, fx.hero[p].sx, fx.hero[p].sy,
					fd.hero[p].x, fd.hero[p].y, fd.hero[p].sx, fd.hero[p].sy, fd.hero[p].ox, fd.hero[p].oy
					);
			}

			int bpsin = client->get_socket_stat(NL_AVE_BYTES_RECEIVED);
			int bpsout = client->get_socket_stat(NL_AVE_BYTES_SENT);
			int bpstraffic = bpsin + bpsout;
			textprintf(drawbuf, font, 72*8-2, ply+plh+  5, col[COLINFO], "BPS:%4i", bpstraffic);
			textprintf(drawbuf, font, 71*8-2, ply+plh+ 15, col[COLINFO], "%4i:%4i", bpsin, bpsout);
		}


		//unlock frame mutex
		//LOG1("unlocking HOW=%i",HOWMANY);
		pthread_mutex_unlock( &frame_mutex );
		//LOG1("unlocked! HOW=%i",HOWMANY);

		// another frame, calc FPS...
		//
		totalframecount++;
		framecount++;
		double baixo = get_time() - starttime;
		if (baixo > 0) {
			if (baixo > 1.0) {
				FPS = ((double)framecount) / baixo;
				starttime = get_time();
				framecount = 0;
			}
		}
	}

	// draw help
	void draw_game_help() {
		clear_to_color(drawbuf, col[COLMENUGRAY]);

		int x = -40;
		int y = -70;

		textprintf(drawbuf, font, x+100, y+100, col[COLWHITE], "Outgun : HELP      --> Press ESC or F1 to go back. <--");

		textprintf(drawbuf, font, x+100, y+120, col[COLWHITE], "For more information access Outgun's website at:");
		textprintf(drawbuf, font, x+100, y+130, col[COLGREEN], "                http://www.amok.com.br/outgun/en/");

#ifndef NO_BOTS

		textprintf(drawbuf, font, x+100, y+160, col[COLWHITE], " MOVING     ARROW KEYS = MOVE       >>> BOT PREFERENCES: <<<");
		textprintf(drawbuf, font, x+100, y+170, col[COLWHITE], "  YOUR      CONTROL    = SHOOT!     F5,F6 = Less/More bots");
		textprintf(drawbuf, font, x+100, y+180, col[COLWHITE], "CHARACTER:  ALT        = STRAFE     F7    = Vote for NO BOTS");
		textprintf(drawbuf, font, x+100, y+190, col[COLWHITE], "  >>>>>     SHIFT      = RUN        F8    = Let others decide");

#else

		textprintf(drawbuf, font, x+100, y+160, col[COLWHITE], "           MOVING            ARROW KEYS = MOVE");
		textprintf(drawbuf, font, x+100, y+170, col[COLWHITE], "            YOUR             CONTROL    = SHOOT!");
		textprintf(drawbuf, font, x+100, y+180, col[COLWHITE], "          CHARACTER:         ALT        = STRAFE");
		textprintf(drawbuf, font, x+100, y+190, col[COLWHITE], "            >>>>>            SHIFT      = RUN");

#endif

		textprintf(drawbuf, font, x+100, y+210, col[COLWHITE], "TALKING TO ALL PLAYERS: Just type your message and hit ENTER");

		textprintf(drawbuf, font, x+100, y+230, col[COLWHITE], "TALKING JUST TO YOUR TEAM: Just place a dot ('.') at the very");
		textprintf(drawbuf, font, x+100, y+240, col[COLWHITE], " beginning of your message (first char)");

		textprintf(drawbuf, font, x+100, y+260, col[COLWHITE], "GAME CONCEPT: You are a member of a team, either RED or BLUE,");
		textprintf(drawbuf, font, x+100, y+270, col[COLWHITE], " assigned to you at random when you connect. Your goal is to");
		textprintf(drawbuf, font, x+100, y+280, col[COLWHITE], " help your team to win, by capturing 8 (default) times the ememy");
		textprintf(drawbuf, font, x+100, y+290, col[COLWHITE], " flag. To capture the flag, a member of your team must steal");
		textprintf(drawbuf, font, x+100, y+300, col[COLWHITE], " the enemy flag and bring it to your team's flag, provided your");
		textprintf(drawbuf, font, x+100, y+310, col[COLWHITE], " flag has not been stolen already! Capiche?");

		textprintf(drawbuf, font, x+100, y+330, col[COLWHITE], "HEALTH AND ENERGY: If your health reaches zero, you die. Energy");
		textprintf(drawbuf, font, x+100, y+340, col[COLWHITE], " is used for running, shooting and health protection when you");
		textprintf(drawbuf, font, x+100, y+350, col[COLWHITE], " have the SHIELD powerup (you'll know when you see it...).");
		textprintf(drawbuf, font, x+100, y+360, col[COLWHITE], " Health and energy regenerate with time.");

		textprintf(drawbuf, font, x+100, y+380, col[COLWHITE], "MINIMAP: On the upper-right corner of the screen is the minimap.");
		textprintf(drawbuf, font, x+100, y+390, col[COLWHITE], " It shows the contents of all rooms of the map that have at least");
		textprintf(drawbuf, font, x+100, y+400, col[COLWHITE], " one player of your team.");

		textprintf(drawbuf, font, x+100, y+420, col[COLWHITE], "CHANGING TEAMS: Hit the 'END' key to set wether you want to");
		textprintf(drawbuf, font, x+100, y+430, col[COLWHITE], " change teams or not. Your will change teams when appropriate.");

		textprintf(drawbuf, font, x+100, y+450, col[COLWHITE], "POWERUPS: If you see an animated item lying on the ground, grab");
		textprintf(drawbuf, font, x+100, y+460, col[COLWHITE], " it. It's a special power-up item.");

		textprintf(drawbuf, font, x+100, y+480, col[COLWHITE], "ETC.: Hit DEL to kill yourself. Hold TAB to see other player's");
		textprintf(drawbuf, font, x+100, y+490, col[COLWHITE], " ping times (in milisecons) on the scoreboard below the minimap.");
		textprintf(drawbuf, font, x+100, y+500, col[COLWHITE], " Hit HOME to change world colors and CTRL+HOME to restore them.");
		textprintf(drawbuf, font, x+100, y+510, col[COLWHITE], " Hit F10 to receive a random name. Hit F11 to take a screenshot.");
	}

	//draws the game menu
	void draw_game_menu() {

		//"3d" menu
		if (menu != 1) {
			rect(drawbuf,  99,  69, 539, 409, col[COLMENUWHITE]);
			rect(drawbuf, 101, 71, 541, 411, col[COLMENUBLACK]);
			rectfill(drawbuf, 100, 70, 540, 410, col[COLMENUGRAY]);
			textprintf(drawbuf, font, 150, 120, col[COLWHITE], "Outgun         version %s", GAME_VERSION);
			textprintf(drawbuf, font, 150, 135, col[COLGREEN], "http://www.amok.com.br/outgun/en/");
		}

		if (menu == 0) {
			static int DELY = 10;

			textprintf(drawbuf, font, 150, 185-DELY, col[COLWHITE], "  [ 1 ]   Connect");
			textprintf(drawbuf, font, 150, 200-DELY, col[COLWHITE], "  [ 2 ]   Disconnect");
			if (connected)
				textprintf(drawbuf, font, 150+22*8, 200-DELY, col[COLGREEN], "(%s)", address);
			textprintf(drawbuf, font, 150, 215-DELY, col[COLWHITE], "  [ 3 ]   Change Player Name & Password");
			textprintf(drawbuf, font, 150, 227-DELY, col[COLGREEN], "          '%s' (%s)", playername, namestatus);
			textprintf(drawbuf, font, 150, 243-DELY, col[COLWHITE], "  [ 4 ]   Start/stop local server");
			if (listen_server_running)
				textprintf(drawbuf, font, 150, 255-DELY, col[COLGREEN], "          SERVER RUNNING ON PORT %i", listen_port_running);
			textprintf(drawbuf, font, 150, 271-DELY, col[COLWHITE], "  [ 5 ]   Toggle fullscreen/windowed mode");

			if (validtheme) {
				textprintf(drawbuf, font, 150, 286-DELY, col[COLWHITE], "  [ 6 ]   Change sound theme: (%s)", sfxthemedir);
				textprintf_centre(drawbuf, font, 150+180, 300-DELY, col[COLGREEN], "'%s'", sfxthemename);
			}
			else {
				textprintf(drawbuf, font, 150, 286-DELY, col[COLWHITE], "  [ 6 ]   Change sound theme:");
				textprintf(drawbuf, font, 150, 300-DELY, col[COLGREEN], "          no sfx themes found.");
			}
			textprintf(drawbuf, font, 150, 340-DELY, col[COLWHITE], "Hit CTRL+F12 to EXIT THE GAME");
			textprintf(drawbuf, font, 150, 355-DELY, col[COLWHITE], "Hit ESC to HIDE OR SHOW THIS MENU");
			textprintf(drawbuf, font, 150, 370-DELY, col[COLORA], "Hit F1 to SHOW THE HELP SCREEN");
		}
		else if (menu == 1) {

			//Big F Menu
			rect(drawbuf,  19,  19, 620, 460, col[COLMENUWHITE]);
			rect(drawbuf,  21,  21, 621, 461, col[COLMENUBLACK]);

			int lotext = makecol(0x99, 0x99, 0x99);


			if (showmaster) {

				int hi = makecol(0x68, 0x68, 0x88); //col[COLMENUGRAY]; //makecol(0x99,0x99,0x99);
				int lo = makecol(0x68,0x48,0x48);
				//hilight all
				rectfill(drawbuf, 20, 20, 620, 460, hi);
				//first bar hi vs lo
				rectfill(drawbuf, 20, 20, 320, 50, hi);
				rectfill(drawbuf, 320, 19, 621, 50, 0);
				rectfill(drawbuf, 320, 24, 616, 50, lo);
				vline(drawbuf, 320, 20, 50, col[COLMENUBLACK]);
				hline(drawbuf, 320, 50, 621, col[COLMENUWHITE]);
				hline(drawbuf, 320, 24, 616, lotext);
				vline(drawbuf, 616, 24, 49, col[COLMENUBLACK]);
				textprintf_centre(drawbuf, font, 170, 35, col[COLWHITE], "INTERNET SEARCH");
				textprintf_centre(drawbuf, font, 470, 35, lotext, "FAVORITES");
				//textprintf_centre(drawbuf, font, 320, 40, col[COLWHITE], "Showing INTERNET LISTING page (TAB = FAVORITES)");

				if (((int)(get_time() * 1)) % 2)
					textprintf_centre(drawbuf, font, 320, 65, col[COLGREEN], "F2 = UPDATE LIST OF SERVERS");
				else
					textprintf_centre(drawbuf, font, 320, 65, col[COLYELLOW], "F2 = UPDATE LIST OF SERVERS");

				textprintf_centre(drawbuf, font, 320, 80, col[COLWHITE], "Press SPACE to refresh the servers");
				//textprintf_centre_x(drawbuf, font, 320, 75, col[COLGREEN], 0, "TAB = Change to FAVORITES page");

				//textprintf_centre(drawbuf, font, 320, 115, col[COLWHITE], "ARROWS:Select - ENTER:Connect - ESC:Cancel - SPACE:Refresh");

				textprintf_centre(drawbuf, font, 320, 440, col[COLWHITE], "TAB:Favorites  ARROWS:Select  ENTER:Connect  ESC:Cancel  SPACE:Refresh");
			}
			else {

				int hi = makecol(0x88, 0x68, 0x68); //col[COLMENUGRAY]; //makecol(0x99,0x99,0x99);
				int lo = makecol(0x48,0x48,0x68);
				//hilight all
				rectfill(drawbuf, 20, 20, 620, 460, hi);
				//first bar lo vs hi
				rectfill(drawbuf, 320, 20, 620, 50, hi);
				rectfill(drawbuf, 19, 19, 320, 50, 0);
				rectfill(drawbuf, 24, 24, 320, 50, lo);
				vline(drawbuf, 320, 19, 50, col[COLMENUWHITE]);//?
				hline(drawbuf, 19, 50, 320, col[COLMENUWHITE]);
				hline(drawbuf, 24, 24, 320, lotext);
				vline(drawbuf, 24, 24, 49, col[COLMENUWHITE]);
				textprintf_centre(drawbuf, font, 170, 35, lotext, "INTERNET SEARCH");
				textprintf_centre(drawbuf, font, 470, 35, col[COLWHITE], "FAVORITES");

				//textprintf_centre(drawbuf, font, 320, 40, col[COLWHITE], "Showing FAVORITES page (TAB = INTERNET LISTING)");
				textprintf_centre(drawbuf, font, 320, 65, col[COLWHITE], "Type the IP address of the server and hit ENTER");
				textprintf_centre(drawbuf, font, 320, 80, col[COLWHITE], "Press SPACE to refresh the servers");
				//textprintf_centre_x(drawbuf, font, 320, 75, col[COLYELLOW], 0, "TAB = Change to INTERNET LISTING page");

				textprintf_centre(drawbuf, font, 320, 440, col[COLWHITE], "TAB:Internet  ARROWS:Select  ENTER:Connect  ESC:Cancel  SPACE:Refresh");
			}

			int xi = 50 - 8*2;

			textprintf(drawbuf, font, xi, 105, col[COLWHITE], "IP Address             Ping #P Version/Hostname");

			char blinkchar[2];

			int yi;

			for (int i=0;i<MAX_GAMESPY;i++) {

				yi = 120 + i*13;

				//selectr
				if (gi == i) {
					rectfill(drawbuf, xi-3,yi-3,xi+550+8*3,yi+12,col[COLSHADOW]);

					//blink cursor
					if (((int)(get_time() * 4)) % 2)
						blinkchar[0]=' ';
					else
						blinkchar[0]='<';
					blinkchar[1]=0;
				}
				else
					blinkchar[0]=0;

				//server edit prompt
				if (showmaster) {
					textprintf(drawbuf, font, xi, yi, col[COLGREEN], ":%s%s",mgamespy[i].address, blinkchar);

					//favs watermarks
					if (mgamespy[i].favs)
						textprintf(drawbuf, font, xi - 12, yi, makecol(0x99,0x78,0x78), "*");
				}
				else
					textprintf(drawbuf, font, xi, yi, col[COLGREEN], ":%s%s",gamespy[i].address, blinkchar);

				//draw gamespy entry
				bool refreshed, invalid, noresponse;
				if (showmaster) {
					refreshed  = mgamespy[i].refreshed;
					invalid    = mgamespy[i].invalid;
					noresponse = mgamespy[i].noresponse;
				}
				else {
					refreshed  = gamespy[i].refreshed;
					invalid    = gamespy[i].invalid;
					noresponse = gamespy[i].noresponse;
				}

				if (!refreshed) { // not refreshed
					//server info
					textprintf(drawbuf, font, xi + (18+5)*8, yi, col[COLWHITE], "press SPACEBAR to refresh...");
				}
				else if (invalid) {	//refreshed, invalid
					//server info
					textprintf(drawbuf, font, xi + (18+5)*8, yi, col[COLWHITE], "---");
				}
				else if (noresponse) {	//refreshed, no response
					//server info
					textprintf(drawbuf, font, xi + (18+5)*8, yi, col[COLWHITE], "no response.");
				}
				else {  //refreshed, valid
					//server info
					if (showmaster)
						textprintf(drawbuf, font, xi + (18+5)*8, yi, col[COLGREEN], "%s", mgamespy[i].info);
					else
						textprintf(drawbuf, font, xi + (18+5)*8, yi, col[COLGREEN], "%s", gamespy[i].info);
				}
			}
		}
		else if (menu == 2) {
			textprintf(drawbuf, font, 150, 230, col[COLWHITE], dialogmessage);
			textprintf(drawbuf, font, 150, 250, col[COLWHITE], dialogmessage2);
		}
		else if (menu == 3) {
			textprintf(drawbuf, font, 150, 170, col[COLWHITE], "Type in your player name. If you have");
			textprintf(drawbuf, font, 150, 185, col[COLWHITE], "registered your name on the Outgun");
			textprintf(drawbuf, font, 150, 200, col[COLWHITE], "website, then type in your password!");

			textprintf(drawbuf, font, 150, 220, col[COLWHITE], "ENTER = OK   ESC = CANCEL  TAB = NEXT FIELD");
			textprintf(drawbuf, font, 150, 260, col[COLGREEN], "NAME     :%s%s", editplayername, namecursor);

			//password field: '********'
			char starpass[32]; int c=0;
			for (; editplayerpass[c]; c++) starpass[c] = '*';
			starpass[c] = 0;

			textprintf(drawbuf, font, 150, 285, col[COLGREEN], "PASSWORD :%s%s", starpass, passcursor);

			textprintf(drawbuf, font, 150, 350, col[COLWHITE], "Registration status: %s", namestatus);
		}
		else {
			textprintf(drawbuf, font, 150, 150, col[COLRED], "unknown menu %i", menu);
		}
	}

	//show a specific menu screen
	void set_menu(int menumber) {
		menu = menumber;
		clear_keybuf(); //clear keystrokes buffer
	}

	//disconnect command
	void disconnect_command() {

		//disconnect the client here if was connected, else does nothing
		client->connect(false);

		//dialogz
		strcpy(dialogmessage, "You are disconnected. Press ESC");
		strcpy(dialogmessage2, "");
		menushow = true;
		set_menu(2);	//dialog menu
	}

	void client_connected(char *data, int length) {

		//not trying anymore
		trying_connection = false;

		//no host ad yet
		hostad = 0;
		hostadname[0]=0;

		//"data" from connection accepted:
		//  BYTE		maxplayers
		//  STRING	hostname
		int count = 0;
		NLubyte maxpl;
		readByte(data, count, maxpl);
		maxplayers = maxpl;
		//strcpy(hostname, data);
		readString(data, count, hostname);
		hostname[32]=0;		//truncate at 32 chars
		strlen_hostname = strlen(hostname);	//for drawing

		//V0.4.4: read server's NOTCP flag value
		NLubyte noflag;
		readByte(data, count, noflag);
		server_no_tcp = (noflag > 0);

		//set window title: the hostname
		char lecaption[256];
		sprintf(lecaption, "Connected to: %s (%s)", hostname, address);
		server_status_string(lecaption);

		int i;
		//default scoreboard state
		for (i=0;i<MAX_PLAYERS;i++)	scoreboard[i]=i;

		//don't want to change teams by default
		want_change_teams = false;

		//don't want to exit map by default
		want_map_exit = false;

		//default bot preferences
		botmode = 0;
		botsize = 0;
		bot_pref_change = false;

		//avoid "dropped" plaque
		lastpackettime = get_time() + 1.0;

		// reset gamestate?
		connected = true;
		gameshow = true;
		fx.frame = -1.0;		// no data
		fd.frame = -1.0;		// no data
		me = -1;	//don't know who am I

		//hide menu : must be AFTER gameshow = true
		menushow = false;  // hide menu

		//reset chat buffer
		talkbuffer[0]=0;
		for (i=0;i<CHAT_SIZE;i++)
			chatbuffer[i][0]=0;
		chaterasetime = get_time() + 10.0;

		//reset world data
		// players

		for (i=0;i<MAX_PLAYERS;i++)
			player[i].clear(false, false, i, 0, "(name unknown)"); //#NR: replaced manual initialization including memset(0)

		//reset FPS count vars
		framecount = 0;
		starttime = get_time();
		FPS = 666.0;

		//send name update request
		issue_change_name_command();

		//init game frame state
		//fx.frame = 0;				//hmm
		//fx.skipped = true;	//hmm
		//fd.frame = 0;				//hmm
		//fd.skipped = true;	//hmm

		// MOVED FROM GAMECLIENT_C::START():

		// default map
		//load_default_map(&map);
		map_ready = false;		// NO map change commands from server yet
		servermap[0]=0;

		//not showing gameover plaque
		gameover_plaque = NEXTMAP_NONE;

		//clear fx
		clear_fx();
	}

	void client_disconnected() {

		//restore window title
		server_status_string("Outgun client - CTRL+F12 to quit");

		// the gamestate?
		connected = false;
		gameshow = false;

		// show a message
		strcpy(dialogmessage, "You have been disconnected. Press ESC");
		strcpy(dialogmessage2, "");
		menushow = true;
		set_menu(2);	//dialog menu

		//namestatus
		if (namestatus_code == 0)
			strcpy(namestatus, "NO PASSWORD SET?");
		else if ((namestatus_code == 1) || (namestatus_code == 3)) {
			strcpy(namestatus, "LOGGED (");
			strcat(namestatus, player_token);
			strcat(namestatus, ")");
		}
		else if (namestatus_code == 2) {
			strcpy(namestatus, "ERROR: WRONG ID?");
		}

	}

	void connect_failed_denied(char *data, int length) {

		//not trying anymore
		trying_connection = false;

		//extract message
		char message[256];
		if (length > 0) {
			int count = 0;
			readString(data, count, message);
		}
		else
			strcpy(message, "no reason given.");

		// show a message
		strcpy(dialogmessage, "Connection refused     (press ESC)");
		strcpy(dialogmessage2, message);
		menushow = true;
		set_menu(2);	//dialog menu
	}

	void connect_failed_unreachable() {

		//not trying anymore
		trying_connection = false;

		// show a message
		strcpy(dialogmessage, "No response from server. Press ESC");
		strcpy(dialogmessage2, "");
		menushow = true;
		set_menu(2);	//dialog menu
	}

	//refresh servers command
	void refresh_command() {

		if (showmaster)
			refresh_command_2(mgamespy);
		else
			refresh_command_2(gamespy);
	}

	//refresh servers command
	void refresh_command_2(gamespy_t *gamespy) {

		show_progress("", "Refreshing servers...", "");

		NLsocket sock = nlOpen(0, NL_UNRELIABLE);

		if (sock == NL_INVALID) {
			LOG2("LIXAO!!!!!! %s %s\n", nlGetErrorStr(nlGetError()), nlGetSystemErrorStr(nlGetSystemError()) );
			return;
		}

		char   lebuf[512];

		//debug
		char dinfo[2000];
		char lix[256];
		strcpy(dinfo, "D=");

		int i;

		// no response from all calc addresses num_valid
		//
		double st[MAX_GAMESPY][4];	//send time
		int	rc[MAX_GAMESPY];		//resposta count
		double rt[MAX_GAMESPY];		//resposta time
		int num_valid = 0;
		for (i=0;i<MAX_GAMESPY;i++) {

			rc[i]=0;	//no responses
			rt[i]=0;	//no time

			gamespy[i].noresponse = true;
			gamespy[i].invalid = true; // invalid entry for default
			gamespy[i].refreshed = true;	//refreshed now

			nlOpen(0,0);//force invalid enum error

			nlStringToAddr(gamespy[i].address, &gamespy[i].addr);

			//test if the address is invalid (important)
			if (nlGetError() == NL_INVALID_ENUM) {

				nlOpen(0,0);//force invalid enum error

				//v0.4.2: if address has no port or has invalid port, set it to the default value (25000)
				int door = nlGetPortFromAddr(&gamespy[i].addr);
				if (door == 0)
					nlSetAddrPort(&gamespy[i].addr, DEFAULT_UDP_PORT); //port);//server PORT!!!!!!

				//test if set was ok
				if (nlGetError() == NL_INVALID_ENUM) {

					gamespy[i].invalid = false; // non-invalid entry
					num_valid ++;
				}
			}
		}

		//send
		//
		for (int t=0;t<4;t++) { //send four times

			// (1) SEND
			//
			for (int i=0;i<MAX_GAMESPY;i++)
			if (!gamespy[i].invalid)
			{
				int count =0;
				writeLong(lebuf, count, 0);			//special packet
				writeLong(lebuf, count, 200);		//serverinfo request
				writeByte(lebuf, count, (NLubyte)i);		//connect entry (am I lazy or what)
				writeByte(lebuf, count, (NLubyte)t);		//packet number

				nlSetRemoteAddr(sock, &gamespy[i].addr);
				int res = nlWrite(sock, lebuf, count);	//send
				st[i][t] = get_time();	//for ping measure

				sprintf(lix, "%i,", res);
				strcat(dinfo, lix);

			}//send loop


			//(2) pause before each send
			//
		for (int bla=0;bla<20;bla++)
		{
			MS_SLEEP(5);			//*** NO CPU PROBLEM HERE ***

			//(3) collect any responses so far
			//
			// [h0ly] 'i' will be setted by the readByte later on
			int i;
			int am = 0;
			do {
				am = nlRead(sock, lebuf, 512);
				if (am > 0) {

					strcat(dinfo,"R,");

					int count = 0;
					NLulong along;
					NLubyte pack;
					readLong(lebuf, count, along); // should be 0..
					if (along == 0) {
						readLong(lebuf, count, along); // should be 200...
						if (along == 200) {
							readByte(lebuf, count, i); // client's gamespy entry
							readByte(lebuf, count, pack); // packet #
							readString(lebuf, count, gamespy[i].info);

							//add to ping statistics
							rc[i]++;
							rt[i] += ( get_time() - st[i][pack] );

							if (gamespy[i].noresponse)	//dec replies expected count
								num_valid--;
							gamespy[i].noresponse = false;	//response obtained
						}
					}
				}
			} while (am > 0);
		}

		}

		//(4) wait for mising responses for a timeout period  (1.5 seconds)
		//
		for (int wa=0;wa<300;wa++) {

			//quit when done
			if (num_valid <= 0)
				break;

			//sleep a bit
			MS_SLEEP(5);	//*** NO CPU PROBLEM HERE ***

			//collect responses

			int i;
			int am = 0;
			do {
				am = nlRead(sock, lebuf, 512);
				if (am > 0) {

					strcat(dinfo,"R,");

					int count = 0;
					NLulong along;
					NLubyte pack;
					readLong(lebuf, count, along); // should be 0..
					if (along == 0) {
						readLong(lebuf, count, along); // should be 200...
						if (along == 200) {
							readByte(lebuf, count, i); // client's gamespy entry
							readByte(lebuf, count, pack); // packet #
							readString(lebuf, count, gamespy[i].info);

							//add to ping statistics
							rc[i]++;
							rt[i] += ( get_time() - st[i][pack] );

							if (gamespy[i].noresponse)	//dec replies expected count
								num_valid--;
							gamespy[i].noresponse = false;	//response obtained
						}
					}
				}
			} while (am > 0);

		}

		// add ping to statistics
		//
		for (i=0;i<MAX_GAMESPY;i++)
		if (!gamespy[i].noresponse)	//got at least 1 response?
		{
			int daping;
			if (rc[i] > 0)
				daping = (int)(1000.0 * rt[i] / rc[i]);
			else
				daping = -666;

			char thelix[2000];
			sprintf(thelix, "%4i %s", daping, gamespy[i].info);
			strcpy(gamespy[i].info, thelix);
		}

		nlClose(sock);
	}

	//connect command
	void connect_command() {

		// disconnect
		//
		client->connect(false);

		// copy gamespy address
		//
		if (showmaster)
			strcpy(address, mgamespy[gi].address);
		else
			strcpy(address, gamespy[gi].address);


		// start connecting to specified IP/port
		// connection results will come through the CFUNC_CONNECTION_UPDATE callback
		char addr[256];
		if (address[0] == '\0')	{ //empty address == my own ip
			NLaddress myadr;
			get_self_IP(&myadr);
			nlAddrToString(&myadr, address);
			if (showmaster)
				strcpy(mgamespy[gi].address, address);	//copy to gamespy
			else
				strcpy(gamespy[gi].address, address);	//copy to gamespy
		}

		sprintf(addr, "%s:%i", address, port);
		client->set_server_address(addr);

		//set connect-data (goes in every connect packet): outgun game name and version strings
		char lebuf[256]; int count = 0;
		writeString(lebuf, count, GAME_STRING);
		writeString(lebuf, count, GAME_PROTOCOL);
		client->set_connect_data(lebuf, count);

		client->connect(true);

		// set flags, show dialog...
		trying_connection = true;
		sprintf(dialogmessage, "trying to connect... ESC=CANCEL");
		dialogmessage2[0]='\0';
		set_menu(2);	// dialog
	}

	//send player token message
	void send_player_token() {
		if (player_token_set) {
			char lebuf[256]; int count = 0;
			writeByte(lebuf, count, 30);	// 30 = pass registration token to server
			writeString(lebuf, count, player_token);
			client->send_message(lebuf, count);
		}
	}

	//issue change name command
	void issue_change_name_command() {

		//regular change name
		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 1);		// "1" = client request name change
		playername[15] = 0;	//truncate player name, max 16 chars
		writeString(lebuf, count, playername);	// the name
		client->send_message(lebuf, count);

		//name changed:
		player_token_new = true;	//getting a NEW token, not refreshing the token

		//get it (V0.4.9) -- força um "ENTER" logo apos o cara conectar ou trocar de nome ao estar
		//conectado...
		check_change_pass_command();

		//FIXME: code == 2 (?)
		if ((namestatus_code == 1) || (namestatus_code == 3))
			strcpy(namestatus, "NAME CHANGED...");
		else
			namestatus_code = 0;						//take the "*" out of the name

		//v0.4.4 follow-up-message: since changing name DISABLES the registration status
		//  of the player, we need to send a new message requesting registration, if the
		//  player has a token
		//v0.4.9 : substituido pelo "enter" acima do check_...()
		//send_player_token();
	}

	//change name command
	void change_name_command() {
		//set new name, close menu
		strcpy(playername, editplayername);
		if (menushow)
			set_menu(0);

		//send reliable net message with the name
		if (connected) {
			issue_change_name_command();
		}
	}

	//send the client's frame to server (keypresses)
	void send_frame() {

		char lebuf[256]; int count = 0;

		//client send: l,r,u,d keys and strafe
		NLubyte  keys = 0;
		if (key[KEY_LEFT])  keys += 1;
		if (key[KEY_RIGHT]) keys += 2;
		if (key[KEY_UP])    keys += 4;
		if (key[KEY_DOWN])  keys += 8;
		if ((key[KEY_ALT]) || (key[KEY_ALTGR]))   //strafe
			keys += 16;
		if ((key[KEY_LSHIFT] || (key[KEY_RSHIFT])))	//run
			keys += 32;
		writeByte(lebuf, count, keys);

		//send the client input
		client->send_frame(lebuf, count);
	}

	//helper do helper: reconstitui 1 rocket
	void client_set_rocket(int id, int dir, NLulong frameno, int owner, int px, int py, int x, int y, int xdelta) {

		rocket_c  *rock = &fx.rock[id];

		rock->hit_time = 0;
		rock->deg = dir * PIOIT;

		//REMENDO: avanca 0,5 frame
		//rock->time = frameno;
		rock->cl_time = (double)frameno - 0.5;	// "meio frame" atras, isto, e o tiro adianta
																						//porque foi atirado mais antes

		rock->owner = owner;
		rock->dontdraw = false;		//DO draw...
		rock->team = owner/TSIZE;
		rock->x = x;
		rock->y = y;
		rock->px = px;
		rock->py = py;
		rock->x += cos(rock->deg + PI/2) * xdelta;
		rock->y += sin(rock->deg + PI/2) * xdelta;
	}

	//helper: reconstitui varios rockets de uma mensagem "7" tipo rocket fire.
	void client_rebuild_shot(int pow, int dir, int *rids, NLulong frameno, int owner, int px, int py, int x, int y) {

		//char lix[2000];
		//sprintf(lix, "rids = %i %i %i", rids[0], rids[1], rids[2]);
		//print_message(lix);

		switch (pow) {
		case 1:
			client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, 0);
			break;
		case 2:
			client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, - SHOT_DELTAX);
			client_set_rocket(rids[1], dir,frameno,owner,px,py,x,y, + SHOT_DELTAX);
			break;
		case 3:
			client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[1], dir,frameno,owner,px,py,x,y, - SHOT_DELTAX * 2);
			client_set_rocket(rids[2], dir,frameno,owner,px,py,x,y, + SHOT_DELTAX * 2);
			// V0.4.8 : NEW TRIPEL SHOT
			//client_set_rocket(rids[1], dir+1,frameno,owner,px,py,x,y, 0);
			//client_set_rocket(rids[2], dir-1,frameno,owner,px,py,x,y, 0);
			break;
		case 4:
			client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, - SHOT_DELTAX);
			client_set_rocket(rids[1], dir,frameno,owner,px,py,x,y, + SHOT_DELTAX);
			client_set_rocket(rids[2], dir+1,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[3], dir-1,frameno,owner,px,py,x,y, 0);
			break;
		case 5:
			client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[1], dir,frameno,owner,px,py,x,y, - SHOT_DELTAX * 2);
			client_set_rocket(rids[2], dir,frameno,owner,px,py,x,y, + SHOT_DELTAX * 2);
			client_set_rocket(rids[3], dir+2,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[4], dir-2,frameno,owner,px,py,x,y, 0);
			break;
		case 6:
			client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, - SHOT_DELTAX);
			client_set_rocket(rids[1], dir,frameno,owner,px,py,x,y, + SHOT_DELTAX);
			client_set_rocket(rids[2], dir+1,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[3], dir-1,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[4], dir+2,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[5], dir-2,frameno,owner,px,py,x,y, 0);
			break;
		case 7:
			client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[1], dir,frameno,owner,px,py,x,y, - SHOT_DELTAX * 2);
			client_set_rocket(rids[2], dir,frameno,owner,px,py,x,y, + SHOT_DELTAX * 2);
			client_set_rocket(rids[3], dir+2,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[4], dir-2,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[5], dir+3,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[6], dir-3,frameno,owner,px,py,x,y, 0);
			break;
		case 8:
			client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, - SHOT_DELTAX);
			client_set_rocket(rids[1], dir,frameno,owner,px,py,x,y, + SHOT_DELTAX);
			client_set_rocket(rids[2], dir+1,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[3], dir-1,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[4], dir+2,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[5], dir-2,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[6], dir+3,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[7], dir-3,frameno,owner,px,py,x,y, 0);
			break;
		case 9:
			client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[1], dir,frameno,owner,px,py,x,y, - SHOT_DELTAX * 2);
			client_set_rocket(rids[2], dir,frameno,owner,px,py,x,y, + SHOT_DELTAX * 2);
			client_set_rocket(rids[3], dir+1,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[4], dir-1,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[5], dir+2,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[6], dir-2,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[7], dir+3,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[8], dir-3,frameno,owner,px,py,x,y, 0);
			break;
		}
	}

	//process incoming data
	void process_incoming_data(char *data, int length) {

		//this is a HACK:
		int whatme = 0;

		//lock frame mutex
		//LOG("locking INCOMING\n");
		pthread_mutex_lock( &frame_mutex );
		//LOG("locked! INCOMING\n");

		// (0) update lastpackettime
		lastpackettime = get_time();

		//(1) process server frame data
		//
		int count = 0;
		NLulong svframe;	//server's frame
		readLong(data, count, svframe);

		//discard older frames
		//overwrite always the newer frames
		// TARGET FRAME: just one
		if (svframe > fx.frame)
		{
			fx.frame = svframe;
			fx.time  = get_time();		//hope it's good enough... needed 10ms clock (1/100 sec) at least.

			NLulong	players_present;		//LONG players present (32 players max)
			readLong(data, count, players_present);
			int i;
			for (i=0;i<maxplayers;i++) {
				//decode players_present: sets if "player" record is used or not, in clientside
				if (players_present & (1 << i))
					player[i].used = true;
				else
					player[i].used = false;
			}

			//UGLY HACK
			static NLulong old_players_present;
			if (old_players_present != players_present) {
				old_players_present = players_present;
				update_scoreboard();
			}

			//----- PLAYER SPECIFIC DATA -----

			//extra byte of information
			// BIT 0: extra health
			// BIT 1: extra energy
			// BIT 2 (****VERY IMPORTANT****): NO MORE DATA ON PACKET BECAUSE PLAYER IS NOT READY
			NLubyte xtra = 0;
			readByte(data, count, xtra);

			//moved below: after health assignment
			//if (xtra & 1) player[me].health += 256;
			//if (xtra & 2) player[me].energy += 256;

			bool empty_frame_cause_not_ready_yet;
			if (xtra & 4)
				empty_frame_cause_not_ready_yet = true;
			else
				empty_frame_cause_not_ready_yet = false;

			//read "me" (v0.3.9 tentando espantar bug com tiro de canhao!)
			// BITS 3..8 == what player id
			whatme = 0;
			if (xtra & 8)   whatme += 1;
			if (xtra & 16)  whatme += 2;
			if (xtra & 32)  whatme += 4;
			if (xtra & 64)  whatme += 8;
			if (xtra & 128) whatme += 16;

			// v0.4.1: ISSO AQUI TAVA FALTANDO. como que tu vai ler um monte de coisa
			//    dependente do "me" sem ter ele definido??????
			//
			me = whatme;

			//EMPTY FRAME? if yes, do something about it, if not, parse it
			if (empty_frame_cause_not_ready_yet) {

				//an empty frame

				// mark somewhere that the frame (fx) should not be read/simulated?
				//					hmm...
				fx.skipped = true;

			}
			else {

				//a regular frame
				fx.skipped = false;

				//V 0.3.9 NEW : read screen of "me" player
				NLubyte  scr;
				readByte(data, count, scr);		//player.x
				player[me].x = scr;
				readByte(data, count, scr);		//player.y
				player[me].y = scr;

				//read "players onscreen" vector
				NLulong	players_onscreen;
				readLong(data, count, players_onscreen);

				//decode players_onscreen and update player data
				for (i=0;i<maxplayers;i++) {

					//decode players_onscreen: sets if "player" record is there to be read
					if (players_onscreen & (1 << i))
						player[i].onscreen = true;
					else
						player[i].onscreen = false;

					//if player on screen, parse the data
					hero_t	*h;
					if (player[i].onscreen) {

						h = &(fx.hero[i]);

						//V0.3.9: took out screen reading, replacing for the same screen of "me"
						// that is set above
						player[i].x = player[me].x;	//same screen since it's on the "players on same screen" vector
						player[i].y = player[me].y;

						//coords & speeds
//						NLshort sho;
						NLubyte xy;
						//V0.3.9 : transmissao x,y de 4 bytes para 3
						//256+512+1024+2048 = 3840    last 4 bits mask
						//xy = ((hx & 3840) >> 8) + ((hy & 3840) >> 4); //x: bit 8-11 to 0-3  y: bit 8-11 to 4-7
						//writeByte(lebuf, lecount, xy);   //last 4 bits x + last 4 bits y

						readByte(data, count, xy);		//first 8 bits x
						h->x = ((double)xy);
						readByte(data, count, xy);		//first 8 bits y
						h->y = ((double)xy);
						readByte(data, count, xy);		//first 4 bits x + first 4 bits y
						h->x += ((double)  ( (xy &  15) << 8 )   );	//bits 0-3 to 8-11
						h->y += ((double)  ( (xy & 240) << 4 )   ); //bits 4-7 to 8-11

						//readShort(data, count, sho);		//x
						//h->x = ((double)sho);
						//readShort(data, count, sho);		//y
						//h->y = ((double)sho);

						//V0.3.9 speed em bytes, xinelao mesmo
						NLbyte sxy;
						readByte(data, count, sxy);	//sx
						h->sx = ((double)sxy) / 2.0;
						readByte(data, count, sxy);	//sy
						h->sy = ((double)sxy) / 2.0;

						//readShort(data, count, sho);		//sx
						//h->sx = ((double)sho) / 100.0;
						//readShort(data, count, sho);		//sy
						//h->sy = ((double)sho) / 100.0;

						//EXTRA BYTE
						NLubyte byt, extra;
						readByte(data, count, extra);			//extra byte

						//FLAGS BYTE
						//
						player[i].dead = (extra & 1) != 0;  //DEAD PLAYER = extra bit 0
						player[i].item_deathbringer = (extra & 2) != 0;		//deathbringer: extra bit 1
						player[i].deathbringer_affected = (extra & 4) != 0; //deathbringer-affected: extra bit 2
						// ITEMS: movido para este byte
						player[i].item_shield = (extra & 8) != 0;
						player[i].item_speed = (extra & 16) != 0;
						player[i].item_quad = (extra & 32) != 0;

						//verifica se acabou de morrer - play death sound
						if ((player[i].dead) && (!player[i].old_dead))
							sound(SAMPLE_DEATH + rand() % 2);
						player[i].old_dead = player[i].dead;

						//l,r,u,d  accel
						readByte(data, count, h->keys);
						h->l = ((h->keys & 1) != 0);
						h->r = ((h->keys & 2) != 0);
						h->u = ((h->keys & 4) != 0);
						h->d = ((h->keys & 8) != 0);

						//RUN!!! (COMO QUE NAO TINHA ISSO???)
						//  devo ter apagado sem querer... ????
						h->run = ((h->keys & 16) != 0);

						//bits 5..7 : gundir= 0..7
						h->gundir = (h->keys & (32+64+128)) / 32;

						//read items
						//readByte(data, count, byt);
						//player[i].item_shield =    ((byt & 1) != 0);
						//player[i].item_speed =    ((byt & 2) != 0);
						//player[i].item_quad =    ((byt & 4) != 0);

						//read helm byte
						readByte(data, count, byt);
						player[i].item_helm = byt;
					}
				}

				//read "enemies on team vislist"
				NLushort eviz;
				readShort(data, count, eviz);
				if (me >= 0)
					player[me].enemyvis = eviz;

				//read who,x,y
				NLubyte who,whox,whoy;
				readByte(data, count, who);
				readByte(data, count, whox);
				readByte(data, count, whoy);

				//update this player's px,py,x,y
				//ignore self and anybody onscreen -- because then I've got better accuracy
				if (who != me)
				if (!player[who].onscreen) {
					player[who].x = whox / (255/map.w);	//screen = 0..255 / (WXMAX/255)
					player[who].y = whoy / (255/map.h);
					fx.hero[who].x = (whox % (255/map.w)) * plw / (255/map.w);	//posicao dentro da tela especifica
					fx.hero[who].y = (whoy % (255/map.h)) * plh / (255/map.h);
				}

				//read player's health and energy
				NLubyte healt, energ;
				readByte(data, count, healt);
				if (me >= 0) {
					player[me].health = healt;
					//EXTRA BIT FROM WAYY ABOVE
					if (xtra & 1) player[me].health += 256;
				}

				readByte(data, count, energ);
				if (me >= 0) {
					player[me].energy = energ;
					//EXTRA BIT FROM WAYY ABOVE
					if (xtra & 2) player[me].energy += 256;
				}

				//read ping of player frame % MAX_PLAYERS
				NLushort daping;
				readShort(data, count, daping);
				player[ svframe % maxplayers ].ping = daping;
			}//frame not empty
		}

		//(2) process messages
		//
		char *msg;
		char *lebuf;
		int msglen;
		NLulong		fragz;

		//switch vars
		char *chatmsg;

		do {
			lebuf = msg = client->receive_message(&msglen);
			if (msg != 0) {

				//switch tempvars
				char mapname[128];
				int rids[16]; //rocket ids pra msg 7
				NLubyte rowner, rpx, rpy, code, pid, team, carried, abyte, rockid, iid, rpow, rdir, sx, sy;
				NLshort   rokx, roky;	//rocket hit msg 8
				NLushort	usho, hx, hy;
				NLulong frameno, prank, pscore, nscore;	//v0.4.8 NEG SCORE
				NLfloat aflo;
				char debuf[666]; debuf[0]=0;
				NLshort	ashort, rx, ry;
				int k = 0;
				int count = 0;
				//get msg code
				readByte(msg, count, code);

				//parse rest of message
				switch (code) {

				// name update
				case 1:
					readByte(msg, count, pid);		// player id
					readString(msg, count, player[pid].name);		// da name
					player[pid].name[15] = 0;		// force terminate string (paranoia)
					update_scoreboard();		//tentando consertar bug change teams
					break;

				//text message
				case 2:
					chatmsg = &(msg[1]);		//avoid a useless readString...
					print_message(chatmsg);		//print it to the "console"
					//talk sound
					if ((strlen(chatmsg) >= 2) && (chatmsg[0] == '@') && (chatmsg[1] == 'I')) {
						//don't play talk
					}
					else
						sound(SAMPLE_TALK);

					break;

				//"hello" one-time server information ("first packet")
				case 3:
					readByte(msg, count, pid);	//"who am I"

					//DEBUG msg
					if (pid != whatme) {
						char lixoverde[200];
						sprintf(lixoverde, "###WARNING###: me %i memsg %i whatme %i\n", me, pid, whatme);
						send_chat(lixoverde);
					}

					me = pid;

					//reset want-change-teams: this message is send when players are swapped also
					want_change_teams = false;

					readByte(msg, count, abyte);	//team 0 score
					fx.flag[0].score = abyte;
					readByte(msg, count, abyte);	//team 1 score
					fx.flag[1].score = abyte;

					//server physics parameters
					readFloat(msg, count, aflo);
					svp_fric = aflo;
					readFloat(msg, count, aflo);
					svp_accel = aflo;
					readFloat(msg, count, aflo);
					svp_maxspeed = aflo;
					readFloat(msg, count, aflo);
					svp_fric_run = aflo;
					readFloat(msg, count, aflo);
					svp_accel_run = aflo;
					readFloat(msg, count, aflo);
					svp_maxspeed_run = aflo;
					readFloat(msg, count, aflo);
					svp_fric_turbo = aflo;
					readFloat(msg, count, aflo);
					svp_accel_turbo = aflo;
					readFloat(msg, count, aflo);
					svp_maxspeed_turbo = aflo;
					readFloat(msg, count, aflo);
					svp_fric_turborun = aflo;
					readFloat(msg, count, aflo);
					svp_accel_turborun = aflo;
					readFloat(msg, count, aflo);
					svp_maxspeed_turborun = aflo;
					readFloat(msg, count, aflo);
					svp_flag_penalty = aflo;

					//update scoreboard!
					update_scoreboard();

					break;

				//frags update
				case 4:
					readByte(msg, count, pid);	// what player
					readLong(msg, count, fragz);	// new frag value
					player[pid].frags = fragz;
					update_scoreboard();		// update clientside scoreboard
					break;

				//CTF flag update
				case 6:
					readByte(msg, count, team);	// team of the flag
					readByte(msg, count, carried); // 0==not carried 1==carried
					if (carried == 0) {
						fx.flag[team].carried = false;
						//not carried: update position
						readByte(msg, count, abyte);		//px
						fx.flag[team].pos.px = abyte;
						readByte(msg, count, abyte);		//py
						fx.flag[team].pos.py = abyte;
						readShort(msg, count, ashort);	//x
						fx.flag[team].pos.x = ashort;
						readShort(msg, count, ashort);	//y
						fx.flag[team].pos.y = ashort;
					}
					else {
						fx.flag[team].carried = true;
						//carried: get carrier
						readByte(msg, count, abyte);	//carrier
						fx.flag[team].carrier = abyte;

						sound(SAMPLE_CTF_GOT);
					}
					break;

				//rocket fire notification
				case 7:

					// add to clientside rocket objects list
					//
					//readByte(lebuf, count, rpowdir);	// rocket powerdir
					readByte(lebuf, count, rpow);	// rocket powerdir
					readByte(lebuf, count, rdir);	// rocket powerdir

					//para cada pow, tira um id de shot pra alocar
					for (k=0;k<rpow;k++) {
						readByte(lebuf, count, rockid);	// rocket powerdir
						rids[k] = (int)rockid;
					}

					readLong(lebuf, count, frameno);	// frame # of shot
					readByte(lebuf, count, rowner);	// owner
					readByte(lebuf, count, rpx); //px
					readByte(lebuf, count, rpy); //py
					readShort(lebuf, count, rx); //x
					readShort(lebuf, count, ry); //y

					//rebuild client-side shot
					client_rebuild_shot(rpow, rdir, rids, frameno, rowner, rpx, rpy, rx, ry);

					//play sound if rocket on screen
					if (me >= 0)
					if (rpx == player[me].x)
					if (rpy == player[me].y) {

						//fir sound
						sound(SAMPLE_FIRE);

						//if owner is quaded, play quad sound
						if (player[rowner].item_quad) {
							if (get_time() > player[rowner].quad_sound_finished) {
								sound(SAMPLE_QUAD_FIRE);
								player[rowner].quad_sound_finished = get_time() + 0.1;		//0.3.9 : quad_sound disabled, play always
							}
						}
					}
					break;

				//rocket deletion notification
				case 8:
					readByte(lebuf, count, rockid);	// rocket object id
					readByte(lebuf, count, abyte);	// target player
					//hit position
					readShort(lebuf, count, rokx);
					readShort(lebuf, count, roky);
					fx.rock[rockid].hitx = rokx;
					fx.rock[rockid].hity = roky;
					//signal died
					fx.rock[rockid].hit_time = get_time(); //die now
					//target
					fx.rock[rockid].hit_target = abyte;
					break;

				//CTF team score update
				case 9:
					readByte(lebuf, count, abyte);		//team
					readByte(lebuf, count, rockid);		//new score
					fx.flag[abyte].score = rockid;	// update the score
					break;

				//sound event
				case 14:
					readByte(lebuf, count, abyte);		// sample #
					sound(abyte);
					break;

				//pickup visible
				case 15:
					//print_message("POWERUP_VISIBLE!!!");
					readByte(lebuf, count, iid);		// item id
					readByte(lebuf, count, abyte);		// kind
					fx.item[iid].kind = abyte;
					readByte(lebuf, count, abyte);		// screen x
					fx.item[iid].px = abyte;
					readByte(lebuf, count, abyte);		// screen y
					fx.item[iid].py = abyte;
					readShort(lebuf, count, usho);		// pos x
					fx.item[iid].x = usho;
					readShort(lebuf, count, usho);		// pos y
					fx.item[iid].y = usho;
					break;

				//pickup picked
				case 16:
					readByte(lebuf, count, iid);
					fx.item[iid].kind = 0;		// no more!
					break;

				//powerup clientside timer set
				case 17:
					readByte(lebuf, count, iid);	//kind
					readShort(lebuf, count, usho);	//amount of time
					if (me >= 0) {
						if (iid == 2)
							player[me].item_speed_time = get_time() + ((double)usho);
						else if (iid == 3)
							player[me].item_helm_time = get_time() + ((double)usho);
						else if (iid == 4)
							player[me].item_quad_time = get_time() + ((double)usho);
					}
					break;

				//my weapon notify change
				case 18:
					readByte(lebuf, count, abyte);	// weapon level
					if (me >= 0) {
						player[me].weapon = abyte;
					}
					break;

				//server commands client to change map
				case 20:
					map_ready = false;	// map NOT ready anymore: must load/change
					want_map_exit =false;		// and player does not want to exit the map anymore
					readByte(lebuf, count, abyte);			// read map kind (1=builtin 2=custom)
					if (abyte == 1) {
						readByte(lebuf, count, abyte);		// what built-in map
						server_builtin_map_command(abyte);
					}
					else if (abyte == 2) {
						readShort(lebuf, count, usho);				//read CRC16 of map
						readString(lebuf, count, mapname);		//read map name
						server_map_command(mapname, usho);
					}
					else {
						//FIXME: unknown map kind
					}
					break;

				//server shows gameover plaque
				case 24:
					readByte(lebuf, count, abyte);
					gameover_plaque = abyte;		// kind of plaque (capture limit or vote exit)
					if (gameover_plaque == NEXTMAP_CAPTURE_LIMIT || gameover_plaque == NEXTMAP_VOTE_EXIT) {
						readByte(lebuf, count, abyte);	//RED team final score
						red_final_score = abyte;
						readByte(lebuf, count, abyte);  //BLUE team final score
						blue_final_score = abyte;
					}
					break;

				//server hides gameover plaque
				case 25:
					gameover_plaque = NEXTMAP_NONE;		//hide
					break;

				//deathbringer shot
				case 26:
					//print_message("DEATHBRINGER!!!");
					readByte(lebuf, count, abyte);	//what player
					readLong(lebuf, count, frameno);		// start time
					//spawn clientside fx at the owner's position
					//V0.4.6: sending explicitly the screen/coord of the shot
					readByte(lebuf, count, sx);
					readByte(lebuf, count, sy);
					readShort(lebuf, count, hx);
					readShort(lebuf, count, hy);
					cfx_create_deathbringer(abyte, get_time() + (fx.frame - frameno) * 0.1, hx, hy, sx, sy);

					//cfx_create_deathbringer(abyte, get_time() + (fx.frame - frameno) * 0.1, (int)fx.hero[abyte].x, (int)fx.hero[abyte].y, player[abyte].x, player[abyte].y);
					//if (player[abyte].x == player[me].x)
					//if (player[abyte].y == player[me].y)
					//print_message("DEATHBRINGER ON MY SCREEENN!!!");
					/*
					writeByte(lebuf, count, 26);	//26==deathbringer
					writeByte(lebuf, count, ((NLubyte)target));	//team/target player
					//LIE: x,y can be taken from the player since it's rock-dead on the bringer's position
					//v0.4.6: somehow the graphical effect is playing on the wrong screen of the clients
					//  trying sending screen x,y and player x y
					writeByte(lebuf, count, ((NLubyte)player[target].x));
					writeByte(lebuf, count, ((NLubyte)player[target].y));
					writeShort(lebuf, count, ((NLushort)world.hero[target].x));
					writeShort(lebuf, count, ((NLushort)world.hero[target].y));
					writeLong(lebuf, count, frame);		//frame # of the bringer shot (message can be delayed)
					*/
					//sprintf(debuf, "t %i sx %i sy %i x %i y %i
					//print_message
					break;

				//v0.4.4: UDP FILE DOWNLOAD: incoming chunk
				case 28:
					readByte(lebuf, count, abyte);		//"last chunk"?
					readLong(lebuf, count, frameno);	//absolute pos of the chunk on file
					readShort(lebuf, count, usho);		//chunk size
					//PROCESS IT
					process_udp_download_chunk(abyte, frameno, usho, &(lebuf[count]));
					break;

				//v0.4.4: registration response from server
				case 31:
					readByte(lebuf, count, abyte);
					if (abyte == 1) {
						//success!
						strcpy(namestatus, "RECORDING (");
						strcat(namestatus, player_token);
						strcat(namestatus, ")");
						//code
						namestatus_code = 3;
					}
					else {
						//fail
						strcpy(namestatus, "REJECTED (");
						strcat(namestatus, player_token);
						strcat(namestatus, ")");
					}

					break;

				//v0.4.5: CRAPZ UPDATE message -- updates lots of crap about a player
				case 32:
					readByte(lebuf, count, pid);			//waht player slot
					readByte(lebuf, count, abyte);		//reg char
					readLong(lebuf, count, prank);		//ranking#
					readLong(lebuf, count, pscore);		//score
					readLong(lebuf, count, nscore);		//score	NEG v0.4.8
					readLong(lebuf, count, max_world_rank);		//world players count
					readLong(lebuf, count, max_world_score);		//world score max
					player[pid].reg_status = (char)abyte;
					player[pid].rank = (int)prank;
					player[pid].score = (int)pscore;
					player[pid].neg_score = (int)nscore;
					//LOG4("CRAPZ UPDATE %i %c %i %i\n", pid, abyte, prank, pscore);
					break;

				default:
					LOG1("BAD ERROR: UNKNOWN SERVER MESSAGE CODE = %i!!\n", code);
					break;
				}
			}
		} while (msg != 0);

		// (3) calc stuff dependant on received data
		//

		//detect screen changes / clear powerup fx's
		if (me >= 0)
		if ((player[me].x != player[me].oldx) || (player[me].y != player[me].oldy)) {
			//screen changed.

			//V0.4.7: now, why would I want to do that??
			//clear all clientside fx's
			//for (int f=0;f<MAX_CLIENTFX;f++)
			//	cfx[f].used = false;

			//clear all powerups on my old screen
			for (int j=0;j<MAX_PICKUPS;j++)
			if (fx.item[j].kind)	// item present
			if (fx.item[j].px == player[me].oldx)	//on old screen
			if (fx.item[j].py == player[me].oldy)
				fx.item[j].kind = 0;	// erase

			//set new old's
			player[me].oldx = player[me].x;
			player[me].oldy = player[me].y;
		}

		//this is a HACK:
		me = whatme;

		//unlock frame mutex
		//LOG("unlocking INCOMING\n");
		pthread_mutex_unlock( &frame_mutex );
		//LOG("unlocked! INCOMING\n");
	}

	//send chat message
	void send_chat(char *msg) {
		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 2);	//want to chat!
		writeString(lebuf, count, msg);	// the message
		client->send_message(lebuf, count);
	}

	//print message to "console"
	void print_message(const char *msg) {

		// find top
		int top = 0;
		if (msg[0] == '\0')
			top = CHAT_SIZE;
		else {
			for (int i=0;i<CHAT_SIZE;i++)
			if (chatbuffer[i][0] != 0)
				top = i + 1;
		}

		// back up if top == CHAT_SIZE
		if (top >= CHAT_SIZE) {
			for (int i=0;i<CHAT_SIZE - 1;i++)
				strcpy(chatbuffer[i], chatbuffer[i+1]);
			top = CHAT_SIZE - 1;
		}

		// set new at top position
		strcpy(chatbuffer[top], msg);

		// print message to log, if it is not empty
		if (msg[0] != '\0') {
			// date and time
			time_t tt = time(0);
			struct tm* tmb = localtime(&tt);
			message_log << tmb->tm_year + 1900 << '-' << setfill('0') << setw(2) << tmb->tm_mon + 1
				<< '-' << setfill('0') << setw(2) << tmb->tm_mday
				<< ' ' << setw(2) << tmb->tm_hour << ':' << setfill('0') << setw(2) << tmb->tm_min << ':'
				<< setfill('0') << setw(2) << tmb->tm_sec << "  ";
				// message
			message_log << (msg[0] == '@' ? msg + 2 : msg) << '\n';
		}

		// time to erase
		chaterasetime = get_time() + 10.0;
	}

	//save screenshot
	void save_screenshot() {

		// find the filename
		char fname[256];
		int i = 0;
		do {
			if (i<10) sprintf(fname, "outgun0%i.tga", i); else sprintf(fname, "outgun%i.tga", i);
			if (!exists(fname))
				break;
			i++;
		} while (i < 99);

		//dump
	BITMAP *bmp;
	PALETTE pal;
	get_palette(pal);
	bmp = create_sub_bitmap(screen, 0, 0, SCREEN_W, SCREEN_H);
	save_bitmap(fname, bmp, pal);
	destroy_bitmap(bmp);

		//nice message
		char lixox[256];
		sprintf(lixox, "saved screenshot to %s.", fname);
		print_message(lixox);
	}

	//toggle help screen
	void toggle_help() {

		helpshow = !helpshow;

		if (helpshow)
			sound(SAMPLE_WEAPON_UP);
		else
			sound(SAMPLE_FIRE);
	}

	//show progress (for tight loops that don't work with the regular screen flip loop)
	void show_progress(char *t1, char *t2, char *t3, int fg = -1, int bg = 0) {
		if (fg == -1) fg = col[COLWHITE];
		vsync();
		acquire_screen();
		rect(screen, 320 - 200-1, 240 - 50-1, 320 + 200-1, 240 + 50-1, col[COLWHITE]);
		rect(screen, 320 - 200+1, 240 - 50+1, 320 + 200+1, 240 + 50+1, col[COLDARKGRAY]);
		rectfill(screen, 320 - 200, 240 - 50, 320 + 200, 240 + 50, bg);
		textprintf_centre(screen, font, 320, 240 - 25, fg, t1);
		textprintf_centre(screen, font, 320, 240     , fg, t2);
		textprintf_centre(screen, font, 320, 240 + 25, fg, t3);
		release_screen();
	}

	//show progress / press any key / dialog
	void show_dialog(char *t1, char *t2, char *t3, int fg = -1, int bg = 0) {
		clear_keybuf();
		do {
			show_progress(t1,t2,t3,fg,bg);
			if (keypressed())
				break;
			MS_SLEEP(50);
		} while (1);
		//wait until ESC released
		while (key[KEY_ESC]) ;
		clear_keybuf();
	}

	//GET SERVERS FROM MASTER!!!
	void get_servers_from_master() {

		int i = 0;
		NLsocket sock;

		//open a nonblocking socket
		nlDisable(NL_BLOCKING_IO);
		sock = nlOpen(0, NL_RELIABLE);
		if (sock == NL_INVALID) {
			//show "cant open socket to master" error
			show_dialog("ERROR", "Can't open socket!", "Press any key.", 0,makecol(0xff,0xaa,0xaa));
			return;
		}

		//connect the nonblocking way
		nlConnect(sock, &master_address);

		//build query
		char querybuf[1024]; int qcount = 0;
		writeString(querybuf, qcount, "GET /servlet/fcecin.m3/index.html?get=x\n\n");
		qcount--;	//take the zero out

		show_progress("Getting updated internet server list", "Contacting server...", "Press ESC to cancel");

		//keep trying to write the query until user presses ESC
		NLint result;
		do {
			result = nlWrite(sock, querybuf, qcount);
			MS_SLEEP(10);
			if (key[KEY_ESC]) {
				// 'attempt cancelled'
				nlClose(sock);
				clear_keybuf(); //clear keystrokes buffer
				while (key[KEY_ESC]); //wait to release esc
				return;
			}
		} while ((result == NL_INVALID) && (nlGetError() == NL_CON_PENDING));

		//check bogus
		if (result == NL_INVALID) {
			nlClose(sock);
			// show 'some problem occured try later'
			show_dialog("Problem connecting to master server (1).", "Try again later.", "Press any key.", 0, makecol(0xff,0x88,0x88));
			return;
		}

		show_progress("Getting updated internet server list", "Waiting response...", "Press ESC to cancel");

		//log ok
		LOG3("QUERY TO MASTER '%s', result = %i, count = %i\n", querybuf, result, qcount);

		//FIXME: show 'request sent... waiting reply'

		//try to read the reply or until user presses ESC
		//parse the response (should be <HTML><BODY> etc... with "@I @I @I ... @K" on it
		bool html_end = false;
		int nostuffcound = 0;
		char lebuf[65536];
		int n = 0;
		do {

			//read
			result = nlRead(sock, &(lebuf[n]), 1);

			//no byte
			if (result == 0) {

				if (nostuffcound > 0) {
					nostuffcound++;

					if (html_end) {
						if (nostuffcound > 200) {		//2 seconds after it came some stuff but now without coming more stuff
							lebuf[n+1] = 0;
							LOG("2 SEC TIMEOUT READING STUFF AFTER </HTML>");
							LOG1("---- Full response START ----\n%s\n---- Full response END ----\n", lebuf);
							break;
						}
					}
					//did not receive end yet -- then wait...
					else {
					}
				}

				MS_SLEEP(10);
				if (key[KEY_ESC]) {
					// 'attempt cancelled'
					nlClose(sock);
					clear_keybuf(); //clear keystrokes buffer
					while (key[KEY_ESC]); //wait to release esc
					return;
				}
			}

			//error occured
			if (result == NL_INVALID) {

				//if already got html_end, no error
				if (html_end)
					break;

				LOG1("MASTER CLIENT QUERY ERROR READING RESPONSE result = %i\n", result);
				//show 'some problem occured try later'
				/*
				sprintf(dialogmessage, "Problem connecting! Try later. (2)");//%i %s %i %s", nlGetError(), nlGetErrorStr(nlGetError()), nlGetSystemError(), nlGetSystemErrorStr(nlGetSystemError()));
				strcpy(dialogmessage2, "Press ESC");
				set_menu(2);	//dialog menu
				*/
				nlClose(sock);
				show_dialog("Problem connecting to master server (2).", "Try again later.", "Press any key.", 0, makecol(0xff,0x88,0x88));
				return;
			}

			//received anything below 32: turn them into "+" signals...
			if (lebuf[n] < 32)
				lebuf[n] = '+';

			//check for received </HTML>
			if (n >= 6) {
				if (
					(lebuf[n-6] == '<') &&
					(lebuf[n-5] == '/') &&
					((lebuf[n-4] == 'h') || (lebuf[n-4] == 'H')) &&
					((lebuf[n-3] == 't') || (lebuf[n-3] == 'T')) &&
					((lebuf[n-2] == 'm') || (lebuf[n-2] == 'M')) &&
					((lebuf[n-1] == 'l') || (lebuf[n-1] == 'L')) &&
					(lebuf[n-0] == '>')
				)
				{
					LOG1("CLIENT MASTER QUERY RECEIVED </HTML>! SUCCESS!! n=%i\n", n);
					html_end = true;
					nostuffcound = 1;
					lebuf[n+1] = 0;
					LOG1("---- Full response START ----\n%s\n---- Full response END ----\n", lebuf);
					break;
				}
			}

			//check for received another <HTML> : reset all stuff
			if (n >= 5) {
				if (
					(lebuf[n-5] == '<') &&
					((lebuf[n-4] == 'h') || (lebuf[n-4] == 'H')) &&
					((lebuf[n-3] == 't') || (lebuf[n-3] == 'T')) &&
					((lebuf[n-2] == 'm') || (lebuf[n-2] == 'M')) &&
					((lebuf[n-1] == 'l') || (lebuf[n-1] == 'L')) &&
					(lebuf[n-0] == '>')
				)
				{
					lebuf[n+1]=0;
					//LOG1("\n** READ <HTML>, DISCARDING BUFFER '%s' **\n", lebuf);
					html_end = false;
					n = -1;
				}
			}

			//read next
			n++;

		} while (1);

		//clear the old gamespy master screen
		//
		for (int j=0;j<MAX_GAMESPY;j++) {
			mgamespy[j].address[0]=0;
			mgamespy[j].refreshed = false;
			mgamespy[j].invalid = false;	//don't know the status yet
			mgamespy[j].favs = false;
		}

		//parse the successful response into the gamespy screen
		//
		int c = 0;
		int m = 0;		//gamespy entry
		bool found_k = false;
		bool found_i = false;
		do {

			// check command char
			if (lebuf[c++] == '@') {

				//check IP char
				if (lebuf[c] == 'I') {

					found_i = true; //found an @I

					//point to first char of IP
					c++;

					//parse IP into a buf
					char ipbuf[30];
					int  ic = 0;
					do {

						//copy one
						ipbuf[ic++] = lebuf[c++];

					} while (
						//V0.4.2: ":" para port number
						(lebuf[c] == '1') || (lebuf[c] == '4') || (lebuf[c] == '7') || (lebuf[c] == '0') ||
						(lebuf[c] == '2') || (lebuf[c] == '5') || (lebuf[c] == '8') || (lebuf[c] == '.') ||
						(lebuf[c] == '3') || (lebuf[c] == '6') || (lebuf[c] == '9') || (lebuf[c] == ':')
					);

					//zero terminate
					ipbuf[ic] = 0;

					//copy if enough room
					if (m < MAX_GAMESPY) {
						strcpy(mgamespy[m].address, ipbuf);
						m++;	//next entry
					}
				}
				//check DONE char
				else if (lebuf[c] == 'K') {
					//done!
					found_k = true;
					break;
				}
			}

		} while (lebuf[c] != 0);

		//copy addresses from favourites to holes in master entries, anyway
		//if (!found_k) {
		// for (int h=0;h<MAX_GAMESPY;h++)
		// strcpy(mgamespy[h].address, gamespy[h].address);
		//}
		int f = 0;	//favorites entry
		int minf = m;		//first favorites entry
		while ((m < MAX_GAMESPY) && (f < MAX_GAMESPY)) {		//slot in master list
			if (gamespy[f].address[0] != '\0') {

				//scan for duplicate: then ignore
				bool dup = false;
				for (i=0;i<MAX_GAMESPY;i++)
					if (!strcmp(gamespy[f].address, mgamespy[i].address)) {
						dup = true;		//dup
						break;
					}

				//no dup? add
				if (!dup) {
					strcpy(mgamespy[m].address, gamespy[f].address);
					mgamespy[m].favs = true;
					m++;	//next m slot
				}
			}
			f++;	//next f anyways
		}

		//refresh (has own progress dialog)
		//
		// OBS.: will refresh even if master server fails -- refreshes favourites
		//
		refresh_command();

		//remove all invalid IPs
		//
		int e;
		for (e=0;e<MAX_GAMESPY;e++)
			if (mgamespy[e].invalid)	//erase address
				mgamespy[e].address[0]=0;

		//remove all "no response"s below minf
		//
		for (e=minf;e<MAX_GAMESPY;e++)
			if (mgamespy[e].noresponse)	{ //erase address
				mgamespy[e].address[0]=0;
				mgamespy[e].invalid = true;
			}

		//compress entries
		//
		gamespy_t  temp[MAX_GAMESPY];
		memcpy(temp, mgamespy, sizeof(gamespy_t)*MAX_GAMESPY);	//copy to temp
		for (e=0;e<MAX_GAMESPY;e++) {
			mgamespy[e].address[0]=0;	//erase all master
			mgamespy[e].favs=false;
			//0.4.1:
			mgamespy[e].invalid = true;		//address is invalid
		}
		int next=0;	//master entry next
		for (e=0;e<MAX_GAMESPY;e++)
			if (temp[e].address[0] != 0) {
				memcpy(&(mgamespy[next]), &(temp[e]), sizeof(gamespy_t));	//copy back to master
				next++;
			}

		//show an error dialog if no @K in message
		if (!found_k) {
			if (found_i)
				show_dialog("ERROR: corrupted response.", "Try again later.", "Press any key.", 0, makecol(0xff,0x88,0x88));
			else
				show_dialog("ERROR: service unavailable", "Try again in a minute.", "Press any key.", 0, makecol(0xff,0x88,0x88));
		}

		//close socket
		//
		nlClose(sock);
	}

	//loop
	void loop() {

		bool notquit = true;

		//show menu and not game yet
		menushow = true;
		set_menu(0);
		gameshow = false;

		//reset speed counter
		speed_counter = 0;
		client_netsend_counter = 0;

		//loop
		bool quick_fix, key_fire=false, key_kill=false, key_swap=false, key_votexit=false;
		char key_up=0, key_down=0, key_left=0, key_right=0;
		int i;
		while (notquit) {

			//LOG("** another loop()...\n");

			// (-1) try to release "reverse" voices that have finished playing
			//
			// versao 0.4.1 : EARLY OPTIMIZATION IS THE ROOT OF ALL EVIL
			// versao 0.4.1 : EARLY OPTIMIZATION IS THE ROOT OF ALL EVIL
			// versao 0.4.1 : EARLY OPTIMIZATION IS THE ROOT OF ALL EVIL
			/*
			for (int voi=0;voi<16;voi++)		//FIXME: just guessing at 16 voices...
				if (voice_check(voi) != 0)
					if (voice_get_position(voi) == -1) {
						//char vois[200];
						//sprintf(vois, "voice %i finished", voi);
						//print_message(vois);
						release_voice(voi);
					}
			*/

			// (0) check for time to send delayed messages
			//
			//bot preferences update
#ifndef	NO_BOTS

			if (bot_pref_change)
			if (get_time() > bot_pref_time)
			{
				bot_pref_change = false;

				char lebuf[256]; int count = 0;
				writeByte(lebuf, count, 19);		//19==update preferences
				writeByte(lebuf, count, 1);			//1 == bot preferences
				writeByte(lebuf, count, botmode);
				writeByte(lebuf, count, botsize);
				client->send_message(lebuf, count);
			}
#endif // NO_BOTS

			// (1) loop doing input/sleep before next simulation/draw time
			//
			quick_fix = true;
			do {

				//quit key
				if ((key[KEY_LCONTROL]) || (key[KEY_RCONTROL]))
				if (key[KEY_F12]) {
					notquit = false;
					break;
				}

				//help showing
				if (helpshow) {

					while (keypressed()) {
						//get key
						int ch = readkey();
						int sc = ch >> 8;	//scancode
						ch = ch & 0xff;			//char

						//toggle help
						if (sc == KEY_F1)
							toggle_help();
					}
				}
				//menu keypresses (from char buf) - ESC already dealed with, ignore
				else if (menushow) {
					while (keypressed()) {
						string lerud_abloxon;

						//get key
						int ch = readkey();
						int sc = ch >> 8;	//scancode
						ch = ch & 0xff;			//char

						//screenshot
						if (sc == KEY_F11) {
							save_screenshot();
						}

						//toggle help
						if (sc == KEY_F1)
							toggle_help();

						//test key
						switch (menu) {
						//main menu
						case 0:
							if ((key[KEY_SPACE]) && (sc == KEY_F8)) {
								port++;
								if (port > 25005)
									port = 25000;
							}
							else if (sc == KEY_F10) {
								lerud_abloxon = RandomName();
								strcpy(editplayername, lerud_abloxon.c_str());
								change_name_command();
								FreeName(lerud_abloxon);
							}
							switch (ch) {
							case '1': set_menu(1); break;
							case '2': disconnect_command(); break;
							case '3':
								strcpy(editplayername, playername);
								strcpy(editplayerpass, player_password);
								strcpy(namecursor, "_");
								strcpy(passcursor, "");
								set_menu(3);
								break;
							case '4': // start/stop listenserver
								if (listen_server_running)
									listen_stop();
								else
									listen_start();
								break;
							case '5':
								winclient = !winclient;
								reset_video_mode();
								break;
							case '6': next_sfx_theme(); break;
							default:;
							}
							break;
						//connect screen
						case 1:
							if (showmaster)
								i = strlen(mgamespy[gi].address);
							else
								i = strlen(gamespy[gi].address);
							if (
										//v0.4.2: including +6 chars for :xxxxx (port)
										(i < 21)
										//(i < 16)	// max length of IP address typein
										&&
										//v0.4.2 ":" para port#
										(((ch >= '0') && (ch <= '9')) || (ch == '.') || (ch == ':'))
							) {
								if (showmaster) {
									mgamespy[gi].address[i] = (char)ch;
									mgamespy[gi].address[i+1] = 0;
									mgamespy[gi].refreshed = false;
								}
								else {
									gamespy[gi].address[i] = (char)ch;
									gamespy[gi].address[i+1] = 0;
									gamespy[gi].refreshed = false;
								}
							}
							else if (sc == KEY_UP) {
								gi--;
								if (gi < 0)
									gi = MAX_GAMESPY-1;
							}
							else if (sc == KEY_DOWN) {
								gi++;
								if (gi >= MAX_GAMESPY)
									gi = 0;
							}
							else if (sc == KEY_SPACE) {

								refresh_command();
								clear_keybuf();	//clear the key buffer to stop too much refreshing.
							}
							else if (sc == KEY_BACKSPACE) {
								if (i > 0) {
									if (showmaster) {
										mgamespy[gi].address[i-1] = 0;
										mgamespy[gi].refreshed = false;
									}
									else {
										gamespy[gi].address[i-1] = 0;
										gamespy[gi].refreshed = false;
									}
								}
							}
							else if (sc == KEY_ENTER) {
								connect_command();
							}
							else if (sc == KEY_TAB) {
								showmaster = !showmaster;

								//make the first refresh of favorites when showing it
								if ((!showmaster) && (first_fav_refresh == false)) {
									first_fav_refresh = true;
									refresh_command();
								}
							}
							else if ((sc == KEY_F2) && (showmaster)) {
								//update from master
								get_servers_from_master();
							}
							break;
						//dialog screen : just ESC
						case 2:
							break;
						//change name screen
						case 3:

							if (namecursor[0]=='_')
								i = strlen(editplayername);
							else
								i = strlen(editplayerpass);

							if (((ch >= '0') && (ch <= '9')) || ((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z')) || (ch == '-') || (ch == '_')) {

								if (namecursor[0]=='_') {
									if (i < 15) {
										editplayername[i] = (char)ch;
										editplayername[i+1] = 0;
										editplayerpass[0]=0; //reset password after editing name
									}
								}
								else {
									if (i < 8) {
										editplayerpass[i] = (char)ch;
										editplayerpass[i+1] = 0;
									}
								}
							}
							else if (sc == KEY_BACKSPACE) {
								if (i > 0) {
									if (namecursor[0]=='_') {
										editplayername[i-1] = 0;
										editplayerpass[0]=0; //reset password after editing name
									}
									else
										editplayerpass[i-1] = 0;
								}
							}
							else if (sc == KEY_ENTER) {
								change_name_command();
								//get it (V0.4.9) -- força um "ENTER" logo apos o cara conectar ou trocar de nome ao estar
								//conectado...
								check_change_pass_command();
							}
							else if (sc == KEY_TAB) {
								//switch fields
								if (namecursor[0]==0) {
									strcpy(namecursor, "_");
									strcpy(passcursor, "");
								}
								else {
									strcpy(namecursor, "");
									strcpy(passcursor, "_");
								}
							}
							break;
						//wtf?
						default:;
						}
					}
				}
				// menu not showing: send keypresses to the game
				else {

					bool sendnow = false;

					// ctrl == fire event
					//
					if (key[KEY_LCONTROL] || key[KEY_RCONTROL]) {
						if (!key_fire) {
							key_fire = true;

							//"fire" message (+ATTACK)
							char lebuf[16]; int count = 0;
							writeByte(lebuf, count, 5);	// 5 = +attack (fire button down)
							client->send_message(lebuf, count);

							//send early keys packet
							sendnow = true;
						}
					}
					else {
						if (key_fire) {
							key_fire = false;

							//"un-fire" message (-ATTACK)
							char lebuf[16]; int count = 0;
							writeByte(lebuf, count, 11);	// 11 = -attack (fire button up)
							client->send_message(lebuf, count);

							//send early keys packet
							sendnow = true;
						}
					}

					// del == suicide event
					//
					if (key[KEY_DEL]) {
						if (!key_kill) {
							key_kill = true;

							//"suicide" message
							char lebuf[16]; int count = 0;
							writeByte(lebuf, count, 10);	// 10 = suicide!!
							client->send_message(lebuf, count);
						}
					}
					else key_kill = false;

					// page down == drop flag
					//
					if (key[KEY_PGDN]) {
						char lebuf[16]; int count = 0;
						writeByte(lebuf, count, 34);	// 34 = drop flag
						client->send_message(lebuf, count);
					}

					// end == want/don't want swap team
					//
					if (key[KEY_END]) {
						if (!key_swap) {
							key_swap = true;

							//toggle my local option
							want_change_teams = !want_change_teams;

							//want to swap/dont want  message
							char lebuf[16]; int count = 0;
							if (want_change_teams)
								writeByte(lebuf, count, 12);	// 12 -- want
							else
								writeByte(lebuf, count, 13);	// 13 -- dont want
							client->send_message(lebuf, count);
						}
					}
					else key_swap = false;

					// F4 == want/don't want to exit map
					//
					if (key[KEY_F4]) {
						if (!key_votexit) {
							key_votexit = true;

							//toggle my local option
							want_map_exit = !want_map_exit;

							//want to swap/dont want  message
							char lebuf[16]; int count = 0;
							if (want_map_exit)
								writeByte(lebuf, count, 22);	// 22 -- want
							else
								writeByte(lebuf, count, 23);	// 23 -- dont want
							client->send_message(lebuf, count);
						}
					}
					else key_votexit = false;

					// l,r,u,d,fire game keys
					//
					if ((key[KEY_UP]    != key_up)    ||
						(key[KEY_DOWN]  != key_down)  ||
						(key[KEY_LEFT]  != key_left)  ||
						(key[KEY_RIGHT] != key_right)) {
						//keys changed
						key_up    = key[KEY_UP];
						key_down  = key[KEY_DOWN];
						key_left  = key[KEY_LEFT];
						key_right = key[KEY_RIGHT];
						//send early keys packet
						sendnow = true;
					}

					//send client's input packet now
					if (sendnow) {
						//V0.3.9: just speed up increasing the netsend counter if it's too small
						//send_frame();
						//client_netsend_counter = 3;	//should be set to 0 but we want to send a new packet soon after this one
						if (client_netsend_counter < 4)
							client_netsend_counter = 4;
					}

					// keypresses to talk prompt
					//
					while (keypressed()) {
						//get key
						int ch = readkey();
						int sc = ch >> 8;	//scancode
						ch = ch & 0xff;			//char

						i = strlen(talkbuffer);

						//toggle help
						if (sc == KEY_F1)
							toggle_help();

						//change colours
						if (sc == KEY_HOME) {
							flagpos_ready = false;	// flag position mark colour not right anymore
							if (key[KEY_LCONTROL] || key[KEY_RCONTROL]) {
								col[COLGROUND] = makecol(0x10, 0x40, 0);
								col[COLWALL] = makecol(0x30, 0xC0, 0);

								//col[COLGREEN] = makecol(0,0xff,0);
							}
							else {
								col[COLGROUND] = makecol(rand() % 256, rand() % 256, rand() % 256);
								col[COLWALL] = makecol(rand() % 256, rand() % 256, rand() % 256);

								/*
								int r,g,b;
								r = rand() % 256;
								g = rand() % 256;
								b = rand() % 256;

								col[COLGREEN] = makecol(r,g,b);

								char mes[200];
								sprintf(mes, "%3i %3i %3i", r,g,b);
								print_message(mes);
								*/
							}
						}

						// ins == change name
						//
						if (sc == KEY_F10) {
							string lerud_abloxon = RandomName();
							strcpy(editplayername, lerud_abloxon.c_str());
							change_name_command();
							FreeName(lerud_abloxon);
						}
						else if (sc == KEY_PGUP) {
							//v0.4.9 DEBUGGING: request broadcast my crap
							char lebuf[4]; int count = 0;
							writeByte(lebuf, count, 33);		// 33 = "refresh crap"
							client->send_message(lebuf, count);
						}
						else if (sc == KEY_F3) {
							option_show_names = !option_show_names;
						}
#ifndef	NO_BOTS
						// BOT PREFERENCES: F5: less bots (botmode 2)
						else if (sc == KEY_F5) {
							if ((botsize > 0) || (botmode != 2)) {
								botmode = 2;
								if (botsize > 0)
									botsize--;
								//send update to server:
								bot_pref_change = true;
								bot_pref_time = get_time() + 3.0;
							}
						}
						// BOT PREFERENCES: F6: more bots (botmode 2)
						else if (sc == KEY_F6) {
							if ((botsize < TSIZE) || (botmode != 2)) {
								botmode = 2;
								//v0.4.7 : maximum botsize allowed
								if ( (botsize < TSIZE) && (botsize < MAX_BOTSIZE) )
									botsize++;
								//send update to server:
								bot_pref_change = true;
								bot_pref_time = get_time() + 3.0;
							}
						}
						// BOT PREFERENCES: F7: NO BOTS (v0.4.7) (botmode 1)
						else if (sc == KEY_F7) {
							if (botmode != 1) {
								botmode = 1;
								//send update to server:
								bot_pref_change = true;
								bot_pref_time = get_time() + 3.0;
							}
						}
						// BOT PREFERENCES: F8: default/no opinion (botmode 0)
						else if (sc == KEY_F8) {
							if (botmode != 0) {
								botmode = 0;
								//send update to server:
								bot_pref_change = true;
								bot_pref_time = get_time() + 3.0;
							}
						}
#endif // NO_BOTS
						// F11:screenshot
						else if (sc == KEY_F11) {
							save_screenshot();
						}
						//backspace erase one
						else if (sc == KEY_BACKSPACE) {
							if (i>0) talkbuffer[i-1] = 0;
						}
						//enter key: submit text
						else if (sc == KEY_ENTER || sc == KEY_ENTER_PAD) {
							if (i > 0) {
								send_chat(talkbuffer);
								talkbuffer[0]=0;
							}
						}
						//else text add keys. max text length = 60
						else if (i < 60 && ch >= 32) {
							talkbuffer[i] = (char)ch;
							talkbuffer[i+1] = 0;
						}
					}
				}

				//ESC = show/hide menu, go back menu (special key)
				static bool kesc = false;
				if (key[KEY_ESC]) {
					if (!kesc) {
						kesc = true;
						if (talkbuffer[0] != '\0') // cancel chat
							talkbuffer[0] = '\0';
						else if (trying_connection) {		//trying connection
							trying_connection = false;	//not anymore

							//this cancels the attempt to connect
							client->connect(false);
							//go back to main screen
							set_menu(0);
						}
						//se mostrando help, quita
						else if (helpshow) {
							toggle_help();
						}
						else if (!menushow) {		// no menu, show
							menushow = true;
							set_menu(0);
						}
						else {		// menu
							if (menu > 0)		// go back one screen
								set_menu(0);
							else						// hide menu
								menushow = false;
						}
					}
				}
				else
					kesc = false;

				//sleep a bit
				if (quick_fix)
					quick_fix = false;
				else
					MS_SLEEP(2);				// *** OPTIMIZE THIS ***

			} while (speed_counter < 1);

			//LOG("** ...exited spd counter>0\n");

			//must be time for another frame later
			while (speed_counter > 0) {
				speed_counter--;
				client_netsend_counter++;
			}

			//speed_counter == 60 FPS
			// client netsend == 10 FPS
			// so when netsend == 6, it's reset to zero and the packet is sent
			if (client_netsend_counter >= (targetfps / 10) ) {
				client_netsend_counter = 0;
				send_frame();
			}

			// (2) if game is showing, simulate
			//
			if (gameshow) {
				//LOG("** ...calc game frame\n");
				calc_game_frame(); //calculate game frame to show
				//LOG("** ...game frame calced ok\n");
			}

			// (3) draw operations
			//
			//if (page_flipping) {
				//LOG1("acquire_bitmap/gs=%i...",gameshow);
				acquire_bitmap(drawbuf);
				//LOG("OK\n");
			//}

			if (gameshow) {
				//clear_to_color(drawbuf, makecol(rand(),0,0));	// clear buffer
				//LOG("draw_game_frame()\n");
				draw_game_frame(drawbuf); // draw game frame
				//LOG("exit draw_game_frame()\n");
			} else {
				clear_to_color(drawbuf, 0);	// clear buffer
				int co = makecol(0x22, 0x22, 0x22);
				textprintf(drawbuf, font, 0, 0, co, "page-flipping = %i", page_flipping);
				textprintf(drawbuf, font, 0, 10, co, "port = %i", port);
			}

			//force menu open if no game
			if (!gameshow)
				menushow = true;

			if (menushow)
				draw_game_menu();	// draw the game menu

			if (helpshow)
				draw_game_help();	// draw help

			//if (page_flipping) {
				//LOG("** releasing bitmap...");
				release_bitmap(drawbuf);
				//LOG("OK!\n");
			//}

			// (4) flip or blt
			//
			if (page_flipping) {
				show_video_bitmap(drawbuf);

				if (drawbuf == vidpage1)
					drawbuf = vidpage2;
				else
					drawbuf = vidpage1;
			}
			else {
				acquire_screen();
				blit(drawbuf, screen, 0,0, 0,0, SCREEN_W,SCREEN_H);
				release_screen();
			}
		}

		//client exit cleanup: done at stop wich needs to be called after loop
	}

	//stop
	void stop() {

		//at least disconnect
		disconnect_command();

		//join with token request thread, if any
		if (player_password_set) {
			LOG("**** CLIENT JOINING PASSWORD-TOKEN THREAD.... ****\n");
			player_password_set = false;
			pthread_join( passthread, 0 );
		}

		//clear all samples
		unload_samples();

		for (int i = 0; i < 2; i++)
			destroy_bitmap(flagpos_buf[i]);

		//save configuration file
		//try to load client configuration
		char dest[WHERE_PATH_SIZE];
		append_filename(dest, wheregamedir, "clconfig.txt", WHERE_PATH_SIZE);
		LOG1("dest for clconfig.txt OUT = %s\n", dest);

		FILE *cfg = fopen(dest, "w");
		if (cfg) {

			//0.4.7: no theme dir?
			if (validtheme)
				fprintf(cfg, "%s\n", sfxthemedir);
			else
				fprintf(cfg, "NO_SFX_THEME_DIR\n");

			//v0.4.7: empty name?
			if (playername[0] != '\0')
				fprintf(cfg, "%s\n", playername);
			else
				fprintf(cfg, "Unnamed_Bastard\n");

			for (int i=0;i<MAX_GAMESPY;i++) {
				LOG1("SAVING GAMESPY ADDRESS = '%s'\n", gamespy[i].address);
				fprintf(cfg, "%s\n", gamespy[i].address);
			}
			fclose(cfg);
		}

		//save client's password
		LOG("Saving password file...\n");
		append_filename(dest, wheregamedir, "password.bin", WHERE_PATH_SIZE);
		FILE *psf = fopen(dest, "wb");
		if (psf) {

			char cha;
			for (int c=0;c<PASSBUFFER;c++) {
				if (c < (int)strlen(player_password)) {
					//write toggling bits
					int rot = player_password[c];
					rot = 255 - rot;
					cha = (char)rot;
					fputc(cha, psf);
				}
				else
					fputc(255, psf);		//255 = 0 toggled! (important)
			}

			fclose(psf);
		}
		else
			LOG("ERROR: CANNOT OPEN PASSWORD FILE FOR WRITING\n");

		//clear udpdq
		for (int uq=0;uq<MAX_UDPDQ;uq++)
			if (udpdq[uq] != 0) {
				delete udpdq[uq];
				udpdq[uq] = 0;
			}
		message_log.close();
	}

	//ctor
	gameclient_c() {

		//net client
		client = 0;

		//not showing
		option_show_names = false;

		//all the players to show including me
		//player_t player[MAX_PLAYERS];
		for (int p=0;p<MAX_PLAYERS;p++)
			player[p].used=false;

		//explosion fx's
		for (int x=0;x<MAX_CLIENTFX;x++)
			cfx[x].used = false;

		//wich player I am
		me = -1;

		//time of last packet received
		lastpackettime=0;

		//audio samples
		for (int s=0;s<NUM_OF_SAMPLES;s++)
			sample[s]=0;

		//menu showing?
		menushow = false;
		menu = 0;		//menu screen #

		//game showing?
		gameshow = false;

		//frames and seconds for FPS counter
		FPS=0;
		framecount = 0;
		totalframecount = 0;
		starttime = 0;

		//if player wants to changeteams
		want_change_teams = false;

		//trying connection? if true, ESC cancels it
		trying_connection = false;

		//connected? (that is, "connection accepted")
		connected = false;

		//connect screen, my "mini-gamespy"
		gi=0;	//what game entry

		playername[0]=0; //the player's name (max name len = 16)
		namestatus[0]=0;
		editplayername[0]=0; //the player's name edit buffer
		address[0]=0;		//server IP address
		dialogmessage[0]=0;	//dialog message
		dialogmessage2[0]=0;	//dialog message line 2
		talkbuffer[0]=0;			// chat input buffer
		for (int i=0;i<CHAT_SIZE;i++)	// last chat messages list
			chatbuffer[i][0]=0;
		chaterasetime = 0;				// time to erase a chat message from the list
		minibg = 0;		//bitmap for minimap background (level walls, screen grid)

		pthread_mutex_init(&frame_mutex, 0);

		pthread_mutex_init(&udpdq_mutex, 0);		//UDP download queue
		udpdq_size = 0;
	}

	//dtor
	virtual ~gameclient_c() {

		if (client) {
			delete client;
			client = 0;
		}

		pthread_mutex_destroy(&frame_mutex);

		pthread_mutex_destroy(&udpdq_mutex);
	}

};


void gameclient_c::check_flagpos_marks() {
	if (!flagpos_ready) {
		const int radius = FLAGPOS_RAD;
		for (int i = 0; i < 2; i++) {
			if (!flagpos_buf[i])
  				flagpos_buf[i] = create_bitmap(2 * radius, 2 * radius);
			clear_to_color(flagpos_buf[i], col[COLGROUND]);
		}
		int r1, r2;
		int b1, b2;
		r1 = r2 = getr(col[COLGROUND]);
		const int g = getg(col[COLGROUND]);
		b1 = b2 = getb(col[COLGROUND]);
		const int step = 10;
		for (int i = radius; i >= 0; i--) {
			// red flag
			r1 = min(r1 + step, 255);
			circlefill(flagpos_buf[0], radius, radius, i, makecol(r1, g, b1));
			// blue flag
			b2 = min(b2 + step, 255);
			circlefill(flagpos_buf[1], radius, radius, i, makecol(r2, g, b2));
		}
		flagpos_ready = true;
	}
}

// gameclient instance
gameclient_c *gameclient;

int cfunc_connection_update(client_runes_t *arg) {

	int count;
	char lebuf[256];

	switch (arg->connect_result) {
	case 0:
		LOG("client connected.\n");
		gameclient->client_connected(arg->data, arg->length);
		break;
	case 1:
		LOG("client disconnected.\n");
		gameclient->client_disconnected();
		break;
	case 2:
		LOG("cannot connect, server denied (full?)\n");
		gameclient->connect_failed_denied(arg->data, arg->length);
		break;
	case 3:
		LOG("cannot connect, server not responding\n");
		gameclient->connect_failed_unreachable();
		break;
	case 4:
		LOG("cannot connect, net-server is full!");
		count = 0;
		writeString(lebuf, count, "Server is full.");
		gameclient->connect_failed_denied(lebuf, count);
		break;
	default:
		LOG1("WARNING: client connection update unknown code = %i\n", arg->connect_result);
		break;
	}
	return 0;
}

int cfunc_server_data(client_runes_t *arg) {

	//LOG1("client data=%i\n", arg->length);

	gameclient->process_incoming_data(arg->data, arg->length);

	return 0;
}

//============================================================
//  client "download file from server" thread
//============================================================

void *thread_clientdownloader_f(void *arg) {

	gameclient->client_download_thread(arg);

	return 0;
}

//============================================================
//  client make login (user,password) to get a token
//============================================================

//client password-to-token retrieval thread
void *thread_clientpassword_f(void *arg) {

	gameclient->client_password_thread(arg);

	return 0;
}

//************************************************************
//  mainloop
//************************************************************

/*
bool reset_video_mode() {

	// free backbuffer
	//
	if (backbuf) {
		destroy_bitmap(backbuf);
		backbuf = 0;
		LOG("BACKBUFFER DESTROYED\n");
	}

	// hicolor
	//
	set_color_depth(16);

	// set mode
	//
	set_gfx_mode(WINMODE, RESOL_X, RESOL_Y, 0, 0);

	//transparent text
	//
	text_mode(-1);

	//create back buffer
	//
	backbuf = create_bitmap(SCREEN_W, SCREEN_H);

	//error?
	//
	if (!backbuf) {
		LOG("ERROR: cannot create memory backbuffer!\n");
		return false; // FATAL
	}

	// ok
	//
	setcolors();
	drawbuf = backbuf;
	page_flipping = false;
	return true;
}
*/

bool reset_video_mode() {

		char err1[1024];
		char err2[1024];
		char err3[1024];
		char err4[1024];

		//un-show any video bitmaps?
		//show_video_bitmap(screen);

		//destroy all old surfaces
		if (vidpage1) { LOG("destroying vidpage1\n"); destroy_bitmap(vidpage1); vidpage1 = 0; }
		if (vidpage2) { LOG("destroying vidpage2\n"); destroy_bitmap(vidpage2); vidpage2 = 0; }
		if (backbuf) { LOG("destroying backbuf\n"); destroy_bitmap(backbuf); backbuf = 0; }

		int notok;

		set_color_depth(16); // hicolor
		if (winclient) // set mode
			notok = set_gfx_mode(WINMODE, RESOL_X, RESOL_Y, 0, 0);
		else
			notok = set_gfx_mode(FULLMODE, RESOL_X, RESOL_Y, 0, 0);

// ***** INICIO *******

		if (notok < 0) {
			LOG1("ERROR: cannot set 640x480x16 windowed?=%i graphics mode!\n", winclient);
			LOG1("Allegro error: '%s'\n", allegro_error);
			strcpy(err1, allegro_error);

			//try again...
			winclient = !winclient;
			set_color_depth(16); // hicolor
			if (winclient) // set mode
				notok = set_gfx_mode(WINMODE, RESOL_X, RESOL_Y, 0, 0);
			else
				notok = set_gfx_mode(FULLMODE, RESOL_X, RESOL_Y, 0, 0);

			if (notok < 0) {
				LOG1("ERROR: cannot set 640x480x16 windowed?=%i graphics mode!\n", winclient);
				LOG1("Allegro error: '%s'\n", allegro_error);
				strcpy(err2, allegro_error);

				//try again...
				winclient = !winclient;
				set_color_depth(15); // ===> different color depth
				if (winclient) // set mode
					notok = set_gfx_mode(WINMODE, RESOL_X, RESOL_Y, 0, 0);
				else
					notok = set_gfx_mode(FULLMODE, RESOL_X, RESOL_Y, 0, 0);

				if (notok < 0) {

					LOG1("ERROR: cannot set 640x480x15 windowed?=%i graphics mode!\n", winclient);
					LOG1("Allegro error: '%s'\n", allegro_error);
					strcpy(err3, allegro_error);

					//AND try again.....
					winclient = !winclient;
					set_color_depth(15); // ===> different color depth
					if (winclient) // set mode
						notok = set_gfx_mode(WINMODE, RESOL_X, RESOL_Y, 0, 0);
					else
						notok = set_gfx_mode(FULLMODE, RESOL_X, RESOL_Y, 0, 0);

					if (notok < 0) {

						LOG1("ERROR: cannot set 640x480x15 windowed?=%i graphics mode!\n", winclient);
						LOG1("Allegro error: '%s'\n", allegro_error);
						strcpy(err4, allegro_error);

						char elmsg[4096];
						sprintf(elmsg, "ERROR: cannot initialize graphics! reasons:\n1 = '%s'\n2 = '%s'\n3 = '%s'\n4 = '%s'", err1, err2, err3, err4);
						allegro_message(elmsg);
						return false;	// FATAL error
					}

				}
			}
		}

// ***** FIM *******

		text_mode(-1);	//transparent text

#ifndef SWITCH_PAUSE_CLIENT
		if (set_display_switch_mode(SWITCH_BACKAMNESIA) == -1) {
			if (set_display_switch_mode(SWITCH_BACKGROUND) == -1) // allow running in the background
			{
				LOG("ERROR: client cannot run in the background!\n");
				return false; // FATAL
			}
			else LOG("switch_background set ok.");
		}
		else LOG("switch_BACKAMNESIA set ok.");
#endif

		//attempt to create two video bitmaps, else use RAM backbuffer
		if (trypageflip) {
			vidpage1 = create_video_bitmap(SCREEN_W, SCREEN_H);
			vidpage2 = create_video_bitmap(SCREEN_W, SCREEN_H);
			LOG2("create vidpage1=%p vidpage2=%p\n", vidpage1, vidpage2);
		}

		//delete any partial video pages
		if ((!vidpage1) || (!vidpage2)) {

			if (trypageflip)
				LOG("PAGE FLIPPING: not enought video memory. using bruteforce doublebuffering\n")
			else
				LOG("USING SAFE MODE VIDEO -- DOUBLE-BUFFERING (option -dbuf). for page flipping use -flip\n")

			if (vidpage1) { destroy_bitmap(vidpage1); vidpage1 = 0; LOG("destroyed vidpage1\n"); }
			if (vidpage2) { destroy_bitmap(vidpage2); vidpage2 = 0; LOG("destroyed vidpage2\n");}

			//create RAM backbuffer
			backbuf = create_bitmap(SCREEN_W, SCREEN_H);
			if (!backbuf) {
				LOG("ERROR: cannot create memory backbuffer!\n");
				return false; // FATAL
			}
			drawbuf = backbuf;
			page_flipping = false;
		}
		else {
			drawbuf = vidpage1;
			page_flipping = true;
		}
		setcolors();

		return true; //ok
}

// this simple task is turning into a major headache...
//
bool set_shitty_mode() {

	//V0.5.0 : -text  flag
	if (textserver) return true;

	int DTC = desktop_color_depth();

	set_color_depth( DTC );

	if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 240, 0, 0))
		LOG1("ERROR: could not set gfx mode 320x240 windowed.. try 1 with %i", DTC)
	else
		return true;	// OK

	if ((DTC == 16) || (DTC == 15)) {

		if (DTC == 15)
			DTC = 16;
		else
			DTC = 15;

		set_color_depth( DTC );

		if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 240, 0, 0))
			LOG1("ERROR: could not set gfx mode 320x240 windowed.. try 2 with %i", DTC)
		else
			return true;	// OK
	}

	// the last hope. WARNING: this can be buggy for multiple dedicated servers.
	//
	DTC = 8;

	set_color_depth( DTC );

	if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 240, 0, 0)) {
		LOG1("ERROR: could not set gfx mode 320x240 windowed.. tried with %i", DTC);
		return false;	//GIVE UP
	}
	else
		return true;	// OK AT LAST!!!
}

int main(int argc, char *argv[]) {


	//random random
	srand((unsigned)time(0));

	strcpy(teamname[0], "RED");
	strcpy(teamname[1], "BLUE");

	// general init
	//
	gameclient = 0;
	gameserver = 0;

	//LOG_OPEN("outgun.log");

	// Set the text encoding format for Allegro as 8 bit Ascii
	set_uformat(U_ASCII);

	allegro_init();
	install_keyboard();
	install_timer();

	// find out where we are
	//
	char *exespec = new char[2048];
	get_executable_name(exespec, 2048);
	replace_filename((char*)wheregamedir, (const char *)exespec, "", 256); //Replaces the specified path+filename with a new filename tail, storing at most size bytes into the dest buffer. Returns a copy of the dest parameter
	delete exespec;
	exespec = 0;

	char lognamebuf[1024];

	//open outgun.log at EXE root path
	append_filename(lognamebuf, wheregamedir, "outgun.log", WHERE_PATH_SIZE);
	LOG_OPEN(lognamebuf);

	LOG3("OUTGUN LOG FILE. STRING %s PROTOCOL %s VERSION %s", GAME_STRING, GAME_PROTOCOL, GAME_VERSION);

	if (nlInit() == NL_FALSE) {
		LOG("ERROR: cannot init HawkNL!\n");
		return 0;
	}
	if (nlSelectNetwork(NL_IP) == NL_FALSE) {
		LOG("ERROR: no IP network!\n");
		return 0;
	}

	// enable statistics
	//
	nlEnable(NL_SOCKET_STATS);

	// check args
	//
	for (int i=1;i<argc;i++) {
		if (!strcmp(argv[i], "-ded"))
			dedserver = true;
		else if (!strcmp(argv[i], "-text"))		//v0.5.0 UNIX TEXTMODE SERVER
			textserver = true;
		else if (!strcmp(argv[i], "-priv"))
			privateserver = true;
		else if (!strcmp(argv[i], "-info"))
			showinfo = true;
		else if (!strcmp(argv[i], "-defaultprio"))
			defaultprio = true;
		else if (!strcmp(argv[i], "-prio")) {
			if (++i<argc) {
				targetprio = strtol(argv[i], NULL, 10);
			}
		}
		else if (!strcmp(argv[i], "-win"))
			winclient = true;
		else if (!strcmp(argv[i], "-flip"))
			trypageflip = true;
		else if (!strcmp(argv[i], "-dbuf"))
			trypageflip = false;
		else if (!strcmp(argv[i], "-fs"))
			winclient = false;
		else if (!strcmp(argv[i], "-fps")) {
			if (++i<argc) {
				targetfps = strtol(argv[i], NULL, 10);
				if (targetfps < 1)
					targetfps = 1;
				if (targetfps > 1000)
					targetfps = 1000;
			}
		}
		else if (!strcmp(argv[i], "-maxp")) {
			if (++i<argc) {
				server_maxplayers = strtol(argv[i], NULL, 10);
				if (server_maxplayers % 2 == 1)	//ímpar: des-impariza
					server_maxplayers++;
				if (server_maxplayers < 2)
					server_maxplayers = 2;
				if (server_maxplayers > MAX_PLAYERS)
					server_maxplayers = MAX_PLAYERS;
			}
		}
		else if (!strcmp(argv[i], "-port")) {
			if (++i<argc) {
				port = strtol(argv[i], NULL, 10);
			}
		}
		else if (!strcmp(argv[i], "-nosound"))
			nosound = true;
		else if (!strcmp(argv[i], "-notcp"))
			no_tcp_download = true;
		else if (!strcmp(argv[i], "-yestcp"))
			no_tcp_download = false;
		else if (!strcmp(argv[i], "-ip")) {
			if (++i<argc) {
				force_ip = true;			//force IP
				strcpy(force_ip_name, argv[i]);	//to next parameter value
			}
		}
		else
			LOG2("WARNING: command-line argument #%i is unknown ('%s')\n", i, argv[i]);
	}

	// resolve master server address
	//
	LOG("resolving master server address...\n");
	try {
		nlGetAddrFromName("www.mycgiserver.com", &master_address);	//www.mycgiserver.com
	} catch (...) {
		LOG("caugth exception probably on nlGetAddrFromNameAsync()\n");
		master_address.valid = NL_FALSE;
	}

	if (master_address.valid == NL_FALSE) {
		LOG("can't resolve master server address to IP.\n");
		nlStringToAddr("212.69.162.53", &master_address);					//last known resolution for www.mycgiserver.com
	} else if (master_address.valid == NL_TRUE) {
		LOG("address resolved sucessfully.\n");
	}

	nlSetAddrPort(&master_address, 80);													//port 80
	LOG("master server address set.\n");

	// here must get the safest and shittiest windowed gfx mode available
	//
	if (set_shitty_mode() == false)
		return 0;		//if this ever executes then the world is a sucky sucky place.

/*
	// holding left shift on boot = switch to dedicated server
	// holding right control on boot = toggle winclient flag
	MS_SLEEP(500);
	if (key[KEY_LSHIFT])
		dedserver = true;
	if (key[KEY_LCONTROL])
		winclient = !winclient;
	if (key[KEY_ALT])
		trypageflip = !trypageflip;
*/

	// install higher-accuracy timer interrupt
	//
	LOCK_VARIABLE(speed_counter);
	LOCK_FUNCTION(increment_time_counter);
	install_int_ex(increment_time_counter, BPS_TO_TIMER(200));		//5 ms accuracy is already 10 times better than clock()

	// install server timer (used for both dedicated and listen server)
	//
	LOCK_VARIABLE(server_speed_counter);
	LOCK_FUNCTION(increment_server_speed_counter);
	install_int_ex(increment_server_speed_counter, BPS_TO_TIMER(10));		//10Hz

	// get system thread priorities
	//
	int						pmin = sched_get_priority_min(SCHED_OTHER);
	int						pmax = sched_get_priority_max(SCHED_OTHER);
	int						policy;
	pthread_t			tme = pthread_self();
	sched_param		param;
	int						rc = pthread_getschedparam(tme, &policy, &param); // get priority of current thread (wich is the default value)
	int						pdef = param.sched_priority;
	LOG("\nThread priorities:");
	LOG4("   rc = %i policy = %i OTHER=%i sched_prio = %i\n", rc, policy, SCHED_OTHER, param.sched_priority);
	LOG3("   pmin %i pmax %i pdef = %i\n", pmin, pmax, pdef);

	//show info
	if (showinfo) {

		//get all local addresses
		NLaddress *locals;
		NLint     locsize;

		locals = nlGetAllLocalAddr(&locsize);

		char infobuf[2048];
		sprintf(infobuf, "Information:\n\nThread priorities for -prio <val> parameter:\n* Minimum -prio <val> : %i\n* Maximum -prio <val> : %i\n* System default (use -defaultprio) : %i\n\nLocal addresses:\n", pmin, pmax, pdef);

		for (int z=0;z<locsize;z++) {
			char adrs[222];
			nlAddrToString( &(locals[z]), adrs );
			strcat(infobuf, adrs);
			strcat(infobuf, "\n");
		}

		allegro_message(infobuf);
		return 0;
	}

	// run server
	//
	if (dedserver) {

		// here must get the safest and shittiest windowed gfx mode available
		//
		if (set_shitty_mode() == false)
			return 0;		//if this ever executes then the world is a sucky sucky place.

		// dedicated server - set process priority (all threads) to a higher value
		//		--> threads filhas estao com as priorities certas? LOGAR pra  ver. senao mudar p/ INHERIT
		int ptarg;

		//change default priority?
		if (!defaultprio) {

			//if -prio parameter is unspecified
			if (targetprio == TARGET_PRIO_UNSPECIFIED) {

				//guess one below system maximum (wich usually means realtime and should never be used really)
				//
				if (pmin < pmax) { // pmin ..... OI! . PMAX
					ptarg = pmax - 1;
				}
				else {             // PMAX . OI! ..... pmin
					ptarg = pmax + 1;
				}

			}
			else
				ptarg = targetprio;

			param.sched_priority = ptarg;
			policy = SCHED_OTHER;
			pthread_setschedparam(tme, policy, &param);
			LOG1("\nNEW PRIORITY VALUE SET FOR DEDICATED SERVER: %i\n", ptarg);
		}
		else
			LOG("\n-defaultprio: Leaving thread priorities on their default values");

		// log
		LOG_CLOSE();

		//open log at EXE root path
		append_filename(lognamebuf, wheregamedir, "out_svr.log", WHERE_PATH_SIZE);
		LOG_OPEN(lognamebuf);

		// gfx init
		//
		if (set_display_switch_mode(SWITCH_BACKAMNESIA) == -1) // allow running in the background
		{
			LOG("can't set SWITCH_BACKAMNESIA for SERVER");
			if (set_display_switch_mode(SWITCH_BACKGROUND) == -1) {
				LOG("ERROR: server window cannot run in the background!\n");
				return 0;
			}
			else
				LOG("set SWITCH_BACKGROUND for SERVER");
		}
		else
			LOG("set SWITCH_BACKAMNESIA for SERVER");
		setcolors();

		// run server
		//
		gameserver = new gameserver_c();
		if (!gameserver->start(server_maxplayers)) {
			LOG("ERROR: cannot start gameserver!\n");
			return 0;
		}
		gameserver->loop(0);
		gameserver->stop();
		delete gameserver;
		gameserver = 0;
	}
	// run client
	//
	else {

		//require CLIENT_MAPS_DIR directory
		char mappath[256];
		strcpy(mappath, CLIENT_MAPS_DIR);  // cmaps
		char dest[1024];
		append_filename(dest, wheregamedir, mappath, WHERE_PATH_SIZE);	// <FULL-DIR>/maps/*.txt, I hope
		LOG1("CMAPS DIR IS = '%s'\n", dest);
		al_ffblk mapffblk;	//for al_find*
		int result = al_findfirst(dest, &mapffblk, FA_DIREC | FA_ARCH);
		if (result != 0) {
			allegro_message("Error: directory '%s' not found\n\nPlease create this directory.\n\nThe game cannot run without it.", dest);
			return 0;
		}

		//window title
		server_status_string("Outgun client - CTRL+F12 to quit");

		// log
		LOG_CLOSE();

		//open log at EXE root path
		append_filename(lognamebuf, wheregamedir, "out_cli.log", WHERE_PATH_SIZE);
		LOG_OPEN(lognamebuf);

		// try install sound
		//
		if (!nosound) {

			if (install_sound(DIGI_AUTODETECT, MIDI_NONE, 0)) {
			//if (install_sound(DIGI_WAVOUTID(0), MIDI_NONE, 0)) {

				LOG("INSTALL_SOUND failed. no sound.\n");
				sound_inited = false;
			}	else {
				LOG("INSTALL_SOUND ok.\n");
				sound_inited = true;
			}
		}
		else
			LOG("SOUND DISABLED by command line option -nosound\n");

		// gfx init
		//
		if (!reset_video_mode())		// fatal error
			return 0;

		// install client timer
		//
		LOCK_VARIABLE(speed_counter);
		LOCK_FUNCTION(increment_speed_counter);
		install_int_ex(increment_speed_counter, BPS_TO_TIMER(targetfps));		//client MAX FPS

		// run client
		//
		gameclient = new gameclient_c();
		if (!gameclient->start()) {
			LOG("ERROR: cannot start gameclient!\n");
			return 0;
		}
		gameclient->loop();

		// disconnect client
		//
		gameclient->stop();
		delete gameclient;
		gameclient = 0;

		// stop listenserver if it was running
		//
		listen_stop();
	}

	// exit HawkNL
	nlShutdown();

	// log close
	LOG_CLOSE();

	return 0;
} END_OF_MAIN();
