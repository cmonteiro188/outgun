/*

	conalleg

	a console_c derivate for rendering on allegro bitmaps using the default allegro font

*/

//allegro includes
#include <allegro.h>			

//header include
#include "conalleg.h"

//your global console handle (probably you won't need more than one)
conalleg_c *con = 0;

//function for creating the global console handle. returns true of ok, false if console already created
bool create_console(int size) {
	if (con)
		return false;
	else {
		con = new conalleg_c(size);
		return true;
	}
}

//function for deleting the global console handle. if console already deleted, does nothing
void delete_console() {
	if (con) {
		delete con;
		con = 0;
	}
}