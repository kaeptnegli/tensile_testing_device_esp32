#ifndef PM3_PINS_H
#define PM3_PINS_H

#include <stdint.h>

// Hardware configuration (pin numbers for the ESP32-C6-DevKitC-1)
// The Wokwi simulation (https://wokwi.com/projects/478425644051706881)
// uses a classic ESP32: its pins are noted as "ESP32: ..." for reference.
// constexpr instead of #define: typed, scoped, visible in the debugger.

// LCD display
constexpr uint8_t I2C_ADDR    = 0x27;
constexpr uint8_t LCD_COLUMNS = 20;
constexpr uint8_t LCD_LINES   = 4;
constexpr uint8_t I2C_SDA     = 23; // C6: Standard-SDA   (ESP32: 21)
constexpr uint8_t I2C_SCL     = 22; // C6: Standard-SCL   (ESP32: 22)

// Rotary encoder
constexpr uint8_t CLK_PIN     = 4;  // ESP32: 27
constexpr uint8_t DT_PIN      = 5;  // ESP32: 26
constexpr uint8_t SW_PIN      = 6;  // ESP32: 25

// Load cell
constexpr uint8_t DOUT_PIN    = 2;  // ESP32: 16
constexpr uint8_t SCK_PIN     = 3;  // ESP32: 4

// Stepper
constexpr uint8_t STEP_PIN    = 7;  // ESP32: 32
constexpr uint8_t EN_PIN      = 11; // ESP32: 13
constexpr uint8_t DIR_PIN     = 10; // ESP32: 33

// Limit Switch
constexpr uint8_t SIG_PIN     = 1;  // ESP32: 14 (INPUT_PULLUP)

// Noch nicht verwendet:
// SD-Karte SPI: SCK 21, MISO 20, MOSI 19, CS 18 (C6-SPI-Standardpins)
// frei: GPIO0

// Misc
constexpr float M_pif = 3.14159265358979323846f;

#endif // PM3_PINS_H
