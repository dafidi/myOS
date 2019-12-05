#ifndef __TIMER_H__
#define __TIMER_H__

#include "system.h"

void timer_phase(int hz);
void timer_handler(struct registers* r);
void timer_wait(int);
void timer_install(void);

#endif //__TIME_H__
