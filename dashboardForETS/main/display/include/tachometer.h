#ifndef TACHOMETER_H
#define TACHOMETER_H

#include "stdio.h"
#include <lvgl.h>

extern lv_obj_t *scale;
extern lv_obj_t *needle;

void create_tachometer(void);

#endif // TACHOMETER_H
