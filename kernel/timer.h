#ifndef __TIMER_H__
#define __TIMER_H__

#include "system.h"

/**
 * Maybe this actually doesn't work so well because some ups disable
 * interrupts. It might be worth looking into cpu instructions that get time.
 * Though now that I thin about it, that would be a change to the
 * implementation of mark_time and not this macro - so maybe this comment
 * doesn't belong here.
 * Also, this macro doesn't account for integer overflow.
 */
#define time_op(op, delta, val) {             							\
    int start_time, end_time; 								            \
																        \
    start_time = mark_time(); 	                                        \
	val = op;                           								\
	end_time = mark_time();		  								        \
																        \
	delta = end_time - start_time;										\
}

void init_timer(void);
void timer_phase(int hz);
void timer_handler(struct registers* r);
void timer_wait(int);
void timer_install(void);

int mark_time(void);

#endif //__TIME_H__
