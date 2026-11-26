#ifndef PM3_LIMITSWITCH_H
#define PM3_LIMITSWITCH_H

// Limit switch module (polled, no interrupt).
// Used by motion.cpp for homing and by main.cpp for status/errors.

// Configure the pin. Call once in setup(), before homing.
void limitSwitchBegin();

// true = switch triggered (with an NC switch also: wire broken).
bool limitSwitchPressed();

#endif // PM3_LIMITSWITCH_H
