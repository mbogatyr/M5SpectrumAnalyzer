#pragma once

#include <M5Unified.h>

#include "BarBallistics.h"
#include "FrequencyReadout.h"
#include "SpectrumAnalyzer.h"

// Показание в шапке. Обновляется реже спектра, чтобы цифры успевали
// прочитать.
struct Readout {
    bool valid = false; // false — тишина, вместо цифр прочерки
    char hz[12] = "";
    Note note{"", 0, 0};
};

// Рисует анализатор на встроенном дисплее StickS3, положенном набок
// (240x135): шапка с частотой и тюнером, 48 столбиков, ось частот.
//
// Кадр собирается целиком в спрайте и выталкивается одним вызовом, иначе
// столбики мерцают. Спектр меняется каждый кадр, поэтому сравнения с
// прошлым кадром здесь нет.
class Renderer {
  public:
    // Высота шкалы столбиков в децибелах.
    static constexpr float kRangeDb = 60.0f;

    // Вызывать после M5.begin(). Диапазон частот нужен для оси.
    void begin(const AnalyzerConfig &config);

    // bottomDb — уровень, с которого начинается столбик; шкала занимает
    // kRangeDb вверх от него.
    void draw(const BarBallistics &bars, const Spectrum &spectrum,
              const Readout &readout, float bottomDb, bool frozen);

  private:
    void paintReadout(const Readout &readout);
    void paintTuner(const Readout &readout);
    void paintBars(const BarBallistics &bars, float bottomDb);
    void paintPeakMarker(const Spectrum &spectrum);
    void paintAxis();
    void paintHold();

    int xForHz(float hz) const;

    M5Canvas canvas_{&M5.Display};
    float minHz_ = 50.0f;
    float maxHz_ = 8000.0f;
    size_t bandCount_ = 48;
};
