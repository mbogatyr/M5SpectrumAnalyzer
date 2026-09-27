#pragma once

#include <stddef.h>
#include <stdint.h>

#include <vector>

#include "Fft.h"

struct AnalyzerConfig {
    uint32_t sampleRate = 16000;
    size_t fftSize = 2048;

    // Диапазон, в котором ищется пик и который делится на полосы.
    float minHz = 50.0f;
    float maxHz = 8000.0f;

    // Число полос в логарифмической шкале — по столбику на полосу.
    size_t bandCount = 48;

    // Пик тише этого уровня считается тишиной.
    float silenceDb = -70.0f;

    // Насколько пик должен подниматься над своими соседями, чтобы
    // считаться тоном, а не случайным выбросом шума. У белого шума самый
    // громкий из сотен бинов выше медианы лишь на 9-11 дБ.
    float minProminenceDb = 15.0f;
};

// Результат анализа одного окна. Уровни — в дБFS: синус полной шкалы
// int16 даёт 0 дБ, синус половинной — около -6 дБ.
struct Spectrum {
    std::vector<float> bandDb;

    bool hasPeak = false;
    float peakHz = 0.0f;
    float peakDb = 0.0f;
    size_t peakBand = 0; // полоса, над которой стоит частота пика
};

// Окно Ханна, БПФ, поиск самой сильной гармоники и сведение бинов в
// логарифмические полосы.
//
// Результат зависит только от входа: между вызовами анализатор ничего не
// помнит, у него есть лишь рабочие буферы, выделенные один раз в
// конструкторе.
class SpectrumAnalyzer {
  public:
    // Уровень, которым заменяется логарифм нуля.
    static constexpr float kFloorDb = -120.0f;

    explicit SpectrumAnalyzer(const AnalyzerConfig &config = AnalyzerConfig{});

    // samples — ровно fftSize отсчётов, от старых к новым.
    const Spectrum &analyze(const int16_t *samples);

    const AnalyzerConfig &config() const { return config_; }

  private:
    // Бины, из которых собирается одна полоса: first..last включительно.
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
    float dbOffset_;   // переводит квадрат модуля бина в дБFS
    size_t minBin_;    // диапазон поиска пика
    size_t maxBin_;

    std::vector<float> window_;
    std::vector<float> re_;
    std::vector<float> im_;
    std::vector<float> binDb_; // бины 0..fftSize/2
    std::vector<BandBins> bands_;

    Spectrum spectrum_;
};
