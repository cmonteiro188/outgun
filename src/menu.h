#ifndef MENU_H_INC
#define MENU_H_INC

#include <string>
#include <vector>

// Base class of menu components
class Component {
public:
	Component(const std::string& caption_): caption(caption_) { }
	virtual ~Component() = 0;

	virtual void draw(BITMAP* buffer) const = 0;
	virtual void draw(BITMAP* buffer, int x, int y) const = 0;
	virtual int width() const = 0;

protected:
	std::string caption;
};

class Menu : public Component {
public:
	Menu(const std::string& caption_): Component(caption_), selected_item(0) { }

	void add_component(Component* comp) { components.push_back(comp); }

	void prev();
	void next();

	void draw(BITMAP* buffer) const;
	int width() const;

private:
	int total_width() const;
	int total_height() const;

	std::vector<Component*> components;
	int selected_item;
};

class Textfield : public Component {
public:
	Textfield(const std::string& caption_, const std::string& value_): Component(caption), value(value_) { }

	void draw(BITMAP* buffer, int x, int y) const;
	int width() const;

private:
	std::string value;
};

class Numberfield : public Component {
public:
	Numberfield(const std::string& caption_, int value_): Component(caption), value(value_) { }

private:
	int value;
};

template <typename T>
class Select : public Component {
public:
	Select(): selected(0) { }

private:
	std::vector<T> selections;
	int selected;
};

class Checkbox : public Component {
public:
	Checkbox(const std::string& caption_): Component(caption), selected(false) { }

private:
	bool selected;
};

class Textarea : public Component {
public:
	Textarea(const std::string& caption_, const std::string& text_): Component(caption), text(text_) { }

private:
	std::string text;
};

#endif // MENU_H_INC

