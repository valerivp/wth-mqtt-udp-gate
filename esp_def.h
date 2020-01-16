
#pragma once

#ifdef ESP32
//#include <FS.h>
#endif // ESP32


#if defined(ESP8266)

#define DISABLE_TIMER_INTERRUPTS() ETS_FRC1_INTR_DISABLE()
#define ENABLE_TIMER_INTERRUPTS() ETS_FRC1_INTR_ENABLE()
#define ADD_TIMER_INTERRUPT(proc, us) timer1_isr_init(); \
	timer1_attachInterrupt(proc); \
	timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP); \ 
	timer1_write((clockCyclesPerMicrosecond() / 16) * us);

#elif defined(ESP32)

#define DISABLE_TIMER_INTERRUPTS()
#define ENABLE_TIMER_INTERRUPTS()
#define ADD_TIMER_INTERRUPT(proc, us) auto timer_tmp = timerBegin(0, 80, true); \
	timerAttachInterrupt(timer_tmp, proc, true); \
	timerAlarmWrite(timer_tmp, us * 1000, true); \
	timerAlarmEnable(timer_tmp);

#endif



#if defined(ESP8266)

#define ESP_getChipIdStr() String(ESP.getChipId(), 16)
#define ESP_wdtEnable(interval) ESP.wdtEnable(interval)
#define ESP_wdtFeed() ESP.wdtFeed()

#elif defined(ESP32)

#define ESP_getChipIdStr() String((uint16_t)(ESP.getEfuseMac() >> 32), 16) + String((uint32_t)ESP.getEfuseMac(), 16)
#define ESP_wdtEnable(interval)
#define ESP_wdtFeed()


#endif


#ifdef ESP32
//extern fs::FS SPIFFS;
#endif // ESP32

#if defined(ESP8266)


#elif defined(ESP32)


#endif


