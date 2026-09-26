#pragma once

#include <M5Unified.h>

#include "TrafficLight.h"

// Рисует светофор на встроенном дисплее StickS3 (135x240).
//
// Кадр собирается целиком в спрайте и выталкивается одним вызовом:
// рисование кругов прямо на экране даёт заметное мерцание.
class Renderer {
  public:
    // Вызывать после M5.begin().
    void begin();

    // Перерисовывает экран только тогда, когда картинка изменилась.
    void draw(Phase phase, const Lamps &lamps, uint8_t secondsRemaining);

    // Сбрасывает память о последнем кадре. Нужно после пробуждения
    // дисплея: его содержимое потеряно, а сравнение с прошлым кадром
    // иначе решит, что перерисовывать нечего.
    void invalidate();

  private:
    void paint(Phase phase, const Lamps &lamps, uint8_t secondsRemaining);

    M5Canvas canvas_{&M5.Display};

    bool hasPrevious_ = false;
    Phase previousPhase_ = Phase::Red;
    Lamps previousLamps_{false, false, false};
    uint8_t previousSeconds_ = 0;
};
