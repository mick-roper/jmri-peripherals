#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Adafruit_ADS1X15.h>

// Static MAC address array
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // Set your MAC address here

// MQTT server address
const char* mqttServer = "192.168.1.100"; // Replace with your MQTT broker's IP address

// Initialize Ethernet client
EthernetClient ethClient;
PubSubClient client(ethClient);

// Initialize ADS1115 modules
Adafruit_ADS1115 ads1;
Adafruit_ADS1115 ads2;
Adafruit_ADS1115 ads3;
Adafruit_ADS1115 ads4;

// Error LEDs - assign pins 2 to 5
const int errorLEDs[4] = {2, 3, 4, 5}; // LEDs for error indication

// Error states
byte errorState = 0b0000; // 4-bit error state, 0000 means no errors

/*
  Error State Descriptions:
  -------------------------
  errorState is a 4-bit variable where each bit represents a specific error.
  The corresponding LED will light up to indicate an error.

  Bit 3 (0b1000): Ethernet connection error (LED on pin 2)
  Bit 2 (0b0100): MQTT connection error (LED on pin 3)
  Bit 1 (0b0010): ADS1115 initialization error (LED on pin 4)
  Bit 0 (0b0001): ADS1115 reading error (LED on pin 5)
  
  Example:
  - 0000: No errors (all LEDs off)
  - 0001: ADS1115 reading error (LED on pin 5 is on)
  - 0010: ADS1115 initialization error (LED on pin 4 is on)
  - 0100: MQTT connection error (LED on pin 3 is on)
  - 1000: Ethernet connection error (LED on pin 2 is on)
  - 1111: Multiple errors (all LEDs on)
*/

// MQTT topic base
const char* topicBase = "sensor";

// Threshold value for determining ACTIVE or INACTIVE
const int16_t threshold = 1000;

// Time tracking for 50ms interval
unsigned long lastPublishTime = 0;
const unsigned long publishInterval = 50; // 50 ms

// Track current ADC to read
uint8_t currentADC = 0;
uint8_t currentChannel = 0;

// Explicit names for each ADC channel
const char* channelNames[4][4] = {
  {"sensor1_ch1", "sensor1_ch2", "sensor1_ch3", "sensor1_ch4"},
  {"sensor2_ch1", "sensor2_ch2", "sensor2_ch3", "sensor2_ch4"},
  {"sensor3_ch1", "sensor3_ch2", "sensor3_ch3", "sensor3_ch4"},
  {"sensor4_ch1", "sensor4_ch2", "sensor4_ch3", "sensor4_ch4"}
};

void setup() {
  // Set up LEDs as output
  for (int i = 0; i < 4; i++) {
    pinMode(errorLEDs[i], OUTPUT);
  }

  // Illuminate LEDs in order during setup
  for (int i = 0; i < 4; i++) {
    digitalWrite(errorLEDs[i], HIGH);  // Turn on LED
    delay(250);                        // Wait for 250ms
    digitalWrite(errorLEDs[i], LOW);   // Turn off LED
  }

  // Start the Ethernet connection using DHCP with the static MAC address
  if (Ethernet.begin(mac) == 0) {
    errorState |= 0b1000; // Set bit 3 (Ethernet connection error)
    updateErrorLEDs();
    while (true) {
      // Blink LED or reset if needed
    }
  }

  delay(1000);

  // Set MQTT server
  client.setServer(mqttServer, 1883);

  // Initialize ADS1115 modules
  if (!initializeADS1115()) {
    errorState |= 0b0010; // Set bit 1 (ADS1115 initialization error)
    updateErrorLEDs();
  }
}

void loop() {
  // Reconnect to MQTT if disconnected
  if (!client.connected()) {
    errorState |= 0b0100; // Set bit 2 (MQTT connection error)
    updateErrorLEDs();
    reconnect();
  } else {
    errorState &= ~0b0100; // Clear bit 2 if MQTT connected
    updateErrorLEDs();
  }
  client.loop();

  // Check if it's time to publish (every 50ms)
  unsigned long currentMillis = millis();
  if (currentMillis - lastPublishTime >= publishInterval) {
    lastPublishTime = currentMillis;

    // Read the current ADC input and publish
    if (!readAndPublishCurrentADC()) {
      errorState |= 0b0001; // Set bit 0 (ADS1115 reading error)
    } else {
      errorState &= ~0b0001; // Clear bit 0 if reading is successful
    }
    updateErrorLEDs();

    // Move to the next ADC input
    advanceToNextADC();
  }
}

// Function to read the current ADC input and publish it
bool readAndPublishCurrentADC() {
  int16_t adcValue;
  const char* channelName = channelNames[currentADC][currentChannel];

  switch (currentADC) {
    case 0:
      adcValue = ads1.readADC_SingleEnded(currentChannel);
      break;
    case 1:
      adcValue = ads2.readADC_SingleEnded(currentChannel);
      break;
    case 2:
      adcValue = ads3.readADC_SingleEnded(currentChannel);
      break;
    case 3:
      adcValue = ads4.readADC_SingleEnded(currentChannel);
      break;
  }

  if (adcValue == -1) {
    return false; // Return false if there was an error reading the ADC
  }

  char topic[50];
  snprintf(topic, sizeof(topic), "%s/%s/feedback", topicBase, channelName);

  const char* payload;
  if (adcValue > threshold) {
    payload = "ACTIVE";
  } else {
    payload = "INACTIVE";
  }

  client.publish(topic, payload);
  return true;
}

// Function to initialize ADS1115 modules
bool initializeADS1115() {
  bool success = true;
  if (!ads1.begin(0x48)) success = false;
  if (!ads2.begin(0x49)) success = false;
  if (!ads3.begin(0x4A)) success = false;
  if (!ads4.begin(0x4B)) success = false;
  return success;
}

// Function to move to the next ADC input
void advanceToNextADC() {
  currentChannel++;
  if (currentChannel >= 4) {
    currentChannel = 0;
    currentADC++;
    if (currentADC >= 4) {
      currentADC = 0;
    }
  }
}

// Function to update the error LEDs based on the error state
void updateErrorLEDs() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(errorLEDs[i], (errorState >> (3 - i)) & 0b0001);
  }
}

// Function to reconnect to MQTT
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    // Attempt to connect
    if (client.connect("ArduinoClient")) {
      errorState &= ~0b0100; // Clear bit 2 (MQTT connection error)
      updateErrorLEDs();
    } else {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
