/*

	conalleg

	a console_c derivate for rendering on allegro bitmaps using the default allegro font

*/

#ifndef _CONALLEG_H_
#define _CONALLEG_H_

//zig engine base console
#include <console.h>

//your global console handle (probably you won't need more than one)
class conalleg_c;
extern conalleg_c *con;

//function for creating the global console handle. returns true of ok, false if console already created
//size: size of console, in kilobytes
bool create_console(int size = 64);

//function for deleting the global console handle. if console already deleted, does nothing
void delete_console();

//allegro console class
class conalleg_c : public console_c {
public:

	//ctor - passa bitmap: colunas e linhas do console expandem para ocupar todo o bitmap!
	// se quiser passar uma parte da tela apenas, fornecer um sub-bitmap.
	conalleg_c(BITMAP *b, int size) : console_c(size, (b->w-20)/8, (b->h-20)/10) {
		set_bitmap(b);
	}

	//ctor sem bitmap : vai precisar passar o bitmap depois
	conalleg_c(int size) : console_c(size) {
		set_bitmap(0);
	}

	// muda o bitmap para desenhos. 0 = nenhum
	void set_bitmap(BITMAP *b) {
		bmp = b;
		if (bmp) {
			
			// *** CHANGE THIS TO ANY COLORS YOU LIKE BETTER ***
			bcol = makecol(50,50,50);
			tcol = makecol(255,255,255);
			pcol = makecol(0,255,0);

			text_mode(-1);

			enable_display((bmp->w - 20) / 8, (bmp->h - 20) / 10);

			draw_page(true);
		}
		else
			disable_display();
	}

	//JUST SET bitmap (for pageflipping)
	void just_set_bitmap(BITMAP *b) {
		bmp = b;
	}

	// read allegro keyboard
	void read_keyboard() {
		while (keypressed()) {

			int key = readkey();
			int ascii = key & 0xff;
			key = key >> 8;
			
			if (key == KEY_PGUP)
				con->scroll_page_up();
			else if (key == KEY_PGDN)
				con->scroll_page_down();
			else if (key == KEY_END)
				con->scroll_all_down();
			else
				con->read_char(ascii);

			//printf("ascii = %i key = %i\n", ascii, key);
		}

	}

protected:

	//bgcolor / text color for drawing
	int bcol, tcol, pcol;	

	// console command-line processing
	//	cmdstr: command line typed by console user (or maybe some automated script)
	virtual void interprete_command(char* cmdstr);

	// render a console line. line: line offset onscreen. buf: line of text.
	virtual void draw_line(int line, char *buf) {
		int col = tcol;
		if (line >= conRows - 1) 
			col = pcol;
		textprintf(bmp, font, 10, 10 + line * 10, col, buf);
	}

  // called when console must be redrawn.
	virtual void clear() { 
		clear_to_color(bmp, bcol);
	}

  // called when console input must be redrawn.
	virtual void clear_prompt() { 
		rectfill(bmp, 10, conRows * 10, 10 + conCols * 8, conRows * 10 + 8, bcol);
	}

	// bitmap to draw
	BITMAP *bmp;
};

#endif