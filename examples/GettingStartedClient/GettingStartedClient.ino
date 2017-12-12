/*
* Author - Marcel Schulz (alias Schullebernd)
* *************************************************************
* See further library details on https://github.com/Schullebernd/SBNetwork
* *************************************************************
* This Getting started is prepared for using it with a Arduino as a client device.
* If you want to use a Wemos, then change the line 38 "SBNetwork networkDevice(true, 6, 7);" to the correct pinout for a Wemos "SBNetwork networkDevice(true, D2, D8);".
* **************************************************************
* Step 1 - Prepare your device
* Connect a nRF24L01 transmitter to a Wemos D1 mini or an Arduino Device
* WEMOS > RF24      ARDUINO > RF24      --------------------------------------
* ------------      --------------     |   GND #  # VCC          TOP VIEW     |
*   3V3 > VCC          VCC  > VCC      |    CE #  # CSN          of nRF24L01  |
*   GND > GND          GND  > GND      |   SCK #  # MOSI                      |
*    D2 > CE             6  > CE       |  MISO #  # IRQ                       |
*    D8 > CSN            7  > CSN      |                                      |
*    D7 > MOSI          11  > MOSI      --------------------------------------
*    D6 > MISO          12  > MISO
*    D5 > SCK           13  > SCK
*
* Step 2 - Build the sketch for the client device
* Connect the Arduino via USB to the PC, select the right board and COM interface in the tools menu and run the project.
* After building and loading up to the Arduino, the serial monitor should show some log data.
* If you already run a master device you should now see the pairing process in the serial montor for both devices.
*/

#include <SBNetwork_config.h>
#include <SBNetwork.h>

// Type in here the mac address of the device.
// This must be unique within your complete network otherwise the network will not work.
//SBMacAddress deviceMac(0x01, 0x02, 0x03, 0x04, 0x05);
SBMacAddress deviceMac(0x05, 0x04, 0x03, 0x02, 0x01);

// Create a new network device with Wemos D1 mini and set the _ce and _cs pin
// The first argument defines the type of the device. false=master and true=client device
//SBNetwork networkDevice(false, D2, D8);
SBNetwork networkDevice(true, 6, 7);

// time variables
uint32_t wait = 4000;
uint32_t lastWait = wait;

void setup() {

	Serial.begin(19200);
	Serial.println(F("*** PRESS 'N' to reset the device"));

	// Initialize the network device
	networkDevice.initialize(deviceMac);

	lastWait = millis();
}

void loop() {

	if (Serial.available())
	{
		char c = toupper(Serial.read());
		if (c == 'N') {
			networkDevice.resetData();
		}
	}

	// Call this in the loop() function to maintain the network device
	networkDevice.update();

	// Check, if there are messages available
	uint8_t messageSize = networkDevice.available();
	if (messageSize > 0) {
		// Read the message from network device and print out the text.
		Serial.print(F("Received Content: "));
		Serial.println((char*)networkDevice.getMessage());
	}
	// Send a message to the master every 4 seconds
	if (lastWait < millis()) {
		char* message = "Hello Master, here is your client, can you hear me?";
		networkDevice.sendToDevice(networkDevice.NetworkDevice.MasterMAC, message, strlen(message) + 1);
		lastWait += wait;
	}

} // Loop
