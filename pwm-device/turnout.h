// turnout.h

#ifndef TURNOUT_H
#define TURNOUT_H

#include <Adafruit_PWMServoDriver.h>

class Turnout {
public:
    String id;
    Adafruit_PWMServoDriver* board;  // Pointer to the PWM board
    int servoNumber;
    int thrownPosition;
    int closedPosition;

    Turnout(String tId, Adafruit_PWMServoDriver* pwmBoard, int s, int thrownPos, int closedPos);

    void throwTurnout();
    void closeTurnout();
    void moveServo(uint16_t pulse);
};

#endif // TURNOUT_H
