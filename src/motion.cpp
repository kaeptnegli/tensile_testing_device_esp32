#include <Arduino.h>
#include <AccelStepper.h>
#include "motion.h"
#include "pins.h"
#include "limitswitch.h"

static AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// Drive: change these if the motor, microstepping or spindle changes
static constexpr float STEPS_PER_TURN = 800.0f;  // placeholder: half a turn per counter (200 * 1/8 / 2)
//static constexpr float MICROSTEPS     = 8.0f;    // TMC2209, MS1 + MS2 open
//static constexpr float LEAD_MM        = 2.0f;    // TODO: your spindle lead (mm per turn)
//static constexpr float STEPS_PER_MM   = STEPS_PER_TURN * MICROSTEPS / LEAD_MM;  // 800

// Homing
static constexpr long  HOMING_DIR       = -1;     // direction towards the switch: CHECK before first run!
static constexpr long  MAX_HOMING_STEPS = 16000;  // safety: give up after 10 turns (1600 steps/turn)
static constexpr long  BACKOFF_STEPS    = 200;    // move off the switch after it triggered
static constexpr float HOMING_SPEED     = 400.0f; // steps/s, slow

static float normalSpeed = 0;   // max speed from motionBegin(), restored after homing
static bool  homed       = false;

void motionBegin(float speed, float acceleration) {
  stepper.setEnablePin(EN_PIN);
  stepper.setPinsInverted(false, false, true); // EN ist active-low
  stepper.enableOutputs();

  normalSpeed = speed;
  stepper.setMaxSpeed(speed);             // Schritte/s
  stepper.setAcceleration(acceleration);  // Schritte/s²
}

bool motionHome() {
  homed = false;
  stepper.setMaxSpeed(HOMING_SPEED);
  stepper.move(HOMING_DIR * MAX_HOMING_STEPS);

  // drive towards the switch until it triggers
  while (!limitSwitchPressed()) {
    if (!stepper.run()) {               // max distance travelled, no switch
      stepper.setCurrentPosition(stepper.currentPosition());  // stop
      stepper.setMaxSpeed(normalSpeed);
      return false;
    }
  }
  stepper.setCurrentPosition(0);        // hard stop, switch position = 0

  // free the switch again; this point is home
  stepper.runToNewPosition(-HOMING_DIR * BACKOFF_STEPS);
  stepper.setCurrentPosition(0);

  stepper.setMaxSpeed(normalSpeed);
  homed = true;
  return true;
}

bool motionIsHomed() {
  return homed;
}

void motionMoveToHeight(long relPosition) {
  if (!homed) {
    return;   // without a reference, positions are meaningless
  }
  stepper.moveTo(lroundf(relPosition * STEPS_PER_TURN));
}

void motionRun() {
  // never drive further into the switch; driving away is allowed
  if (limitSwitchPressed() && stepper.distanceToGo() * HOMING_DIR > 0) {
    stepper.setCurrentPosition(stepper.currentPosition());  // hard stop
  }
  stepper.run();  // nicht-blockierend, immer aufrufen

  // z.B. in mm umrechnen: pos / schritteProMm
  //static unsigned long lastPrint = 0;
  //if (millis() - lastPrint > 200) {
  //  lastPrint = millis();
  //  Serial.println(stepper.currentPosition());
  //}
}
