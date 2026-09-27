#pragma once

#include <stddef.h>

// How to show a frequency to a person: the nearest equal-tempered note
// (A4 = 440 Hz) and the number itself in hertz.

struct Note {
    const char *name; // "C", "C#", ... "B": ASCII only, board fonts lack ♯
    int octave;       // scientific pitch notation: A4 = 440 Hz, C4 is middle C
    int cents;        // deviation from the note, -50 to +50
};

// hz is a positive frequency in the audio range.
Note noteFor(float hz);

// Four significant digits: "82.41", "440.3", "1234".
void formatHz(float hz, char *buf, size_t size);
