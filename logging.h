#include "./config.h"
#include <Arduino.h>

#ifdef USE_OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif

namespace logging {
static void setup() {
#ifdef USE_SERIAL
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("Serial output initialised...");
#endif // USE_SERIAL

#ifdef USE_OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;)
      ;
  }

  display.clearDisplay();
  display.setTextSize(0);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("ready!");
  display.display();
  delay(2000);
#endif
}

static void println(String s) {
#ifdef USE_SERIAL
  Serial.println(s);
#endif // USE_SERIAL

#ifdef USE_OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(s);
  display.display();
  delay(1000);
#endif
};

static void println(unsigned int i, int x = 10) {
#ifdef USE_SERIAL
  Serial.println(i, x);
#endif // USE_SERIAL

#ifdef USE_OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(i, x);
  display.display();
  delay(1000);
#endif
}

static void print(String s) {
#ifdef USE_SERIAL
  Serial.print(s);
#endif // USE_SERIAL

#ifdef USE_OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(s);
  display.display();
  delay(1000);
#endif
}

static void print(unsigned int i, int x = 10) {
#ifdef USE_SERIAL
  Serial.print(i, x);
#endif // USE_SERIAL

#ifdef USE_OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(i, x);
  display.display();
  delay(1000);
#endif
}
} // namespace logging
