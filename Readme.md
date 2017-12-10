# SBNetwork
Schullebernd Network library is a simple and easy to use library to connect devices like arduinos or ESP8266 via a nRF24L01 Transmitter.

## Master Client Network
With this library you can build a simple master/client network.
One device will be defined as a master device. All other devices are clients and connecting to the master. The master device is like a basic station (e.g. weather station with many sensors).
Usually the master device should have a connection to the LAN or WLAN (e.g. by using an ESP8266) and can forward communication from or to the clients (e.g. MQTT-Client or build in web interface). The client devices are (automatically) connecting to (pairing with) the master device. Typical clients are sensors or actors in a smart home automation environment  (e.g. temperature sensor).

## Target of this library
The target of this library is tobuild up a small and easy closed network of devices. I simply wanted to have a library for an easy integration of multible sensor and actor devices. But the devices shouldn't be a kind of an IoT device, which are directly connected to the LAN or internet. I know this is not a typical IoT thinking, but this definitifly reduced the overhead communication in the local LAN/WLAN. The target of this library is explicitly NOT to have TCP/IP library for nRF24L01 connected devices. Ther are many really good libraries available to achive this.

# How does it work?
It's quite simple. One device is defined as the master device. In my case it is an Wemos D1 mini / ESP826. The master devices is the device where the clients are connecting to. The master can allow or deny new client connections. If a client gets connected to the master, the master stores its mac in the internal flash. If a client is sending transmissions to a master and the master has not stored the clients mac address, the communication will be blocked. Clients are automatically searching for and connecting to a master. You don't have to configure this. The only thing that has to be done is to define a unique mac adress for the clients in the sketch. If a client is connected to a master the first time, it stores the master mac in the internal flash. Even if the client loses the power or turnes off, the master mac gets stored. After turning on the client again, the connection to the master gets restored.
The library uses the basic RF24 lib from https://tmrh20.github.io/RF24 and adds further functions for a simple master/client network.

