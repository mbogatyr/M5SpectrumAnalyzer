#pragma once

#include <stdint.h>

// Решает по акселерометру, перевёрнут ли прибор на 180°.
//
// Экран альбомный: когда прибор держат или ставят длинной стороной вниз,
// тяжесть направлена поперёк платы, вдоль её короткой оси X. Знак X говорит,
// какой из двух длинных сторон он смотрит вниз. Стоймя (тяжесть вдоль Y) и
// плашмя на столе (вдоль Z) верх не определить — тогда ориентация остаётся
// прежней.
//
// Как и всё в lib/, железа не касается: на входе время и ускорение, на
// выходе решение.
class Orientation {
  public:
    static constexpr float kMinG = 0.6f;       // проекция тяжести на X, g
    static constexpr float kShakeG = 0.25f;    // допуск |a| от 1 g
    static constexpr uint32_t kSettleMs = 400; // сколько держать новое положение

    // ax, ay, az — ускорение в g в осях платы (M5.Imu.getAccel).
    // Возвращает flipped().
    bool update(uint32_t nowMs, float ax, float ay, float az);

    bool flipped() const { return flipped_; }

    // Следующее ясное показание применится сразу, без ожидания. Нужно
    // при старте и после сна экрана: прибор могли перевернуть, пока
    // экран был погашен.
    void reset();

  private:
    enum class Side : uint8_t { Unknown, Normal, Flipped };

    static Side sideOf(float ax, float ay, float az);

    bool flipped_ = false;
    bool decided_ = false;
    Side pending_ = Side::Unknown;
    uint32_t pendingSinceMs_ = 0;
};
