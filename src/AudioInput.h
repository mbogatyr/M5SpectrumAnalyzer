#pragma once

#include <M5Unified.h>

#include "BlockRing.h"

// Continuous capture from the StickS3's built-in mic (ES8311 codec).
//
// The mic records in 512-sample blocks in an M5Unified background task.
// poll() keeps its queue full, and window() joins the four latest
// completed blocks into an FFT window. A new window is ready every 32 ms
// and overlaps the previous one by three quarters.
class AudioInput {
  public:
    static constexpr uint32_t kSampleRate = 16000;
    static constexpr size_t kBlockLen = 512;
    static constexpr size_t kWindowBlocks = 4;
    static constexpr size_t kWindowLen = kBlockLen * kWindowBlocks;

    // The window plus two blocks in the mic queue. That is enough: a new
    // record() is queued only after the window has been copied. The extra
    // block is a margin in case the order in poll()/window() changes.
    static constexpr size_t kRingBlocks = kWindowBlocks + BlockRing::kInFlight + 1;
    static_assert(kRingBlocks >= kWindowBlocks + BlockRing::kInFlight,
                  "the mic would write into a block that is still being read");

    // Call after M5.begin(). Returns false if the board has no mic.
    bool begin();

    // Tops up the mic queue. Returns true if a new block has been written
    // since the last window() and the window can be analyzed.
    bool poll();

    // The kWindowLen most recent samples, oldest first.
    const int16_t *window();

  private:
    BlockRing ring_{kBlockLen, kRingBlocks};
    int16_t window_[kWindowLen];
    uint32_t consumed_ = 0;
};
