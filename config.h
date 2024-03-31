#ifndef CONFIG_H
#define CONFIG_H

#include <Ethernet.h>

#define USE_SERIAL
#define USE_ETHERNET
#define USE_MQTT
#define USE_RFID
#define USE_NFC
#define USE_SERVOS

#ifdef USE_ETHERNET
// ETHERNET STUFF
byte ethernetMacAddress[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
const IPAddress ethernetIpAddress(192, 168, 1, 177);
#endif // USE_ETHERNET

// MQTT STUFF
#ifdef USE_MQTT
const char broker[] = "HW101075";
const int port = 1883;
const char topic[] = "track/#";
#endif // USE_MQTT


// RFID STUFF
#ifdef USE_RFID
#endif // USE_RFID

// NFC STUFF
#ifdef USE_NFC
#endif // USE_NFC

#endif // CONFIG_H