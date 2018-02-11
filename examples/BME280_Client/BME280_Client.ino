/*
* Author - Marcel Schulz (alias Schullebernd)
* *************************************************************
* See further library details on https://github.com/Schullebernd/SBNetwork
* *************************************************************
* This example is a client with a BME280 temperatur, humidity and barometer sensor on I2C.
* It sends the measured values every 20 seconds to the master.
* To setup the master device for this example, open the UniversalSensorMaster example and run it on a second device.
* **************************************************************
* Step 1 - Prepare your device like in the example sketch GettingStartedClient
*
* Step 2 - Connect the BME280 sensor to your arduino/wemos device
* Connect		Arduino --> BME280
*           ------------------ 
*                A4 --> SDA
*                A5 --> SCL
*               3V3 --> VCC
*               GND --> GND
*             empty --> CSB
*             empty --> SD0
* The I2C Address of BME280 usually is 0x76 --> Put this into  bme.begin(0x76) at setup function. If this is not working try 0x77.
*
* Step 3 - Download and include the Sparkfun BME280 library.
* https://github.com/sparkfun/SparkFun_BME280_Arduino_Library
* 
* Step 4 - Run the project
* Connect the Arduino via USB to the PC, select the right board and COM interface in the tools menu and run the project.
*/

#include <SparkFunBME280.h>
#include <SBNetwork_config.h>
#include <SBNetwork.h>


// Type in here the mac address of the device.
// This must be unique within your complete network otherwise the network will not work.
//SBMacAddress deviceMac(0x01, 0x02, 0x03, 0x04, 0x05);
SBMacAddress deviceMac(0x05, 0x04, 0x03, 0x02, 0x01);

// Create a new network device with Arduino and set the _ce and _cs pin
// The first argument defines the type of the device. false=master and true=client device
//SBNetwork networkDevice(false, D2, D8);
SBNetwork networkDevice(true, 6, 7);

// time variables
uint32_t wait = 20000;
uint32_t lastWait = wait;

// Define the BME280 sensor
BME280 bme;

void setup() {

	Serial.begin(19200);
	Serial.println(F("*** PRESS 'N' to reset the device"));

	// Initialize the network device
	networkDevice.initialize(deviceMac);

	// Initialize the BME280 sensor
	bme.settings.commInterface = I2C_MODE;
	bme.settings.I2CAddress = 0x76;
	bme.settings.runMode = 3;
	bme.settings.tStandby = 0;
	bme.settings.filter = 0;
	bme.settings.tempOverSample = 1;
	bme.settings.pressOverSample = 1;
	bme.settings.humidOverSample = 1;
	Serial.print("Starting BME280... result of .begin(): 0x");

	//Calling .begin() causes the settings to be loaded
	delay(10);  //Make sure sensor had enough time to turn on. BME280 requires 2ms to start up.
	Serial.println(bme.begin(), HEX);
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

	// Send a message to the master every 4 seconds
	if (lastWait < millis()) {
		float fTemp = bme.readTempC();
		Serial.print("Temperature: ");
		Serial.print(fTemp, 2);
		Serial.println(" degrees C");
		float fPressure = bme.readFloatPressure();
		Serial.print("Pressure: ");
		Serial.print(fPressure, 2);
		Serial.println(" Pa");
		float fHumidity = bme.readFloatHumidity();
		Serial.print("%RH: ");
		Serial.print(fHumidity, 2);
		Serial.println(" %");
		byte message[10 + (3*4)]; // the first 10 bytes containing the type of the sensor. The 3*4 bytes are needed for the values. float has a length of 32 bits = 4 bytes and we need 3 of them.
		strcpy((char*)message, "BME280");
		memcpy((void*)(message + 10), &fTemp, sizeof(float));
		memcpy((void*)(message + 10 + 4), &fPressure, sizeof(float));
		memcpy((void*)(message + 10 + 4 + 4), &fHumidity, sizeof(float));
		networkDevice.sendToDevice(networkDevice.NetworkDevice.MasterMAC, message, 10 + (3*4));
		lastWait += wait;
	}

} // Loop
