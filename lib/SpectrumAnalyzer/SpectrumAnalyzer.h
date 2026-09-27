#pragma once

#include <stddef.h>
#include <stdint.h>

#include <vector>

#include "Fft.h"

struct AnalyzerConfig {
    uint32_t sampleRate = 16000;
    size_t fftSize = 2048;

    // Range that is searched for the peak and divided into bands.
    float minHz = 50.0f;
    float maxHz = 8000.0f;

    // Number of bands on a logarithmic scale, one bar per band.
    size_t bandCount = 48;

    // A peak quieter than this level counts as silence.
    float silenceDb = -70.0f;

    // How far a peak must rise above its neighbors to count as a tone
    // rather than a random noise spike. In white noise the loudest of
    // hundreds of bins is only 9-11 dB above the median.
    float minProminenceDb = 15.0f;
};

// Result of analyzing one window. Levels are in dBFS: a full-scale int16
// sine gives 0 dB, a half-scale sine about -6 dB.
struct Spectrum {
    std::vector<float> bandDb;

    bool hasPeak = false;
    float peakHz = 0.0f;
    float peakDb = 0.0f;
    size_t peakBand = 0; // band the peak frequency falls in
};

// Hann window, FFT, a search for the strongest harmonic, and merging of
// bins into logarithmic bands.
//
// The result depends only on the input: the analyzer remembers nothing
// between calls. All it has are scratch buffers, allocated once in the
// constructor.
class SpectrumAnalyzer {
  public:
    // Level that stands in for the logarithm of zero.
    static constexpr float kFloorDb = -120.0f;

    explicit SpectrumAnalyzer(const AnalyzerConfig &config = AnalyzerConfig{});

    // samples holds exactly fftSize samples, oldest first.
    const Spectrum &analyze(const int16_t *samples);

    const AnalyzerConfig &config() const { return config_; }

  private:
    // Bins that make up one band: first..last inclusive.
    struct BandBins {
        size_t first;
        size_t last;
    };

    void findPeak();
    float prominenceDb(size_t peakBin) const;
    void fillBands();
    size_t bandForHz(float hz) const;

    AnalyzerConfig config_;
    Fft fft_;
    float binHz_;
    float dbOffset_;   // converts a bin's squared magnitude to dBFS
    size_t minBin_;    // peak search range
    size_t maxBin_;

    std::vector<float> window_;
    std::vector<float> re_;
    std::vector<float> im_;
    std::vector<float> binDb_; // bins 0..fftSize/2
    std::vector<BandBins> bands_;

    Spectrum spectrum_;
};
