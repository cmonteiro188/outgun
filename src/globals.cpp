#include "commont.h"
#include "mutex.h"

// put here only those globals that don't have a module they naturally belong to; also keep globals to be eliminated in commont.cpp

//server timer (10Hz)
volatile int server_speed_counter = 0;

//client game timer (60Hz)
volatile int speed_counter = 0;
volatile int client_netsend_counter = 0;		//sub-counter for keypress sending

//this timer will be used to emulate a better clock()
volatile unsigned long time_counter = 0;

char directory_separator;

MutexHolder nlOpenMutex;
