#ifndef MUTEX_H_INC
#define MUTEX_H_INC

#include <pthread.h>
#include "nassert.h"
#include "utility.h"

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

// Threadsafe: Wrapper of an object of type ObjT providing a thread safe very limited interface.
template<class ObjT>
class Threadsafe {
	mutable MutexHolder mutex;
	volatile ObjT obj;

public:
	Threadsafe() { }
	Threadsafe(const ObjT& o) : obj(o) { }

	Threadsafe& operator=(const ObjT& o) { mutex.lock(); volatile_ref_cast<ObjT>(obj) = o; mutex.unlock(); return *this; }
	ObjT read() const { mutex.lock(); ObjT o = volatile_ref_cast<const ObjT>(obj); mutex.unlock(); return o; }	// Get a *copy* of the object

	// for more complex operations, use lock(), access() and unlock()
	void lock() const { mutex.lock(); }
	void unlock() const { mutex.unlock(); }
	      ObjT& access()       { return obj; }	// use obj only between lock() and unlock()
	const ObjT& access() const { return obj; }	// use obj only between lock() and unlock()
};

extern MutexHolder nlOpenMutex;

#endif
