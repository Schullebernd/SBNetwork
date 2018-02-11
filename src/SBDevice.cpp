#include "SBDevice.h"

SBMasterStorage SBMasterStorage::initialize() {
	SBMasterStorage storage;
#if defined(ESP8266)
	EEPROM.begin(SB_NETWORK_FLASH_SIZE);
#endif
	EEPROM.get(0 + sizeof(SBNetworkDevice), storage);
	if (storage.ID[0] != 'M' || storage.ID[1] != 'S') {
		// We have to create a new one
		storage.ID[0] = 'M';
		storage.ID[1] = 'S';
		Serial.println("Creating new Master Storage");
		EEPROM.put(0 + sizeof(SBNetworkDevice), storage);
#if defined(ESP8266)
		EEPROM.commit();
#endif
	}	
#if defined(ESP8266)
	EEPROM.end();
#endif
	return storage;
}

void SBMasterStorage::save() {
#if defined(ESP8266)
	EEPROM.begin(SB_NETWORK_FLASH_SIZE);
#endif
	EEPROM.put(sizeof(SBNetworkDevice), *this);
#if defined(ESP8266)
	EEPROM.commit();
	EEPROM.end();
#endif
}