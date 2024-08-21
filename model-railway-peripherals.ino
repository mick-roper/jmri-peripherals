#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Define the min and max pulse lengths
#define SERVOMIN  150  // This is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  600  // This is the 'maximum' pulse length count (out of 4096)

// Update these with values suitable for your network
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  // MAC address of the Ethernet shield
IPAddress ip(192, 168, 1, 177);                       // IP address for the Arduino
IPAddress mqtt_server(192, 168, 1, 2);                // IP address of the MQTT broker
const char* mqtt_topic_base = "turnout/";             // Base topic for turnouts

EthernetClient ethClient;
PubSubClient client(ethClient);

// Define PCA9685 instances with their unique I2C addresses
const int NUM_PWM_BOARDS = 1;  // Number of PCA9685 boards
Adafruit_PWMServoDriver pwms[NUM_PWM_BOARDS] = {
    Adafruit_PWMServoDriver(0x40),
};

// Turnout class definition
class Turnout {
public:
    String id;
    int boardIndex;
    int servoNumber;
    int thrownPosition;
    int closedPosition;
    bool isThrown;

    Turnout(String tId, int bIdx, int s, int thrownPos, int closedPos) {
        id = tId;
        boardIndex = bIdx;
        servoNumber = s;
        thrownPosition = thrownPos;
        closedPosition = closedPos;
        isThrown = false;
    }

    void throwTurnout() {
        isThrown = true;
        moveServo();
    }

    void closeTurnout() {
        isThrown = false;
        moveServo();
    }

    void toggleTurnout() {
        isThrown = !isThrown;
        moveServo();
    }

    void moveServo() {
        uint16_t pulse = isThrown ? map(thrownPosition, 0, 90, SERVOMIN, SERVOMAX) : map(closedPosition, 0, 90, SERVOMIN, SERVOMAX);
        if (boardIndex >= 0 && boardIndex < NUM_PWM_BOARDS) {
            pwms[boardIndex].setPWM(servoNumber, 0, pulse);
        }
    }

    String getState() {
        return isThrown ? "THROWN" : "CLOSED";
    }
};

// Array of turnouts
const int NUM_TURNOUTS = 8;  // Number of turnouts
Turnout turnouts[NUM_TURNOUTS] = {
    Turnout("up_fdl_arr_1", 0, 0, 90, 0),
    Turnout("up_fdl_arr_2", 0, 1, 90, 0),
    Turnout("up_fdl_dep_1", 0, 2, 90, 0),
    Turnout("up_fdl_dep_2", 0, 3, 90, 0),
    Turnout("dn_fdl_arr_1", 0, 4, 90, 0),
    Turnout("dn_fdl_arr_2", 0, 5, 90, 0),
    Turnout("dn_fdl_dep_1", 0, 6, 90, 0),
    Turnout("dn_fdl_dep_2", 0, 7, 90, 0),
};

void setupOLED() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    // If OLED fails, just enter an infinite loop
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("MQTT Turnout Control");
  display.display();
  delay(2000);
  display.clearDisplay();
}

void updateOLED(String message) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(message);
  display.display();
}

void sendFeedbackToJMRI(const String &turnoutId, const String &state) {
  String feedbackTopic = String(mqtt_topic_base) + turnoutId + "/feedback";
  client.publish(feedbackTopic.c_str(), state.c_str(), true);
}

void callback(char* topic, byte* payload, unsigned int length) {
  String topicStr = String(topic);
  String message;

  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  // Extract TURNOUT_ID from the topic
  String turnoutId = topicStr.substring(strlen(mqtt_topic_base));

  // Find the corresponding turnout
  for (int i = 0; i < NUM_TURNOUTS; i++) {
    if (turnouts[i].id == turnoutId) {
      // Execute the command on the found turnout
      if (message == "THROW") turnouts[i].throwTurnout();
      else if (message == "CLOSE") turnouts[i].closeTurnout();
      else if (message == "TOGGLE") turnouts[i].toggleTurnout();

      // Update OLED with the received command
      updateOLED("Turnout " + turnoutId + ": " + message);

      // Send feedback to JMRI with the new state
      sendFeedbackToJMRI(turnoutId, turnouts[i].getState());

      break;
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    // Attempt to connect
    if (client.connect("ArduinoClient")) {
      // Subscribe to each turnout topic
      for (int i = 0; i < NUM_TURNOUTS; i++) {
        String topic = String(mqtt_topic_base) + turnouts[i].id;
        client.subscribe(topic.c_str());
      }
    } else {
      delay(5000);  // Wait 5 seconds before retrying
    }
  }
}

void setup() {
  // Start Ethernet and connect to network
  Ethernet.begin(mac, ip);
  delay(1000);  // Allow the Ethernet shield to initialize

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  for (int i = 0; i < NUM_PWM_BOARDS; i++) {
    pwms[i].begin();
    pwms[i].setPWMFreq(60);  // Analog servos run at ~60 Hz updates
  }

  setupOLED();  // Initialize the OLED display
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
