// (c) valerivp@gmail.com
#ifndef treceiver_H
#define treceiver_H

#include <Arduino.h>
//#include <stdint.h>
//#include <string.h>

#pragma once

class THReceiverClass {
	// type 0
	/*Датчик посылает 7 раз по 36 импульсов:
	§  Заголовок от 0 до 7
	§  Батарея: 8
	§  Канал: от 10 до 11
	§  Температура: от 12 до 24, в десятых долях градуса цельсия.
	§  Влажность: от 25 до 31
	§  неизвестно: от 32 до 39

	*/

	// type 1
	/*Датчик посылает 6 раз по 36 импульсов:
	Данные передаются импульсами низкого уровня разной длительности, разделенными импульсами высокого уровня постоянной длительности.
	Таким образом, мы проверяем каждый из импульсов, чтобы определить 0 или 1. Длинный импульс означает 1, а короткий импульс означает 0. В результате получаем следующий вывод:
	010111110011100000001100101100101011
	Каждая группа бит имеет определенный смысл:
	§  Заголовок от 0 до 4
	§  ID: от 5 до 11
	§  Батарея: 12
	§  TX режим: 13
	§  Канал: от 14 до 15
	§  Температура: от 16 до 27
	§  Влажность: от 28 до 35
	0101 1111|0011 1 0 00| 00001100|1011 0010|1011

	§  Заголовок определяется как двоичное число 0101, фактически получается это часть ID
	§  ID определяется как двоичное число, генерируется при рестарте передатчика
	§  Батарея определяет состояние аккумулятора. для type1 - не корректно. для type3 - корректно
	§  Режим TX определяет, был ли сигнал, послан автоматически или вручную
	§  Канал определяется в виде двоичного числа и определяет какой канал использует датчик, я включил его в ID
	§  Температура определяется как двоичное число, и представляет температуру
	§  Влажность определяется как двоичное число, и представляет собой влажность
  */

	// type 2
	/*Датчик посылает 15 раз по 40 импульсов:
	§  Заголовок от 0 до 7
	§  Батарея: 8
	§  TX режим: 9
	§  Канал: от 10 до 11
	§  Температура: от 12 до 24, в десятых долях фаренгейта. 
		для перевода в десятые доли цельсия: С = (F - 320) *5 / 9 - 500
	§  Влажность: от 25 до 31
	§  Контрольная сумма: от 32 до 39 - младший разряд суммы предыдущих байтов

	*/
	// type 3
	/*аналогично type1, но 7 раз по 37 импульсов:
	§  Батарея: 12 - 1 = low
	§  Неизвестно: от 36 до 36
	*/

	/* WH31a
	Датчик посылает данные 2 раза
	Высокий уровень - мин. 0.3мс
	низкий уровень - 1мс
	48 бит - 1/0 по 50...60нс
	1,2: 16 бит - последовательность 0x2DD4
	3,4: 16 бит - ID (вторые 8 бит)
	5,6: 7 - хз
	6...4 - канал
	3 - если 1 - низкий заряд батареи
	2..0, 7...0 - температура в 0.1с -40
	7:	7...0 - влажность
	8:	хз
	9:	сумма байтов 3...8
	*/

public:
	enum Const {
		packet_len_bytes = 10,
		ring_mask = 0b00001111,
		ring_len = (ring_mask + 1)
	};
	enum SensorType :uint8_t {
		type0 = 0,
		type1 = 1,
		type2 = 2,
		type3 = 3,
		WH31a = 4
	};
	typedef struct packet_data {
		SensorType type;
		uint8_t bytes[Const::packet_len_bytes];
		inline uint8_t& operator[](int i) { return bytes[i]; }
		inline bool operator==(const packet_data& B) { return !memcmp(this->bytes, B.bytes, Const::packet_len_bytes); }
		inline packet_data& operator=(const int x) { memset(this->bytes, x, Const::packet_len_bytes); return *this;}
	} packet_data;

private:
	static packet_data ring_buf[Const::ring_len];
	static uint8_t posPush;
	static uint8_t posPop;
private:
	static void check_data_receiver_onL_type0(uint8_t countH, uint8_t countL);
	static void check_data_receiver_onL_type1(uint8_t countH, uint8_t countL);
	static void check_data_receiver_onL_type2(uint8_t countH, uint8_t countL);
	static void check_data_receiver_onL_type3(uint8_t countH, uint8_t countL);
	static void check_data_receiver_WH31a();
	static inline bool CHECK_IN_RANGE(uint8_t x, uint8_t l, uint8_t h) { return ((l) <= (x) && (x) <= (h)); };

public:
	static void timer_interruptDiv10();
	static void timer_interrupt();
	static void begin();
	static packet_data * next_data();

};

extern THReceiverClass THReceiver;

#endif // treceiver_H
