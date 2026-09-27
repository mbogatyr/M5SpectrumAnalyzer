# M5SpectrumAnalyzer

A real-time audio spectrum analyzer for the M5StickS3.
It listens through the built-in microphone, runs an FFT, and draws the
spectrum as 48 bars. The header shows the frequency of the strongest harmonic,
its musical note, and a tuner scale. No external hardware needed.

```
┌────────────────────────────────────────────────┐
│ 440.1 Hz                    A4           +2 ct │
│                        ├────┼────┼█───┼────┤   │
│                    ▼                           │
│                    ▆                           │
│                    █     ▆   ▁                 │
│ ▂▁▁▂▁▁▂▁▁▂▁▁▂▁▁▂▁▁▇█▇▂▁▁▄█▄▂▃█▃▁▅▂▁▁▂▁▁▂▃▄▃▄▃▄ │
│ 50   100                  1k            5k  8k │
└────────────────────────────────────────────────┘
```

## Features

- **Spectrum:** 48 bars on a logarithmic 50 Hz – 8 kHz scale, with peak caps
  that hold for half a second and then fall. Bars are green, turning yellow
  above 60% of the scale and red above 85%. About 31 frames per second.
- **Strongest harmonic:** its frequency, refined between FFT bins by parabolic
  interpolation. On test tones played from a laptop speaker (305, 440, 1000
  and 3000 Hz) the readout was within 0.5 Hz. A cyan marker points to its bar.
- **Note and tuner:** the nearest equal-tempered note (A4 = 440 Hz), cents off
  it, and a ±50-cent scale. The needle is green within 5 cents, yellow within
  20, red beyond.
- **Silence shows dashes.** A frequency appears only for a real tone, not for
  background noise.
- **Auto-rotation:** turn the device so its other long edge points down, and
  the picture flips 180°.
- **Screen timeout:** the screen turns off after 3 minutes with no button
  press and no detected tone, and wakes on either.

## Controls

| Control | Action |
|---|---|
| KEY1 | Pause: freeze the frame (a `HOLD` badge appears); press again to resume |
| KEY2 | Sensitivity: cycle three steps that shift the bar scale by 0 / +12 / +24 dB |
| Side button, single press | Power on / reset |
| Side button, double press | Power off |
| Side button, long hold | Download mode (the green LED blinks) |

The side button is handled by the power-management chip, not by the
firmware. A button press on a dark screen only wakes it.

## Things to know

- **Strongest harmonic is not always the pitch you hear.** For voice and
  string instruments the second or third harmonic is often the loudest, so the
  frequency and note can read an octave (or an octave and a fifth) high. This
  is by design: the analyzer shows the strongest peak.
- **Low notes.** The microphone chain barely passes fundamentals below about
  60–70 Hz, so such sounds show up at the frequency of their second or third
  harmonic.
- **The top few bars never quite go flat.** Even in a quiet room the
  microphone picks up broadband noise at 5–8 kHz around -64 dBFS. It shows as
  low bars at the right edge but never as a frequency reading.
- **ASCII notes.** The display fonts are ASCII-only, so sharps are written
  `C#` and cents `ct`.

## Building and flashing

You need [PlatformIO](https://platformio.org/).

```bash
pio run -e sticks3              # build the firmware
pio run -e sticks3 -t upload    # flash it
pio device monitor -e sticks3   # serial monitor, 115200 baud
pio test -e native              # run the unit tests on your computer
```

The first build downloads the ESP32-S3 toolchain and takes several minutes.

PlatformIO has no board definition for the StickS3, so `platformio.ini` uses
`esp32-s3-devkitc-1` with octal PSRAM (`qio_opi`) and 8 MB partitions.

If flashing fails with `Failed to connect to ESP32-S3: No serial data
received`, hold the side button until the green LED blinks and flash again.

Once a second the firmware prints a status line to Serial:

```
fps 31  analyze 4.9 ms  draw 17.4 ms  peak 440.1 Hz -28.4 dB
```

## How it works

1. The microphone (through the board's ES8311 codec) is recorded at 16 kHz in
   512-sample blocks, with two blocks always queued so capture has no gaps.
2. Every 32 ms the latest 2048 samples (128 ms) go through a Hann window and
   an FFT, giving bins 7.8 Hz apart.
3. The peak is the highest local maximum between 50 Hz and 8 kHz. It must be
   louder than -70 dBFS and stand at least 15 dB above nearby bins. That
   second test is what keeps broadband noise from producing a random
   frequency.
4. Bins are folded into 48 logarithmic bands. Bars rise instantly and fall at
   a fixed rate.
5. The frame is drawn into an off-screen sprite and pushed to the display in
   one go, so it doesn't flicker.

## Project layout

The code is split so that all the signal processing can be tested on a
computer without the board:

| Path | Contents |
|---|---|
| `lib/Fft/` | In-place radix-2 FFT |
| `lib/SpectrumAnalyzer/` | Window, FFT, dBFS levels, peak search, log bands |
| `lib/FrequencyReadout/` | Note and cents, frequency formatting |
| `lib/BarBallistics/` | Bar decay and peak caps |
| `lib/BlockRing/` | Ring of capture blocks, stitched into the FFT window |
| `lib/Orientation/` | Accelerometer-based 180° flip decision |
| `lib/DisplayTimeout/` | When to turn the screen off |
| `src/` | Board-specific code: microphone capture, rendering, main loop |
| `test/` | Unity unit tests for everything in `lib/` |

`lib/` is plain C++ with no Arduino or M5Unified dependencies. The `native`
environment in `platformio.ini` builds and tests it on the host.
