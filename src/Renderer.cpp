#include "Renderer.h"

#include "SevenSegment.h"

namespace {

// --- цвет ---

struct Rgb {
    uint8_t r, g, b;
};

Rgb mix(const Rgb &a, const Rgb &b, float t) {
    if (t < 0.0f) {
        t = 0.0f;
    } else if (t > 1.0f) {
        t = 1.0f;
    }
    return Rgb{static_cast<uint8_t>(a.r + (b.r - a.r) * t),
               static_cast<uint8_t>(a.g + (b.g - a.g) * t),
               static_cast<uint8_t>(a.b + (b.b - a.b) * t)};
}

uint16_t to565(M5Canvas &canvas, const Rgb &c) {
    return canvas.color565(c.r, c.g, c.b);
}

// Ядро, середина, край, погашенное стекло и цвет ореола одной лампы.
struct LampPalette {
    Rgb core, mid, edge, dark, glow;
};

constexpr LampPalette kRedLamp{
    {255, 148, 138}, {255, 59, 48}, {176, 26, 18}, {46, 16, 14}, {255, 59, 48}};
constexpr LampPalette kYellowLamp{
    {255, 236, 168}, {255, 196, 0}, {178, 128, 0}, {44, 36, 10}, {255, 196, 0}};
constexpr LampPalette kGreenLamp{
    {180, 255, 200}, {40, 220, 90}, {16, 134, 56}, {12, 44, 24}, {40, 220, 90}};

constexpr Rgb kHousing{26, 27, 32};
constexpr Rgb kHousingHighlight{62, 64, 74};
constexpr Rgb kHousingShadow{10, 10, 13};
constexpr Rgb kVisorColor{16, 17, 21};
constexpr Rgb kBlack{0, 0, 0};
constexpr Rgb kWhite{255, 255, 255};
constexpr Rgb kGlassReflection{120, 130, 145};

// --- геометрия под портретный экран 135x240 ---

constexpr int kHousingX = 19;
constexpr int kHousingW = 97;
constexpr int kHousingY = 4;
constexpr int kHousingH = 192;
constexpr int kHousingRadius = 15;

constexpr int kSlotH = 64;
constexpr int kLampR = 22;
constexpr int kVisorH = 10;
constexpr int kVisorOverhang = 6; // насколько козырёк шире лампы

constexpr int kGlowRings = 9;

constexpr int kCountdownCenterY = 219;
constexpr int kDigitW = 22;
constexpr int kDigitH = 37;
constexpr int kSegThickness = 5;
constexpr int kDigitGap = 5;

int lampTop(int index) { return kHousingY + 4 + index * kSlotH; }

int lampCenterY(int index) { return lampTop(index) + kVisorH + kLampR + 2; }

// --- примитивы светофора ---
//
// В 16-битном спрайте нет прозрачности, поэтому градиенты и ореолы
// собраны из концентрических кругов с заранее посчитанным цветом:
// смешивание идёт с заведомо известным фоном на этапе рисования.

// Ореол вокруг горящей лампы. Рисуется до козырька, чтобы тот лёг сверху.
void paintGlow(M5Canvas &canvas, int cx, int cy, const LampPalette &p) {
    for (int k = kGlowRings; k >= 1; --k) {
        const float strength =
            0.30f * (1.0f - static_cast<float>(k) / (kGlowRings + 1));
        canvas.fillCircle(cx, cy, kLampR + k,
                          to565(canvas, mix(kHousing, p.glow, strength)));
    }
}

// Козырёк — трапеция из двух треугольников: снизу шире, сверху со скосом.
void paintVisor(M5Canvas &canvas, int cx, int top) {
    const uint16_t color = to565(canvas, kVisorColor);
    const int halfW = kLampR + kVisorOverhang;
    const int bottom = top + kVisorH;

    canvas.fillTriangle(cx - halfW, bottom, cx - halfW + 3, top,
                        cx + halfW - 3, top, color);
    canvas.fillTriangle(cx - halfW, bottom, cx + halfW - 3, top, cx + halfW,
                        bottom, color);
}

void paintLamp(M5Canvas &canvas, int cx, int cy, const LampPalette &p,
               bool lit) {
    canvas.fillCircle(cx, cy, kLampR + 2, to565(canvas, kHousingShadow));

    for (int r = kLampR; r >= 1; --r) {
        const float t = static_cast<float>(r) / kLampR;
        const Rgb color =
            lit ? ((t > 0.55f) ? mix(p.mid, p.edge, (t - 0.55f) / 0.45f)
                               : mix(p.core, p.mid, t / 0.55f))
                : mix(mix(p.dark, kBlack, 0.35f), p.dark, 1.0f - t);
        canvas.fillCircle(cx, cy, r, to565(canvas, color));
    }

    // Блик: у горящей лампы яркий, у погашенной — тусклое отражение неба.
    if (lit) {
        canvas.fillCircle(cx - 7, cy - 8, 5,
                          to565(canvas, mix(p.core, kWhite, 0.55f)));
        canvas.fillCircle(cx - 8, cy - 9, 2, TFT_WHITE);
    } else {
        canvas.fillCircle(cx - 7, cy - 8, 4,
                          to565(canvas, mix(p.dark, kGlassReflection, 0.35f)));
    }
}

void paintDigit(M5Canvas &canvas, int x, int y, uint8_t value, uint16_t color) {
    const uint8_t mask = segmentsForDigit(value);
    const int t = kSegThickness;
    const int half = (kDigitH - 3 * t) / 2;
    const int innerW = kDigitW - 2 * t;

    if (mask & SegA) canvas.fillRect(x + t, y, innerW, t, color);
    if (mask & SegF) canvas.fillRect(x, y + t, t, half, color);
    if (mask & SegB) canvas.fillRect(x + kDigitW - t, y + t, t, half, color);
    if (mask & SegG) canvas.fillRect(x + t, y + t + half, innerW, t, color);
    if (mask & SegE) canvas.fillRect(x, y + 2 * t + half, t, half, color);
    if (mask & SegC)
        canvas.fillRect(x + kDigitW - t, y + 2 * t + half, t, half, color);
    if (mask & SegD) canvas.fillRect(x + t, y + kDigitH - t, innerW, t, color);
}

void paintCountdown(M5Canvas &canvas, uint8_t seconds, uint16_t color) {
    const int digits = (seconds >= 10) ? 2 : 1;
    const int totalW = digits * kDigitW + (digits - 1) * kDigitGap;
    const int y = kCountdownCenterY - kDigitH / 2;
    int x = (canvas.width() - totalW) / 2;

    if (digits == 2) {
        paintDigit(canvas, x, y, static_cast<uint8_t>(seconds / 10), color);
        x += kDigitW + kDigitGap;
    }
    paintDigit(canvas, x, y, static_cast<uint8_t>(seconds % 10), color);
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
    const int centerX = canvas_.width() / 2;

    canvas_.fillSprite(TFT_BLACK);

    // тень под корпусом, сам корпус, фаска по верхней кромке
    canvas_.fillRoundRect(kHousingX + 2, kHousingY + 3, kHousingW, kHousingH,
                          kHousingRadius, to565(canvas_, kHousingShadow));
    canvas_.fillRoundRect(kHousingX, kHousingY, kHousingW, kHousingH,
                          kHousingRadius, to565(canvas_, kHousing));
    canvas_.fillRect(kHousingX + 4, kHousingY + 2, kHousingW - 8, 1,
                     to565(canvas_, kHousingHighlight));

    const LampPalette *palettes[3] = {&kRedLamp, &kYellowLamp, &kGreenLamp};
    const bool isLit[3] = {lamps.red, lamps.yellow, lamps.green};

    for (int i = 0; i < 3; ++i) {
        const int centerY = lampCenterY(i);

        if (isLit[i]) {
            paintGlow(canvas_, centerX, centerY, *palettes[i]);
        }
        paintVisor(canvas_, centerX, lampTop(i));
        paintLamp(canvas_, centerX, centerY, *palettes[i], isLit[i]);
    }

    // Цифры окрашены по фазе, а не по лампам: иначе на погашенном такте
    // мигающего зелёного они меняли бы цвет.
    const LampPalette *accent = &kRedLamp;
    switch (phase) {
    case Phase::Red:
        accent = &kRedLamp;
        break;
    case Phase::RedYellow:
    case Phase::Yellow:
        accent = &kYellowLamp;
        break;
    case Phase::Green:
    case Phase::GreenBlink:
        accent = &kGreenLamp;
        break;
    }
    paintCountdown(canvas_, secondsRemaining, to565(canvas_, accent->mid));

    canvas_.pushSprite(0, 0);
}
