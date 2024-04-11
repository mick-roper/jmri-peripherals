#include "./config.h"
#include "./logging.h"

#include <Adafruit_PWMServoDriver.h>
#include <Ethernet.h>
#include <PN532.h>
#include <PN532_I2C.h>
#include <PubSubClient.h>
#include <Wire.h>

#include "./EmonLib.h"

#ifdef USE_ETHERNET
EthernetClient ethernet;
#endif

#ifdef USE_MQTT
PubSubClient mqttClient;
#endif

#ifdef USE_RFID
PN532_I2C pn532_i2c(Wire);
PN532 nfc = PN532(pn532_i2c);
#endif

EnergyMonitor emon1;

void setup() {
  logging::setup();

  logging::println("initialising peripherals...");

#ifdef USE_ETHERNET
  Ethernet.init();
  Ethernet.begin(mac);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    logging::println("Ethernet shield was not found.  Sorry, can't run without "
                     "hardware. :(");
    while (true)
      ; // do nothing, no point running without Ethernet hardware
  }

  if (Ethernet.linkStatus() == LinkOFF) {
    logging::println("Ethernet cable is not connected.");
    while (true)
      ;
  }

  logging::println("device is connected to the network!");
#endif

#ifdef USE_MQTT
  mqttClient.setClient(ethernet);
  mqttClient.setServer(brokerIp, brokerPort);
  mqttClient.setCallback(mqttMessageHandler);

  while (!mqttClient.connected()) {
    if (mqttClient.connect("arduinoClient")) {
      logging::println("MQTT connected!");
    } else {
      logging::print("MQTT connection failed: ");
      logging::println(mqttClient.state());
      delay(1000);
    }
  }

  mqttClient.subscribe(mqttTopic, 2);

  logging::println("MQTT is set up!");
#endif

#ifdef USE_SERVOS
  logging::println("configuring PWM drivers...");
  for (uint8_t i = 0; i < driverCount; i++) {
    drivers[i].begin();
    drivers[i].setOscillatorFrequency(27000000);
    drivers[i].setPWMFreq(50);
  }

  logging::println("PWM drivers configured!");
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

#ifdef USE_ANALOG_DETECTION
  Wire.begin();

  // CT readers
  emon1.current(A0, 111.1);
#endif

  logging::println("--- peripherals initialised! ---\n\n");
}

uint64_t lastServoStateChange;

void loop() {
#ifdef USE_MQTT
  mqttClient.loop();
#endif

  moveServos();
  readRfidTags();
  reportAnalogOccupancy();

  if (millis() - lastServoStateChange > 5000) {
    if (millis() % 2 == 0) {
      for (uint8_t i = 0; i < servoCount; i++) {
        servos[i].state = ServoState::INTENT_TO_THROW;
      }
    } else {

      for (uint8_t i = 0; i < servoCount; i++) {
        servos[i].state = ServoState::INTENT_TO_CLOSE;
      }
    }

    lastServoStateChange = millis();
  }
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
  if (millis() - lastServoMove < 25) {
    return;
  }

  for (uint8_t i = 0; i < servoCount; i++) {
    if (servos[i].state == ServoState::INTENT_TO_CLOSE) {
      if (servos[i].currentPos > servos[i].pwmMin) {
        drivers[servos[i].driver].setPWM(servos[i].pin, 0,
                                         servos[i].currentPos--);
      } else {
        servos[i].state = ServoState::CLOSED;
        drivers[servos[i].driver].setPin(servos[i].pin + 8, 0);
      }

      continue;
    }

    if (servos[i].state == ServoState::INTENT_TO_THROW) {
      if (servos[i].currentPos < servos[i].pwmMax) {
        drivers[servos[i].driver].setPWM(servos[i].pin, 0,
                                         servos[i].currentPos++);
      } else {
        servos[i].state = ServoState::THROWN;
        drivers[servos[i].driver].setPin(servos[i].pin + 8, 4096);
      }

      continue;
    }
  }

  lastServoMove = millis();
#endif
}

// uint64_t lastServoReport;

// void reportServoState() {
// #ifdef USE_SERVOS
//   if (millis() - lastServoReport < 3000) {
//     return;
//   }

//   for (uint8_t i = 0; i < servoCount; i++) {
//     switch(servos[i].state) {
//       case ServoState::THROWN: {
//         mqttClient.publish(servos[i].id.c_str(), "THROWN");
//         break;
//       }
//       case ServoState::CLOSED: {
//         mqttClient.publish(servos[i].id.c_str(), "CLOSED");
//         break;
//       }
//       default: {
//         mqttClient.publish(servos[i].id.c_str(), "UNKNOWN");
//         break;
//       }
//     }
//   }

//   lastServoReport = millis();
// #endif
// }

uint64_t lastRfidRead;

void readRfidTags() {
#ifdef USE_RFID
  if (millis() - lastRfidRead > 333) {
    uint8_t success;
    uint8_t uid[7];
    uint8_t uidLength;
    String topic;
    char message[14];
    for (uint8_t i = 0; i < rfidReaderCount; i++) {
      pcaSelect(rfidReaders[i].pcaAddress, rfidReaders[i].pin);
      success =
          nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 25);

      if (success) {
        topic = "show/reporter/" + i;
        sprintf(message, "%x", uid);

        logging::print("message: ");
        logging::println(message);

        publishMessage(topic.c_str(), message);
      }
    }

    lastRfidRead = millis();
  }
#endif
}

void publishMessage(const char *topic, const char *message) {
#ifdef USE_MQTT
  mqttClient.publish(topic, message, false);
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
  if (millis() - lastAnalogRead < 1000) {
    return;
  }

  lastAnalogRead = millis();
  double irms = emon1.calcIrms(1000);

  logging::print("analog detection -- DETECTOR 0: ");
  if (irms * 230.0 > 15) {
    logging::println("OCCUPIED");
  } else {
    logging::println("UNOCCUPIED");
  }
#endif
}

void mqttMessageHandler(char *topic, byte *payload, unsigned int length) {
  logging::print("got message on topic: ");
  logging::println(topic);

  char data[length];
  for (unsigned int i = 0; i < length; i++) {
    data[i] = payload[i];
  }
  String message = String(data);

#ifdef USE_SERVOS
  for (uint8_t i = 0; i < servoCount; i++) {
    if (servos[i].id == topic) {
      if (message == "THROWN") {
        logging::println("setting state to INTENT_TO_THROW");
        servos[i].state = ServoState::INTENT_TO_THROW;
      } else if (message == "CLOSED") {
        logging::println("setting state to INTENT_TO_CLOSE");
        servos[i].state = ServoState::INTENT_TO_CLOSE;
      }
      return;
    }
  }
#endif

  // TODO: handle other cases here
}