
#pragma once

#if defined(ESP8266)

#define DISABLE_TIMER_INTERRUPTS() ETS_FRC1_INTR_DISABLE()
#define ENABLE_TIMER_INTERRUPTS() ETS_FRC1_INTR_ENABLE()
#define ADD_TIMER_INTERRUPT(proc, us) timer1_isr_init(); \
	timer1_attachInterrupt(proc); \
	timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP); \ 
	timer1_write((clockCyclesPerMicrosecond() / 16) * us);

#endif



#define ESP_getChipIdStr() String(ESP.getChipId(), 16)
#define ESP_wdtEnable(interval) ESP.wdtEnable(interval)
#define ESP_wdtFeed() ESP.wdtFeed()

