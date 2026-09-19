/*
 * ES3C28P - Board interface implementation for Bruce firmware
 *
 * هذا الملف مسؤول عن المنطق الخاص بالجهاز: الأزرار، اللمس، البطارية (ثابتة)، النوم.
 */

#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include <Arduino.h>
#include <globals.h>
#include <interface.h>
#include <XPT2046_Touchscreen.h>

// قيم المعايرة الحقيقية والدقيقة المقاسة للشاشة
#define TOUCH_MIN_X 480
#define TOUCH_MAX_X 3845
#define TOUCH_MIN_Y 393
#define TOUCH_MAX_Y 3673
#define TOUCH_Z_THRESHOLD 350

XPT2046_Touchscreen ts(TOUCH_CS);
static bool touchInitialized = false;

void _setup_gpio() {
    Serial.begin(115200);
    Serial.println("CP1");

    // الأزرار الخمسة - PULLUP داخلي، الضغط = LOW (BTN_ACT)
    pinMode(SEL_BTN, INPUT_PULLUP);
    pinMode(UP_BTN, INPUT_PULLUP);
    pinMode(DW_BTN, INPUT_PULLUP);
    pinMode(L_BTN, INPUT_PULLUP);
    pinMode(R_BTN, INPUT_PULLUP);

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

// البطارية ملغاة عمدًا - ثابتة دايمًا على 100% (تشغيل مستمر من مصدر خارجي)
int getBattery() { return 100; }

bool isCharging() { return false; }

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
    esp_sleep_enable_ext0_wakeup((gpio_num_t)SEL_BTN, BTN_ACT);
    esp_deep_sleep_start();
}

void goToDeepSleep() { powerOff(); }

// إطفاء بالضغط المطوّل على يسار + يمين مع بعض (نفس منطق smoochiee)
void checkReboot() {
    int countDown = 0;
    if (digitalRead(L_BTN) == BTN_ACT && digitalRead(R_BTN) == BTN_ACT) {
        uint32_t time_count = millis();
        while (digitalRead(L_BTN) == BTN_ACT && digitalRead(R_BTN) == BTN_ACT) {
            if (millis() - time_count > 500) {
                if (countDown == 0) {
                    int textWidth = tft.textWidth("PWR OFF IN 3/3", 1);
                    tft.fillRect(tftWidth / 2 - textWidth / 2, 7, textWidth, 18, bruceConfig.bgColor);
                }
                tft.setTextSize(1);
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                countDown = (millis() - time_count) / 1000 + 1;
                if (countDown < 4)
                    tft.drawCentreString("PWR OFF IN " + String(countDown) + "/3", tftWidth / 2, 12, 1);
                else {
                    tft.fillScreen(bruceConfig.bgColor);
                    while (digitalRead(L_BTN) == BTN_ACT || digitalRead(R_BTN) == BTN_ACT);
                    delay(200);
                    powerOff();
                }
                delay(10);
            }
        }
        delay(30);
        if (millis() - time_count > 500) {
            tft.fillRect(60, 12, tftWidth - 60, tft.fontHeight(1), bruceConfig.bgColor);
            drawStatusBar();
        }
    }
}

// معالجة مدخلات اللمس + الأزرار الخمسة معًا وتمريرها لنظام Bruce
void InputHandler() {
    static long d_tmp = 0;
    static unsigned long btn_tm = 0;

    // ---------------- اللمس ----------------
    if (millis() - d_tmp > 150 || LongPress) {
#ifdef HAS_TOUCH
        if (touchInitialized && ts.touched()) {
            TS_Point p = ts.getPoint();

            if (p.z > TOUCH_Z_THRESHOLD) {
                int mappedX = map(p.x, TOUCH_MIN_X, TOUCH_MAX_X, 0, TFT_WIDTH);
                int mappedY = map(p.y, TOUCH_MIN_Y, TOUCH_MAX_Y, 0, TFT_HEIGHT);

                mappedX = constrain(mappedX, 0, TFT_WIDTH - 1);
                mappedY = constrain(mappedY, 0, TFT_HEIGHT - 1);

                uint8_t rot = bruceConfigPins.rotation;
                if (rot == 1) {
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

                touchPoint.pressed = true;
                touchHeatMap(touchPoint);
            }

        END_TOUCH:
            d_tmp = millis();
        }
#endif
    }

    // ---------------- الأزرار الخمسة ----------------
    if (millis() - btn_tm < 200 && !LongPress) return;

    bool _u = digitalRead(UP_BTN);
    bool _d = digitalRead(DW_BTN);
    bool _l = digitalRead(L_BTN);
    bool _r = digitalRead(R_BTN);
    bool _s = digitalRead(SEL_BTN);

    if (_u == BTN_ACT || _d == BTN_ACT || _l == BTN_ACT || _r == BTN_ACT || _s == BTN_ACT) {
        btn_tm = millis();
        if (!wakeUpScreen()) AnyKeyPress = true;
        else return;
    } else {
        return;
    }

    if (_l == BTN_ACT) { PrevPress = true; }
    if (_r == BTN_ACT) { NextPress = true; }
    if (_u == BTN_ACT) {
        UpPress = true;
        PrevPagePress = true;
    }
    if (_d == BTN_ACT) {
        DownPress = true;
        NextPagePress = true;
    }
    if (_s == BTN_ACT) { SelPress = true; }
    if (_l == BTN_ACT && _r == BTN_ACT) {
        EscPress = true;
        NextPress = false;
        PrevPress = false;
    }

    checkPowerSaveTime();
}

void taskInputHandler(void *arg) {
    // تأخير بسيط قبل بداية أول قراءة عشان نتفادى السباق مع تحميل الثيم/أنميشن البوت على SPI
    vTaskDelay(pdMS_TO_TICKS(1500));

    while (true) {
        InputHandler();
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}
