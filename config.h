#ifndef CONFIG_H
#define CONFIG_H

#include <Adafruit_PWMServoDriver.h>
#include <Ethernet.h>
#include <PN532.h>
#include <PN532_I2C.h>

// #define USE_SERIAL
#define USE_ETHERNET
#define USE_MQTT
// #define USE_SERVOS
// #define USE_SERVO_RELAYS
// #define USE_RFID
#define USE_ANALOG_DETECTION

#ifdef USE_ETHERNET
byte mac[] = {0xA8, 0x61, 0x0A, 0xAF, 0x07, 0x2C};
#endif

#ifdef USE_MQTT
const char *broker = "WINDOWS-NASO1K0";
const uint16_t brokerPort = 1883;
const char mqttTopic[] = "send/turnout/#";
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
  uint8_t driver;
  uint8_t pin;
  uint16_t pwmMin;
  uint16_t pwmMax;
  uint16_t currentPos;
  ServoState state;
};

const uint8_t driverCount = 1;
Adafruit_PWMServoDriver drivers[driverCount] = {Adafruit_PWMServoDriver(0x40)};

const uint8_t servoCount = 8;
Servo servos[servoCount] = {
    Servo{0, 0, 220, 280}, Servo{0, 1, 220, 260}, Servo{0, 2, 220, 260},
    Servo{0, 3, 200, 260}, Servo{0, 4, 200, 260}, Servo{0, 5, 200, 280},
    Servo{0, 6, 220, 260}, Servo{0, 7, 220, 280},
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