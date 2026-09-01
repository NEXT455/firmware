/*
 * ES3C28P - Board interface implementation for Bruce firmware
 */

#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include <Arduino.h>
#include <globals.h>
#include <interface.h>

#define ES3C28P_BTN_PIN 0 
#define ES3C28P_BTN_ACT LOW

static bool touchInitialized = false;

void _setup_gpio() {
    touchInitialized = true;

    // NRF24 GPIO Assignments (Matching ES3C28P.ini)
    bruceConfigPins.rfCs = NRF24_CS_PIN;
    bruceConfigPins.rfCe = NRF24_CE_PIN;

    // IR GPIO Assignments (Matching ES3C28P.ini)
    bruceConfigPins.irRx = IR_RX_PIN;
    bruceConfigPins.irTx = IR_TX_PIN;

    Serial.begin(115200);
}

void _post_setup_gpio() {
    pinMode(TFT_BL, OUTPUT);
    analogWrite(TFT_BL, 255);
}

int getBattery() {
    static bool adcInitialized = false;
    if (!adcInitialized) {
        pinMode(ANALOG_BAT_PIN, INPUT);
        analogSetAttenuation(ADC_11db);
        adcInitialized = true;
    }

    uint32_t adcReading = analogReadMilliVolts(ANALOG_BAT_PIN);
    float actualVoltage = (float)adcReading * 2.0f;

    const float MIN_VOLTAGE = 2500.0f;
    const float MAX_VOLTAGE = 4200.0f;

    int percent = (int)(((actualVoltage - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE)) * 100.0f);

    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    return percent;
}

void _setBrightness(uint8_t brightval) {
    if (brightval == 0) {
        analogWrite(TFT_BL, 0);
    } else {
        int bl = MINBRIGHT + round(((255 - MINBRIGHT) * brightval / 100.0f));
        analogWrite(TFT_BL, bl);
    }
}

void powerOff() {
    analogWrite(TFT_BL, 0);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)ES3C28P_BTN_PIN, ES3C28P_BTN_ACT);
    esp_deep_sleep_start();
}

void goToDeepSleep() { powerOff(); }

void checkReboot() {
    int c = 0;
    while (digitalRead(ES3C28P_BTN_PIN) == ES3C28P_BTN_ACT) {
        delay(100);
        c++;
        if (c > 20) {
            powerOff();
        }
    }
}

bool isCharging() { return false; }

void taskInputHandler(void *arg) {
    static long tm = 0;

    while (true) {
        if (millis() - tm > 200 || LongPress) {
            if (touchInitialized) {
                uint16_t t_x = 0, t_y = 0;
                bool touched = tft.getTouchX() > 0 || tft.getTouchY() > 0;

                if (touched) {
                    tm = millis();

                    if (!wakeUpScreen()) AnyKeyPress = true;
                    else continue;

                    touchPoint.x = tft.getTouchX();
                    touchPoint.y = tft.getTouchY();
                    touchPoint.pressed = true;
                    touchHeatMap(touchPoint);
                }
            }

            if (digitalRead(ES3C28P_BTN_PIN) == ES3C28P_BTN_ACT) {
                if (!wakeUpScreen()) {
                    AnyKeyPress = true;
                    SelPress = true;
                }
                while (digitalRead(ES3C28P_BTN_PIN) == ES3C28P_BTN_ACT) delay(10);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
