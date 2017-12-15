
#ifndef _SB_TYPES_
#define _SB_TYPES_

#include <arduino.h>
// Will be sent to check, if a device is available
#define	SB_COMMAND_PING				0
// Will be sent, if normal data is transported
#define	SB_COMMAND_NO_COMMAND		1
// Will be sent from a slave to find search a master
#define	SB_COMMAND_SEARCH_MASTER	2
// Will be sent from a master after receiving a search master request
#define	SB_COMMAND_MASTER_ACK		3
// Will be sent from a new client, in case he wants to join the network
#define	SB_COMMAND_REQUEST_PAIRING	4
// Will be sent from the master after successfule adding a new client
#define	SB_COMMAND_PAIRING_ACK		5
// Will always be sent, when packages are received and the other device is a known device
#define SB_COMMAND_ACK				6

class SBMacAddress{
  public:
	  uint8_t Bytes[5];
	  SBMacAddress(){};
	  SBMacAddress(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5);
	  bool isEquals(SBMacAddress otherAddres);

	  operator uint8_t*(){
		  return (uint8_t*)Bytes;
	  }
};

typedef struct {
	SBMacAddress	FromAddress;
	SBMacAddress	ToAddress;
	uint8_t			CommandType;
	uint8_t			PackageId; // The unique ID of the package. Will be 
	uint8_t			FragmentCount; // How many fragments will be sent
	uint8_t			FragmentNr; // Which fragment is this package
} SBNetworkHeader;

#define MAX_PACKAGE_SIZE (32 - sizeof(SBNetworkHeader))

typedef struct{
	SBNetworkHeader	Header;
	uint8_t			MessageSize;
	uint8_t*		Message;
} SBNetworkFrame;

#define MAX_FRAME_SIZE 200

#endif

