#pragma once

#include <stdint.h>

// Фазы светофора в порядке следования внутри цикла.
enum class Phase : uint8_t {
    Red,
    RedYellow,
    Green,
    GreenBlink,
    Yellow,
};

// Длительность каждой фазы в миллисекундах.
struct PhaseDurations {
    uint32_t redMs = 8000;
    uint32_t redYellowMs = 2000;
    uint32_t greenMs = 8000;
    uint32_t greenBlinkMs = 3000;
    uint32_t yellowMs = 3000;
};

// Какие лампы горят прямо сейчас. Мигание уже учтено.
struct Lamps {
    bool red;
    bool yellow;
    bool green;
};

// Конечный автомат светофора.
//
// Не хранит состояние между вызовами и не обращается к железу: фаза —
// чистая функция от времени, поэтому автомат тестируется на хосте, а
// прошивка просто передаёт ему millis().
class TrafficLight {
  public:
    // Период мигания зелёного: 500 мс горит, 500 мс погашен.
    static constexpr uint32_t kBlinkPeriodMs = 1000;

    explicit TrafficLight(const PhaseDurations &durations = PhaseDurations{});

    Phase phaseAt(uint32_t nowMs) const;
    Lamps lampsAt(uint32_t nowMs) const;

    // Секунды до конца текущей фазы, округлённые вверх: последняя
    // секунда фазы показывается как «1» целиком, а не как «0».
    uint8_t secondsRemainingAt(uint32_t nowMs) const;

    uint32_t cycleMs() const;

  private:
    // Где мы внутри цикла: фаза, сколько в ней уже прошло и сколько
    // она длится целиком.
    struct Position {
        Phase phase;
        uint32_t elapsedMs;
        uint32_t durationMs;
    };

    Position positionAt(uint32_t nowMs) const;

    PhaseDurations durations_;
};
