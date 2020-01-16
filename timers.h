#ifndef TIMERS_H
#define TIMERS_H

#pragma once

#include <Arduino.h>
//#include <stdint.h>




class TimersClass {

private:
	typedef const __FlashStringHelper* pgmptr;
	struct TimerDataStruct {
		void(*callbackForRun)(void);
		uint32_t periodMSec;
		uint32_t lastTime;
		bool once;
		pgmptr info;
	};

	static TimerDataStruct timersData[16];
	static uint8_t timersCount;
public:
	static void doLoop();
	static void add(void(*callback)(void), uint32_t period, pgmptr info);
	static void once(void(*callback)(void), uint32_t period);

};

extern TimersClass Timers;

#endif //TIMERS_H
