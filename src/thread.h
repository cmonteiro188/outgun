#ifndef THREAD_H_INC
#define THREAD_H_INC

#include <pthread.h>
#include "nassert.h"	// for STACK_GUARD

void threadRandomize();	// implemented in utility.cpp; does just { srand(time(0)); }; not inlined to avoid extra headers here

class Thread {
	template<class Function>
	struct ThreadData0 {
		Function function;
		ThreadData0(Function fun) : function(fun) { }
	};
	template<class Function>
	static void* starter0(void* pv_arg) {
		unsigned long stackGuard = STACK_GUARD;	(void)stackGuard;
		threadRandomize();	// help for implementations on which each thread has it's own random seed
		ThreadData0<Function>* tdata = static_cast<ThreadData0<Function>*>(pv_arg);
		tdata->function();
		delete tdata;
		return 0;
	}

	template<class Function, class ArgumentT>
	struct ThreadData1 {
		Function function;
		ArgumentT arg;
		ThreadData1(Function fun, ArgumentT arg_) : function(fun), arg(arg_) { }
	};
	template<class Function, class ArgumentT>
	static void* starter1(void* pv_arg) {
		unsigned long stackGuard = STACK_GUARD;	(void)stackGuard;
		threadRandomize();	// help for implementations on which each thread has it's own random seed
		ThreadData1<Function, ArgumentT>* tdata = static_cast<ThreadData1<Function, ArgumentT>*>(pv_arg);
		tdata->function(tdata->arg);
		delete tdata;
		return 0;
	}

	pthread_t thread;
	bool running;	// set if running AND not detached

	// deny copying
	Thread(const Thread&) { }
	Thread& operator=(const Thread&) { return *this; }

public:
	Thread() : running(false) { }
	~Thread() { nAssert(!running); }

	template<class Function>
	static int startDetachedThread(Function fun) {
		ThreadData0<Function>* td = new ThreadData0<Function>(fun);
		pthread_t tthread;
		int val = pthread_create(&tthread, 0, starter0<Function>, td);
		if (val != 0)
			return val;
		nAssert(0 == pthread_detach(tthread));
		return 0;
	}
	template<class Function> static void startDetachedThread_assert(Function fun) { int val = startDetachedThread(fun); numAssert(val == 0, val); }

	template<class Function, class ArgumentT>
	static int startDetachedThread(Function fun, ArgumentT arg) {
		ThreadData1<Function, ArgumentT>* td = new ThreadData1<Function, ArgumentT>(fun, arg);
		pthread_t tthread;
		int val = pthread_create(&tthread, 0, starter1<Function, ArgumentT>, td);
		if (val != 0)
			return val;
		nAssert(0 == pthread_detach(tthread));
		return 0;
	}
	template<class Function, class ArgumentT> static void startDetachedThread_assert(Function fun, ArgumentT arg) { int val = startDetachedThread(fun, arg); numAssert(val == 0, val); }

	template<class Function>
	int start(Function fun) {
		nAssert(!running);
		running = true;
		ThreadData0<Function>* td = new ThreadData0<Function>(fun);
		return pthread_create(&thread, 0, starter0<Function>, td);
	}
	template<class Function> void start_assert(Function fun) { int val = start(fun); numAssert(val == 0, val); }

	template<class Function, class ArgumentT>
	int start(Function fun, ArgumentT arg) {
		nAssert(!running);
		running = true;
		ThreadData1<Function, ArgumentT>* td = new ThreadData1<Function, ArgumentT>(fun, arg);
		return pthread_create(&thread, 0, starter1<Function, ArgumentT>, td);
	}
	template<class Function, class ArgumentT> void start_assert(Function fun, ArgumentT arg) { int val = start(fun, arg); numAssert(val == 0, val); }

	bool isRunning() const { return running; }	// note: this tells if there's need for join or detach rather than if the thread is active
	void join(bool acceptRecursive = false) {
		nAssert(running);
		running = false;
		int ret = pthread_join(thread, 0);
		numAssert(ret == 0 || (acceptRecursive && ret == EDEADLK), ret);
	}
	void detach() {
		nAssert(running);
		running = false;
		nAssert(0 == pthread_detach(thread));
	}
};

#endif
