#include <M5Unified.h>

#include "AudioInput.h"
#include "BarBallistics.h"
#include "DisplayTimeout.h"
#include "FrequencyReadout.h"
#include "Renderer.h"
#include "SpectrumAnalyzer.h"

namespace {

constexpr uint8_t kBrightness = 120;

// Цифры в шапке обновляются 5 раз в секунду, спектр — каждый кадр.
constexpr uint32_t kReadoutPeriodMs = 200;

// Нижний край шкалы столбиков для каждой ступени чувствительности.
// Замерено на плате: тихая комната даёт -83 дБ в средних полосах,
// тон из динамика ноутбука рядом — от -17 до -28 дБ. С нижней ступенью
// тишина лежит на дне, а громкий тон не упирается в потолок (-20 дБ).
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

const Spectrum *latest = nullptr;
Readout readout;
uint32_t lastReadoutMs = 0;

bool displayAwake = true;
bool frozen = false;
bool needsPaint = false;
size_t sensitivity = 0;

// Раз в секунду строка в Serial: помогает подбирать пороги и следить,
// укладывается ли кадр в 32 мс.
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
    } else {
        // Подсветка — главный потребитель, гасим её отдельно от
        // усыпления самой панели.
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
    // На паузе анализа нет, с погашенным экраном нет отрисовки, поэтому
    // средние считаются каждое по своему счётчику.
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
    // Динамик сидит на тех же линиях I2S, что и микрофон.
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

    // Нажатие на погашенном экране только будит его.
    if (displayAwake) {
        if (M5.BtnA.wasPressed()) {
            frozen = !frozen;
            // Цифры обновляются раз в 200 мс и могли отстать от спектра;
            // на замороженном кадре они должны описывать именно его.
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

    // Микрофон опрашивается всегда, даже на паузе и с погашенным экраном:
    // иначе очередь опустеет и в захвате появится дыра.
    const bool fresh = audio.poll();
    bool signal = false;

    if (fresh) {
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

    // Выключение питания здесь не обрабатывается: боковая кнопка
    // делает это сама двойным щелчком через PMIC.
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
