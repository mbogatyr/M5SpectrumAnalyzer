#include "Orientation.h"

#include <math.h>

bool Orientation::update(uint32_t nowMs, float ax, float ay, float az) {
    const Side side = sideOf(ax, ay, az);
    if (side == Side::Unknown) {
        pending_ = Side::Unknown;
        return flipped_;
    }

    const bool wantFlipped = side == Side::Flipped;
    if (!decided_) {
        flipped_ = wantFlipped;
        decided_ = true;
        pending_ = Side::Unknown;
        return flipped_;
    }

    if (wantFlipped == flipped_) {
        pending_ = Side::Unknown;
    } else if (pending_ != side) {
        pending_ = side;
        pendingSinceMs_ = nowMs;
    } else if (nowMs - pendingSinceMs_ >= kSettleMs) {
        // Беззнаковое вычитание корректно переживает переполнение millis().
        flipped_ = wantFlipped;
        pending_ = Side::Unknown;
    }
    return flipped_;
}

void Orientation::reset() {
    decided_ = false;
    pending_ = Side::Unknown;
}

Orientation::Side Orientation::sideOf(float ax, float ay, float az) {
    // Трясут или несут: в ускорении есть что-то кроме тяжести.
    const float g2 = ax * ax + ay * ay + az * az;
    const float lo = 1.0f - kShakeG;
    const float hi = 1.0f + kShakeG;
    if (g2 < lo * lo || g2 > hi * hi) {
        return Side::Unknown;
    }

    const float x = fabsf(ax);
    if (x < kMinG || x < fabsf(ay) || x < fabsf(az)) {
        return Side::Unknown;
    }
    // Normal соответствует setRotation(1) — KEY1 справа от экрана.
    // Проверено на плате: при ax > 0 изображение стоит правильно, при
    // ax < 0 — после переворота на 180°.
    return ax > 0 ? Side::Normal : Side::Flipped;
}
