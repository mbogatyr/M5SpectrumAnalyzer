#include "BarBallistics.h"

BarBallistics::BarBallistics(size_t bandCount, float floorDb,
                             const BallisticsConfig &config)
    : config_(config), floorDb_(floorDb),
      bands_(bandCount, Band{floorDb, floorDb, 0, floorDb}) {}

void BarBallistics::update(const float *bandDb, uint32_t nowMs) {
    // Unsigned subtraction handles millis() wraparound correctly.
    const float dtSec = started_ ? (nowMs - lastMs_) / 1000.0f : 0.0f;
    started_ = true;
    lastMs_ = nowMs;

    for (size_t i = 0; i < bands_.size(); ++i) {
        Band &band = bands_[i];

        float level = band.level - config_.releaseDbPerSec * dtSec;
        if (bandDb[i] > level) {
            level = bandDb[i];
        }
        if (level < floorDb_) {
            level = floorDb_;
        }
        band.level = level;

        // The peak cap is a function of the time since it was last raised:
        // it holds for capHoldMs, then falls at a constant rate.
        const uint32_t sinceSet = nowMs - band.capSetMs;
        const float fallSec = (sinceSet > config_.capHoldMs)
                                  ? (sinceSet - config_.capHoldMs) / 1000.0f
                                  : 0.0f;
        float cap = band.capPeakDb - config_.capReleaseDbPerSec * fallSec;
        if (level >= cap) {
            cap = level;
            band.capPeakDb = level;
            band.capSetMs = nowMs;
        }
        if (cap < floorDb_) {
            cap = floorDb_;
        }
        band.cap = cap;
    }
}

float BarBallistics::level(size_t band) const { return bands_[band].level; }

float BarBallistics::cap(size_t band) const { return bands_[band].cap; }
