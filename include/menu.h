#pragma once
#include "machine.h"

// LCD menu module.
// The LCD object and the menu texts live privately in menu.cpp.

// Navigation state of the user interface. Owned by main.cpp, changed by the
// state machine, read by drawMenu().
struct MenuState {
  int  menuDepth     = 0;
  int  activeSubMenu = -1;   // 0 -> test, 1 -> insert, 2 -> youngs, 3 -> info, -1 -> main menu
  bool displayWrite  = true; // request a redraw; drawMenu() clears it
};

// Initialise the LCD. Wire.begin() must have been called before.
void menuBegin();

// Draw the current page if `redraw` is set or `ui.displayWrite` was requested.
void drawMenu(MenuState &ui,
              const MachineData &data,
              int modulusMenuCounter,
              int modulusMenuCounterTwo,
              bool redraw);
