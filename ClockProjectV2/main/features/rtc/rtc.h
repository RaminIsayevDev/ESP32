#ifndef RTC.H
#define RTC.H

extern struct tm RTC_timeinfo;
void RTC_task(void *pvParameters);

#endif