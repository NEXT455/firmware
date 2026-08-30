#pragma once

#include "pins_arduino.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <precompiler_flags.h>
#include <set>

class DummyPPM {
public:
    void enableOTG() {}
    void disableOTG() {}
    bool isCharging() { return false; }
    int getBattery() { return 100; }
};
inline DummyPPM PPM;

enum RFModules {
    NRF24_SPI_MODULE = 0
};

class BruceConfigPins {
public:
    struct SPIPins {
        gpio_num_t sck = GPIO_NUM_NC;
        gpio_num_t miso = GPIO_NUM_NC;
        gpio_num_t mosi = GPIO_NUM_NC;
        gpio_num_t cs = GPIO_NUM_NC;
        gpio_num_t io0 = GPIO_NUM_NC;
        gpio_num_t io2 = GPIO_NUM_NC;

        SPIPins()
            : sck(GPIO_NUM_NC), miso(GPIO_NUM_NC), mosi(GPIO_NUM_NC), cs(GPIO_NUM_NC), io0(GPIO_NUM_NC),
              io2(GPIO_NUM_NC) {}

        SPIPins(
            gpio_num_t sck_val, gpio_num_t miso_val, gpio_num_t mosi_val, gpio_num_t cs_val,
            gpio_num_t io0_val = GPIO_NUM_NC, gpio_num_t io2_val = GPIO_NUM_NC
        )
            : sck(sck_val), miso(miso_val), mosi(mosi_val), cs(cs_val), io0(io0_val), io2(io2_val) {}

        void fromJson(JsonObject obj) {
            sck = (gpio_num_t)(obj["sck"] | (int)GPIO_NUM_NC);
            miso = (gpio_num_t)(obj["miso"] | (int)GPIO_NUM_NC);
            mosi = (gpio_num_t)(obj["mosi"] | (int)GPIO_NUM_NC);
            cs = (gpio_num_t)(obj["cs"] | (int)GPIO_NUM_NC);
            io0 = (gpio_num_t)(obj["io0"] | (int)GPIO_NUM_NC);
            io2 = (gpio_num_t)(obj["io2"] | (int)GPIO_NUM_NC);
        }

        void toJson(JsonObject obj) const {
            obj["sck"] = sck;
            obj["miso"] = miso;
            obj["mosi"] = mosi;
            obj["cs"] = cs;
            obj["io0"] = io0;
            obj["io2"] = io2;
        }

        bool checkConflict(uint8_t p) {
            gpio_num_t pin = (gpio_num_t)p;
            if (sck == pin || miso == pin || mosi == pin || cs == pin) return true;
            return false;
        }
    };

    const char *filepath = "/brucePins.conf";

#ifdef NRF24_SCK_PIN
    SPIPins NRF24_bus = {
        (gpio_num_t)NRF24_SCK_PIN,
        (gpio_num_t)NRF24_MISO_PIN,
        (gpio_num_t)NRF24_MOSI_PIN,
        (gpio_num_t)NRF24_SS_PIN,
        (gpio_num_t)NRF24_CE_PIN
    };
#else
    SPIPins NRF24_bus;
#endif

    int rotation = 1;

    String bleName = String("Keyboard_" + String((uint8_t)(ESP.getEfuseMac() >> 32), HEX));

    int irTx = 5;
    uint8_t irTxRepeats = 0;
    int irRx = 4;

    int rfTx = -1;
    int rfRx = -1;
    int rfModule = NRF24_SPI_MODULE;
    float rfFreq = 2400.0;
    int rfFxdFreq = 1;
    int rfScanRange = 3;

    int iButton = -1;

    BruceConfigPins() {};

    void createFile();
    void saveFile();
    void fromFile(bool checkFS = true);
    void loadFile(JsonDocument &jsonDoc, bool checkFS = true);
    void factoryReset();
    void validateConfig();
    void fromJson(JsonObject obj);
    void toJson(JsonObject obj) const;

    void setNrf24Pins(SPIPins value);
    void setSpiPins(SPIPins value);
    void validateSpiPins(SPIPins value);

    void setRotation(int value);
    void validateRotationValue();
    void setBleName(const String name);

    void setIrTxPin(int value);
    void setIrTxRepeats(uint8_t value);
    void setIrRxPin(int value);

    void setRfModule(RFModules value);
    void validateRfModuleValue();
    void setRfFreq(float value, int fxdFreq = 2);
    void setRfFxdFreq(float value);
    void setRfScanRange(int value, int fxdFreq = 0);
    void validateRfScanRangeValue();

    void setiButtonPin(int value);
};
