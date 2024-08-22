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
#define REPORT_INTERVAL 2000  // Interval to report servo state (in milliseconds)

// Update these with values suitable for your network
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  // MAC address of the Ethernet shield
const char* mqtt_server = "mqtt.example.com";         // Named MQTT server
const char* mqtt_topic_base = "turnout/";             // Base topic for turnouts

EthernetClient ethClient;
PubSubClient client(ethClient);

// Define PCA9685 instances with their unique I2C addresses
const int NUM_PWM_BOARDS = 3;  // Number of PCA9685 boards
Adafruit_PWMServoDriver pwms[NUM_PWM_BOARDS] = {
    Adafruit_PWMServoDriver(0x40),  // First PCA9685, I2C address 0x40
    Adafruit_PWMServoDriver(0x41),  // Second PCA9685, I2C address 0x41
    Adafruit_PWMServoDriver(0x42)   // Third PCA9685, I2C address 0x42
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
        moveServo(map(thrownPosition, 0, 90, SERVOMIN, SERVOMAX));
    }

    void closeTurnout() {
        isThrown = false;
        moveServo(map(closedPosition, 0, 90, SERVOMIN, SERVOMAX));
    }

    void moveServo(uint16_t pulse) {
        if (boardIndex >= 0 && boardIndex < NUM_PWM_BOARDS) {
            // Move the primary servo to the specified position
            pwms[boardIndex].setPWM(servoNumber, 0, pulse);

            // Move the corresponding servo on the same board at (servoNumber + 8) to fully open/close
            if (servoNumber + 8 < 16) {  // Ensure we stay within the 16 available channels
                if (isThrown) {
                    pwms[boardIndex].setPWM(servoNumber + 8, 0, SERVOMAX);
                } else {
                    pwms[boardIndex].setPWM(servoNumber + 8, 0, SERVOMIN);
                }
            }
        }
    }

    String getState() {
        return isThrown ? "THROWN" : "CLOSED";
    }

    void reportState() {
        sendFeedbackToJMRI(id, getState());
    }
};

// Array of turnouts
const int NUM_TURNOUTS = 3;  // Number of turnouts
Turnout turnouts[NUM_TURNOUTS] = {
    Turnout("turnout1", 0, 0, 90, 0),  // ID: "turnout1", Board 0 (0x40), Servo 0
    Turnout("turnout2", 0, 1, 90, 0),  // ID: "turnout2", Board 0 (0x40), Servo 1
    Turnout("turnout3", 1, 0, 90, 0)   // ID: "turnout3", Board 1 (0x41), Servo 0
};

// Array to store the last 4 messages
String lastMessages[4] = {"", "", "", ""};

unsigned long lastReportTime = 0;

void setupOLED() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    // If OLED fails, just enter an infinite loop
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
}

void updateOLED() {
  display.clearDisplay();
  display.setCursor(0, 0);
  for (int i = 0; i < 4; i++) {
    display.println(lastMessages[i]);
  }
  display.display();
}

void sendFeedbackToJMRI(const String &turnoutId, const String &state) {
  String feedbackTopic = String(mqtt_topic_base) + turnoutId + "/feedback";
  client.publish(feedbackTopic.c_str(), state.c_str(), true);
}

void storeMessage(const String &message) {
  // Shift the messages up and store the new message at the end
  for (int i = 0; i < 3; i++) {
    lastMessages[i] = lastMessages[i + 1];
  }
  lastMessages[3] = message;
  updateOLED();  // Update OLED with new messages
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
      // Handle only "THROWN" and "CLOSED" messages
      if (message == "THROWN") {
        turnouts[i].throwTurnout();
      } else if (message == "CLOSED") {
        turnouts[i].closeTurnout();
      } else {
        return;  // Ignore other messages
      }

      // Store the message to be displayed on the OLED
      storeMessage("Turnout " + turnoutId + ": " + message);

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
  Ethernet.begin(mac);
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

  // Periodically report the state of each turnout every 2 seconds
  unsigned long currentMillis = millis();
  if (currentMillis - lastReportTime >= REPORT_INTERVAL) {
    lastReportTime = currentMillis;
    for (int i = 0; i < NUM_TURNOUTS; i++) {
      turnouts[i].reportState();
    }
  }
}
