#ifndef INCALLEG_H_INC
#define INCALLEG_H_INC

#include <allegro.h>
#ifdef ALLEGRO_WINDOWS
#include <winalleg.h>
#endif

#if ALLEGRO_VERSION == 4 && ALLEGRO_SUB_VERSION == 0
// these are implemented in utility.cpp
void textprintf_ex(struct BITMAP* bmp, AL_CONST FONT *f, int x, int y, int color, int bg, AL_CONST char* format, ...);
void textprintf_centre_ex(struct BITMAP* bmp, AL_CONST FONT *f, int x, int y, int color, int bg, AL_CONST char* format, ...);
void textprintf_right_ex(struct BITMAP* bmp, AL_CONST FONT *f, int x, int y, int color, int bg, AL_CONST char* format, ...);
void textout_ex(struct BITMAP* bmp, AL_CONST FONT *f, AL_CONST char* text, int x, int y, int color, int bg);
void textout_centre_ex(struct BITMAP* bmp, AL_CONST FONT *f, AL_CONST char* text, int x, int y, int color, int bg);
void textout_right_ex(struct BITMAP* bmp, AL_CONST FONT *f, AL_CONST char* text, int x, int y, int color, int bg);

inline void set_close_button_callback(void (*fn)()) {
	set_window_close_hook(fn);
}
#endif	// ALLEGRO_VERSION == 4 && ALLEGRO_SUB_VERSION == 0

#if ALLEGRO_VERSION == 4 && (ALLEGRO_SUB_VERSION == 0 || (ALLEGRO_SUB_VERSION == 1 && ALLEGRO_WIP_VERSION <= 11))
#define set_clip_rect set_clip
#endif

#endif	// INCALLEG_H_INC
