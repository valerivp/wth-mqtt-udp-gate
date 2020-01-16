#include "common_def.h"
#include "common_store.h"
#include "EEPROM.h"

ConfigStoreClass ConfigStore;

ConfigStoreClass::StoredData_t ConfigStoreClass::DefaultData = { true, {PP_HTONL(LWIP_MAKEU32(192,168,1,255))}, 1883};
ConfigStoreClass::StoredData_t ConfigStoreClass::Data;

void ConfigStoreClass::save()
{
	Data.crc8 = crc8((const uint8_t *)&Data, sizeof(Data) - sizeof(Data.crc8));

	EEPROM.begin(sizeof(Data));
	EEPROM.put(0, Data);
	EEPROM.commit();
	EEPROM.end();
}

bool ConfigStoreClass::load()
{
	StoredData_t tempData;
	EEPROM.begin(sizeof(tempData));
	EEPROM.get(0, tempData);
	EEPROM.end();

	uint8_t crc = crc8((const uint8_t *)&tempData, sizeof(tempData) - sizeof(Data.crc8));
	if (!tempData._hasData || crc != tempData.crc8) {
		Data = DefaultData;
		return false;
	}else {
		Data = tempData;
		return true;
	}
}


