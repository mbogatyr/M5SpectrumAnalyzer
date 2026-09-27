#include "BlockRing.h"

#include <string.h>

BlockRing::BlockRing(size_t blockLen, size_t blockCount)
    : blockLen_(blockLen), blockCount_(blockCount),
      samples_(blockLen * blockCount, 0) {}

int16_t *BlockRing::nextWriteBlock() {
    return samples_.data() + (queued_ % blockCount_) * blockLen_;
}

void BlockRing::markQueued() { ++queued_; }

uint32_t BlockRing::completedBlocks() const {
    return (queued_ > kInFlight) ? queued_ - kInFlight : 0;
}

bool BlockRing::copyLatest(int16_t *dst, size_t blocks) const {
    const uint32_t completed = completedBlocks();
    if (completed < blocks) {
        return false;
    }

    // Порядковые номера блоков окна: completed - blocks .. completed - 1.
    for (size_t i = 0; i < blocks; ++i) {
        const uint32_t seq = completed - blocks + i;
        const int16_t *src = samples_.data() + (seq % blockCount_) * blockLen_;
        memcpy(dst + i * blockLen_, src, blockLen_ * sizeof(int16_t));
    }
    return true;
}
