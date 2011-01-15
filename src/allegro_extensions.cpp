/*
 *  allegro_extensions.cpp
 *
 *  Copyright (C) 2003-2011 - Niko Ritari
 *  Copyright (C) 2003-2009 - Jani Rivinoja
 *
 *  This file is part of Outgun.
 *
 *  Outgun is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Outgun is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Outgun; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <cstdlib>

#include "incalleg.h"
#include "utility.h"

using std::max;
using std::min;

#if ALLEGRO_VERSION == 4 && ALLEGRO_SUB_VERSION == 0

void textprintf_ex(struct BITMAP* bmp, AL_CONST FONT *f, int x, int y, int color, int bg, AL_CONST char* format, ...) throw () {
    text_mode(bg);
    va_list argptr;
    char xbuf[16384];
    va_start(argptr, format);
    platVsnprintf(xbuf, 16384, format, argptr);
    va_end(argptr);
    textout(bmp, f, xbuf, x, y, color);
}

void textprintf_centre_ex(struct BITMAP* bmp, AL_CONST FONT *f, int x, int y, int color, int bg, AL_CONST char* format, ...) throw () {
    text_mode(bg);
    va_list argptr;
    char xbuf[16384];
    va_start(argptr, format);
    platVsnprintf(xbuf, 16384, format, argptr);
    va_end(argptr);
    textout_centre(bmp, f, xbuf, x, y, color);
}

void textprintf_right_ex(struct BITMAP* bmp, AL_CONST FONT *f, int x, int y, int color, int bg, AL_CONST char* format, ...) throw () {
    text_mode(bg);
    va_list argptr;
    char xbuf[16384];
    va_start(argptr, format);
    platVsnprintf(xbuf, 16384, format, argptr);
    va_end(argptr);
    textout_right(bmp, f, xbuf, x, y, color);
}

void textout_ex(struct BITMAP* bmp, AL_CONST FONT *f, AL_CONST char* text, int x, int y, int color, int bg) throw () {
    text_mode(bg);
    textout(bmp, f, text, x, y, color);
}

void textout_centre_ex(struct BITMAP* bmp, AL_CONST FONT *f, AL_CONST char* text, int x, int y, int color, int bg) throw () {
    text_mode(bg);
    textout_centre(bmp, f, text, x, y, color);
}

void textout_right_ex(struct BITMAP* bmp, AL_CONST FONT *f, AL_CONST char* text, int x, int y, int color, int bg) throw () {
    text_mode(bg);
    textout_right(bmp, f, text, x, y, color);
}

#endif // ALLEGRO_VERSION == 4 && ALLEGRO_SUB_VERSION == 0

void set_trans_mode(int alpha) throw () {
    nAssert(alpha >= 0 && alpha <= 255);
    if (alpha != 255) {
        set_trans_blender(0, 0, 0, alpha);
        drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
    }
    else
        solid_mode();
}

int makecolBounded(int r, int g, int b) throw () {
    return makecol(bound(r, 0, 255), bound(g, 0, 255), bound(b, 0, 255));
}

int set_gfx_mode_if_new(int depth, int card, int w, int h, int v_w, int v_h) throw () {
    static int oldDepth = -1, oldCard = -1, oldW = -1, oldH = -1, oldVw = -1, oldVh = -1;
    if (depth == oldDepth && card == oldCard && w == oldW && h == oldH && v_w == oldVw && v_h == oldVh)
        return 0;
    set_color_depth(depth);
    const int ret = set_gfx_mode(card, w, h, v_w, v_h);
    if (ret == 0) {
        oldDepth = depth;
        oldCard = card;
        oldW = w; oldH = h;
        oldVw = v_w; oldVh = v_h;
    }
    return ret;
}

void set_alpha_channel(BITMAP* bitmap, BITMAP* alpha) throw () {
    set_write_alpha_blender();
    drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
    if (alpha)
        draw_trans_sprite(bitmap, alpha, 0, 0);
    else    // maximum alpha level
        rectfill(bitmap, 0, 0, bitmap->w - 1, bitmap->h - 1, 255);
    solid_mode();
}

void rotate_trans_sprite(BITMAP* bmp, BITMAP* sprite, int x, int y, fixed angle, int alpha) throw () { // x,y are destination coords of the sprite center
    nAssert(sprite);
    nAssert(sprite->w == sprite->h);    // if otherwise, would have to use max(sprite->w, sprite->h) below, and use more complex coords in rotate
    // make room so that rotating won't clip the corners off
    const int size = sprite->h + sprite->h / 2;
    Bitmap buffer = create_bitmap(size, size);
    nAssert(buffer);
    clear_to_color(buffer, bitmap_mask_color(buffer));
    rotate_sprite(buffer, sprite, sprite->h / 4, sprite->h / 4, angle);
    set_trans_mode(alpha);
    draw_trans_sprite(bmp, buffer, x - buffer->w / 2, y - buffer->h / 2);
    solid_mode();
}

void rotate_alpha_sprite(BITMAP* bmp, BITMAP* sprite, int x, int y, fixed angle) throw () {    // x,y are destination coords of the sprite center
    nAssert(bitmap_color_depth(sprite) == 32);
    nAssert(sprite->w == sprite->h);    // if otherwise, would have to use max(sprite->w, sprite->h) below, and use more complex coords in rotate
    // make room so that rotating won't clip the corners off
    const int size = sprite->h + sprite->h / 2;
    Bitmap buffer = create_bitmap_ex(32, size, size);
    nAssert(buffer);
    clear_to_color(buffer, bitmap_mask_color(buffer));
    rotate_sprite(buffer, sprite, sprite->h / 4, sprite->h / 4, angle);
    drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
    set_alpha_blender();
    draw_trans_sprite(bmp, buffer, x - buffer->w / 2, y - buffer->h / 2);
    solid_mode();
}

int colorTo32(int color) throw () {
    return makecol32(getr(color), getg(color), getb(color));
}

void tileBlit(BITMAP* target, int x1, int y1, int x2, int y2, BITMAP* tex) throw () {
    ++x2; ++y2; // makes calculating widths easier
    const int txs = x1 % tex->w; int ws = tex->w - txs, we = x2 % tex->w, xe = x2 - we;
    const int tys = y1 % tex->h; int hs = tex->h - tys, he = y2 % tex->h, ye = y2 - he;
    if (x1 + ws >= x2) {
        ws = x2 - x1;
        xe = x1;
    }
    if (y1 + hs >= y2) {
        hs = y2 - y1;
        ye = y1;
    }
    for (int y = y1;;) {
        const int h = y == y1 ? hs : y == ye ? he : tex->h;
        const int ty = y % tex->h;
        blit(tex, target, txs, ty, x1, y, ws, h);
        for (int x = x1 + ws; x < xe; x += tex->w)
            blit(tex, target, 0, ty, x, y, tex->w, h);
        if (we)
            blit(tex, target, 0, ty, xe, y, we, h);
        if (y == ye)
            break;
        y += h;
    }
}

TemporaryClipRect::TemporaryClipRect(BITMAP* bitmap, int x1_, int y1_, int x2_, int y2_, bool respectOld) throw () : b(bitmap) {
    get_clip_rect(b, &x1, &y1, &x2, &y2);
    if (respectOld) {
        x1_ = max(x1_, x1);
        y1_ = max(y1_, y1);
        x2_ = min(x2_, x2);
        y2_ = min(y2_, y2);
    }
    set_clip_rect(b, x1_, y1_, x2_, y2_);
}

TemporaryClipRect::~TemporaryClipRect() throw () {
    set_clip_rect(b, x1, y1, x2, y2);
}
