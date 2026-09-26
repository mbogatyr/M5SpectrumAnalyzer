#include <M5Unified.h>

#include "DisplayTimeout.h"
#include "Renderer.h"
#include "TrafficLight.h"

namespace {

constexpr uint8_t kBrightness = 120;

TrafficLight light;
Renderer renderer;
DisplayTimeout displayTimeout;

bool displayAwake = true;

void setDisplayAwake(bool awake) {
    if (awake == displayAwake) {
        return;
    }
    displayAwake = awake;

    if (awake) {
        M5.Display.wakeup();
        M5.Display.setBrightness(kBrightness);
        renderer.invalidate();
    } else {
        // Подсветка — главный потребитель, гасим её отдельно от
        // усыпления самой панели.
        M5.Display.setBrightness(0);
        M5.Display.sleep();
    }
}

} // namespace

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);

    M5.Display.setBrightness(kBrightness);
    renderer.begin();
    displayTimeout.begin(millis());
}

void loop() {
    M5.update();

    // millis() переполняется примерно через 49 суток; фаза при этом
    // один раз скакнёт, после чего цикл пойдёт дальше как обычно.
    const uint32_t now = millis();

    // Выключение питания здесь не обрабатывается: боковая кнопка
    // делает это сама двойным щелчком через PMIC.
    const bool activity = M5.BtnA.isPressed() || M5.BtnB.isPressed();
    setDisplayAwake(displayTimeout.shouldBeOn(now, activity));

    if (displayAwake) {
        renderer.draw(light.phaseAt(now), light.lampsAt(now),
                      light.secondsRemainingAt(now));
    }

    delay(20);
}
