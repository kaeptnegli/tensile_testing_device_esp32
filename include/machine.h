#ifndef PM3_MACHINE_H
#define PM3_MACHINE_H

#include <stdint.h>

// State machine states
enum States {
  MENU,
  TEST,
  INSERT_PROBE,
  YOUNGS_MODULUS,
  INFO,
  HOMING
};

// Measured / machine values. Owned by main.cpp and passed by reference
// to whoever needs them (e.g. drawMenu), instead of being hidden globals.
struct MachineData {
  // Probe
  bool    probeReady    = false;
  int64_t youngsModulus = 0;

  // Machine
  bool    testComplete  = false;
  int64_t rodHeight     = 0;
  int64_t force         = 0;
  int64_t strain        = 0;
};

#endif // PM3_MACHINE_H
