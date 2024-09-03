#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "config.h"
#include "turnout.h"

#ifdef ENABLE_SERIAL
  #define logMessage(message) if (Serial) { Serial.println(message); }
#else
  #define logMessage(message)
#endif

EthernetClient ethClient;
PubSubClient client(ethClient);

unsigned long lastReportTime = 0;

// Function to set error code on LEDs
void setErrorCode(uint8_t errorCode) {
    digitalWrite(ERROR_LED_0, errorCode & 0x01);
    digitalWrite(ERROR_LED_1, (errorCode >> 1) & 0x01);
    digitalWrite(ERROR_LED_2, (errorCode >> 2) & 0x01);
    digitalWrite(ERROR_LED_3, (errorCode >> 3) & 0x01);
    logMessage("Error code set to: " + String(errorCode, BIN));
}

// Function to cycle through all 4 LEDs on startup
void cycleErrorLEDs() {
    for (int i = 0; i < 4; i++) {
        digitalWrite(ERROR_LED_0, HIGH);
        delay(200);
        digitalWrite(ERROR_LED_0, LOW);
        digitalWrite(ERROR_LED_1, HIGH);
        delay(200);
        digitalWrite(ERROR_LED_1, LOW);
        digitalWrite(ERROR_LED_2, HIGH);
        delay(200);
        digitalWrite(ERROR_LED_2, LOW);
        digitalWrite(ERROR_LED_3, HIGH);
        delay(200);
        digitalWrite(ERROR_LED_3, LOW);
    }
    logMessage("Cycled all error LEDs");
}

// Utility function to report the state of a turnout
void reportState(Turnout &turnout) {
    uint16_t currentPWM = turnout.board->getPWM(turnout.servoNumber);
    String state = (currentPWM >= SERVOMIN && currentPWM <= map(turnout.thrownPosition, 0, 90, SERVOMIN, SERVOMAX)) ? "THROWN" : "CLOSED";
    sendFeedbackToJMRI(turnout.id, state);
}

// Forward declare the callback function
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
            if (message == "THROWN") {
                turnouts[i].throwTurnout();
            } else if (message == "CLOSED") {
                turnouts[i].closeTurnout();
            } else {
                logMessage("Ignored message: " + message);
                return;
            }
            break;
        }
    }
}

void reconnect() {
    while (!client.connected()) {
        logMessage("Attempting MQTT connection...");
        if (client.connect(mqtt_client_name)) {
            logMessage("Connected");
            for (int i = 0; i < NUM_TURNOUTS; i++) {
                String topic = String(mqtt_topic_base) + turnouts[i].id;
                client.subscribe(topic.c_str());
                logMessage("Subscribed to topic: " + topic);
            }
        } else {
            logMessage("Failed, rc=" + String(client.state()) + " try again in 5 seconds");
            setErrorCode(0x04);
            delay(5000);
        }
    }
}

void sendFeedbackToJMRI(const String &turnoutId, const String &state) {
    String feedbackTopic = String(mqtt_topic_base) + turnoutId + "/feedback";
    if (!client.publish(feedbackTopic.c_str(), state.c_str(), true)) {
        setErrorCode(0x01);  // Example error handling
    }
}

void setup() {
    #ifdef ENABLE_SERIAL
    Serial.begin(9600);
    logMessage("Serial communication started");
    #endif

    pinMode(ERROR_LED_0, OUTPUT);
    pinMode(ERROR_LED_1, OUTPUT);
    pinMode(ERROR_LED_2, OUTPUT);
    pinMode(ERROR_LED_3, OUTPUT);

    cycleErrorLEDs();
    setErrorCode(0x00);

    Ethernet.begin(mac);
    delay(1000);
    logMessage("Ethernet initialized");

    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);

    for (int i = 0; i < NUM_PWM_BOARDS; i++) {
        pwms[i].begin();
        pwms[i].setPWMFreq(60);
        logMessage("Initialized PWM board " + String(i));
    }
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    unsigned long currentMillis = millis();
    if (currentMillis - lastReportTime >= REPORT_INTERVAL) {
        lastReportTime = currentMillis;
        for (int i = 0; i < NUM_TURNOUTS; i++) {
            reportState(turnouts[i]);
        }
    }
}
