#ifndef DEBUG_H_INC
#define DEBUG_H_INC

#ifdef NDEBUG

class ValidityChecker {
public:
	void checkValidity() const { }
};

typedef ValidityChecker PointerLeakBuffer;

#else	// NDEBUG

#include <iostream>
#include <stdio.h>
#include "nassert.h"

class ProfileTimer {
	const char* n;
	unsigned long val;

public:
	ProfileTimer(const char* name) : n(name), val(0) { }
	~ProfileTimer() { std::cerr << n << " : " << val << '\n'; }
	unsigned long* getPtr() { return &val; }
};

extern volatile unsigned long time_counter;
class Profiler {
	unsigned long* sump;
	unsigned long start;

public:
	Profiler(ProfileTimer& timer) : sump(timer.getPtr()), start(time_counter) { }
	~Profiler() { *sump += time_counter - start; }
};

class ValidityChecker {
	int check;

  public:
	ValidityChecker() : check(0xC044EC7) { }
	void checkValidity() const
		{ numAssert((unsigned)this>0x10000 && (unsigned)this<0xFFFF0000, (int)this); nAssert(check!=0xDE7E7ED); nAssert(check==0xC044EC7); }
	~ValidityChecker() { checkValidity(); check=0xDE7E7ED; }
};

#pragma pack(push, 1)
template<int size> class PointerLeakBuffer : private ValidityChecker {
	char buffer[size];

  public:
	PointerLeakBuffer() {
		for (int i=0; i<size; i++)
			buffer[i]=0x11;
	}
	void checkValidity() const {
		for (int i=0; i<size; i++)
			if (buffer[i]!=0x11) {
				printf("Leak buffer (@%p-%p) changed at %d (%p), data: ", &buffer[0], &buffer[size-1], i, &buffer[i]);
				for (int x=i; x<size && buffer[x]!=0x11; x++)
					printf("%02X ", buffer[x]);
				printf("\nas string: ");
				for (; i<size && buffer[i]!=0x11; i++)
					printf("%c", buffer[i]);
				printf("\n\n");
			}
		ValidityChecker::checkValidity();
	}
	~PointerLeakBuffer() {
		checkValidity();
	}
};
#pragma pack(pop)

#endif	// NDEBUG

#endif	// DEBUG_H_INC

