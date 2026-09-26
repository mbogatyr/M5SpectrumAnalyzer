#include "Renderer.h"

namespace {

// Геометрия под портретный экран 135x240.
constexpr int kHousingTop = 8;
constexpr int kHousingHeight = 170;
constexpr int kHousingWidth = 96;
constexpr int kLampRadius = 23;
constexpr int kLampGap = 8;
constexpr int kCountdownCenterY = 209;

int lampCenterY(int index) {
    return kHousingTop + kLampGap + kLampRadius +
           index * (2 * kLampRadius + kLampGap);
}

} // namespace

void Renderer::begin() {
    M5.Display.setRotation(0); // портрет: 135 в ширину, 240 в высоту
    M5.Display.fillScreen(TFT_BLACK);

    canvas_.setColorDepth(16);
    canvas_.setPsram(true); // 135*240*2 = 65 КБ, на StickS3 есть 8 МБ PSRAM
    canvas_.createSprite(M5.Display.width(), M5.Display.height());
}

void Renderer::invalidate() { hasPrevious_ = false; }

void Renderer::draw(Phase phase, const Lamps &lamps, uint8_t secondsRemaining) {
    const bool unchanged = hasPrevious_ && previousPhase_ == phase &&
                           previousLamps_.red == lamps.red &&
                           previousLamps_.yellow == lamps.yellow &&
                           previousLamps_.green == lamps.green &&
                           previousSeconds_ == secondsRemaining;
    if (unchanged) {
        return;
    }

    paint(phase, lamps, secondsRemaining);

    hasPrevious_ = true;
    previousPhase_ = phase;
    previousLamps_ = lamps;
    previousSeconds_ = secondsRemaining;
}

void Renderer::paint(Phase phase, const Lamps &lamps, uint8_t secondsRemaining) {
    const uint16_t redOn = canvas_.color565(255, 59, 48);
    const uint16_t yellowOn = canvas_.color565(255, 196, 0);
    const uint16_t greenOn = canvas_.color565(40, 220, 90);

    const uint16_t redOff = canvas_.color565(48, 12, 10);
    const uint16_t yellowOff = canvas_.color565(48, 38, 0);
    const uint16_t greenOff = canvas_.color565(8, 44, 20);

    const int centerX = canvas_.width() / 2;

    canvas_.fillSprite(TFT_BLACK);

    // Корпус светофора
    canvas_.fillRoundRect(centerX - kHousingWidth / 2, kHousingTop,
                          kHousingWidth, kHousingHeight, 16,
                          canvas_.color565(26, 26, 30));

    const uint16_t lampColors[3] = {
        lamps.red ? redOn : redOff,
        lamps.yellow ? yellowOn : yellowOff,
        lamps.green ? greenOn : greenOff,
    };

    for (int i = 0; i < 3; ++i) {
        canvas_.fillCircle(centerX, lampCenterY(i), kLampRadius, lampColors[i]);
    }

    // Цифры отсчёта окрашены по фазе, а не по лампам: иначе на
    // погашенном такте мигающего зелёного они меняли бы цвет.
    uint16_t countdownColor = redOn;
    switch (phase) {
    case Phase::Red:
        countdownColor = redOn;
        break;
    case Phase::RedYellow:
    case Phase::Yellow:
        countdownColor = yellowOn;
        break;
    case Phase::Green:
    case Phase::GreenBlink:
        countdownColor = greenOn;
        break;
    }

    canvas_.setFont(&fonts::Font7);
    canvas_.setTextDatum(middle_center);
    canvas_.setTextColor(countdownColor, TFT_BLACK);
    canvas_.drawNumber(secondsRemaining, centerX, kCountdownCenterY);

    canvas_.pushSprite(0, 0);
}
