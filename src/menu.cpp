#include "incalleg.h"
#include "nassert.h"

#include "menu.h"

using std::max;
using std::min;
using std::string;
using std::vector;

// character width and line height in pixels
const int char_w = 8;
const int line_h = 12;

int col_background	= makecol(0x30, 0x30, 0x30);
int col_caption		= makecol(0x30, 0xFF, 0x30);
int col_active		= makecol(0x80, 0xFF, 0x80);
int col_disabled	= makecol(0x30, 0x70, 0x30);
int col_value		= makecol(0x30, 0xA0, 0x30);

int Component::captionColor(bool active) const {
	if (!isEnabled())
		return col_disabled;
	if (active)
		return col_active;
	return col_caption;
}

void Menu::home() {
	for (selected_item = 0; !components[selected_item]->isEnabled(); ++selected_item)
		if (selected_item >= static_cast<int>(components.size())) {
			selected_item = 0;	// maybe this isn't needed
			return;
		}
}

bool Menu::prev() {
	int original = selected_item;
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
	int original = selected_item;
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
col_background	= makecol(0x30, 0x30, 0x30);
col_caption		= makecol(0x30, 0xFF, 0x30);
col_active		= makecol(0x80, 0xFF, 0x80);
col_disabled	= makecol(0x30, 0x70, 0x30);
col_value		= makecol(0x30, 0xA0, 0x30);
	drawHook.call(*this);

	if (!components[selected_item]->isEnabled())	// a disabled component can not be active
		if (!next())	// try moving down first - feels intuitive
			prev();	// if it fails, so be it

	// calculate menu width and height
	const int padding = 12;
	const int max_width = buffer->w;
	const int min_width = total_width() + padding * 2;
	const int max_height = buffer->h;
	const int min_height = total_height() + padding * 2;
	const int w = min(max_width, min_width);
	const int h = min(max_height, min_height);
	const int mx = max_width / 2;
	const int my = max_height / 2;
	const int x1 = mx - w / 2;
	const int y1 = my - h / 2;
	const int x2 = mx + w / 2;
	const int y2 = my + h / 2;

	rectfill(buffer, x1, y1, x2, y2, col_background);

	int y = y1 + padding;

	// draw caption
	textout_centre_ex(buffer, font, caption.c_str(), mx, y, col_caption, -1);
	y += 2 * line_h;

	// draw components
	for (int compi = 0; compi < static_cast<int>(components.size()); ++compi) {
		Component* component = components[compi];
		//#todo: show shortcut numbers if the active component doesn't needNumberKeys()
		component->draw(buffer, x1 + padding, y, compi == selected_item);
		y += component->height();
	}
}

void Menu::handleKeypress(char scan, char chr) {
	if (scan == KEY_UP)
		prev();
	else if (scan == KEY_DOWN)
		next();
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

bool Menu::handleKey(char scan, char chr) {
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
	x += (caption.length() + 1) * char_w;
	if (maskChar)
		textout_ex(buffer, font, string(value.length(), maskChar).c_str(), x, y, col_value, -1);
	else
		textout_ex(buffer, font, value.c_str(), x, y, col_value, -1);
}

int Textfield::width() const {
	return (caption.length() + 1 + maxlen) * char_w;
}

int Textfield::height() const {
	return line_h;
}

bool Textfield::handleKey(char scan, char chr) {
	if (scan == KEY_BACKSPACE && !value.empty())
		value.erase(value.end() - 1);
	else if (chr >= 32 && static_cast<int>(value.length()) < maxlen)
		value += chr;
	else
		return false;
	callHook(*this);
	return true;
}


void Select::draw(BITMAP* buffer, int x, int y, bool active) const {
	textout_ex(buffer, font, caption.c_str(), x, y, captionColor(active), -1);
	x += (caption.length() + 1) * char_w;
	nAssert(!options.empty());
	nAssert(selected >= 0 && selected < static_cast<int>(options.size()));
	textout_ex(buffer, font, options[selected].c_str(), x, y, col_value, -1);
}

int Select::width() const {
	string::size_type maxSelLength = 0;	//#todo: precache
	for (vector<string>::const_iterator si = options.begin(); si != options.end(); ++si)
		if (si->length() > maxSelLength)
			maxSelLength = si->length();
	return (caption.length() + 1 + maxSelLength) * char_w;
}

int Select::height() const {
	return line_h;
}

bool Select::handleKey(char scan, char chr) {
	(void)chr;
	if (scan == KEY_LEFT && selected > 0)
		--selected;
	else if (scan == KEY_RIGHT && selected + 1 < static_cast<int>(options.size()))
		++selected;
	else
		return false;
	callHook(*this);
	return true;
}


void Checkbox::draw(BITMAP* buffer, int x, int y, bool active) const {
	const char selection = selected ? '×' : ' ';
	textprintf_ex(buffer, font, x, y, col_value, -1, "[%c]", selection);
	x += 4 * char_w;
	textout_ex(buffer, font, caption.c_str(), x, y, captionColor(active), -1);
}

int Checkbox::width() const {
	return (3 + 1 + caption.length()) * char_w;
}

int Checkbox::height() const {
	return line_h;
}

bool Checkbox::handleKey(char scan, char chr) {
	(void)chr;
	if (scan == KEY_SPACE)
		selected = !selected;
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
	int x0 = x + (caption.length() + 1) * char_w;
	int barLength = (x + width() - 2 - x0) * (val - vmin) / (vmax - vmin);
	rect(buffer, x0, y, x + width() - 1, y + height() - 1, captionColor(active));
	if (barLength)
		rectfill(buffer, x0 + 1, y + 1, x0 + barLength, y + height() - 2, col_value);
}

bool Slider::handleKey(char scan, char chr) {
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

bool Textarea::handleKey(char scan, char chr) {
	(void)chr;
	if (scan == KEY_ENTER || scan == KEY_ENTER_PAD) {
		callHook(*this);
		return true;
	}
	return false;
}
