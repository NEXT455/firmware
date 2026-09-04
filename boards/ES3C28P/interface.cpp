/*
 * ES3C28P - Board interface implementation for Bruce firmware
 *
 * ملاحظة مهمة:
 * لا تضيف هنا أي أسطر لتعريف بنات NRF24 أو CC1101 أو TFT أو Touch
 * (مثل bruceConfigPins.NRF24_bus.cs أو bruceConfigPins.tftMosi).
 * هذي البنات تتقرأ تلقائيًا من الـ -D فلاجز الموجودة بملف ES3C28P.ini
 * عبر core/configPins.h (اللي يستخدم #ifdef على أسماء مثل NRF24_SCK_PIN).
 * هذا الملف مسؤول بس عن المنطق الخاص بالجهاز: الأزرار، الباطري، النوم، واللمس.
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

    Serial.begin(115200);
    Serial.println("CP1");

    // IR pins: مأخوذة من IR_RX_PIN / IR_TX_PIN المعرفة بالـ .ini
    bruceConfigPins.irRx = (gpio_num_t)IR_RX_PIN;
    bruceConfigPins.irTx = (gpio_num_t)IR_TX_PIN;

    Serial.println("CP2");
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

void InputHandler() {
    // Stub function required by Bruce interface
}

void taskInputHandler(void *arg) {
    static long tm = 0;
    // Boot animation (boot_screen_anim) draws to the TFT for ~7s right after
    // setup(). TFT_eSPI is not thread-safe, so reading touch from this task
    // while the main task is animating causes SPI/task corruption and a
    // FreeRTOS assert crash. Skip touch polling for the first 3.5s to avoid
    // overlapping with the animation window.
    const unsigned long BOOT_TOUCH_GUARD_MS = 3500;

    while (true) {
        if (millis() - tm > 200 || LongPress) {
            if (touchInitialized && millis() > BOOT_TOUCH_GUARD_MS) {
                uint16_t t_x = 0, t_y = 0;
                bool touched = tft.getTouch(&t_x, &t_y);

                if (touched) {
                    tm = millis();

                    if (!wakeUpScreen()) AnyKeyPress = true;
                    else continue;

                    touchPoint.x = t_x;
                    touchPoint.y = t_y;
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
