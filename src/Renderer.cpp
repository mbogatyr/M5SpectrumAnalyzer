#include "Renderer.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

namespace {

// --- color ---

struct Rgb {
    uint8_t r, g, b;
};

uint16_t to565(M5Canvas &canvas, const Rgb &c) {
    return canvas.color565(c.r, c.g, c.b);
}

constexpr Rgb kGreen{40, 220, 90};
constexpr Rgb kYellow{255, 196, 0};
constexpr Rgb kRed{255, 59, 48};
constexpr Rgb kCyan{58, 214, 255};
constexpr Rgb kAmber{255, 176, 0};
constexpr Rgb kText{255, 255, 255};
constexpr Rgb kMuted{154, 154, 154};
constexpr Rgb kDim{74, 74, 74};      // digits during silence
constexpr Rgb kCapColor{216, 216, 216};
constexpr Rgb kScaleLine{85, 85, 85};
constexpr Rgb kScaleTick{102, 102, 102};
constexpr Rgb kScaleCenter{153, 153, 153};
constexpr Rgb kInTuneZone{18, 54, 29};
constexpr Rgb kAxisTick{85, 85, 85};
constexpr Rgb kAxisLabel{138, 138, 138};

// Landscape orientations: in kRotation the image is upright when KEY1 is
// to the right of the screen (verified on the board); kFlippedRotation is
// the same image rotated 180°.
constexpr uint8_t kRotation = 1;
constexpr uint8_t kFlippedRotation = 3;

// --- geometry for the 240x135 screen on its side ---

constexpr int kReadoutX = 5;
constexpr int kReadoutBaseline = 28;

constexpr int kTunerLeft = 134;
constexpr int kTunerRight = 234;
constexpr int kNoteBaseline = 14;
constexpr int kScaleY = 27;
constexpr int kInTuneCents = 5;  // green zone and green color
constexpr int kCloseCents = 20;  // yellow up to this, red beyond

constexpr int kBarsTop = 42;
constexpr int kBarsBottom = 122;
constexpr int kBarsHeight = kBarsBottom - kBarsTop;
constexpr int kSegmentH = 2;     // one "LED" of a bar
constexpr int kSegmentPitch = 3; // segment plus gap

constexpr int kAxisTickY = 123;
constexpr int kAxisBaseline = 133;

const Rgb &colorForCents(int cents) {
    const int deviation = abs(cents);
    if (deviation <= kInTuneCents) {
        return kGreen;
    }
    return (deviation <= kCloseCents) ? kYellow : kRed;
}

} // namespace

void Renderer::begin(const AnalyzerConfig &config) {
    minHz_ = config.minHz;
    maxHz_ = config.maxHz;
    bandCount_ = config.bandCount;

    M5.Display.setRotation(kRotation); // on its side: 240 wide, 135 tall
    M5.Display.fillScreen(TFT_BLACK);

    canvas_.setColorDepth(16);
    canvas_.setPsram(true); // 240*135*2 = 65 KB; StickS3 has 8 MB of PSRAM
    canvas_.createSprite(M5.Display.width(), M5.Display.height());
}

void Renderer::draw(const BarBallistics &bars, const Spectrum &spectrum,
                    const Readout &readout, float bottomDb, bool frozen) {
    canvas_.fillSprite(TFT_BLACK);

    paintReadout(readout);
    paintTuner(readout);
    paintBars(bars, bottomDb);
    if (spectrum.hasPeak) {
        paintPeakMarker(spectrum);
    }
    paintAxis();
    if (frozen) {
        paintHold();
    }

    canvas_.pushSprite(0, 0);
}

bool Renderer::setFlipped(bool flipped) {
    if (flipped == flipped_) {
        return false;
    }
    flipped_ = flipped;
    M5.Display.setRotation(flipped ? kFlippedRotation : kRotation);
    return true;
}

int Renderer::xForHz(float hz) const {
    return static_cast<int>(canvas_.width() * logf(hz / minHz_) /
                            logf(maxHz_ / minHz_));
}

// Large frequency at the top left, followed by a small "Hz".
void Renderer::paintReadout(const Readout &readout) {
    const char *text = readout.valid ? readout.hz : "---.-";

    canvas_.setTextDatum(textdatum_t::baseline_left);
    canvas_.setFont(&fonts::FreeSansBold18pt7b);
    canvas_.setTextColor(to565(canvas_, readout.valid ? kText : kDim));
    canvas_.drawString(text, kReadoutX, kReadoutBaseline);
    const int width = canvas_.textWidth(text);

    canvas_.setFont(&fonts::FreeSans9pt7b);
    canvas_.setTextColor(to565(canvas_, readout.valid ? kMuted : kDim));
    canvas_.drawString("Hz", kReadoutX + width + 4, kReadoutBaseline);
}

// Note, cents and a ±50-cent scale with a needle, at the top right.
void Renderer::paintTuner(const Readout &readout) {
    const int center = (kTunerLeft + kTunerRight) / 2;
    const int halfWidth = (kTunerRight - kTunerLeft) / 2;
    auto xForCents = [&](int cents) { return center + cents * halfWidth / 50; };

    char note[8] = "--";
    if (readout.valid) {
        snprintf(note, sizeof note, "%s%d", readout.note.name,
                 readout.note.octave);
    }
    canvas_.setTextDatum(textdatum_t::baseline_left);
    canvas_.setFont(&fonts::FreeSansBold9pt7b);
    canvas_.setTextColor(to565(canvas_, readout.valid ? kText : kDim));
    canvas_.drawString(note, kTunerLeft, kNoteBaseline);

    if (readout.valid) {
        char cents[12];
        snprintf(cents, sizeof cents, "%+d ct", readout.note.cents);
        canvas_.setTextDatum(textdatum_t::baseline_right);
        canvas_.setFont(&fonts::Font2);
        canvas_.setTextColor(to565(canvas_, colorForCents(readout.note.cents)));
        canvas_.drawString(cents, kTunerRight, kNoteBaseline);
    }

    const int zoneLeft = xForCents(-kInTuneCents);
    canvas_.fillRect(zoneLeft, kScaleY - 4, xForCents(kInTuneCents) - zoneLeft,
                     8, to565(canvas_, kInTuneZone));
    canvas_.drawFastHLine(kTunerLeft, kScaleY, kTunerRight - kTunerLeft + 1,
                          to565(canvas_, kScaleLine));
    for (int cents = -50; cents <= 50; cents += 25) {
        const int h = (cents == 0) ? 8 : 4;
        canvas_.drawFastVLine(xForCents(cents), kScaleY - h / 2, h,
                              to565(canvas_, cents == 0 ? kScaleCenter
                                                        : kScaleTick));
    }

    if (readout.valid) {
        canvas_.fillRect(xForCents(readout.note.cents) - 1, kScaleY - 8, 3, 15,
                         to565(canvas_, colorForCents(readout.note.cents)));
    }
}

// Bars made of "LEDs": green at the bottom, yellow above 60% of the scale,
// red above 85%. Each bar has a peak cap above it.
void Renderer::paintBars(const BarBallistics &bars, float bottomDb) {
    const uint16_t green = to565(canvas_, kGreen);
    const uint16_t yellow = to565(canvas_, kYellow);
    const uint16_t red = to565(canvas_, kRed);
    const uint16_t capColor = to565(canvas_, kCapColor);

    const int pitch = canvas_.width() / static_cast<int>(bandCount_);
    const int barW = pitch - 1;

    auto heightFor = [&](float db) {
        const float fraction = (db - bottomDb) / kRangeDb;
        if (fraction <= 0.0f) {
            return 0;
        }
        return (fraction >= 1.0f) ? kBarsHeight
                                  : static_cast<int>(fraction * kBarsHeight);
    };

    for (size_t i = 0; i < bandCount_; ++i) {
        const int x = static_cast<int>(i) * pitch;
        const int h = heightFor(bars.level(i));

        for (int y = 0; y + kSegmentH <= h; y += kSegmentPitch) {
            const int percent = y * 100 / kBarsHeight;
            const uint16_t color =
                (percent < 60) ? green : (percent < 85) ? yellow : red;
            canvas_.fillRect(x, kBarsBottom - y - kSegmentH, barW, kSegmentH,
                             color);
        }

        const int cap = heightFor(bars.cap(i));
        if (cap > 0) {
            canvas_.drawFastHLine(x, kBarsBottom - cap, barW, capColor);
        }
    }
}

// Cyan triangle above the band with the strongest peak.
void Renderer::paintPeakMarker(const Spectrum &spectrum) {
    const int pitch = canvas_.width() / static_cast<int>(bandCount_);
    const int x = static_cast<int>(spectrum.peakBand) * pitch;
    canvas_.fillTriangle(x - 1, kBarsTop - 5, x + pitch, kBarsTop - 5,
                         x + pitch / 2, kBarsTop - 1, to565(canvas_, kCyan));
}

void Renderer::paintAxis() {
    const uint16_t tick = to565(canvas_, kAxisTick);
    canvas_.setFont(&fonts::Font0);
    canvas_.setTextColor(to565(canvas_, kAxisLabel));

    canvas_.setTextDatum(textdatum_t::baseline_left);
    canvas_.drawString("50", 1, kAxisBaseline);
    canvas_.setTextDatum(textdatum_t::baseline_right);
    canvas_.drawString("8k", canvas_.width() - 1, kAxisBaseline);

    canvas_.setTextDatum(textdatum_t::baseline_center);
    const struct {
        float hz;
        const char *label;
    } kMarks[] = {{100.0f, "100"}, {1000.0f, "1k"}, {5000.0f, "5k"}};
    for (const auto &mark : kMarks) {
        const int x = xForHz(mark.hz);
        canvas_.drawFastVLine(x, kAxisTickY, 3, tick);
        canvas_.drawString(mark.label, x, kAxisBaseline);
    }
}

// Badge in the top-right corner of the spectrum while the frame is frozen.
void Renderer::paintHold() {
    const int w = 32;
    const int x = canvas_.width() - w - 2;
    canvas_.fillRoundRect(x, kBarsTop + 1, w, 11, 2, to565(canvas_, kAmber));
    canvas_.setFont(&fonts::Font0);
    canvas_.setTextDatum(textdatum_t::middle_center);
    canvas_.setTextColor(TFT_BLACK);
    canvas_.drawString("HOLD", x + w / 2, kBarsTop + 7);
}
