#include "SpectrumAnalyzer.h"

#include <math.h>

#include <algorithm>

namespace {

const double kTwoPi = 6.283185307179586;

// A squared magnitude below this threshold gives kFloorDb, not -inf.
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

    // A sine of amplitude A after the Hann window (gain 0.5) has magnitude
    // A*N/4 in its bin. Samples are already divided by 32768, so full scale
    // becomes N/4, which has to be subtracted to get 0 dB.
    dbOffset_ = static_cast<float>(-20.0 * log10(n / 4.0));

    // The parabola needs neighbors on both sides, and the summit (local
    // maximum) search reaches one bin below minBin_, so minBin_ is at least 2.
    minBin_ = static_cast<size_t>(ceilf(config.minHz / binHz_));
    if (minBin_ < 2) {
        minBin_ = 2;
    }
    maxBin_ = static_cast<size_t>(floorf(config.maxHz / binHz_));
    if (maxBin_ > n / 2 - 1) {
        maxBin_ = n / 2 - 1;
    }

    // Band i covers frequencies [e_i, e_{i+1}), where e_i grow
    // geometrically from minHz to maxHz. It contains the bins whose
    // frequencies fall within that interval.
    const double ratio = static_cast<double>(config.maxHz) / config.minHz;
    for (size_t i = 0; i < config.bandCount; ++i) {
        const double lo =
            config.minHz * pow(ratio, static_cast<double>(i) / config.bandCount);
        const double hi = config.minHz *
                          pow(ratio, static_cast<double>(i + 1) / config.bandCount);
        size_t first = static_cast<size_t>(ceil(lo / binHz_));
        size_t last = static_cast<size_t>(ceil(hi / binHz_)) - 1;

        // At the low end a band can be narrower than the bin spacing and
        // contain no bins at all. It then shows the bin nearest its center;
        // otherwise the bars would have permanent dips.
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
    // The peak is the highest summit, not the loudest bin: at the edge of
    // the range the loudest bin may be on the slope of a tone lying outside
    // it, and a parabola through a slope gives a spurious frequency. The
    // search starts one bin below minBin_ so that a tone right at the
    // boundary, whose summit falls on the neighboring bin, is not lost.
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

    // Parabola through three points in decibels. At a summit the offset stays
    // within ±0.5 bin. For the Hann window the systematic error of this
    // estimate is hundredths of a bin, i.e. fractions of a hertz.
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

// Height of the peak above the median of the bins 4-12 bins away from it.
// Not closer: the Hann window's main lobe spans ±2 bins, plus a little
// clearance. Not farther either: for voices with a fundamental of 110 Hz
// (14 bins) and up, the neighboring harmonics would fall in. The median
// rather than the mean, so that a single such harmonic, if one does get
// in, cannot raise the background.
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
