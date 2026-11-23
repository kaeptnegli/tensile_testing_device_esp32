#include <Arduino.h>
#include "encoder.h"
#include "pins.h"

// Module-private state: `static` = only visible in this file,
// `volatile` = written by an ISR, so always read from memory.
static volatile int      rotaryCounts   = 0;
static volatile bool     buttonPressed  = false;
static volatile uint32_t lastButtonMs   = 0;
static volatile uint8_t  encState       = 0;    // letzter Zustand (CLK<<1 | DT)
static volatile int8_t   encSteps       = 0;    // Teilschritte bis zur naechsten Raste
static volatile bool     encoderChanged = true; // nur die ISR setzt das

// Zustandstabelle, Index = (alt << 2) | neu
// 00 -> 10 -> 11 -> 01 -> 00 (rechts herum)
// 00 -> 01 -> 11 -> 10 -> 00 (links herum)
// CLK MSB - DT LSB
static const int8_t ENC_TABLE[16] = {0, -1, 1, 0,  1, 0, 0, -1,  -1, 0, 0, 1,  0, 1, -1, 0};

// Interrupts
// Auswertung mit Zustandstabelle, Prellen hebt sich selbst auf
static void IRAM_ATTR readEncoderRotationsISR() {
  uint8_t now = (digitalRead(CLK_PIN) << 1) | digitalRead(DT_PIN);
  encSteps += ENC_TABLE[(encState << 2) | now];
  encState = now;
  if (encSteps >= 4) {
    rotaryCounts--;
    encSteps = 0;
    encoderChanged = true;
  }
  if (encSteps <= -4) {
    rotaryCounts++;
    encSteps = 0;
    encoderChanged = true;
  }
}

static void IRAM_ATTR readEncoderButtonISR() {
  uint32_t now = millis();
  if (now - lastButtonMs < 50) return;          // Prellen -> ignorieren
  if (digitalRead(SW_PIN) != LOW) return;       // Flanke vom Loslassen -> ignorieren
  lastButtonMs = now;
  buttonPressed = true;
}

// Public functions

void encoderBegin() {
  pinMode(CLK_PIN, INPUT_PULLUP);
  pinMode(DT_PIN, INPUT_PULLUP);
  pinMode(SW_PIN, INPUT_PULLUP);
  encState = (digitalRead(CLK_PIN) << 1) | digitalRead(DT_PIN);   // Startzustand
  attachInterrupt(digitalPinToInterrupt(CLK_PIN), readEncoderRotationsISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(DT_PIN),  readEncoderRotationsISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SW_PIN),  readEncoderButtonISR, FALLING);
}

void encoderRead(int &counts, bool &changed) {
  noInterrupts();
  counts = rotaryCounts;
  changed = encoderChanged;
  encoderChanged = false;
  interrupts();
}

bool encoderButtonPressed() {
  return buttonPressed;
}

void encoderClearButton() {
  buttonPressed = false;
}

void encoderReset() {
  noInterrupts();
  rotaryCounts = 0;
  encSteps = 0;
  interrupts();
}
