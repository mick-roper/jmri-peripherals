#include "./analog_detection.h"
#include <Arduino.h>

#define ANALOG_DETECTION_DEVICE_COUNT 10
#define ANALOG_DETECTION_LOOP_DELAY 1000
#define ANALOG_DETECTION_TOLERANCE 0.235
#define ANALOG_DETECTION_SAMPLES 1000

namespace analog_detection {
uint64_t lastTick;

struct analog_detector {
  uint8_t exp;
  uint8_t pin;
  int avr;
};

analog_detector detectors[ANALOG_DETECTION_DEVICE_COUNT] = {
    {0x20, 0}, {0x20, 1}, {0x20, 2}, {0x20, 3}, {0x20, 4},
    {0x20, 5}, {0x20, 6}, {0x20, 7}, {0x20, 8}, {0x20, 9},
};

void setup() {
  for (uint8_t i = 0; i < ANALOG_DETECTION_DEVICE_COUNT; i++) {
    for (uint16_t s = 0; s < ANALOG_DETECTION_SAMPLES; s++) {
      detectors[i].avr += readRawValue(detectors[i].exp, detectors[i].pin);
      delayMicroseconds(50);
    }
  }

  for (uint8_t i = 0; i < ANALOG_DETECTION_SAMPLES; i++) {
    detectors[i].avr /= 1000;
  }
}

int readRawValue(uint8_t detector, uint8_t pin) {}
} // namespace analog_detection