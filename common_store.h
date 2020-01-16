#include "Arduino.h"
#include "lwip/ip_addr.h"

#pragma once
class ConfigStoreClass
{
public:
	typedef struct {
		bool _hasData;
		ip_addr_t MqttUdpServer;
		uint16_t MqttUdpPort;
		uint8_t crc8;
	} StoredData_t;

	static StoredData_t Data;
	static StoredData_t DefaultData;

	static void save();
	static bool load();


};


extern ConfigStoreClass ConfigStore;