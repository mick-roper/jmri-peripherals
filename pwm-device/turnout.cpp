// turnout.cpp

#include "turnout.h"
#include "config.h"

Turnout::Turnout(String tId, Adafruit_PWMServoDriver* pwmBoard, int s, int thrownPos, int closedPos)
  : id(tId), board(pwmBoard), servoNumber(s), thrownPosition(thrownPos), closedPosition(closedPos) {}

void Turnout::throwTurnout() {
    moveServo(map(thrownPosition, 0, 90, SERVOMIN, SERVOMAX));
}

void Turnout::closeTurnout() {
    moveServo(map(closedPosition, 0, 90, SERVOMIN, SERVOMAX));
}

void Turnout::moveServo(uint16_t pulse) {
    if (board) {
        board->setPWM(servoNumber, 0, pulse);
    } else {
        // Handle null board reference error
    }
}
