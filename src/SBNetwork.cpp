#include "SBNetwork.h"
#include <SPI.h>
#include <RF24.h>
#include <EEPROM.h>

void printAddress(byte address[5]){
  Serial.print("0x");
  Serial.print(address[0], HEX);
  Serial.print(" 0x");
  Serial.print(address[1], HEX);
  Serial.print(" 0x");
  Serial.print(address[2], HEX);
  Serial.print(" 0x");
  Serial.print(address[3], HEX);
  Serial.print(" 0x");
  Serial.print(address[4], HEX);
}

void printDeviceData(SBNetworkDevice &device){
	Serial.print(F("Device MAC = "));
	printAddress(device.MAC.Bytes);
	Serial.println();
	Serial.print(F("Master MAC = "));
	printAddress(device.MasterMAC.Bytes);
	Serial.println("");
	Serial.print(F("NetKey = "));
	Serial.print(device.NetworkKey, DEC);
	Serial.println("");
}

SBNetwork::SBNetwork(bool client, uint8_t cePin, uint8_t csPin) : radio(cePin, csPin){
	RunAsClient = client;
}

void SBNetwork::initialize(SBMacAddress mac){
	Serial.print(F("SBNetwork Version "));
	Serial.println(F(SB_VERSION));
	Serial.println(F("===================="));
	Serial.println();

	_LastTime = 0;
	_Uptime = 0;
	_NextCheck = 0;

	this->initializeNetworkDevice(NetworkDevice, mac);

	if (!this->RunAsClient) {
		this->MasterStorage = SBMasterStorage::initialize();
		for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
			Serial.print("Masterstorage Slot "); Serial.print(i); Serial.print(" ");
			printAddress(MasterStorage.Slaves[i]);
			Serial.println();
		}
	}

	Serial.print(F("Initializing NRF24L01 transmitter..."));
	this->radio.begin(); 
	
	// Set the PA Level low to prevent power supply related issues since this is a
	// getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
	this->radio.setPALevel(RF24_PA_HIGH);
	this->radio.enableDynamicPayloads();
	
	//this->radio.enableDynamicAck();
	this->radio.setAutoAck(false); // We use our own ack handling
	//this->radio.enableAckPayload();
	this->radio.setRetries(40, 5);
	
	// Listen at the own address
	this->radio.openReadingPipe(0, NetworkDevice.MAC);
    
	// Listen at the broadcast address
	this->radio.openReadingPipe(1, _BroadcastAddress);

	// Start the listening phase
	this->radio.startListening();
	Serial.println(F("Done"));

	if (this->RunAsClient) {
		// Connect to a master
		_Connected = false;
		while (!_Connected) {
			_Connected = connectToNetwork();
			delay(500); // This can be an endless loop in case of no connection to master is available
		}
	}
}

void SBNetwork::initializeNetworkDevice(SBNetworkDevice &device, SBMacAddress mac){
	Serial.print(F("Try to read device config from internal flash..."));
#if defined(ESP8266)
	EEPROM.begin(SB_NETWORK_FLASH_SIZE);
#endif
	EEPROM.get(0, device); // The first two bytes of a storage must always be 'D' 'S' ID to identifiy, that the device was already initiated
	if (device.ID[0] == 'D' && device.ID[1] == 'S'){
		Serial.println(F("Done"));
		printDeviceData(device);
	}
	else{
		Serial.println(F("Failed"));
		Serial.println(F("Creating new device config and stroing it to internal flash..."));
		device.ID[0] = 'D';
		device.ID[1] = 'S';
		device.MAC = mac;
		device.ConnectedToMaster = 0;
		device.NetworkKey = 0;
		// Write the data to EEPROM
		EEPROM.put(0, device);
#if defined(ESP8266)
		EEPROM.commit();
#endif
		Serial.println("Done");
		printDeviceData(device);
	}
#if defined(ESP8266)
	EEPROM.end();
#endif
}


void SBNetwork::resetData(){
  Serial.print(F("Erasing device configuration data..."));
#if defined(ESP8266)
  EEPROM.begin(SB_NETWORK_FLASH_SIZE);
#endif
  for(uint16_t i = 0; i < SB_NETWORK_FLASH_SIZE; i++){
    EEPROM.write(i, 0);
  }
#if defined(ESP8266)
  EEPROM.commit();
  EEPROM.end();
#endif
  Serial.println(F("Done"));
}


bool SBNetwork::sendToDevice(SBMacAddress mac, void* message, uint8_t messageSize){
#if defined(_DEBUG)
	Serial.print("Sending transmission");
#endif
	SBNetworkHeader header;
	header.ToAddress = mac;
	header.FromAddress = this->NetworkDevice.MAC;
	header.CommandType = SB_COMMAND_NO_COMMAND;
	
	SBNetworkFrame frame = SBNetworkFrame();
	frame.Header = header;
	uint8_t maxPackageSize = MAX_PACKAGE_SIZE;
	if (messageSize <= maxPackageSize){
		// We can send directly without fragmentation
		frame.Header.FragmentNr = 0;
		frame.Header.FragmentCount = 1;
		frame.Header.PackageId = millis();
		frame.MessageSize = messageSize;
		frame.Message = (uint8_t*)message;
		bool bSuccess = this->sendToDevice(frame);
#if defined(_DEBUG)
		if (bSuccess) {
			Serial.println(" Done");
		}
		else {
			Serial.println(" Failed");
		}
#endif
		return bSuccess;
	}
	else{
		// We have to send it in fragments
		uint8_t fragmentCount = messageSize / maxPackageSize;
		if ((fragmentCount * maxPackageSize) < messageSize){
			fragmentCount++;
		}
		for (uint8_t i = 0; i < fragmentCount; i++){
#if defined(_DEBUG)
			Serial.print(".");
#endif
			uint8_t buffer[32];
			if (i != (fragmentCount - 1)){
				memcpy(buffer, (uint8_t*)message + (i * maxPackageSize), maxPackageSize);
				frame.Message = (uint8_t*)buffer;
				frame.MessageSize = maxPackageSize;
			}
			else{
				memcpy(buffer, (uint8_t*)message + (i * maxPackageSize), messageSize - (i*maxPackageSize));
				frame.Message = (uint8_t*)buffer;
				frame.MessageSize = messageSize - (i*maxPackageSize);
			}
			frame.Header.FragmentCount = fragmentCount;
			frame.Header.FragmentNr = i;

			bool bSuccess = this->sendToDevice(frame);
			if (!bSuccess){
#if defined(_DEBUG)
				Serial.println(" Failed ");
#endif
				return false;
			}
		}
#if defined(_DEBUG)
		Serial.println(" Done");
#endif
		return true;
	}
}

bool SBNetwork::sendToDeviceInternal(SBNetworkFrame frame, bool waitForAck) {
	uint8_t bufferSize = sizeof(SBNetworkHeader) + frame.MessageSize;
	uint8_t buffer[32]; // = (uint8_t*)malloc(bufferSize);
	memcpy(buffer, &frame.Header, sizeof(SBNetworkHeader));
	if (frame.MessageSize > 0) {
		memcpy(buffer + sizeof(SBNetworkHeader), frame.Message, frame.MessageSize);
	}
	bool bSuccess = false;
	uint8_t iCounter = 0;
	while (!bSuccess && iCounter < RETRY_COUNT) {
		// Send to broadcast
		radio.stopListening();
		radio.openWritingPipe(frame.Header.ToAddress);
		bSuccess = radio.write(buffer, bufferSize);
		radio.openReadingPipe(0, this->NetworkDevice.MAC);
		radio.startListening();
		if (bSuccess) {
			bSuccess = waitForAck ? waitForAckFrom(frame.Header.ToAddress) : true;
		}
		delay(40); // Waittime between two sendings
		iCounter++;
	}
	return bSuccess;
}

bool SBNetwork::sendToDevice(SBNetworkFrame frame){
	return sendToDeviceInternal(frame, true);
}

bool SBNetwork::waitForAckFrom(SBMacAddress mac) {
#if defined(_DEBUG)
	Serial.print(F("Wait for Ack... "));
#endif
	long lNow = millis();
	// We need the counter in case of the unoccationally case, that during the wait, the timer overflows ans starts with 0
	uint16_t iCounter = 1;
	SBNetworkFrame frame;
	while (lNow + ACK_WAIT > millis() && iCounter) {
		if (this->receiveInternal(&frame)) {
			if (frame.Header.FromAddress.isEquals(mac)) {
				if (frame.Header.CommandType == SB_COMMAND_ACK) {
#if defined(_DEBUG)
					Serial.print(F("Done - Counter = ")); Serial.println(iCounter);
#endif
					return true;
				}
			}
		}
		iCounter++;
	}
#if defined(_DEBUG)
	Serial.print(F("Failed - Counter = ")); Serial.println(iCounter);
#endif
	return false;
}

bool SBNetwork::sendAckTo(SBMacAddress mac) {
	SBNetworkFrame frame;
	frame.Header.ToAddress = mac;
	frame.Header.FromAddress = this->NetworkDevice.MAC;
	frame.Header.FragmentCount = 1;
	frame.Header.FragmentNr = 0;
	frame.Header.PackageId = millis();
	frame.Header.CommandType = SB_COMMAND_ACK;
	frame.MessageSize = 0;
	return sendToDeviceInternal(frame, false);
}

bool SBNetwork::receiveInternal(SBNetworkFrame *frame) {
	uint8_t pipe = -1;
	if (radio.available(&pipe)) {
		// Variable for the received timestamp
		uint8_t size = radio.getDynamicPayloadSize();
		if (size == 0) {
			return false;
		}
		else {
			byte buffer[32];
			radio.read(buffer, size);
			// We cant use the target address of frame, because the first element in frame is the header
			memcpy(frame, buffer, sizeof(SBNetworkHeader));
			frame->MessageSize = size - sizeof(SBNetworkHeader);
			if (frame->MessageSize > 0) {
				//uint8_t *payload = (uint8_t*)malloc(frame->MessageSize);
				memcpy(_ReceiveBuffer, buffer + sizeof(SBNetworkHeader), frame->MessageSize);
				frame->Message = _ReceiveBuffer;
			}
			return true;
		}
	}
	return false;
}

bool SBNetwork::receive(SBNetworkFrame *frame){
	if (receiveInternal(frame)) {
		// We must check, if the received package is a NO_COMMAND_PACKAGE otherwise we have to handle it internally
		return this->handleCommandPackage(frame);
	}
}

bool SBNetwork::receiveMessage(void **message, uint8_t *messageSize, SBMacAddress *mac){
	uint8_t pipe = -1;
	uint8_t maxPackageSize = MAX_PACKAGE_SIZE;
	if (radio.available()){
		SBNetworkFrame frame;
		bool bReceive = this->receive(&frame);
		if (bReceive) {
#ifdef _DEBUG
			Serial.print("Incomming transmission from ");
			printAddress(frame.Header.FromAddress);
			Serial.println();
#endif
			if (frame.Header.FragmentCount == 1) {
				// We only have to receive this package
				memcpy(_ReadBuffer, frame.Message, maxPackageSize);
				(*message) = _ReadBuffer;
				(*messageSize) = frame.MessageSize;
				(*mac) = frame.Header.FromAddress;
				return true;
			}
			else if (frame.Header.FragmentNr == 0) {
				//Serial.print(frame.Header.FragmentNr + 1); Serial.print("/"); Serial.println(frame.Header.FragmentCount);
				// We have to receive more packages
				memcpy(_ReadBuffer, frame.Message, maxPackageSize);
				delay(50); // We need a delay here, because the opposite needs time to send the next package
				while (radio.available()) {
					bReceive = this->receive(&frame);
					if (!bReceive) {
						return false;
					}
					else {
						memcpy(_ReadBuffer + (frame.Header.FragmentNr * maxPackageSize), frame.Message, frame.MessageSize);
						if (frame.Header.FragmentNr == (frame.Header.FragmentCount - 1)) {
							// Last fragment received
							*message = _ReadBuffer;
							*messageSize = ((frame.Header.FragmentCount - 1) * maxPackageSize) + frame.MessageSize;
							(*mac) = frame.Header.FromAddress;
							return true;
						}
						delay(10);
					}
				}
				return false;
			}
			else {
				return false;
			}
		}
		else {
			return false;
		}
	}
	return false;
}

bool SBNetwork::connectToNetwork(){
	if (this->RunAsClient) {
		Serial.print(F("Try to connect to master..."));
		// First we have to check, if we already have a master stored
		if (!this->NetworkDevice.ConnectedToMaster) {
			Serial.println(F("Warning - Not paired to a master"));
			Serial.print(F("Sending broadcast transmission to find a master..."));
			// If not, we have to search for a master
			SBNetworkHeader header;
			header.ToAddress = this->_BroadcastAddress;
			header.FromAddress = this->NetworkDevice.MAC;
			header.CommandType = SB_COMMAND_SEARCH_MASTER;
			header.FragmentCount = 1;
			header.PackageId = millis();

			SBNetworkFrame frame;
			frame.Header = header;
			frame.Message = NULL;
			frame.MessageSize = 0;
			bool bMasterAck = this->sendToDeviceInternal(frame, false);
			unsigned long started_waiting_at = millis();
			boolean timeout = false;
			while (!this->receive(&frame)) {
				if ((millis() - started_waiting_at) > 1000) {
					timeout = true;
					break;
				}
			}

			if (timeout) {
				Serial.println(F("Timeout"));
				return false;
			}
			else {
				if (frame.Header.CommandType != SB_COMMAND_MASTER_ACK) {
					if (frame.MessageSize > 0) {
						free(frame.Message);
					}
					Serial.println(F("Failed - Got answer but no master ack"));
					return false;
				}
				else {
					Serial.println(F("Done"));
					Serial.print(F("Got answer from a master. Master-MAC is "));
					printAddress(frame.Header.FromAddress);
					Serial.println();
					Serial.print(F("Try to pair with master..."));
					SBNetworkFrame conFrame;
					conFrame.Header.CommandType = SB_COMMAND_REQUEST_PAIRING;
					conFrame.Header.FragmentCount = 1;
					conFrame.Header.FragmentNr = 0;
					conFrame.Header.FromAddress = this->NetworkDevice.MAC;
					conFrame.Header.PackageId = millis();
					conFrame.Header.ToAddress = frame.Header.FromAddress;
					conFrame.MessageSize = 0;
					if (!this->sendToDeviceInternal(conFrame, false)) {
						Serial.println(F("Failed - Sending pairing request"));
					}
					else {
						while (!this->receive(&frame)) {
							if (millis() - started_waiting_at > 1000) {
								timeout = true;
								break;
							}
						}
						if (timeout) {
							Serial.println(F("Timeout"));
							return false;
						}
						if (frame.Header.CommandType != SB_COMMAND_PAIRING_ACK) {
							Serial.println(F("Failed - Pairing rejected from the master"));
							return false;
						}
						else {
							this->NetworkDevice.MasterMAC = frame.Header.FromAddress;
							this->NetworkDevice.NetworkKey = *(frame.Message);
							this->NetworkDevice.ConnectedToMaster = -1;
							EEPROM.put(0, NetworkDevice);
							Serial.println("Suceeded");
							Serial.print("Try to ping to master...");
							delay(100);
						}
					}
				}
			}
		}

		bool bMasterAvailable = this->pingDevice(this->NetworkDevice.MasterMAC);
		return bMasterAvailable;
	}
	else {
		return false;
	}
}

bool SBNetwork::pingDevice(SBMacAddress mac){
	SBNetworkHeader header;
	header.ToAddress = mac;
	header.FromAddress = this->NetworkDevice.MAC;
	header.CommandType = SB_COMMAND_PING;
	header.FragmentCount = 1;
	header.FragmentNr = 0;
	header.PackageId = millis();

	SBNetworkFrame frame;
	frame.Header = header;
	frame.Message = NULL;
	frame.MessageSize = 0;

	bool bSend = this->sendToDevice(frame);
	if (bSend) {
		Serial.println(F("Done - Device available"));
	}
	else {
		Serial.println("Failed - Device not responding");
	}
	return bSend;
}

bool SBNetwork::handleCommandPackage(SBNetworkFrame *frame){	
	if (!this->RunAsClient) {
		// First check, if the device is listed in the storage
		bool bFound = false;
		for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
			if (this->MasterStorage.Slaves[i].isEquals(frame->Header.FromAddress)) {
				_SlavePings[i] = _Uptime;
				bFound = true;
				break;
			}
		}

		// Look, if we must handle a command package
		switch (frame->Header.CommandType) {
		case SB_COMMAND_PING: {
#ifdef _DEBUG
			Serial.println(F("Received 'PING'"));
#endif
			// Only, when the device is a paired slave, send a ping back
			if (bFound) {
				sendAckTo(frame->Header.FromAddress);
			}
			break;
		}
		case SB_COMMAND_SEARCH_MASTER: {
			Serial.print(F("Received 'SEARCH_MASTER'. "));
			// When automatic Client adding is activated
			if (_EnableAutomaticClientAdding) {
				Serial.print(F("Send MasterACK..."));
				delay(20);
				bool bSend = sendMasterAck(frame->Header.FromAddress);
				if (bSend) {
					Serial.println(F("Done"));
				}
				else {
					Serial.println(F("Failed"));
				}
			}
			else {
				Serial.println(F("AutomaticClientAdding is deactivaed. Ignoring package."));
			}
			break;
		}
		case SB_COMMAND_REQUEST_PAIRING: {
			Serial.print(F("Received 'PAIRING_REQUEST'. "));
			// When automatic Client adding is activated
			if (_EnableAutomaticClientAdding) {
				Serial.print(F("Send PairingACK... "));
				delay(20);
				// This is the point where we could stop prpcessing and wait for an user input on the controller to let the new device access the network
				bool bSend = sendPairingAck(frame->Header.FromAddress);
				// If sending was successfull, then add the new slave
				if (bSend) {
					Serial.println(F("Done")); 
					Serial.print(F("Storing new MAC to MasterStorage... "));
					addMac(frame->Header.FromAddress);
					Serial.println(F("Done"));
				}
				else {
					Serial.println(F("Failed"));
				}
			}
			else {
				Serial.println(F("AutomaticClientAdding is deactivaed. Ignoring package."));
			}
			break;
		}
		case SB_COMMAND_NO_COMMAND:
#ifdef _DEBUG
			Serial.println(F("Received 'NO_COMMAND'"));
#endif
			if (bFound) {
				return sendAckTo(frame->Header.FromAddress);
			}
		}
		// Package was handled by handleCommandPackage();
		return false;
	}
	else {
		return sendAckTo(frame->Header.FromAddress);
	}
}

bool SBNetwork::sendMasterAck(SBMacAddress mac){
	if (!this->RunAsClient) {
		SBNetworkHeader header;
		header.ToAddress = mac;
		header.FromAddress = this->NetworkDevice.MAC;
		header.CommandType = SB_COMMAND_MASTER_ACK;
		header.FragmentCount = 1;
		header.PackageId = millis();

		SBNetworkFrame frame;
		frame.Header = header;
		frame.Message = (uint8_t*)&(this->NetworkDevice.NetworkKey);
		frame.MessageSize = sizeof(uint32_t);
		return this->sendToDeviceInternal(frame, false);
	}
	else {
		return false;
	}
}


bool SBNetwork::sendPairingAck(SBMacAddress mac){
	if (!this->RunAsClient) {
		SBNetworkHeader header;
		header.ToAddress = mac;
		header.FromAddress = this->NetworkDevice.MAC;
		header.CommandType = SB_COMMAND_PAIRING_ACK;
		header.FragmentCount = 1;
		header.PackageId = millis();

		SBNetworkFrame frame;
		frame.Header = header;
		frame.Message = NULL;
		frame.MessageSize = 0;
		return this->sendToDeviceInternal(frame, false);
	}
	else {
		return false;
	}
}

bool SBNetwork::checkMaster(){
	if (this->RunAsClient) {
		if (this->pingDevice(this->NetworkDevice.MasterMAC)) {
#ifdef _DEBUG
			Serial.println("Master OK");
#endif
			return true;
		}
		else {
#ifdef _DEBUG
			Serial.println("Master ERROR");
#endif
			return false;
		}
	}
	else {
		return false;
	}
}

void SBNetwork::update(){

	// Update the uptime counter
	if (_LastTime > millis()){
		long add = millis();
		_Uptime += add;
	}
	else{
		long add = millis() - _LastTime;
		_Uptime += add;
	}
	_LastTime = millis();

	if (this->RunAsClient) {
		if (NetworkDevice.ConnectedToMaster && MASTER_CHECK_INTERVAL) {
			if (_Uptime > _NextCheck) {
				// Now we have to check our sensors if they are still available
				_NextCheck = _Uptime + MASTER_CHECK_INTERVAL;
				checkMaster();
			}
		}
	}

	_LastReceivedMessageSize = 0;
	_LastReceivedMessage = NULL;
	SBMacAddress fromAddress;
	if (!receiveMessage((void**)&_LastReceivedMessage, &_LastReceivedMessageSize, &_LastReceivedFromAddress)) {
		_LastReceivedMessageSize = 0;
		_LastReceivedMessage = NULL;
	}
}

uint8_t SBNetwork::addMac(SBMacAddress mac){
	if (!this->RunAsClient) {
		// iterate through the storage and look if the mac already exists
		uint8_t iPos;
		for (iPos = 0; iPos < MAX_CLIENTS; iPos++) {
			if (MasterStorage.Slaves[iPos].isEquals(mac)) {
				return iPos;
			}
		}
		// Search the first free place and add the mac
		for (iPos = 0; iPos < MAX_CLIENTS; iPos++) {
			if (MasterStorage.Slaves[iPos].isEquals(EMPTY_MAC)) {
				MasterStorage.Slaves[iPos] = mac;
				MasterStorage.save();
				return iPos;
			}
		}
		return -1;
	}
	else {
		return -1;
	}
}

uint8_t SBNetwork::removeMac(SBMacAddress mac){
	if (!this->RunAsClient) {
		// iterate through the storage and look if the mac is in the list, if not, then return -1. If yes, remove it.
		for (uint8_t iPos = 0; iPos < MAX_CLIENTS; iPos++) {
			if (MasterStorage.Slaves[iPos].isEquals(mac)) {
				MasterStorage.Slaves[iPos] = EMPTY_MAC;
				MasterStorage.save();
				return iPos;
			}
		}
		return -1;
	}
}
