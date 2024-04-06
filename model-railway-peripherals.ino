
#include "./logging.h"

#include <Adafruit_PWMServoDriver.h>
#include <Ethernet.h>
#include <MQTTClient.h>
#include <PN532.h>
#include <PN532_I2C.h>
#include <Wire.h>

byte ethernetMacAddress[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
const IPAddress ethernetIpAddress(192, 168, 1, 201);
const char broker[] = "HW101075";
const uint16_t port = 1883;
const char topic[] = "track/#";

EthernetClient ethernet;
MQTTClient mqttClient(ethernet);

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
  uint8_t pwmMin;
  uint8_t pwmMax;
  uint8_t currentPos;
  ServoState state;
};

const uint8_t servoCount = 8;
Adafruit_PWMServoDriver pwmDrivers[servoCount / 8] = {
    Adafruit_PWMServoDriver(0x40)};
Servo servos[servoCount] = {
    Servo{0, 0, 350, 450}, Servo{0, 1, 350, 450}, Servo{0, 2, 350, 450},
    Servo{0, 3, 350, 450}, Servo{0, 4, 350, 450}, Servo{0, 5, 350, 450},
    Servo{0, 6, 350, 450}, Servo{0, 7, 350, 450},
};
#endif

#ifdef USE_RFID
struct RfidReader {
  uint8_t pcaAddress;
  uint8_t pin;
};

PN532_I2C pn532_i2c(Wire);
PN532 nfc = PN532(pn532_i2c);
const uint8_t rfidReaderCount = 2;
RfidReader rfidReaders[rfidReaderCount] = {
    RfidReader{0x70, 0},
    RfidReader{0x70, 1},
};
#endif

void setup() {
  logging::setup();

  logging::println("initialising peripherals...");

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

#ifdef USE_RFID
  for (uint8_t i = 0; i < rfidReaderCount; i++) {
    logging::print("checking for RFID reader on pca: ");
    logging::print(rfidReaders[i].pcaAddress, HEX);
    logging::print(" pin: ");
    logging::print(rfidReaders[i].pin);
    logging::println("...");

    pcaSelect(rfidReaders[i].pcaAddress, rfidReaders[i].pin);
    nfc.begin();

    uint32_t versiondata = nfc.getFirmwareVersion();
    if (!versiondata) {
      logging::println("Didn't find PN53x board");
      continue;
    }

    logging::print("Found chip PN5");
    logging::println((versiondata >> 24) & 0xFF, HEX);
    logging::print("Firmware ver. ");
    logging::print((versiondata >> 16) & 0xFF, DEC);
    logging::print('.');
    logging::println((versiondata >> 8) & 0xFF, DEC);

    nfc.SAMConfig();
  }
#endif

  logging::println("--- peripherals initialised! ---\n\n");
}

void loop() {
#ifdef USE_MQTT
  mqttClient.loop();
#endif

  moveServos();
  reportServoState();
  readRfidTags();
  reportAnalogOccupancy();
}

void pcaSelect(uint8_t pca, uint8_t pin) {
  if (pin > 7)
    return;

  Wire.beginTransmission(pca);
  Wire.write(1 << pin);
  Wire.endTransmission();
}

#ifdef USE_SERVOS
uint64_t lastServoMove;
#endif

void moveServos() {
#ifdef USE_SERVOS
  if (millis() - lastServoMove < 50) {
    return;
  }

  Adafruit_PWMServoDriver driver;

  for (uint8_t i = 0; i < servoCount; i++) {
    if (servos[i].state == ServoState::INTENT_TO_CLOSE) {
      driver = Adafruit_PWMServoDriver(servos[i].driver);
      if (servos[i].currentPos > servos[i].pwmMin) {
        driver.setPWM(servos[i].pin, 0, servos[i].currentPos--);
      } else {
        servos[i].state = ServoState::CLOSED;
      }
    } else if (servos[i].state == ServoState::INTENT_TO_THROW) {
      driver = Adafruit_PWMServoDriver(servos[i].driver);
      if (servos[i].currentPos < servos[i].pwmMax) {
        driver.setPWM(servos[i].pin, 0, servos[i].currentPos++);
      } else {
        servos[i].state = ServoState::THROWN;
      }
    }
  }

  lastServoMove = millis();
#endif
}

#ifdef USE_SERVOS
uint64_t lastServoReport;
#endif

void reportServoState() {
#ifdef USE_SERVOS
  if (millis() - lastServoReport < 1000) {
    return;
  }

  char topic[15];
  const String closed = "CLOSED";
  const String thrown = "THROWN";
  const String unknown = "UNKNOWN";
  for (uint8_t i = 0; i < servoCount; i++) {
    sprintf(topic, "track/turnout/%d", i + 1);
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
#endif
}

void mqttMessageHandler(String &topic, String &payload) {
#ifdef USE_SERVOS
  if (topic.startsWith("track/turnout/")) {
    uint8_t i = topic.substring(14).toInt();
    i -= 1;
    if (payload == "THROWN") {
      servos[i].state = ServoState::INTENT_TO_THROW;
    } else if (payload == "CLOSED") {
      servos[i].state = ServoState::INTENT_TO_CLOSE;
    }
  }
#endif

  // TODO: handle other cases here
}

uint64_t lastRfidRead;

void readRfidTags() {
#ifdef USE_RFID
  if (millis() - lastRfidRead > 250) {
    uint8_t success;
    uint8_t uid[7];
    uint8_t uidLength;
    for (uint8_t i = 0; i < rfidReaderCount; i++) {
      pcaSelect(rfidReaders[i].pcaAddress, rfidReaders[i].pin);
      success =
          nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 25);

      if (success) {
        logging::print("pca: ");
        logging::print(rfidReaders[i].pcaAddress, HEX);
        logging::print(" pin: ");
        logging::print(rfidReaders[i].pin, DEC);
        logging::println(" uid: ");
        for (uint8_t b = 0; b < uidLength; b++) {
          logging::print(uid[b], HEX);
        }
        logging::print("\n\n");
      }
    }

    lastRfidRead = millis();
  }
#endif
}

void publishMessage(String const &topic, String const &message) {
#ifdef USE_MQTT
  mqttClient.publish(topic, message);
#endif
}

void printCurrentRfidReaderFirmwareVersion() {
#ifdef USE_RFID
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.print("Didn't find PN53x board");
    return;
  }

  Serial.print("Found chip PN5");
  Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);
#endif
}

#ifdef USE_ANALOG_DETECTION
uint64_t lastAnalogRead;
#endif

void reportAnalogOccupancy() {
#ifdef USE_ANALOG_DETECTION
  if (millis() - lastAnalogRead < 500) {
    return;
  }

  int value = analogRead(PIN0);
  logging::print("analog detection -- DETECTOR 0: ");
  if (value) {
    logging::println("OCCUPIED");
  } else {
    logging::println("UNOCCUPIED");
  }

  lastAnalogRead = millis();
#endif
}