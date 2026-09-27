#pragma once

#include <M5Unified.h>

#include "BlockRing.h"

// Непрерывная запись со встроенного микрофона StickS3 (кодек ES8311).
//
// Микрофон пишет блоками по 512 отсчётов в фоновой задаче M5Unified.
// poll() держит его очередь полной, а window() склеивает четыре последних
// готовых блока в окно для БПФ. Новое окно появляется каждые 32 мс и
// перекрывается с предыдущим на три четверти.
class AudioInput {
  public:
    static constexpr uint32_t kSampleRate = 16000;
    static constexpr size_t kBlockLen = 512;
    static constexpr size_t kWindowBlocks = 4;
    static constexpr size_t kWindowLen = kBlockLen * kWindowBlocks;

    // Окно плюс два блока в очереди микрофона. Этого хватает: новый
    // record() ставится только после того, как окно скопировано. Лишний
    // блок — запас на случай, если порядок в poll()/window() изменится.
    static constexpr size_t kRingBlocks = kWindowBlocks + BlockRing::kInFlight + 1;
    static_assert(kRingBlocks >= kWindowBlocks + BlockRing::kInFlight,
                  "микрофон начнёт писать в блок, который ещё читается");

    // Вызывать после M5.begin(). false — микрофона на плате нет.
    bool begin();

    // Дозаполняет очередь микрофона. true — с прошлого window() дописан
    // новый блок и окно можно анализировать.
    bool poll();

    // kWindowLen свежих отсчётов, от старых к новым.
    const int16_t *window();

  private:
    BlockRing ring_{kBlockLen, kRingBlocks};
    int16_t window_[kWindowLen];
    uint32_t consumed_ = 0;
};
