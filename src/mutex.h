#ifndef MUTEX_H_INC
#define MUTEX_H_INC

#include <pthread.h>

class MutexHolder {
	pthread_mutex_t mutex;

public:
	MutexHolder() { pthread_mutex_init(&mutex, 0); }
	~MutexHolder() { pthread_mutex_destroy(&mutex); }
	void lock() { pthread_mutex_lock(&mutex); }
	void unlock() { pthread_mutex_unlock(&mutex); }
	pthread_mutex_t* operator&() { return &mutex; }
};

extern MutexHolder nlOpenMutex;

#endif
