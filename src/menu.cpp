/*
 *  menu.cpp
 *
 *  Copyright (C) 2004 - Niko Ritari
 *  Copyright (C) 2004 - Jani Rivinoja
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

#include <algorithm>
#include <cmath>

#include "incalleg.h"
#include "client.h"
#include "nassert.h"

#include "menu.h"

using std::max;
using std::min;
using std::string;
using std::swap;
using std::vector;

// character width and line height in pixels
const int char_w = 8;
const int line_h = 16;

int col_background, col_borderShadow, col_borderHighlight, col_menuCaption, col_menuCaptionBg, col_caption,
	col_active, col_disabled, col_value, col_scrollbar, col_scrollbarBg, col_shortcutDisabled, col_shortcutEnabled;

void scrollbar(BITMAP* buffer, int x, int y, int height, int bar_y, int bar_h, int col1, int col2) {
	const int width = 10;
	if (height > 0) {
		rectfill(buffer, x, y, x + width - 1, y + height - 1, col2);
		if (bar_h > 0)
			rectfill(buffer, x, y + bar_y, x + width - 1, y + bar_y + bar_h - 1, col1);
	}
}

void drawKeySymbol(BITMAP* buffer, int x, int y, const string& text) {
	int width = text.length() * char_w;
	hline(buffer, x - 4, y - 1, x + width + 3, makecol(0xEE, 0xEE, 0xEE));
	vline(buffer, x - 4, y - 1, y + 8        , makecol(0xEE, 0xEE, 0xEE));
	hline(buffer, x - 4, y + 8, x + width + 3, makecol(0x22, 0x22, 0x22));
	vline(buffer, x + width + 3, y - 1, y + 8, makecol(0x22, 0x22, 0x22));
	rectfill(buffer, x - 3, y, x + width + 2, y + 7, makecol(0x99, 0x99, 0x99));
	textout_ex(buffer, font, text.c_str(), x, y, 0, -1);
}

int Component::captionColor(bool active) const {
	if (!isEnabled())
		return col_disabled;
	if (active)
		return col_active;
	return col_caption;
}

void Menu::home() {
	nAssert(!components.empty());
	start = 0;
	for (selected_item = 0; !components[selected_item]->isEnabled(); ++selected_item)
		if (selected_item >= static_cast<int>(components.size()) - 1) {
			selected_item = 0;	// maybe this isn't needed
			return;
		}
}

void Menu::end() {
	nAssert(!components.empty());
	for (selected_item = static_cast<int>(components.size()) - 1; !components[selected_item]->isEnabled(); --selected_item)
		if (selected_item == 0)
			break;
}

bool Menu::prev() {
	nAssert(!components.empty());
	const int original = selected_item;
	do {
		--selected_item;
		if (selected_item < 0) {
			selected_item = original;
			--start;	// allow seeing unselectable components
			return false;
		}
	} while (!components[selected_item]->isEnabled());
	return true;
}

bool Menu::next() {
	nAssert(!components.empty());
	const int original = selected_item;
	do {
		++selected_item;
		if (selected_item >= static_cast<int>(components.size())) {
			selected_item = original;
			++start;	// allow seeing unselectable components
			return false;
		}
	} while (!components[selected_item]->isEnabled());
	return true;
}

void Menu::setSelection(int selection) {
	nAssert(selection >= 0);
	if (selection >= (int)components.size())
		selected_item = components.size() - 1;
	else
		selected_item = selection;
}

void Menu::draw(BITMAP* buffer) {
	//#fix: handle colors and other drawing details with a separate class connected with Graphics
	// colors are initialized in every draw because they must be initialized after every color depth change
	col_background		= makecol(0x30, 0x40, 0x30);
	col_borderShadow	= makecol(0x50, 0x60, 0x50);
	col_borderHighlight	= makecol(0xA0, 0xB0, 0xA0);
	col_menuCaption 	= makecol(0xFF, 0xFF, 0xFF);
	col_menuCaptionBg	= makecol(0x00, 0x77, 0x00);
	col_caption			= makecol(0x40, 0xFF, 0x40);
	col_active			= makecol(0xFF, 0xFF, 0x00);
	col_disabled		= makecol(0x00, 0xAA, 0x00);
	col_value			= makecol(0xFF, 0xFF, 0xFF);
	col_scrollbar		= makecol(0x00, 0xFF, 0x00);
	col_scrollbarBg		= makecol(0x00, 0x77, 0x00);
	col_shortcutDisabled= makecol(0x50, 0x60, 0x50);
	col_shortcutEnabled	= makecol(0xB0, 0xD0, 0xB0);

	drawHook.call(*this);

	nAssert(!components.empty());
	nAssert(selected_item >= 0 && selected_item < static_cast<int>(components.size()));

	if (!components.empty() && !components[selected_item]->isEnabled())	// a disabled component can not be active
		if (!next())	// try moving down first - feels intuitive
			prev();	// if this fails too, so be it

	// calculate menu width and height
	const int padding = 30;
	const int max_width = buffer->w - 2;	// 2 pixels for borders
	const int min_width = total_width() + 2 * padding;
	const int max_height = buffer->h - 2;	// 2 pixels for borders
	const int min_height = total_height() + 2 * padding;
	const int w = min(max_width, min_width);
	const int h = min(max_height, min_height);
	const int mx = max_width / 2;
	const int my = max_height / 2;
	const int x1 = mx - w / 2;
	const int y1 = my - h / 2;
	const int x2 = mx + w / 2;
	const int y2 = my + h / 2;

	rectfill(buffer, x1, y1, x2, y2, col_background);
	hline(buffer, x1 - 1, y1 - 1, x2 + 1, col_borderHighlight);
	vline(buffer, x1 - 1, y1 - 1, y2 + 1, col_borderHighlight);
	hline(buffer, x1 - 1, y2 + 1, x2 + 1, col_borderShadow);
	vline(buffer, x2 + 1, y1 - 1, y2 + 1, col_borderShadow);

	const int x_start = x1 + padding;
	int y = y1 + padding;

	// draw caption
	rectfill(buffer, x1, y + 4 - line_h / 2, x2, y + 4 + line_h / 2, col_menuCaptionBg);
	textout_centre_ex(buffer, font, caption.c_str(), mx, y, col_menuCaption, -1);
	y += 2 * line_h;

	const int scrollbar_start_y = y;

	// draw components and calculate visible items
	if (h == min_height)	// everything fits
		start = 0;
	else {
		if (selected_item < start)
			start = selected_item;
		if (selected_item >= start + visible_items)
			start = selected_item - visible_items + 1;
		if (start >= static_cast<int>(components.size()) - visible_items)
			start = components.size() - visible_items;
		if (start < 0)
			start = 0;
	}
	visible_items = 0;
	int selecti = 1;
	for (int ci = 0; ci < start; ++ci)	// find the initial selecti for next loop at compi = start
		if (components[ci]->canBeEnabled())
			++selecti;
	const int shortcutColor = components[selected_item]->needsNumberKeys() ? col_shortcutDisabled : col_shortcutEnabled;
	for (int compi = start; compi < static_cast<int>(components.size()); ++compi) {
		Component* component = components[compi];
		if (y + component->minHeight() > y2 - padding)
			break;

		if (components[compi]->canBeEnabled()) {
			if (selecti <= 10 && shortcuts)
				textprintf_right_ex(buffer, font, x_start - char_w, y, shortcutColor, -1, "%d", selecti % 10);
			++selecti;
		}

		const int h = min(component->height(), y2 - padding - y);
		component->draw(buffer, x_start, y, h, compi == selected_item);
		y += component->height();
		visible_items++;
	}

	// draw scrollbar if everything didn't fit
	if (visible_items != static_cast<int>(components.size())) {
		const int x = x2 - padding + char_w;
		const int y = scrollbar_start_y;
		const int height = visible_items * line_h;
		const int bar_y = static_cast<int>(static_cast<double>(height * start) / components.size() + 0.5);
		const int bar_h = static_cast<int>(static_cast<double>(height * visible_items) / components.size() + 0.5);
		scrollbar(buffer, x, y, height, bar_y, bar_h, col_scrollbar, col_scrollbarBg);
	}
}

void Menu::handleKeypress(char scan, unsigned char chr) {
	nAssert(!components.empty());
	nAssert(selected_item >= 0 && selected_item < static_cast<int>(components.size()));
	if (scan == KEY_UP || (scan == KEY_TAB && (key[KEY_LSHIFT] || key[KEY_RSHIFT])))
		prev();
	else if (scan == KEY_DOWN || scan == KEY_TAB)
		next();
	else if (scan == KEY_HOME)
		home();
	else if (scan == KEY_END)
		end();
	else if (shortcuts && ((isdigit(chr) && !components[selected_item]->needsNumberKeys()) || chr == 0)) {	// check for number, and Alt + number
		int shortcut;
		if (chr == 0)	// with alt
			switch (scan) {
				case KEY_1: shortcut = 0; break;
				case KEY_2: shortcut = 1; break;
				case KEY_3: shortcut = 2; break;
				case KEY_4: shortcut = 3; break;
				case KEY_5: shortcut = 4; break;
				case KEY_6: shortcut = 5; break;
				case KEY_7: shortcut = 6; break;
				case KEY_8: shortcut = 7; break;
				case KEY_9: shortcut = 8; break;
				case KEY_0: shortcut = 9; break;
				default: shortcut = -1; break;
			}
		else if (chr == '0')
			shortcut = 9;
		else
			shortcut = chr - '1';	// relies on "123456789" being sequential and in that order (as in ASCII)
		if (shortcut != -1) {
			bool found = false;
			int newsel;
			for (newsel = 0; newsel < (int)components.size(); ++newsel)
				if (components[newsel]->canBeEnabled())
					if (shortcut-- == 0) {
						found = true;
						break;
					}
			if (found && components[newsel]->isEnabled()) {
				selected_item = newsel;
				components[selected_item]->shortcutActivated();
				return;
			}
		}
	}
	nAssert(selected_item >= 0 && selected_item < static_cast<int>(components.size()));
	if (components[selected_item]->isEnabled() && components[selected_item]->handleKey(scan, chr))
		return;
	if (scan == KEY_ENTER || scan == KEY_ENTER_PAD)
		okHook.call(*this);
}

int Menu::width() const {
	return caption.length() * char_w;
}

int Menu::height() const {
	return line_h;
}

void Menu::draw(BITMAP* buffer, int x, int y, int h, bool active) const {
	if (h < minHeight())
		return;
	textout_ex(buffer, font, caption.c_str(), x, y, captionColor(active), -1);
}

bool Menu::handleKey(char scan, unsigned char chr) {
	(void)chr;
	if (!(scan == KEY_ENTER || scan == KEY_ENTER_PAD || scan == KEY_SPACE))
		return false;
	callHook(*this);
	return true;
}

void Menu::shortcutActivated() {
	nAssert(isEnabled());
	callHook(*this);
}

int Menu::total_width() const {
	int min_width = caption.length() * char_w;
	for (vector<Component*>::const_iterator comp = components.begin(); comp != components.end(); ++comp)
		min_width = max(min_width, (*comp)->width());
	return min_width;	//#todo: leave space for shortcut numbers
}

int Menu::total_height() const {
	int height = 2 * line_h;	// for caption
	for (vector<Component*>::const_iterator comp = components.begin(); comp != components.end(); ++comp)
		height += (*comp)->height();
	return height;
}


int Spacer::height() const {
	return space * line_h / 10;
}


void Textfield::draw(BITMAP* buffer, int x, int y, int h, bool active) const {
	if (h < minHeight())
		return;
	textout_ex(buffer, font, caption.c_str(), x, y, captionColor(active), -1);
	x += caption.length() * char_w;
	textout_ex(buffer, font, ":", x, y, captionColor(active), -1);
	x += 2 * char_w;
	if (maskChar)
		textout_ex(buffer, font, string(value.length(), maskChar).c_str(), x, y, col_value, -1);
	else
		textout_ex(buffer, font, value.c_str(), x, y, col_value, -1);
	if (active) {
		x += value.length() * char_w;
		textout_ex(buffer, font, "_", x, y, col_value, -1);	// cursor
	}
}

int Textfield::width() const {
	return (caption.length() + 2 + maxlen + 1) * char_w;	// 2 for ": " and 1 for cursor
}

int Textfield::height() const {
	return line_h;
}

bool Textfield::handleKey(char scan, unsigned char chr) {
	bool stateChange = false;
	if (scan == KEY_BACKSPACE) {
		if (!value.empty()) {
			value.erase(value.end() - 1);
			stateChange = true;
		}
	}
	else if (!is_nonprintable_char(chr)) {
		if ((int)value.length() < maxlen) {
			value += chr;
			stateChange = true;
		}
	}
	else
		return callKeyHook(*this, scan, chr);	// note: callHook is not executed regardless of the return
	if (stateChange)
		callHook(*this);
	return true;
}


void SelectBase::draw(BITMAP* buffer, int x, int y, int h, bool active) const {
	if (h < minHeight())
		return;
	textout_ex(buffer, font, caption.c_str(), x, y, captionColor(active), -1);
	x += (caption.length()) * char_w;
	textout_ex(buffer, font, ":", x, y, captionColor(active), -1);
	x += 2 * char_w;
	nAssert(!options.empty());
	nAssert(selected >= 0 && selected < static_cast<int>(options.size()));
	if (active && selected > 0)
		drawKeySymbol(buffer, x, y, "<");
	x += 2 * char_w;
	textout_ex(buffer, font, options[selected].c_str(), x, y, col_value, -1);
	if (active && selected + 1 < (int)options.size()) {
		x += (options[selected].length() + 1) * char_w;
		drawKeySymbol(buffer, x, y, ">");
	}
}

int SelectBase::width() const {
	string::size_type maxSelLength = 0;	//#todo: precache
	for (vector<string>::const_iterator si = options.begin(); si != options.end(); ++si)
		if (si->length() > maxSelLength)
			maxSelLength = si->length();
	return (caption.length() + 2 + 2 + maxSelLength + 2) * char_w;
}

int SelectBase::height() const {
	return line_h;
}

bool SelectBase::handleKey(char scan, unsigned char chr) {
	(void)chr;
	if (scan == KEY_LEFT && selected > 0)
		--selected;
	else if (scan == KEY_RIGHT && selected + 1 < static_cast<int>(options.size()))
		++selected;
	else
		return false;
	virtualCallHook();
	return true;
}


void Colorselect::draw(BITMAP* buffer, int x, int y, int h, bool active) const {
	if (h < minHeight())
		return;
	textout_ex(buffer, font, caption.c_str(), x, y, captionColor(active), -1);
	x += (caption.length()) * char_w;
	textout_ex(buffer, font, ":", x, y, captionColor(active), -1);
	x += 2 * char_w;
	if (active)
		drawKeySymbol(buffer, x, y, "+");
	x += 2 * char_w;
	nAssert(!options.empty());
	nAssert(selected >= 0 && selected < static_cast<int>(options.size()));
	const int bw = 10;
	const int bh = 15;
	for (int i = 0; i < static_cast<int>(options.size()); i++)
		rectfill(buffer, x + (bw + 2) * i + 2, y - 2, x + (bw + 2) * i + bw - 2, y + bh - 6, graphics->player_color(options[i]));
	if (active) {	// mark selection
		const int i = selected;
		rect    (buffer, x + (bw + 2) * i    , y - 4, x + (bw + 2) * i + bw    , y + bh - 4, captionColor(active));
	}
	if (active) {
		x += options.size() * (bw + 2) + bw - 2;
		drawKeySymbol(buffer, x, y, "-");
	}
}

int Colorselect::width() const {
	return (caption.length() + 2) * char_w + options.size() * 12 + 2 * 2 * char_w;
}

int Colorselect::height() const {
	return line_h;
}

bool Colorselect::handleKey(char scan, unsigned char chr) {
	if ((scan == KEY_LEFT || chr == '+') && selected > 0) {
		if (key[KEY_LCONTROL] || key[KEY_RCONTROL] || chr == '+')
			swap(options[selected], options[selected - 1]);
		--selected;
	}
	else if ((scan == KEY_RIGHT || chr == '-') && selected + 1 < static_cast<int>(options.size())) {
		if (key[KEY_LCONTROL] || key[KEY_RCONTROL] || chr == '-')
			swap(options[selected], options[selected + 1]);
		++selected;
	}
	else
		return false;
	callHook(*this);
	return true;
}


void Checkbox::draw(BITMAP* buffer, int x, int y, int h, bool active) const {
	if (h < minHeight())
		return;
	textprintf_ex(buffer, font, x, y, col_value, -1, "[%c]", checked ? '×' : ' ');
	x += 4 * char_w;
	textout_ex(buffer, font, caption.c_str(), x, y, captionColor(active), -1);
}

int Checkbox::width() const {
	return (3 + 1 + caption.length()) * char_w;
}

int Checkbox::height() const {
	return line_h;
}

bool Checkbox::handleKey(char scan, unsigned char chr) {
	(void)chr;
	if (scan == KEY_SPACE || scan == KEY_X)
		toggle();
	else
		return false;
	callHook(*this);
	return true;
}

void Checkbox::shortcutActivated() {
	nAssert(isEnabled());
	toggle();
	callHook(*this);
}


void Slider::boundSet(int value) {
	val = bound(value, vmin, vmax);
}

int Slider::width() const {
	int fieldWidth;
	if (graphic)
		fieldWidth = 20;	// arbitrary bar length
	else
		fieldWidth = max(numberWidth(vmin), numberWidth(vmax));
	return char_w * (caption.length() + 1 + fieldWidth);
}

int Slider::height() const {
	return line_h;
}

void Slider::draw(BITMAP* buffer, int x, int y, int h, bool active) const {
	if (h < minHeight())
		return;
	textout_ex(buffer, font, caption.c_str(), x, y, captionColor(active), -1);
	const int x0 = x + (caption.length() + 1) * char_w;
	if (graphic) {
		const int barLength = (x + width() - 2 - x0) * (val - vmin) / (vmax - vmin);
		const int yb = y - 4;
		rect(buffer, x0, yb, x + width() - 1, yb + height() - 1, captionColor(active));
		if (barLength)
			rectfill(buffer, x0 + 1, yb + 1, x0 + barLength, yb + height() - 2, col_value);
		textprintf_centre_ex(buffer, font, (x0 + x + width() - 1) / 2, y, col_disabled, -1, "%.0f%%", 100 * static_cast<double>(val - vmin) / (vmax - vmin));
	}
	else
		textprintf_ex(buffer, font, x0, y, col_value, -1, "%d", val);
}

bool Slider::handleKey(char scan, unsigned char chr) {
	(void)chr;
	if ((scan == KEY_LEFT || chr == '-') && val > vmin) {
		if (key[KEY_LCONTROL] || key[KEY_RCONTROL] || chr == '-')
			--val;
		else if (step == 0)	// logarithmic
			val -= (val - vmin) / 11 + 1;	// /11 to have ++,-- or --,++ result in the original value; it's magic ;)
		else
			val -= step;
		if (val < vmin)
			val = vmin;
	}
	else if ((scan == KEY_RIGHT || chr == '+') && val < vmax) {
		if (key[KEY_LCONTROL] || key[KEY_RCONTROL] || chr == '+')
			++val;
		else if (step == 0)	// logarithmic
			val += (val - vmin) / 10 + 1;
		else
			val += step;
		if (val > vmax)
			val = vmax;
	}
	else
		return false;
	callHook(*this);
	return true;
}


void NumberEntry::boundSet(int value) {
	entry = val = bound(value, vmin, vmax);
}

int NumberEntry::width() const {
	int fieldWidth;	// don't count the cursor to this
	// in basic case space for val_ is needed
	// for the case of entry < vmin = val, space is needed for "entry_ (val)" (this can't happen when vmin == 0)
	if (vmin > 0) {
		int withEntry = numberWidth(vmin - 1) + numberWidth(vmin) + 3;	// the widest value for entry in this case is vmin - 1
		fieldWidth = max(withEntry, numberWidth(vmax));
	}
	else
		fieldWidth = numberWidth(vmax);
	return char_w * (caption.length() + 2 + fieldWidth + 1);	// 2 for ": ", 1 for cursor
}

int NumberEntry::height() const {
	return line_h;
}

void NumberEntry::draw(BITMAP* buffer, int x, int y, int h, bool active) const {
	if (h < minHeight())
		return;
	textout_ex(buffer, font, caption.c_str(), x, y, captionColor(active), -1);
	x += caption.length() * char_w;
	textout_ex(buffer, font, ":", x, y, captionColor(active), -1);
	x += 2 * char_w;
	if (entry != val)
		textprintf_ex(buffer, font, x, y, col_value, -1, "%d%s (%d)", entry, active ? "_" : "", val);
	else
		textprintf_ex(buffer, font, x, y, col_value, -1, "%d%s", val, active ? "_" : "");
}

bool NumberEntry::handleKey(char scan, unsigned char chr) {
	if ((scan == KEY_LEFT || chr == '-') && entry > vmin)
		--entry;
	else if ((scan == KEY_RIGHT || chr == '+') && entry < vmax)
		++entry;
	else if (scan == KEY_BACKSPACE)
		entry /= 10;	// discard the last digit
	else if (chr >= '0' && chr <= '9') {	// assuming sequentiality and ASCII order "0123456789"
		entry = entry * 10 + (chr - '0');
		if (entry > vmax) {
			entry /= 10;	// back up to where we were
			return false;
		}
	}
	else
		return false;
	int oldVal = val;
	if (entry >= vmin)
		val = entry;
	else
		val = vmin;
	if (val != oldVal)
		callHook(*this);
	return true;
}


int Textarea::width() const {
	int len = caption.length() + text.length();
	if (!caption.empty() && !text.empty())
		++len;
	return len * char_w;
}

int Textarea::height() const {
	return line_h;
}

void Textarea::draw(BITMAP* buffer, int x, int y, int h, bool active) const {
	if (h < minHeight())
		return;
	textout_ex(buffer, font, caption.c_str(), x, y, captionColor(active), -1);
	if (!caption.empty())
		x += (caption.length() + 1) * char_w;
	textout_ex(buffer, font, text.c_str(), x, y, col_value, -1);
}

bool Textarea::handleKey(char scan, unsigned char chr) {
	(void)chr;
	if (scan == KEY_ENTER || scan == KEY_ENTER_PAD || scan == KEY_SPACE) {
		callHook(*this);
		return true;
	}
	else if (callKeyHook(*this, scan, chr))
		return true;
	return false;
}

void Textarea::shortcutActivated() {
	nAssert(isEnabled());
	callHook(*this);
}


int StaticText::width() const {
	int len = caption.length() + text.length();
	if (!caption.empty() && !text.empty())
		++len;
	return len * char_w;
}

int StaticText::height() const {
	return line_h;
}

void StaticText::draw(BITMAP* buffer, int x, int y, int h, bool active) const {
	active = false;	// don't draw as active even if caller thinks this should be active
	if (h < minHeight())
		return;
	textout_ex(buffer, font, caption.c_str(), x, y, captionColor(active), -1);
	if (!caption.empty())
		x += (caption.length() + 1) * char_w;
	textout_ex(buffer, font, text.c_str(), x, y, col_value, -1);
}


int Textobject::width() const {
	int len = 0;
	for (vector<string>::const_iterator li = lines.begin(); li != lines.end(); ++li)
		len = max<int>(len, li->length());
	return len * char_w;
}

int Textobject::height() const {
	return lines.size() * line_h;
}

void Textobject::draw(BITMAP* buffer, int x, int y0, int h, bool active) const {
	(void)active;
	if (start > static_cast<int>(lines.size()) - h / line_h)
		start = lines.size() - h / line_h;
	if (start < 0)
		start = 0;
	visible_lines = 0;
	for (int i = start, y = y0; i < static_cast<int>(lines.size()); ++i) {
		if (y + line_h > y0 + h)
			break;
		textout_ex(buffer, font, lines[i].c_str(), x, y, col_value, -1);
		y += line_h;
		++visible_lines;
	}

	// draw scrollbar if everything didn't fit
	if (visible_lines != static_cast<int>(lines.size())) {
		const int sbx = x + width() + char_w;
		const int height = visible_lines * line_h;
		const int bar_y = static_cast<int>(static_cast<double>(height * start) / lines.size() + 0.5);
		const int bar_h = static_cast<int>(static_cast<double>(height * visible_lines) / lines.size() + 0.5);
		scrollbar(buffer, sbx, y0, height, bar_y, bar_h, col_scrollbar, col_scrollbarBg);
	}
}

bool Textobject::handleKey(char scan, unsigned char chr) {
	(void)chr;
	// If start goes out of range, draw method fixes it.
	if (scan == KEY_UP)
		--start;
	else if (scan == KEY_DOWN)
		++start;
	else if (scan == KEY_HOME)
		start = 0;
	else if (scan == KEY_END)
		start = INT_MAX;
	else if (scan == KEY_PGUP)
		start -= visible_lines;
	else if (scan == KEY_PGDN)
		start += visible_lines;
	else
		return false;
	return true;
}

