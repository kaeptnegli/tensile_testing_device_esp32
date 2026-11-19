#include <Arduino.h>  // H2: in PlatformIO-.cpp noetig
#include <Wire.h>     // H2: fuer Wire.begin(12, 22)
#include <vector>
#include <LiquidCrystal_I2C.h>

// PREPROCESSOR
// ------------
// LCD DISPLAy
#define I2C_ADDR    0x27
#define LCD_COLUMNS 20
#define LCD_LINES   4
#define I2C_SDA     12 // H2
#define I2C_SCL     22 // H2
// Rotary Encoder
#define CLK_PIN     0  // H2: war 27
#define DT_PIN      1  // H2: war 26
#define SW_PIN      5  // H2: war 25


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

LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_LINES);

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

//Interrupts

// NEU: Auswertung mit Zustandstabelle, Prellen hebt sich selbst auf
void IRAM_ATTR readEncoderRotations()
{
  uint8_t now = (digitalRead(CLK_PIN) << 1) | digitalRead(DT_PIN);
  encSteps += ENC_TABLE[(encState << 2) | now];
  encState = now;
  if (encSteps >= 4)  { rotaryCounts--; encSteps = 0; encoderChanged = true; }
  if (encSteps <= -4) { rotaryCounts++; encSteps = 0; encoderChanged = true; }
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
  // INIT: LCD screen
  Wire.begin(I2C_SDA, I2C_SCL);  // H2
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
        delay(3000);               // NEU: delay vor dem Zurücksetzen
        machineState = States::MENU;
        activeSubMenu = -1;
        menuDepth = 0;
        resetEncoder();
        buttonPressed = false;
        displayWrite = true;
        break;
      }
    case States::YOUNGS_MODULUS: {
        delay(3000);
        machineState = States::MENU;
        activeSubMenu = -1;
        menuDepth = 0;
        resetEncoder();
        buttonPressed = false;
        displayWrite = true;
        break;
      }

    case States::INFO: {
        delay(3000);
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

