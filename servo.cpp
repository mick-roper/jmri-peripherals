#include "./servo.h"
#include <Adafruit_PWMServoDriver.h>

namespace Servos {
ServoController::ServoController() {
  const uint8_t base_address = 0x40;
  uint16_t s_index;
  for (uint8_t d_index = 0; d_index < driver_count; d_index++) {
    drivers[d_index] = Adafruit_PWMServoDriver(base_address + d_index);
    drivers[d_index].begin();
    drivers[d_index].setOscillatorFrequency(27000000);
    drivers[d_index].setPWMFreq(50);

    for (uint8_t s_pin = 0; s_pin < servos_per_board; s_pin++) {
      s_index = (d_index * servos_per_board) + s_pin;
      servos[s_index].driver = d_index;
      servos[s_index].pin = s_pin;
      servos[s_index].servoMin = 250;
      servos[s_index].servoMax = 450;
      servos[s_index].pos = 250;
      servos[s_index].state = ServoState::Ready;
    }
  }
}

void ServoController::Update() {
  if (lastUpdate + min_loop_delay_ms < millis()) {
    uint8_t step = 1;

    for (uint16_t i = 0; i < servo_count; i++) {
      switch (servos[i].state) {
      case ServoState::IntentToThrow:
        if (servos[i].pos >= servos[i].servoMax) {
          servos[i].state = ServoState::Thrown;
          SetRelay(servos[i].driver, servos[i].pin + 8, false);
          continue;
        }

        drivers[servos[i].driver].setPWM(servos[i].pin, 0,
                                         servos[i].pos += step);
        break;
      case ServoState::IntentToClose:
        if (servos[i].pos <= servos[i].servoMin) {
          servos[i].state = ServoState::Closed;
          SetRelay(servos[i].driver, servos[i].pin + 8, true);
          continue;
        }

        drivers[servos[i].driver].setPWM(servos[i].pin, 0,
                                         servos[i].pos -= step);
        break;
      default:
        break;
      }
    }

    lastUpdate = millis();
  }
};

void ServoController::SetRelay(uint8_t driver, uint8_t pin, bool thrown) {
  if (thrown) {
    drivers[driver].setPWM(pin, 4096, 0);
  } else {
    drivers[driver].setPWM(pin, 0, 4096);
  }
};

void ServoController::GetServoStates(ServoState *data) {
  for (uint16_t i = 0; i < servo_count; i++) {
    data[i] = servos[i].state;
  }
}

void ServoController::SetServoProps(uint16_t index, uint16_t servoMin, uint16_t servoMax) {
  servos[index].servoMin = servoMin;
  servos[index].servoMax = servoMax;
}
} // namespace Servos