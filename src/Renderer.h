#pragma once

#include <M5Unified.h>

#include "BarBallistics.h"
#include "FrequencyReadout.h"
#include "SpectrumAnalyzer.h"

// The header readout. Updates less often than the spectrum so the digits
// can be read.
struct Readout {
    bool valid = false; // false: silence, dashes instead of digits
    char hz[12] = "";
    Note note{"", 0, 0};
};

// Draws the analyzer on the StickS3's built-in display turned on its side
// (240x135): a header with frequency and tuner, 48 bars, a frequency axis.
//
// The whole frame is built in a sprite and pushed in a single call;
// otherwise the bars flicker. The spectrum changes every frame, so there
// is no comparison with the previous frame here.
class Renderer {
  public:
    // Height of the bar scale, in decibels.
    static constexpr float kRangeDb = 60.0f;

    // Call after M5.begin(). The frequency range is needed for the axis.
    void begin(const AnalyzerConfig &config);

    // bottomDb is the level where a bar starts; the scale spans kRangeDb
    // upward from it.
    void draw(const BarBallistics &bars, const Spectrum &spectrum,
              const Readout &readout, float bottomDb, bool frozen);

    // Flips the image 180° when the device is turned with its other long
    // side down. The 240x135 sprite fits both landscape orientations.
    // Returns true if the orientation changed and the frame needs to be
    // redrawn.
    bool setFlipped(bool flipped);

    // Writes the last drawn frame to out: a "SNAP <width> <height>" line,
    // then width*height RGB565 pixels, two bytes each, high byte first.
    // tools/screenshot.py turns this into a PNG. The frame is taken from
    // the sprite, so it comes out upright whichever way the screen is
    // flipped.
    void writeSnapshot(Print &out);

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
    bool flipped_ = false;
};
