#pragma once

#include <stddef.h>
#include <stdint.h>

#include <vector>

// Кольцо блоков для непрерывной записи с микрофона.
//
// M5.Mic.record() принимает до двух запросов вперёд и заполняет их по
// очереди без разрывов. Прошивка держит очередь полной, поэтому из всех
// поставленных блоков два самых свежих ещё пишутся, а всё, что старше, уже
// готово. Кольцо помнит, сколько блоков поставлено, и по этому счётчику
// склеивает последние готовые блоки в непрерывное окно для БПФ.
//
// Кольцо должно быть хотя бы на два блока больше окна: иначе следующий
// record() начнёт писать в блок, который ещё читается.
class BlockRing {
  public:
    // Столько блоков одновременно стоит в очереди микрофона.
    static constexpr size_t kInFlight = 2;

    BlockRing(size_t blockLen, size_t blockCount);

    size_t blockLen() const { return blockLen_; }

    // Буфер для следующего вызова record().
    int16_t *nextWriteBlock();

    // record() принял буфер из nextWriteBlock().
    void markQueued();

    // Сколько блоков уже записано целиком за всё время.
    uint32_t completedBlocks() const;

    // Копирует последние `blocks` готовых блоков в dst, от старых к новым.
    // Возвращает false, если столько блоков ещё не накопилось.
    bool copyLatest(int16_t *dst, size_t blocks) const;

  private:
    size_t blockLen_;
    size_t blockCount_;
    std::vector<int16_t> samples_;
    uint32_t queued_ = 0;
};
