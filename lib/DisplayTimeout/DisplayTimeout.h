#pragma once

#include <stdint.h>

// Decides when to turn the screen off after inactivity.
//
// It does not handle power-off: a double press of the StickS3 side button
// turns the board off on its own, in the PMIC, with no firmware involved.
//
// Like the rest of lib/, it does not touch the hardware: it takes the time
// and whether there was a press, and returns a decision.
class DisplayTimeout {
  public:
    static constexpr uint32_t kIdleMs = 180000; // 3 minutes

    explicit DisplayTimeout(uint32_t idleMs = kIdleMs);

    // Sets the point idle time is measured from. Call once at startup.
    void begin(uint32_t nowMs);

    // activity: whether there was activity this tick, either a button press
    // or a sound in which the analyzer found a peak.
    bool shouldBeOn(uint32_t nowMs, bool activity);

  private:
    uint32_t idleMs_;
    uint32_t lastActivityMs_ = 0;
};
