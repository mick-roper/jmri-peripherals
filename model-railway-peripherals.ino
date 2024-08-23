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

#define SERVOMIN  150  // This is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  600  // This is the 'maximum' pulse length count (out of 4096)
#define REPORT_INTERVAL 2000  // Interval to report servo state (in milliseconds)

// Updated MAC address and MQTT server IP
byte mac[] = { 0xA8, 0x61, 0x0A, 0xAF, 0x07, 0x2C };  // Custom MAC address
const char* mqtt_server = "192.168.178.69";           // MQTT server IP address
const char* mqtt_topic_base = "turnout/";             // Base topic for turnouts

EthernetClient ethClient;
PubSubClient client(ethClient);

const int NUM_PWM_BOARDS = 1;  // Number of PCA9685 boards
Adafruit_PWMServoDriver pwms[NUM_PWM_BOARDS] = {
    Adafruit_PWMServoDriver(0x40),  // First PCA9685, I2C address 0x40
    // Adafruit_PWMServoDriver(0x41),  // Second PCA9685, I2C address 0x41
    // Adafruit_PWMServoDriver(0x42)   // Third PCA9685, I2C address 0x42
};

// Centralized function for serial logging
void logMessage(const String &message) {
    if (Serial) {
        Serial.println(message);
    }
}

// Function to send feedback to JMRI without serial logging
void sendFeedbackToJMRI(const String &turnoutId, const String &state) {
    String feedbackTopic = String(mqtt_topic_base) + turnoutId + "/feedback";
    client.publish(feedbackTopic.c_str(), state.c_str(), true);
}

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
        logMessage("Turnout " + id + " thrown");
    }

    void closeTurnout() {
        isThrown = false;
        moveServo(map(closedPosition, 0, 90, SERVOMIN, SERVOMAX));
        logMessage("Turnout " + id + " closed");
    }

    void moveServo(uint16_t pulse) {
        if (boardIndex >= 0 && boardIndex < NUM_PWM_BOARDS) {
            // Move the primary servo to the specified position
            pwms[boardIndex].setPWM(servoNumber, 0, pulse);

            // Move the corresponding servo on the same board at (servoNumber + 8) to fully open/close
            if (servoNumber + 8 < 16) {  // Ensure we stay within the 16 available channels
                if (isThrown) {
                    pwms[boardIndex].setPWM(servoNumber + 8, 0, SERVOMAX);
                    logMessage("Secondary servo " + String(servoNumber + 8) + " on board " + String(boardIndex) + " set to max");
                } else {
                    pwms[boardIndex].setPWM(servoNumber + 8, 0, SERVOMIN);
                    logMessage("Secondary servo " + String(servoNumber + 8) + " on board " + String(boardIndex) + " set to min");
                }
            }
        }
    }

    void reportState() {
        String state = isThrown ? "THROWN" : "CLOSED";
        sendFeedbackToJMRI(id, state);
    }
};

// Array of turnouts
const int NUM_TURNOUTS = 8;  // Number of turnouts
Turnout turnouts[NUM_TURNOUTS] = {
    Turnout("lower_storage_1", 0, 0, 60, 30),  // ID: "turnout1", Board 0 (0x40), Servo 0
    Turnout("lower_storage_2", 0, 1, 60, 30),  // ID: "turnout1", Board 0 (0x40), Servo 0
    Turnout("lower_storage_3", 0, 2, 60, 30),  // ID: "turnout1", Board 0 (0x40), Servo 0
    Turnout("lower_storage_4", 0, 3, 60, 30),  // ID: "turnout1", Board 0 (0x40), Servo 0
    Turnout("lower_storage_5", 0, 4, 60, 30),  // ID: "turnout1", Board 0 (0x40), Servo 0
    Turnout("lower_storage_6", 0, 5, 60, 30),  // ID: "turnout1", Board 0 (0x40), Servo 0
    Turnout("lower_storage_7", 0, 6, 60, 30),  // ID: "turnout1", Board 0 (0x40), Servo 0
    Turnout("lower_storage_8", 0, 7, 60, 30),  // ID: "turnout1", Board 0 (0x40), Servo 0
};

// Array to store the last 4 messages
String lastMessages[4] = {"", "", "", ""};

unsigned long lastReportTime = 0;

void setupOLED() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    logMessage("OLED initialization failed");
    // If OLED fails, just enter an infinite loop
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  logMessage("OLED initialized successfully");
}

void updateOLED() {
  display.clearDisplay();
  display.setCursor(0, 0);
  for (int i = 0; i < 4; i++) {
    display.println(lastMessages[i]);
  }
  display.display();
  logMessage("OLED updated with new messages");
}

void storeMessage(const String &message) {
  // Shift the messages up and store the new message at the end
  for (int i = 0; i < 3; i++) {
    lastMessages[i] = lastMessages[i + 1];
  }
  lastMessages[3] = message;
  updateOLED();  // Update OLED with new messages
  logMessage("Stored message: " + message);
}

void callback(char* topic, byte* payload, unsigned int length) {
  String topicStr = String(topic);
  String message;

  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  logMessage("Received message on topic: " + topicStr + " - Message: " + message);

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
        logMessage("Ignored message: " + message);
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
    logMessage("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ArduinoClient")) {
      logMessage("Connected");
      // Subscribe to each turnout topic
      for (int i = 0; i < NUM_TURNOUTS; i++) {
        String topic = String(mqtt_topic_base) + turnouts[i].id;
        client.subscribe(topic.c_str());
        logMessage("Subscribed to topic: " + topic);
      }
    } else {
      logMessage("Failed, rc=" + String(client.state()) + " try again in 5 seconds");
      delay(5000);  // Wait 5 seconds before retrying
    }
  }
}

void setup() {
  Serial.begin(9600);
  logMessage("Serial communication started");

  // Start Ethernet and connect to network
  Ethernet.begin(mac);
  delay(1000);  // Allow the Ethernet shield to initialize
  logMessage("Ethernet initialized");

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  for (int i = 0; i < NUM_PWM_BOARDS; i++) {
    pwms[i].begin();
    pwms[i].setPWMFreq(60);  // Analog servos run at ~60 Hz updates
    logMessage("Initialized PWM board " + String(i));
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
      turnouts[i].reportState();  // Reporting state now excludes serial logging
    }
  }
}
