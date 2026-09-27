#include "SpectrumAnalyzer.h"

#include <math.h>

#include <algorithm>

namespace {

const double kTwoPi = 6.283185307179586;

// Квадрат модуля ниже этого порога даёт kFloorDb и не уходит в -inf.
const float kTinyPower = 1e-30f;

} // namespace

SpectrumAnalyzer::SpectrumAnalyzer(const AnalyzerConfig &config)
    : config_(config), fft_(config.fftSize),
      binHz_(static_cast<float>(config.sampleRate) / config.fftSize),
      window_(config.fftSize), re_(config.fftSize), im_(config.fftSize),
      binDb_(config.fftSize / 2 + 1), bands_(config.bandCount) {
    const size_t n = config.fftSize;

    for (size_t i = 0; i < n; ++i) {
        window_[i] = static_cast<float>(0.5 - 0.5 * cos(kTwoPi * i / n));
    }

    // Синус амплитуды A после окна Ханна (усиление 0.5) даёт в своём бине
    // модуль A*N/4. Отсчёты уже поделены на 32768, так что полная шкала
    // превращается в N/4, и её надо вычесть, чтобы получить 0 дБ.
    dbOffset_ = static_cast<float>(-20.0 * log10(n / 4.0));

    // Параболе нужны соседи с обеих сторон, а поиск вершины заходит на
    // бин ниже minBin_, поэтому minBin_ не меньше 2.
    minBin_ = static_cast<size_t>(ceilf(config.minHz / binHz_));
    if (minBin_ < 2) {
        minBin_ = 2;
    }
    maxBin_ = static_cast<size_t>(floorf(config.maxHz / binHz_));
    if (maxBin_ > n / 2 - 1) {
        maxBin_ = n / 2 - 1;
    }

    // Полоса i охватывает частоты [e_i, e_{i+1}), где e_i растут
    // геометрически от minHz до maxHz. В неё входят бины, чья частота
    // попала в этот промежуток.
    const double ratio = static_cast<double>(config.maxHz) / config.minHz;
    for (size_t i = 0; i < config.bandCount; ++i) {
        const double lo =
            config.minHz * pow(ratio, static_cast<double>(i) / config.bandCount);
        const double hi = config.minHz *
                          pow(ratio, static_cast<double>(i + 1) / config.bandCount);
        size_t first = static_cast<size_t>(ceil(lo / binHz_));
        size_t last = static_cast<size_t>(ceil(hi / binHz_)) - 1;

        // На низах полоса бывает уже шага бина и не содержит ни одного.
        // Тогда она показывает ближайший к своей середине бин, иначе в
        // столбиках появились бы вечные провалы.
        if (first > last) {
            first = last = static_cast<size_t>(lround(sqrt(lo * hi) / binHz_));
        }
        if (last > n / 2) {
            last = n / 2;
        }
        bands_[i] = BandBins{first, last};
    }

    spectrum_.bandDb.assign(config.bandCount, kFloorDb);
}

const Spectrum &SpectrumAnalyzer::analyze(const int16_t *samples) {
    const size_t n = config_.fftSize;

    for (size_t i = 0; i < n; ++i) {
        re_[i] = samples[i] * (1.0f / 32768.0f) * window_[i];
        im_[i] = 0.0f;
    }

    fft_.transform(re_.data(), im_.data());

    for (size_t k = 0; k <= n / 2; ++k) {
        const float power = re_[k] * re_[k] + im_[k] * im_[k];
        binDb_[k] = (power < kTinyPower)
                        ? kFloorDb
                        : 10.0f * log10f(power) + dbOffset_;
        if (binDb_[k] < kFloorDb) {
            binDb_[k] = kFloorDb;
        }
    }

    findPeak();
    fillBands();
    return spectrum_;
}

void SpectrumAnalyzer::findPeak() {
    // Пик — самая высокая вершина, а не самый громкий бин: у края
    // диапазона самым громким может оказаться склон тона, который лежит
    // снаружи, и парабола через склон даёт выдуманную частоту. Поиск
    // начинается на бин раньше minBin_, чтобы тон у самой границы, чья
    // вершина приходится на соседний бин, не потерялся.
    bool found = false;
    size_t k = minBin_;
    for (size_t i = minBin_ - 1; i <= maxBin_; ++i) {
        const bool isSummit =
            binDb_[i] >= binDb_[i - 1] && binDb_[i] >= binDb_[i + 1];
        if (isSummit && (!found || binDb_[i] > binDb_[k])) {
            k = i;
            found = true;
        }
    }

    // Парабола через три точки в децибелах. На вершине сдвиг не выходит
    // за ±0.5 бина. Для окна Ханна систематическая ошибка такой оценки —
    // сотые доли бина, то есть доли герца.
    const float a = binDb_[k - 1];
    const float b = binDb_[k];
    const float c = binDb_[k + 1];
    const float denominator = a - 2.0f * b + c;
    const float delta = (denominator != 0.0f) ? 0.5f * (a - c) / denominator : 0.0f;

    spectrum_.peakHz = (static_cast<float>(k) + delta) * binHz_;
    spectrum_.peakDb = b - 0.25f * (a - c) * delta;
    spectrum_.hasPeak = found && spectrum_.peakHz >= config_.minHz &&
                        spectrum_.peakHz <= config_.maxHz &&
                        spectrum_.peakDb >= config_.silenceDb &&
                        prominenceDb(k) >= config_.minProminenceDb;
    spectrum_.peakBand = bandForHz(spectrum_.peakHz);
}

// Высота пика над медианой бинов, отстоящих от него на 4-12 бинов.
// Ближе не берём: главный лепесток окна Ханна занимает ±2 бина и ещё
// немного на отстройку. Дальше тоже: при основном тоне от 110 Гц (14
// бинов) туда попали бы соседние гармоники голоса. Медиана, а не
// среднее, чтобы одна такая гармоника всё же не подняла фон.
float SpectrumAnalyzer::prominenceDb(size_t peakBin) const {
    const size_t kNear = 4;
    const size_t kFar = 12;
    const size_t lastBin = config_.fftSize / 2;

    float neighbors[2 * (kFar - kNear + 1)];
    size_t count = 0;
    for (size_t d = kNear; d <= kFar; ++d) {
        if (peakBin >= 1 + d) {
            neighbors[count++] = binDb_[peakBin - d];
        }
        if (peakBin + d <= lastBin) {
            neighbors[count++] = binDb_[peakBin + d];
        }
    }

    std::nth_element(neighbors, neighbors + count / 2, neighbors + count);
    return binDb_[peakBin] - neighbors[count / 2];
}

void SpectrumAnalyzer::fillBands() {
    for (size_t i = 0; i < bands_.size(); ++i) {
        float loudest = kFloorDb;
        for (size_t k = bands_[i].first; k <= bands_[i].last; ++k) {
            if (binDb_[k] > loudest) {
                loudest = binDb_[k];
            }
        }
        spectrum_.bandDb[i] = loudest;
    }
}

size_t SpectrumAnalyzer::bandForHz(float hz) const {
    if (hz <= config_.minHz) {
        return 0;
    }
    const float position = config_.bandCount * logf(hz / config_.minHz) /
                           logf(config_.maxHz / config_.minHz);
    const size_t band = static_cast<size_t>(position);
    return (band < config_.bandCount) ? band : config_.bandCount - 1;
}
