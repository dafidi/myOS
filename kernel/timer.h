#ifndef __TIMER_H__
#define __TIMER_H__

#include "system.h"

void timer_phase(int hz);
void timer_handler();
void timer_wait_timer_install();
void timer_wait();

#endif //__TIME_H__
