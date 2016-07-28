#ifndef SEE_H
#define SEE_H

#include "Types.h"

void InitSEE();
uint8 SEEW(Move m);
uint8 SEEB(Move m);

inline uint8 SEE(Move m)
{
	return WTM? SEEW(m): SEEB(m);
}


#endif
