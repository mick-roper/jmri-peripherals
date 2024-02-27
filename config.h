#ifndef CONFIG_H
#define CONFIG_H

#include <Ethernet.h>

#define USE_SERIAL

// ETHERNET STUFF
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
const IPAddress ip(192, 168, 1, 177);

// MQTT STUFF
const char broker[] = "HW101075";
const int port = 1883;
const char topic[] = "track/*";

// RFID STUFF
#define USE_RFID

// NFC STUFF
#define USE_NFC
#endif // CONFIG_H