#ifndef PINS_NAMES_H
#define PINS_NAMES_H


#pragma once
//#undef Pins_Arduino_h
#include "pins_arduino.h"



#if defined(ARDUINO_ESP8266_ESP01)

enum pins : uint8_t {
	D3 = 0,
	D4 = 2,
	GPIO0 = 0,
	GPIO2 = 2,
	RX = 3,
	TX = 1,
	LED = 3
};
#elif defined(ARDUINO_ESP8266_GENERIC) && defined(Sonoff)

enum pins : uint8_t {
	gpio14 = 14,
	rx = 3,
	tx = 1,
	led = 13,
	button = 0,
	relay = 12
};
#elif defined(ARDUINO_ESP8266_NODEMCU) || defined(ARDUINO_ESP8266_WEMOS_D1MINI)

enum pins : uint8_t {
	d0 = 16,
	d1 = 5, scl = 20,
	d2 = 4, sda = 20,
	d3 = 0,
	d4 = 2, led = 2,
	d5 = 14,
	d6 = 12,
	d7 = 13,
	d8 = 15,
	rx = 3,
	tx = 1,
	button = 0,

};

#endif

#endif