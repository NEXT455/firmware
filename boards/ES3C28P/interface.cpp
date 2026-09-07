/*
 * ES3C28P - Board interface implementation for Bruce firmware
 *
 * هذا الملف مسؤول عن المنطق الخاص بالجهاز: الأزرار، البطارية، النوم، واللمس.
 */

#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include <Arduino.h>
#include <globals.h>
#include <interface.h>
#include <XPT2046_Touchscreen.h>

#define ES3C28P_BTN_PIN 0
#define ES3C28P_BTN_ACT LOW

// حد أدنى وأقصى لقراءات الـ ADC الخام من شريحة XPT2046 للمعايرة
#define TOUCH_MIN_X 200
#define TOUCH_MAX_X 3800
#define TOUCH_MIN_Y 200
#define TOUCH_MAX_Y 3800

XPT2046_Touchscreen ts(TOUCH_CS);
static bool touchInitialized = false;

void _setup_gpio() {
    Serial.begin(115200);
    Serial.println("CP1");

    bruceConfigPins.irRx = (gpio_num_t)IR_RX_PIN;
    bruceConfigPins.irTx = (gpio_num_t)IR_TX_PIN;

    Serial.println("CP2");
}

void _post_setup_gpio() {
    pinMode(TFT_BL, OUTPUT);
    analogWrite(TFT_BL, 255);

#ifdef HAS_TOUCH
    ts.begin(tft.getSPIinstance());
    ts.setRotation(ROTATION);
    touchInitialized = true;
    Serial.println("=== Touch init done ===");
#endif
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

// معالجة مدخلات اللمس وتمريرها لنظام Bruce
void InputHandler() {
    static long d_tmp = 0;

    if (millis() - d_tmp > 150 || LongPress) {
#ifdef HAS_TOUCH
        if (touchInitialized && ts.touched()) {
            TS_Point p = ts.getPoint();

            // 1. تحويل القراءات الخام (ADC) إلى مقاسات الشاشة الحقيقية (240x320)
            int mappedX = map(p.x, TOUCH_MIN_X, TOUCH_MAX_X, 0, TFT_WIDTH);
            int mappedY = map(p.y, TOUCH_MIN_Y, TOUCH_MAX_Y, 0, TFT_HEIGHT);

            // ضمان عدم خروج القيم عن نطاق أبعاد الشاشة
            mappedX = constrain(mappedX, 0, TFT_WIDTH);
            mappedY = constrain(mappedY, 0, TFT_HEIGHT);

            // 2. ضبط اتجاه المحاور بناءً على تدوير الشاشة (ROTATION 1)
            uint8_t rot = bruceConfigPins.rotation;
            if (rot == 1) {
                // وضع LNDSCAPE العادي
                touchPoint.x = mappedY;
                touchPoint.y = TFT_WIDTH - mappedX;
            } else if (rot == 3) {
                touchPoint.x = TFT_HEIGHT - mappedY;
                touchPoint.y = mappedX;
            } else {
                touchPoint.x = mappedX;
                touchPoint.y = mappedY;
            }

            if (!wakeUpScreen()) {
                AnyKeyPress = true;
            } else {
                goto END_TOUCH;
            }

            // 3. إرسال النقاط المعايرة لخريطة Bruce الاستشعارية
            touchPoint.pressed = true;
            touchHeatMap(touchPoint);

        END_TOUCH:
            d_tmp = millis();
        }
#endif
    }

#ifdef HAS_BTN
    checkPowerSaveTime();
    if (digitalRead(ES3C28P_BTN_PIN) == ES3C28P_BTN_ACT) {
        if (!wakeUpScreen()) AnyKeyPress = true;
        SelPress = true;
        long tmp = millis();
        while ((millis() - tmp) < 200 && digitalRead(ES3C28P_BTN_PIN) == ES3C28P_BTN_ACT);
    }
#endif
}

void taskInputHandler(void *arg) {
    while (true) {
        InputHandler();
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}
