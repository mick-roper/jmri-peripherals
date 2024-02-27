#ifndef SERVO_H
#define SERVO_H

#include <Adafruit_PWMServoDriver.h>

namespace Servos {
const uint8_t min_loop_delay_ms = 50;
const uint8_t driver_count = 1;
const uint8_t servos_per_board = 8;
const uint16_t servo_count =
    driver_count * servos_per_board;

enum ServoState : uint8_t { Ready, Closed, Thrown, IntentToClose, IntentToThrow };

class Servo {
public:
  uint8_t driver;
  uint8_t pin;
  uint16_t servoMin;
  uint16_t servoMax;
  uint16_t pos;
  ServoState state;
};

class ServoController {
private:
  uint64_t lastUpdate;
  Adafruit_PWMServoDriver drivers[driver_count];
  Servo servos[servo_count];
  void SetRelay(uint8_t driver, uint8_t pin, bool thrown);

public:
  ServoController();
  void SetServoProps(uint16_t index, uint16_t servoMin, uint16_t servoMax);
  void Update();
  void Throw(uint16_t ix);
  void Close(uint16_t ix);
  void GetServoStates(ServoState* data);
};
}; // namespace Servos
#endif // SERVO_H