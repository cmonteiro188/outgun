#ifndef MUTEX_H_INC
#define MUTEX_H_INC

#include <pthread.h>
#include "nassert.h"

class MutexHolder {
	pthread_mutex_t mutex;

public:
	MutexHolder()  { nAssert(0 == pthread_mutex_init(&mutex, 0)); }
	~MutexHolder() { nAssert(0 == pthread_mutex_destroy(&mutex)); }
	void lock()    { nAssert(0 == pthread_mutex_lock(&mutex)); }
	void unlock()  { nAssert(0 == pthread_mutex_unlock(&mutex)); }
};

class MutexLock {
	MutexHolder& mutex;

public:
	MutexLock(MutexHolder& mutex_) : mutex(mutex_) { mutex.lock(); }
	~MutexLock() { mutex.unlock(); }
};

extern MutexHolder nlOpenMutex;

#endif
