#ifndef LANGUAGE_H_INC
#define LANGUAGE_H_INC

#include <string>
#include <map>

class Language {
public:
    Language() { }
    ~Language() { }

    bool load(const std::string& lang);

    std::string gettext(const std::string& text) const;
    std::string code() const { return lang_code; }

private:
	std::map<std::string, std::string> texts;
	std::string lang_code;
};

extern Language language;

std::string _(const std::string& text);

#endif

