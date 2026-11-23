#include <Arduino.h>  // C6: in PlatformIO-.cpp noetig
#include <Wire.h>     // C6: fuer Wire.begin(23, 22)
#include <vector>
#include "HX711.h"
#include <LiquidCrystal_I2C.h>
#include <AccelStepper.h>

// Vocab:
// ISR: Interruption Service Routine
// IRAM_ATTR: store ISR function is fast memory


// TODO:
// use displayWrite as Argument for drawMenu
// FIX drawMenu if else structure
// Fix insert Probe State if else structure
// Fix Rotary Encode button bounce

// PREPROCESSOR
// ------------
// LCD DISPLAy
#define I2C_ADDR    0x27
#define LCD_COLUMNS 20
#define LCD_LINES   4
#define I2C_SDA     23 // C6: Standard-SDA
#define I2C_SCL     22 // C6: Standard-SCL
// Rotary Encoder
#define CLK_PIN     4  // C6: war 27
#define DT_PIN      5  // C6: war 26
#define SW_PIN      6  // C6: war 25
// Load Cell
#define DOUT_PIN    2  // C6: war 16
#define SCK_PIN     3  // C6: war 4
//Stepper
#define STEP_PIN    7  // C6: war 32
#define EN_PIN      11 // C6: war 13
#define DIR_PIN     10 // C6: war 33
// C6: reserviert fuer spaeter (noch nicht verwendet)
// SD-Karte SPI: SCK 21, MISO 20, MOSI 19, CS 18 (C6-SPI-Standardpins)
// Endschalter: GPIO1 (INPUT_PULLUP), frei: GPIO0
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
const int8_t ENC_TABLE[16] = {0, -1, 1, 0,  1, 0, 0, -1,  -1, 0, 0, 1,  0, 1, -1, 0};

//User Interface - LCD Display Menu Items
int menuDepth = 0;
int activeSubMenu = -1; // 1 -> test, 2 -> insert, 3 -> youngs, 4 -> status -1 -> Main menu
volatile bool encoderChanged = true;   // nur die ISR setzt das
bool displayWrite = true;              // nur loop()/States setzen das, kein volatile nötig
using UserInterface = std::vector<std::vector<std::vector<String>>>;
const UserInterface menuItems = {
  {{"Start Test"}, {"Insert Probe First,", "and Confirm!", "", ">Click to go back"},{"Force:", "#FORCE", "Strain:", "#STRAIN"},{"Test finished!", "Youngs Modulus:", "#YOUNGS_MODULUS", ""}}, // 0 -> main menucel specimen insert
  {{"Insert Probe"}, {"", "#HEIGHT", "", "Press to continue"}, {"Clamp Specimen", "", "", "Press To continue"}, {"", "confirm insertion", "cancel insertion", ""}},
  {{"Youngs Modulus"}, {"Youngs Modulus:", "270 GPa", "", ""}},
  {{"INFO"}, {"Status:", "Finished", "Error:", "None"}}   // NEU: einfache Klammern
};

// Probe
bool probeReady = false;
int64_t youngsModulus = 0;

//Machine
bool testComplete = false;
int64_t rodHeight = 0;
int64_t force = 0;
int64_t strain = 0;


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
              int modulusMenuCounterTwo,
              const UserInterface MenuItems,
              int menuDepth,
              int activeSubMenu,
              bool redraw) {
  if (!redraw && !displayWrite) {
    return;
  }
  displayWrite = false;
  lcd.clear();
  //Serial.println(modulusMenuCounter);
  for (int i = 0; i <= 3; i++) {
    lcd.setCursor(1, i);
    if (modulusMenuCounter == i && activeSubMenu == -1) { // Main Menu Page with cursor
      lcd.print("> " + MenuItems[i][0][0]);
    } else if (activeSubMenu == -1) { // Main Menu Page with Cursor
      lcd.print(MenuItems[i][0][0]);
    } else if (activeSubMenu == 0 && menuDepth == 2) { // Show running test 
      if (MenuItems[activeSubMenu][menuDepth][i] == "#FORCE") {
        lcd.print(force);
      } else if (MenuItems[activeSubMenu][menuDepth][i] == "#STRAIN"){
        lcd.print(strain);
      } else {
        lcd.print(MenuItems[activeSubMenu][menuDepth][i]);
      }
    } else if (activeSubMenu == 0 && menuDepth == 3) { // test finished show YM
	if (MenuItems[activeSubMenu][menuDepth][i] == "#YOUNGS_MODULUS") {
	lcd.print(youngsModulus);
	} else {
        lcd.print(MenuItems[activeSubMenu][menuDepth][i]);
	}
    } else if (activeSubMenu == 1 && menuDepth == 1) { //insert probe page 1
      if (MenuItems[activeSubMenu][menuDepth][i] == "#HEIGHT") { // Replace playeholder #HEIGHT with rodHeight
        lcd.print(rodHeight);
      } else {
        lcd.print(MenuItems[activeSubMenu][menuDepth][i]);
      }
    } else if (activeSubMenu == 1 && menuDepth == 2) { // insert probe page 2
        lcd.print(MenuItems[activeSubMenu][menuDepth][i]);
    } else if (activeSubMenu == 1 && menuDepth == 3) { // insert probe page 3
      if ((modulusMenuCounterTwo+1) == i) { // Insertion confirmation  with Cursor c
        lcd.print("> " + MenuItems[activeSubMenu][menuDepth][i]);
      } else {
        lcd.print(MenuItems[activeSubMenu][menuDepth][i]);
      }
    } else if (activeSubMenu != -1) { // Device Info and Youngs Modulus
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

void testFinished() {
  testComplete = true;
}

//Interrupts

// NEU: Auswertung mit Zustandstabelle, Prellen hebt sich selbst auf
// 00 → 10 → 11 → 01 → 00 (rechts herum)
// 00 → 01 → 11 → 10 → 00 (links herum)
// CLK MSB - DT LSB
// const int8_t ENC_TABLE[16] = {0,-1, 1, 0,  1, 0, 0,-1,  -1, 0, 0, 1,  0, 1,-1, 0};
void IRAM_ATTR readEncoderRotationsISR()
{
  uint8_t now = (digitalRead(CLK_PIN) << 1) | digitalRead(DT_PIN);
  encSteps += ENC_TABLE[(encState << 2) | now];
  encState = now;
  if (encSteps >= 4)  {
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

void IRAM_ATTR readEncoderButtonISR() {
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
  //loadCell.begin(DOUT_PIN, SCK_PIN);
  //loadCell.set_scale(420.0);   // Kalibrierfaktor, selbst bestimmen
  //loadCell.tare();

  // INIT: LCD screen
  Wire.begin(I2C_SDA, I2C_SCL);  // C6: I2C-Pins explizit setzen
  lcd.init();
  lcd.backlight();

  // INIT: rotary encoder
  pinMode(CLK_PIN, INPUT_PULLUP);   // NEU: PULLUP
  pinMode(DT_PIN, INPUT_PULLUP);    // NEU: PULLUP
  pinMode(SW_PIN, INPUT_PULLUP);
  encState = (digitalRead(CLK_PIN) << 1) | digitalRead(DT_PIN);   // NEU: Startzustand
  attachInterrupt(digitalPinToInterrupt(CLK_PIN), readEncoderRotationsISR, CHANGE);  // NEU: CHANGE
  attachInterrupt(digitalPinToInterrupt(DT_PIN),  readEncoderRotationsISR, CHANGE);  // NEU: auch DT
  attachInterrupt(digitalPinToInterrupt(SW_PIN), readEncoderButtonISR, FALLING);

  //INIT: stepper motor
  //stepper.setEnablePin(EN_PIN);
  //stepper.setPinsInverted(false, false, true); // EN ist active-low
  //stepper.enableOutputs();

  //stepper.setMaxSpeed(100);     // Schritte/s
  //stepper.setAcceleration(500);  // Schritte/s²
  //stepper.moveTo(2000);          // Ziel: Schritt 2000
}

void loop() {
  noInterrupts();
  int counts = rotaryCounts;
  bool redraw = encoderChanged;    // NEU: encoderChanged statt displayWrite
  encoderChanged = false;
  interrupts();

  int modulusMenuCounter = ((counts % 4) + 4) % 4;
  int modulusMenuCounterTwo = ((counts % 2) + 2) % 2;
  switch (machineState) {
    case States::MENU: {
        int menuItem = modulusMenuCounter;
        if (buttonPressed) {
          switch (menuItem) {
            case 0: {
                machineState = States::TEST;
                activeSubMenu = menuItem;
	        if (!probeReady) {
	            menuDepth = 1;
		} else {
		    menuDepth = 2;
		}
                break;
              }
            case 1: {
                machineState = States::INSERT_PROBE;
                activeSubMenu = menuItem; // 1
                menuDepth++;
                break;
              }
            case 2: {
                machineState = States::YOUNGS_MODULUS;
                activeSubMenu = menuItem;
                menuDepth++;
                break;
              }
            case 3: {
                machineState = States::INFO;
                activeSubMenu = menuItem;
                menuDepth++;
                break;
              }
          }
          displayWrite = true;
          buttonPressed = false;
        }
        break;
      }
    case States::TEST: {
        if ((!probeReady && buttonPressed && menuDepth == 1) || menuDepth == 3) { 
	  if (menuDepth == 3) {
	      delay(3000);
	  }
          machineState = States::MENU;
          activeSubMenu = -1;
          menuDepth = 0;
          resetEncoder();
          buttonPressed = false;
          displayWrite = true;
        } else if (menuDepth == 2) {
          delay(3000);
	  menuDepth = 3;
          displayWrite = true;
	} 
        break;
      }
    case States::INSERT_PROBE: {
        if (redraw && menuDepth == 1) {
          if (rodHeight + counts < 0) {
            rodHeight = 0;
          } else {
            rodHeight += counts;
          }
          displayWrite = true;
          resetEncoder();
        }

        if (buttonPressed && menuDepth < 3) {
          menuDepth++;
          displayWrite = true;
        } else if (buttonPressed && menuDepth == 3) {
          Serial.println("case");
          if (modulusMenuCounterTwo == 0) {
            probeReady = true;
          } else {
            probeReady = false;
          }
          machineState = States::MENU;
          activeSubMenu = -1;
          menuDepth = 0;
          displayWrite = true;
        }

        buttonPressed = false;
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
  drawMenu(lcd, modulusMenuCounter, modulusMenuCounterTwo, menuItems, menuDepth, activeSubMenu, redraw);
}

