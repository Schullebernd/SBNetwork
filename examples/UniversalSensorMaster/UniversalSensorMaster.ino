/*
* Author - Marcel Schulz (alias Schullebernd)
* *************************************************************
* See further library details on https://github.com/Schullebernd/SBNetwork
* *************************************************************
* This Getting started is prepared for using it with a Wemos D1 mini as a master device.
* If you want to use an Arduino, then change the line 36 "SBNetwork networkDevice(false, D2, D8);" to the correct pinout for an Arduino "SBNetwork networkDevice(false, 6, 7)".
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
* Step 2 - Build the sketch for the master device
* Connect the Wemos via USB to the PC, select the right board and COM interface in the tools menu and run the project.
* After building and loading up to the Wemos, the serial monitor should show some log data.
*/

#include <SBNetwork_config.h>
#include <SBNetwork.h>

// Type in here the mac address of the device.
// This must be unique within your complete network otherwise the network will not work.
SBMacAddress deviceMac(0x01, 0x02, 0x03, 0x04, 0x05);
//SBMacAddress deviceMac(0x05, 0x04, 0x03, 0x02, 0x01);

// Create a new network device with Wemos D1 mini and set the _ce and _cs pin.
// The first argument defines the type of the device. false=master and true=client device
SBNetwork networkDevice(false, D2, D8);
//SBNetwork networkDevice(true, 6, 7);

void setup() {
	// Init serial connection
	Serial.begin(19200);

	// Initialize the network device
	networkDevice.initialize(deviceMac);

	Serial.println(F("*** PRESS 'N' to reset the device"));
}

void loop() {

	// This routine is for resetting the device
	// All flash data will be deleted
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
		byte* message = (byte*)networkDevice.getMessage();
		Serial.print(F("Received Content from sensor: "));
		Serial.println((char*)message);
		if (strcmp((char*)message, "BME280") == 0) {
			// We have received a BME280 transmission
			float fTemp, fPressure, fHumidity;
			memcpy(&fTemp, (void*)(message + 10), sizeof(float));
			memcpy(&fPressure, (void*)(message + 10 + 4), sizeof(float));
			memcpy(&fHumidity, (void*)(message + 10 + 4 + 4), sizeof(float));
			Serial.print("Temperature: ");
			Serial.print(fTemp, 2);
			Serial.println(" degrees C");
			Serial.print("Pressure: ");
			Serial.print(fPressure, 2);
			Serial.println(" Pa");
			Serial.print("%RH: ");
			Serial.print(fHumidity, 2);
			Serial.println(" %");
		}
	}
} // Loop
