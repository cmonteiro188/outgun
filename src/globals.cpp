#include "commont.h"
#include "mutex.h"

// put here only those globals that don't have a module they naturally belong to; also keep globals to be eliminated in commont.cpp

volatile unsigned long server_speed_counter = 0;	// 10 Hz (100 ms) server frame counter
volatile unsigned long time_counter = 0;	// 200 Hz (5 ms) counter used by get_time() and for client frame timing

char directory_separator;
std::string wheregamedir;

MutexHolder nlOpenMutex;
