#include <vector>
#include "HX711.h"
#include <LiquidCrystal_I2C.h>
#include <AccelStepper.h>

// PREPROCESSOR
// ------------
// LCD DISPLAy
#define I2C_ADDR    0x27
#define LCD_COLUMNS 20
#define LCD_LINES   4
// Rotary Encoder
#define CLK_PIN     27 // GPIO27, matches the wiring above
#define DT_PIN      26  // GPIO26
#define SW_PIN      25  // GPIO25
// Load Cell
#define DOUT_PIN    16
#define SCK_PIN     4
//Stepper
#define STEP_PIN    32
#define EN_PIN      13
#define DIR_PIN     33
//Misc
#define M_pif 3.14159265358979323846f


// Global Variables
// ------------
// User Interface - Rotary Encoder
volatile int rotaryCounts = 0;
volatile bool buttonPressed = false;
volatile uint32_t lastButtonMs = 0;
volatile uint8_t encState = 0;   // NEU: letzter Zustand (CLK<<1 | DT)
volatile int8_t encSteps = 0;    // NEU: Teilschritte bis zur nächsten Raste

// NEU: Zustandstabelle, Index = (alt << 2) | neu
const int8_t ENC_TABLE[16] = {0,-1, 1, 0,  1, 0, 0,-1,  -1, 0, 0, 1,  0, 1,-1, 0};

//User Interface - LCD Display Menu Items
int menuDepth = 0;
int activeSubMenu = -1; // 1 -> test, 2 -> insert, 3 -> youngs, 4 -> status -1 -> Main menu
volatile bool encoderChanged = true;   // nur die ISR setzt das
bool displayWrite = true;              // nur loop()/States setzen das, kein volatile nötig
using UserInterface = std::vector<std::vector<std::vector<String>>>;
const UserInterface menuItems = {
  {{"Start Test"}, {"Insert Probe First,", "and Confirm!", "", ""}}, // 0 -> main menu
  {{"Insert Probe"}, {"Use Wheel to", "adjust height", "", ""}},
  {{"Youngs Modulus"}, {"Youngs Modulus:", "270 GPa", "", ""}},
  {{"INFO"}, {"Status:", "Finished", "Error:", "None"}}   // NEU: einfache Klammern
};

// Probe
bool probeReady = false;

// State Machine States
enum States {
  MENU,
  TEST,
  INSERT_PROBE,
  YOUNGS_MODULUS,
  INFO,
  HOMING
} machineState = States::MENU;

HX711 loadCell;
LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_LINES);
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

//-----------------------------
//- Functions and Subroutines -
//-----------------------------

void drawMenu(LiquidCrystal_I2C &lcd,
              int modulusMenuCounter,
              const UserInterface MenuItems,
              int menuDepth,
              int activeSubMenu,
              bool redraw) {
  if (!redraw && !displayWrite) {
    return;
  }
  displayWrite = false;
  lcd.clear();
  Serial.println(modulusMenuCounter);
  for (int i = 0; i <= 3; i++) {
    lcd.setCursor(1, i);
    if (modulusMenuCounter == i && activeSubMenu == -1) {
      lcd.print("> " + MenuItems[i][0][0]);
    } else if (activeSubMenu == -1) {
      lcd.print(MenuItems[i][0][0]);
    } else if (activeSubMenu != -1) {
      lcd.print(MenuItems[activeSubMenu][menuDepth][i]);
    }
  }
}

// NEU: Encoder-Zähler zurücksetzen (ISR kurz sperren)
void resetEncoder() {
  noInterrupts();
  rotaryCounts = 0;
  encSteps = 0;
  interrupts();
}

void runStepper() {
  stepper.run();  // nicht-blockierend, immer aufrufen

  // z.B. in mm umrechnen: pos / schritteProMm
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 200) {
    lastPrint = millis();
    Serial.println(stepper.currentPosition());
  }
}

//Interrupts

// NEU: Auswertung mit Zustandstabelle, Prellen hebt sich selbst auf
void IRAM_ATTR readEncoderRotations()
{
  uint8_t now = (digitalRead(CLK_PIN) << 1) | digitalRead(DT_PIN);
  encSteps += ENC_TABLE[(encState << 2) | now];
  encState = now;
  if (encSteps >= 4)  { rotaryCounts++; encSteps = 0; encoderChanged = true; }
  if (encSteps <= -4) { rotaryCounts--; encSteps = 0; encoderChanged = true; }
}

void IRAM_ATTR readEncoderButton() {
  uint32_t now = millis();
  if (now - lastButtonMs < 50) return;          // Prellen -> ignorieren
  if (digitalRead(SW_PIN) != LOW) return;       // Flanke vom Loslassen -> ignorieren
  lastButtonMs = now;
  buttonPressed = true;
}

/// Setup and Super loop

void setup() {
  Serial.begin(115200);
  // INIT: load cell
  loadCell.begin(DOUT_PIN, SCK_PIN);
  loadCell.set_scale(420.0);   // Kalibrierfaktor, selbst bestimmen
  loadCell.tare();

  // INIT: LCD screen
  lcd.init();
  lcd.backlight();

  // INIT: rotary encoder
  pinMode(CLK_PIN, INPUT_PULLUP);   // NEU: PULLUP
  pinMode(DT_PIN, INPUT_PULLUP);    // NEU: PULLUP
  pinMode(SW_PIN, INPUT_PULLUP);
  encState = (digitalRead(CLK_PIN) << 1) | digitalRead(DT_PIN);   // NEU: Startzustand
  attachInterrupt(digitalPinToInterrupt(CLK_PIN), readEncoderRotations, CHANGE);  // NEU: CHANGE
  attachInterrupt(digitalPinToInterrupt(DT_PIN),  readEncoderRotations, CHANGE);  // NEU: auch DT
  attachInterrupt(digitalPinToInterrupt(SW_PIN), readEncoderButton, FALLING);

  //INIT: stepper motor
  stepper.setEnablePin(EN_PIN);
  stepper.setPinsInverted(false, false, true); // EN ist active-low
  stepper.enableOutputs();

  stepper.setMaxSpeed(100);     // Schritte/s
  stepper.setAcceleration(500);  // Schritte/s²
  stepper.moveTo(2000);          // Ziel: Schritt 2000
}

void loop() {
  noInterrupts();
  int counts = rotaryCounts;
  bool redraw = encoderChanged;    // NEU: encoderChanged statt displayWrite
  encoderChanged = false;
  interrupts();

  int modulusMenuCounter = ((counts % 4) + 4) % 4;
  switch (machineState) {
    case States::MENU: {
        int menuItem = modulusMenuCounter;
        if (buttonPressed) {
          switch (menuItem) {
            case 0: {
                machineState = States::TEST;
                activeSubMenu = menuItem;
                break;
              }
            case 1: {
                machineState = States::INSERT_PROBE;
                activeSubMenu = menuItem;
                break;
              }
            case 2: {
                machineState = States::YOUNGS_MODULUS;
                activeSubMenu = menuItem;
                break;
              }
            case 3: {
                machineState = States::INFO;
                activeSubMenu = menuItem;
                break;
              }
          }
          displayWrite = true;
          buttonPressed = false;
          menuDepth++;
        }
        break;
      }
    case States::TEST: {
        if (!probeReady) { //FIXME nur provisorisch zum testen.
          delay(3000);
          machineState = States::MENU;
          activeSubMenu = -1;
          menuDepth = 0;
          resetEncoder();          // NEU
          buttonPressed = false;   // NEU: nach dem delay, Drücke verwerfen
          displayWrite = true;
        }
        break;
      }
    case States::INSERT_PROBE: {
        delay(6000);               // NEU: delay vor dem Zurücksetzen
        machineState = States::MENU;
        activeSubMenu = -1;
        menuDepth = 0;
        resetEncoder();
        buttonPressed = false;
        displayWrite = true;
        break;
      }
    case States::YOUNGS_MODULUS: {
        delay(6000);
        machineState = States::MENU;
        activeSubMenu = -1;
        menuDepth = 0;
        resetEncoder();
        buttonPressed = false;
        displayWrite = true;
        break;
      }

    case States::INFO: {
        delay(6000);
        machineState = States::MENU;
        activeSubMenu = -1;
        menuDepth = 0;
        resetEncoder();
        buttonPressed = false;
        displayWrite = true;
        break;
      }
    case States::HOMING: {

        break;
      }
  }
  // things need to be done every caycle after state machine
  drawMenu(lcd, modulusMenuCounter, menuItems, menuDepth, activeSubMenu, redraw);
}
