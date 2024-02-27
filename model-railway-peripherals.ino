#pragma once

#include <Adafruit_PWMServoDriver.h>
#include <ArduinoMqttClient.h>
#include <Ethernet.h>

#include "./config.h"
#include "./servo.h";

EthernetClient ethernet;
MqttClient mqttClient(ethernet);
Servos::ServoController servoCtrl = Servos::ServoController();

void setup() {
#ifdef USE_SERIAL
  Serial.begin(9600);
  Serial.println("Serial output initialised..");
#endif

  // Ethernet.begin(mac, ip);
  // while (!ethernet.connected()) {
  //   Serial.print("ethernet not connected...");
  //   Serial.println();
  //   delay(100);
  // }

  // if (!mqttClient.connect(broker, port)) {
  //   Serial.print("MQTT connection failed...");
  //   Serial.println();
  //   while (1)
  //     ;
  // }

  // Serial.print("MQTT broker connection established!");
  // Serial.println();

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