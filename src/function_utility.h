#ifndef FUNCTION_UTILITY_INC
#define FUNCTION_UTILITY_INC

// function_utility.h : template classes to wrap different kinds of function objects

// HookFunctionBase# provides a common interface for storing and accessing function references
// HookFunctionHolder# manages a pointer to HookFunctionBase and behaves just like a regular function pointer would
// Hook# is a HookFunctionHolder#-like object that can be unbound and provides a different (more explicit) call syntax
// Hookable# is a base class to allow easy inclusion of a Hook# in a class (use with care)

// RedirectToMemFun# (deriving from HookFunctionBase#) is a helper allowing function objects bound to a member function of an object

// generating new versions with different amounts of arguments should be easy when needed; shame creating them can't be circumvented


// 0-argument HookFunctionBase#, Hook#, Hookable# :

template<class RetT>
class HookFunctionBase0 {
public:
	virtual ~HookFunctionBase0() { }
	virtual RetT operator()() = 0;
	virtual HookFunctionBase0* clone() = 0;
};

template<class RetT>
class HookFunctionHolder0 {
public:
	typedef HookFunctionHolder0 ThisT;
	typedef HookFunctionBase0<RetT> FunctionT;

	HookFunctionHolder0(FunctionT* fn) : hookFn(fn) { }
	HookFunctionHolder0(const ThisT& o) : hookFn(o.hookFn->clone()) { }
	~HookFunctionHolder0() { delete hookFn; }
	void operator=(const ThisT& o) { delete hookFn; hookFn = o.hookFn->clone(); }

	RetT operator()() { return (*hookFn)(); }

private:
	FunctionT* hookFn;
};

template<class RetT>
class Hook0 {
public:
	typedef HookFunctionBase0<RetT> FunctionT;

	Hook0() : hookFn(0) { }
	~Hook0() { free(); }
	void set(FunctionT* fn) { free(); hookFn = fn; }	// the ownership is transferred
	bool active() const { return hookFn != 0; }
	RetT call() { if (hookFn) return (*hookFn)(); else return RetT(); }

private:
	void free() { if (hookFn) delete hookFn; }

	FunctionT* hookFn;
};

template<class RetT>
class Hookable0 {
public:
	typedef typename Hook0<RetT>::FunctionT HookFunctionT;

	virtual ~Hookable0() { }
	void setHook(HookFunctionT* fn) { hook.set(fn); }	// the ownership is transferred
	bool isHooked() const { return hook.active(); }

protected:
	RetT callHook() { return hook.call(); }

private:
	Hook0<RetT> hook;
};

// 1-argument HookFunctionBase#, Hook#, Hookable# :

template<class RetT, class Arg1T>
class HookFunctionBase1 {
public:
	virtual ~HookFunctionBase1() { }
	virtual RetT operator()(Arg1T) = 0;
	virtual HookFunctionBase1* clone() = 0;
};

template<class RetT, class Arg1T>
class HookFunctionHolder1 {
public:
	typedef HookFunctionHolder1 ThisT;
	typedef HookFunctionBase1<RetT, Arg1T> FunctionT;

	HookFunctionHolder1(FunctionT* fn) : hookFn(fn) { }
	HookFunctionHolder1(const ThisT& o) : hookFn(o.hookFn->clone()) { }
	~HookFunctionHolder1() { delete hookFn; }
	void operator=(const ThisT& o) { delete hookFn; hookFn = o.hookFn->clone(); }

	RetT operator()(Arg1T a1) { return (*hookFn)(a1); }

private:
	FunctionT* hookFn;
};

template<class RetT, class Arg1T>
class Hook1 {
public:
	typedef HookFunctionBase1<RetT, Arg1T> FunctionT;

	Hook1() : hookFn(0) { }
	~Hook1() { free(); }
	void set(FunctionT* fn) { free(); hookFn = fn; }	// the ownership is transferred
	bool active() const { return hookFn != 0; }
	RetT call(Arg1T a1) { if (hookFn) return (*hookFn)(a1); else return RetT(); }

private:
	void free() { if (hookFn) delete hookFn; }

	FunctionT* hookFn;
};

template<class RetT, class Arg1T>
class Hookable1 {
public:
	typedef typename Hook1<RetT, Arg1T>::FunctionT HookFunctionT;

	virtual ~Hookable1() { }
	void setHook(HookFunctionT* fn) { hook.set(fn); }	// the ownership is transferred
	bool isHooked() const { return hook.active(); }

protected:
	RetT callHook(Arg1T a1) { return hook.call(a1); }

private:
	Hook1<RetT, Arg1T> hook;
};

// 2-argument HookFunctionBase#, Hook#, Hookable# :

template<class RetT, class Arg1T, class Arg2T>
class HookFunctionBase2 {
public:
	virtual ~HookFunctionBase2() { }
	virtual RetT operator()(Arg1T, Arg2T) = 0;
	virtual HookFunctionBase2* clone() = 0;
};

template<class RetT, class Arg1T, class Arg2T>
class HookFunctionHolder2 {
public:
	typedef HookFunctionHolder2 ThisT;
	typedef HookFunctionBase2<RetT, Arg1T, Arg2T> FunctionT;

	HookFunctionHolder2(FunctionT* fn) : hookFn(fn) { }
	HookFunctionHolder2(const ThisT& o) : hookFn(o.hookFn->clone()) { }
	~HookFunctionHolder2() { delete hookFn; }
	void operator=(const ThisT& o) { delete hookFn; hookFn = o.hookFn->clone(); }

	RetT operator()(Arg1T a1, Arg2T a2) { return (*hookFn)(a1, a2); }

private:
	FunctionT* hookFn;
};

template<class RetT, class Arg1T, class Arg2T>
class Hook2 {
public:
	typedef HookFunctionBase2<RetT, Arg1T, Arg2T> FunctionT;

	Hook2() : hookFn(0) { }
	~Hook2() { free(); }
	void set(FunctionT* fn) { free(); hookFn = fn; }	// the ownership is transferred
	bool active() const { return hookFn != 0; }
	RetT call(Arg1T a1, Arg2T a2) { if (hookFn) return (*hookFn)(a1, a2); else return RetT(); }

private:
	void free() { if (hookFn) delete hookFn; }

	FunctionT* hookFn;
};

template<class RetT, class Arg1T, class Arg2T>
class Hookable2 {
public:
	typedef typename Hook2<RetT, Arg1T, Arg2T>::FunctionT HookFunctionT;

	virtual ~Hookable2() { }
	void setHook(HookFunctionT* fn) { hook.set(fn); }	// the ownership is transferred
	bool isHooked() const { return hook.active(); }

protected:
	RetT callHook(Arg1T a1, Arg2T a2) { return hook.call(a1, a2); }

private:
	Hook2<RetT, Arg1T, Arg2T> hook;
};

// 3-argument HookFunctionBase#, Hook#, Hookable# :

template<class RetT, class Arg1T, class Arg2T, class Arg3T>
class HookFunctionBase3 {
public:
	virtual ~HookFunctionBase3() { }
	virtual RetT operator()(Arg1T, Arg2T, Arg3T) = 0;
	virtual HookFunctionBase3* clone() = 0;
};

template<class RetT, class Arg1T, class Arg2T, class Arg3T>
class HookFunctionHolder3 {
public:
	typedef HookFunctionHolder3 ThisT;
	typedef HookFunctionBase3<RetT, Arg1T, Arg2T, Arg3T> FunctionT;

	HookFunctionHolder3(FunctionT* fn) : hookFn(fn) { }
	HookFunctionHolder3(const ThisT& o) : hookFn(o.hookFn->clone()) { }
	~HookFunctionHolder3() { delete hookFn; }
	void operator=(const ThisT& o) { delete hookFn; hookFn = o.hookFn->clone(); }

	RetT operator()(Arg1T a1, Arg2T a2, Arg3T a3) { return (*hookFn)(a1, a2, a3); }

private:
	FunctionT* hookFn;
};

template<class RetT, class Arg1T, class Arg2T, class Arg3T>
class Hook3 {
public:
	typedef HookFunctionBase3<RetT, Arg1T, Arg2T, Arg3T> FunctionT;

	Hook3() : hookFn(0) { }
	~Hook3() { free(); }
	void set(FunctionT* fn) { free(); hookFn = fn; }	// the ownership is transferred
	bool active() const { return hookFn != 0; }
	RetT call(Arg1T a1, Arg2T a2, Arg3T a3) { if (hookFn) return (*hookFn)(a1, a2, a3); else return RetT(); }

private:
	void free() { if (hookFn) delete hookFn; }

	FunctionT* hookFn;
};

template<class RetT, class Arg1T, class Arg2T, class Arg3T>
class Hookable3 {
public:
	typedef typename Hook3<RetT, Arg1T, Arg2T, Arg3T>::FunctionT HookFunctionT;

	virtual ~Hookable3() { }
	void setHook(HookFunctionT* fn) { hook.set(fn); }	// the ownership is transferred
	bool isHooked() const { return hook.active(); }

protected:
	RetT callHook(Arg1T a1, Arg2T a2, Arg3T a3) { return hook.call(a1, a2, a3); }

private:
	Hook3<RetT, Arg1T, Arg2T, Arg3T> hook;
};

// 0-argument RedirectToMemFun

template<class HostClass, class ReturnT>
class RedirectToMemFun : public HookFunctionBase0<ReturnT> {
	HostClass* host;
	ReturnT (HostClass::*function)();

public:
	RedirectToMemFun(HostClass* h, ReturnT (HostClass::*memFun)()) : host(h), function(memFun) { }
	ReturnT operator()() { return (host->*function)(); }
	RedirectToMemFun* clone() { return new RedirectToMemFun(host, function); }
};

// 1-argument RedirectToMemFun

template<class HostClass, class ReturnT, class Arg1T>
class RedirectToMemFun1 : public HookFunctionBase1<ReturnT, Arg1T> {
	HostClass* host;
	ReturnT (HostClass::*function)(Arg1T);

public:
	RedirectToMemFun1(HostClass* h, ReturnT (HostClass::*memFun)(Arg1T)) : host(h), function(memFun) { }
	ReturnT operator()(Arg1T arg) { return (host->*function)(arg); }
	RedirectToMemFun1* clone() { return new RedirectToMemFun1(host, function); }
};

// 2-argument RedirectToMemFun

template<class HostClass, class ReturnT, class Arg1T, class Arg2T>
class RedirectToMemFun2 {
	HostClass* host;
	ReturnT (HostClass::*function)(Arg1T, Arg2T);

public:
	RedirectToMemFun2(HostClass* h, ReturnT (HostClass::*memFun)(Arg1T, Arg2T)) : host(h), function(memFun) { }
	ReturnT operator()(Arg1T arg1, Arg2T arg2) { return (host->*function)(arg1, arg2); }
};

#endif
