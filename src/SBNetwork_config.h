
#ifndef _SB_NETWORK_CONFIG_
#define _SB_NETWORK_CONFIG_

// Uncomment the following line, to compile the library for a master device.
//#define RUN_AS_MASTER

//#define _DEBUG

#define MASTER_CHECK_INTERVAL 0 // All sensors will ping the master every xxx milliseconds. if set to 0, they will not ping the master

#define MAX_CLIENTS 10

#define CLIENT_TIMEOUT 20000

#define FLASH_SIZE 512

#define ACK_WAIT 100

#endif
