#pragma once

// Rotary encoder module.
// All encoder state (counter, button flag, ...) lives as `static volatile`
// variables in encoder.cpp and is only reachable through these functions.

// Configure pins and attach the interrupts. Call once in setup().
void encoderBegin();

// Atomic snapshot: returns the current counter in `counts` and whether the
// encoder moved since the last call in `changed` (the flag is cleared).
void encoderRead(int &counts, bool &changed);

// Button: set by the ISR, cleared by the caller once handled.
bool encoderButtonPressed();
void encoderClearButton();

// Reset the counter to 0 (interrupts briefly disabled).
void encoderReset();
