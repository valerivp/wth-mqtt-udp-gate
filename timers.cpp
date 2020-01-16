#include <Streaming.h>


#define LOCAL_H
#include "timers.h"
#undef LOCAL_H


/*
  Modul Timers for ESP8266
  part of Arduino Mega Server project
*/
//template <typename T> inline Print & operator << (Print &s, T n) { s.print(n); return s; }
//#define eol F("\n")

#define sizeofArray(array) (sizeof(array) / sizeof(array[0]))

TimersClass Timers;

TimersClass::TimerDataStruct TimersClass::timersData[];// __attribute__((section(".noinit")));
uint8_t TimersClass::timersCount = 0;



void TimersClass::add(void(*callback)(void), uint32_t period, pgmptr info)
{
	if (timersCount < sizeofArray(timersData)) {
		TimerDataStruct & data = timersData[timersCount];
		data.callbackForRun = callback;
		data.periodMSec = period;
		data.lastTime = 0;
		data.once = false;
		data.info = info;
		timersCount++;
		Serial << F("Add timer: ") << info << F(": ") << period << F("ms") << endl;
	}
	else {
		Serial << F("Error: too many timers: ") << info << endl;
	}
}

void TimersClass::once(void(*callback)(void), uint32_t period)
{
	if (timersCount < sizeofArray(timersData)) {
		TimerDataStruct & data = timersData[timersCount];
		data.callbackForRun = callback;
		data.periodMSec = period;
		data.lastTime = millis();
		data.once = true;
		timersCount++;
	}
	else {
		Serial << F("Error: too many timers") << endl;
	}
}


void TimersClass::doLoop()
{
	unsigned long cutrTime = millis();
	for (int8_t i = 0; i < timersCount; i++) {
		TimerDataStruct & data = timersData[i];
		if ((cutrTime - data.lastTime) > data.periodMSec) {
			data.lastTime = cutrTime;
			data.callbackForRun();
			if (data.once) {
				memmove((void*)&timersData[i], (void*)&timersData[i + 1], sizeof(TimerDataStruct) * (timersCount - i - 1));
				timersCount--;
				i--;
				//for (int8_t j = i; j < timersCount; j++)
				  //  data[j] = data[j + 1];
			}else if((millis() - cutrTime) > 1000){
				Serial << F("It was more than one second: ") << data.info << endl;

			}
		}
	}
}
