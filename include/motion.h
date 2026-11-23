#pragma once

// Stepper motor module. The AccelStepper object lives privately in motion.cpp.

// Configure the driver. Call once in setup().
void motionBegin();

// Non-blocking: call every loop() cycle while the motor should move.
void motionRun();
