#ifndef GAMEMOD_H_INC
#define GAMEMOD_H_INC

#include <sstream>
#include <string>
#include <vector>

#include "function_utility.h"
#include "world.h"	// for WorldSettings::Team_balance

class LogSet;
class MapInfo;

class GamemodSetting {	// base class
public:
	virtual ~GamemodSetting() { }
	virtual bool set(LogSet& log, const std::string& value) = 0;	// returns false if the value is not accepted
	bool matchCommand(const std::string& command) const { return command == name; }

protected:
	GamemodSetting(std::string name_) : name(name_) { }
	bool basicErrorMessage(LogSet& log, const std::string& value, const std::string& expect);	// always returns false to provide easy returns

	std::string name;
};

// generic setting prototypes

template<class ValT>
class GS_IntT : public GamemodSetting {
public:
	typedef std::numeric_limits<ValT> lim;

	GS_IntT(const std::string& name, ValT* pVar, ValT min_ = lim::min(), ValT max_ = lim::max(), int mul_ = 1, int add_ = 0, bool allow0_ = false)
		: GamemodSetting(name), var(pVar), vmin(min_), vmax(max_), mul(mul_), add(add_), allow0(allow0_) { }
	bool set(LogSet& log, const std::string& value);

private:
	ValT* var;
	ValT vmin, vmax;
	int mul, add;
	bool allow0;
};

typedef GS_IntT<int> GS_Int;
typedef GS_IntT<NLulong> GS_Ulong;

template<class ValT>
class GS_FloatT : public GamemodSetting {
public:
	GS_FloatT(const std::string& name, ValT* pVar, float min_ = FLT_MIN, float max_ = FLT_MAX, float mul_ = 1., float add_ = 0.)
		: GamemodSetting(name), var(pVar), vmin(min_), vmax(max_), mul(mul_), add(add_) { }
	bool set(LogSet& log, const std::string& value);

private:
	ValT* var;
	float vmin, vmax, mul, add;
};

typedef GS_FloatT<float> GS_Float;
typedef GS_FloatT<double> GS_Double;

class GS_Boolean : public GamemodSetting {
public:
	GS_Boolean(const std::string& name, bool* pVar) : GamemodSetting(name), var(pVar) { }
	bool set(LogSet& log, const std::string& value);

private:
	bool* var;
};

class GS_String : public GamemodSetting {
public:
	GS_String(const std::string& name, std::string* pVar) : GamemodSetting(name), var(pVar) { }
	bool set(LogSet&, const std::string& value) {
		*var = value;
		return true;
	}

private:
	std::string* var;
};

class GS_AddString : public GamemodSetting {
public:
	GS_AddString(const std::string& name, std::vector<std::string>* pVar) : GamemodSetting(name), var(pVar) { }
	bool set(LogSet&, const std::string& value) {
		var->push_back(value);
		return true;
	}

private:
	std::vector<std::string>* var;
};

class GS_ForwardStr : public GamemodSetting {
public:
	GS_ForwardStr(const std::string& name, HookFunctionBase1<void, const std::string&>& pVar) : GamemodSetting(name), var(pVar) { }
	bool set(LogSet&, const std::string& value) {
		var(value);
		return true;
	}

private:
	HookFunctionBase1<void, const std::string&>& var;
};

class GS_ForwardInt : public GamemodSetting {
public:
	GS_ForwardInt(const std::string& name, const std::string& expect_, HookFunctionBase1<bool, int>& check_, HookFunctionBase1<bool, int>& pVar)
		: GamemodSetting(name), expect(expect_), checkValue(check_), var(pVar) { }
	bool set(LogSet& log, const std::string& value);

private:
	const std::string expect;
	HookFunctionBase1<bool, int>& checkValue;
	HookFunctionBase1<bool, int>& var;
};

// specific settings that require special handling

class GS_Map : public GamemodSetting {
public:
	GS_Map(const std::string& name, std::vector<MapInfo>* pVar) : GamemodSetting(name), var(pVar) { }
	bool set(LogSet& log, const std::string& value);

private:
	std::vector<MapInfo>* var;
};

class GS_PowerupNum : public GamemodSetting {
public:
	GS_PowerupNum(const std::string& name, int* pVar, bool* pPercentFlag) : GamemodSetting(name), var(pVar), percentFlag(pPercentFlag) { }
	bool set(LogSet& log, const std::string& value);

private:
	int* var;
	bool* percentFlag;
};

class GS_Balance : public GamemodSetting {
public:
	GS_Balance(const std::string& name, WorldSettings::Team_balance* pVar) : GamemodSetting(name), var(pVar) { }
	bool set(LogSet& log, const std::string& value);

private:
	WorldSettings::Team_balance* var;
};

// template implementation

template<class ValT>
bool GS_IntT<ValT>::set(LogSet& log, const std::string& value) {
	static const std::istream::traits_type::int_type eof_ch = std::istream::traits_type::eof();
	std::istringstream rd(trim(value));
	ValT val;
	rd >> val;
	if (!rd || rd.peek() != eof_ch || ((val < vmin || val > vmax) && !(val == 0 && allow0))) {
		std::string orZero;
		if (allow0)
			orZero = ", or 0";
		const bool minBound = (vmin != lim::min() || static_cast<float>(lim::min()) > -100000.);	// compare floats to avoid range and signedness warnings
		const bool maxBound = (vmax != lim::max() || static_cast<float>(lim::max()) <  100000.);
		if (minBound && maxBound) {
			if (vmax == vmin + 1)
				return basicErrorMessage(log, value, std::string() + itoa(vmin) + " or " + itoa(vmax) + orZero);
			else
				return basicErrorMessage(log, value, std::string() + "an integer, between " + itoa(vmin) + " and " + itoa(vmax) + " inclusive" + orZero);
		}
		else if (maxBound)
			return basicErrorMessage(log, value, std::string() + "an integer, at most " + itoa(vmax) + orZero);
		else if (minBound)
			return basicErrorMessage(log, value, std::string() + "an integer, at least " + itoa(vmin) + orZero);
		else
			return basicErrorMessage(log, value, "an integer");
		return false;
	}
	*var = val * mul + add;
	return true;
}

template<class ValT>
bool GS_FloatT<ValT>::set(LogSet& log, const std::string& value) {
	static const std::istream::traits_type::int_type eof_ch = std::istream::traits_type::eof();
	std::istringstream rd(trim(value));
	float val;
	rd >> val;
	if (!rd || rd.peek() != eof_ch || val < vmin || val > vmax) {
		if (vmin == FLT_MIN && vmax == FLT_MAX)
			return basicErrorMessage(log, value, "a real number");
		else if (vmin == FLT_MIN)
			return basicErrorMessage(log, value, std::string() + "a real number, at most " + fcvt(vmax));
		else if (vmax == FLT_MAX)
			return basicErrorMessage(log, value, std::string() + "a real number, at least " + fcvt(vmin));
		else
			return basicErrorMessage(log, value, std::string() + "a real number, between " + fcvt(vmin) + " and " + fcvt(vmax) + ", inclusive");
	}
	*var = val * mul + add;
	return true;
}

#endif
