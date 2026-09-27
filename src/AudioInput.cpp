#include "AudioInput.h"

bool AudioInput::begin() {
    auto cfg = M5.Mic.config();
    cfg.sample_rate = kSampleRate;
    cfg.over_sampling = 1;
    // M5Unified's noise filter is a first-order low-pass; it would roll off
    // the top of the spectrum.
    cfg.noise_filter_level = 0;
    // M5Unified multiplies samples by magnification / (2 * over_sampling),
    // so 2 means unity gain. The ES8311 codec already applies +32 dB of
    // digital volume on its own.
    cfg.magnification = 2;
    cfg.dma_buf_len = 256;
    cfg.dma_buf_count = 3;
    M5.Mic.config(cfg);

    if (!M5.Mic.isEnabled()) {
        return false;
    }

    // The speaker and mic share the I2S clock lines. setup() disables the
    // speaker via cfg.internal_spk; end() is here in case it gets enabled.
    M5.Speaker.end();
    return M5.Mic.begin();
}

bool AudioInput::poll() {
    while (M5.Mic.isRecording() < BlockRing::kInFlight) {
        if (!M5.Mic.record(ring_.nextWriteBlock(), kBlockLen, kSampleRate)) {
            break;
        }
        ring_.markQueued();
    }

    const uint32_t completed = ring_.completedBlocks();
    return completed >= kWindowBlocks && completed != consumed_;
}

const int16_t *AudioInput::window() {
    ring_.copyLatest(window_, kWindowBlocks);
    consumed_ = ring_.completedBlocks();
    return window_;
}
