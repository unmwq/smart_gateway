#ifndef __MAIN_H
#define __MAIN_H	

#include "key_led.h"
#include "delay.h"
#include "detector.h"

#include "jq6500.h"
#include "tick.h"
#include "delay.h"
#include "sys.h"
#include "wifi.h"
#include <Tick.h>
#include "rtc.h"
#include <temp-humi.h>
#include "timer.h"
#include "usart.h"
#include "cc1101.h"
#include "store.h"

#define CLI()        __set_PRIMASK(1)           
                                      
#define SEI()        __set_PRIMASK(0)

void CPU_IntDis (void);
void CPU_IntEn (void);

#endif