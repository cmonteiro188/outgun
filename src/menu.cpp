#include <allegro.h>
#ifdef ALLEGRO_WINDOWS
#include <winalleg.h>
#endif

#include "menu.h"

using std::max;
using std::min;
using std::vector;

// character width and line height in pixels
const int char_w = 8;
const int line_h = 12;

void Menu::prev() {
	--selected_item;
	if (selected_item < 0)
		selected_item = 0;
}

void Menu::next() {
	++selected_item;
	if (selected_item >= components.size())
		selected_item = components.size() - 1;
}

void Menu::draw(BITMAP* buffer) const {
	// calculate menu width and height
	const int padding = 4;
	const int max_width = buffer->w;
	const int min_width = total_width() + padding;
	const int max_height = buffer->h;
	const int min_height = total_height() + padding;
	const int w = min(max_width, min_width);
	const int h = min(max_height, min_height);
	const int mx = max_width / 2;
	const int my = max_height / 2;
	const int x1 = mx - w / 2;
	const int y1 = my - h / 2;
	const int x2 = mx + w / 2;
	const int y2 = my + h / 2;

	rectfill(buffer, x1, y1, x2, y2, col_background);

	int line = 1;

	// draw caption
	textout_centre_ex(buffer, font, caption.c_str(), mx, y1 + line++ * line_h, col_caption, -1);
	line++;

	// draw components
	for (vector<Component*>::const_iterator comp = components.begin(); comp != components.end(); ++comp) {
		comp->draw(buffer, x1 + padding, y1 + line * line_h);
		line++;
	}
}

int Menu::width() const {
	return caption.length() * char_w;
}

int Menu::total_width() const {
	int min_width = 0;
	for (vector<Component*>::const_iterator comp = components.begin(); comp != components.end(); ++comp)
		min_width = max(min_width, comp->width());
	return min_width;
}

int Menu::total_height() const {
	int min_height = 0;
	for (vector<Component*>::const_iterator comp = components.begin(); comp != components.end(); ++comp)
		min_height += line_h;
	return min_height;
}

void Textfield::draw(BITMAP* buffer, int x, int y) const {
	textout_ex(buffer, font, caption.c_str(), x, y, col_caption, -1);
	x += caption.length() + 1;
	textout_ex(buffer, font, value.c_str(), x, y, col_value, -1);
}

int Textfield::width() const {
	return (caption.length() + 1 + value.length()) * char_w;
}

