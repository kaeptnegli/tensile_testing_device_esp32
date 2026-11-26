#include <Arduino.h>
#include "limitswitch.h"
#include "pins.h"

// Switch between SIG_PIN and GND, internal pull-up.
// NO switch (normally open):   pressed = LOW
// NC switch (normally closed): pressed = HIGH (a broken wire then also reads as pressed)
static constexpr int PRESSED_LEVEL = LOW;

void limitSwitchBegin() {
  pinMode(SIG_PIN, INPUT_PULLUP);
}

bool limitSwitchPressed() {
  return digitalRead(SIG_PIN) == PRESSED_LEVEL;
}
