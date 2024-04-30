#include "./config.h"

#include <Adafruit_PWMServoDriver.h>
#include <Ethernet.h>
#include <PN532.h>
#include <PN532_I2C.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Wire.h>

#include "./logging.h"

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

#ifdef USE_ANALOG_DETECTION
#define ANALOG_DETECTOR_COUNT 18
#define ANALOG_DETECTOR_CT_RATIO 1000
#define ANALOG_DETECTOR_SHUNT_RES 20
#define ANALOG_DETECTOR_REF_VOLTAGE 1024
uint16_t i;
uint8_t j;
const uint8_t analogDetectionPinOffset = 2;
const uint16_t samples = 256;
uint16_t r_array[samples];
const uint8_t pinZero = 0;
float detectors[ANALOG_DETECTOR_COUNT];
#endif

void setup() {
  logging::setup();

  logging::println("initialising peripherals...");

  Wire.begin();

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

  logging::println("MQTT client is configured!");
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

  logging::println("--- peripherals initialised! ---\n\n");
}

uint64_t lastServoStateChange, lastOledPrint;
uint8_t lastPrintedDetector;

void loop() {
#ifdef USE_MQTT
  mqttReconnect();
#endif

  moveServos();
#ifdef USE_ANALOG_DETECTION
  analogDetectorLoop();
#endif

  readRfidTags();
  reportAnalogOccupancy();
  reportServoStatus();

  if (millis() - lastOledPrint > 1000) {
    char buf[20];

    sprintf(buf, "detector %d: %dmA", i,
            detectors[i + analogDetectionPinOffset]);
    logging::println(buf);

    if (lastPrintedDetector >= ANALOG_DETECTOR_COUNT) {
      lastPrintedDetector = 0;
    } else {
      lastPrintedDetector++;
    }

    lastOledPrint = millis();
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
  if (millis() - lastAnalogRead > 1000) {
    char buf[16];

    for (i = 0; i < ANALOG_DETECTOR_COUNT; i++) {
      sprintf(buf, "recv/sensor/%d", i);
      if (detectors[i] > 0) {
        publishMessage(buf, "ACTIVE");
      } else {
        publishMessage(buf, "INACTIVE");
      }
    }

    lastAnalogRead = millis();
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

void analogDetectorLoop() {
#ifdef USE_ANALOG_DETECTION
  for (i = 0; i < ANALOG_DETECTOR_COUNT; i++) {
    detectors[i] = readRms(i + analogDetectionPinOffset);
  }
#endif
}

float dc_offset = 0;
float rms = 0;

float readRms(uint8_t pin) {
  dc_offset = 0;
  rms = 0;

  for (i = 0; i < samples; i++) {
    r_array[i] = 0;
  }

  // read voltage at INPUT_CHANNEL 'n' times and save data to 'r_array'
  for (i = 0; i < samples; i++) {
    // adding another 2 bits using oversampling technique
    for (j = 0; j < 16; j++) {
      r_array[i] += digitalRead(pin);
    }
    r_array[i] /= 4;
  }

  // calculate signal average value (DC offset)
  for (i = 0; i < samples; i++) {
    dc_offset += r_array[i];
  }

  dc_offset = dc_offset / samples;

  // calculate AC signal RMS value
  for (i = 0; i < samples; i++) {
    if (abs(r_array[i] - dc_offset) > 3)
      rms += sq(r_array[i] - dc_offset);
  }

  rms = rms / samples;

  // calculate Arduino analog channel input RMS voltage in millivolts
  rms = sqrt(rms) * ANALOG_DETECTOR_REF_VOLTAGE /
        4096; // 4096 is max digital value for 12-bit number (oversampled ADC)

  // calculate current passing through the shunt resistor by applying Ohm's Law
  // (in milli Amps)
  rms = rms / ANALOG_DETECTOR_SHUNT_RES;
  // now we can get current passing through the CT in milli Amps
  return rms * ANALOG_DETECTOR_CT_RATIO;
}