#pragma once

#include "./config.h"
#include "./logging.h"

#ifdef USE_ETHERNET
#include <Ethernet.h>
#endif

#ifdef USE_MQTT
#include <ArduinoMqttClient.h>
#endif // USE_MQTT

#ifdef USE_SERVOS
#include "./servo.h";
#include <Adafruit_PWMServoDriver.h>
#endif // USE_SERVOS

EthernetClient wire;
MqttClient mqttClient(wire);
Servos::ServoController servoCtrl = Servos::ServoController();

void setup() {
  logging::setup();

#ifdef USE_ETHERNET
  Ethernet.begin(ethernetMacAddress, ethernetIpAddress);
#endif // USE_ETHERNET

  while (!wire.connected()) {
    logging::println("network not connected...");
    delay(100);
  }

#ifdef USE_MQTT
  if (!mqttClient.connect(broker, port)) {
    logging::println("MQTT connection failed!");
    while (1)
      ;
  }

  Serial.print("MQTT broker connection established!");
  Serial.println();
#endif // USE_MQTT

  // servo setup:
  // setupServos();
}

void loop() {
  // 1: get input from MQTT

  // 2: update stuff

  // 3: report status
  // reportServoState();
}

void reportServoState() {
  Servos::ServoState data[Servos::servo_count];
  servoCtrl.GetServoStates(data);

  for (uint16_t i = 0; i < Servos::servo_count; i++) {
#ifdef USE_SERIAL
    Serial.print("servo: ");
    Serial.print(i, DEC);
    Serial.print(" > ");

    switch (data[i]) {
    case Servos::ServoState::Ready:
      Serial.print("READY");
      break;
    case Servos::ServoState::Closed:
      Serial.print("CLOSED");
      break;
    case Servos::ServoState::Thrown:
      Serial.print("THROWN");
      break;
    case Servos::ServoState::IntentToClose:
      Serial.print("INTENT=CLOSE");
      break;
    case Servos::ServoState::IntentToThrow:
      Serial.print("INTENT=THROW");
      break;
    default:
      break;
    }

    Serial.println();
#endif
  }
}

void setupServos() {
  servoCtrl.SetServoProps(0, 250, 450);
  servoCtrl.SetServoProps(1, 250, 450);
  servoCtrl.SetServoProps(2, 250, 450);
  servoCtrl.SetServoProps(3, 250, 450);
  servoCtrl.SetServoProps(4, 250, 450);
  servoCtrl.SetServoProps(5, 250, 450);
  servoCtrl.SetServoProps(6, 250, 450);
  servoCtrl.SetServoProps(7, 250, 450);
}

void reportBlockOccupancyState() {
#ifdef USE_SERIAL
  Serial.println("block occupancy not implemented!");
#endif
}

void reportStockDetectionState() {
#ifdef USE_SERIAL
  Serial.println("stock detection not implemented!");
#endif
}