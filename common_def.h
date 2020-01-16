#ifndef COMMON_DEF_H
#define COMMON_DEF_H

#pragma once

#include <Arduino.h>
extern "C" {
#include <time.h>
}


//#include "flashstr.h"

#include <Streaming.h>
#include "debug.h"

#include "pins_names.h"

#define sizeofArray(array) (sizeof(array) / sizeof(array[0]))

const char * toHex(uint8_t x);
const char * toHex(uint16_t x);
const char * toBin(uint8_t x);

const char * strrchar(char* ptr, char c);

uint16_t crc16(const uint8_t* input, uint16_t len, uint16_t crc = 0);
uint8_t crc8(const uint8_t *addr, uint8_t len);


template <typename T>
bool inrange(T arg, T lv, T rv, const char* Desc) {
	if ((lv <= arg) && (arg <= rv))
		return true;
	if (Desc)
		DEBUG_PRINT(Desc << (" ") << arg << (" not in range ") << lv << ("...") << rv);
	return false;
};
template <typename T>
bool inrange(T arg, T lv, T rv) {
	return ((lv <= arg) && (arg <= rv));
};

// format time YYYYmmDDTHHMMSS
const char * time2str(time_t t, const char * format = NULL);
// time from string YYYYmmDDTHHMMSS
time_t str2time(const char* str);


#endif // !COMMON_DEF_H
