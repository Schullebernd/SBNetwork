
#ifndef _SB_NETWORK_CONFIG_
#define _SB_NETWORK_CONFIG_

// Generates details debug messages about sending and receiving data packages
//#define _DEBUG

// All slaves will ping the master every xxx milliseconds. if set to 0, they will not ping the master
#define MASTER_CHECK_INTERVAL 0

#define MAX_CLIENTS 10

#define CLIENT_TIMEOUT 60000

// Waittime for an ACK package
#define ACK_WAIT 50

// Count of trys to send a data package
#define RETRY_COUNT 10

// Milliseconds to wait between two retries
#define RETRY_DALY 40

#endif
