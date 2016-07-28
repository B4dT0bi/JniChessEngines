
// protocol.h

#include <windows.h>

#ifndef PROTOCOL_H
#define PROTOCOL_H

// includes

#include "util.h"

// variables

extern CRITICAL_SECTION CriticalSection; 

extern int NumberThreads;
// functions

extern void loop  ();
extern void event ();
extern void book_parameter();
extern void init_threads();

extern void get   (char string[], int size);
extern void send  (const char format[], ...);

#endif // !defined PROTOCOL_H

// end of protocol.h

