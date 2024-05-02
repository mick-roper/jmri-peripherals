#ifndef ANALOG_DETECTION_H
#define ANALOG_DETECTION_H

#define ANALOG_DETECTOR_CT_RATIO 1000
#define ANALOG_DETECTOR_SHUNT_RES 40
#define ANALOG_DETECTOR_REF_VOLTAGE 1024

#define PINS_PER_EXP 16
#define EXPANDERS 2

#define ENABLE_PIN_0 10
#define READER_PIN A0
#define S0_PIN 4
#define S1_PIN 5
#define S2_PIN 6
#define S3_PIN 7

namespace analaog_detection {
float values[PINS_PER_EXP * EXPANDERS];
uint8_t expander;
const uint16_t samples = 256;
uint16_t r_array[samples];

void setup() {
  pinMode(S0_PIN, OUTPUT);
  pinMode(S1_PIN, OUTPUT);
  pinMode(S2_PIN, OUTPUT);
  pinMode(S3_PIN, OUTPUT);

  for (uint8_t exp = 0; exp < EXPANDERS; exp++) {
    digitalWrite(exp + ENABLE_PIN_0, HIGH);
  }
}

void loop() {
  for (uint8_t exp = 0; exp < EXPANDERS; exp++) {
    digitalWrite(exp + ENABLE_PIN_0, LOW);

    for (uint8_t pin = 0; pin < PINS_PER_EXP; pin++) {
      switch (pin) {
      case 0: {
        digitalWrite(S0_PIN, LOW); // 1
        digitalWrite(S1_PIN, LOW); // 2
        digitalWrite(S2_PIN, LOW); // 4
        digitalWrite(S3_PIN, LOW); // 8
        break;
      }
      case 1: {
        digitalWrite(S0_PIN, HIGH);
        digitalWrite(S1_PIN, LOW);
        digitalWrite(S2_PIN, LOW);
        digitalWrite(S3_PIN, LOW);
        break;
      }
      case 2: {
        digitalWrite(S0_PIN, LOW);
        digitalWrite(S1_PIN, HIGH);
        digitalWrite(S2_PIN, LOW);
        digitalWrite(S3_PIN, LOW);
        break;
      }
      case 3: {
        digitalWrite(S0_PIN, HIGH);
        digitalWrite(S1_PIN, HIGH);
        digitalWrite(S2_PIN, LOW);
        digitalWrite(S3_PIN, LOW);
        break;
      }
      case 4: {
        digitalWrite(S0_PIN, LOW);
        digitalWrite(S1_PIN, LOW);
        digitalWrite(S2_PIN, HIGH);
        digitalWrite(S3_PIN, LOW);
        break;
      }
      case 5: {
        digitalWrite(S0_PIN, HIGH);
        digitalWrite(S1_PIN, LOW);
        digitalWrite(S2_PIN, HIGH);
        digitalWrite(S3_PIN, LOW);
        break;
      }
      case 6: {
        digitalWrite(S0_PIN, LOW);
        digitalWrite(S1_PIN, HIGH);
        digitalWrite(S2_PIN, HIGH);
        digitalWrite(S3_PIN, LOW);
        break;
      }
      case 7: {
        digitalWrite(S0_PIN, HIGH);
        digitalWrite(S1_PIN, HIGH);
        digitalWrite(S2_PIN, HIGH);
        digitalWrite(S3_PIN, LOW);
        break;
      }
      case 8: {
        digitalWrite(S0_PIN, LOW);
        digitalWrite(S1_PIN, LOW);
        digitalWrite(S2_PIN, LOW);
        digitalWrite(S3_PIN, HIGH);
        break;
      }
      case 9: {
        digitalWrite(S0_PIN, HIGH);
        digitalWrite(S1_PIN, LOW);
        digitalWrite(S2_PIN, LOW);
        digitalWrite(S3_PIN, HIGH);
        break;
      }
      case 10: {
        digitalWrite(S0_PIN, LOW);
        digitalWrite(S1_PIN, HIGH);
        digitalWrite(S2_PIN, LOW);
        digitalWrite(S3_PIN, HIGH);
        break;
      }
      case 11: {
        digitalWrite(S0_PIN, HIGH);
        digitalWrite(S1_PIN, HIGH);
        digitalWrite(S2_PIN, LOW);
        digitalWrite(S3_PIN, HIGH);
        break;
      }
      case 12: {
        digitalWrite(S0_PIN, LOW);
        digitalWrite(S1_PIN, LOW);
        digitalWrite(S2_PIN, HIGH);
        digitalWrite(S3_PIN, HIGH);
        break;
      }
      case 13: {
        digitalWrite(S0_PIN, HIGH);
        digitalWrite(S1_PIN, LOW);
        digitalWrite(S2_PIN, HIGH);
        digitalWrite(S3_PIN, HIGH);
        break;
      }
      case 14: {
        digitalWrite(S0_PIN, LOW);
        digitalWrite(S1_PIN, HIGH);
        digitalWrite(S2_PIN, HIGH);
        digitalWrite(S3_PIN, HIGH);
        break;
      }
      case 15: {
        digitalWrite(S0_PIN, HIGH);
        digitalWrite(S1_PIN, HIGH);
        digitalWrite(S2_PIN, HIGH);
        digitalWrite(S3_PIN, HIGH);
        break;
      }
      }

      values[(exp * pin) + pin] = read_rms();
    }

    digitalWrite(exp + ENABLE_PIN_0, HIGH);
  }
}

float read_rms() {
  uint16_t i;
  uint8_t j;
  float dc_offset, rms = 0;

  for (uint8_t i = 0; i < samples; i++) {
    r_array[i] = 0;
  }

  // read voltage at INPUT_CHANNEL 'n' times and save data to 'r_array'
  for (i = 0; i < samples; i++) {
    // adding another 2 bits using oversampling technique
    for (j = 0; j < 16; j++) {
      r_array[i] += analogRead(READER_PIN);
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
} // namespace analaog_detection

#endif