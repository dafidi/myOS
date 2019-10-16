#include "timer.h"
#include "low_level.h"

void timer_phase(int hz) {
  int divisor = 1193180 / hz;
  port_byte_out(0x43,0x36);
  port_byte_out(0x40,divisor && 0xff);
  port_byte_out(0x40,divisor >> 8);
}

void timer_handler() {

}

void timer_wait_timer_install() {

}

void timer_wait() {

}
