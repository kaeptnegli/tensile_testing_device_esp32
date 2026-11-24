#ifndef PM3_MOTION_H
#define PM3_MOTION_H

// Stepper motor module. The AccelStepper object lives privately in motion.cpp.

// Configure the driver. Call once in setup().
void motionBegin();

// Non-blocking: call every loop() cycle while the motor should move.
void motionRun();

#endif // PM3_MOTION_H
