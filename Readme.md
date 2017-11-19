# SBNetwork
Schullebernd Network library is a simple and easy to use library to connect devices like arduinos or ESP8266 via a nRF24L01 Transmitter.
It uses the basic RF24 lib from https://tmrh20.github.io/RF24 and add further functions for a simple master/client network.

# Master Client Network
With this library you can build a simple master/client network.
One device will be defined as master device. The master device is like a basic station and all clients are connected to it.
Usually the master device should have a connection to the LAN or WLAN (e.g. by using an ESP8266) and can forward communication from or to the clients (e.g. MQTT-Client or own web interface).
The clients devices are connecting (pairing) to a master device. Typical clients are sensors or actors in a smart home automation environment  (e.g. temperature sensor).


