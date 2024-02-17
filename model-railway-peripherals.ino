#include <ArduinoMqttClient.h>
#include <Ethernet.h>

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
const IPAddress ip(192, 168, 1, 177);

EthernetClient ethernet;
MqttClient mqttClient(ethernet);

const char broker[] = "HW101075";
int        port     = 1883;
const char topic[]  = "track/*";

const uint8_t minLoopDelay = 10;
unsigned long lastUpdate;

void setup() {
  Serial.begin(9600);
  while (!Serial) {};

  Serial.print("Serial output initialised..");
  Serial.println();

  Ethernet.begin(mac, ip);
  while (!ethernet.connected()) {
    Serial.print("ethernet not connected...");
    Serial.println();
    delay(100);
  }

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed...");
    Serial.println();
    while(1);
  }

  Serial.print("MQTT broker connection established!");
  Serial.println();
}

void loop() {
  if (millis() - lastUpdate >= minLoopDelay) {
    lastUpdate = millis();
    mqttClient.poll();
  }
}