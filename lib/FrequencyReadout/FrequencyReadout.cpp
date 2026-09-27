#include "FrequencyReadout.h"

#include <math.h>
#include <stdio.h>

namespace {

const char *const kNames[12] = {"C",  "C#", "D",  "D#", "E",  "F",
                                "F#", "G",  "G#", "A",  "A#", "B"};

const int kMidiA4 = 69;
const float kHzA4 = 440.0f;

} // namespace

Note noteFor(float hz) {
    // !(hz > 0) ловит и ноль, и отрицательные, и NaN.
    if (!(hz > 0.0f) || !isfinite(hz)) {
        return Note{"", 0, 0};
    }

    // Номер ноты MIDI дробный: целая часть — ближайшая нота, остаток — центы.
    const float midi = kMidiA4 + 12.0f * log2f(hz / kHzA4);
    const int nearest = static_cast<int>(lroundf(midi));
    const int cents = static_cast<int>(lroundf(100.0f * (midi - nearest)));

    // Деление с округлением вниз: у отрицательных номеров MIDI обычные
    // % и / округляют к нулю и дали бы отрицательный индекс.
    const int octaveFromC = (nearest >= 0) ? nearest / 12 : (nearest - 11) / 12;
    const int pitchClass = nearest - octaveFromC * 12;
    return Note{kNames[pitchClass], octaveFromC - 1, cents};
}

void formatHz(float hz, char *buf, size_t size) {
    // Порог сдвинут на половину последнего разряда: 99.996 округляется до
    // 100.00, а это уже пять цифр, так что его место в следующем формате.
    if (hz < 99.995f) {
        snprintf(buf, size, "%.2f", hz);
    } else if (hz < 999.95f) {
        snprintf(buf, size, "%.1f", hz);
    } else {
        snprintf(buf, size, "%.0f", hz);
    }
}
