#include <pthread.h>
#include "thread.h"

void Thread::randomize() {
	srand(time(0));
}

int Thread::doStart(pthread_t* pThread, void* (*function)(void*), void* argument, bool detached, int priority) {
	pthread_attr_t attr;
	int val = pthread_attr_init(&attr);

	if (val != 0)
		return val;
	nAssert(0 == pthread_attr_setdetachstate(&attr, detached ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE));

	sched_param param;
	param.sched_priority = priority;
	nAssert(0 == pthread_attr_setschedparam(&attr, &param));

	val = pthread_create(pThread, &attr, function, argument);

	nAssert(0 == pthread_attr_destroy(&attr));

	return val;
}

void Thread::doSetPriority(pthread_t thread, int priority) {
	sched_param param;
	param.sched_priority = priority;
	nAssert(0 == pthread_setschedparam(thread, SCHED_OTHER, &param));
}

void Thread::join(bool acceptRecursive) {
	nAssert(running);
	running = false;
	int ret = pthread_join(thread, 0);
	numAssert(ret == 0 || (acceptRecursive && ret == EDEADLK), ret);
}

void Thread::detach() {
	nAssert(running);
	running = false;
	nAssert(0 == pthread_detach(thread));
}

