#ifndef MENU_H_INC
#define MENU_H_INC

#include <string>
#include <vector>
#include <stack>
#include "utility.h"	// for Hook

// Base class of menu components
class Component {
public:
	Component(const std::string& caption_): caption(caption_), enabled(true) { }
	virtual ~Component() { };

	void setCaption(const std::string& text) { caption = text; }

	void setEnable(bool state) { enabled = state; }

	virtual bool isEnabled() const { return enabled; }
	virtual bool needsNumberKeys() const { return false; }

	virtual void draw(BITMAP* buffer, int x, int y, bool active) const = 0;
	virtual int width() const = 0;
	virtual int height() const = 0;
	virtual bool handleKey(char, char) { return false; }	// an object should either have an active handleKey() or override so that !isEnabled()

protected:
	int captionColor(bool active) const;	// decides a color based on isEnabled() and active

	std::string caption;
	bool enabled;
};

// Menu being a Component is a hack: the Component part is a link to the menu
class Menu : public Component, public Hookable<Menu> {
public:
	Menu(const std::string& caption_): Component(caption_), selected_item(0) { }

	void add_component(Component* comp) { components.push_back(comp); }

	void open() { openHook.call(*this); home(); }
	void close() { closeHook.call(*this); }

	void home();	// moves the cursor to topmost selectable item
	void draw(BITMAP* buffer);	// no const because drawHook might modify the menu
	void handleKeypress(char scan, char chr);

	void  setDrawHook(Hook<Menu>::FunctionT* fn) {  drawHook.set(fn); }	// called before drawing
	void  setOpenHook(Hook<Menu>::FunctionT* fn) {  openHook.set(fn); }	// called by open()
	void setCloseHook(Hook<Menu>::FunctionT* fn) { closeHook.set(fn); }	// called by close()
	void    setOkHook(Hook<Menu>::FunctionT* fn) {    okHook.set(fn); }	// called when enter is pressed (and not handled by active entry)

	// inherited interface
	int width() const;
	int height() const;
	void draw(BITMAP* buffer, int x, int y, bool active) const;
	bool handleKey(char, char);

private:
	int total_width() const;
	int total_height() const;

	bool prev();
	bool next();

	std::vector<Component*> components;
	int selected_item;

	Hook<Menu> drawHook, openHook, closeHook, okHook;
};

class MenuStack {
public:
	bool empty() const { return st.empty(); }
	Menu* top() const { return st.top(); }
	void open(Menu* menu) { menu->open(); st.push(menu); }
	void close() { nAssert(!empty()); Menu* menu = st.top(); st.pop(); menu->close(); }
	void clear() { while (!empty()) close(); }
	void draw(BITMAP* buf) { nAssert(!empty()); st.top()->draw(buf); }
	void handleKeypress(char scan, char chr) { nAssert(!empty()); st.top()->handleKeypress(scan, chr); }

private:
	std::stack<Menu*> st;
};

class Textfield : public Component, public Hookable<Textfield> {
public:
	Textfield(const std::string& caption_, const std::string& init_text, int fieldLength, char mask = 0): Component(caption_), value(init_text), maxlen(fieldLength), maskChar(mask) { }
	void set(const std::string& text) { value = text; }
	std::string operator()() const { return value; }

	// inherited interface
	bool needsNumberKeys() const { return true; }
	int width() const;
	int height() const;
	void draw(BITMAP* buffer, int x, int y, bool active) const;
	bool handleKey(char scan, char chr);

private:
	std::string value;
	int maxlen;
	char maskChar;	// 0 for no masking
};

class Select : public Component, public Hookable<Select> {
public:
	Select(const std::string caption_): Component(caption_), selected(0) { }
	void clearOptions() { options.clear(); selected = 0; }
	void addOption(const std::string& text) { options.push_back(text); }
	void set(int selection) { nAssert(selection >= 0 && selection < static_cast<int>(options.size())); selected = selection; }
	int operator()() const { return selected; }
	std::string asString() const { return options[selected]; }

	// inherited interface
	int width() const;
	int height() const;
	void draw(BITMAP* buffer, int x, int y, bool active) const;
	bool handleKey(char scan, char chr);

private:
	std::vector<std::string> options;
	int selected;
};

class Checkbox : public Component, public Hookable<Checkbox> {
public:
	Checkbox(const std::string& caption_, bool init_value = false): Component(caption_), selected(init_value) { }
	void toggle() { selected = !selected; }
	void set(bool value) { selected = value; }
	bool operator()() const { return selected; }

	// inherited interface
	int width() const;
	int height() const;
	void draw(BITMAP* buffer, int x, int y, bool active) const;
	bool handleKey(char scan, char chr);

private:
	bool selected;
};

class Slider : public Component, public Hookable<Slider> {
public:
	Slider(const std::string caption_, int vmin_, int vmax_) : Component(caption_), vmin(vmin_), vmax(vmax_), val(vmin_) { }
	void set(int value) { val = value; }
	int operator()() const { return val; }

	// inherited interface
	int width() const;
	int height() const;
	void draw(BITMAP* buffer, int x, int y, bool active) const;
	bool handleKey(char scan, char chr);

private:
	int vmin, vmax, val;
};

class Textarea : public Component, public Hookable<Textarea> {
public:
	Textarea(const std::string& caption_, const std::string& text_ = std::string()): Component(caption_), text(text_) { }
	void set(const std::string& val) { text = val; }

	// inherited interface
	int width() const;
	int height() const;
	void draw(BITMAP* buffer, int x, int y, bool active) const;
	bool handleKey(char scan, char chr);

	// override isEnabled() : can't be enabled if not hooked
	bool isEnabled() const { return Component::isEnabled() && isHooked(); }

private:
	std::string text;
};

#endif // MENU_H_INC

