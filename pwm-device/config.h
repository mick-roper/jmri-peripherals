// config.h

#ifndef CONFIG_H
#define CONFIG_H

#include "turnout.h"  // Include the Turnout class first to ensure it's fully defined
#include <Adafruit_PWMServoDriver.h>

// Enable Serial output (Uncomment to enable)
// #define ENABLE_SERIAL

#define SERVOMIN 150          // Minimum pulse length count (out of 4096)
#define SERVOMAX 600          // Maximum pulse length count (out of 4096)
#define REPORT_INTERVAL 2000  // Interval to report servo state (in milliseconds)

#define ERROR_LED_0 2  // LED for error bit 0
#define ERROR_LED_1 3  // LED for error bit 1
#define ERROR_LED_2 4  // LED for error bit 2
#define ERROR_LED_3 5  // LED for error bit 3

// Network and MQTT configuration
inline byte mac[] = { 0xA8, 0x61, 0x0A, 0xAF, 0x07, 0x2C };   // Custom MAC address
inline const char* mqtt_server = "192.168.178.74";            // MQTT server IP address
inline const char* mqtt_topic_base = "turnout/";              // Base topic for turnouts
inline const char* mqtt_client_name = "turnout-control-1";  // MQTT client name

// PWM Boards Configuration
inline const int NUM_PWM_BOARDS = 1;  // Number of PCA9685 boards
inline Adafruit_PWMServoDriver pwms[NUM_PWM_BOARDS] = {
  Adafruit_PWMServoDriver(0x40),  // First PCA9685, I2C address 0x40
};

// Turnout Configuration
inline const int NUM_TURNOUTS = 8;  // Number of turnouts
inline Turnout turnouts[NUM_TURNOUTS] = {
  Turnout("lower_storage_1", &pwms[0], 0, 60, 30),  // ID: "turnout1", Board 0 (0x40), Servo 0
  Turnout("lower_storage_2", &pwms[0], 1, 60, 30),  // ID: "turnout2", Board 0 (0x40), Servo 1
  Turnout("lower_storage_3", &pwms[0], 2, 60, 30),  // ID: "turnout3", Board 1 (0x41), Servo 0
  Turnout("lower_storage_4", &pwms[0], 3, 60, 30),  // ID: "turnout3", Board 1 (0x41), Servo 0
  Turnout("lower_storage_5", &pwms[0], 4, 60, 30),  // ID: "turnout3", Board 1 (0x41), Servo 0
  Turnout("lower_storage_6", &pwms[0], 5, 63, 30),  // ID: "turnout3", Board 1 (0x41), Servo 0
  Turnout("lower_storage_7", &pwms[0], 6, 60, 30),  // ID: "turnout3", Board 1 (0x41), Servo 0
  Turnout("lower_storage_8", &pwms[0], 7, 60, 30),  // ID: "turnout3", Board 1 (0x41), Servo 0
};

#endif  // CONFIG_H
