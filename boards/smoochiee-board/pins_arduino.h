#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

static const uint8_t TX = 1;
static const uint8_t RX = 2;

// Modified elsewhere
static const uint8_t SS = 3;
static const uint8_t MOSI = 17;
static const uint8_t MISO = 8;
static const uint8_t SCK = 18;

#define SERIAL_RX 2
#define SERIAL_TX 1

#define BTN_ALIAS "\"OK\""
#define HAS_5_BUTTONS
#define SEL_BTN 0
#define UP_BTN 41
#define DW_BTN 40
#define R_BTN 38
#define L_BTN 39
#define BTN_ACT LOW

#define RXLED 4
#define TXLED 5
#define LED_ON HIGH
#define LED_OFF LOW

#define USE_NRF24_VIA_SPI
#define NRF24_CE_PIN 21
#define NRF24_SS_PIN 14
#define NRF24_MOSI_PIN SPI_MOSI_PIN
#define NRF24_SCK_PIN SPI_SCK_PIN
#define NRF24_MISO_PIN SPI_MISO_PIN

#define HAS_SCREEN 1
#define ROTATION 1
#define MINBRIGHT (uint8_t)1

#define USER_SETUP_LOADED 1
#define ILI9341_DRIVER 1
#define TFT_RGB_ORDER 0
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#define TFT_BACKLIGHT_ON 1
#define TFT_BL -1
#define TFT_RST 12
#define TFT_DC 15
#define TFT_MISO -1
#define TFT_MOSI 17
#define TFT_SCLK 18
#define TFT_CS 7

// إرجاع إعدادات التاتش بالكامل
#define TOUCH_CS 9
#define SMOOTH_FONT 1
#define TOUCH_MOSI 17
#define TOUCH_MISO 8
#define TOUCH_CLK 18

#define SPI_FREQUENCY 4000000
#define SPI_READ_FREQUENCY 2000000
#define SPI_TOUCH_FREQUENCY 2500000

#define SDCARD_CS -1
#define SDCARD_SCK -1
#define SDCARD_MISO -1
#define SDCARD_MOSI -1

#define SPI_SCK_PIN 18
#define SPI_MOSI_PIN 17
#define SPI_MISO_PIN 8
#define SPI_SS_PIN 43

#define HAS_RGB_LED 0

#endif /* Pins_Arduino_h */
