#include "./config.h"
#include "./logging.h"

#include <Adafruit_PWMServoDriver.h>
#include <Ethernet.h>
#include <PN532.h>
#include <PN532_I2C.h>
#include <PubSubClient.h>
#include <SPI.h>
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

int i;

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
  mqttClient.setServer(broker, brokerPort);
  mqttClient.setCallback(mqttMessageHandler);

  logging::println("MQTT is set up!");
#endif

#ifdef USE_SERVOS
  logging::println("configuring PWM drivers...");
  for (i = 0; i < driverCount; i++) {
    drivers[i].begin();
    drivers[i].setOscillatorFrequency(27000000);
    drivers[i].setPWMFreq(50);
  }

  logging::println("PWM drivers configured!");
#endif

#ifdef USE_RFID
  for (i = 0; i < rfidReaderCount; i++) {
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
  mqttReconnect();
#endif

  moveServos();
  readRfidTags();
  reportAnalogOccupancy();
  reportServoStatus();
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

  for (i = 0; i < servoCount; i++) {
    if (servos[i].state == ServoState::INTENT_TO_CLOSE) {
      if (servos[i].currentPos > servos[i].pwmMin) {
        drivers[servos[i].driver].setPWM(servos[i].pin, 0,
                                         servos[i].currentPos--);
      } else {
        servos[i].state = ServoState::CLOSED;
#ifdef USE_SERVO_RELAYS
        drivers[servos[i].driver].setPin(servos[i].pin + 8, 0);
#endif
      }

      continue;
    }

    if (servos[i].state == ServoState::INTENT_TO_THROW) {
      if (servos[i].currentPos < servos[i].pwmMax) {
        drivers[servos[i].driver].setPWM(servos[i].pin, 0,
                                         servos[i].currentPos++);
      } else {
        servos[i].state = ServoState::THROWN;
#ifdef USE_SERVO_RELAYS
        drivers[servos[i].driver].setPin(servos[i].pin + 8, 4096);
#endif
      }

      continue;
    }
  }

  lastServoMove = millis();
#endif
}

#ifdef USE_SERVOS
uint64_t lastServoReport;
#endif

void reportServoStatus() {
#ifdef USE_SERVOS
  if (millis() - lastServoReport > 2000) {
    char topicBuffer[15];
    for (i = 0; i < servoCount; i++) {
      snprintf(topicBuffer, 15, "recv/turnout/%d", i);
      switch (servos[i].state) {
      case ServoState::THROWN:
        publishMessage(topicBuffer, "THROWN");
        break;
      case ServoState::CLOSED:
        publishMessage(topicBuffer, "CLOSED");
        break;
      default:
        publishMessage(topicBuffer, "UNKNOWN");
        break;
      }
    }

    lastServoReport = millis();
  }
#endif
}

uint64_t lastRfidRead;

void readRfidTags() {
#ifdef USE_RFID
  if (millis() - lastRfidRead > 333) {
    uint8_t success;
    uint8_t uid[7];
    uint8_t uidLength;
    String topic;
    char message[14];
    for (i = 0; i < rfidReaderCount; i++) {
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
  if (length == 0) {
    return;
  }

  String t = String(topic);

#ifdef USE_SERVOS
  i = t.lastIndexOf('/');
  if (i > -1) {
    i = t.substring(i + 1).toInt();

    if (strncmp((char *)payload, "THROWN", length)) {
      servos[i].state = ServoState::INTENT_TO_THROW;
    } else if (strncmp((char *)payload, "CLOSED", length)) {
      servos[i].state = ServoState::INTENT_TO_CLOSE;
    }
  }
#endif

  // TODO: handle other cases here
}

void mqttReconnect() {
#ifdef USE_MQTT
  while (!mqttClient.connected()) {
    logging::println("Attempting MQTT connection...");
    if (mqttClient.connect("model-rail-peripherals")) {
      logging::println("connected!");
      mqttClient.publish("hello", "connected!");
      mqttClient.subscribe(mqttTopic);
      break;
    }

    logging::print("mqtt connection failed, rc=");
    logging::print(mqttClient.state());
    logging::println(" retrying...");
    delay(2000);
  }
  mqttClient.loop();
#endif
}