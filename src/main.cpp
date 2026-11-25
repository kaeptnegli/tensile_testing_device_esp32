#include <Arduino.h>  // C6: in PlatformIO-.cpp noetig
#include <Wire.h>     // C6: fuer Wire.begin(23, 22)
#include "pins.h"
#include "machine.h"
#include "encoder.h"
#include "menu.h"
#include "motion.h"
#include "loadcell.h"

// Vocab:
// ISR: Interruption Service Routine
// IRAM_ATTR: store ISR function is fast memory


// TODO:
// FIX drawMenu if else structure
// Fix insert Probe State if else structure
// Fix Rotary Encode button bounce


// Program state
// ------------
// Owned by main.cpp (static = invisible to other files) and passed
// explicitly to the modules that need it.
static States      machineState = States::MENU;
static MenuState   ui;
static MachineData data;

//-----------------------------
//- Functions and Subroutines -
//-----------------------------

void testFinished(MachineData &d) {
  d.testComplete = true;
}

/// Setup and Super loop

void setup() {
  Serial.begin(115200);

  loadCellBegin();

  Wire.begin(I2C_SDA, I2C_SCL, 400000);  // C6: I2C-Pins explizit setzen
  menuBegin();

  encoderBegin();

  motionBegin();
}

void loop() {
  int counts;
  bool redraw;
  encoderRead(counts, redraw);

  int modulusMenuCounter = ((counts % 4) + 4) % 4;
  int modulusMenuCounterTwo = ((counts % 2) + 2) % 2;
  switch (machineState) {
    case States::MENU: {
        int menuItem = modulusMenuCounter;
        if (encoderButtonPressed()) {
          switch (menuItem) {
            case 0: {
                machineState = States::TEST;
                ui.activeSubMenu = menuItem;
                if (!data.probeReady) {
                  ui.menuDepth = 1;
                } else {
                  ui.menuDepth = 2;
                }
                break;
              }
            case 1: {
                machineState = States::INSERT_PROBE;
                ui.activeSubMenu = menuItem; // 1
                ui.menuDepth++;
                break;
              }
            case 2: {
                machineState = States::YOUNGS_MODULUS;
                ui.activeSubMenu = menuItem;
                ui.menuDepth++;
                break;
              }
            case 3: {
                machineState = States::INFO;
                ui.activeSubMenu = menuItem;
                ui.menuDepth++;
                break;
              }
          }
          ui.displayWrite = true;
          encoderClearButton();
        }
        break;
      }
    case States::TEST: {
        if ((!data.probeReady && encoderButtonPressed() && ui.menuDepth == 1) || ui.menuDepth == 3) {
          if (ui.menuDepth == 3) {
            delay(3000);
          }
          machineState = States::MENU;
          ui.activeSubMenu = -1;
          ui.menuDepth = 0;
          encoderReset();
          encoderClearButton();
          ui.displayWrite = true;
        } else if (ui.menuDepth == 2) {
          delay(3000);
          ui.menuDepth = 3;
          ui.displayWrite = true;
        }
        break;
      }
    case States::INSERT_PROBE: {
        if (redraw && ui.menuDepth == 1) {
          if (data.rodHeight + counts < 0) {
            data.rodHeight = 0;
          } else {
            data.rodHeight += counts;
          }
          ui.displayWrite = true;
          encoderReset();
        }

        if (encoderButtonPressed() && ui.menuDepth < 3) {
          ui.menuDepth++;
          ui.displayWrite = true;
        } else if (encoderButtonPressed() && ui.menuDepth == 3) {
          Serial.println("case");
          if (modulusMenuCounterTwo == 0) {
            data.probeReady = true;
          } else {
            data.probeReady = false;
          }
          machineState = States::MENU;
          ui.activeSubMenu = -1;
          ui.menuDepth = 0;
          ui.displayWrite = true;
        }

        encoderClearButton();
        break;
      }
    case States::YOUNGS_MODULUS: {
        delay(6000);
        machineState = States::MENU;
        ui.activeSubMenu = -1;
        ui.menuDepth = 0;
        encoderReset();
        encoderClearButton();
        ui.displayWrite = true;
        break;
      }

    case States::INFO: {
        delay(6000);
        machineState = States::MENU;
        ui.activeSubMenu = -1;
        ui.menuDepth = 0;
        encoderReset();
        encoderClearButton();
        ui.displayWrite = true;
        break;
      }
    case States::HOMING: {

        break;
      }
  }
  // things need to be done every cycle after state machine
  drawMenu(ui, data, modulusMenuCounter, modulusMenuCounterTwo, redraw);
}
