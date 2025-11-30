#ifndef DISPLAY.H
#define DISPLAY.H

#include "tm1637.h"

extern tm1637_led_t *led;
void display_task(void* pvParameters);

#endif