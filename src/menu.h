#ifndef MENU_H_INC
#define MENU_H_INC

#include <string>
#include <vector>
#include <stack>

#include "function_utility.h"
#include "nassert.h"

class Graphics;
class BITMAP;

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
	virtual bool handleKey(char, unsigned char) { return false; }	// an object should either have an active handleKey() or override so that !isEnabled()

protected:
	int captionColor(bool active) const;	// decides a color based on isEnabled() and active

	std::string caption;
	bool enabled;
};

template<class CallerT>
class MenuHookable : public Hookable1<void, CallerT&> { };

template<class CallerT>
class MenuHook : public Hook1<void, CallerT&> {	};

template<class CallerT>
class KeyHookable {
	// can't privately inherit KeyHookable from Hookable3 because the compiler still makes ambiguities of the functions when inheriting from KeyHookable!
	class Helper : public Hookable3<bool, CallerT&, char, unsigned char> {
	public:
		bool callHook_helper(CallerT& a1, char a2, unsigned char a3) { return callHook(a1, a2, a3); }
	};
	Helper helper;

public:
	typedef typename Hookable3<bool, CallerT&, char, unsigned char>::HookFunctionT HookFunctionT;

	void setKeyHook(HookFunctionT* fn) { helper.setHook(fn); }	// the ownership is transferred
	bool isKeyHooked() const { return helper.isHooked(); }

protected:
	bool callKeyHook(CallerT& a1, char a2, unsigned char a3) { return helper.callHook_helper(a1, a2, a3); }
};

template<class CallerT>
class KeyHook : public Hook3<bool, CallerT&, char, unsigned char> { };

// Menu being a Component is a hack: the Component part is a link to the menu
class Menu : public Component, public MenuHookable<Menu> {
public:
	// visible_items to 20 is there to prevent scrollbar when starting the game and pressing down arrow same time
	Menu(const std::string& caption_): Component(caption_), start(0), selected_item(0), visible_items(20) { }

	void clear_components() { components.clear(); }
	void add_component(Component* comp) { components.push_back(comp); }

	void open() { openHook.call(*this); home(); }
	void close() { closeHook.call(*this); }

	void draw(BITMAP* buffer);	// no const because drawHook might modify the menu
	void handleKeypress(char scan, unsigned char chr);

	void  setDrawHook(MenuHook<Menu>::FunctionT* fn) {  drawHook.set(fn); }	// called before drawing
	void  setOpenHook(MenuHook<Menu>::FunctionT* fn) {  openHook.set(fn); }	// called by open()
	void setCloseHook(MenuHook<Menu>::FunctionT* fn) { closeHook.set(fn); }	// called by close()
	void    setOkHook(MenuHook<Menu>::FunctionT* fn) {    okHook.set(fn); }	// called when enter is pressed (and not handled by active entry)

	// inherited interface
	int width() const;
	int height() const;
	void draw(BITMAP* buffer, int x, int y, bool active) const;
	bool handleKey(char, unsigned char);

private:
	void scrollbar(BITMAP* buffer, int x, int y, int height, int bar_y, int bar_h, int col1, int col2);

	int total_width() const;
	int total_height() const;

	void home();	// moves the cursor to topmost selectable item
	void end();		// moves the cursor to the last selectable item
	bool prev();
	bool next();

	std::vector<Component*> components;
	int start;
	int selected_item;
	int visible_items;

	MenuHook<Menu> drawHook, openHook, closeHook, okHook;
};

class MenuStack {
public:
	bool empty() const { return st.empty(); }
	Menu* top() const { nAssert(!st.empty()); return st.top(); }
	Menu* safeTop() const { if (st.empty()) return 0; return st.top(); }
	void open(Menu* menu) { menu->open(); st.push(menu); }
	void close() { nAssert(!empty()); Menu* menu = st.top(); st.pop(); menu->close(); }
	void clear() { while (!empty()) close(); }
	void draw(BITMAP* buf) { nAssert(!empty()); st.top()->draw(buf); }
	void handleKeypress(char scan, unsigned char chr) { nAssert(!empty()); st.top()->handleKeypress(scan, chr); }

private:
	std::stack<Menu*> st;
};

class Textfield : public Component, public MenuHookable<Textfield> {
public:
	Textfield(const std::string& caption_, const std::string& init_text, int fieldLength, char mask = 0): Component(caption_), value(init_text), maxlen(fieldLength), maskChar(mask) { }
	void set(const std::string& text) { value = text; }
	const std::string& operator()() const { return value; }

	// inherited interface
	bool needsNumberKeys() const { return true; }
	int width() const;
	int height() const;
	void draw(BITMAP* buffer, int x, int y, bool active) const;
	bool handleKey(char scan, unsigned char chr);

private:
	std::string value;
	int maxlen;
	char maskChar;	// 0 for no masking
};

class SelectBase : public Component {
public:
	virtual ~SelectBase() { }
	void draw(BITMAP* buffer, int x, int y, bool active) const;
	int width() const;
	int height() const;
	bool handleKey(char scan, unsigned char chr);

	virtual void virtualCallHook() = 0;

protected:
	SelectBase(const std::string caption_): Component(caption_), selected(0) { }

	std::vector<std::string> options;
	int selected;
};

template<class ValueT>
class Select : public SelectBase, public MenuHookable< Select<ValueT> > {
public:
	Select(const std::string caption_): SelectBase(caption_) { }
	void clearOptions() { options.clear(); values.clear(); selected = 0; }
	void addOption(const std::string& text, const ValueT& value) { options.push_back(text); values.push_back(value); }
	bool set(const ValueT& value);	// returns false if there is no value in the options
//	void set(int selection) { nAssert(selection >= 0 && selection < static_cast<int>(options.size())); selected = selection; }
	const ValueT& operator()() const { return values[selected]; }

private:
	void virtualCallHook() { callHook(*this); }
	std::vector<ValueT> values;	// should always be in sync with options
};

class Colorselect : public Component, public MenuHookable<Colorselect> {
public:
	Colorselect(const std::string caption_): Component(caption_), selected(0), graphics(0) { }
	void setGraphicsCallBack(const Graphics& graph) { graphics = &graph; }
	void clearOptions() { options.clear(); selected = 0; }
	void addOption(int col) { options.push_back(col); }
	void set(int selection) { nAssert(selection >= 0 && selection < static_cast<int>(options.size())); selected = selection; }
	int operator()() const { return selected; }
	int asInt() const { return options[selected]; }
	const std::vector<int>& values() const { return options; }

	// inherited interface
	int width() const;
	int height() const;
	void draw(BITMAP* buffer, int x, int y, bool active) const;
	bool handleKey(char scan, unsigned char chr);

private:
	std::vector<int> options;
	int selected;
	const Graphics* graphics;
};

class Checkbox : public Component, public MenuHookable<Checkbox> {
public:
	Checkbox(const std::string& caption_, bool init_value = false): Component(caption_), checked(init_value) { }
	void toggle() { checked = !checked; }
	void set(bool value) { checked = value; }
	bool operator()() const { return checked; }

	// inherited interface
	int width() const;
	int height() const;
	void draw(BITMAP* buffer, int x, int y, bool active) const;
	bool handleKey(char scan, unsigned char chr);

private:
	bool checked;
};

class Slider : public Component, public MenuHookable<Slider> {
public:
	Slider(const std::string caption_, int vmin_, int vmax_) : Component(caption_), vmin(vmin_), vmax(vmax_), val(vmin_) { }
	Slider(const std::string caption_, int vmin_, int vmax_, int init_value) : Component(caption_), vmin(vmin_), vmax(vmax_), val(init_value) { }
	void set(int value) { val = value; }
	int operator()() const { return val; }

	// inherited interface
	int width() const;
	int height() const;
	void draw(BITMAP* buffer, int x, int y, bool active) const;
	bool handleKey(char scan, unsigned char chr);

private:
	int vmin, vmax, val;
};

class Textarea : public Component, public MenuHookable<Textarea>, public KeyHookable<Textarea> {
public:
	Textarea(const std::string& caption_, const std::string& text_ = std::string()): Component(caption_), text(text_) { }
	void set(const std::string& val) { text = val; }

	// inherited interface
	int width() const;
	int height() const;
	void draw(BITMAP* buffer, int x, int y, bool active) const;
	bool handleKey(char scan, unsigned char chr);

	// override isEnabled() : can't be enabled if not hooked
	bool isEnabled() const { return Component::isEnabled() && isHooked(); }

private:
	std::string text;
};

// this template does the necessary wrapping of member function references to be given to Components as callbacks
// MenuCallback<UserClass>::A<ComponentType, &UserClass::handlerFunction>(userClassInstance)
//		calls void userClassInstance.handlerFunction(ComponentType&)
// MenuCallback<UserClass>::N<ComponentType, &UserClass::handlerFunction>(userClassInstance)
//		calls void userClassInstance.handlerFunction()
//		used when the component reference is not needed
template<class CallClassT>
class MenuCallback {
public:
	template<class ArgT, void (CallClassT::*memFun)(ArgT&)>
	class A : public HookFunctionBase1<void, ArgT&> {
	public:
		A(CallClassT* host_) : host(host_) { }
		void operator()(ArgT& obj) { (host->*memFun)(obj); }
		A* clone() { return new A(host); }

	private:
		CallClassT* host;
	};

	template<class ArgT, void (CallClassT::*memFun)()>
	class N : public HookFunctionBase1<void, ArgT&> {
	public:
		N(CallClassT* host_) : host(host_) { }
		void operator()(ArgT&) { (host->*memFun)(); }
		N* clone() { return new N(host); }

	private:
		CallClassT* host;
	};
};

template<class CallClassT>
class MenuKeyCallback {
public:
	template<class ArgT, bool (CallClassT::*memFun)(ArgT&, char, unsigned char)>
	class A : public HookFunctionBase3<bool, ArgT&, char, unsigned char> {
	public:
		A(CallClassT* host_) : host(host_) { }
		bool operator()(ArgT& obj, char scan, unsigned char chr) { return (host->*memFun)(obj, scan, chr); }
		A* clone() { return new A(host); }

	private:
		CallClassT* host;
	};

	template<class ArgT, bool (CallClassT::*memFun)(ArgT&, char, unsigned char)>
	class N : public HookFunctionBase3<bool, ArgT&, char, unsigned char> {
	public:
		N(CallClassT* host_) : host(host_) { }
		bool operator()(ArgT&, char scan, unsigned char chr) { return (host->*memFun)(scan, chr); }
		N* clone() { return new N(host); }

	private:
		CallClassT* host;
	};
};

// template implementation

template<class ValueT>
bool Select<ValueT>::set(const ValueT& value) {
	for (int i = 0; i < (int)values.size(); ++i)
		if (values[i] == value) {
			selected = i;
			return true;
		}
	return false;
}

#endif // MENU_H_INC

