// (c) valerivp@gmail.com

#include <SPI.h>
#include "cc1101.h"
#include "Streaming.h"


#include "treceiver.h"
#include "pins_names.h"

THReceiverClass THReceiver;
THReceiverClass::packet_data THReceiverClass::ring_buf[];
uint8_t THReceiverClass::posPush = 1;
uint8_t THReceiverClass::posPop = 0;

#define RECEIVER_433_DATA_PIN pins::d1
#define RECEIVER_868_DATA_PIN pins::d0

#define LED_ON() digitalWrite(pins::led, LOW)
#define LED_OFF() digitalWrite(pins::led, HIGH)

void THReceiverClass::begin()
{
	//Настраиваем пин на вход - данные приемника

	pinMode(RECEIVER_433_DATA_PIN, INPUT);

	SPI_INIT();

	CC1101_init();
	halRfWriteReg(IOCFG0, 0x0D);    //GDO0 Output Pin Configuration
	halRfWriteReg(FIFOTHR, 0x47);   //RX FIFO and TX FIFO Thresholds
	halRfWriteReg(PKTCTRL0, 0x32);  //Packet Automation Control
	halRfWriteReg(FSCTRL1, 0x08);   //Frequency Synthesizer Control
	halRfWriteReg(FREQ2, 0x21);     //Frequency Control Word, High Byte
	halRfWriteReg(FREQ1, 0x66);     //Frequency Control Word, Middle Byte
	halRfWriteReg(FREQ0, 0xE4);     //Frequency Control Word, Low Byte
	halRfWriteReg(MDMCFG4, 0xC9);   //Modem Configuration
	halRfWriteReg(MDMCFG3, 0x5C);   //Modem Configuration
	halRfWriteReg(MDMCFG2, 0x80);   //Modem Configuration
	halRfWriteReg(DEVIATN, 0x44);   //Modem Deviation Setting
	halRfWriteReg(MCSM0, 0x18);     //Main Radio Control State Machine Configuration
	halRfWriteReg(FOCCFG, 0x16);    //Frequency Offset Compensation Configuration
	halRfWriteReg(AGCCTRL2, 0x43);  //AGC Control
	halRfWriteReg(WORCTRL, 0xFB);   //Wake On Radio Control
	halRfWriteReg(FSCAL3, 0xE9);    //Frequency Synthesizer Calibration
	halRfWriteReg(FSCAL2, 0x2A);    //Frequency Synthesizer Calibration
	halRfWriteReg(FSCAL1, 0x00);    //Frequency Synthesizer Calibration
	halRfWriteReg(FSCAL0, 0x1F);    //Frequency Synthesizer Calibration

	cmdStrobe(SRX); //set radio chip to listen mode

	pinMode(RECEIVER_868_DATA_PIN, INPUT);


	timer1_isr_init();
	timer1_attachInterrupt(timer_interrupt);
	timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);  //5MHz (5 ticks/us - 1677721.4 us max)
	timer1_write((clockCyclesPerMicrosecond() / 16) * 10); // 100kHz

	pinMode(pins::d2, OUTPUT);


}

void ICACHE_RAM_ATTR THReceiverClass::check_data_receiver_onL_type0(uint8_t countH, uint8_t countL) {
	const byte	packet_len = 36;
	static byte buffer[2 * Const::packet_len_bytes];
	static packet_data buffers[2] = { SensorType::type0 };
	static byte nPacketNum = 0;
	static bool nEqPackets = false;
	static byte bits_count = 0;

	static unsigned long last_packet_time = 0;
	int8_t bit = -1;

	if (CHECK_IN_RANGE(countH, 4, 6)) {
		if (CHECK_IN_RANGE(countL, 35, 42)) { // 9.1ms
			if (bits_count == packet_len) { // полная посылка
				last_packet_time = micros(); // отметка времни получения пакета
				nPacketNum++; // увеличим счетчик пакетов
				if (buffers[0] == buffers[1]) {
					// приняли два одинаковых пакета
					LED_ON();
					if (!nEqPackets) {
						ring_buf[posPush] = buffers[0];
						++posPush &= Const::ring_mask;
						if (posPush == posPop)
							++posPop &= Const::ring_mask;
					}
					nEqPackets = true;
				}
				else {
					nEqPackets = false;
					LED_OFF();
				}
			}
		}
		else if (CHECK_IN_RANGE(countL, 7, 11)) {  // 1.9ms
			bit = 0;
		}else if (CHECK_IN_RANGE(countL, 17, 22)) { // 3.9ms
			bit = 1;
		}
		if (bit >= 0 && bits_count < packet_len) { // принимаем только биты в количестве длины пакета
			packet_data& buffer = buffers[nPacketNum % 2];

			if (!bits_count) {
				buffer = 0;
			}

			if (bit) // буфер чист - он же очищается перед приемом первого бита
				buffer[bits_count / 8] |= 0x80 >> (bits_count % 8);

			bits_count++;
		}
		else {
			bits_count = 0;
			if (nPacketNum && (micros() - last_packet_time) > 1000000ul) {
				// прошло больше секунды, чистим счетчик пакетов
				nPacketNum = 0;
				nEqPackets = false;
			}
		}
	}
}

void ICACHE_RAM_ATTR THReceiverClass::check_data_receiver_onL_type1(uint8_t countH, uint8_t countL) {
	const byte	packet_len = 36;
	static byte buffer[2 * Const::packet_len_bytes];
	static packet_data buffers[2] = { SensorType::type1 };
	static byte nPacketNum = 0;
	static bool nEqPackets = false;
	static byte bits_count = 0;

	static unsigned long last_packet_time = 0;
	int8_t bit = -1;

	if (CHECK_IN_RANGE(countH, 4, 6)) {
		if (CHECK_IN_RANGE(countL, 80, 96)) { // 9.1ms
			if (bits_count == packet_len) { // полная посылка
				last_packet_time = micros(); // отметка времни получения пакета
				nPacketNum++; // увеличим счетчик пакетов
				if (buffers[0] == buffers[1]) {
					// приняли два одинаковых пакета
					LED_ON();
					if (!nEqPackets) {
						ring_buf[posPush] = buffers[0];
						++posPush &= Const::ring_mask;
						if (posPush == posPop)
							++posPop &= Const::ring_mask;
					}
					nEqPackets = true;
				}else{
					nEqPackets = false;
					LED_OFF();
				}
			}
		}else if (CHECK_IN_RANGE(countL, 16, 21)) {  // 1.9ms
			bit = 0;
		}else if (CHECK_IN_RANGE(countL, 36, 42)) { // 3.9ms
			bit = 1;
		}
		if (bit >= 0 && bits_count < packet_len) { // принимаем только биты в количестве длины пакета
			packet_data& buffer = buffers[nPacketNum % 2];

			if (!bits_count) {
				buffer = 0;
			}

			if (bit) // буфер чист - он же очищается перед приемом первого бита
				buffer[bits_count / 8] |= 0x80 >> (bits_count % 8);

			bits_count++;
		}else {
			bits_count = 0;
			if (nPacketNum && (micros() - last_packet_time) > 1000000ul) {
				// прошло больше секунды, чистим счетчик пакетов
				nPacketNum = 0;
				nEqPackets = false;
			}
		}
	}
}

void ICACHE_RAM_ATTR THReceiverClass::check_data_receiver_onL_type2(uint8_t countH, uint8_t countL) {
	const byte	packet_len = 40;
	static byte buffer[2 * Const::packet_len_bytes];
	static packet_data buffers[2] = {SensorType::type2};
	static byte nPacketNum = 0;
	static bool nEqPackets = false;
	static byte bits_count = 0;

	static unsigned long last_packet_time = 0;
	int8_t bit = -1;
	if (CHECK_IN_RANGE(countH, 7, 9) && CHECK_IN_RANGE(countL, 6, 7)) { // начало пакета
		bits_count = 0;
	}else if (CHECK_IN_RANGE(countH, 7, 8) && CHECK_IN_RANGE(countL, 8, 10)) { // конец пакета
		if (bits_count == packet_len) { // полная посылка
			last_packet_time = micros(); // отметка времни получения пакета
			nPacketNum++; // увеличим счетчик пакетов
			if (buffers[0] == buffers[1]) {
				// приняли два одинаковых пакета
				LED_ON();
				if (!nEqPackets) {
					ring_buf[posPush] = buffers[0];
					++posPush &= Const::ring_mask;
					if (posPush == posPop)
						++posPop &= Const::ring_mask;
				}
				nEqPackets = true;
			}else{
				nEqPackets = false;
				LED_OFF();
			}
		}
	}else if (CHECK_IN_RANGE(countH, 4, 5)) {
		bit = 1;
	}else if (CHECK_IN_RANGE(countH, 2, 3)) {
		bit = 0;
	}
	if (bit >= 0 && bits_count < packet_len) { // принимаем только биты в количестве длины пакета
		packet_data& buffer = buffers[nPacketNum % 2];

		if (!bits_count)
			buffer = 0;

		if (bit) // буфер чист - он же очищается перед приемом первого бита
			buffer[bits_count / 8] |= 0x80 >> (bits_count % 8);

		bits_count++;
	}
	else {
		if (nPacketNum && (micros() - last_packet_time) > 1000000ul) {
			// прошло больше секунды, чистим счетчик пакетов
			nPacketNum = 0;
			nEqPackets = false;

		}
	}

}

void ICACHE_RAM_ATTR THReceiverClass::check_data_receiver_onL_type3(uint8_t countH, uint8_t countL) {
	const byte	packet_len = 37;
	//static byte buffer[2 * Const::packet_len_bytes];
	static packet_data buffers[2] = { SensorType::type3 };
	static byte nPacketNum = 0;
	static bool nEqPackets = false;
	static byte bits_count = 0;

	static unsigned long last_packet_time = 0;
	int8_t bit = -1;

	if (CHECK_IN_RANGE(countH, 5, 7)) {
		if (CHECK_IN_RANGE(countL, 85, 96)) { // 8.9ms
			if (bits_count == packet_len) { // полная посылка
				last_packet_time = micros(); // отметка времни получения пакета
				nPacketNum++; // увеличим счетчик пакетов
				if (buffers[0] == buffers[1]) {
					// приняли два одинаковых пакета
					LED_ON();
					if (!nEqPackets) {
						ring_buf[posPush] = buffers[0];
						++posPush &= Const::ring_mask;
						if (posPush == posPop)
							++posPop &= Const::ring_mask;
					}
					nEqPackets = true;
				}
				else {
					nEqPackets = false;
					LED_OFF();
				}
			}
		}
		else if (CHECK_IN_RANGE(countL, 16, 21)) {  // 1.9ms
			bit = 0;
		}
		else if (CHECK_IN_RANGE(countL, 36, 42)) { // 3.9ms
			bit = 1;
		}
		if (bit >= 0 && bits_count < packet_len) { // принимаем только биты в количестве длины пакета
			packet_data& buffer = buffers[nPacketNum % 2];

			if (!bits_count) {
				buffer = 0;
			}

			if (bit) // буфер чист - он же очищается перед приемом первого бита
				buffer[bits_count / 8] |= 0x80 >> (bits_count % 8);

			bits_count++;
		}
		else {
			bits_count = 0;
			if (nPacketNum && (micros() - last_packet_time) > 1000000ul) {
				// прошло больше секунды, чистим счетчик пакетов
				nPacketNum = 0;
				nEqPackets = false;
			}
		}
	}
}


void ICACHE_RAM_ATTR THReceiverClass::timer_interruptDiv10()
{
	static uint8_t countH = 0;
	static uint8_t countL = 0;
	if (digitalRead(RECEIVER_433_DATA_PIN)) {
		countH++;
		//LED_ON();
	}else{
		//LED_OFF();
		if (countH >= 2) { // шумоподавление. 
			// если countH > 0 - значит задний фронт
			//LED_ON();
			check_data_receiver_onL_type0(countH, countL);
			check_data_receiver_onL_type1(countH, countL);
			check_data_receiver_onL_type2(countH, countL);
			check_data_receiver_onL_type3(countH, countL);
			countL = 0;
			//LED_OFF();
		}
		countH = 0; // сейчас L, поэтому H обнуляем
		countL++; 
	}

}


void ICACHE_RAM_ATTR THReceiverClass::timer_interrupt()
{

	check_data_receiver_WH31a();

	static byte count = 0;
	if (!(count++ % 10))
		timer_interruptDiv10();

}

void ICACHE_RAM_ATTR THReceiverClass::check_data_receiver_WH31a() {
	static packet_data buffers[2] = { {SensorType::WH31a}, {SensorType::WH31a} };
	static byte nPacketNum = 0;
	static bool nEqPackets = false;

	static uint8_t countH = 0, countH1 = 0, countL = 0;
	static uint32_t timeLabel10 = 0;
	timeLabel10 += 10;

	bool pinState = digitalRead(pins::d0);


	static byte bufnum = 0;
	static uint32_t speed = 0;
	static int8_t leftBits = 0, currentBit = 0, minHCount = 0;
	static int8_t syncCount = -1;
	if (timeLabel10 > 50000) {
		// нет пакета
		syncCount = -1;
		timeLabel10 = 0;
		leftBits = 0;
		bufnum = 0;
	}

	if (leftBits) {
		if ((timeLabel10 / speed) != currentBit) {
			if (countH1 >= minHCount) {
				buffers[nPacketNum].bytes[currentBit >> 3] |= 0b10000000 >> (currentBit & 0b111);
				//digitalWrite(pins::d2, HIGH);
			} 
			countH1 = 0;
			leftBits--;
			currentBit++;
			if (!leftBits) {
				/*for (uint8_t i = 0; i < 10; i++) {
					Serial.print(buffers[nPacketNum].bytes[i], BIN);
					Serial.print(' ');
				}
				Serial.println();*/

				nPacketNum++, nPacketNum &= 1;
				if (!nPacketNum && buffers[0] == buffers[1]) {
					// приняли два одинаковых пакета
					ring_buf[posPush] = buffers[0];
					++posPush &= Const::ring_mask;
					if (posPush == posPop)
						++posPop &= Const::ring_mask;
					//LED_OFF();
				}
			}
			//digitalWrite(pins::d2, LOW);
		}
	}

	if (pinState) {
		countH1++;
		if (countL) {
			if (countH >= 30 && countL >= 90) {
				// начало пакета
				syncCount = 0;
				timeLabel10 = 0;
				//LED_ON();
			}else if (syncCount >= 0) { // считаем биты синхронизации
				syncCount++;
				if (syncCount == 20) { // это 40 битов, начало 41го, есть синхронизация
					syncCount = -1;
					speed = timeLabel10 / 40; // делим на 40
					minHCount = speed / 20 + 1; // делим на 2, потом на 10 и + 1
					timeLabel10 = 0;
					leftBits = 80;
					currentBit = 0;
					countH1 = 1;
					buffers[nPacketNum] = 0;

				}

			}

			countH = 0;
			countL = 0;
		}
		countH++;
	}
	else {

		countL++;
	}

}


THReceiverClass::packet_data * THReceiverClass::next_data()
{
	static packet_data buffer;
	packet_data* res = nullptr;

	cli();

	uint8_t newPopPos = (posPop + 1 & Const::ring_mask);
	if (newPopPos != posPush) {
		posPop = newPopPos;
		buffer = ring_buf[posPop];
		res = &buffer;
	}

	sei();

	return res;
}
