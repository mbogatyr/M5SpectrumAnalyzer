#pragma once

#include <stddef.h>
#include <stdint.h>

#include <vector>

struct BallisticsConfig {
    float releaseDbPerSec = 60.0f;   // скорость спада столбика
    uint32_t capHoldMs = 500;        // сколько колпачок висит на максимуме
    float capReleaseDbPerSec = 30.0f; // потом падает, медленнее столбика
};

// Поведение столбиков во времени: подъём мгновенный, спад плавный,
// над каждым столбиком колпачок пикового уровня.
//
// Как и DisplayTimeout, железа не касается: время приходит снаружи,
// поэтому тесты подставляют любые моменты без ожидания.
class BarBallistics {
  public:
    BarBallistics(size_t bandCount, float floorDb,
                  const BallisticsConfig &config = BallisticsConfig{});

    // bandDb — свежие уровни полос, по одному на столбик.
    void update(const float *bandDb, uint32_t nowMs);

    float level(size_t band) const;
    float cap(size_t band) const;

  private:
    struct Band {
        float level;
        float capPeakDb; // уровень, на котором колпачок был поднят
        uint32_t capSetMs;
        float cap;
    };

    BallisticsConfig config_;
    float floorDb_;
    std::vector<Band> bands_;
    bool started_ = false;
    uint32_t lastMs_ = 0;
};
