#pragma once

#include <stdint.h>

// Decides from the accelerometer whether the device is rotated 180°.
//
// The screen is landscape: when the device is held or stood on a long edge,
// gravity points across the board, along its short X axis. The sign of X
// tells which of the two long edges faces down. Standing on end (gravity
// along Y) or lying flat on a table (along Z), the top cannot be determined,
// so the orientation stays as it was.
//
// Like everything in lib/, it does not touch the hardware: it takes the time
// and acceleration, and returns a decision.
class Orientation {
  public:
    static constexpr float kMinG = 0.6f;       // gravity component along X, g
    static constexpr float kShakeG = 0.25f;    // tolerance of |a| around 1 g
    static constexpr uint32_t kSettleMs = 400; // hold time for a new position

    // ax, ay, az: acceleration in g along the board axes (M5.Imu.getAccel).
    // Returns flipped().
    bool update(uint32_t nowMs, float ax, float ay, float az);

    bool flipped() const { return flipped_; }

    // The next clear reading takes effect immediately, without waiting.
    // Needed at startup and after the screen sleeps: the device may have
    // been turned over while the screen was off.
    void reset();

  private:
    enum class Side : uint8_t { Unknown, Normal, Flipped };

    static Side sideOf(float ax, float ay, float az);

    bool flipped_ = false;
    bool decided_ = false;
    Side pending_ = Side::Unknown;
    uint32_t pendingSinceMs_ = 0;
};
