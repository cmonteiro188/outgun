#include <algorithm>
#include "incalleg.h"
#include "nassert.h"

#include "client.h"
#include "menu.h"

using std::max;
using std::min;
using std::string;
using std::swap;
using std::vector;

// character width and line height in pixels
const int char_w = 8;
const int line_h = 20;

int col_background, col_borderShadow, col_borderHighlight, col_menuCaption, col_menuCaptionBg, col_caption, col_active, col_disabled, col_value, col_scrollbar, col_scrollbarBg;

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
			return false;
		}
	} while (!components[selected_item]->isEnabled());
	return true;
}

void Menu::draw(BITMAP* buffer) {
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
	drawHook.call(*this);

	if (selected_item >= static_cast<int>(components.size()))
		selected_item = components.size() - 1;

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
/*	hline(buffer, x1 + 1, y1 + 1, x2 - 1, col_borderHighlight);
	vline(buffer, x1 + 1, y1 + 1, y2 - 1, col_borderHighlight);
	hline(buffer, x1 + 1, y2 - 1, x2 - 1, col_borderShadow);
	vline(buffer, x2 - 1, y1 + 1, y2 - 1, col_borderShadow);*/
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
	visible_items = 0;
	for (int compi = start; compi < static_cast<int>(components.size()); ++compi) {
		Component* component = components[compi];
		if (y + component->height() > y2 - padding)
			break;
		//#todo: show shortcut numbers if the active component doesn't needNumberKeys()
		//textprintf_right_ex(buffer, font, x_start - char_w, y, col_menuCaption, -1, "%d", compi + 1);
		component->draw(buffer, x_start, y, compi == selected_item);
		y += component->height();
		visible_items++;
	}

	// draw scrollbar if everything didn't fit
	if (visible_items != static_cast<int>(components.size())) {
		const int x = x2 - padding + char_w;
		const int y = scrollbar_start_y;
		const int height = visible_items * line_h;
		const int bar_y = static_cast<int>(static_cast<float>(height * start) / components.size() + 0.5);
		const int bar_h = static_cast<int>(static_cast<float>(height * visible_items) / components.size() + 0.5);
		scrollbar(buffer, x, y, height, bar_y, bar_h, col_scrollbar, col_scrollbarBg);
	}
}

void Menu::scrollbar(BITMAP* buffer, int x, int y, int height, int bar_y, int bar_h, int col1, int col2) {
	const int width = 10;
	if (height > 0) {
		rectfill(buffer, x, y, x + width - 1, y + height - 1, col2);
		if (bar_h > 0)
			rectfill(buffer, x, y + bar_y, x + width - 1, y + bar_y + bar_h - 1, col1);
	}
}

void Menu::handleKeypress(char scan, unsigned char chr) {
	if (scan == KEY_UP || (scan == KEY_TAB && (key[KEY_LSHIFT] || key[KEY_RSHIFT])))
		prev();
	else if (scan == KEY_DOWN || scan == KEY_TAB)
		next();
	else if (scan == KEY_HOME)
		home();
	else if (scan == KEY_END)
		end();
	else if (chr == 0) {	// Alt + number
		switch (scan) {
			case KEY_1: selected_item = 0; break;
			case KEY_2: selected_item = 1; break;
			case KEY_3: selected_item = 2; break;
			case KEY_4: selected_item = 3; break;
			case KEY_5: selected_item = 4; break;
			case KEY_6: selected_item = 5; break;
			case KEY_7: selected_item = 6; break;
			case KEY_8: selected_item = 7; break;
			case KEY_9: selected_item = 8; break;
		}
	}
	if (selected_item >= static_cast<int>(components.size()))
		selected_item = components.size() - 1;
	if (selected_item < start)
		start = selected_item;
	else if (selected_item >= start + visible_items)
		start = selected_item - visible_items + 1;
	//#todo: handle number shortcuts if the active component doesn't needNumberKeys()
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

void Menu::draw(BITMAP* buffer, int x, int y, bool active) const {
	textout_ex(buffer, font, caption.c_str(), x, y, captionColor(active), -1);
}

bool Menu::handleKey(char scan, unsigned char chr) {
	(void)chr;
	if (!(scan == KEY_ENTER || scan == KEY_ENTER_PAD))
		return false;
	callHook(*this);
	return true;
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


void Textfield::draw(BITMAP* buffer, int x, int y, bool active) const {
	textout_ex(buffer, font, caption.c_str(), x, y, captionColor(active), -1);
	x += (caption.length()) * char_w;
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
	return (caption.length() + 2 + maxlen) * char_w;
}

int Textfield::height() const {
	return line_h;
}

bool Textfield::handleKey(char scan, unsigned char chr) {
	if (scan == KEY_BACKSPACE && !value.empty())
		value.erase(value.end() - 1);
	else if (((chr >= 32 && chr <= 127) || chr >= 160) && static_cast<int>(value.length()) < maxlen)
		value += chr;
	else
		return false;
	callHook(*this);
	return true;
}


void SelectBase::draw(BITMAP* buffer, int x, int y, bool active) const {
	textout_ex(buffer, font, caption.c_str(), x, y, captionColor(active), -1);
	x += (caption.length()) * char_w;
	textout_ex(buffer, font, ":", x, y, captionColor(active), -1);
	x += 2 * char_w;
	nAssert(!options.empty());
	nAssert(selected >= 0 && selected < static_cast<int>(options.size()));
	textout_ex(buffer, font, options[selected].c_str(), x, y, col_value, -1);
}

int SelectBase::width() const {
	string::size_type maxSelLength = 0;	//#todo: precache
	for (vector<string>::const_iterator si = options.begin(); si != options.end(); ++si)
		if (si->length() > maxSelLength)
			maxSelLength = si->length();
	return (caption.length() + 2 + maxSelLength) * char_w;
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


void Colorselect::draw(BITMAP* buffer, int x, int y, bool active) const {
	textout_ex(buffer, font, caption.c_str(), x, y, captionColor(active), -1);
	x += (caption.length()) * char_w;
	textout_ex(buffer, font, ":", x, y, captionColor(active), -1);
	x += 2 * char_w;
	nAssert(!options.empty());
	nAssert(selected >= 0 && selected < static_cast<int>(options.size()));
	const int w = 10;
	const int h = 15;
	for (int i = 0; i < static_cast<int>(options.size()); i++) {
		rectfill(buffer, x + (w + 2) * i + 2, y, x + (w + 2) * i + w - 2, y + h - 4, graphics->player_color(options[i]));
		if (selected == i)
			rect(buffer, x + (w + 2) * i, y - 2, x + (w + 2) * i + w, y + h - 2, captionColor(active));
	}
}

int Colorselect::width() const {
	return (caption.length() + 2) * char_w + options.size() * 12;
}

int Colorselect::height() const {
	return line_h;
}

bool Colorselect::handleKey(char scan, unsigned char chr) {
	if (scan == KEY_LEFT && selected > 0)
		--selected;
	else if (scan == KEY_RIGHT && selected + 1 < static_cast<int>(options.size()))
		++selected;
	else if (chr == '-' && selected > 0) {
		swap(options[selected], options[selected - 1]);
		--selected;
	}
	else if (chr == '+' && selected + 1 < static_cast<int>(options.size())) {
		swap(options[selected], options[selected + 1]);
		++selected;
	}
	else
		return false;
	callHook(*this);
	return true;
}


void Checkbox::draw(BITMAP* buffer, int x, int y, bool active) const {
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
	if (scan == KEY_SPACE)
		toggle();
	else
		return false;
	callHook(*this);
	return true;
}


int Slider::width() const {
	return char_w * (caption.length() + 20);	// arbitrary bar length
}

int Slider::height() const {
	return line_h;
}

void Slider::draw(BITMAP* buffer, int x, int y, bool active) const {
	textout_ex(buffer, font, caption.c_str(), x, y, captionColor(active), -1);
	const int x0 = x + (caption.length() + 1) * char_w;
	const int barLength = (x + width() - 2 - x0) * (val - vmin) / (vmax - vmin);
	y -= 4;
	rect(buffer, x0, y, x + width() - 1, y + height() - 1, captionColor(active));
	if (barLength)
		rectfill(buffer, x0 + 1, y + 1, x0 + barLength, y + height() - 2, col_value);
}

bool Slider::handleKey(char scan, unsigned char chr) {
	(void)chr;
	if (scan == KEY_LEFT && val > vmin)
		--val;
	else if (scan == KEY_RIGHT && val < vmax)
		++val;
	else
		return false;
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

void Textarea::draw(BITMAP* buffer, int x, int y, bool active) const {
	textout_ex(buffer, font, caption.c_str(), x, y, captionColor(active), -1);
	if (!caption.empty())
		x += (caption.length() + 1) * char_w;
	textout_ex(buffer, font, text.c_str(), x, y, col_value, -1);
}

bool Textarea::handleKey(char scan, unsigned char chr) {
	(void)chr;
	if (scan == KEY_ENTER || scan == KEY_ENTER_PAD) {
		callHook(*this);
		return true;
	}
	else if (callKeyHook(*this, scan, chr)) {
		//callHook(*this);
		return true;
	}
	return false;
}
