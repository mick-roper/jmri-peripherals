#include "./config.h"
#include <Arduino.h>

namespace logging {
static void setup() {
#ifdef USE_SERIAL
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("Serial output initialised...");
#endif // USE_SERIAL
}

static void println(String s) {
#ifdef USE_SERIAL
  Serial.println(s);
#endif // USE_SERIAL
};

static void println(unsigned int i, int x = 10) {
#ifdef USE_SERIAL
  Serial.println(i, x);
#endif // USE_SERIAL
}

static void print(String s) {
#ifdef USE_SERIAL
  Serial.print(s);
#endif // USE_SERIAL
}

static void print(unsigned int i, int x = 10) {
#ifdef USE_SERIAL
  Serial.print(i, x);
#endif // USE_SERIAL
}
} // namespace logging
