#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

#ifndef DEVICE_NAME
#define DEVICE_NAME "ES3C28P"
#endif

// =============================================
// USB & UART0
// =============================================
#define USB_VID 0x303a
#define USB_PID 0x1001

static const uint8_t TX = 43;
static const uint8_t RX = 44;

// =============================================
// Main SPI Bus & RFID SS Aliases
// =============================================
#define SPI_SCK_PIN 14
#define SPI_MOSI_PIN 3
#define SPI_MISO_PIN 2
#define SPI_SS_PIN 21
#define SPI_SS2 21

static const uint8_t SS = SPI_SS_PIN;
static const uint8_t MOSI = SPI_MOSI_PIN;
static const uint8_t SCK = SPI_SCK_PIN;
static const uint8_t MISO = SPI_MISO_PIN;
static const uint8_t SDA = 8;
static const uint8_t SCL = 9;

// =============================================
// TFT Display & Touch (ILI9341 + XPT2046)
// =============================================
#define USER_SETUP_LOADED
#define ILI9341_2_DRIVER 1
#define TFT_INVERSION_ON 1
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#define TFT_MISO 13
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_CS 10
#define TFT_DC 46
#define TFT_RST -1 
#define TFT_BL 45  
#define TFT_BACKLIGHT_ON HIGH
#define SMOOTH_FONT 1

#define TOUCH_CS 17 

#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 20000000
#define SPI_TOUCH_FREQUENCY 2500500

#define HAS_SCREEN 1
#define ROTATION 1 
#define MINBRIGHT 1
#define BACKLIGHT 45

#define HAS_TOUCH 1
#define HAS_RESISTIVE_TOUCH 1

// =============================================
// NRF24L01 2.4GHz Radio (Safe Pins - No 5, 6, 16)
// =============================================
#define USE_NRF24_VIA_SPI
#define NRF24_CE_PIN 1      
#define NRF24_SS_PIN 21
#define NRF24_MOSI_PIN SPI_MOSI_PIN
#define NRF24_SCK_PIN SPI_SCK_PIN
#define NRF24_MISO_PIN SPI_MISO_PIN

// =============================================
// Infrared (IR TX / RX) - (Excluding 5, 6, 16)
// =============================================
#define TXLED 4     
#define RXLED 7     
#define LED_ON HIGH
#define LED_OFF LOW

#define IR_TX_PINS {{"GPIO4", 4}, {"GPIO14", 14}, {"GPIO21", 21}}
#define IR_RX_PINS {{"GPIO7", 7}, {"GPIO14", 14}, {"GPIO21", 21}}

// =============================================
// RF Pins Maps
// =============================================
#define RF_TX_PINS {{"GPIO4", 4}, {"GPIO14", 14}, {"GPIO21", 21}}
#define RF_RX_PINS {{"GPIO7", 7}, {"GPIO14", 14}, {"GPIO21", 21}}

// =============================================
// Buttons & Battery
// =============================================
#define HAS_BTN 1
#define BTN_ALIAS "\"Boot\""
#define BTN_PIN 0 
#define BTN_ACT LOW
#define SEL_BTN 0 

#define ANALOG_BAT_PIN 9
#define ANALOG_BAT_MULTIPLIER 2.0f 

// =============================================
// Serial & Deep Sleep
// =============================================
#define SERIAL_TX 43
#define SERIAL_RX 44
#define GPS_SERIAL_TX SERIAL_TX
#define GPS_SERIAL_RX SERIAL_RX

#define DEEPSLEEP_WAKEUP_PIN 0 
#define DEEPSLEEP_PIN_ACT LOW

#endif /* Pins_Arduino_h */
