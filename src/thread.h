#ifndef THREAD_H_INC
#define THREAD_H_INC

#include <pthread.h>

class Thread {
	template<class Function>
	struct ThreadData0 {
		Function function;
		ThreadData0(Function fun) : function(fun) { }
	};
	template<class Function>
	static void* starter0(void* pv_arg) {
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
	int start(Function fun) {
		nAssert(!running);
		running = true;
		ThreadData0<Function>* td = new ThreadData0<Function>(fun);
		return pthread_create(&thread, 0, starter0<Function>, td);
	}

	template<class Function, class ArgumentT>
	int start(Function fun, ArgumentT arg) {
		nAssert(!running);
		running = true;
		ThreadData1<Function, ArgumentT>* td = new ThreadData1<Function, ArgumentT>(fun, arg);
		return pthread_create(&thread, 0, starter1<Function, ArgumentT>, td);
	}

	void join() {
		nAssert(running);
		running = false;
		nAssert(0 == pthread_join(thread, 0));
	}
	void detach() {
		nAssert(running);
		running = false;
		nAssert(0 == pthread_detach(thread));
	}
};

#endif
