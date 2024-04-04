
#include "./logging.h"

#include <Adafruit_PWMServoDriver.h>
#include <Ethernet.h>
#include <MQTTClient.h>
#include <NfcAdapter.h>
#include <PN532.h>
#include <PN532_I2C.h>
#include <Wire.h>

// #define USE_ETHERNET
// #define USE_MQTT

enum ServoState : uint8_t {
  UNKNOWN,
  INTENT_TO_CLOSE,
  CLOSED,
  INTENT_TO_THROW,
  THROWN,
};

struct Servo {
  uint8_t driverIndex;
  uint8_t pinIndex;
  uint8_t pwmMin;
  uint8_t pwmMax;
  uint8_t currentPos;
  ServoState state;
};

struct RfidReader {
  uint8_t pcaAddress;
  uint8_t pin;
};

byte ethernetMacAddress[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
const IPAddress ethernetIpAddress(192, 168, 1, 201);
const char broker[] = "HW101075";
const uint16_t port = 1883;
const char topic[] = "track/#";

EthernetClient ethernet;
MQTTClient mqttClient(ethernet);

const uint8_t servoCount = 8;
Adafruit_PWMServoDriver pwmDrivers[servoCount / 8] = {
    Adafruit_PWMServoDriver(0x40)};
Servo servos[servoCount] = {
    Servo{0, 0, 350, 450}, Servo{0, 1, 350, 450}, Servo{0, 2, 350, 450},
    Servo{0, 3, 350, 450}, Servo{0, 4, 350, 450}, Servo{0, 5, 350, 450},
    Servo{0, 6, 350, 450}, Servo{0, 7, 350, 450},
};

PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);
const uint8_t rfidReaderCount = 2;
RfidReader rfidReaders[rfidReaderCount] = {
    RfidReader{0x70, 0},
    RfidReader{0x70, 1},
};

void setup() {
  logging::setup();
  Wire.begin();

#ifdef USE_ETHERNET
  Ethernet.begin(ethernetMacAddress, ethernetIpAddress);
  while (!ethernet.connected()) {
    logging::println("network not connected...");
    delay(100);
  }
#endif

#ifdef USE_MQTT
  if (!mqttClient.connect(broker, port)) {
    logging::println("MQTT connection failed!");
    while (1)
      ;
  }

  logging::println("MQTT connected!");

  mqttClient.onMessage(mqttMessageHandler);
  mqttClient.subscribe(topic);
#endif
}

void loop() {
#ifdef USE_MQTT
  mqttClient.loop();
#endif

  // 1: update stuff
  moveServos();

  // 2: report status
  reportServoState();
  readRfidTags();
}

void pcaSelect(uint8_t pca, uint8_t pin) {
  if (pin > 7)
    return;

  Wire.beginTransmission(pca);
  Wire.write(1 << pin);
  Wire.endTransmission();
}

uint64_t lastServoMove;

void moveServos() {
  if (millis() - lastServoMove < 50) {
    return;
  }

  for (uint8_t i = 0; i < servoCount; i++) {
    if (servos[i].state == ServoState::INTENT_TO_CLOSE) {
      if (servos[i].currentPos > servos[i].pwmMin) {
        servos[i].currentPos--;
        pwmDrivers[servos[i].driverIndex].setPWM(servos[i].pinIndex, 0,
                                                 servos[i].currentPos);
      } else {
        servos[i].state = ServoState::CLOSED;
      }
    } else if (servos[i].state == ServoState::INTENT_TO_THROW) {
      if (servos[i].currentPos < servos[i].pwmMax) {
        servos[i].currentPos++;
        pwmDrivers[servos[i].driverIndex].setPWM(servos[i].pinIndex, 0,
                                                 servos[i].currentPos);
      } else {
        servos[i].state = ServoState::THROWN;
      }
    }
  }

  lastServoMove = millis();
}

uint64_t lastServoReport;

void reportServoState() {
  if (millis() - lastServoReport < 1000) {
    return;
  }

  char topic[15];
  const String closed = "CLOSED";
  const String thrown = "CLOSED";
  const String unknown = "CLOSED";
  for (uint8_t i = 0; i < servoCount; i++) {
    sprintf(topic, "track/turnout/%d", i+1);
    switch (servos[i].state) {
    case ServoState::CLOSED: {
      publishMessage(topic, closed);
      break;
    }
    case ServoState::THROWN: {
      publishMessage(topic, thrown);
      break;
    }
    default:
      publishMessage(topic, unknown);
      break;
    }
  }

  lastServoReport = millis();
}

void mqttMessageHandler(String &topic, String &payload) {
  if (topic.startsWith("track/turnout/")) {
    uint8_t i = topic.substring(14).toInt();
    i -= 1;
    if (payload == "THROWN") {
      servos[i].state = ServoState::INTENT_TO_THROW;
    } else if (payload == "CLOSED") {
      servos[i].state = ServoState::INTENT_TO_CLOSE;
    }
  }

  // TODO: handle other cases here
}

uint64_t lastRfidRead;

void readRfidTags() {
  if (millis() - lastRfidRead < 250) {
    return;
  }

  char buffer[40];
  for (uint8_t i = 0; i < rfidReaderCount; i++) {
    pcaSelect(rfidReaders[i].pcaAddress, rfidReaders[i].pin);
    if (nfc.tagPresent()) {
      NfcTag t = nfc.read();
      sprintf(buffer, "pca: %d pin: %d: UID: %s", rfidReaders[i].pcaAddress, rfidReaders[i].pin, t.getUidString());
      logging::println(buffer);
    }
  }

  lastRfidRead = millis();
}

void publishMessage(String const &topic, String const &message) {
#ifdef USE_MQTT
  mqttClient.publish(topic, message);
#endif
}