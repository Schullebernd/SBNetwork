#ifndef _SB_NETWORK_DEVCIE_
#define _SB_NETWORK_DEVCIE_

#include <EEPROM.h>

class SBNetworkDevice {
public:
	char			ID[2] = { 'D', 'S' }; // DS stands for device storage
	SBMacAddress	MAC;
	SBMacAddress	MasterMAC;
	byte			ConnectedToMaster;
	uint32_t		NetworkKey;
};

class SBMasterStorage{
public:
	SBMasterStorage(){};
	char			ID[2] = { 'M', 'S' }; // MS stands for master storage
	SBMacAddress	Slaves[MAX_CLIENTS];

	static SBMasterStorage initialize(){
		SBMasterStorage storage;
#if defined(ESP8266)
		EEPROM.begin(FLASH_SIZE);
#endif
		EEPROM.get(0 + sizeof(SBNetworkDevice), storage);
		if (storage.ID[0] != 'M' || storage.ID[1] != 'S'){
			// We have to create a new one
			storage.ID[0] = 'M';
			storage.ID[1] = 'S';
			Serial.println("Creating new Master Storage");
			EEPROM.put(0 + sizeof(SBNetworkDevice), storage);
#if defined(ESP8266)
			EEPROM.commit();
			EEPROM.end();
#endif
		}
		return storage;
	}

	void save(){
#if defined(ESP8266)
		EEPROM.begin(FLASH_SIZE);
#endif
		EEPROM.put(0 + sizeof(SBNetworkDevice), *this);
#if defined(ESP8266)
		EEPROM.commit();
		EEPROM.end();
#endif
	}
};

#endif