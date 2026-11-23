#include <Arduino.h>
#include "HX711.h"
#include "loadcell.h"
#include "pins.h"

static HX711 loadCell;

void loadCellBegin() {
  //loadCell.begin(DOUT_PIN, SCK_PIN);
  //loadCell.set_scale(420.0);   // Kalibrierfaktor, selbst bestimmen
  //loadCell.tare();
}
