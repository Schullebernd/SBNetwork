#ifndef _SB_NETWORK_DEVCIE_
#define _SB_NETWORK_DEVCIE_

#include "SBNetwork_config.h"
#include "SBTypes.h"
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

	static SBMasterStorage initialize();

	void save();
};

#define SB_NETWORK_FLASH_SIZE (sizeof(SBNetworkDevice) + sizeof(SBMasterStorage))

#endif