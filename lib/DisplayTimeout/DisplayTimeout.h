#pragma once

#include <stdint.h>

// Решает, когда гасить экран после простоя.
//
// Выключением питания не занимается: боковая кнопка StickS3 выключает
// плату двойным щелчком сама, на уровне PMIC, без участия прошивки.
//
// Как и весь lib/, железа не касается: на входе время и факт
// нажатия, на выходе решение.
class DisplayTimeout {
  public:
    static constexpr uint32_t kIdleMs = 180000; // 3 минуты

    explicit DisplayTimeout(uint32_t idleMs = kIdleMs);

    // Задаёт точку отсчёта простоя. Вызывать один раз при старте.
    void begin(uint32_t nowMs);

    // activity — была ли активность в этом такте: нажатие кнопки или
    // звук, в котором анализатор нашёл пик.
    bool shouldBeOn(uint32_t nowMs, bool activity);

  private:
    uint32_t idleMs_;
    uint32_t lastActivityMs_ = 0;
};
