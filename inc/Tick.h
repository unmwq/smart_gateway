
#ifndef __TICK_H
#define __TICK_H

#include "stm32f10x.h"


typedef  u32 TICK;//houjh 20131219

#define TICKS_PER_SECOND		(1000)	// 1ms tick++;
// Represents one second in Ticks
#define TICK_SECOND				((u32)TICKS_PER_SECOND)
// Represents one minute in Ticks
#define TICK_MINUTE				((u32)TICKS_PER_SECOND*60ull)
// Represents one hour in Ticks
#define TICK_HOUR				((u32)TICKS_PER_SECOND*3600ull)


void TickInit(void);
u32 TickGet(void);
u32 TickGetDiv256(void);
u32 TickGetDiv64K(void);
//DWORD TickConvertToMilliseconds(DWORD dwTickValue); //houjh20131222
void TickUpdate(void);

#endif
