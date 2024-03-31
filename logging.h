#include "./config.h"
#include <Arduino.h>

namespace logging {
static void setup() {
#ifdef USE_SERIAL
  Serial.begin(9600);
  while (!Serial)
    ;
  Serial.println("Serial output initialised...");
#endif // USE_SERIAL
}

static void println(String s){
#ifdef USE_SERIAL
  Serial.println(s);
#endif // USE_SERIAL
};
} // namespace logging
