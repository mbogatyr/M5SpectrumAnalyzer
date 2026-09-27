#pragma once

#include <stddef.h>
#include <stdint.h>

#include <vector>

struct BallisticsConfig {
    float releaseDbPerSec = 60.0f;   // bar release rate
    uint32_t capHoldMs = 500;        // peak cap hold time at the maximum
    float capReleaseDbPerSec = 30.0f; // then falls, slower than the bar
};

// Bar behavior over time: instant attack, smooth release, and a peak cap
// above each bar.
//
// Like DisplayTimeout, it does not touch the hardware: time comes from
// outside, so tests can plug in any moment without waiting.
class BarBallistics {
  public:
    BarBallistics(size_t bandCount, float floorDb,
                  const BallisticsConfig &config = BallisticsConfig{});

    // bandDb holds the latest band levels, one per bar.
    void update(const float *bandDb, uint32_t nowMs);

    float level(size_t band) const;
    float cap(size_t band) const;

  private:
    struct Band {
        float level;
        float capPeakDb; // level the peak cap was raised to
        uint32_t capSetMs;
        float cap;
    };

    BallisticsConfig config_;
    float floorDb_;
    std::vector<Band> bands_;
    bool started_ = false;
    uint32_t lastMs_ = 0;
};
