#include "AudioInput.h"

bool AudioInput::begin() {
    auto cfg = M5.Mic.config();
    cfg.sample_rate = kSampleRate;
    cfg.over_sampling = 1;
    // Шумовой фильтр M5Unified — это ФНЧ первого порядка, он завалил бы
    // верх спектра.
    cfg.noise_filter_level = 0;
    // M5Unified умножает отсчёты на magnification / (2 * over_sampling),
    // так что 2 — это единичное усиление. Кодек ES8311 и так уже
    // добавляет +32 дБ цифровой громкости.
    cfg.magnification = 2;
    cfg.dma_buf_len = 256;
    cfg.dma_buf_count = 3;
    M5.Mic.config(cfg);

    if (!M5.Mic.isEnabled()) {
        return false;
    }

    // Динамик и микрофон делят тактовые линии I2S; динамик отключён в
    // M5.config(), end() здесь на случай, если его кто-то включит.
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
