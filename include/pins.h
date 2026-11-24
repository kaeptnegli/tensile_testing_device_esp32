#ifndef PM3_PINS_H
#define PM3_PINS_H

#include <stdint.h>

// Hardware configuration (pin numbers for the ESP32-C6)
// constexpr instead of #define: typed, scoped, visible in the debugger.

// LCD display
constexpr uint8_t I2C_ADDR    = 0x27;
constexpr uint8_t LCD_COLUMNS = 20;
constexpr uint8_t LCD_LINES   = 4;
constexpr uint8_t I2C_SDA     = 23; // C6: Standard-SDA
constexpr uint8_t I2C_SCL     = 22; // C6: Standard-SCL

// Rotary encoder
constexpr uint8_t CLK_PIN     = 4;  // C6: war 27
constexpr uint8_t DT_PIN      = 5;  // C6: war 26
constexpr uint8_t SW_PIN      = 6;  // C6: war 25

// Load cell
constexpr uint8_t DOUT_PIN    = 2;  // C6: war 16
constexpr uint8_t SCK_PIN     = 3;  // C6: war 4

// Stepper
constexpr uint8_t STEP_PIN    = 7;  // C6: war 32
constexpr uint8_t EN_PIN      = 11; // C6: war 13
constexpr uint8_t DIR_PIN     = 10; // C6: war 33

// C6: reserviert fuer spaeter (noch nicht verwendet)
// SD-Karte SPI: SCK 21, MISO 20, MOSI 19, CS 18 (C6-SPI-Standardpins)
// Endschalter: GPIO1 (INPUT_PULLUP), frei: GPIO0

// Misc
constexpr float M_pif = 3.14159265358979323846f;

#endif // PM3_PINS_H
