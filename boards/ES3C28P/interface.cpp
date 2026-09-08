#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "core/utils.h"

#include <Arduino.h>
#include <globals.h>
#include <interface.h>
#include <XPT2046_Touchscreen.h>

#define ES3C28P_BTN_PIN 0
#define ES3C28P_BTN_ACT LOW

// ============================================================
// XPT2046 TOUCH CALIBRATION
// ============================================================

#define TOUCH_MIN_X 480
#define TOUCH_MAX_X 3845
#define TOUCH_MIN_Y 393
#define TOUCH_MAX_Y 3673

#define TOUCH_Z_THRESHOLD 350

// مقدار التنعيم
#define TOUCH_SMOOTH_NUM 2
#define TOUCH_SMOOTH_DEN 3

// ============================================================
// TOUCH OBJECT
// ============================================================

XPT2046_Touchscreen ts(TOUCH_CS);

static bool touchInitialized = false;

// حالة اللمس السابقة
static bool wasTouching = false;

// آخر إحداثيات مستقرة
static int lastTouchX = -1;
static int lastTouchY = -1;

// ============================================================
// GPIO SETUP
// ============================================================

void _setup_gpio() {

    Serial.begin(115200);

    Serial.println("CP1");

#ifdef IR_RX_PIN
    bruceConfigPins.irRx = (gpio_num_t)IR_RX_PIN;
#endif

#ifdef IR_TX_PIN
    bruceConfigPins.irTx = (gpio_num_t)IR_TX_PIN;
#endif

    Serial.println("CP2");
}

// ============================================================
// POST GPIO SETUP
// ============================================================

void _post_setup_gpio() {

    /*
     * TFT_BL = -1 في Pins_Arduino.h
     * لذلك لا نحاول استخدامه كـ GPIO.
     */
#if defined(TFT_BL) && (TFT_BL >= 0)

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);

#endif

    // --------------------------------------------------------
    // TOUCH INIT
    // --------------------------------------------------------

#if defined(HAS_TOUCH)

    Serial.println("=== Initializing XPT2046 Touch ===");

    /*
     * نستخدم نفس SPI bus الخاص بالشاشة.
     */
    ts.begin(tft.getSPIinstance());

    /*
     * نخلي المكتبة تعرف اتجاه الشاشة.
     * لا نسوي rotation يدوي بعد قراءة الإحداثيات.
     */
    ts.setRotation(ROTATION);

    touchInitialized = true;

    Serial.println("=== Touch init done ===");

#endif
}

// ============================================================
// BATTERY
// ============================================================

int getBattery() {

#ifdef ANALOG_BAT_PIN

    uint32_t totalMv = 0;

    for (int i = 0; i < 4; i++) {
        totalMv += analogReadMilliVolts(ANALOG_BAT_PIN);
        delay(2);
    }

    uint32_t mv = totalMv / 4;

#ifdef ANALOG_BAT_MULTIPLIER
    mv = (uint32_t)(mv * ANALOG_BAT_MULTIPLIER);
#else
    mv = mv * 2;
#endif

    return constrain((int)mv, 2500, 4200);

#else

    // لا توجد دائرة قياس بطارية معرفة.
    return 0;

#endif
}

// ============================================================
// BRIGHTNESS
// ============================================================

void _setBrightness(uint8_t brightness) {

#if defined(TFT_BL) && (TFT_BL >= 0)

    analogWrite(TFT_BL, brightness);

#endif
}

// ============================================================
// POWER OFF
// ============================================================

void powerOff() {

    Serial.println("Power off");

    /*
     * لا يوجد PMIC معرف حالياً لهذه اللوحة،
     * لذلك لا نرسل أوامر إلى PMIC غير موجود.
     */

    goToDeepSleep();
}

// ============================================================
// DEEP SLEEP
// ============================================================

void goToDeepSleep() {

    Serial.println("Entering deep sleep...");

    delay(100);

    esp_sleep_enable_ext0_wakeup(
        (gpio_num_t)ES3C28P_BTN_PIN,
        ES3C28P_BTN_ACT == LOW ? 0 : 1
    );

    esp_deep_sleep_start();
}

// ============================================================
// REBOOT
// ============================================================

void checkReboot() {

    if (digitalRead(ES3C28P_BTN_PIN) == ES3C28P_BTN_ACT) {

        static uint32_t pressStart = 0;

        if (pressStart == 0) {
            pressStart = millis();
        }

        if (millis() - pressStart > 3000) {

            Serial.println("Rebooting...");

            delay(100);

            ESP.restart();
        }

    } else {

        static uint32_t pressStart = 0;
        pressStart = 0;
    }
}

// ============================================================
// CHARGING
// ============================================================

bool isCharging() {

    /*
     * لا يوجد PMIC / charger GPIO معرف حالياً.
     */
    return false;
}

// ============================================================
// TOUCH READING
// ============================================================

static bool readTouch(int &x, int &y) {

#if defined(HAS_TOUCH)

    if (!touchInitialized) {
        return false;
    }

    if (!ts.touched()) {
        return false;
    }

    TS_Point p = ts.getPoint();

    // --------------------------------------------------------
    // الضغط الضعيف / القراءة غير الصالحة
    // --------------------------------------------------------

    if (p.z < TOUCH_Z_THRESHOLD) {
        return false;
    }

    // --------------------------------------------------------
    // RAW -> SCREEN
    // --------------------------------------------------------

    int mappedX = map(
        p.x,
        TOUCH_MIN_X,
        TOUCH_MAX_X,
        0,
        tftWidth - 1
    );

    int mappedY = map(
        p.y,
        TOUCH_MIN_Y,
        TOUCH_MAX_Y,
        0,
        tftHeight - 1
    );

    // --------------------------------------------------------
    // LIMIT
    // --------------------------------------------------------

    mappedX = constrain(
        mappedX,
        0,
        tftWidth - 1
    );

    mappedY = constrain(
        mappedY,
        0,
        tftHeight - 1
    );

    // --------------------------------------------------------
    // SMOOTHING
    // --------------------------------------------------------

    if (lastTouchX < 0 || lastTouchY < 0) {

        x = mappedX;
        y = mappedY;

    } else {

        x =
            (lastTouchX * TOUCH_SMOOTH_NUM +
             mappedX) /
            (TOUCH_SMOOTH_NUM + 1);

        y =
            (lastTouchY * TOUCH_SMOOTH_NUM +
             mappedY) /
            (TOUCH_SMOOTH_NUM + 1);
    }

    lastTouchX = x;
    lastTouchY = y;

    return true;

#else

    return false;

#endif
}

// ============================================================
// INPUT HANDLER
// ============================================================

void InputHandler() {

    static uint32_t lastInputTime = 0;

    /*
     * لا نقرأ اللمس بسرعة كبيرة جداً.
     */
    if (millis() - lastInputTime < 30 && !LongPress) {
        return;
    }

    lastInputTime = millis();

    // ========================================================
    // TOUCH
    // ========================================================

#if defined(HAS_TOUCH)

    int x = 0;
    int y = 0;

    bool touching = readTouch(x, y);

    // --------------------------------------------------------
    // FINGER IS DOWN
    // --------------------------------------------------------

    if (touching) {

        /*
         * إذا الشاشة كانت مطفأة، أول لمسة فقط توقظ الشاشة.
         */
        if (!wakeUpScreen()) {

            AnyKeyPress = true;

        } else {

            // -----------------------------------------------
            // TOUCH POINT
            // -----------------------------------------------

            touchPoint.x = x;
            touchPoint.y = y;
            touchPoint.pressed = true;

            /*
             * touchHeatMap يعتمد على x/y لتوليد أزرار Bruce
             * الافتراضية.
             */
            touchHeatMap(touchPoint);

        }

        wasTouching = true;

    }

    // --------------------------------------------------------
    // FINGER RELEASED
    // --------------------------------------------------------

    else {

        if (wasTouching) {

            /*
             * نرسل حالة release إلى touchPoint.
             *
             * touchHeatMap نفسه لا يعتمد على pressed،
             * لذلك لا نستدعيه هنا حتى لا نولد ضغطة جديدة
             * بعد رفع الإصبع.
             */
            touchPoint.pressed = false;

            wasTouching = false;
        }

        /*
         * تصفير الإحداثيات السابقة حتى تبدأ اللمسة التالية
         * من قراءة جديدة بدون smoothing مع اللمسة السابقة.
         */
        lastTouchX = -1;
        lastTouchY = -1;
    }

#endif

    // ========================================================
    // PHYSICAL BUTTON
    // ========================================================

#if defined(HAS_BTN)

    if (digitalRead(ES3C28P_BTN_PIN) == ES3C28P_BTN_ACT) {

        if (!wakeUpScreen()) {

            AnyKeyPress = true;

        } else {

            SelPress = true;
            AnyKeyPress = true;
        }
    }

#endif

    // ========================================================
    // POWER SAVE
    // ========================================================

    checkPowerSaveTime();

    // ========================================================
    // REBOOT CHECK
    // ========================================================

    checkReboot();
}

// ============================================================
// INPUT TASK
// ============================================================

void taskInputHandler(void *arg) {

    (void)arg;

    while (true) {

        InputHandler();

        vTaskDelay(pdMS_TO_TICKS(30));
    }
}
