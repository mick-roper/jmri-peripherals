#ifndef CONFIG_H
#define CONFIG_H

#include <Ethernet.h>
#include <PN532.h>
#include <PN532_I2C.h>
#include <Adafruit_PWMServoDriver.h>

#define USE_SERIAL
// #define USE_ETHERNET
// #define USE_MQTT
#define USE_SERVOS
// #define USE_RFID
// #define USE_ANALOG_DETECTION

#ifdef USE_ETHERNET
byte ethernetMacAddress[] = {0xA8, 0x61, 0x0A, 0xAF, 0x07, 0x2C};
const IPAddress ethernetIpAddress(192, 168, 1, 202);
#endif

#ifdef USE_MQTT
const char broker[] = "HW101075";
const uint16_t port = 1883;
const char topic[] = "track/turnout/#";
#endif

#ifdef USE_SERVOS
enum ServoState : uint8_t {
  UNKNOWN,
  INTENT_TO_CLOSE,
  CLOSED,
  INTENT_TO_THROW,
  THROWN,
};

struct Servo {
  String id;
  uint8_t driver;
  uint8_t pin;
  uint8_t pwmMin;
  uint8_t pwmMax;
  uint8_t currentPos;
  ServoState state;
};

const uint8_t driverCount = 1;
Adafruit_PWMServoDriver drivers[driverCount] = {
  Adafruit_PWMServoDriver(0x40)
};

const uint8_t servoCount = 8;
Servo servos[servoCount] = {
    Servo{"track/turnout/1", 0, 0, 350, 450},
    Servo{"track/turnout/2", 0, 1, 350, 450},
    Servo{"track/turnout/3", 0, 2, 350, 450},
    Servo{"track/turnout/4", 0, 3, 350, 450},
    Servo{"track/turnout/5", 0, 4, 350, 450},
    Servo{"track/turnout/6", 0, 5, 350, 450},
    Servo{"track/turnout/7", 0, 6, 350, 450},
    Servo{"track/turnout/8", 0, 7, 350, 450},
};
#endif

#ifdef USE_RFID
struct RfidReader {
  uint8_t pcaAddress;
  uint8_t pin;
};

const uint8_t rfidReaderCount = 2;
RfidReader rfidReaders[rfidReaderCount] = {
    RfidReader{0x70, 0},
    RfidReader{0x70, 1},
};
#endif

#endif // CONFIG_H