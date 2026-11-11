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
// Rotary Encoder
int counter = 0;
int currentStateCLK;
int lastStateCLK;
bool currentStateSW;
bool lastStateSW;
// Probe
bool probeReady = false;
const String menu[4] = {"Insert Probe", "Start Test", "Youngs Modulus", "Last Error"};

HX711 loadCell;
LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_LINES);
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// State Machine States
enum States {
  MENU,
  TEST,
  INSERT_PROBE,
  YOUNGS_MODULUS,
  HOMING,
  ERROR
} machineState = States::MENU;

//fucntion declaration
void readRotaryEncoder(int &counter);

void drawMenu(LiquidCrystal_I2C &lcd, int menuCounter);

void runStepper();


void setup() {
  Serial.begin(115200);
  // INIT: load cell
  loadCell.begin(DOUT_PIN, SCK_PIN);
  loadCell.set_scale(420.0);   // Kalibrierfaktor, selbst bestimmen
  loadCell.tare();

  // INIT: LCD screen
  lcd.init();
  lcd.backlight();
  drawMenu(lcd, counter); //Write to LCD-Screen

  // INIT: rotary encoder
  pinMode(CLK_PIN, INPUT);
  pinMode(DT_PIN, INPUT);
  pinMode(SW_PIN, INPUT_PULLUP);
  lastStateCLK = digitalRead(CLK_PIN);
  lastStateSW = digitalRead(SW_PIN);

  //INIT: stepper motor
  stepper.setEnablePin(EN_PIN);
  stepper.setPinsInverted(false, false, true); // EN ist active-low
  stepper.enableOutputs();

  stepper.setMaxSpeed(100);     // Schritte/s
  stepper.setAcceleration(500);  // Schritte/s²
  stepper.moveTo(2000);          // Ziel: Schritt 2000
}

void loop() {
  switch (machineState) {
    case States::MENU: {
        readRotaryEncoder(counter);
        runStepper();
        break;
      }
    case States::TEST: {
        if (!probeReady) { //FIXME nur provisorisch zum testen.
          lcd.clear();
          lcd.setCursor(1, 0);
          lcd.print("Insert Probe First,");
          lcd.setCursor(1, 1);
          lcd.print("and Confirm!");
          machineState = States::MENU;
          delay(3000);
          drawMenu(lcd, counter);
        }
        //Read and Print load cell
        Serial.print(loadCell.get_units(10), 2);
        Serial.println(" kg");
        break;
      }
    case States::INSERT_PROBE: {

        break;
      }
    case States::YOUNGS_MODULUS: {

        break;
      }
    case States::HOMING: {

        break;
      }
    case States::ERROR: {

        break;
      }

  }
}

//-----------------------------
//- Functions and Subroutines -
//-----------------------------

void readRotaryEncoder(int &counter) {
  //Rotary Rotated
  currentStateCLK = digitalRead(CLK_PIN);
  if (currentStateCLK != lastStateCLK && currentStateCLK == LOW) {
    if (digitalRead(DT_PIN) == HIGH) {
      counter++;   // rechts
    } else {
      counter--;   // links
    }
    //Serial.print("Position: ");
    //Serial.println(counter);
    drawMenu(lcd, counter); // FIXME: Get this function call out of here
  }
  lastStateCLK = currentStateCLK;

  //Button Pressed
  currentStateSW = digitalRead(SW_PIN);
  if (currentStateSW == LOW && lastStateSW == HIGH) {
    if (((counter % 4) + 4) % 4 == 1) { //FIXME nur provisorisch zum testen.
      machineState = States::TEST;
    }
    delay(50); // debounce
  }
  lastStateSW = currentStateSW;
}

void drawMenu(LiquidCrystal_I2C &lcd, int menuCounter) {
  int modulusMenuCounter = ((menuCounter % 4) + 4) % 4;
  lcd.clear();
  for (int i = 0; i <= 3; i++) {
    lcd.setCursor(2, i);
    if (modulusMenuCounter == i) {
      lcd.print("> " + menu[i]);
    } else {
      lcd.print(menu[i]);
    }
  }
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
