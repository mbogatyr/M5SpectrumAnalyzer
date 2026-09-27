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
    // !(hz > 0) catches zero, negatives and NaN alike.
    if (!(hz > 0.0f) || !isfinite(hz)) {
        return Note{"", 0, 0};
    }

    // Fractional MIDI number: rounded, it gives the nearest note; the
    // remainder is the cents.
    const float midi = kMidiA4 + 12.0f * log2f(hz / kHzA4);
    const int nearest = static_cast<int>(lroundf(midi));
    const int cents = static_cast<int>(lroundf(100.0f * (midi - nearest)));

    // Floor division: for negative MIDI numbers the plain % and / round
    // toward zero and would give a negative index.
    const int octaveFromC = (nearest >= 0) ? nearest / 12 : (nearest - 11) / 12;
    const int pitchClass = nearest - octaveFromC * 12;
    return Note{kNames[pitchClass], octaveFromC - 1, cents};
}

void formatHz(float hz, char *buf, size_t size) {
    // The threshold is shifted by half of the last digit: 99.996 rounds to
    // 100.00, which is already five digits, so it belongs in the next format.
    if (hz < 99.995f) {
        snprintf(buf, size, "%.2f", hz);
    } else if (hz < 999.95f) {
        snprintf(buf, size, "%.1f", hz);
    } else {
        snprintf(buf, size, "%.0f", hz);
    }
}
