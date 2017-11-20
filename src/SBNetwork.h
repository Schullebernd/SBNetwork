
#ifndef _SB_NETWORK_
#define _SB_NETWORK_

#define SB_VERSION "1.0.0"

#include "SBTypes.h"
#include "SBNetwork_config.h"
#include "SBDevice.h"
#include <RF24.h>

#define BROADCAST_MAC (SBMacAddress(0x9E, 0x0E, 0x9E, 0x0E, 0x9E))
#define EMPTY_MAC (SBMacAddress(0x00, 0x00, 0x00, 0x00, 0x00))

class SBNetwork{  
  private:

  /*
   * Stores the uptime of the device
   */
  unsigned long long _Uptime;
  /*
   * Stores the last time from millis()
   */
  unsigned long _LastTime;
  /*
  * Stores the time when the device should ping to the master
  */
  unsigned long _NextCheck;

  /*
  * Stores the connection state to a master device in case of it is a client device
  */
  bool _Connected;

  /**
  * Here the payload will be stored for each frame receive
  */
  uint8_t _ReceiveBuffer[MAX_PACKAGE_SIZE];
  /**
  * Here the received message is stored after a full message receive (fragmented or not fragmented)
  */
  uint8_t _ReadBuffer[MAX_FRAME_SIZE];

#if defined(RUN_AS_MASTER)
  /*
  * Store the times, when the slaves sent the last signal to the master
  */
  unsigned long long _SlavePings[MAX_CLIENTS];
#endif

  /**
  Points to the last message which was received
  */
  void* _LastReceivedMessage;
  /**
  Stores the length of the last receives message
  */
  uint8_t _LastReceivedMessageSize;
  /**
  Stores the mac of the sender of the last received message
  */
  SBMacAddress _LastReceivedFromAddress;

  void initializeNetworkDevice(SBNetworkDevice &device, SBMacAddress mac);

  bool sendToDevice(SBNetworkFrame frame);

  bool handleCommandPackage(SBNetworkFrame *frame);

#if defined(RUN_AS_MASTER)
  bool sendMasterAck(SBMacAddress mac);

  bool sendPairingAck(SBMacAddress mac);
#endif

  bool receive(SBNetworkFrame *frame);

  bool receiveMessage(void **message, uint8_t *messageSize, SBMacAddress *mac);

public:
	/*
	* Define the standard addresses for the sensor network
	*/
	//SBMacAddress _StandardSensorAddress = SBMacAddress(0x01, 0x01, 0x01, 0x01, 0x01);
	SBMacAddress _BroadcastAddress = BROADCAST_MAC;

	SBNetworkDevice NetworkDevice;
#if defined(RUN_AS_MASTER)
	SBMasterStorage _MasterStorage;
#endif

	RF24 radio;

//######################################################################################
	
/*
 * Constructor with setting the used pins for commnicate with the NRF24L01(+) chip.
 */
 SBNetwork(uint8_t cePin, uint8_t csPin);
  
 /*
 * Constructor with setting the used pins for commnicate with the NRF24L01(+) chip.
 */
  SBNetwork();

/*
 * Initializes the sensor / master
 */
  void initialize(SBMacAddress mac);
  void initialize(byte mac[]) { initialize(SBMacAddress(mac[0], mac[1], mac[2], mac[3], mac[4])); }


 /*
  * Resets the Sensors data (including the eeprom).
  * The sensor then will loos the connaction to the master.
  */
  void resetData();

  /*
  Sends a SBNetworkFrame to a device.
  */
  bool sendToDevice(SBMacAddress mac, void* message, uint8_t messageSize);
  /*
  Check if a new incomming transmission is available
  */
  uint8_t available() { return _LastReceivedMessageSize; }
  /*
  Returns a pointer to the buffer, where the last incomming transmission is stored
  */
  void* getMessage() { return _LastReceivedMessage; }

#ifndef RUN_AS_MASTER
  bool connectToNetwork();

  bool checkMaster();
#else

  // Adds a mac to the storage and returns the position in the storage.
  uint8_t addMac(SBMacAddress mac);

  // Removes the mac from the storage. If the mac was stored, it returns the position of the mac, if not, it returns -1.
  uint8_t removeMac(SBMacAddress mac);
#endif

  bool pingDevice(SBMacAddress mac);

  // Updates the uptime counter.
  // If this device is a sensor, it pings the master after a time. The time is definded in the config under SENSOR_CHECK_INTERVAL.
  // If SENSOR_CHECK_INTERVAL is set to 0, it will not ping the master.
  void update();

  unsigned long long uptime(){
	  return _Uptime;
  }

};


#endif
