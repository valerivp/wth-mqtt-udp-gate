#include "common_def.h"

#if defined(__AVR__)
#include <util/crc16.h>
#endif


static const char chars16[] = "0123456789ABCDEF";

const char * toHex(uint8_t x) {
	static char res[3] = "00";
	res[0] = chars16[x >> 4];
	res[1] = chars16[x & 0xf];
	return res;
}

const char * toHex(uint16_t x) {
	//DEBUG_STACK_PRINT(x);
	static char res[5] = "0000";
	//DEBUG_PRINT((x >> 12));
	//DEBUG_PRINT(chars16[x >> 12]);
	res[0] = chars16[x >> 12];
	res[1] = chars16[(x >> 8) & 0xf];
	res[2] = chars16[(x >> 4) & 0xf];
	res[3] = chars16[x & 0xf];
	return res;
}
const char * toBin(uint8_t x) {
	static char res[9] = "00000000";
	for (int8_t i = 0; i < 8; i++) {
		bitWrite(res[7 - i], 0, bitRead(x, i));
	}
	return res;
}

const char * strrchr(char* ptr, char c) {
	const char* t = NULL;

	for (; *ptr; ptr++) {
		if (*ptr == c)
			t = ptr;
	}
	return t;
}


uint16_t crc16(const uint8_t* input, uint16_t len, uint16_t crc)
{
#if defined(__AVR__)
	for (uint16_t i = 0; i < len; i++) {
		crc = _crc16_update(crc, input[i]);
	}
#else
	static const uint8_t oddparity[16] =
	{ 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 };

	for (uint16_t i = 0; i < len; i++) {
		// Even though we're just copying a byte from the input,
		// we'll be doing 16-bit computation with it.
		uint16_t cdata = input[i];
		cdata = (cdata ^ crc) & 0xff;
		crc >>= 8;

		if (oddparity[cdata & 0x0F] ^ oddparity[cdata >> 4])
			crc ^= 0xC001;

		cdata <<= 6;
		crc ^= cdata;
		cdata <<= 1;
		crc ^= cdata;
	}
#endif
	return crc;
}

//
// Compute a Dallas Semiconductor 8 bit CRC. These show up in the ROM
// and the registers.  (note: this might better be done without to
// table, it would probably be smaller and certainly fast enough
// compared to all those delayMicrosecond() calls.  But I got
// confused, so I use this table from the examples.)
//
uint8_t crc8(const uint8_t *addr, uint8_t len)
{
	uint8_t crc = 0;
	uint8_t inbyte = *addr++;
	for (uint8_t i = 8; i; i--) {
		uint8_t mix = (crc ^ inbyte) & 0x01;
		crc >>= 1;
		if (mix) crc ^= 0x8C;
		inbyte >>= 1;
	}
	return crc;
}



// format time YYYYmmDDTHHMMSS
const char * time2str(tm* timestruct, const char * format) {
	static char TimeStringBuf[64]; //20140527T000000

	strftime(TimeStringBuf, sizeof(TimeStringBuf), (format ? format : "%Y%m%dT%H%M%S"), timestruct);
	return TimeStringBuf;
}
const char * time2str(time_t t, const char * format) {
	tm* timestruct = localtime(&t);
	return time2str(timestruct, format);
}


// time from string YYYYmmDDTHHMMSS
time_t str2time(const char* str) {
	Serial.println(String(str));
	if (strlen(str) != 15 || str[8] != 'T')
		return 0;
	char * end;
	unsigned long d = strtoul(str, &end, 10);
	if (end != &str[8])
		return 0;
	unsigned long t = strtoul(&str[9], &end, 10);
	if (end != &str[15])
		return 0;

	tm dt;
	dt.tm_mday = d % 100; if (!inrange((int8_t)dt.tm_mday, (int8_t)1, (int8_t)31, "day")) return 0;
	dt.tm_mon = (d / 100) % 100 - 1; if (!inrange((int8_t)(dt.tm_mon + (int8_t)1), (int8_t)1, (int8_t)12, "mon")) return 0;
	dt.tm_year = d / 10000 - 1900; if (!inrange(dt.tm_year + 1900, 2000, 2100, "year")) return 0;
	dt.tm_sec = t % 100; if (!inrange((int8_t)dt.tm_sec, (int8_t)0, (int8_t)59, "sec")) return 0;
	dt.tm_min = t / 100 % 100; if (!inrange((int8_t)dt.tm_min, (int8_t)0, (int8_t)59, "min")) return 0;
	dt.tm_hour = t / 10000; if (!inrange((int8_t)dt.tm_hour, (int8_t)0, (int8_t)24, "hour")) return 0;
	dt.tm_isdst = 0;
	return mktime(&dt);
}
