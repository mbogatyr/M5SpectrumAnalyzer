#include "TrafficLight.h"

TrafficLight::TrafficLight(const PhaseDurations &durations)
    : durations_(durations) {}

uint32_t TrafficLight::cycleMs() const {
    return durations_.redMs + durations_.redYellowMs + durations_.greenMs +
           durations_.greenBlinkMs + durations_.yellowMs;
}

TrafficLight::Position TrafficLight::positionAt(uint32_t nowMs) const {
    const uint32_t cycle = cycleMs();
    if (cycle == 0) {
        return Position{Phase::Red, 0, 0};
    }

    struct Span {
        Phase phase;
        uint32_t durationMs;
    };
    const Span spans[] = {
        {Phase::Red, durations_.redMs},
        {Phase::RedYellow, durations_.redYellowMs},
        {Phase::Green, durations_.greenMs},
        {Phase::GreenBlink, durations_.greenBlinkMs},
        {Phase::Yellow, durations_.yellowMs},
    };

    uint32_t t = nowMs % cycle;
    for (const Span &span : spans) {
        if (t < span.durationMs) {
            return Position{span.phase, t, span.durationMs};
        }
        t -= span.durationMs;
    }

    // Недостижимо: t < cycle, а cycle — сумма всех длительностей.
    return Position{Phase::Yellow, 0, durations_.yellowMs};
}

Phase TrafficLight::phaseAt(uint32_t nowMs) const {
    return positionAt(nowMs).phase;
}

uint8_t TrafficLight::secondsRemainingAt(uint32_t nowMs) const {
    const Position at = positionAt(nowMs);
    const uint32_t remainingMs = at.durationMs - at.elapsedMs;
    // Округление вверх: «1» держится всю последнюю секунду фазы.
    return static_cast<uint8_t>((remainingMs + 999) / 1000);
}

Lamps TrafficLight::lampsAt(uint32_t nowMs) const {
    const Position at = positionAt(nowMs);
    switch (at.phase) {
    case Phase::Red:
        return Lamps{true, false, false};
    case Phase::RedYellow:
        return Lamps{true, true, false};
    case Phase::Green:
        return Lamps{false, false, true};
    case Phase::GreenBlink: {
        const bool lit =
            (at.elapsedMs % kBlinkPeriodMs) < (kBlinkPeriodMs / 2);
        return Lamps{false, false, lit};
    }
    case Phase::Yellow:
        return Lamps{false, true, false};
    }
    return Lamps{false, false, false};
}
