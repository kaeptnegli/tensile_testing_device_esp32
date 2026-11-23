#include <Arduino.h>
#include <vector>
#include <LiquidCrystal_I2C.h>
#include "menu.h"
#include "pins.h"

// Module-private: only menu.cpp talks to the LCD.
static LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_LINES);

using UserInterface = std::vector<std::vector<std::vector<String>>>;
static const UserInterface menuItems = {
  {{"Start Test"}, {"Insert Probe First,", "and Confirm!", "", ">Click to go back"},{"Force:", "#FORCE", "Strain:", "#STRAIN"},{"Test finished!", "Youngs Modulus:", "#YOUNGS_MODULUS", ""}}, // 0 -> main menucel specimen insert
  {{"Insert Probe"}, {"", "#HEIGHT", "", "Press to continue"}, {"Clamp Specimen", "", "", "Press To continue"}, {"", "confirm insertion", "cancel insertion", ""}},
  {{"Youngs Modulus"}, {"Youngs Modulus:", "270 GPa", "", ""}},
  {{"INFO"}, {"Status:", "Finished", "Error:", "None"}}
};

void menuBegin() {
  lcd.init();
  lcd.backlight();
}

void drawMenu(MenuState &ui,
              const MachineData &data,
              int modulusMenuCounter,
              int modulusMenuCounterTwo,
              bool redraw) {
  if (!redraw && !ui.displayWrite) {
    return;
  }
  ui.displayWrite = false;

  const int activeSubMenu = ui.activeSubMenu;
  const int menuDepth = ui.menuDepth;

  lcd.clear();
  for (int i = 0; i <= 3; i++) {
    lcd.setCursor(1, i);
    if (modulusMenuCounter == i && activeSubMenu == -1) { // Main Menu Page with cursor
      lcd.print("> " + menuItems[i][0][0]);
    } else if (activeSubMenu == -1) { // Main Menu Page without cursor
      lcd.print(menuItems[i][0][0]);
    } else if (activeSubMenu == 0 && menuDepth == 2) { // Show running test
      if (menuItems[activeSubMenu][menuDepth][i] == "#FORCE") {
        lcd.print(data.force);
      } else if (menuItems[activeSubMenu][menuDepth][i] == "#STRAIN") {
        lcd.print(data.strain);
      } else {
        lcd.print(menuItems[activeSubMenu][menuDepth][i]);
      }
    } else if (activeSubMenu == 0 && menuDepth == 3) { // test finished show YM
      if (menuItems[activeSubMenu][menuDepth][i] == "#YOUNGS_MODULUS") {
        lcd.print(data.youngsModulus);
      } else {
        lcd.print(menuItems[activeSubMenu][menuDepth][i]);
      }
    } else if (activeSubMenu == 1 && menuDepth == 1) { // insert probe page 1
      if (menuItems[activeSubMenu][menuDepth][i] == "#HEIGHT") { // Replace placeholder #HEIGHT with rodHeight
        lcd.print(data.rodHeight);
      } else {
        lcd.print(menuItems[activeSubMenu][menuDepth][i]);
      }
    } else if (activeSubMenu == 1 && menuDepth == 2) { // insert probe page 2
      lcd.print(menuItems[activeSubMenu][menuDepth][i]);
    } else if (activeSubMenu == 1 && menuDepth == 3) { // insert probe page 3
      if ((modulusMenuCounterTwo + 1) == i) { // Insertion confirmation with cursor
        lcd.print("> " + menuItems[activeSubMenu][menuDepth][i]);
      } else {
        lcd.print(menuItems[activeSubMenu][menuDepth][i]);
      }
    } else if (activeSubMenu != -1) { // Device Info and Youngs Modulus
      lcd.print(menuItems[activeSubMenu][menuDepth][i]);
    }
  }
}
