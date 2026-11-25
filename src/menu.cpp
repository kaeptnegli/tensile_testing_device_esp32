#include "menu.h"
#include "pins.h"
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <vector>

// Module-private: only menu.cpp talks to the LCD.
static LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_LINES);

using UserInterface = std::vector<std::vector<std::vector<String>>>;
static const UserInterface menuItems = {
    {{"Start Test"},
     {"Insert Probe First,", "and Confirm!", "", ">Click to go back"},
     {"Force:", "#FORCE", "Strain:", "#STRAIN"},
     {"Test finished!", "Youngs Modulus:", "#YOUNGS_MODULUS",
      ""}}, // 0 -> main menucel specimen insert
    {{"Insert Probe"},
     {"", "#HEIGHT", "", "Press to continue"},
     {"Clamp Specimen", "", "", "Press To continue"},
     {"", "confirm insertion", "cancel insertion", ""}},
    {{"Youngs Modulus"}, {"Youngs Modulus:", "270 GPa", "", ""}},
    {{"INFO"}, {"Status:", "Finished", "Error:", "None"}}};

void menuBegin() {
  lcd.init();
  lcd.backlight();
}

void drawMenu(MenuState &ui, const MachineData &data, int modulusMenuCounter,
              int modulusMenuCounterTwo, bool redraw) {
  if (!redraw && !ui.displayWrite) {
    return;
  }
  ui.displayWrite = false;

  const int activeSubMenu = ui.activeSubMenu;
  const int menuDepth = ui.menuDepth;

  lcd.clear();
  for (int i = 0; i <= 3; i++) {
    lcd.setCursor(1, i);
    switch (activeSubMenu) {
    case -1: { // CASE: Main Menu
      if (modulusMenuCounter == i) {
        lcd.print("> " + menuItems[i][0][0]);
      } else {
        lcd.print(menuItems[i][0][0]);
      }
      break;
    }
    case 0: { // CASE: Test
      switch (menuDepth) {
      case 1: { // PAGE 1
        lcd.print(menuItems[activeSubMenu][menuDepth][i]);
        break;
      }
      case 2: { // PAGE 2
        if (menuItems[activeSubMenu][menuDepth][i] == "#FORCE") {
          lcd.print(data.force);
        } else if (menuItems[activeSubMenu][menuDepth][i] == "#STRAIN") {
          lcd.print(data.strain);
        } else {
          lcd.print(menuItems[activeSubMenu][menuDepth][i]);
        }
        break;
      }
      case 3: { // PAGE 3
        if (menuItems[activeSubMenu][menuDepth][i] == "#YOUNGS_MODULUS") {
          lcd.print(data.youngsModulus);
        } else {
          lcd.print(menuItems[activeSubMenu][menuDepth][i]);
        }
        break;
      }
      }
      break;
    }
    case 1: { // CASE: Insert Probe
      switch (menuDepth) {
      case 1: { // PAGE 1
        if (menuItems[activeSubMenu][menuDepth][i] == "#HEIGHT") {
          lcd.print(data.rodHeight);
        } else {
          lcd.print(menuItems[activeSubMenu][menuDepth][i]);
        }
        break;
      }
      case 2: { // PAGE 2
        lcd.print(menuItems[activeSubMenu][menuDepth][i]);
        break;
      }
      case 3: { // PAGE 3
        if ((modulusMenuCounterTwo + 1) == i) {
          lcd.print("> " + menuItems[activeSubMenu][menuDepth][i]);
        } else {
          lcd.print(menuItems[activeSubMenu][menuDepth][i]);
        }
        break;
      }
      }
      break;
    }
    case 2: { // CASE: Youngs Modulus
      lcd.print(menuItems[activeSubMenu][menuDepth][i]);
      break;
    }
    case 3: { // Case: INFO Page
      lcd.print(menuItems[activeSubMenu][menuDepth][i]);
      break;
    }
    }
  }
}
