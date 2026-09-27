#include <M5Unified.h>

#include "AudioInput.h"
#include "BarBallistics.h"
#include "DisplayTimeout.h"
#include "FrequencyReadout.h"
#include "Orientation.h"
#include "Renderer.h"
#include "SpectrumAnalyzer.h"

namespace {

constexpr uint8_t kBrightness = 120;

// The header digits update 5 times a second, the spectrum every frame.
constexpr uint32_t kReadoutPeriodMs = 200;

// Bottom edge of the bar scale for each sensitivity step.
// Measured on the board: a quiet room gives -83 dB in the mid bands; a tone
// from a nearby laptop speaker gives -17 to -28 dB. At the least sensitive
// step, silence sits on the floor and such a tone reaches the red zone under
// the -20 dB ceiling, touching it only at its loudest.
constexpr float kBottomDb[] = {-80.0f, -92.0f, -104.0f};
constexpr size_t kSensitivitySteps = sizeof kBottomDb / sizeof kBottomDb[0];

AnalyzerConfig makeAnalyzerConfig() {
    AnalyzerConfig config;
    config.sampleRate = AudioInput::kSampleRate;
    config.fftSize = AudioInput::kWindowLen;
    return config;
}

AudioInput audio;
SpectrumAnalyzer analyzer(makeAnalyzerConfig());
BarBallistics bars(analyzer.config().bandCount, SpectrumAnalyzer::kFloorDb);
Renderer renderer;
DisplayTimeout displayTimeout;
Orientation orientation;

const Spectrum *latest = nullptr;
Readout readout;
uint32_t lastReadoutMs = 0;

bool displayAwake = true;
bool frozen = false;
bool needsPaint = false;
size_t sensitivity = 0;

// A line to Serial once a second: helps tune thresholds and check that a
// frame fits within 32 ms.
struct Stats {
    uint32_t sinceMs = 0;
    uint32_t frames = 0;
    uint32_t analyses = 0;
    uint32_t analyzeUs = 0;
    uint32_t drawUs = 0;
} stats;

void setDisplayAwake(bool awake) {
    if (awake == displayAwake) {
        return;
    }
    displayAwake = awake;

    if (awake) {
        M5.Display.wakeup();
        M5.Display.setBrightness(kBrightness);
        needsPaint = true;
        // The device may have been turned over while the screen was asleep.
        orientation.reset();
    } else {
        // The backlight is the main power draw, so turn it off separately
        // from putting the panel itself to sleep.
        M5.Display.setBrightness(0);
        M5.Display.sleep();
    }
}

Readout makeReadout(const Spectrum &spectrum) {
    Readout r;
    r.valid = spectrum.hasPeak;
    if (r.valid) {
        formatHz(spectrum.peakHz, r.hz, sizeof r.hz);
        r.note = noteFor(spectrum.peakHz);
    }
    return r;
}

void paint() {
    const uint32_t start = micros();
    renderer.draw(bars, *latest, readout, kBottomDb[sensitivity], frozen);
    stats.drawUs += micros() - start;
    ++stats.frames;
}

void reportStats(uint32_t now) {
    if (now - stats.sinceMs < 1000) {
        return;
    }
    // There is no analysis while paused and no drawing while the screen is
    // off, so each average is divided by its own counter.
    const uint32_t analyses = stats.analyses ? stats.analyses : 1;
    const uint32_t frames = stats.frames ? stats.frames : 1;
    Serial.printf("fps %u  analyze %.1f ms  draw %.1f ms  ",
                  static_cast<unsigned>(stats.frames),
                  stats.analyzeUs / 1000.0f / analyses,
                  stats.drawUs / 1000.0f / frames);
    if (latest && latest->hasPeak) {
        Serial.printf("peak %.1f Hz %.1f dB\n", latest->peakHz, latest->peakDb);
    } else {
        Serial.println("peak --");
    }
    stats = Stats{};
    stats.sinceMs = now;
}

} // namespace

void setup() {
    auto cfg = M5.config();
    // The speaker sits on the same I2S lines as the mic.
    cfg.internal_spk = false;
    M5.begin(cfg);
    Serial.begin(115200);

    M5.Display.setBrightness(kBrightness);
    renderer.begin(analyzer.config());

    if (!audio.begin()) {
        M5.Display.setTextDatum(textdatum_t::middle_center);
        M5.Display.drawString("no microphone", M5.Display.width() / 2,
                              M5.Display.height() / 2);
        for (;;) {
            delay(1000);
        }
    }

    displayTimeout.begin(millis());
}

void loop() {
    M5.update();

    const uint32_t now = millis();

    // A press while the screen is off only wakes it.
    if (displayAwake) {
        if (M5.BtnA.wasPressed()) {
            frozen = !frozen;
            // The digits update every 200 ms and may lag behind the
            // spectrum; on a frozen frame they must describe that very frame.
            if (frozen && latest) {
                readout = makeReadout(*latest);
            }
            needsPaint = true;
        }
        if (M5.BtnB.wasPressed()) {
            sensitivity = (sensitivity + 1) % kSensitivitySteps;
            needsPaint = true;
        }
    }

    // The mic is always polled, even while paused or with the screen off:
    // otherwise the queue runs dry and the capture gets a gap.
    const bool fresh = audio.poll();
    bool signal = false;

    if (fresh) {
        // Read the accelerometer once per analysis frame, about 31 times a
        // second: loop() runs almost every millisecond, and Orientation
        // needs a new position to hold for 400 ms anyway. Also while
        // paused: the frozen frame gets redrawn flipped.
        float ax, ay, az;
        if (displayAwake && M5.Imu.getAccel(&ax, &ay, &az)) {
            if (renderer.setFlipped(orientation.update(now, ax, ay, az))) {
                needsPaint = true;
            }
        }

        const int16_t *window = audio.window();
        if (!frozen) {
            const uint32_t start = micros();
            latest = &analyzer.analyze(window);
            bars.update(latest->bandDb.data(), now);
            stats.analyzeUs += micros() - start;
            ++stats.analyses;

            signal = latest->hasPeak;
            if (now - lastReadoutMs >= kReadoutPeriodMs) {
                lastReadoutMs = now;
                readout = makeReadout(*latest);
            }
            needsPaint = true;
        }
    }

    // Power-off is not handled here: a double press of the side button
    // does it on its own through the PMIC.
    const bool buttons = M5.BtnA.isPressed() || M5.BtnB.isPressed();
    setDisplayAwake(displayTimeout.shouldBeOn(now, buttons || signal));

    if (displayAwake && needsPaint && latest) {
        paint();
        needsPaint = false;
    }

    reportStats(now);

    if (!fresh) {
        delay(1);
    }
}
