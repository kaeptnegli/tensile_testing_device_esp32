#include <Arduino.h>
#include <AccelStepper.h>
#include "motion.h"
#include "pins.h"

static AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

void motionBegin() {
  //stepper.setEnablePin(EN_PIN);
  //stepper.setPinsInverted(false, false, true); // EN ist active-low
  //stepper.enableOutputs();

  //stepper.setMaxSpeed(100);     // Schritte/s
  //stepper.setAcceleration(500);  // Schritte/s²
  //stepper.moveTo(2000);          // Ziel: Schritt 2000
}

void motionRun() {
  stepper.run();  // nicht-blockierend, immer aufrufen

  // z.B. in mm umrechnen: pos / schritteProMm
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 200) {
    lastPrint = millis();
    Serial.println(stepper.currentPosition());
  }
}
