
#include <Arduino.h>

#include "common_def.h"

#include "timers.h"

#include "esp_def.h"

//#include <EEPROM.h>

#include "server.h"

#include "common_store.h"

#include "mqtt_udp.h"

#include <WiFiUdp.h>

#include "treceiver.h"


void onWiFiEvent(WiFiEvent_t event)
{
	if (WiFiEvent_t::WIFI_EVENT_SOFTAPMODE_PROBEREQRECVED != event) {
		DEBUG_PRINT(F("WiFiEvent:") << event);
	}
	switch (event)
	{
		case WiFiEvent_t::WIFI_EVENT_STAMODE_CONNECTED: {
			DEBUG_PRINT(F("Connected to: ") << WiFi.SSID());
			WiFi.enableAP(false);
			WiFi.setAutoReconnect(true);
			setLedState(1);
		}break;

	default:
		break;
	}

}

void initWiFi() {
	String ssidAP = "WTH433-gate-" + ESP_getChipIdStr();
	WiFi.hostname(ssidAP);
	WiFi.softAP(ssidAP.c_str(), "");
	WiFi.mode(WIFI_AP_STA);
	//WiFi.mode(WIFI_AP);
	WiFi.onEvent(onWiFiEvent);
}

int ledState = 0;
uint16_t ledMask;

void setLedState(int newState) {
	static uint16_t ledMasks[] = { 0b0000000000000001, 0b1111111111111110, 0b0000000011111111,
		0b1010101010101010, 0b1110111011100000 };
	ledState = newState;
	ledMask = ledMasks[ledState];
}

void checkButtonAndBlinkLed() {
	static uint16_t count = 0;
	if (!digitalRead(pins::button)) {
		count++;
		digitalWrite(pins::led, count % 10);

	}else{
		if (count) {
			DEBUG_PRINT("Button pressed: " << count);
			if (count < 2) {
				// это дребезг контактов
			}else if (count > 100) {
				DEBUG_PRINT("Restart...");
				// restart
				ESP.restart();

			}else if (count > 50) {
				DEBUG_PRINT("Change WiFi mode...");
				WiFi.mode(WIFI_AP_STA);

			}else if (count > 20) {

				
			}else if (count < 20) {
				// короткое нажатие
			}

			count = 0;
		}
		if (WiFi.getMode() != WIFI_STA) {
			setLedState(2);
		};
		digitalWrite(pins::led, (ledMask & 1));
		ledMask = (ledMask >> 1) | (ledMask << 15);
	}


}

void resetLedState()
{
	if (ledState >= 3)
		setLedState(1);
}

const char * toFixed(int32_t v, int f) {
	static char buf[16];
	static char * format = "%ld.%0?d";
	format[6] = '0' + f;
	int d = 1;
	for (int i = 0; i < f; i++)
		d *= 10;
	sprintf(buf, format, int32_t(v / d), int16_t(v%d));
	return buf;
}






int mqtt_udp_send_pkt(int fd, char *data, size_t len)
{
	WiFiUDP udp;
	udp.beginPacket(IPAddress(ConfigStore.Data.MqttUdpServer.addr), ConfigStore.Data.MqttUdpPort);
	udp.write(data, len);
	udp.endPacket();
	udp.stop();
	return 0;
}


void setup(void) {
	ESP_wdtEnable(10000);

	Serial.begin(115200); Serial.println(); // при старте в порту может быть мусор, начнем с чистого листа

	DEBUG_PRINT("Started");
	DEBUG_PRINT("Version: " << __TIMESTAMP__);

	pinMode(pins::led, OUTPUT);

	pinMode(pins::button, INPUT_PULLUP);

	ConfigStore.load();

	initWiFi();

	Timers.add(checkButtonAndBlinkLed, 100, F("checkButtonAndBlinkLed"));
	//digitalWrite(pins::led, LOW);


	Timers.add(sendSensorsData, 100, F("sendSensorsData"));
	setLedState(4);

	HTTPserver.init();


	TH433Receiver.begin();
}

typedef struct i2c_packet {
	union {
		byte abid[2]; // в самом младшем полубайте поместим тип датчика и номер канала
		uint16_t id;
	};
	//uint8_t type;
	union {
		byte abtemperature[2]; // в самом старшем полубайте (это второй байт) поместим номер датчика
		int16_t temperature;	 
	};
	uint8_t humidity; 
	//uint8_t timeLabel;
	//uint8_t crc;
	/*void calc_crc(uint8_t pos) {
		abtemperature[1] = abtemperature[1] & 0b00001111 | (pos << 4);
		crc = crc8((uint8_t*)this, sizeof(*this) - 1);
	};*/
};

enum Settings :uint16_t {
	maxSensors = 0x20,
	maxTTL = 0x03ff
};
struct KnownSensor {
	uint16_t id;
	uint16_t lt;
};
static KnownSensor KnownSensors[Settings::maxSensors] = {};

void checkKnownSensors() {
	for (int i = 0; i < sizeofArray(KnownSensors); i++) {
		KnownSensor& ks = KnownSensors[i];
		if (ks.id)
			ks.lt++;
		if (ks.lt > Settings::maxTTL)
			ks.id = 0;
	}
}

void PrintData(const TH433ReceiverClass::packet_data& buffer) {
	for (uint8_t i = 0; i < TH433ReceiverClass::packet_len_bytes; i++) {
		Serial.print(toBin(buffer.bytes[i]));
		Serial.print(' ');
	}
	Serial.print(buffer.type);
	Serial.print(' ');
	Serial.println(millis());
}



void sendSensorsData() {
	static i2c_packet tmpData;
	TH433ReceiverClass::packet_data* lpbuffer = TH433Receiver.next_data();
	if (lpbuffer) {
		TH433ReceiverClass::packet_data& buffer = *lpbuffer;

		//PrintData(buffer);
		switch (lpbuffer->type) {
		case TH433ReceiverClass::SensorType::type0: {
			tmpData.abid[1] = buffer[0]; // байты хранятся в обратном порядке
			tmpData.abid[0] = buffer[1] & 0b00110000 | (lpbuffer->type << 2);
			tmpData.temperature = buffer[1] & 0b1111;
			tmpData.temperature = (tmpData.temperature << 8) | (buffer[2]); // 00001100|1011
			tmpData.temperature += 500;

			tmpData.humidity = ((buffer[3] << 4) | (buffer[4] >> 4)) & 0x7f;
			tmpData.humidity |= (buffer[1]) & 0b10000000; // battery
		}break;
		case TH433ReceiverClass::SensorType::type1:
		case TH433ReceiverClass::SensorType::type3: {

			if ((lpbuffer->type == TH433ReceiverClass::SensorType::type1
				&& (buffer[0] & 0b11110000) ^ 0b01010000)
				|| (lpbuffer->type == TH433ReceiverClass::SensorType::type3
					&& (buffer[0] & 0b11110000) ^ 0b10010000)) {
				memset((void*)&tmpData, 0, sizeof(tmpData));
			}
			else {
				tmpData.abid[1] = buffer[0]; // байты хранятся в обратном порядке
				tmpData.abid[0] = buffer[1] & 0b11110011 | (lpbuffer->type << 2);
				tmpData.temperature = buffer[2];
				tmpData.temperature = (tmpData.temperature << 4) | (buffer[3] >> 4); // 00001100|1011
				tmpData.temperature += 500;

				tmpData.humidity = ((buffer[3] << 4) | (buffer[4] >> 4)) & 0x7f;
				if (lpbuffer->type == TH433ReceiverClass::SensorType::type1) {
					tmpData.humidity |= (buffer[1] << 4) & 0b10000000; // battery
				}
				else {
					tmpData.humidity |= (((buffer[1] << 4) ^ 0b10000000) & 0b10000000);// ^ 0b10000000; // battery
																					   //Serial.println((((buffer[1] << 4) ^ 0b10000000) & 0b10000000));
				}
			}

		}break;
		case TH433ReceiverClass::SensorType::type2: {
			uint8_t sum = buffer[0]; sum += buffer[1]; sum += buffer[2]; sum += buffer[3];
			if (sum == buffer[4]) {
				tmpData.abid[1] = buffer[0]; // байты хранятся в обратном порядке
				tmpData.abid[0] = buffer[1] & 0b00110000 | (lpbuffer->type << 2);
				tmpData.temperature = buffer[1] & 0b1111;
				tmpData.temperature = (tmpData.temperature << 8) | (buffer[2]); // 00001100|1011
				tmpData.temperature = (tmpData.temperature - 320) * 5 / 9;
				tmpData.humidity = buffer[3] & 0x7f;
				tmpData.humidity |= (~buffer[1]) & 0b10000000; // battery
			}
			else {
				memset((void*)&tmpData, 0, sizeof(tmpData));
			}
		}break;
		}

		int last = -1, empty = -1, current = -1;
		uint16_t lt = 0;
		for (int i = 0; i < sizeofArray(KnownSensors); i++) {
			KnownSensor& ks = KnownSensors[i];
			if (!ks.id && empty < 0)
				empty = i;
			if (ks.id && ks.lt > lt)
				lt = ks.lt, last = i;
			if (ks.id == tmpData.id) {
				current = i;
				break;
			}
		}

		if (current >= 0) {
			KnownSensors[current].lt = 0;
		}else if (empty >= 0) {
			KnownSensors[empty].id = tmpData.id;
			KnownSensors[empty].lt = 0;
		}else if (last >= 0) {
			KnownSensors[last].id = tmpData.id;
			KnownSensors[last].lt = 0;
		}else {
			KnownSensors[0].id = tmpData.id;
			KnownSensors[0].lt = 0;
		}


		Serial.println();

		for (uint8_t i = 0; i < TH433ReceiverClass::Const::packet_len_bytes; i++) {
			Serial.print(toBin(buffer[i])); Serial.print(' ');
		}
		Serial.println();

		for (uint8_t i = 0; i < sizeof(tmpData); i++) {
			Serial.print(toBin(((uint8_t*)&tmpData)[i])); Serial.print(' ');
		}
		Serial.println();

		Serial.print(tmpData.id, HEX); Serial.print(' ');
		Serial.print(tmpData.temperature); Serial.print(" -> ");
		Serial.print(int((tmpData.temperature & 0x0fff) - 500)); Serial.print(' ');
		Serial.print(tmpData.humidity & 0b1111111); Serial.print(' ');
		Serial.print(tmpData.humidity >> 7); Serial.print(' ');
		Serial.println();
		
		

		if (tmpData.id && current >= 0) {
			char topic[32];
			sprintf(topic, "WTH433-%d/0x%04x", lpbuffer->type, tmpData.id);

			char load[512];
			sprintf(load, "{"
				"\"temperature\":%d, "
				"\"humidity\":%d, "
				"\"battery\":%d}",
				int((tmpData.temperature & 0x0fff) - 500),
				int(tmpData.humidity & 0b1111111),
				int(tmpData.humidity >> 7));

				Serial.print(topic); Serial.print(' '); Serial.print(load);
				if (ConfigStore.Data.MqttUdpServer.addr) {
					mqtt_udp_send(0, topic, load);
					//Serial.print(":send");
				}
				Serial.println();
				//for (;;);
		
		}else{
			Serial.println("ID not found in cashe, ignore");
		
		}
	}

}



void loop(void) {
	static unsigned long lastTime = 0;
	unsigned long currTime = millis();

	if ((currTime - lastTime) > 1000)
		DEBUG_PRINT(F("No loop more than one second"));
	lastTime = currTime;

	ESP_wdtFeed();

	Timers.doLoop();


	if ((millis() - lastTime) > 1000)
		DEBUG_PRINT(F("It was more than one second: Timers.doLoop"));


}

