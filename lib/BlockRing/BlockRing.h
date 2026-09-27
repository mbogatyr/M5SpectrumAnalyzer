#pragma once

#include <stddef.h>
#include <stdint.h>

#include <vector>

// Ring of blocks for continuous recording from the microphone.
//
// M5.Mic.record() accepts up to two requests ahead and fills them in turn
// without gaps. The firmware keeps the queue full, so of all queued blocks
// the two most recent are still being written and everything older is
// complete. The ring counts how many blocks have been queued and uses that
// count to stitch the latest complete blocks into a contiguous FFT window.
//
// The ring must be at least two blocks larger than the window; otherwise
// the next record() would start writing into a block still being read.
class BlockRing {
  public:
    // Number of blocks in the microphone queue at any one time.
    static constexpr size_t kInFlight = 2;

    BlockRing(size_t blockLen, size_t blockCount);

    size_t blockLen() const { return blockLen_; }

    // Buffer for the next record() call.
    int16_t *nextWriteBlock();

    // record() has accepted the buffer from nextWriteBlock().
    void markQueued();

    // Total number of blocks fully written so far.
    uint32_t completedBlocks() const;

    // Copies the latest `blocks` complete blocks into dst, oldest first.
    // Returns false if that many blocks have not accumulated yet.
    bool copyLatest(int16_t *dst, size_t blocks) const;

  private:
    size_t blockLen_;
    size_t blockCount_;
    std::vector<int16_t> samples_;
    uint32_t queued_ = 0;
};
