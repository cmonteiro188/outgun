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

-------------------------------------------------------------------------

	hardcoded stuff:
		CTF: the teams max size is 8, wich implies maxplayers <= 16

-------------------------------------------------------------------------

OK - frags update on the scoreboard
OK - fazer varias salas conectadas, mudando entre elas conforme passa de 1 p/ outra
OK - alocacao de times na conexao
OK - adicionar minimap
OK - adicionar paredes
OK - adicionar player list, fazer a HUD ficar como deveria
OK - send direction to clients (3bit keys)
OK - draw direction (gun)
OK - ALT=strafe
OK - send CTRL=fire!
OK - add the flags for captures (base info on map)
OK - (re)spawn player on teamspawn points
OK - criar os sprites nas 5 direcoes (3 vem via H-mirror)
OK - adicionar ao serverframe a informacao de qual das 8 direcoes (3bits-keys) um jogador
     esta mirando (por causa do strafe) a animacao da frame especifica eh clientside
OK - make fire cmd create a ball simulated on serverside/clientside
OK - tiro hit: dec health, freeze targ's gun, kill ....
OK - show flags on the map if dropped (not carried)
OK - fix drop flag when killed as vezes some bug
OK - nomes das pessoas envolvidas nos eventos CTF
OK - X gibbed Y
OK - shift = run gasta life
OK - flag carrier movimentacao pior (menos maxspeed)
OK - build a sprite for the minimap background (map walls + grid + borda verde + fundo cinza!)
OK ==> MINIMAP / POS UPDATE
	1o: pos update so pra quem ta na mesmissima sala
	2o: minimap mostra quem vai num minimap-show
	3o: quem eh mostrado no minimap:
			- todos os caras do meu time, sempre (aha nao precisa do short pra isso!!)
			- caras do outro time que estao na mesma tela que algum dos do meu time
OK - mostrar flags do minimap:
	OK - qualquer flag parada na sua base
	OK - qualquer flag caida no chao
	OK - flag inimiga carregada
	OK - flag amiga carregada SE o carrier inimigo esta visivel
OK - pseudo-FOG OF WAR: pintar de cinza as salas que nao tem nenhum amigo (nao se sabe o que ocorre la)
OK - MAPA DECENTE
OK - SPAWNPOINTS EM TODO O MAPA
OK - implementar ping broadcast: envia pcts para todos os players, 1 por frame. player
	OK - precisa responder 1 so imediatamente. sempre que eh bem sucedido, o ping do
		   jogador eh atualizado
	OK - segura TAB: exibe PING dos jogadores ao inves do score
OK - delay da morte, mostrar corpo, respawn-time, nao atira ou move se health=0
OK - PARTIDA-MANAGEMENT:
	OK - team scoring (num captures)
	OK - quando captures de um time == 8, reinicia partida
	OK - quando um player conecta e fica == 2, reinicia partida
	OK - reinicio de partida esta ok
OK - suicide command = DEL
OK - cliente mostra icone de desconexao se ultimo packet recebido ultrapassa os 500ms
OK - com flag nao pesa, apenas a corrida eh menos eficiente (igual agora ta ok)
OK - mudar cor do texto p/ ficar LEGIVEL
OK - checagem de versão da l33tnet, client ignorado
OK - home=cor CTRL+HOME=cor original
OK - nomes aleatorios! inicio e INSERT key
OK - checagem de versao do game (outgun)
OK - defends the carrier against enemy: mata alguem com teu carrier na tela  X DEFENDS THE FLAG CARRIER  
OK - defends the flag: mata alguem com tua flag na tela  X DEFENDS THE FLAG
OK - podia fazer barrinha de sprint p/ correr em vez de tirar vida... barrinha de "sprint" - tira health so quando zera
OK - net medidor bandwidth do cliente
OK - +ATTACK e -ATTACK para tiro continuo e menos fuzui
OK - tiro nao tira health: gasta energy da sprint bar!
OK - SAY TEAM:   ponto"." antes da mensagem
OK - COLORFUL TEXT: TEAM / ALL / INFO
OK - BARRA COLORIDA DE LIFE
OK - BUG: segura muito shift mata! -- minimo 30 health pra descontar do run
OK - paralisia do tiro diminuida p/ 1 segundo (de 1.5)
OK - mensagem tiro destroyed: vai ID do player atingido (BYTE) 255==nenhum
OK - player pushed by shots
OK - o server simula fisica muito grosseiramente, resultado eh que o ROCKET DELETE se adianta:
   o server simula 10x 1/10 frames pro rocket nao "escapar" das pessoas, e a msg que ele manda
	 ignora esses moves intermediarios. o cliente pode simular o tiro ate que ele colida visualmente
	 com o alvo, ou ate que 100ms de simulacao desde o comando de delecao do tiro passem
	 SOLUCAO BOA!!! UHUUU
	 OK ===> INCLUSIVE ESPERAR ESSA COLISAO OU 100MS pra PISCAR O INIMIGO HIT !!!
OK - BUG: DEFENDS FLAG bonus consertado
OK - FPS COUNTER!!
OK - COR TEXTO MAIS CLARA dos times na sbar
OK - pontinho gigante do jogador no minimap piscante
OK - ordenar scoreboard antes de desenhar. sempre que chega uma fragupdate message
OK - desconta 1 frag por suicidio (DEL)
OK - END: toggle WANT-CHANGE-TEAMS flag. quando dois do time adversario tao com a flag ligada,
		 eles trocam de posicao nos times (player-ids)
OK - VIROU SORVETE 10% de chance
OK - fx de explosao spawn - puramente clientside
OK - SOM / EVENTOS SONOROS!
		GERAL:		- atira		- tiro acerta alguem		- alguem morre		- entered game		- left game
		CTF:		- lost flag		- got flag		- returned flag		- captured flag		- game over victry
OK - opcao desligar som cmd line (-nosound)  
OK - opcao toggle fullscreen/windowed	 
OK - contador de players conectados no window title do servidor dedicado
OK - tela de conexao:	 N slots de IPs / servername / #players / #maxplayers
	 seta cima/baixo move edicao de IP	 ENTER: connect	 SPACE: refresh all IPs digitados
OK - protocolo de refresh:
		cliente envia 0 / 200  (refresh request) /// servidor responde 0 / 200 (refresh response)   STRING(p/ display na tela!)
OK - salvar config
   OK- salvar o nome (e depois outras preferências) do jogo entre sessoes
   OK- salvar tela conexao arquivo connect.txt
OK - servidor especifica HOSTNAME aparece na tela de connect
OK - powerups + SHIELD powerup
OK - BOTAS powerup (pwup tempo)
OK - ELMO powerup
OK - LUVA/ARMA powerup (30s, instant kill, mesmo com escudo)
OK - dar ICONES ANIMADOS para os powerups
OK - save screenshots - F11
OK - timer dos powerups na sbar junto com os:
OK - icones dos powerups na sbar (nao eh bem "icones" mas tudo bem)
OK - tela do canto inferior direito esta com as paredes estranhas. antes estava bom
 		 como sera que ficou lixao assim!?!??!?! (r.: escrevendo lixo em player[-1])
OK - CONSERTAR PAGEFLIP !!!!!!!!!!!!!!!!!OK!!!!!!!!!!!!!!!!!!! (era mutex)
OK - mensagem connection refused explica quando versao do server eh mais antiga ou mais nova
OK - consertado bug dava pau quando quit: reader thread do client_ci nao dava quit quando
	   nao era mais necessaria, ao receber mensagens tipo "connection refused"
OK - as paredes estao podres - consertar -- 
OK - BOUNCY WALLS!!!!!!1
OK - consertada piscação da tela que as pessoas tavam reclamando (erro no reset_video_mode())
OK - consertados varios bugs que nao deixavam o servidor rodar direito depois que eu
		 mudei o esquema de nao usar mais "new" pra criar stations
OK - consertado bug que se manifestava no release: player unused sendo passado para rotina
	   wallcorrect no calc_game_frame do gameclient

versao 0.1.2
========================
OK - consertado nomes 16 chars agora e 15 mesmo
OK - powerups nao mostram negativo no sbar contador
OK - top-speeds (velocidades maximas) aumentadas no geral, mas jogador ainda desliza igual
OK - powerup SPEED: run gasta energia e saude normalmente
OK - velocidade maxima nao trunca velocidade se passa (decresce suavemente com friccao do solo)
     com isso tb tive que arrumar o calculo do recoil de se tomar um tiro
OK - energia acumula ate 200, mas entre 100 e 200 a regeneracao eh menor que a velocidade normal
OK - consertado bug em que a mensagem de "protocol mismatch" trocava a ordem das versoes
OK - powerup WEAPON UP: arma incrementa tiros (aumenta quantidade, gasta + energy...)
OK - powerup shield soma 100 de energy, podendo ir a 200
OK - consertado bug que fazia procurar sons no diretorio raiz (acho q consertei...)
OK - SBAR com novas informacoes: hostname do server conectado. e mostra IP no menu
OK - consertado salva/carrega hostname.txt e clconfig.txt do mesmo dir. do EXE do jogo
OK - powerups mudam para quem vê, para serem mais interessantes
OK - help online: F1 toggle (fullscreen), desliza p/ dentro da tela, F1 de novo arrasta para fora

versao 0.2.0
========================
??? - acho que nao trava mais o video no modo pageflip!! ?? ALELUIA???


versao 0.2.1 (atualizacao client e server, mas opcional para ambos)
========================
OK - F10 para trocar de nome
OK - modo "safe" eh o default agora.
OK - atalho "run with fast graphics (may hang your computer, especially windows 98)"
OK - bug do tiro explosion que nao aparece as vezes
 

versao 0.2.2 (atualizacao client e server, mas opcional para ambos)
========================

OK - sfx theme selector (junto com sons do Vita-Valtteri Pimiä)
OK - QUAD DAMAGE: dano quadruplo, nada a mais

	
versao 0.2.3 (atualizacao client apenas)
========================
OK - problem with 16 bit color depth vs. 15 bit color depth solved
OK - BURRO! lancou o jogo com DEBUG POWERUPS !!! ARGHH!!!


versao 0.3.0  (futura)
========================
OK - bonus de friction e acceleration se player com boots OU se esta correndo (ou ambos)
OK - fazer com que o servidor negocia com o cliente a velocidade do jogo, assim nao
     precisa lancar uma nova versao sempre que eu fizer "tweak" disso (ver se tem outros
	   parametros para mexer, como os frictions tambem.. parametrizar toda a fisica)
 - megahealth powerup: +100 health e +100 energy, ambos acumulaveis ate 300. se ja
   estiver em 300 com um, passa pro outro
 - novo esquema de quantities de powerup (configuravel)
  
	 
coisas p/ o futuro
========================
 - bots, com as pessoas dizendo q tamanho de times minimo querem (de 0 a 8), pega a media
   e buracos p/ atingir a media sao preenchidos com bots
 - mais built-in maps
 - vote-exit / vote-warp
 - configuracoes (parametros) do jogo (mais powerups... etc.)
 - adicionar suporte para joystick : detectar e sair usando (ver allegro)
 - esquema de master server para:
   - servidores se cadastrarem automaticamente sem precisar admin mandar msg pra mim
	 - jogadores obterem a lista de servidores sem ter que visitar a pagina
	 - nao preciso mais atualizar a pagina com lista de servidores (pelo menos nao "a mao")

 
*/


//same as PLAYER RADIUS (15) + ROCKET RADIUS (3)
#define		SHOT_DELTAX	 18

//#define		OLD_DIRECTIONALS

//#define   SWITCH_PAUSE_CLIENT

//#define		ALWAYS_FRICTION

#define PI 3.1416

#define PIOIT 0.7854 //DOIS PI SOBRE 8 = PI SOBRE 4 = 0.7854    0.3927  //PI/8

//quick debugs
//#define   MIN_ALPHA_FRIENDS  1			//debug value
#define   MIN_ALPHA_FRIENDS  64

#define   ROCKET_SPEED	50.0		//in pixels/0.1s

#define   NUMBER_OF_POWERUP_KINDS   6    //quad shield shadow turbo weapon-up megahealth

//#define  DEBUG_POWERUPS
//#define  REALLY_DEBUG_POWERUPS		//define only if DEBUG_POWERUPS defined

#ifdef DEBUG_POWERUPS
#define PICKUP_RESPAWN_TIME 1.0		//debug time
#else
#define PICKUP_RESPAWN_TIME 30.0		// 30 seconds to respawn a pickup
#endif


// GAME VERSION / GAME STRING
//
#define GAME_STRING "Outgun"
#define GAME_PROTOCOL "6"
#define GAME_VERSION "0.3.0"



#include "allegro.h"	// Allegro

//patching main / _main / WinMain link errors...
#ifdef ALLEGRO_WINDOWS
#include "winalleg.h"
#include "windows.h"
#endif

#include "nl.h"				// HawkNL

#include <leetnet/server.h>		// l33t server
#include <leetnet/client.h>		// l33t client
#include <leetnet/rudp.h>			// get_self_IP
#include <leetnet/sleep.h>		// sleep util

#include "string.h"
#include "math.h"

#include "pthread.h"
#include "sched.h"

#include <string>
using namespace std;
#include "names.h"		//the COOLEST random-name generator by Renato Hentschke

//log utils
//#define LOG_NOLOG		// uncomment to disable logging
#define LOG_EXPR game_log
#define LOG_TIMEFUNC get_time()
#include <leetnet/log.h>
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
#define RESOL_X  640
#define RESOL_Y  480

//play area offset
#define plx 0
#define ply 90

//play area width/height
#define plw 480
#define plh 354

//minimap offset
#define mmx (plx + plw)
#define mmy ply

//scoreboard offset
#define sbx (plx + plw)
#define sby (mmy + 120)			// + XXX = minimap panel height


//************************************************************
//  common stuff
//************************************************************

//root path (game executable path)
#define WHERE_PATH_SIZE 256
char wheregamedir[WHERE_PATH_SIZE];

//function that resets the video mode
bool reset_video_mode();

#define CTF_NUMBER_OF_CAPTURES 8

// server game phisics parameters
//double	sv_frictionx, sv_frictiony, sv_accelx, sv_accely, sv_maxspeedx, sv_maxspeedy;
//double	sv_maxspeedrunx, sv_maxspeedruny, sv_flag_penalty_x, sv_flag_penalty_y;
//double  sv_boots_top_bonus_x, sv_boots_top_bonus_y, sv_boots_accel_bonus, sv_run_accel_bonus;

// NEW server game phisics parameters
double  svp_fric, svp_accel, svp_maxspeed;
double  svp_fric_run, svp_accel_run, svp_maxspeed_run;
double  svp_fric_turbo, svp_accel_turbo, svp_maxspeed_turbo;
double  svp_fric_turborun, svp_accel_turborun, svp_maxspeed_turborun;
double  svp_flag_penalty;

// set the default physics parameters
void set_default_physics() {
	svp_fric     = 1.5;
	svp_accel    = 2.0;
	svp_maxspeed = 12.0;
	svp_fric_run     = svp_fric  * 1.25;
	svp_accel_run    = svp_accel * 1.25;
	svp_maxspeed_run = 20.0;
	svp_fric_turbo     = svp_fric     * 1.5;
	svp_accel_turbo    = svp_accel    * 1.5;
	svp_maxspeed_turbo = 24.0;
	svp_fric_turborun     = svp_fric  * 1.75;
	svp_accel_turborun    = svp_accel * 1.75;
	svp_maxspeed_turborun = 32.0;
	svp_flag_penalty	= 3.0;
}


#define MAX_PLAYERS 16		// maximum number of players

#define MAX_ROCKETS 256		// maximum number of rockets (nao pode ser mais que 256 pq eh usado um unsigned char p/ passar ids)

#define MAX_PICKUPS MAX_PLAYERS	// (maximum) number of pickups laying on the ground at one time in the game

#define MAX_TEAM_SPAWNS 8		// maximum different team spawn points

//arg switches (+ default values)
bool dedserver = false;		//dedicated server?  -ded
bool winclient = true;		//windowed client?	 -win / -fs
bool trypageflip = false;	//try page flipping? -flip / -dbuf
bool nosound = false;			//disable sound?    -nosound
int	 targetfps = 60;			//target (MAX) frames-per-second 
int	 port = 25000;				//the server port

bool sound_inited;		  //install_sound succeeded?
bool sound_enabled = true;		// player wants sounds?


//audio samples : codes
enum {

	SAMPLE_FIRE,			//ok
	SAMPLE_HIT,				//ok
	SAMPLE_DEATH,			//ok
	SAMPLE_DEATH_2,		//ok
	SAMPLE_ENTERGAME,	//ok
	SAMPLE_LEFTGAME,	//ok
	SAMPLE_CHANGETEAM,
	SAMPLE_TALK,
	SAMPLE_WALLBOUNCE,

	SAMPLE_WEAPON_UP,	// new! v0.1.2

	SAMPLE_MEGAHEALTH, // new! v0.3.0

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

	//team colors
	COLRED,			//team 1 (color 0)
	COLBLUE,		//team 2 (color 1)

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

char teamname[2][5];

int	col[NUM_OF_COL];
void setcolors() {

	//the first 8 colors are player's colors
	col[COLGREEN] = makecol(0,0xff,0);
	col[COLYELLOW] = makecol(0xff,0xff,0);
	col[COLWHITE] = makecol(0xff,0xff,0xff);
	col[COLMAG]	= makecol(0xff, 0, 0xff);
	col[COLCYAN]	= makecol(0, 0xff, 0xff);
	col[COLORA]	= makecol(0xff, 0xb0, 0);
	col[COLLRED] = makecol(0xff,0x55,0x44);
	col[COLLBLUE] = makecol(0x44,0x55,0xff);
	
	// team solid colors 
	col[COLBLUE] = makecol(0,0,0xff);		
	col[COLRED] = makecol(0xff,0,0);

	//other
	col[COLFOGOFWAR] = makecol(0xff, 0xff, 0xff);
	col[COLMENUWHITE] = makecol(0xc0,0xc0,0xc0);
	col[COLMENUGRAY]  = makecol(0x68,0x68,0x68);
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
}

//server record
struct gamespy_t {

	NLaddress		addr;

	char				address[128];	//IP/address typein buffer

	bool				invalid;		//address is invalid

	bool				noresponse;		//no response?

	bool				refreshed;		//if data below is valid -------------

	char				info[128];		//server info string

};

//the world is a x,y indexed 2d array of rooms
#define WXMAX 6 		//maximum world size
#define WYMAX 6 
#define WALLMAX 16	//maximum number of walls

class wall_c { public:

	//rectangle coords (a,b)->(c,d)  a==-1 unused
	int a, b, c, d;

	//unused wall
	wall_c() { a = -1; }
};

class room_c { public:

	//a collection of walls (rectangles)
	wall_c		wall[WALLMAX];
};

//entity locale
struct spoint_t {
	
	//screen (if px == -1, unused)
	int px,py;
	//relative (to screen) X,Y position
	int x,y;
};

//team info
struct teaminfo_t {

	//flag position
	spoint_t		flag;

	//team spawn points
	spoint_t		spawn[MAX_TEAM_SPAWNS];

	//last team spawn point used
	int			lastspawn;
};

class map_c { public:

	//array of rooms
	room_c	room[WXMAX][WYMAX];

	//team information for red=0 and blue=1 teams
	teaminfo_t	tinfo[2];	

	//FIXME: other level information (item/powerup spawnpoints etc.)
};

//wall hit?
bool wallhit(double x, double y, wall_c &w) {

	if (((int)x) >= (w.a))
	if (((int)x) <= (w.c))
	if (((int)y) >= (w.b)) 
	if (((int)y) <= (w.d))
		return true;

	return false;
}

//wall collision correction
bool wallcorrect(int p, map_c* map, double *x, double *y, double *sx, double *sy, double *ox, double *oy, int px, int py) {

	if (px < 0) return false;
	if (py < 0) return false;
	if (px >= WXMAX) return false;
	if (py >= WYMAX) return false;

	//delta old to new  (ok)

	double tx,ty;
	tx = (*ox) - (*x);
	ty = (*oy) - (*y);

	//deltas for pushing out of walls: normalize
	double dx, dy;
	if (fabs(tx) > fabs(ty)) {
		dx = 2*tx / fabs(tx);  // ==1.0
		dy = 2*ty / fabs(tx);		// 0 <= val <= 1
	}
	else {
		dx = 2*tx / fabs(ty);	// 0 <= val <= 1
		dy = 2*ty / fabs(ty);	 // ==1.0
	}

	bool ever_had_wall_hit = false;
	bool had_wall_hit; //keep pushing out until no wall hit
	room_c *r = &(map->room[px][py]);

	int runaway = 10;
	do {

		had_wall_hit = false;
		bool y_solved = false;

		for (int w=0;w<WALLMAX;w++)
		if (r->wall[w].a != -1) {

			//check collision. if yes, take x,y out of the wall
			//
			int runaw = 100;
			while (wallhit((*x),(*y),r->wall[w])) {
			//if (wallhit((*x),(*y),r->wall[w])) {

				//at least one wall hit this turn
				had_wall_hit = true;

				//push x out of wall

				(*x) += dx;
				y_solved = false;

				if (!(wallhit((*x),(*y),r->wall[w])))
				break;

				//push y out of wall
				(*y) += dy;
				y_solved = true;

				runaw--;
				if (runaw < 0) {
					LOG1("RA (2) = %i\n", p);
					(*x) = (*ox);
					(*y) = (*oy);
					return false;	//FIXME:throw exception
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
			LOG1("RA = %i\n", p);
			//LOG("WARNING: WALCORRECT RUNAWAY !!!!!\n");
			(*x) = (*ox);
			(*y) = (*oy);
			return false;	//FIXME:throw exception
		}
		
	} while (had_wall_hit);

	if (ever_had_wall_hit) {
		
		//check for probable bug -- sometimes the wall-correct algorythm pushes the
		// players out of bounds. if this is the case, just back to the last valid position
		if ( 
			   ((*x) <= 0.01) || 
			   ((*x) >= ((double)plw) - 0.01) ||
			   ((*y) <= 0.01) || 
			   ((*y) >= ((double)plh) - 0.01)
			 )
		{
			// just back up
			(*x) = (*ox);
			(*y) = (*oy);
		}

		(*ox) = (*x);	//another safe position
		(*oy) = (*y);
	}

	return ever_had_wall_hit;
}

//draw a wall in a map
void drawwall(map_c *m, int x, int y, int a, int b, int c, int d) {
	
	for (int w=0;w<WALLMAX;w++)
	if (m->room[x][y].wall[w].a < 0) {

		m->room[x][y].wall[w].a = a;
		m->room[x][y].wall[w].b = b;
		m->room[x][y].wall[w].c = c;
		m->room[x][y].wall[w].d = d;

		//ok
		break;
	}
}

int COMPX = plw / 4;
int COMPY = plh / 4;
int LARGX = COMPX / 4;
int LARGY = COMPY / 4;

//closewalls
void closelwall(map_c *map, int x, int y) {
	drawwall(map, x, y, 0, 0, LARGX, plh);
}
void closerwall(map_c *map, int x, int y) {
	drawwall(map, x, y, plw-LARGX, 0, plw, plh);
}
void closeuwall(map_c *map, int x, int y) {
	drawwall(map, x, y, 0, 0, plw, LARGY);
}
void closedwall(map_c *map, int x, int y) {
	drawwall(map, x, y, 0, plh-LARGY, plw, plh);
}

//load default map (for testing)
void load_default_map(map_c *map) {

	int i;

	// BUILD WALLS
	//
	//int COMPX = plw / 4;		//globals
	//int COMPY = plh / 4;
	//int LARGX = COMPX / 4;
	//int LARGY = COMPY / 4;

	for (int x=0;x<WXMAX;x++)
	for (int y=0;y<WYMAX;y++)	{

		/*
		map->room[x][y].wall[0].a = 50 + x * 10;
		map->room[x][y].wall[0].b = 50 + y * 10;
		map->room[x][y].wall[0].c = 180 + x * 10; 
		map->room[x][y].wall[0].d = 180 + y * 10; 
		*/

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
	closelwall(map, 0, 0);
	closelwall(map, 0, 1);
	//closelwall(map, 0, 2);
	//closelwall(map, 0, 3);
	closelwall(map, 0, 4);
	closelwall(map, 0, 5);
	closelwall(map, 1, 1);
	closelwall(map, 1, 2);
	closelwall(map, 1, 3);
	closelwall(map, 1, 4);
	closelwall(map, 3, 1);
	closelwall(map, 3, 2);
	closelwall(map, 3, 3);
	closelwall(map, 3, 4);
	closelwall(map, 5, 1);
	closelwall(map, 5, 2);
	closelwall(map, 5, 3);
	closelwall(map, 5, 4);

	closerwall(map, 0, 1);
	closerwall(map, 0, 2);
	closerwall(map, 0, 3);
	closerwall(map, 0, 4);
	closerwall(map, 2, 1);
	closerwall(map, 2, 2);
	closerwall(map, 2, 3);
	closerwall(map, 2, 4);
	closerwall(map, 4, 1);
	closerwall(map, 4, 2);
	closerwall(map, 4, 3);
	closerwall(map, 4, 4);
	closerwall(map, 5, 0);
	closerwall(map, 5, 1);
	//closerwall(map, 5, 2);
	//closerwall(map, 5, 3);
	closerwall(map, 5, 4);
	closerwall(map, 5, 5);

	closeuwall(map, 0, 0);
	closeuwall(map, 1, 0);
	closeuwall(map, 2, 0);
	closeuwall(map, 3, 0);
	closeuwall(map, 4, 0);
	closeuwall(map, 5, 0);
	closeuwall(map, 3, 1);
	closeuwall(map, 2, 5);

	closedwall(map, 0, 5);
	closedwall(map, 1, 5);
	closedwall(map, 2, 5);
	closedwall(map, 3, 5);
	closedwall(map, 4, 5);
	closedwall(map, 5, 5);
	closedwall(map, 3, 0);
	closedwall(map, 2, 4);


	// BUILD RED TEAM INFO
	//
	map->tinfo[0].flag.px = 1;		// flag pos
	map->tinfo[0].flag.py = 1;
	map->tinfo[0].flag.x = plw / 2;
	map->tinfo[0].flag.y = plh / 2;
	map->tinfo[0].lastspawn = 0;		// lastspawn
	for (i=0;i<MAX_TEAM_SPAWNS;i++) {	// teamspawns
		map->tinfo[0].spawn[i].px = 1;
		map->tinfo[0].spawn[i].py = 1;
		map->tinfo[0].spawn[i].x = 80 + 30 * i;
		map->tinfo[0].spawn[i].y = 3 * plh / 4;
	}

	// BUILD BLUE TEAM INFO
	//
	map->tinfo[1].flag.px = WXMAX-2;		// flag pos
	map->tinfo[1].flag.py = WYMAX-2;
	map->tinfo[1].flag.x = plw / 2;
	map->tinfo[1].flag.y = plh / 2;
	map->tinfo[1].lastspawn = 0;		// lastspawn
	for (i=0;i<MAX_TEAM_SPAWNS;i++) {	// teamspawns
		map->tinfo[1].spawn[i].px = WXMAX-2;
		map->tinfo[1].spawn[i].py = WYMAX-2;
		map->tinfo[1].spawn[i].x = 80 + 30 * i;
		map->tinfo[1].spawn[i].y = plh / 4;
	}
}

//************************************************************
//  the game frame common stuff
//************************************************************

// a player's record
struct player_t {

	bool			used;		// player record valid?

	int				weapon;	// poder da arma atual (nivel) 0,1,2,3,4 (...?)

	bool			want_change_teams;		//server-side: player wants do change teams?

	double		speed_drop_time;		//speed powerup FX aux var
	double		wall_sound_time;		// min time to play sound again

	bool			onscreen;	//player onscreen? used only in clientside

	NLubyte		enemyvis;	//enemies being viewed . clientside only

	bool			item_shield;		// SHIELD: bit sent always: shield fx / shield present
	bool      item_quad;			// QUAD damage
	bool			item_speed;			// SPEED BOOTS
	int				item_helm;			// HELM 0== no   1+ == yes, alpha  (-1) 

	double		item_quad_time;		// time of expiring (to print on clientside screen)
	double		item_speed_time;
	double		item_helm_time;

	double		quad_sound_finished;	// to avoid too much quad sound
	
	double		hitfx;		// player-hit fx (relative to time): clientside only

	bool			attack;		// if player is holding attack button
	double		attacktime;	// time of last transition from attack=false to attack=true

	int				id;			// player's id (position on the vectr)
	
	int				cid;		// client id (network identity)

	char			name[256];		// player's name

	double		waitnametime;		// protect from name change flooding
	
	int				x, y;			// position in world (screen coordinates)

	int				oldx, oldy;		// old positions (to detect screen changing)

	int				drawptr;		// HACK: id of the player to draw (depth sorting for drawing order)
	int				drawused;		// HACK: id of the player to draw (depth sorting for drawing order)

	int				ping;				// the ping time

	int				frags;			// integer number displayed on the scoreboard ("frags")
	int				oldfrags;		// last value informed to the client

	int				health;			// current health (sent always 2-byte)

	int				energy;			// player's energy (run/shoot)

	bool			dead;		//dead? zframe byte == 255  clientside only
	bool			old_dead;		// to detect time to play death sound

	double		next_shoot_time;		// minimum time for next shoot

	double		respawn_time;				// time for respawn

	bool			respawn_to_base;		// force respawning to base
};


// a player's sprite state
struct hero_t {

	// position and speed
	double    x, y, z, sx, sy;

	// old coords: garantidamente NAO em paredes
	double		ox, oy;

	// left, right, up, down keypresses (player acceleration vectrs)
	bool			l, r, u, d;			

	// gun direction 0-7 (0 = right 1 = right-down 2 = down ...... 7 = right-up
	int				gundir;

	// strafing?
	bool			strafe;

	//running
	bool			run;

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

	//owning player-id (-1  == unused)
	int	owner;

	//team/color
	int team;

	//power	(na verdade a partir da versao 0.1.2, cada rocket pode ser um multi-rocket!
	NLubyte		power;

	//notification list (bitfield, bit0=player0, bit1=player1... etc.)
	NLulong		vislist;

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
	
	//hit_target. se ==255, ninguem em particular
	int		 hit_target;

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

	//where is it if dropped
	spoint_t	pos;
};

//pickups
class pickup_c {
public:

	NLubyte   kind;		// type of powerup  0==unused     255=valid, but respawning

	double		respawn_time;		// time to respawn

	int				px;   //screen
	int				py;
	int				x;		//position
	int				y;

	pickup_c() { kind=0; }
};

// a game frame, or game state
class frame_t {
public:

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


//************************************************************
//  server stuff
//************************************************************

#define SERVER_DEFAULT_PLAYER_NAME "**DEFAULT**"

// client count
int	player_count;

// server callbacks
int sfunc_client_hello(runes_t *arg);
int sfunc_client_connected(runes_t *arg);
int sfunc_client_disconnected(runes_t *arg);
int sfunc_client_lag_status(runes_t *arg);
int sfunc_client_data(runes_t *arg);
int sfunc_client_ping_result(runes_t *arg);

// the game server state
class gameserver_c {
public:

	// the current worldmap
	map_c		map;

	// net server
	server_c	*server;

	//hostname
	char		hostname[256];

	// the players
	player_t	player[MAX_PLAYERS];
	int				ctop[64];			// client id-to-player id index

	//the current frame (game world simulation state)
	frame_t		world;

	// current frame count
	NLulong		frame;

	//total server traffic in kbytes/sec
	double server_kbps_traffic;

	// ping send counter
	int ping_send_counter, ping_send_client;

	//ctor
	gameserver_c() {
		server = 0;
		hostname[0]=0;	//hostname
		//memset(&world, 0, sizeof(frame_t));		//the current frame (game world simulation state)
		frame = 0;		// current frame count
		server_kbps_traffic = 0;		//total server traffic in kbytes/sec
		ping_send_counter = 0;		// ping send counter
		ping_send_client = 0;
	}

	//dtor
	virtual ~gameserver_c() {
		LOG("GAMESERVER_C() DESTRUCTOR");
		if (server) {
			delete server;
			server = 0;
		}
	}

	//---- CTF/GAME ------------------

	//broadcast new player name
	void broadcast_player_name(int pid) {
		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 1);	// "1" = player name update
		writeByte(lebuf, count, pid);		// what player id
		player[pid].name[15]=0;		// force trunc name at 15 chars (paranoia)
		writeString(lebuf, count, player[pid].name);
		server->broadcast_message(lebuf, count);
	}

	//ugly hack
	int check[MAX_PLAYERS];
	int checount;

	//check if team change requests can be satisfied
	void check_team_changes() {

		// check players in random order
		//
		for (int i=0;i<MAX_PLAYERS;i++) check[i]=0;
		checount = MAX_PLAYERS;
		while (checount > 0) {

			int p = rand() % MAX_PLAYERS;
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

		//count players in each team
		int tc[2];tc[0]=0;tc[1]=0;
		for (int i=0;i<MAX_PLAYERS;i++)
			if (player[i].used)
				tc[i/8]++;

		//check if team changing happens: calculate delta TARGET TEAM - MY TEAM 
		int teamdelta = tc[1-(pid/8)] - tc[pid/8];
		
		//case 0: target team with MORE players: do not move
		if (teamdelta > 0) {
		}
		//case 1: target team with 2 players less:  move player without trades
		else if (teamdelta <= -2) {
			// MOVE W/O TRADE
			for (int i=0;i<MAX_PLAYERS;i++) 
			if (!player[i].used)		// player vago
			if (i/8 != pid/8)			// no time oposto
			{
				move_player(pid, i);	// move pid to free slot
				break;
			}
		}
		//case 2: target team with 0 player less: check for trade, else do nothing
		//case 3: target team with 1 player less: check for trade, else go anyways
		else {
			// FIND A TRADE
			bool found = false;

			for (int i=0;i<MAX_PLAYERS;i++)
			if (player[i].used)	// um player
			if (i/8 != pid/8)		// do outro time
			if (player[i].want_change_teams)		// que quer trocar de time
			{
				found = true;
				swap_players(pid, i);		// make trade
				break;
			}							

			// IF TRADE NOT FOUND AND TEAMDELTA == 1, MOVE W/O TRADE
			if ((teamdelta == -1) && (!found)) {
				for (int i=0;i<MAX_PLAYERS;i++) 
				if (!player[i].used)		// player vago
				if (i/8 != pid/8)			// no time oposto
				{
					move_player(pid, i);	// move pid to free slot
					break;
				}
			}
		}					
	}

	// messages to update moved players (players/clients with new clients/players)
	//
	void move_update_player(int a) {

		char lebuf[256]; int count = 0;
		if (player[a].used) {
			broadcast_player_name(a);

			count = 0;
			writeByte(lebuf, count, 3); // "3" = first-packet information
			writeByte(lebuf, count, ((NLubyte)a) );		// who am I
			writeByte(lebuf, count, ((NLubyte)world.flag[0].score) );		//team 0 current score
			writeByte(lebuf, count, ((NLubyte)world.flag[1].score) );		//team 1 current score
			server->send_message(player[a].cid, lebuf, count);

			//message
			broadcast_message("@I%s moved to %s team", player[a].name, teamname[a/8]);
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
		memcpy(&(player[t]), &(player[f]), sizeof(player_t));

		//copy hero
		world.hero[t] = world.hero[f];

		//remove f
		game_remove_player(f);

		//update ctop 
		ctop[ player[t].cid ] = t;

		//I really dont want to change teams no more..
		player[t].want_change_teams = false;

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
		player_t	ptemp;
		ptemp = player[a];
		player[a] = player[b];
		player[b] = ptemp;

		//swap client id's
		ctop[ player[a].cid ] = a;
		ctop[ player[b].cid ] = b;

		//either don't want to change teams anymore
		player[a].want_change_teams = false;
		player[b].want_change_teams = false;
		
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

		NLubyte te = team;
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
			p = world.flag[team].pos.px;		//px
			writeByte(lebuf, count, p);

			p = world.flag[team].pos.py;		//py
			writeByte(lebuf, count, p);

			sh = world.flag[team].pos.x;		//x
			writeShort(lebuf, count, sh);

			sh = world.flag[team].pos.y;		//y
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

		if (world.flag[t].score == CTF_NUMBER_OF_CAPTURES) {
			//maximum score reached -- restart game
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
		int t = pid/8;

		//choose a team spawn point
		if (++map.tinfo[t].lastspawn >= MAX_TEAM_SPAWNS)
			map.tinfo[t].lastspawn = 0;
		
		spoint_t pos;
		if (!killed) {
			int sp = map.tinfo[t].lastspawn;		//team spawn point #
			pos = map.tinfo[t].spawn[sp];	// the point
		}
		else {
			//if killed, generate a random spot for respawn:
			// - unnocupied screen
			// - away from walls

			//calculate room touch matrix
			bool roompop[WXMAX][WYMAX];
			memset(roompop, 0, WXMAX*WYMAX);
			for (int i=0;i<MAX_PLAYERS;i++)
			if (player[i].used)
			if (player[i].x >= 0)	//enforce valid player screen (needed for "touch room")
			if (player[i].y >= 0)
			if (player[i].x < WXMAX)
			if (player[i].y < WYMAX)
				roompop[player[i].x][player[i].y] = true;
			
			//find screen
			do {
				pos.px = rand() % WXMAX;
				pos.py = rand() % WYMAX;
			} while (roompop[pos.px][pos.py] == true);	//keep trying until unnocupied (==false)

			//find a suitable coordinate -- middle square
			// FIXME: do a check for walls, maybe retrying another screen if hits a wall
			pos.x = plw / 4 + rand() % (plw / 2);
			pos.y = plh / 4 + rand() % (plh / 2);
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
		
		player[pid].weapon = 0;		//default weapon
		//notify player weapon power change
		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 18);		//player power change
		writeByte(lebuf, count, ((NLubyte)player[pid].weapon) );
		server->send_message(player[pid].cid, lebuf, count);

		player[pid].item_shield = false;			// no items
		player[pid].item_quad = false;
		player[pid].item_speed = false;
		player[pid].item_helm = 0;

		//FIXME: add teleport visual effect and sound
	}

	//delete a rocket
	void game_delete_rocket(int r, int framesleft, int targ) {

		//LOG1("game_delete_rocket %i\n", r);

		rocket_c *rock = &(world.rock[r]);
		
		//assembly rocket delete message
		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 8);		// 8 = rocket deletion
		NLubyte byt = (NLubyte)r;
		writeByte(lebuf, count, byt);		// rocket-object id
		byt = (NLubyte)targ;
		writeByte(lebuf, count, byt);		// player-target. if 255, no player in particular was hit
		byt = (NLubyte)framesleft;
		writeByte(lebuf, count, byt);		// 10-msecs' left to the client to animate

		//send message to players that received the rocket
		for (int p=0;p<MAX_PLAYERS;p++) 
		if (player[p].used)								//still valid player? (nao custa checar..)
		if (rock->vislist & (1 << p))			//verifica se o bit de "conhece o rocket" ta ligado
		{
			// send the message to this player
			server->send_message(player[p].cid, lebuf, count);

			//LOG2("...sent to pl=%i rock=%i\n", p, byt);
		}

		//server-side invalidate
		rock->owner = -1;
	}

	//shoot rocket to a certain direction
	//deg: em radianos
	//retorno: id do rocket alocado
	//XDELTA: deslocamento positivo para a direita ou negativo para a esquerda
	NLubyte game_do_shoot_rocket(int playernum, int px, int py, int x, int y, double deg, int xdelta) {

		for (NLubyte i=0;i<MAX_ROCKETS;i++) 
		if (world.rock[i].owner == -1) //unused
		{
			//alloc
			rocket_c *rock = &(world.rock[i]);
			rock->owner = playernum;
			rock->team = playernum/8;
			
			//SERVER NAO USA TIME!
			//rock->sv_time = get_time() - 0.25;		//v0.1.2 : + 1/2 frame

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

			//alocado
			return i;
		}

		//ops
		return 66;
	}

	//versao 0.1.2
	void game_shoot_rocket(int playernum, int shots, int px, int py, int x, int y, int gundir) {

		//char lix[2000];
		//sprintf(lix, "game_shoot_rocket  %i shots", shots);
		//broadcast_message(lix);

		//ids alocados pra shots
		NLubyte		sid[16];

		// center degree
		double cdeg = gundir * PIOIT;

		//allocate a new rocket server-side for each shot
		// shots = qual arma (1-9 tiros!)
		switch (shots) {
#ifdef OLD_DIRECTIONALS
		case 1:
			sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, 0);
			break;
		case 2:
			sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, - SHOT_DELTAX);
			sid[1] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, + SHOT_DELTAX);
			break;
		case 3:
			sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, 0);
			sid[1] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT, 0);
			sid[2] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT, 0);
			break;
		case 4:
			sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, - SHOT_DELTAX);
			sid[1] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, + SHOT_DELTAX);
			sid[2] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT, 0);
			sid[3] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT, 0);
			break;
		case 5:
			sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, 0);
			sid[1] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT, 0);
			sid[2] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT, 0);
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
			sid[1] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT, 0);
			sid[2] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT, 0);
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
			sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, - SHOT_DELTAX);
			sid[1] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, + SHOT_DELTAX);
			sid[2] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT, 0);
			sid[3] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT, 0);
			sid[4] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT * 2, 0);
			sid[5] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT * 2, 0);
			sid[6] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT * 3, 0);
			sid[7] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT * 3, 0);
			sid[8] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT * 4, 0);	//+ ou - tanto faz = 180 graus!
			break;
#else
		case 1:
			sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, 0);
			break;
		case 2:
			sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, - SHOT_DELTAX);
			sid[1] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, + SHOT_DELTAX);
			break;
		case 3:
			sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, 0);
			sid[1] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg + PIOIT, 0);
			sid[2] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg - PIOIT, 0);
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
#endif
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
		powerdir = shots;			// shots
		powerdir += gundir * 16;	//16,32,64,128
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

		//send to all people, build people-that-know WORD
		//send message to players on the same screen
		NLushort  vislist = 0;
		for (int p=0;p<MAX_PLAYERS;p++) 
		if (player[p].used)
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
		
		int enemyteam = 1 - (pid/8);

		//if is carrier of enemy flag, drop it, extra frag for fragging carrier
		if (world.flag[enemyteam].carried)		// attacker team's flag carried
		if (world.flag[enemyteam].carrier == pid) {	//...by the target

			//message
			broadcast_message("@I%s LOST THE %s FLAG!", player[pid].name, teamname[enemyteam]);

			//sound broadcast
			broadcast_sample(SAMPLE_CTF_LOST);
				
			//drop the flag
			ctf_drop_flag(enemyteam, player[pid].x, player[pid].y, (int)world.hero[pid].x, (int)world.hero[pid].y);

			return true;
		}

		return false;
	}

	//damage/kill player
	void game_damage_player(int target, int attacker, int damage) {

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
			if (player[attacker].item_quad) {
				damage *= 4;
			}
			// if no attacker quad, shield absorbs at least 1 shot
			//else 

			//v0.2.2: shield always absorbs
			if (player[target].item_shield) {
				
				player[target].energy -= damage;
				if (player[target].energy <= 0) {
					player[target].energy = 0;
					player[target].item_shield = false;
					broadcast_screen_sample(target, SAMPLE_SHIELD_LOST);
				}
				else
					broadcast_screen_sample(target, SAMPLE_SHIELD_DAMAGE);
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

			int tateam = target/8;
			int atteam = attacker/8;

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
						broadcast_message("@I%s DEFENDS THE %s CARRIER", player[attacker].name, teamname[atteam]);
						player[attacker].frags++; 
					}
				}
				//check if my flag is atbase in target's screen
				//
				else if (world.flag[atteam].atbase) {

					if (world.flag[atteam].pos.px == player[target].x)	// flag in target's screen
					if (world.flag[atteam].pos.py == player[target].y)
					{
						//defends the flag!
						broadcast_message("@I%s DEFENDS THE %s FLAG", player[attacker].name, teamname[atteam]);
						player[attacker].frags++; 
					}
				}
			}

			//drop flag if any
			if (ctf_drop_flag_if_any(target)) {
				//extra frag for fragging a carrier
				if (attacker != target)
					player[attacker].frags++;
			}

			//frag to attacker for the kill
			if (attacker != target)
				player[attacker].frags++;

			//broadcast obituary
			if (attacker != target) {
				char lix[256];
				sprintf(lix, "@I%s was nailed by %s", player[target].name, player[attacker].name);
				broadcast_message(lix);
			}

			//respawn target in a few seconds
			player[target].respawn_time = get_time() + 2.0;
			if (damage == 666666)
				player[target].respawn_to_base = true;	//special respawn-to-base damage
			else
				player[target].respawn_to_base = false;
		}
	}

	//remove player from the game
	void game_remove_player(int pid) {

		//remove all shots from this player
		for (int r=0;r<MAX_ROCKETS;r++) 
		if (world.rock[r].owner == pid)
			game_delete_rocket(r, 0, 255);

		//if player carrying flag, drop it
		ctf_drop_flag_if_any(pid);

		//erase player
		player[pid].used = false;
	}

	//restart ctf game
	void ctf_game_restart() {

		//final score
		char lix[256];
		sprintf(lix, "@ICTF GAME RESTARTED - FINAL SCORE:   %i RED x %i BLUE !", world.flag[0].score, world.flag[1].score);
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

		// zero all player frags and kill them
		for (int i=0;i<MAX_PLAYERS;i++) 
		if (player[i].used)
		{
			//kill - to respawn
			game_damage_player(i, i, 666666);	//666666==special damage - spawn on base
			//zero score
			player[i].frags = 0;
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
		int px, py, i;
		bool hit;
		do {
			hit = false;
			px = rand() % WXMAX;
			py = rand() % WYMAX;
			
			//check players
			for (i=0;i<MAX_PLAYERS;i++)
			if (player[i].used)
			if (player[i].x == px)
			if (player[i].y == py) {
				hit = true;
				break;
			}
			
			//check items if no players found
			if (!hit)
			for (i=0;i<MAX_PICKUPS;i++)
			if (world.item[i].kind != 0)
			if (world.item[i].px == px)
			if (world.item[i].py == py) {
				hit = true;
				break;
			}		

#ifdef REALLY_DEBUG_POWERUPS
			px = 4;
			py = 4;
#endif
			
		} while (hit);

#ifdef REALLY_DEBUG_POWERUPS
		char lixx[2000];
		sprintf(lixx, "pickup %i respawn %i %i RESPAWNING==FALSE KIND>0\n", p, px, py);
		broadcast_message(lixx);
#endif

		//alloc powerup
		world.item[p].kind = 1 + (rand() % NUMBER_OF_POWERUP_KINDS);  //  % x   = x different items
		//world.item[p].respawning = false;
		world.item[p].px = px;
		world.item[p].py = py;
#ifdef DEBUG_POWERUPS
		LOG3("POWERUP %i placed at %i   %i \n", p, px, py);
#endif
		//find a suitable coordinate -- middle square
		// FIXME: do a check for walls, maybe retrying another screen if hits a wall
		world.item[p].x = plw / 4 + rand() % (plw / 2);
		world.item[p].y = plh / 4 + rand() % (plh / 2);
	}

	// verifica powerups unused por jogadores presentes
	void check_pickup_creation() {

		int i, pc, ic;

		//count number of players
		pc = 0;
		for (i=0;i<MAX_PLAYERS;i++)
			if (player[i].used)
				pc++;

		//count number of items
		ic = 0;
		for (i=0;i<MAX_PICKUPS;i++)
			if (player[i].used)
				ic++;

		//while number of players > number of pickups: create a pickup and ic++
		for (i=0;i<MAX_PICKUPS;i++)
#ifndef DEBUG_POWERUPS
		if ((pc > ic) || (ic < 5))
#endif
		if (world.item[i].kind == 0)
		{
			world.item[i].kind = 255;
			world.item[i].respawn_time = get_time() + 2.0;

#ifdef REALLY_DEBUG_POWERUPS
		char lixx[2000];
		sprintf(lixx, "pickup %i creation RESPAWNING==TRUE KIND=0\n", i);
		broadcast_message(lixx);
#endif

		}
	

	/*
		for (int i=0;i<MAX_PLAYERS;i++)
#ifdef DEBUG_POWERUPS
#else
		if ((player[i%8].used) || (player[8+i%8].used))	//jogs de qualquer um dos times
#endif
		if (world.item[i].kind == 0) {
			world.item[i].kind = 255;		// 255=valid but respawning
			//world.item[i].respawning = true;			// can't touch

#ifdef REALLY_DEBUG_POWERUPS
		char lixx[2000];
		sprintf(lixx, "pickup %i creation RESPAWNING==TRUE KIND=0\n", i);
		broadcast_message(lixx);
#endif

			world.item[i].respawn_time = get_time() + PICKUP_RESPAWN_TIME;
		}
		*/
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

			player[p].item_speed = true;	
			player[p].item_speed_time = get_time() + 60.0;

			char lebuf[256]; int count = 0;
			writeByte(lebuf, count, 17);		//powerup time indicator
			writeByte(lebuf, count, it->kind);		//what kind
			writeShort(lebuf, count, (NLushort)60);		//time
			server->send_message(player[p].cid, lebuf, count);

			broadcast_screen_sample(p, SAMPLE_BOOTS_ON);
		}
		//helm
		else if (it->kind == 3) {

			player[p].item_helm = 1;		//invis maximo de inicio
			player[p].item_helm_time = get_time() + 60.0;

			char lebuf[256]; int count = 0;
			writeByte(lebuf, count, 17);		//powerup time indicator
			writeByte(lebuf, count, it->kind);		//what kind
			writeShort(lebuf, count, (NLushort)60);		//time
			server->send_message(player[p].cid, lebuf, count);

			broadcast_screen_sample(p, SAMPLE_HELM_ON);
		}
		//quad
		else if (it->kind == 4) {

			player[p].item_quad = true;		
			player[p].item_quad_time = get_time() + 60.0;

			char lebuf[256]; int count = 0;
			writeByte(lebuf, count, 17);		//powerup time indicator
			writeByte(lebuf, count, it->kind);		//what kind
			writeShort(lebuf, count, (NLushort)60);		//time
			server->send_message(player[p].cid, lebuf, count);

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
			char lebuf[256]; int count = 0;
			writeByte(lebuf, count, 18);		//player power change
			writeByte(lebuf, count, ((NLubyte)player[p].weapon) );
			server->send_message(player[p].cid, lebuf, count);

			broadcast_screen_sample(p, SAMPLE_WEAPON_UP);
		}
		//megahealth
		else if (it->kind == 6) {

			//increase energy +100, upto 300
			player[p].energy += 100;
			if (player[p].energy > 300)
				player[p].energy = 300;

			//increase health +100, upto 300
			player[p].health += 100;
			if (player[p].health > 300)
				player[p].health = 300;

			broadcast_screen_sample(p, SAMPLE_MEGAHEALTH);
		}

		// unused item
		it->kind = 0;		

		// check pickup creation
		check_pickup_creation();
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

				//broadcast_message("sending powerup update\n");
				
				//v0.1.2: PRIMEIRO verifica se tem mais alguem nessa tela. se nao
				//  tiver, verifica se nao seria interessante mudar o "kind" do item
				//muda WHILE item alvo eh powerup cujo time do jogador eh > 30
				bool temjog = false;
				for (int j=0;j<MAX_PLAYERS;j++)
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

						if ((it->kind == 1) && (player[p].health >= 100) && (player[p].energy >= 100) && (player[p].item_shield))
							non_satisfactory = true;
						else if ((it->kind == 2) && (player[p].item_speed) && (player[p].item_speed_time - get_time() > 40.0))
							non_satisfactory = true;
						else if ((it->kind == 3) && (player[p].item_helm) && (player[p].item_helm_time - get_time() > 40.0))
							non_satisfactory = true;
						else if ((it->kind == 4) && (player[p].item_quad) && (player[p].item_quad_time - get_time() > 40.0))
							non_satisfactory = true;

						//re-choose item type
						if (non_satisfactory)	
							it->kind = 1 + rand() % NUMBER_OF_POWERUP_KINDS;

					} while (non_satisfactory);

					//if loop choosed "weapon" powerup (item 5) but you are at maximum, then keep the original choice
					if ((it->kind == 5) && (player[p].weapon >= 8))
						it->kind = original;
				}

				//send a "item on the screen" message
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


	//broadcast team message
	void broadcast_team_message(int team, char *text) {
		
		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 2);
		writeString(lebuf, count, text);

		// send message only to teammates
		for (int i=0;i<MAX_PLAYERS;i++)
		if (player[i].used)
		if ((i/8) == team) {
			server->send_message(player[i].cid, lebuf, count);
		}

	}

	//broadcast message to all players in one screen
	void broadcast_screen_message(int px, int py, char *lebuf, int count) {
	
		for (int j=0;j<MAX_PLAYERS;j++)
		if (player[j].used)
		if (player[j].x == px)
		if (player[j].y == py)
			server->send_message(player[j].cid, lebuf, count); //send the message
	}

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
	//broadcast message to all
	void broadcast_message(char *text) {
		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 2);
		writeString(lebuf, count, text);
		for (int i=0;i<MAX_PLAYERS;i++)
		if (player[i].used)
			server->send_message(player[i].cid, lebuf, count);
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

				LOG1("modline '%s'\n", s);

				if (strlen(s) == 0)	//skip blank
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
					else 
						cmd = 0;

					LOG1("is command %i\n", cmd);
				}
				else {
					double val = 1.0;
					
					if (cmd < 1000) {
						sscanf(s, "%lf", &val);
					}

					LOG3("set cmd %i value to %f from '%s'\n", cmd, val, s);

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
				}

				//parameter
				command = !command;
			}

			fclose(fmod);
		}
	}

	//----- THE REST  ----------------

	//start server
	bool start() {

		ping_send_counter = 0;
		ping_send_client = 0;

		// server game phisics parameters
		// DEFAULT VALUES...
		//
		set_default_physics();

		// default map
		load_default_map(&map);
		
		// reset players
		//players_present = 0;
		player_count = 0;
		for (int i=0;i<MAX_PLAYERS;i++) {
			player[i].used = false;
			player[i].id = i;
			player[i].name[0]=0;
		}

		//load hostname
		char dest[WHERE_PATH_SIZE];
		append_filename(dest, wheregamedir, "hostname.txt", WHERE_PATH_SIZE);
		FILE *cfg = fopen(dest, "r");
		if (cfg) {
			fgets(hostname, 256, cfg);
			hostname[ strlen(hostname) - 1 ] = 0;  //replace newline with \0
			LOG1("HOSTNAME IS = '%s'\n", hostname);
			fclose(cfg);
		}
		else
			strcpy(hostname, "ANNONYMOUS HOST");

		// load physics parameters from gamemod.txt
		load_game_mod();		

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

		// reset game
		ctf_game_restart();

		//default serverinfo
		update_serverinfo();

		return true;
	}

	//update serverinfo
	void update_serverinfo() {
		char sinfo[1024];
		sprintf(sinfo, "%2i %7s/%s", player_count, GAME_VERSION, hostname);
		server->set_server_info(sinfo);
	}

	//client connected (callback function)
	void client_connected(int id) {
		
		//another one...
		player_count++;

		//update serverinfo
		update_serverinfo();

		// se o player_count ficou == 2, reseta partida
		if (player_count == 2)
			ctf_game_restart();

		//2TEAM: check wich team to put player
		int t1, t2, targ;
		t1 = 0;		//red team count
		t2 = 0;		//blue team count
      int i;
		for (i=0;i<MAX_PLAYERS;i++) {
			if (player[i].used)
				if (i/8 == 0)
					t1++;
				else
					t2++;
		}

		//put on red team, blue team, or randomize if same # of players in both teams
		if (t1 < t2)
			targ = 0;
		else if (t1 > t2)
			targ = 8;
		else
			targ = (rand()%2) * 8;	// 0 or 8

		//alloc new player : scans only slots of the team (targ...targ + 7)
		int myself = -1;

//#define DEBUG_SOMETHING

#ifdef DEBUG_SOMETHING
		static int bla = 0;			//vai alocando players disjuntos
		i = bla++;
#else
		for (i=targ;i<(targ+8);i++) {
			if (!player[i].used) {
#endif

				// add player to players_present
				//players_present = players_present | (1 << i);

				// init player
				memset(&(player[i]), 0, sizeof(player_t));
				player[i].ping = 0;
				player[i].frags = 0;	//reset score ?
				player[i].oldfrags = -666;
				player[i].want_change_teams = false;	// don't want to change teams yet
				player[i].next_shoot_time = 0;
				player[i].attack = false;
				player[i].attacktime = 0;
				ctop[id] = i;			// index for fast find
				player[i].cid = id;
				player[i].id = i;
				strcpy(player[i].name, SERVER_DEFAULT_PLAYER_NAME);	//the default name
				player[i].waitnametime = get_time() - 666.0;	//can change name right now
				player[i].used = true;
				myself = i;

				// spawn player
				game_respawn_player(false, i);

				//reset keypresses
				world.hero[i].l = 0;
				world.hero[i].r = 0;
				world.hero[i].u = 0;
				world.hero[i].d = 0;

				//check pickup creation
			  check_pickup_creation();

#ifndef DEBUG_SOMETHING
				break;
			}
		}
#endif

		// internal error can be detected if no player is free (used == false)
		if (myself == -1) {
			LOG("ERROR: BAD BAD BAD INTERNAL GAMESERVER ERROR myself == -1 !!! client_connected()...\n");
			return;
		}

		//update the player with world information
		char lebuf[256]; int count;

		//	- who is he (player #)
		count = 0;
		writeByte(lebuf, count, 3); // "3" = first-packet information
		writeByte(lebuf, count, ((NLubyte)myself) );		// who am I
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
		server->send_message(id, lebuf, count);

		// - world ctf flags information
		ctf_net_flag_status(id, 0);
		ctf_net_flag_status(id, 1);

		// - all other player's names
		// - all other player's frags
		
		for (i=0;i<MAX_PLAYERS;i++)
		if (player[i].used)
		if (i != myself) {

			//player name update
			count = 0;
			writeByte(lebuf, count, 1);	// "1" = player name update
			writeByte(lebuf, count, i);		// what player id
			player[i].name[15]=0;		// force trunc name at 15 chars (paranoia)
			writeString(lebuf, count, player[i].name);
			server->send_message(id, lebuf, count);

			//frags update
			count = 0;
			writeByte(lebuf, count, 4);	//"4" = frags update
			writeByte(lebuf, count, i);		// what player id
			writeLong(lebuf, count, player[i].frags);
			server->send_message(id, lebuf, count);
		}

		//check for team changes
		//
		check_team_changes();
	}

	//client disconnected (callback function)
	void client_disconnected(int id) {

		//less one...
		player_count--;

		//update serverinfo
		update_serverinfo();

		//what player
		int pid = ctop[id];		

		//broadcast a textual message "Player BLABLA left the game"
		char lix[64]; 
		broadcast_message("@I%s left the game with %s frags", player[pid].name, itoa(player[pid].frags, lix, 10));

		//sound
		broadcast_sample(SAMPLE_LEFTGAME);

		//remove player from the game
		game_remove_player(pid);

		//check for team changes
		//
		check_team_changes();
	}

	//client ping result
	void ping_result(int client_id, int ping_time) {

		//save result
		player[ ctop[client_id] ].ping = ping_time;
	}

	//process incoming client data (callback function)
	void incoming_client_data(int id, char *data, int length) {

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
		int	 msglen;
		do {
			msg = server->receive_message(id, &msglen);
			if (msg != 0) {
				// process a client's message
				//
				int count = 0;
				NLubyte code;
				readByte(msg, count, code);
				if (code == 1) {

					//name change flooding protection
					if (get_time() >= player[pid].waitnametime) {

						//check if it's the first name information from client. then it
						// must have just entered the game
						bool entered_game = !strcmp(SERVER_DEFAULT_PLAYER_NAME, player[pid].name);

						// log
						LOG3("client %i player %i '%s' renamed to", id, pid, player[pid].name);
						readString(msg, count, player[pid].name); //name update request
						LOG1("'%s'\n", player[pid].name);

						//send entered-game message
						char dummy[1] = { 0 };
						if (entered_game) {
							broadcast_message("@I%s entered the game%s", player[pid].name, dummy);
							//sound
							broadcast_sample(SAMPLE_ENTERGAME);
						}
						// next time allowed to change name
						player[pid].waitnametime = get_time() + 1.0;

						//broadcast the new player's name 
						broadcast_player_name(pid);
					}
				}
				//chat!
				else if (code == 2) {

					//FIXME: talk flood protection .... ?

					// check for team message:
					if (msg[1] == '.') {
						sprintf(talkmsg, "@T%s: %s", player[pid].name, &(msg[2]));
						broadcast_team_message(pid/8, talkmsg);
					}
					//regular msg
					else {
						sprintf(talkmsg, "%s: %s", player[pid].name, &(msg[1]));
						broadcast_message(talkmsg);
					}

					// log
					LOG4("client %i player %i name %s says: '%s'\n", id, pid, player[pid].name, &(msg[1]));
				}
				//+attack
				else if (code == 5) {

					//if attack was false, it's a fire event (fire false-->true)
					bool fire_event = (player[pid].attack == false);

					player[pid].attack = true;

					if (fire_event) {

						//set time of startfire (may be used for charge-shot)
						player[pid].attacktime = get_time();

						// firing code moved to simulate/broadcast frame (player[id].attack is tested there)
					}
			
				}
				//SUICIDE!!
				else if (code == 10) {

					//only if alive still
					if (player[pid].health > 0) {

						game_damage_player(pid, pid, 30000);

						//frag penalty
						if (player[pid].frags > 0)
							player[pid].frags--;						
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
						broadcast_message("@I%s player '%s' wants to change teams", teamname[pid/8], player[pid].name);
						//check for team changes
						check_team_changes();
					}
				}
				// want changeteams off
				else if (code == 13) {
					// show message if command had effect
					if (player[pid].want_change_teams) {
						player[pid].want_change_teams = false;
						broadcast_message("@I%s player '%s' don't want to change teams", teamname[pid/8], player[pid].name);
					}
				}
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
			char lixx[2000];
			sprintf(lixx, "time to respawn item %i\n", i);
			broadcast_message(lixx);
#endif

			respawn_pickup(i);
		}

		// (0) do stuff for every player
		//
		for (i=0;i<MAX_PLAYERS;i++) 
		if (player[i].used) {

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
				if (player[i].item_helm < 7)		// minimum
					player[i].item_helm = 7;
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
				if (player[i].energy < 0) 	//se ficou menor que zero, atira 1 so
					player[i].energy = 0;
				else {
					for (int k=1;k<player[i].weapon+1;k++) {
						//try add one shot
						player[i].energy -= 2;
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
		for (i=0;i<MAX_ROCKETS;i++)
		if (world.rock[i].owner != -1)	//exists
		{
			rocket_c *rock = &(world.rock[i]);

			//run ten times for better collision accuracy (UGLY UGLY UGLY HACK)
			for (int t=0;t<10;t++)
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

				// check if a player (alive) is hit by this rocket now
				//
				//sqrt( (ex - x)*(ex - x) + (ey - y)*(ey - y) ). Acho que é isto...

				for (int p=0;p<MAX_PLAYERS;p++)
				if (player[p].used)
				if (player[p].health > 0)		// alive
				if (rock->team != (p/8)) // shot is from opposing team
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
						//damage the target: dano 70 = almost sure kill
						game_damage_player(p, rock->owner, 70);

						//if player not dead, push him
						if (player[p].health > 0) {

							//se indo no sentido contrario, primeiro zera
							//(emulando comportamento v0.1.0)
							if (
									((world.hero[p].sx > 0) && (rock->sx < 0))
									||
									((world.hero[p].sx < 0) && (rock->sx > 0))
								 )
								 world.hero[p].sx = 0;
							if (
									((world.hero[p].sy > 0) && (rock->sy < 0))
									||
									((world.hero[p].sy < 0) && (rock->sy > 0))
								 )
								 world.hero[p].sy = 0;

							//adiciona velocidade do rocket/3
							world.hero[p].sx += rock->sx / 3.0;
							world.hero[p].sy += rock->sy / 3.0;

							//FIXME
							//char lix[2000];
							//sprintf(lix, "HIT nsx=%2.2f nsy=%2.2f rsx=%2.2f %2.2f", world.hero[p].sx
							//		, world.hero[p].sy
							//		, rock->sx
							//		, rock->sy
							//	);
							//broadcast_message(lix);
						}

						//delete shot
						game_delete_rocket(i, t, p);

						//2-loop break
						t=999;break;
					}
				}

			}
		}

		// for each player, update positions & speeds
		//
		hero_t  *h;

		for (i=0;i<MAX_PLAYERS;i++) 
		if (player[i].used)
    {
			h = &(world.hero[i]);

			//check if dead/respawn
			if (player[i].health <= 0) {

				if (player[i].respawn_time < get_time()) {
					game_respawn_player( !player[i].respawn_to_base , i);		//time to respawn player
				}

			}
			// player alive: do stuff for alive players
      else {

				//select effective physics vars for the player
				//
				double player_accel;
				double player_friction;
				double player_maxspeed;
				if (h->run) {
					if (player[i].item_speed) {
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
					if (player[i].item_speed) {
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
/*
				//max speed: running?
				double max_speed_x, max_speed_y;
				if (h->run) {
					max_speed_x = sv_maxspeedrunx;
					max_speed_y = sv_maxspeedruny;
				}
				else {
					max_speed_x = sv_maxspeedx;
					max_speed_y = sv_maxspeedy;
				}
*/

				//flag carrier disadvantage when running
				if (h->run)
				if (world.flag[1-(i/8)].carried)
				if (world.flag[1-(i/8)].carrier == i)
					player_maxspeed -= svp_flag_penalty;

				//powerup: boots bonus topspeed
				/*
				double boots_accel_bonus = 1.0;
				if (player[i].item_speed) {
					max_speed_x *= sv_boots_top_bonus_x;
					max_speed_y *= sv_boots_top_bonus_y;
				}
				*/

				//powerup boots / run: bonus accel
				/*
				if (player[i].item_speed)
					boots_accel_bonus = sv_boots_accel_bonus;
				else if (h->run)
					boots_accel_bonus = sv_run_accel_bonus;
				*/

					//friction x - apply if l xor r
#ifndef ALWAYS_FRICTION
//					if ( ((int)h->l + (int)h->r != 1) || (fabs(h->sx) > max_speed_x) ) {
					if ( ((int)h->l + (int)h->r != 1) || (fabs(h->sx) > player_maxspeed) ) {
#endif
						if (h->sx > 0) {
							//h->sx -= sv_frictionx * boots_accel_bonus;
							h->sx -= player_friction;
							if (h->sx < 0) h->sx = 0;
						}
						else if (h->sx < 0) {
							//h->sx += sv_frictionx * boots_accel_bonus;
							h->sx += player_friction;
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
							h->sy -= player_friction;
							if (h->sy < 0) h->sy = 0;
						}
						else if (h->sy < 0) {
							//h->sy += sv_frictiony * boots_accel_bonus;
							h->sy += player_friction;
							if (h->sy > 0) h->sy = 0;
						}
#ifndef ALWAYS_FRICTION
					}
#endif
				
				/*
				//accelerate x if not over maximum speed
				if ((h->l) && (h->sx > -max_speed_x))
					h->sx -= sv_accelx * boots_accel_bonus;
				if ((h->r) && (h->sx < +max_speed_x))
					h->sx += sv_accelx * boots_accel_bonus;

				//accelerate y if not over maximum speed
				if ((h->u) && (h->sy > -max_speed_y))
					h->sy -= sv_accely * boots_accel_bonus;
				if ((h->d) && (h->sy < +max_speed_y))
					h->sy += sv_accely * boots_accel_bonus;
				*/
				//accelerate x if not over maximum speed
				if ((h->l) && (h->sx > -player_maxspeed))
					h->sx -= player_accel;
				if ((h->r) && (h->sx < +player_maxspeed))
					h->sx += player_accel;

				//accelerate y if not over maximum speed
				if ((h->u) && (h->sy > -player_maxspeed))
					h->sy -= player_accel;
				if ((h->d) && (h->sy < +player_maxspeed))
					h->sy += player_accel;
		
				//save ox,oy
				h->ox = h->x;
				h->oy = h->y;

				//move x
				h->x += h->sx;
				if (h->x < 0) h->x = 0;
				else if (h->x > plw) h->x = plw;

				//move y
				h->y += h->sy;
				if (h->y < 0) h->y = 0;
				else if (h->y > plh) h->y = plh;

				//wall collision correction
				wallcorrect(i, &map, &(h->x), &(h->y), &(h->sx), &(h->sy), &h->ox, &h->oy, player[i].x, player[i].y);

				//check room change x
				if (h->x == plw) {
					h->x = 1;
					player[i].x += 1;
					if (player[i].x > WXMAX - 1)
						player[i].x = 0;

					//player screen changed check
					game_player_screen_change(i);
				}
				else if (h->x == 0) {
					h->x = plw - 1;
					player[i].x -= 1;
					if (player[i].x < 0)
						player[i].x = WXMAX - 1;

					//player screen changed check
					game_player_screen_change(i);
				}

				//check room change y
				if (h->y == plh) {
					h->y = 1;
					player[i].y += 1;
					if (player[i].y > WYMAX - 1)
						player[i].y = 0;

					//player screen changed check
					game_player_screen_change(i);
				}
				else if (h->y == 0) {
					h->y = plh - 1;
					player[i].y -= 1;
					if (player[i].y < 0)
						player[i].y = WYMAX - 1;

					//player screen changed check
					game_player_screen_change(i);
				}

				//---------------------------------
				// player every-frame stuff
				//---------------------------------

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

				// loose -2 energy or -2 health if running
				if (h->run) {
					
					if (player[i].energy <= 0) {

						//if (!player[i].item_speed)	// se ta com SPEED, faz nao hurt
						if (player[i].health > 30) {	// se health > 30, desconta
							
							if (ticker % 2)
								player[i].health -= 2;	//desconta 2 (o normal)
							else
								player[i].health -= 1;	//desconta 1 (menos)

							if (player[i].health < 40)		// garante minimo 30
								player[i].health = 40;	
						}

					} else {
						
						if (ticker % 2)
							player[i].energy -= 2; //desconta 2 (o normal)
						else
							player[i].energy -= 1; //desconta 1 (menos)

						if (player[i].energy == -1) { // special case
							player[i].energy++;
							if (player[i].health > 40) {	// se health > 30, desconta
								player[i].health--;
								if (player[i].health < 40)		// garante minimo 30
									player[i].health = 40;	
							}
						}
					}
				}

				//limit health 0 .. 300
				if (player[i].health < 0)
					player[i].health = 0;
				else if (player[i].health > 300)
					player[i].health = 200;

				//limit energy 0 .. 300
				if (player[i].energy < 0)
					player[i].energy = 0;
				else if (player[i].energy > 300)
					player[i].energy = 300;

				//---------------------------------
				// check game object collisions
				//---------------------------------

				int myteam = i/8;
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
				if (!world.flag[enemyteam].carried)	// enemy flag dropped (at base or somewhere)
				if (check_flag_touch(i, player[i].x, player[i].y, h->x, h->y, enemyteam))  // and I touch it
				{
					//FLAG STOLEN!
					player[i].frags += 1;	// just add some frags

					broadcast_message("@I%s GOT THE %s FLAG!", player[i].name, teamname[enemyteam]);

					ctf_steal_flag(enemyteam, i);  //flag stolen!

					//HELM powerup: show player
					if (player[i].item_helm > 0)
						player[i].item_helm = 255;
				}

				// --> CTF FLAG RETURN
				//
				if (!world.flag[myteam].carried)	// my flag dropped
				if (!world.flag[myteam].atbase)	// not at base
				if (check_flag_touch(i, player[i].x, player[i].y, h->x, h->y, myteam))  // and I touch it
				{
					//FLAG RETURNED!
					player[i].frags += 1;	// just add some frags

					broadcast_message("@I%s RETURNED THE %s FLAG!", player[i].name, teamname[myteam]);
									
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
				if (check_flag_touch(i, player[i].x, player[i].y, h->x, h->y, myteam))		// I touch my flag
				{
					//add 15 frags to all players of the team
					for (int h=0;h<MAX_PLAYERS;h++)
					if (player[h].used)
					if ((h/8) == myteam)
						player[h].frags += 15;	

					//return enemy flag to their base
					ctf_return_flag(enemyteam);

					//message
					broadcast_message("@I%s CAPTURED THE %s FLAG!", player[i].name, teamname[enemyteam]);

					//CAPTURE (team count ++)
					world.flag[myteam].score++;
					ctf_update_teamscore(myteam);		//this function can decide to restart the game . (?)

					//sound broadcast
					broadcast_sample(SAMPLE_CTF_CAPTURE);
				}
			
			} // player.health > 0

    }

		// (2)  broadcast the frame
		//
		//		o pacote nao eh o mesmo pra todo mundo, entao nao eh broadcast
		//		m uma parte que depende do player (tipo, qual o health do cara)
		// 
		// server frame format:
		//
		//  --- PRIMEIRA PARTE : igual pra todo mundo ----
		//
		//    1. LONG frame		 the server frame #  (logical simulation time = frame * 0.1 for 10hz server)
		//		2. WORD (bitfield) players present (bit 0 = player 0 present, bit 1. .... bit 7)
		//
		// --- SEGUNDA PARTE : varia p/ cada cliente -----
		//
		//		1. WORD (bitfield) players on screen
		//		2. p/ cada player_on_screen, um PLAYER DATA FIELD (ver abaixo)
		//		3. BYTE	(bitfield) enemies on minimap
		//		4. BYTE qual-enemy-#  BYTE x BYTE y  no minimap
		//		5. SHORT    health do cara
		//		6. SHORT		ping of player frame#%MAX_PLAYERS
		//
		// player data field: (10 bytes per player)
		//	  SHORT x,y,sx,sy     current position, and velocity for extrapolation
		//		BYTE  zframe				OLD JUMP INFO, now reserved for misc use
		//		BYTE	keys					bits 0..3: left,right,up,down  keys held (acceleration vectrs)
		//														 5..7: 3bit direction
		//	  BYTE  effects   SHIELD / SPEED / QUAD
		//		BYTE	helm alpha effect

		//RECALC PLAYERS PRESENT EVERY TIME
		NLushort players_present = 0;
		for (int pp=0;pp<MAX_PLAYERS;pp++)
		if (player[pp].used)
			players_present += (1 << pp);


		// ============================
		//   build common data buffer
		// ============================
		char lebuf[4096];		//common frame data
		int count = 0;

		//frame
		writeLong(lebuf, count, frame);

		//players present
		writeShort(lebuf, count, players_present);
		
		//===============================
		//  build packet for each client
		//		with custom data
		//===============================
		int lecount;	//count after "count"

		//stuff for minimap update: my team's enemy team view
		static int tviter[2] = { 0 , 0 };		
		static int helmiter = 0;		// HELM ITERATOR : manda todo mundo!
		int tview[2][8];		//[time][inimigo# visto? 1-8]
		NLubyte	tview_bits[2];	//enemy view byte (bitfield for the 8 enemies of each team(0,1))

		//atualiza HELM ITERATOR - para em um player valido ou entao qualquer um
		int runaway = MAX_PLAYERS + 1;
		do {
			helmiter++;
			if (helmiter > MAX_PLAYERS - 1)
				helmiter = 0;
			if (player[helmiter].used)
				break;			
		} while (runaway-- > 0);
		
		//atualiza tview
		for (int t=0;t<2;t++) {		// p/ cada time

			tview_bits[t] = 0;

			for (int i=0;i<MAX_PLAYERS;i++)			// p/ cada inimigo desse time
			if (i/8 == 1-t)
			if (player[i].used)
			{
				tview[t][i] = 0;		// default = nao visto

				for (int j=0;j<MAX_PLAYERS;j++)			// verifica se ele esta no campo de visao (tela) de alguem do meu time
				if (j/8 == t)
				if (player[j].used)
				{
					if ((player[j].x == player[i].x) && (player[j].y == player[i].y))	{

						//se o cara tem helm, nao aparece!!
						if (player[i].item_helm > 0)
						{
							//nao visto!
						}
						else						
						{
							//visto!
							tview[t][i%8] = 1;	//visto!
							tview_bits[t] += (1 << (i%8));		//seta bit de "visto"
							break;	
						}
					}
				}
			}

			//avanca tviter do time p/ escolher alguem
			int runaway = MAX_PLAYERS + 3;
			do {
				//avanca proximo candidato a envio
				tviter[t]++;
				if (tviter[t] < 0)
					tviter[t] = 0;
				if (tviter[t] >= MAX_PLAYERS)
					tviter[t] = 0;

				//testa se o candidato se aplica ao visor minimap do time
				//testa apenas used players
				if (player[tviter[t]].used) {

					//do meu time? envia, tenho q saber todos do meu time
					if (tviter[t]/8 == t)
						break;

					//inimigo? so se estiver na visao do time
					if (tviter[t]/8 == 1-t)
					if (tview[t][ tviter[t] % 8 ] == 1)
						break;
				}

			} while (runaway-- > 0);
		}

		for (i=0;i<MAX_PLAYERS;i++)
		if (player[i].used)			// REVISAR: todo client é um player used.  ? atualmente sim.... mas...
		{
			//rewrite past common data
			lecount = count;

			//player data field for each player ON SCREEN
			NLushort		players_onscreen = 0;

			//keep place for players_onscreen
			int p_on_count = lecount;
			writeShort(lebuf, lecount, 0);

			NLubyte keys;
			for (int j=0;j<MAX_PLAYERS;j++) 
			if ((players_present & (1 << j)) != 0)		//player j exists
			// j is on same screen than i (the viewer)
			// AND 
			//   ((j helm != 1)  ||  (j/8 == i/8))  // nao-totalmente invisivel OU e do mesmo time OU j com flag
			if (
					 (player[j].x == player[i].x)
					 && 
					 (player[j].y == player[i].y)
					 &&
					 (
					   (player[j].item_helm != 1) 
						 || 
						 (i/8 == j/8) 
					   || 
					   (
						   (world.flag[1-j/8].carried)
							 &&
							 (world.flag[1-j/8].carrier == j)
						 )
					 )
				 )
			{
				//add to players_onscreen
				players_onscreen += (1 << j);

				h = &(world.hero[j]);

				NLshort sho;

				sho = ((NLshort)player[j].x);
				writeShort(lebuf, lecount, sho);	//player.x (screen)

				sho = ((NLshort)player[j].y);
				writeShort(lebuf, lecount, sho);	//player.y (screen)

				sho = ((NLshort)h->x);
				writeShort(lebuf, lecount, sho);	//x

				sho = ((NLshort)h->y);
				writeShort(lebuf, lecount, sho);	//y

				sho = ((NLshort)(h->sx * 100));
				writeShort(lebuf, lecount, sho );	//sx  30.283482345634... = 30283 = 30.283(depois)

				sho = ((NLshort)(h->sy * 100));
				writeShort(lebuf, lecount, sho );	//sy

				/*
				EXTRA BYTE (ex- zframe)

	       bit 0 : player dead
				 bit 1 : health extra bit
				 bit 2 : energy extra bit
				*/
				NLubyte extra = 0;

				//deadflag
				if (player[j].health <= 0) extra += 1;

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
				keys = keys + (h->gundir * 32);
				
				//write keys
				writeByte(lebuf, lecount, keys);		

				//write SHIELD  BOOTS / QUAD
				NLubyte		itemflags = 0;
				if (player[j].item_shield)		itemflags += 1;
				if (player[j].item_speed)			itemflags += 2;
				if (player[j].item_quad)			itemflags += 4;

				writeByte(lebuf, lecount, itemflags);

				//write HELM alpha level
				writeByte(lebuf, lecount, ((NLubyte)player[j].item_helm));
			}

			//update players_onscreen (it's before the players on screen data (above))
			writeShort(lebuf, p_on_count, players_onscreen);

			//ELMO: visao alem do alcance!!
			NLubyte who;
			if (player[i].item_helm > 0) {
				
				//team "viewed enemies" do meu time (i/8)
				writeByte(lebuf, lecount, 255);		// todos!!!

				//"quem eu vou ficar sabendo no minimap agora?" -- do time
				who = (NLubyte)helmiter;
				writeByte(lebuf, lecount, who);
			}
			//sem elmo: visao normal
			else {
				
				//team "viewed enemies" do meu time (i/8)
				writeByte(lebuf, lecount, tview_bits[i/8]);

				//"quem eu vou ficar sabendo no minimap agora?" -- do time
				who = (NLubyte)tviter[i/8];
				writeByte(lebuf, lecount, who);
			}

			//x do cara, 0..255 (%) do mundo
			NLubyte mx = (NLubyte)(((world.hero[who].x + ((double)(player[who].x * plw))) / (WXMAX*plw)) * 255.0);
			writeByte(lebuf, lecount, mx);

			//y do cara, 0..255 (%) do mundo
			NLubyte my = (NLubyte)(((world.hero[who].y + ((double)(player[who].y * plh))) / (WYMAX*plh)) * 255.0);
			writeByte(lebuf, lecount, my);

			//send player's BASE health (first 8 bits)
			if (player[i].health < 0) player[i].health = 0;
			writeByte(lebuf, lecount, ((NLubyte)(player[i].health & 255)));

			//send player's BASE energy (first 8 bits)
			if (player[i].energy < 0) player[i].energy = 0;
			writeByte(lebuf, lecount, ((NLubyte)(player[i].energy & 255)));

			//extra byte of information
			// BIT 0: extra health
			// BIT 1: extra energy
			NLubyte xtra = 0;
			if (player[i].health & 256) xtra += 1;
			if (player[i].energy & 256) xtra += 2;
			writeByte(lebuf, lecount, xtra);

			//ping of player frame# % MAXPLAYERS
			NLushort theping = player[frame % MAX_PLAYERS].ping;
			writeShort(lebuf, lecount, theping);
			
			//send the packet
			server->send_frame(player[i].cid, lebuf, lecount);	//use client id of the player, and LEcount
		}

		
		// (PING) aproveita o timer e escolhe um jogador pra enviar um ping
		//
		if (++ping_send_counter >= 10) {
			ping_send_counter = 0;

			//choose a valid target
			int runaway = MAX_PLAYERS + 3;
			do {
				ping_send_client++;
				if (ping_send_client >= MAX_PLAYERS)
					ping_send_client = 0;

				// valid player
				if (player[ping_send_client].used) {
					//ping it
					server->ping_client(player[ping_send_client].cid);
				}
			} while (runaway-- > 0);
		}

	}

	//loop server
	// running_flag: pointer to bool, if this bool goes to false, the loop quits.
	void loop(volatile bool *running_flag) {
	
		frame = 0;	//frame to generate next

		//sync with speed counter until it's time to generate one frame (== 1)
		server_speed_counter = -3;
		while (server_speed_counter < 1)
			MS_SLEEP(1);		

		// no flag specified: esc quits
		bool keep_running = true;
		if (!running_flag)
			running_flag = &keep_running;

		static int fubie = 0;

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
				sprintf(elbuf, "%i/16 clients %.1fk/s v%s ESC=quit", player_count, server_kbps_traffic, GAME_VERSION);
				set_window_title(elbuf);
			}

			// sleep while not time to send again
			while (server_speed_counter <= 0) {
				
				// sleep a bit
				MS_SLEEP(1);
			}

			// quit? if no running-flag specified
			if (key[KEY_ESC])
				keep_running = false;
		}
	}

	//stop server
	void stop() {
		
		if (server)
			server->stop(3);
		else
			throw 8384;

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

	LOG1("hello client %i!\n", arg->client_id);
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
		else if (player_count >= MAX_PLAYERS) {		//server full!

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
		else {

			result.client_id = arg->client_id;		//this means "accept the connection"
			//custom data: hostname
			strcpy(lebuf, gameserver->get_hostname());
			result.data = &(lebuf[0]);
			result.length = strlen(lebuf)+1;	//inclui o zero
		}
	}
	
	return (int)(&result);
}

int sfunc_client_connected(runes_t *arg) {

	LOG1("client connected %i\n", arg->client_id);

	gameserver->client_connected(arg->client_id);
	
	return 0;
}

int sfunc_client_disconnected(runes_t *arg) {
	
	LOG1("client disconnected %i\n", arg->client_id);

	gameserver->client_disconnected(arg->client_id);
	
	return 0;
}

int sfunc_client_lag_status(runes_t *arg) {

	LOG2("client %i lagstatus %i\n", arg->client_id, arg->status);

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
//  listen server thread
//============================================================

int listen_port_running;
volatile bool	listen_server_running = false;
pthread_t	listen_server_thread;

void *thread_listenserver_f(void *arg) {

	//save for display
	listen_port_running = port;		//port selectr

	//(1) start the localserver
	//
	gameserver = new gameserver_c();
	if (!gameserver->start()) {
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
	set_window_title("Outgun client - CTRL+F12 to quit");

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
#define MAX_CLIENTFX 64

// size of connect screen
#define MAX_GAMESPY 20

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
};

// the game client state
// this includes graphics, simulation & networking (what a mess)
class gameclient_c {
public:

	// the current worldmap
	map_c		map;
	
	//all the players to show including me
	player_t player[MAX_PLAYERS];

	//explosion fx's
	clientfx_t		cfx[MAX_CLIENTFX];

	//wich player I am
	int	me;

	//the frame to be drawn , frame received from server (extrapolation basis)
	frame_t		fd, fx;

	//incoming framedata mutex
	pthread_mutex_t		frame_mutex;

	//time of last packet received
	double lastpackettime;

	//net client
	client_c *client;

	//audio samples
	SAMPLE *sample[NUM_OF_SAMPLES];

	//sound themes setting
	char sfxthemedir[256];
	char sfxthemename[256];
	al_ffblk	sfxthemeffblk;	//for al_find*
	bool	validtheme;		// if sfxthemedir points to valid dir
	
	//menu showing?
	bool menushow;
	int	 menu;		//menu screen #

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
	gamespy_t		gamespy[MAX_GAMESPY];	
	int					gi;	//what game entry

	char playername[256]; //the player's name (max name len = 16)
	char editplayername[256]; //the player's name edit buffer
	char address[256];		//server IP address
	char dialogmessage[256];	//dialog message
	char dialogmessage2[256];	//dialog message line 2
	char talkbuffer[256];			// chat input buffer
	char chatbuffer[CHAT_SIZE][256];		// last chat messages list
	double chaterasetime;				// time to erase a chat message from the list

	//bitmap for minimap background (level walls, screen grid)
	BITMAP *minibg;

	//start
	bool start() {

		//default physics parameters
		//set_default_physics();
		LOG3("\nNORMAL   fri %.1f acc %.1f mxs %.1f\n", svp_fric, svp_accel, svp_maxspeed);
		  LOG3("RUN      fri %.1f acc %.1f mxs %.1f\n", svp_fric_run, svp_accel_run, svp_maxspeed_run);
		  LOG3("TURBO    fri %.1f acc %.1f mxs %.1f\n", svp_fric_turbo, svp_accel_turbo, svp_maxspeed_turbo);
		  LOG3("TURBORUN fri %.1f acc %.1f mxs %.1f\n", svp_fric_turborun, svp_accel_turborun, svp_maxspeed_turborun);

		//hide helpscreen
		helpshow = false;

		totalframecount = 0;
		framecount = 0;
		
		// default map
		load_default_map(&map);

		//update minimap background
		minibg = create_bitmap(100, 100);
		update_minimap_background();

		//clear fx
		clear_fx();

		trying_connection = false;
		connected = false;

		client = new_client_c();
		client->set_callback(CFUNC_CONNECTION_UPDATE, cfunc_connection_update);
		client->set_callback(CFUNC_SERVER_DATA, cfunc_server_data);
		
		//init gamespy/adresses
		address[0]=0;
		gi = 0;
		for (int i=0;i<MAX_GAMESPY;i++) {
			gamespy[i].address[0]=0;
			gamespy[i].refreshed = false;
			gamespy[i].invalid = false;	//don't know the status yet
		}

		//try to load client configuration
		bool randomname = true; // give random name
		
		char dest[WHERE_PATH_SIZE];
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
					lebuf[15]=0;		// max needed for IP=15!
					strcpy(gamespy[i].address, lebuf);
				}
			}

			fclose(cfg);
		}
		
		//give a random name
		if (randomname) {
			string nome_tri_legal = RandomName();
			strcpy(playername, nome_tri_legal.c_str() );
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
			LOG2("res = %i   name = %i", result, sfxthemeffblk.name);
			if ((!strcmp(sfxthemeffblk.name, ".")) || (!strcmp(sfxthemeffblk.name, "..")))
				result = al_findnext(&sfxthemeffblk);
			LOG2("res = %i   name = %i", result, sfxthemeffblk.name);
			if ((!strcmp(sfxthemeffblk.name, ".")) || (!strcmp(sfxthemeffblk.name, "..")))
				result = al_findnext(&sfxthemeffblk);
			LOG2("res = %i   name = %i", result, sfxthemeffblk.name);

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

		return true;
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
			LOG2("res = %i   name = %i", result, sfxthemeffblk.name);
			if ((!strcmp(sfxthemeffblk.name, ".")) || (!strcmp(sfxthemeffblk.name, "..")))
				result = al_findnext(&sfxthemeffblk);
			LOG2("res = %i   name = %i", result, sfxthemeffblk.name);
			if ((!strcmp(sfxthemeffblk.name, ".")) || (!strcmp(sfxthemeffblk.name, "..")))
				result = al_findnext(&sfxthemeffblk);
			LOG2("res = %i   name = %i", result, sfxthemeffblk.name);

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
	SAMPLE *load_outgun_sample(char *fname) {

		//soundname: add "sound/" to the filename
		char soundname[256];
		strcpy(soundname, "sound");

		//additional: sfx theme dir name
		put_backslash(soundname);
		strcat(soundname, sfxthemedir);

		put_backslash(soundname);
		strcat(soundname, fname);

		//add soundname to where game dir
		char dest[WHERE_PATH_SIZE];
		append_filename(dest, wheregamedir, soundname, WHERE_PATH_SIZE);

		//try load
		SAMPLE *ret = load_sample(dest);

		LOG2("load_sample: '%s' = %i\n", dest, ret);

		return ret;	
	}

	//sample try loads
	void load_samples() {

		if (!sound_inited) return;

		sample[SAMPLE_FIRE] = load_outgun_sample("fire.wav");
		//sample[SAMPLE_FIRE_2] = load_outgun_sample("fire2.wav");
		//sample[SAMPLE_FIRE_3] = load_outgun_sample("fire3.wav");
		sample[SAMPLE_HIT] = load_outgun_sample("hit.wav");
		sample[SAMPLE_DEATH] = load_outgun_sample("death1.wav");
		sample[SAMPLE_DEATH_2] = load_outgun_sample("death2.wav");
		//sample[SAMPLE_RESPAWN] = load_outgun_sample("respawn.wav");
		sample[SAMPLE_ENTERGAME] = load_outgun_sample("entergam.wav");
		sample[SAMPLE_LEFTGAME] = load_outgun_sample("leftgam.wav");
		sample[SAMPLE_CHANGETEAM] = load_outgun_sample("chanteam.wav");
		sample[SAMPLE_TALK] = load_outgun_sample("talk.wav");
		sample[SAMPLE_WALLBOUNCE] = load_outgun_sample("wabounce.wav");
		
		sample[SAMPLE_WEAPON_UP] = load_outgun_sample("weaponup.wav");  //new
		sample[SAMPLE_MEGAHEALTH] = load_outgun_sample("megaheal.wav"); // new
		sample[SAMPLE_SHIELD_PICKUP] = load_outgun_sample("shieldp.wav");
		sample[SAMPLE_SHIELD_DAMAGE] = load_outgun_sample("shieldd.wav");
		sample[SAMPLE_SHIELD_LOST] = load_outgun_sample("shieldl.wav");
		sample[SAMPLE_BOOTS_ON] = load_outgun_sample("speedon.wav");
		sample[SAMPLE_BOOTS_OFF] = load_outgun_sample("speedoff.wav");
		sample[SAMPLE_QUAD_ON] = load_outgun_sample("quadon.wav");
		sample[SAMPLE_QUAD_FIRE] = load_outgun_sample("quadfire.wav");
		sample[SAMPLE_QUAD_OFF] = load_outgun_sample("quadoff.wav");
		sample[SAMPLE_HELM_ON] = load_outgun_sample("helmon.wav");
		sample[SAMPLE_HELM_OFF] = load_outgun_sample("helmoff.wav");
		
		sample[SAMPLE_CTF_GOT] = load_outgun_sample("got.wav");
		sample[SAMPLE_CTF_LOST] = load_outgun_sample("lost.wav");
		sample[SAMPLE_CTF_RETURN] = load_outgun_sample("return.wav");
		sample[SAMPLE_CTF_CAPTURE] = load_outgun_sample("capture.wav");
		sample[SAMPLE_CTF_GAMEOVER] = load_outgun_sample("gameover.wav");
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
		if (sample[s])
			play_sample(sample[s], 255, 127, 1000, false);
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
			scoreused[f] = 0;
			scoreboard[f] = -1;		
		}

		// fill each team
		for (int t=0;t<2;t++)	{

			//team delta
			int td = t*8;

			//itera do 1o ao 8o slot
			for (int s=td;s<8+td;s++) {

				//itera do 1o jogador ao 8o jogador do time
				//busca o maior que ainda nao foi usado
				int maxfrag = -666;
				int maxwho = -1;
				for (int i=td;i<8+td;i++)	
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
			hero_t  *h, *hx;
			for (i=0;i<MAX_PLAYERS;i++) 
			
			if (player[i].onscreen)		// nao eh suficiente usar platyer[i].USED !!!
																// tem que ser ONSCREEN !!!!
			{
				
				//copy all to fill in holes
				memcpy(&fd.hero[i], &fx.hero[i], sizeof(hero_t));

				h = &fd.hero[i];
				hx = &fx.hero[i];

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


				//select effective physics vars for the player
				//
				double player_accel;
				double player_friction;
				double player_maxspeed;
				if (h->run) {
					if (player[i].item_speed) {
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
					if (player[i].item_speed) {
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
/*
				//max speed: running?
				double max_speed_x, max_speed_y;
				if (h->run) {
					max_speed_x = sv_maxspeedrunx;
					max_speed_y = sv_maxspeedruny;
				}
				else {
					max_speed_x = sv_maxspeedx;
					max_speed_y = sv_maxspeedy;
				}
*/

				//flag carrier disadvantage when running
				if (h->run)
				if (fx.flag[1-(i/8)].carried)
				if (fx.flag[1-(i/8)].carrier == i)
					player_maxspeed -= svp_flag_penalty;

				//powerup: boots bonus topspeed
				/*
				double boots_accel_bonus = 1.0;
				if (player[i].item_speed) {
					max_speed_x *= sv_boots_top_bonus_x;
					max_speed_y *= sv_boots_top_bonus_y;
				}
				*/

				//powerup boots / run: bonus accel
				/*
				if (player[i].item_speed)
					boots_accel_bonus = sv_boots_accel_bonus;
				else if (h->run)
					boots_accel_bonus = sv_run_accel_bonus;
				*/

					//friction x - apply if l xor r
#ifndef ALWAYS_FRICTION
					if ( ((int)h->l + (int)h->r != 1) || (fabs(h->sx) > player_maxspeed) ) {
#endif
						if (h->sx > 0) {
							//h->sx -= sv_frictionx * boots_accel_bonus;
							h->sx -= player_friction * f;
							if (h->sx < 0) h->sx = 0;
						}
						else if (h->sx < 0) {
							//h->sx += sv_frictionx * boots_accel_bonus;
							h->sx += player_friction * f;
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
							h->sy -= player_friction * f;
							if (h->sy < 0) h->sy = 0;
						}
						else if (h->sy < 0) {
							//h->sy += sv_frictiony * boots_accel_bonus;
							h->sy += player_friction * f;
							if (h->sy > 0) h->sy = 0;
						}
#ifndef ALWAYS_FRICTION
					}
#endif
				
				/*
				//accelerate x if not over maximum speed
				if ((h->l) && (h->sx > -max_speed_x))
					h->sx -= sv_accelx * boots_accel_bonus;
				if ((h->r) && (h->sx < +max_speed_x))
					h->sx += sv_accelx * boots_accel_bonus;

				//accelerate y if not over maximum speed
				if ((h->u) && (h->sy > -max_speed_y))
					h->sy -= sv_accely * boots_accel_bonus;
				if ((h->d) && (h->sy < +max_speed_y))
					h->sy += sv_accely * boots_accel_bonus;
				*/
				//accelerate x if not over maximum speed
				if ((h->l) && (h->sx > -player_maxspeed))
					h->sx -= player_accel * f;
				if ((h->r) && (h->sx < +player_maxspeed))
					h->sx += player_accel * f;

				//accelerate y if not over maximum speed
				if ((h->u) && (h->sy > -player_maxspeed))
					h->sy -= player_accel * f;
				if ((h->d) && (h->sy < +player_maxspeed))
					h->sy += player_accel * f;

/*
					//max speed: running?
					double max_speed_x, max_speed_y;
					if (h->run) {
						max_speed_x = sv_maxspeedrunx;
						max_speed_y = sv_maxspeedruny;
					}
					else {
						max_speed_x = sv_maxspeedx;
						max_speed_y = sv_maxspeedy;
					}
					
					//flag carrier disadvantage
					if (h->run)
					if (fx.flag[1-(i/8)].carried)
					if (fx.flag[1-(i/8)].carrier == i) {
						max_speed_x -= sv_flag_penalty_x;
						max_speed_y -= sv_flag_penalty_y;
					}

					//powerup: boots bonus topspeed
					double boots_accel_bonus = 1.0;
					if (player[i].item_speed) {
						max_speed_x *= sv_boots_top_bonus_x;
						max_speed_y *= sv_boots_top_bonus_y;
					}

				  //powerup boots / run: bonus accel
					if (player[i].item_speed)
						boots_accel_bonus = sv_boots_accel_bonus;
					else if (h->run)
						boots_accel_bonus = sv_run_accel_bonus;

					//friction x - apply if l xor r
#ifndef ALWAYS_FRICTION
					if ( ((int)h->l + (int)h->r != 1) || (fabs(h->sx) > max_speed_x) ) {
#endif
						if (h->sx > 0) {
							h->sx -= sv_frictionx * boots_accel_bonus * f;
							if (h->sx < 0) h->sx = 0;
						}
						else if (h->sx < 0) {
							h->sx += sv_frictionx * boots_accel_bonus * f;
							if (h->sx > 0) h->sx = 0;
						}
#ifndef ALWAYS_FRICTION
					}
#endif

					//friction y
#ifndef ALWAYS_FRICTION
					if ( ((int)h->u + (int)h->d != 1) || (fabs(h->sy) > max_speed_y) ){
#endif
						if (h->sy > 0) {
							h->sy -= sv_frictiony * boots_accel_bonus * f;
							if (h->sy < 0) h->sy = 0;
						}
						else if (h->sy < 0) {
							h->sy += sv_frictiony * boots_accel_bonus * f;
							if (h->sy > 0) h->sy = 0;
						}
#ifndef ALWAYS_FRICTION
					}
#endif

				//accelerate x if not over maximum speed
				if ((h->l) && (h->sx > -max_speed_x))
					h->sx -= sv_accelx * boots_accel_bonus * f;
				if ((h->r) && (h->sx < +max_speed_x))
					h->sx += sv_accelx * boots_accel_bonus * f;

				//accelerate y if not over maximum speed
				if ((h->u) && (h->sy > -max_speed_y))
					h->sy -= sv_accely * boots_accel_bonus * f;
				if ((h->d) && (h->sy < +max_speed_y))
					h->sy += sv_accely * boots_accel_bonus * f;
*/
				
					//save ox,oy
					h->ox = h->x;
					h->oy = h->y;
				
				  //move x
					h->x += h->sx * f;
					if (h->x < 0) h->x = 0;
					else if (h->x > plw) h->x = plw;

					//move y
					h->y += h->sy * f;
					if (h->y < 0) h->y = 0;
					else if (h->y > plw) h->y = plw;

					//wall collision correction
					//wallcorrect(&map, &(h->x), &(h->y), h->sx, h->sy, player[i].x, player[i].y);
					//LOG("wc: ");
					if (wallcorrect(i, &map, &(h->x), &(h->y), &(h->sx), &(h->sy), &(h->ox), &(h->oy), player[i].x, player[i].y)) {
						
						//player bounced: play bounce sample if minimum time elapsed
						if (get_time() > player[i].wall_sound_time) {
							player[i].wall_sound_time = get_time() + 0.2;
							sound(SAMPLE_WALLBOUNCE);
						}
					}
					//LOG("ok; ");
					
				}
			}

			//rocket "interpolation"?
			// 
			for (i=0;i<MAX_ROCKETS;i++)
			if (fx.rock[i].owner != -1)
			{
				rocket_c *rd = &(fd.rock[i]);
				rocket_c *rx = &(fx.rock[i]);

				//find pos for draw
				// pos = startpos + sin/cos deg * timetravel * speed
				rd->x = (int)( rx->x + (fd.frame - rx->cl_time) * cos(rx->deg) * ROCKET_SPEED );
				rd->y = (int)( rx->y + (fd.frame - rx->cl_time) * sin(rx->deg) * ROCKET_SPEED );

				//SPECIAL CASE: check if rocket just died
				if (rx->hit_time > 0)
				if (get_time() > rx->hit_time)
				{
					rx->owner = -1;		// nao rola mais
					if (rx->hit_target != 255)  //if a player was hit
						player[rx->hit_target].hitfx = get_time() + 0.3;	// blink player

					//spawn clientside fx
					cfx_create_gunexplo((int)rd->x, ((int)rd->y) - 10, rx->px, rx->py);
				}

				// check out of screen (erase)
				if ((rd->x < -20) || (rd->y < -20) || (rd->x > plw + 20) || (rd->y > plh + 20)) {
					rx->owner = -1; // erase from clientside simulation
				}

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
			px = ((double)fx.flag[f].pos.px * (double)plw + fx.flag[f].pos.x) / ((double)plw * WXMAX);
			py = ((double)fx.flag[f].pos.py * (double)plh + fx.flag[f].pos.y) / ((double)plh * WYMAX);
			int pix = mmx + 21 + ((int)(px*98));
			int piy = mmy + 11 + ((int)(py*98));

			//draw mastro
			rectfill(drawbuf, pix, piy-5, pix, piy, col[COLYELLOW]);
			//draw bandeira
			rectfill(drawbuf, pix+1,piy-5,pix+5,piy-2,teamcol[f]);
	}

	//update the minimap background
	void update_minimap_background() {

		//black bg
		clear_to_color(minibg, 0);
		
		//draw screen boundaries
		int MMSCRW = (int)(98.0/((double)WXMAX));
		int MMSCRH = (int)(98.0/((double)WYMAX));
      int j;
		for (j=1;j<WXMAX;j++)
			line(minibg, 2+MMSCRW * j, 1, 2+MMSCRW * j, 100, col[COLSHADOW]);
		for (j=1;j<WYMAX;j++)
			line(minibg, 1, 2+MMSCRH * j, 100, 2+MMSCRH * j, col[COLSHADOW]);

		//draw solid walls
		int a,b,c,d;
		double maxx = plw*(WXMAX);
		double maxy = plh*(WYMAX);
		for (int x=0;x<WXMAX;x++)
		for (int y=0;y<WYMAX;y++)
		for (int w=0;w<WALLMAX;w++)
		if (map.room[x][y].wall[w].a != -1)
		{
			wall_c *wa = &(map.room[x][y].wall[w]);
			
			a = 1 + (int)( (((double)wa->a + x*plw) / maxx) * 98.0 );
			b = 1 + (int)( (((double)wa->b + y*plh) / maxy) * 98.0 );
			c = 1 + (int)( (((double)wa->c + x*plw) / maxx) * 98.0 );
			d = 1 + (int)( (((double)wa->d + y*plh) / maxy) * 98.0 );

			rectfill(minibg, a,b,c,d, makecol(0x00, 0x77, 0x00));
		}

		//green border
		rect(minibg, 0, 0, minibg->w -1, minibg->h -1, col[COLGREEN]);
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
		xg *= 0.7; yg *= 0.7;	//preguica de digitar valores
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
		ellipsefill(drawbuf, plx + x, ply + y - 15 - 0, 15, 15, pc1);
		// inner color: self color
		ellipsefill(drawbuf, plx + x, ply + y - 15 - 0, 10, 10, pc2);

		//desenha arma depois se dir 0,1,2,3,4
		if (gundir < 5) {
			line(drawbuf, 0+plx + x, 0+ply + y - 15, 0+plx + xg, 0+ply + yg, pc1);
			line(drawbuf, 1+plx + x, 0+ply + y - 15, 1+plx + xg, 0+ply + yg, pc1);
			line(drawbuf, 1+plx + x, 1+ply + y - 15, 1+plx + xg, 1+ply + yg, pc1);
		}

		if (alpha < 255)
			solid_mode();
	}

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
		if (me >= MAX_PLAYERS)
			return;

		//lock frame mutex
		//LOG1("locking HOW=%i",HOWMANY);
		pthread_mutex_lock( &frame_mutex );
		//LOG1("locked! HOW=%i",HOWMANY);

		//SUPER USELESS MODE   //if (rand() % 10 < 2) MS_SLEEP(30);
		//clear_to_color(drawbuf, makecol(rand(),0,0));	// clear buffer

		// game screen background		
		//
		clear_to_color(drawbuf, col[COLSHADOW]);	

		// the PLAY AREA: border, walls and pits
		//
		rectfill(drawbuf, plx, ply, plx + plw, ply + plh, col[COLGROUND]);		//ground

		if (player[me].x >= 0)
		if (player[me].y >= 0)
		if (player[me].x < WXMAX)
		if (player[me].y < WYMAX) 
		{
			room_c *r = &(map.room[player[me].x][player[me].y]);
			if (r) {
				for (int w=0;w<WALLMAX;w++) 
				if (r->wall[w].a != -1)	{
					rectfill(drawbuf, 
						plx + r->wall[w].a, 
						ply + r->wall[w].b, 
						plx + r->wall[w].c, 
						ply + r->wall[w].d, 
						col[COLWALL]
					);
				}
			}
		}

		// frame is valid?
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
					
					ellipsefill(drawbuf, plx + it->x, ply + it->y, 12, 12, col[COLGREEN]);
					
				}
				//boots
				else if (it->kind == 2) {

					ellipsefill(drawbuf, plx + it->x + rand()%6-3, ply + it->y + rand()%6-3, 12, 12, col[COLDARKORA]);
					ellipsefill(drawbuf, plx + it->x + rand()%8-4, ply + it->y + rand()%8-4, 12, 12, col[COLORA]);
					ellipsefill(drawbuf, plx + it->x + rand()%12-6, ply + it->y + rand()%12-6, 12, 12, col[COLYELLOW]);
				}
				//helm
				else if (it->kind == 3) {
					
					drawing_mode(DRAW_MODE_TRANS, 0,0,0);
					int alpha = ((int)(get_time() * 600.0)) % 400;
					if (alpha > 200)
						alpha = 400 - alpha;
					set_trans_blender(0,0,0,55+alpha);
					ellipsefill(drawbuf, plx + it->x, ply + it->y, 12, 12, col[COLMAG]);
					solid_mode();
				}
				//instant kill shots
				else if (it->kind == 4) {

					if (((int)(get_time() * 30)) % 2)
						ellipsefill(drawbuf, plx + it->x, ply + it->y, 13, 13, col[COLWHITE]);
					else
						ellipsefill(drawbuf, plx + it->x, ply + it->y, 11, 11, col[COLCYAN]);
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
						case 0: ellipsefill(drawbuf, plx + it->x + dx, ply + it->y + dy, 4, 4, col[COLGREEN]); break;
						case 1: ellipsefill(drawbuf, plx + it->x + dx, ply + it->y + dy, 4, 4, col[COLBLUE]); break;
						case 2: ellipsefill(drawbuf, plx + it->x + dx, ply + it->y + dy, 4, 4, col[COLRED]); break;
						case 3: ellipsefill(drawbuf, plx + it->x + dx, ply + it->y + dy, 4, 4, col[COLYELLOW]); break;
						}
					}
				}
				//megahealth
				else if (it->kind == 6) {

					//caixa de saude pulsante
					int varia = ((int)(get_time() * 15)) % 12;

					if (varia > 6)
						varia = 12 - varia;

					int itemsize = 11 + varia;
					int crossize = 8 + varia;
					int crosslar = 3;//aria/2;

					// health box black border
					rectfill(drawbuf, plx + it->x - itemsize - 2, ply + it->y - itemsize - 2,
												    plx + it->x + itemsize + 2, ply + it->y + itemsize + 2,
									 0);

					// health box
					rectfill(drawbuf, plx + it->x - itemsize, ply + it->y - itemsize,
												    plx + it->x + itemsize, ply + it->y + itemsize,
									 col[COLWHITE]);

					// red cross
					rectfill(drawbuf, plx + it->x - crossize, ply + it->y - crosslar,
						                plx + it->x + crossize, ply + it->y + crosslar,
														col[COLRED]);
					rectfill(drawbuf, plx + it->x - crosslar, ply + it->y - crossize,
						                plx + it->x + crosslar, ply + it->y + crossize,
														col[COLRED]);
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
			if (fx.rock[i].px == player[me].x)	//in same screen
			if (fx.rock[i].py == player[me].y)
			{
				rocket_c *r = &(fd.rock[i]);

				//QUAD ROCKET!
				if (player[ fx.rock[i].owner ].item_quad) {
					//draw rocket shadow
					ellipsefill(drawbuf, plx + r->x, ply + r->y, 6, 3, col[COLSHADOW]);
					//draw the rocket
					if (((int)(get_time() * 30)) % 2)
						ellipsefill(drawbuf, plx + r->x, ply + r->y - 15, 6, 6, col[COLWHITE]);	//y-12?
					else
						ellipsefill(drawbuf, plx + r->x, ply + r->y - 15, 4, 4, teamlcol[fx.rock[i].team]); //y-12??
				}
				else {
					//draw rocket shadow
					ellipsefill(drawbuf, plx + r->x, ply + r->y, 4, 2, col[COLSHADOW]);
					//draw the rocket
					ellipsefill(drawbuf, plx + r->x, ply + r->y - 15, 4, 4, teamcol[fx.rock[i].team]); //y-10??
				}
			}
			
			// sort order of drawing of the players 
			//
			for (i=0;i<MAX_PLAYERS;i++) {
				player[i].drawused = 0;
				player[i].drawptr = -1;
			}
			
			double miny; 
			int minyid;
			
			for (i=0;i<MAX_PLAYERS;i++) {
				minyid = -1;
				miny = 999999;
				
				for (int j=0;j<MAX_PLAYERS;j++) 
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
			for (int k=0;k<MAX_PLAYERS;k++) {

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
						if (i/8 == me/8)	// teammate or myself
						if (alpha < MIN_ALPHA_FRIENDS) alpha = MIN_ALPHA_FRIENDS;
						set_trans_blender(0,0,0,alpha);
					}
	
					// the player's shadow: showing last valid position
					ellipsefill(drawbuf, plx + fx.hero[i].x, ply + fx.hero[i].y, 15, 3, col[COLSHADOW]);

					if (player[i].item_helm > 0)
						solid_mode();

					// DRAW FLAG IF PLAYER IS CARRIER OF A FLAG
					for (int t=0; t<2; t++)
						if (fx.flag[t].carried == true)
						if (fx.flag[t].carrier == i)
						{
							draw_flag_at(drawbuf, t, fd.hero[i].x, fd.hero[i].y);
						}

					// se player morto, desenha poca de sangue
					if (player[i].dead) {

						// virou sorvete!
						if ((player[i].frags >= 10) && (player[i].frags % 10 == 0)) {
							ellipsefill(drawbuf, plx + fx.hero[i].x, ply + fx.hero[i].y - 15, 6, 15, col[COLORA]);
							ellipsefill(drawbuf, plx + fx.hero[i].x - 8, ply + fx.hero[i].y - 10-15, 8, 8, col[COLBLUE]);
							ellipsefill(drawbuf, plx + fx.hero[i].x + 8, ply + fx.hero[i].y - 10-15, 8, 8, col[COLMAG]);
							ellipsefill(drawbuf, plx + fx.hero[i].x + 0, ply + fx.hero[i].y - 20-15, 8, 8, col[COLGREEN]);
							textprintf_centre(drawbuf, font, plx + fx.hero[i].x + 0, ply + fx.hero[i].y - 20-43, col[COLWHITE], "VIROU");
							textprintf_centre(drawbuf, font, plx + fx.hero[i].x + 0, ply + fx.hero[i].y - 20-33, col[COLWHITE], "SORVETE!");
						}
						else {
							ellipsefill(drawbuf, plx + fx.hero[i].x, ply + fx.hero[i].y, 20, 6, col[COLRED]);
							ellipsefill(drawbuf, plx + fx.hero[i].x, ply + fx.hero[i].y - 10, 12, 12, col[COLRED]);
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
							cfx_create_speedfx(fx.hero[i].x, fx.hero[i].y, player[i].x, player[i].y, teamcol[i/8], col[i%8], fx.hero[i].gundir);
						}
						
						//blink player when hit
						//
						int pc1,pc2;	
						pc1 = teamcol[i/8];
						pc2 = col[i%8];
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
						draw_player(drawbuf, fd.hero[i].x, fd.hero[i].y, fd.hero[i].gundir, pc1, pc2, alpha);

						// SHIELD FX!!
						if (player[i].item_shield) {
							ellipse(drawbuf, plx + fd.hero[i].x, ply + fd.hero[i].y - 15, 24+rand()%3, 24+rand()%3, makecol(rand(),rand(),rand()));
							ellipse(drawbuf, plx + fd.hero[i].x, ply + fd.hero[i].y - 15, 24+rand()%5, 24+rand()%5, makecol(rand(),rand(),rand()));
							ellipse(drawbuf, plx + fd.hero[i].x, ply + fd.hero[i].y - 15, 24+rand()%9, 24+rand()%9, makecol(rand(),rand(),rand()));
						}
					}

				}
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
							ellipse(drawbuf, plx + cfx[i].x, ply + cfx[i].y, rad, rad, makecol(rand(),rand(),rand()));
						}
					}
				}
			}

		}

		// the MINIMAP
		//
		blit(minibg, drawbuf, 0, 0, mmx + 20, mmy + 10, minibg->w, minibg->h);	// minimap bg

		//draw the miniflags
		// - qualquer flag no chao (na base ou nao, carried == false)
		for (int f=0;f<2;f++)
		if (fx.flag[f].carried == false)
			draw_mini_flag(drawbuf, f);

		//room "touch" matrix - for fog-of-war
		bool roomvis[WXMAX][WYMAX];
		if (me >= 0)
		if (player[me].item_helm > 0)	// has helm invis visao alem do alcance thundercats?
			memset(roomvis, 1, WXMAX*WYMAX);	// sem fog! ve tudo!!
		else
			memset(roomvis, 0, WXMAX*WYMAX);  // tapa a fog

		// draw all teammates and enemies on screens where there are teammates
		//draw all the players - put a pixel where they are
		if (me >= 0)
		if (fx.frame >= 0)
		for (int i=0;i<MAX_PLAYERS;i++) 
		if (player[i].used)
		if (player[i].x >= 0)	//enforce valid player screen (needed for "touch room")
		if (player[i].y >= 0)
		if (player[i].x < WXMAX)
		if (player[i].y < WYMAX)
		if (
					(i/8 == me/8)		//show all teammates
					||
					(
						(i/8 != me/8)		//enemies
						&&
						((player[me].enemyvis & (1 << (i%8) )) != 0)		//a visible enemy
					)
			 )
		{
			//"touch" room (calculating fog of war)
			roomvis[player[i].x][player[i].y] = true;

			// coord on minimap 
			double px, py;
			px = ((double)player[i].x * (double)plw + fx.hero[i].x) / ((double)plw * WXMAX);
			py = ((double)player[i].y * (double)plh + fx.hero[i].y) / ((double)plh * WYMAX);
			int pix = mmx + 21 + ((int)(px*98));
			int piy = mmy + 11 + ((int)(py*98));

			//verifica se o jogador a ser desenhado é um carrier de flag inimiga 
			int enemyteam = 1-i/8;
			if (fx.flag[enemyteam].carried)
			if (fx.flag[enemyteam].carrier == i) {
				
				// update flag position for draw
				fx.flag[enemyteam].pos.px = player[i].x;
				fx.flag[enemyteam].pos.py = player[i].y;
				fx.flag[enemyteam].pos.x = fx.hero[i].x;
				fx.flag[enemyteam].pos.y = fx.hero[i].y;

				// draw the miniflag here
				draw_mini_flag(drawbuf, enemyteam);
			}

			if (i != me) {
				putpixel(drawbuf, pix+0, piy+0, teamcol[i/8]);	//3 pixel teamcol
				putpixel(drawbuf, pix+1, piy+0, teamcol[i/8]);	//3 pixel teamcol
				putpixel(drawbuf, pix+0, piy+1, teamcol[i/8]);	//3 pixel teamcol
				putpixel(drawbuf, pix+1, piy+1, col[i%8]);		// 1 pixel personal-color
			}
			else {
				//myself: draw differently
				if ( ((int) (get_time() * 15) ) % 3 > 0 ) {
					ellipsefill(drawbuf, pix, piy, 2, 2, col[COLYELLOW]);
					ellipsefill(drawbuf, pix, piy, 1, 1, teamlcol[i/8]);
				} else
					ellipsefill(drawbuf, pix, piy, 2, 2, 0);
			}
		}

		// paint fog of war in all invisible rooms
		//
		for (int ry=0;ry<WYMAX;ry++)
		for (int rx=0;rx<WXMAX;rx++) 
		if (roomvis[rx][ry] == false)	//not seeing map room[rx][ry]- paint fog
		{
			drawing_mode(DRAW_MODE_TRANS, 0,0,0);

			set_trans_blender(0,0,0,0x38);

			// FOG OF WAR!!!!!!!!
			int a,b,c,d;
			a = mmx+21 + rx*98/WXMAX;
			b = mmy+11 + ry*98/WYMAX;
			c = mmx+21 + (rx+1)*98/WXMAX-1;
			d = mmy+11 + (ry+1)*98/WYMAX-1;
			rectfill(drawbuf, a,b,c,d, col[COLFOGOFWAR]);

			solid_mode();
		}

		// the SCOREBOARD
		//
		if (key[KEY_TAB]) {
      textprintf(drawbuf, font, sbx + 4, sby + 2, teamlcol[0], "Red Team:   (PINGS)");
			textprintf(drawbuf, font, sbx + 4, sby + 2+ 10*12 -2, teamlcol[1], "Blue Team:  (PINGS)");
		}
		else {
			textprintf(drawbuf, font, sbx + 4, sby + 2, teamlcol[0], "Red Team:   %2i capt", fx.flag[0].score);
			textprintf(drawbuf, font, sbx + 4, sby + 2+ 10*12 -2, teamlcol[1], "Blue Team:  %2i capt", fx.flag[1].score);
		}
    
		int i;

		for (int dp=0;dp<MAX_PLAYERS;dp++)
		{
			// i = player #   dp = draw slot (0-7 = red team's  8-15 = blue team's)
			i = scoreboard[dp];

			//draw it
			if (i != -1)
			if (player[i].used)	
			{
				// print player name
				textprintf(drawbuf, font, sbx + 4, sby + 2 +13 + dp * 12 + (dp/8)*(26-2), col[i%8], "%s", player[i].name);
				
				// show ping or frags
				if (key[KEY_TAB]) {
					if (player[i].ping > 9999) player[i].ping = 9999;			//fix ping if too big
					textprintf(drawbuf, font, sbx + 4 + 15*8, sby + 2 +13 + dp * 12 + (dp/8)*(26-2), teamlcol[i/8], "%4i", player[i].ping);
				} 
				else
					textprintf(drawbuf, font, sbx + 4 + 15*8, sby + 2 +13 + dp * 12 + (dp/8)*(26-2), teamlcol[i/8], "%4i", player[i].frags);
			}
		}

		// the STATUSBAR : traffic
		//
		int bpsin = client->get_socket_stat(NL_AVE_BYTES_RECEIVED);
		int bpsout = client->get_socket_stat(NL_AVE_BYTES_SENT);
		int bpstraffic = bpsin + bpsout;
		textprintf(drawbuf, font, 72*8-2, ply+plh+  5, col[COLINFO], "BPS:%4i", bpstraffic);
		textprintf(drawbuf, font, 71*8-2, ply+plh+ 15, col[COLINFO], "%4i:%4i", bpsin, bpsout);

		//FPS
		textprintf(drawbuf, font, plx+10, ply+plh-14, 0, "FPS:%3.0f", FPS);

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
		}

		//server hostname
		textprintf(drawbuf, font, plx+6*8+334+(32-strlen_hostname)*8, ply+plh+25, col[COLINFO], "%s", hostname);
		
		//show "want change teams" flag if active
		if (want_change_teams) 
		{
			int c = col[COLWHITE];
			if ( ( (int) (get_time() * 2.0) ) % 2 )	// blink!
				c = col[COLRED];
			textprintf(drawbuf, font, plx + plw - 6*8 - 10,     ply + plh - 18, c, "CHANGE");
			textprintf(drawbuf, font, plx + plw - 6*8 - 10 + 4, ply + plh -  9, c, "TEAMS");
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
				else
					themsg = chatbuffer[i];	//don't discard 2 chars because there's no "@x" rpefix

				//colorful text
				textprintf(drawbuf, font, 3, 3+i*11, tcol, themsg);
			}
		}
		
		// the HUD: input text on "top" of message output
		//
		if (talkbuffer[0] != 0) {

			static char themsg[128];
			sprintf(themsg, "say: %s_", talkbuffer);
			
			//nice border
			textprintf(drawbuf, font, +1+3, +0+3+top*11, 0, themsg);
			textprintf(drawbuf, font, +1+3, +1+3+top*11, 0, themsg);
			textprintf(drawbuf, font, +0+3, +1+3+top*11, 0, themsg);
			textprintf(drawbuf, font, -1+3, +1+3+top*11, 0, themsg);
			textprintf(drawbuf, font, -1+3, +0+3+top*11, 0, themsg);
			textprintf(drawbuf, font, -1+3, -1+3+top*11, 0, themsg);
			textprintf(drawbuf, font, +0+3, -1+3+top*11, 0, themsg);
			textprintf(drawbuf, font, +1+3, -1+3+top*11, 0, themsg);

			// the prompt text
			textprintf(drawbuf, font, 3, 3+top*11, col[COLWHITE], themsg);
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
		textprintf(drawbuf, font, x+100, y+130, col[COLGREEN], "                http://www.inf.ufrgs.br/~fcecin/outgun");

		textprintf(drawbuf, font, x+100, y+160, col[COLWHITE], "MOVING YOUR CHARACTER:   ARROW KEYS = MOVE");
		textprintf(drawbuf, font, x+100, y+170, col[COLWHITE], "                         CONTROL    = SHOOT!");
		textprintf(drawbuf, font, x+100, y+180, col[COLWHITE], "                         ALT        = STRAFE");
		textprintf(drawbuf, font, x+100, y+190, col[COLWHITE], "                         SHIFT      = RUN");
		
		textprintf(drawbuf, font, x+100, y+210, col[COLWHITE], "TALKING TO ALL PLAYERS: Just type your message and hit ENTER");

		textprintf(drawbuf, font, x+100, y+230, col[COLWHITE], "TALKING JUST TO YOUR TEAM: Just place a dot ('.') at the very");
		textprintf(drawbuf, font, x+100, y+240, col[COLWHITE], " beginning of your message (first char)");

		textprintf(drawbuf, font, x+100, y+260, col[COLWHITE], "GAME CONCEPT: You are a member of a team, either RED or BLUE,");
		textprintf(drawbuf, font, x+100, y+270, col[COLWHITE], " assigned to you at random when you connect. Your goal is to");
		textprintf(drawbuf, font, x+100, y+280, col[COLWHITE], " help your team to win, by capturing eight (8) times the ememy");
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
			//small menu
			rect(drawbuf,  99,  69, 539, 409, col[COLMENUWHITE]);
			rect(drawbuf, 101, 71, 541, 411, col[COLMENUBLACK]);
			rectfill(drawbuf, 100, 70, 540, 410, col[COLMENUGRAY]);
			textprintf(drawbuf, font, 150, 120, col[COLWHITE], "Outgun         version %s", GAME_VERSION);
			textprintf(drawbuf, font, 150, 135, col[COLWHITE], "a L33TNET demonstration game");
		}
		else {
			//Big F Menu
			rect(drawbuf,  19,  19, 619, 459, col[COLMENUWHITE]);
			rect(drawbuf,  21,  21, 621, 461, col[COLMENUBLACK]);
			rectfill(drawbuf, 20, 20, 620, 460, col[COLMENUGRAY]);
			textprintf(drawbuf, font, 40, 40, col[COLWHITE], "Outgun         version %s", GAME_VERSION);
			textprintf(drawbuf, font, 40, 55, col[COLWHITE], "a L33TNET demonstration game");
		}



		if (menu == 0) {
			static int DELY = 10;

			textprintf(drawbuf, font, 150, 185-DELY, col[COLWHITE], "  [ 1 ]   Connect");
			textprintf(drawbuf, font, 150, 200-DELY, col[COLWHITE], "  [ 2 ]   Disconnect");
			if (connected)
				textprintf(drawbuf, font, 150+22*8, 200-DELY, col[COLGREEN], "(%s)", address);
			textprintf(drawbuf, font, 150, 215-DELY, col[COLWHITE], "  [ 3 ]   Change Player Name");
			textprintf(drawbuf, font, 150, 227-DELY, col[COLGREEN], "          '%s'", playername);
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

			
			textprintf(drawbuf, font, 50, 70, col[COLWHITE], "Type server's IP to connect.");
			textprintf(drawbuf, font, 50, 85, col[COLWHITE], "Select a slot using UP and DOWN arrows");
			textprintf(drawbuf, font, 50, 100, col[COLWHITE], "ENTER = OK     ESC = CANCEL    SPACE = REFRESH");
			textprintf(drawbuf, font, 50, 115, col[COLWHITE], "Enter empty address to connect to localhost");

			textprintf(drawbuf, font, 50, 135, col[COLWHITE], "IP Address        Ping #P Version/Hostname");

			char blinkchar[2];

			int xi,yi;
			for (int i=0;i<MAX_GAMESPY;i++) {

				xi = 50;
				yi = 150 + 00 + i*15;

				//selectr
				if (gi == i) {
					rectfill(drawbuf, xi-3,yi-3,xi+550,yi+12,col[COLSHADOW]);

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
				textprintf(drawbuf, font, xi, yi, col[COLGREEN], ":%s%s",gamespy[i].address, blinkchar);

				//draw gamespy entry
				if (!gamespy[i].refreshed) { // not refreshed
					//server info
					textprintf(drawbuf, font, xi + 18*8, yi, col[COLWHITE], "press SPACEBAR to refresh...");
				}
				else if (gamespy[i].invalid) {	//refreshed, invalid
					//server info
					textprintf(drawbuf, font, xi + 18*8, yi, col[COLWHITE], "---");
				}
				else if (gamespy[i].noresponse) {	//refreshed, no response
					//server info
					textprintf(drawbuf, font, xi + 18*8, yi, col[COLWHITE], "no response.");
				}
				else {  //refreshed, valid
					//server info
					textprintf(drawbuf, font, xi + 18*8, yi, col[COLGREEN], "%s", gamespy[i].info);
				}
			}
		}
		else if (menu == 2) {
			textprintf(drawbuf, font, 150, 230, col[COLWHITE], dialogmessage);
			textprintf(drawbuf, font, 150, 250, col[COLWHITE], dialogmessage2);
		}
		else if (menu == 3) {
			textprintf(drawbuf, font, 150, 220, col[COLWHITE], "Type your desired name.");
			textprintf(drawbuf, font, 150, 235, col[COLWHITE], "ENTER = OK     ESC = CANCEL");
			textprintf(drawbuf, font, 150, 290, col[COLGREEN], ":%s_", editplayername);
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

		//hide menu
		menushow = false;  // hide menu

		//hostname in connection-accepted packet. todo o "data" é a string do hostname
		strcpy(hostname, data);
		hostname[32]=0;		//truncate at 32 chars
		strlen_hostname = strlen(hostname);	//for drawing

		int i;

		//default scoreboard state
		for (i=0;i<MAX_PLAYERS;i++)	scoreboard[i]=i;

		//don't want to change teams by default
		want_change_teams = false;

		//avoid "dropped" plaque
		lastpackettime = get_time() + 1.0;

		// reset gamestate?
		connected = true;
		gameshow = true;		
		fx.frame = -1.0;		// no data
		fd.frame = -1.0;		// no data
		me = -1;	//don't know who am I

		//reset chat buffer
		talkbuffer[0]=0;
		for (i=0;i<CHAT_SIZE;i++)
			chatbuffer[i][0]=0;
		chaterasetime = get_time() + 10.0;

		//reset world data
		// players
      
		for (i=0;i<MAX_PLAYERS;i++) {
         memset(&(player[i]), 0, sizeof(player_t));
			player[i].ping = 0;
			player[i].id = i;
			strcpy(player[i].name, "(name unknown)");
			player[i].frags = 0;
			player[i].used = false;		// set to true/false on frame updates. if the positional data is there,
																//	then the player must be present. false otherwise.
		}

		//reset FPS count vars
		framecount = 0;
		starttime = get_time();
		FPS = 666.0;

		//send name update request
		char lebuf[256]; int count = 0;
		writeByte(lebuf, count, 1);		// "1" = client request name change
		playername[15] = 0;	//truncate player name, max 16 chars
		writeString(lebuf, count, playername);	// the name
		client->send_message(lebuf, count);
	}

	void client_disconnected() {

		// the gamestate?
		connected = false;
		gameshow = false;	
		
		// show a message
		strcpy(dialogmessage, "You have been disconnected. Press ESC");
		strcpy(dialogmessage2, "");
		menushow = true;
		set_menu(2);	//dialog menu
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
		int		 rc[MAX_GAMESPY];		//resposta count
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

				nlSetAddrPort(&gamespy[i].addr, port);//server PORT!!!!!!
				
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
            MS_SLEEP(5);			
   
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
			MS_SLEEP(5);

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
				daping = -666.0;
			
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
		strcpy(address, gamespy[gi].address);
		
		// start connecting to specified IP/port 
		// connection results will come through the CFUNC_CONNECTION_UPDATE callback
		char addr[256];
		if (strlen(address) == 0)	{ //empty address == my own ip
			NLaddress myadr;
			get_self_IP(&myadr);
			nlAddrToString(&myadr, address);
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
		sprintf(dialogmessage, "trying to connect... ESC=CANCEL", addr);
		sprintf(dialogmessage2, "");
		set_menu(2);	// dialog
	}

	//change name command
	void change_name_command() {
		//set new name, close menu
		strcpy(playername, editplayername);
		if (menushow)
			set_menu(0);

		//send reliable net message with the name
		if (connected) {
			char lebuf[256]; int count = 0;
			writeByte(lebuf, count, 1);		// "1" = client request name change
			playername[15] = 0;	//truncate player name, max 16 chars
			writeString(lebuf, count, playername);	// the name
			client->send_message(lebuf, count);
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
		rock->team = owner/8;
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
#ifdef OLD_DIRECTIONALS
		case 1:
			//sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, 0);	
			client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, 0);
			break;
		case 2:
			//sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, 0);	
			client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, - SHOT_DELTAX);
			client_set_rocket(rids[1], dir,frameno,owner,px,py,x,y, + SHOT_DELTAX);
			break;
		case 3:
			//sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, 0);	
			client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[1], dir+1,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[2], dir-1,frameno,owner,px,py,x,y, 0);
			break;
		case 4:
			//sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, 0);	
			client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, - SHOT_DELTAX);
			client_set_rocket(rids[1], dir,frameno,owner,px,py,x,y, + SHOT_DELTAX);
			client_set_rocket(rids[2], dir+1,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[3], dir-1,frameno,owner,px,py,x,y, 0);
			break;
		case 5:
			client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[1], dir+1,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[2], dir-1,frameno,owner,px,py,x,y, 0);
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
			client_set_rocket(rids[1], dir+1,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[2], dir-1,frameno,owner,px,py,x,y, 0);
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
			client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, - SHOT_DELTAX);
			client_set_rocket(rids[1], dir,frameno,owner,px,py,x,y, + SHOT_DELTAX);
			client_set_rocket(rids[2], dir+1,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[3], dir-1,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[4], dir+2,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[5], dir-2,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[6], dir+3,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[7], dir-3,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[8], dir-4,frameno,owner,px,py,x,y, 0);
			break;
#else
		case 1:
			//sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, 0);	
			client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, 0);
			break;
		case 2:
			//sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, 0);	
			client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, - SHOT_DELTAX);
			client_set_rocket(rids[1], dir,frameno,owner,px,py,x,y, + SHOT_DELTAX);
			break;
		case 3:
			//sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, 0);	
			client_set_rocket(rids[0], dir,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[1], dir+1,frameno,owner,px,py,x,y, 0);
			client_set_rocket(rids[2], dir-1,frameno,owner,px,py,x,y, 0);
			break;
		case 4:
			//sid[0] = game_do_shoot_rocket(playernum,px,py,x,y, cdeg, 0);	
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
#endif
		}
	}

	//process incoming data
	void process_incoming_data(char *data, int length) {

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

			NLushort	players_present;		//players present
			readShort(data, count, players_present);
         int i;
			for (i=0;i<MAX_PLAYERS;i++) {
				//decode players_present: sets if "player" record is used or not, in clientside
				if (players_present & (1 << i))
					player[i].used = true;
				else
					player[i].used = false;
			}

			//----- PLAYER SPECIFIC DATA -----

			NLushort	players_onscreen;		//players onscreen
			readShort(data, count, players_onscreen);
			
			//decode players_onscreen and update player data
			for (i=0;i<MAX_PLAYERS;i++) {
				
				//decode players_onscreen: sets if "player" record is there to be read
				if (players_onscreen & (1 << i))
					player[i].onscreen = true;
				else
					player[i].onscreen = false;

				//if player on screen, parse the data
				hero_t	*h;
				if (player[i].onscreen) {
					
					h = &(fx.hero[i]);
					
					NLshort sho;
					
					readShort(data, count, sho);		//player.x
					player[i].x = ((double)sho);
					readShort(data, count, sho);		//player.y
					player[i].y = ((double)sho);
					
					
					readShort(data, count, sho);		//x
					h->x = ((double)sho);
					readShort(data, count, sho);		//y
					h->y = ((double)sho);
					readShort(data, count, sho);		//sx
					h->sx = ((double)sho) / 100.0;
					readShort(data, count, sho);		//sy
					h->sy = ((double)sho) / 100.0;

					//FIXME: read / recalc z
					NLubyte byt, extra;
					readByte(data, count, extra);			//zframe
					h->z = 0;

					//remendo: (zframe == 255) ==  dead player
					//DEAD PLAYER = extra bit 0
					if (extra & 1)
						player[i].dead = true;
					else
						player[i].dead = false;

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
					readByte(data, count, byt);
					
					//bool oldquad = player[i].item_quad;
					//bool oldspeed = player[i].item_speed;
					//bool oldhelm = (player[i].item_helm != 0);

					player[i].item_shield =    ((byt & 1) != 0);
					player[i].item_speed =    ((byt & 2) != 0);
					player[i].item_quad =    ((byt & 4) != 0);

					//read helm byte
					readByte(data, count, byt);
					player[i].item_helm = byt;
				}
			}

			//read "enemies on team vislist"
			NLubyte eviz;
			readByte(data, count, eviz);
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
				player[who].x = whox / (255/WXMAX);	//screen = 0..255 / (WXMAX/255)
				player[who].y = whoy / (255/WYMAX);
				fx.hero[who].x = (whox % (255/WXMAX)) * plw / (255/WXMAX);	//posicao dentro da tela especifica
				fx.hero[who].y = (whoy % (255/WYMAX)) * plh / (255/WXMAX);
			}

			//read player's health and energy
			NLubyte healt, energ;
			readByte(data, count, healt);
			if (me >= 0)
				player[me].health = healt;

			readByte(data, count, energ);
			if (me >= 0)
				player[me].energy = energ;

			//extra byte of information
			// BIT 0: extra health
			// BIT 1: extra energy
			NLubyte xtra = 0;
			readByte(data, count, xtra);
			if (xtra & 1) player[me].health += 256;
			if (xtra & 2) player[me].energy += 256;

			//read ping of player frame % MAX_PLAYERS
			NLushort daping;
			readShort(data, count, daping);
			player[ svframe % MAX_PLAYERS ].ping = daping;
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
				//char lixox[256];
				int rids[16]; //rocket ids pra msg 7
				NLubyte rowner, rpx, rpy, code, pid, team, carried, abyte, rockid, timeleft, iid, rpow, rdir;
				NLushort	usho;
				NLulong frameno;
				NLfloat aflo;
				//rocket_c *rock;
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
					if (
						  (strlen(chatmsg) >= 2) 
							&&
							(chatmsg[0] == '@') 
							&& 
							(chatmsg[1] == 'I')
						 )
					{
						//don't play talk
					}
					else
						sound(SAMPLE_TALK);

					break;

				//"hello" one-time server information ("first packet")
				case 3:
					readByte(msg, count, pid);	//"who am I"
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

					LOG("after server tell physics...");
					LOG3("\nNORMAL   fri %.1f acc %.1f mxs %.1f\n", svp_fric, svp_accel, svp_maxspeed);
					LOG3("RUN      fri %.1f acc %.1f mxs %.1f\n", svp_fric_run, svp_accel_run, svp_maxspeed_run);
					LOG3("TURBO    fri %.1f acc %.1f mxs %.1f\n", svp_fric_turbo, svp_accel_turbo, svp_maxspeed_turbo);
					LOG3("TURBORUN fri %.1f acc %.1f mxs %.1f\n", svp_fric_turborun, svp_accel_turborun, svp_maxspeed_turborun);

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

						//char lix[256];
						//sprintf(lix, "flag %i dropped at %i %i %i %i", 
						//	team,
					  //	fx.flag[team].pos.px,
					  //		fx.flag[team].pos.py,
					  // 		fx.flag[team].pos.x,
						//	fx.flag[team].pos.y
						//);
						//print_message(lix);
					}
					else {
						fx.flag[team].carried = true;
						//carried: get carrier
						readByte(msg, count, abyte);	 //carrier
						fx.flag[team].carrier = abyte;	

						sound(SAMPLE_CTF_GOT);

						//char lix[256];
						//sprintf(lix, "flag %i carried by %i", 
						//							team,
						//							fx.flag[team].carrier
						//);
						//print_message(lix);
					}
					break;

				//rocket fire notification
				case 7:
				
					// add to clientside rocket objects list
					//
					//readByte(lebuf, count, rpowdir);	// rocket powerdir
					readByte(lebuf, count, rpow);	// rocket powerdir
					readByte(lebuf, count, rdir);	// rocket powerdir
					//rpow = rpowdir & 15;
					//rdir = rpowdir >> 4;

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
								player[rowner].quad_sound_finished = get_time() + 1.0;
							}
						}
					}
					break;

				//rocket deletion notification
				case 8:
					readByte(lebuf, count, rockid);	// rocket object id
					readByte(lebuf, count, abyte);	// target player
					readByte(lebuf, count, timeleft);	// frames left of animation

					//jura o rocket de morte
					fx.rock[rockid].hit_time = get_time() + 0.01 * timeleft;		// tempo de morte
					
					//sprintf(lixox, "tempo = %i  %f %f", timeleft, get_time(), fx.rock[rockid].hit_time);
					//print_message(lixox);

					fx.rock[rockid].hit_target = abyte;		// alvo
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
		if ((player[me].x != player[me].oldx) ||
		    (player[me].y != player[me].oldy))
		{
			//screen changed.

			//clear all clientside fx's
			for (int f=0;f<MAX_CLIENTFX;f++)
				cfx[f].used = false;

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
	void print_message(char *msg) {

		// find top
		int top = 0;
		if (strlen(msg) == 0)
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
		bool quick_fix, key_fire=false, key_kill=false, key_swap=false;
		char key_up=0, key_down=0, key_left=0, key_right=0;
		int i;
		while (notquit) {

			//LOG("** another loop()...\n");

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
							}
							switch (ch) {
							case '1': set_menu(1); break;
							case '2': disconnect_command(); break;
							case '3': strcpy(editplayername, playername); set_menu(3); break;
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
							i = strlen(gamespy[gi].address);
							if (
										(i < 16)	// max length of IP address typein
										&&
										(((ch >= '0') && (ch <= '9')) || (ch == '.')) 
								 )							
							{
								gamespy[gi].address[i] = ch;
								gamespy[gi].address[i+1] = 0;
								gamespy[gi].refreshed = false;
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
							}
							else if (sc == KEY_BACKSPACE) {
								if (i > 0) {
									gamespy[gi].address[i-1] = 0;
									gamespy[gi].refreshed = false;
								}
							}
							else if (sc == KEY_ENTER) {
								connect_command();
							}
							break;
						//dialog screen : just ESC
						case 2:
							break;
						//change name screen
						case 3:
							i = strlen(editplayername);
							if ((i < 15) && (((ch >= '0') && (ch <= '9')) || ((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z')) || (ch == '-') || (ch == '_'))) {
								editplayername[i] = ch;
								editplayername[i+1] = 0;
							}
							else if (sc == KEY_BACKSPACE) {
								if (i > 0)
									editplayername[i-1] = 0;
							}
							else if (sc == KEY_ENTER) {
								change_name_command();
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

					// l,r,u,d,fire game keys
					//
					if (
						   (key[KEY_UP]    != key_up)    ||
						   (key[KEY_DOWN]  != key_down)  ||
						   (key[KEY_LEFT]  != key_left)  ||
						   (key[KEY_RIGHT] != key_right)
						 )
					{
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
						send_frame();
						client_netsend_counter = 3;	//should be set to 0 but we want to send a new packet soon after this one
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

						//xuta cor
						if (sc == KEY_HOME) {
							if (key[KEY_LCONTROL]) {
								col[COLGROUND] = makecol(0x10, 0x40, 0);
								col[COLWALL] = makecol(0x30, 0xC0, 0);
							}
							else {
								col[COLGROUND] = makecol(rand(),rand(),rand());
								col[COLWALL] = makecol(rand(),rand(),rand());
							}
						}

						// ins == change name
						//
						if (sc == KEY_F10) {
							string lerud_abloxon = RandomName();
							strcpy(editplayername, lerud_abloxon.c_str());
							change_name_command();
						}
						// F11:screenshot
						else if (sc == KEY_F11) {
							save_screenshot();
						}
						//del key: delete
						else if (sc == KEY_DEL) {
							talkbuffer[0]=0;
						}
						//backspace erase one
						else if (sc == KEY_BACKSPACE) {
							if (i>0) talkbuffer[i-1] = 0;
						}
						//enter key: submit text
						else if (sc == KEY_ENTER) {
							if (i > 0) {
								send_chat(talkbuffer);
								talkbuffer[0]=0;
							}
						}
						//else text add keys. max text length = 60
						else if ((i < 60) && (ch >= 32) && (ch <= 127)) {
							talkbuffer[i] = ch;
							talkbuffer[i+1] = 0;
						}
					}
				}

				//ESC = show/hide menu, go back menu (special key)
				static bool kesc = false;
				if (key[KEY_ESC]) {
					if (!kesc) {
						kesc = true;
						if (strlen(talkbuffer) > 0) { // cancel chat
							talkbuffer[0]=0;
						}
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
					MS_SLEEP(1);

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
				textprintf(drawbuf, font, 0, 0, col[COLWHITE], "page-flipping = %i", page_flipping);
				textprintf(drawbuf, font, 0, 10, col[COLWHITE], "port = %i", port);
			}

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
				
				//DEBUG-panico
				/*
				if ((drawbuf != vidpage1) && (drawbuf != vidpage2)) {
					LOG("***** ERRAO ******* DRAWBUF != VIDPAGE1 or 2!!");
					return;
				}

				//

				//DEBUG-panico
				if (
					  (
							(vidpage1 != vidarr[0])	||
							(vidpage1 != vidarr[100])	||
							(vidpage1 != vidarr[200])	||
							(vidpage1 != vidarr[400])	||
							(vidpage1 != vidarr[900])	||
							(vidpage1 != vidarr[901])	||
							(vidpage1 != vidarr[907])	||
							(vidpage1 != vidarr[842])	||
							(vidpage1 != vidarr[1333])	||
							(vidpage1 != vidarr[1712])	||
							(vidpage1 != vidarr[2000])	||
							(vidpage1 != vidarr[1900])	||
							(vidpage1 != vidarr[1100])	||
							(vidpage1 != vidarr[555])	||
							(vidpage1 != vidarr[666])	||
							(vidpage1 != vidarr[777])
						)
						||
					  (
							(vidpage2 != vidarr[2099])	||
							(vidpage2 != vidarr[2100])	||
							(vidpage2 != vidarr[2200])	||
							(vidpage2 != vidarr[2340])	||
							(vidpage2 != vidarr[2410])	||
							(vidpage2 != vidarr[2777])	||
							(vidpage2 != vidarr[2778])	||
							(vidpage2 != vidarr[3107])	||
							(vidpage2 != vidarr[3999])	||
							(vidpage2 != vidarr[4000])	||
							(vidpage2 != vidarr[3333])	||
							(vidpage2 != vidarr[3446])	||
							(vidpage2 != vidarr[3666])	||
							(vidpage2 != vidarr[3668])	||
							(vidpage2 != vidarr[3234])	||
							(vidpage2 != vidarr[2556])
						)
						)
				{
					LOG("***** ERRAO ******* VIDPAGE1 ou 2 CORROMPIDAS!!");
					LOG2("VIDPAGE1 = %i     VIDPAGE2 = %i\n", vidpage1, vidpage2);

					for (int i=0;i<4096;i++)
					{
						LOG2("vidarr[%i] = %i\n", i, vidarr[i]);
					}

					return;
				}
				*/

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

		//clear all samples
		unload_samples();

		//save configuration file
		//try to load client configuration
		char dest[WHERE_PATH_SIZE];
		append_filename(dest, wheregamedir, "clconfig.txt", WHERE_PATH_SIZE);
		LOG1("dest for clconfig.txt OUT = %s\n", dest);

		FILE *cfg = fopen(dest, "w");
		if (cfg) {
			fprintf(cfg, "%s\n", sfxthemedir);
			fprintf(cfg, "%s\n", playername);
			for (int i=0;i<MAX_GAMESPY;i++)
				fprintf(cfg, "%s\n", gamespy[i].address);
			fclose(cfg);
		}

	}

	//ctor
	gameclient_c() {
		
		//net client
		client = 0;

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
	}

	//dtor
	virtual ~gameclient_c() {
		
		if (client) {
			delete client;
			client = 0;
		}

		pthread_mutex_destroy(&frame_mutex);
	}

};

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


//************************************************************
//  mainloop
//************************************************************

bool reset_video_mode() {

		char err1[1024];
		char err2[1024];
		char err3[1024];
		char err4[1024];

		//un-show any video bitmaps?
		show_video_bitmap(screen);
		
		//destroy all old surfaces
		if (vidpage1) { LOG("destroying vidpage1\n"); destroy_bitmap(vidpage1); vidpage1 = 0; }
		if (vidpage2) { LOG("destroying vidpage2\n"); destroy_bitmap(vidpage2); vidpage2 = 0; }
		if (backbuf) { LOG("destroying backbuf\n"); destroy_bitmap(backbuf); backbuf = 0; }

		int notok;
		
		set_color_depth(16); // hicolor
		if (winclient) // set mode
			notok = set_gfx_mode(GFX_AUTODETECT_WINDOWED, RESOL_X, RESOL_Y, 0, 0); 
		else
			notok = set_gfx_mode(GFX_AUTODETECT, RESOL_X, RESOL_Y, 0, 0);

		if (notok < 0) {
			LOG1("ERROR: cannot set 640x480x16 windowed?=%i graphics mode!\n", winclient);
			LOG1("Allegro error: '%s'\n", allegro_error);
			strcpy(err1, allegro_error);
					
			//try again...
			winclient = !winclient;
			set_color_depth(16); // hicolor
			if (winclient) // set mode
				notok = set_gfx_mode(GFX_AUTODETECT_WINDOWED, RESOL_X, RESOL_Y, 0, 0); 
			else
				notok = set_gfx_mode(GFX_AUTODETECT, RESOL_X, RESOL_Y, 0, 0);
	
			if (notok < 0) {
				LOG1("ERROR: cannot set 640x480x16 windowed?=%i graphics mode!\n", winclient);
				LOG1("Allegro error: '%s'\n", allegro_error);
				strcpy(err2, allegro_error);

				//try again...
				winclient = !winclient;
				set_color_depth(15); // ===> different color depth
				if (winclient) // set mode
					notok = set_gfx_mode(GFX_AUTODETECT_WINDOWED, RESOL_X, RESOL_Y, 0, 0); 
				else
					notok = set_gfx_mode(GFX_AUTODETECT, RESOL_X, RESOL_Y, 0, 0);

				if (notok < 0) {

					LOG1("ERROR: cannot set 640x480x15 windowed?=%i graphics mode!\n", winclient);
					LOG1("Allegro error: '%s'\n", allegro_error);
					strcpy(err3, allegro_error);

					//AND try again.....
					winclient = !winclient;
					set_color_depth(15); // ===> different color depth
					if (winclient) // set mode
						notok = set_gfx_mode(GFX_AUTODETECT_WINDOWED, RESOL_X, RESOL_Y, 0, 0); 
					else
						notok = set_gfx_mode(GFX_AUTODETECT, RESOL_X, RESOL_Y, 0, 0);

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
			LOG2("create vidpage1=%i vidpage2=%i\n", vidpage1, vidpage2); 
		}
		if ((!vidpage1) || (!vidpage2)) {
			
			if (trypageflip)
				LOG("PAGE FLIPPING: not enought video memory. using bruteforce doublebuffering\n")
			else
				LOG("USING SAFE MODE VIDEO -- DOUBLE-BUFFERING (option -dbuf). for page flipping use -flip\n")

			if (vidpage1) { destroy_bitmap(vidpage1); vidpage1 = 0; }
			if (vidpage2) { destroy_bitmap(vidpage2); vidpage2 = 0; }
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

		//DEBUG-panico
		/*
		for (int i=0;i<4096;i++)
			if ((i / 2048) == 1)
				vidarr[i] = vidpage2;
			else
				vidarr[i] = vidpage1;
		*/
			
		return true; //ok
}

int main(int argc, char *argv[]) {

	//random random
	srand((unsigned)time(0));

	strcpy(teamname[0], "RED");
	strcpy(teamname[1], "BLUE");

	// server parameters. speeds are in pixels / 0.1 sec
	//
	/*
	sv_frictionx = 1.5;
	sv_frictiony = 1.5;
	sv_accelx = 2.0;
	sv_accely = 2.0;
	sv_maxspeedx = 12.0;
	sv_maxspeedy = 12.0;
	sv_maxspeedrunx = 22.0;
	sv_maxspeedruny = 22.0;
	sv_flag_penalty_x = 3.0;	//run-with-flag penalty
	sv_flag_penalty_y = 3.0;
	sv_boots_top_bonus_x = 1.5;
	sv_boots_top_bonus_y = 1.5;
	sv_boots_accel_bonus = 2.0;
	sv_run_accel_bonus = 1.5;
	*/
	/*
	sv_frictionx = 1.5;
	sv_frictiony = 1.5;
	sv_accelx = 4.0;
	sv_accely = 4.0;
	sv_maxspeedx = 12.0;
	sv_maxspeedy = 12.0;
	sv_maxspeedrunx = 22.0;
	sv_maxspeedruny = 22.0;
	sv_flag_penalty_x = 3.0;	//run-with-flag penalty
	sv_flag_penalty_y = 3.0;
	sv_boots_top_bonus_x = 1.5;
	sv_boots_top_bonus_y = 1.5;
	sv_boots_accel_bonus = 1.5;
	sv_run_accel_bonus = 1.25;
	*/

	// general init
	//
	gameclient = 0;
	gameserver = 0;

	LOG_OPEN("outgun.log");
	
	allegro_init();
	install_keyboard();
	install_timer();

	LOG(argv[0]);

	// find out where we are
	//
	char *exespec = new char[2048];
	get_executable_name(exespec, 2048);
	LOG1("exespec = '%s'\n", exespec);
	replace_filename((char*)wheregamedir, (const char *)exespec, "", 256); //Replaces the specified path+filename with a new filename tail, storing at most size bytes into the dest buffer. Returns a copy of the dest parameter
	//put_backslash((char*)wheregamedir); //-nao poe isso porque transforma diretorio atual "" em raiz!
	LOG1("where = '%s'\n", wheregamedir);
	delete exespec; 
	exespec = 0;

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
		else if (!strcmp(argv[i], "-port")) {
			if (++i<argc) {
				port = strtol(argv[i], NULL, 10);
			}			
		}
		else if (!strcmp(argv[i], "-nosound"))
			nosound = true;
		else
			LOG2("WARNING: command-line argument #%i is unknown ('%s')\n", i, argv[i]);
	}

	// try install sound
	//
	if (!nosound) {
		if (install_sound(DIGI_AUTODETECT, MIDI_NONE, 0)) {
			LOG("INSTALL_SOUND failed. no sound.\n");
			sound_inited = false;
		}	else {
			LOG("INSTALL_SOUND ok.\n");
			sound_inited = true;
		}
	}
	else
		LOG("SOUND DISABLED by command line option -nosound\n");
	
	// here must get the safest and shittiest windowed gfx mode available
	//
	set_color_depth(8);
	if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 240, 0, 0)) {
		LOG("ERROR: could not set gfx mode 320x240 windowed");
		return 0;
	}
	
	// holding left shift on boot = switch to dedicated server
	// holding right control on boot = toggle winclient flag
	MS_SLEEP(500);
	if (key[KEY_LSHIFT])
		dedserver = true;
	if (key[KEY_LCONTROL])
		winclient = !winclient;
	if (key[KEY_ALT])
		trypageflip = !trypageflip;

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

	// run server
	//
	if (dedserver) {

		// FIXME/REVIEW: dedicated server - set process priority (all threads) to a higher value
		//		--> threads filhas estao com as priorities certas? LOGAR pra  ver. senao mudar p/ INHERIT
		int pmin = sched_get_priority_min(SCHED_OTHER);
    int pmax = sched_get_priority_max(SCHED_OTHER);
		int ptarg;
		//chutando: um "passo" abaixo do maximo
		if (pmin < pmax) { // pmin ..... OI! . PMAX
			ptarg = pmax - 1;
		}
		else {             // PMAX . OI! ..... pmin
			ptarg = pmax + 1;
		}
		pthread_t		tme = pthread_self();
		int					policy;
		sched_param param;
		int rc = pthread_getschedparam(tme, &policy, &param);
		LOG4("rc = %i policy = %i OTHER=%i sched_prio = %i\n", rc, policy, SCHED_OTHER, param.sched_priority);
		param.sched_priority = ptarg;
		policy = SCHED_OTHER;
		LOG3("pmin %i pmax %i ptarg = %i\n", pmin, pmax, ptarg);
		pthread_setschedparam(tme, policy, &param);

		// log 
		LOG_CLOSE();
		LOG_OPEN("out_svr.log");
		
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
		if (!gameserver->start()) {
			LOG("ERROR: cannot start gameserver!\n");
			return 0;
		}
		gameserver->loop(0);
		gameserver->stop();	// FIXME: this wasn't called before (review)
		delete gameserver;
		gameserver = 0;
	}
	// run client
	//
	else {

		set_window_title("Outgun client - CTRL+F12 to quit");

		// log 
		LOG_CLOSE();
		LOG_OPEN("out_cli.log");

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
		gameclient->stop();	// FIXME: when this was not called (gameclient deleted directly)
												//        the program GPF'ed. test if it's the deletion or not
												//				disconnecting before leaving the program that causes it.
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