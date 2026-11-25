#ifndef PM3_PINS_H
#define PM3_PINS_H

#include <stdint.h>

// Hardware configuration (pin numbers for the classic ESP32-DevKitC)
// Wiring as in the simulation: https://wokwi.com/projects/478425644051706881
// constexpr instead of #define: typed, scoped, visible in the debugger.

// LCD display
constexpr uint8_t I2C_ADDR    = 0x27;
constexpr uint8_t LCD_COLUMNS = 20;
constexpr uint8_t LCD_LINES   = 4;
constexpr uint8_t I2C_SDA     = 21; // Standard-SDA
constexpr uint8_t I2C_SCL     = 22; // Standard-SCL

// Rotary encoder
constexpr uint8_t CLK_PIN     = 27;
constexpr uint8_t DT_PIN      = 26;
constexpr uint8_t SW_PIN      = 25;

// Load cell
constexpr uint8_t DOUT_PIN    = 16;
constexpr uint8_t SCK_PIN     = 4;

// Stepper
constexpr uint8_t STEP_PIN    = 32;
constexpr uint8_t EN_PIN      = 13;
constexpr uint8_t DIR_PIN     = 33;

// Noch nicht verwendet: SD-Karte und Endschalter, Pins noch festzulegen
// (die bisherigen Reservierungen galten nur fuer den C6)

// Misc
constexpr float M_pif = 3.14159265358979323846f;

#endif // PM3_PINS_H
