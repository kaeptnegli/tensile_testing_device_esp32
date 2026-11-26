#ifndef PM3_MOTION_H
#define PM3_MOTION_H

// Stepper motor module. The AccelStepper object lives privately in motion.cpp.

// Configure the driver. Call once in setup().
void motionBegin(float speed, float acceleration);

// Blocking: drive to the limit switch, back off, set home = 0.
// Needs limitSwitchBegin() first. Returns false if the switch was not
// found within the maximum homing distance.
bool motionHome();

// true after a successful motionHome().
bool motionIsHomed();

// Move to a height above home (in counter units). Ignored until homed.
void motionMoveToHeight(long relPosition);

// Non-blocking: call every loop() cycle while the motor should move.
// Stops the motor if it would drive further into the limit switch.
void motionRun();

#endif // PM3_MOTION_H
