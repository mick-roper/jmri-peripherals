#ifndef CONFIG_H
#define CONFIG_H

#include <Ethernet.h>
#include <PN532.h>
#include <PN532_I2C.h>
#include <Adafruit_PWMServoDriver.h>

#define USE_SERIAL
#define USE_ETHERNET
// #define USE_MQTT
// #define USE_SERVOS
// #define USE_RFID
// #define USE_ANALOG_DETECTION

#ifdef USE_ETHERNET
byte mac[] = {0xA8, 0x61, 0x0A, 0xAF, 0x07, 0x2C};
#endif

#ifdef USE_MQTT
const char broker[] = "HW101075";
const uint16_t port = 1883;
const char topic[] = "track/turnout/#";
#endif

#ifdef USE_SERVOS
enum ServoState : uint8_t {
  INTENT_TO_CLOSE,
  CLOSED,
  INTENT_TO_THROW,
  THROWN,
};

struct Servo {
  String id;
  uint8_t driver;
  uint8_t pin;
  uint16_t pwmMin;
  uint16_t pwmMax;
  uint16_t currentPos;
  ServoState state;
};

const uint8_t driverCount = 1;
Adafruit_PWMServoDriver drivers[driverCount] = {
  Adafruit_PWMServoDriver(0x40)
};

const uint8_t servoCount = 8;
Servo servos[servoCount] = {
    Servo{"track/turnout/0", 0, 0, 215, 270},
    Servo{"track/turnout/1", 0, 1, 200, 280},
    Servo{"track/turnout/2", 0, 2, 200, 280},
    Servo{"track/turnout/3", 0, 3, 200, 260},
    Servo{"track/turnout/4", 0, 4, 200, 280},
    Servo{"track/turnout/5", 0, 5, 200, 280},
    Servo{"track/turnout/6", 0, 6, 200, 280},
    Servo{"track/turnout/7", 0, 7, 200, 280},
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