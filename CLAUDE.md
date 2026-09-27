# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

An audio spectrum analyzer on the M5StickS3: the built-in microphone, an FFT,
48 bars on the display, and the frequency of the strongest harmonic with its
note and a tuner. No external hardware.

Documentation, code comments and commit messages are in English. Commits up
to `4d2a770` are in Russian.

## Commands

PlatformIO is not installed globally but into a venv by the official
installer. The binary lives at `~/.platformio/penv/bin/pio`. It is not on
`PATH`, so call it by its full path.

```bash
~/.platformio/penv/bin/pio test -e native                      # host-side unit tests of the logic
~/.platformio/penv/bin/pio test -e native -f test_fft          # a single test suite
~/.platformio/penv/bin/pio run -e sticks3                      # build the firmware
~/.platformio/penv/bin/pio run -e sticks3 -t upload            # flash the board
~/.platformio/penv/bin/pio run -e sticks3 -t merged            # single image for M5Burner
~/.platformio/penv/bin/pio device monitor -e sticks3           # serial monitor, 115200
```

The first `sticks3` build pulls the xtensa-esp32s3 toolchain and takes about
eight minutes; later builds take seconds.

## Architecture

The split between `lib/` and `src/` is load-bearing, not cosmetic:

- `lib/Fft/` — in-place complex radix-2 FFT.
- `lib/SpectrumAnalyzer/` — Hann window, FFT, dBFS, peak search, and folding
  bins into logarithmic bands.
- `lib/FrequencyReadout/` — the note with its cents, and formatting hertz.
- `lib/BarBallistics/` — bar decay and peak caps over time.
- `lib/BlockRing/` — the ring of capture blocks, and stitching the finished
  blocks into a window.
- `lib/DisplayTimeout/` — decides when to turn the screen off after idling.
- `lib/Orientation/` — decides from the accelerometer whether the device is
  turned over by 180°.
- `src/` — everything that knows about the board: `AudioInput` runs the
  microphone, `Renderer` draws on the display, `main.cpp` ties the logic to
  the hardware.

Everything in `lib/` is plain C++ with no Arduino, no M5Unified, and no
hardware access of any kind.

The `native` environment builds only `lib/` (PlatformIO's `test_build_src`
defaults to `no`), so the logic is tested on the Mac without flashing the
board. **Do not pull hardware dependencies into `lib/` — that breaks the
tests.** Anything that needs M5Unified lives in `src/`. For the same reason
the FFT is our own rather than ESP-DSP: ESP-DSP ships with the framework, but
only for the ESP32 target.

### Pipeline

16 kHz, 512-sample blocks, a 2048-sample window (128 ms), a new frame every
32 ms:

1. `AudioInput::poll()` keeps the `M5.Mic.record()` queue full, with two
   blocks in flight. From the count of queued blocks, `BlockRing` knows that
   everything older than the last two is finished, and stitches four finished
   blocks together in order. M5Unified's `Mic_FFT` example gets this wrong: it
   analyzes the start of the ring rather than the freshest samples.
2. `SpectrumAnalyzer::analyze()` is a pure function of the window: its work
   buffers are allocated in the constructor, and it keeps no state between
   calls.
3. `BarBallistics::update()` takes the time from outside, like
   `DisplayTimeout`.
4. The header digits update every 200 ms; the spectrum updates every frame.

Budget on the board: ~5 ms analysis, ~18 ms drawing, ~31 fps. Once a second
the firmware prints a `fps … analyze … draw … peak …` line to Serial, which
shows whether a frame fits in 32 ms.

### When a peak counts as a peak

The peak is the highest **summit** (a bin no lower than either neighbor), not
the loudest bin. At the edge of the range, the loudest bin can be the skirt of
a tone lying outside it, and a parabola through a skirt invents a frequency: a
43 Hz tone read as 15.6 Hz, and around 42.5 Hz the result went negative. The
search starts one bin below 50 Hz, and only a refined frequency inside
50 Hz – 8 kHz counts.

On top of that, the peak must be louder than `silenceDb` (-70 dBFS) **and**
rise at least `minProminenceDb` (15 dB) above the median of the bins 4–12 bins
away from it. The last condition is not redundant: even in a quiet room the
StickS3 microphone brings in broadband noise at 5–8 kHz around -64 dB, and
with a level threshold alone the device would show a random frequency there.
It has been checked that this is not interference from the display (the noise
does not change with the screen off) and not an ADC artifact near Nyquist (at
48 kHz the noise stays at the same frequencies). The level threshold cannot be
raised instead: quiet speech sits at -50…-60 dB.

For voice and strings the strongest harmonic is often the second or third, so
the note can be an octave above the one you hear. This is deliberate: the
display shows the strongest peak, exactly as specified.

### Rendering

`Renderer` assembles the whole frame in an `M5Canvas` (a 240x135 sprite in
PSRAM) and pushes it with a single `pushSprite`. Drawing straight to the
screen flickers visibly. The spectrum changes every frame, so there is no
comparison with the previous frame.

M5GFX fonts are ASCII only: sharps are written as `#`, cents as `ct`.

### Screen rotation

Turning the device so that its other long edge points down flips the
picture: `setRotation(1)` ↔ `setRotation(3)`. Once per analysis frame
`main.cpp` reads `M5.Imu.getAccel()` (a BMI270; M5Unified enables it and
remaps its axes for the StickS3 on its own) and passes the `Orientation`
decision to `Renderer::setFlipped()`. The sign is verified on the board: with
ax > 0 rotation 1 is upright, with KEY1 to the right of the screen. The
decision follows the sign of ax when |ax| ≥ 0.6 g and must hold for 400 ms.
Lying flat on a table or standing on end does not change the orientation.
After the screen wakes, the first clear reading applies immediately.

## Board specifics

M5StickS3 — ESP32-S3-PICO-1-N8R8, 8 MB flash, 8 MB octal PSRAM, ST7789P3
135x240 display.

- PlatformIO has **no** `m5stack-sticks3` board id. The project uses
  `esp32-s3-devkitc-1` plus `board_build.arduino.memory_type = qio_opi` and
  the `default_8MB.csv` partitions. Do not "fix" this to a board id that does
  not exist.
- USB is native, with no CH9102 bridge, so on macOS the port is called
  `/dev/cu.usbmodem*`, not `/dev/cu.usbserial*`. Serial output needs the
  `-DARDUINO_USB_CDC_ON_BOOT=1` flag, which is already set.
- The buttons are taken: KEY1 on G11 (`M5.BtnA`) is pause, KEY2 on G12
  (`M5.BtnB`) is sensitivity. Grove (G9/G10) and HAT2 (G1-G8, G43, G44, G2,
  G3) are free.
- The microphone is connected through an ES8311 codec over I2S; M5Unified
  configures it on its own. The codec adds +32 dB of digital volume, hence
  `magnification = 2` (unity gain: M5Unified multiplies by
  `magnification / (2 * over_sampling)`). For the first second after power-up
  the ES8311 returns zeros, which shows on screen as silence.
- The speaker shares the I2S clock lines with the microphone, and the two
  cannot run at the same time. Hence `cfg.internal_spk = false`.
- The microphone's `noise_filter_level` is left at 0: it is a low-pass filter
  and would roll off the top of the spectrum.
- The signal chain barely passes fundamentals below ~60–70 Hz. On the board
  such a sound shows up at the frequency of its second or third harmonic, and
  the note an octave (or an octave and a fifth) higher. This is not a
  calculation error but the honest strongest peak of what came from the
  microphone. Likely causes: the MEMS microphone's low-frequency roll-off and
  the DC-removal filter in the ES8311 (M5Unified writes `0x1C = 0x6A`). A
  laptop speaker cannot check this: below ~100 Hz it barely makes a sound.

### Publishing to M5Burner

M5Burner writes the uploaded file starting at address 0x0, so it needs a full
image. The app-only `firmware.bin` belongs at 0x10000; flashed at 0x0 it
would overwrite the bootloader. `pio run -e sticks3 -t merged` (the
`tools/merged_image.py` extra script) stitches the bootloader, partition
table, `boot_app0` and the app into `.pio/build/sticks3/firmware-merged.bin`
with esptool `merge_bin`. It takes the offsets and flash parameters (dio,
80m, 8MB) from PlatformIO's own upload configuration, so the image matches
what `upload` writes.

How this was established: of six StickS3 firmwares on burner.m5stack.com,
five, including the official UIFlow2.0, are full images. Each has the
bootloader at 0x0 (header `e9 03 02 3f`), the partition table at 0x8000
(`aa 50`) and the app at 0x10000. One was app-only. The merged image was
verified by flashing it alone at 0x0 with esptool.

v1.0.0 was uploaded on 2026-09-27 and went to review (Public). The upload form
at burner.m5stack.com/developer/firmware/upload asks for:
- name, category (Audio & Media) and supported devices (StickS3);
- description and version description, both Markdown;
- version and project link;
- the `.bin` package;
- visibility: Public needs review;
- a cover image: the README screenshot works.

### The side button is handled by the PMIC, not the firmware

| Action | Result |
|---|---|
| Single press | Power on / reset |
| Double press | Power off |
| Long hold | Download mode (the internal green LED blinks) |

That is why the firmware has no software power-off, and **none should be
added**: the hardware already does it more reliably. On top of that,
`M5.Power.powerOff()` on the StickS3 has bug
[M5Unified#235](https://github.com/m5stack/M5Unified/issues/235): the board
powered off and immediately woke up again on a timer.

`M5.Power` does not set `_wakeupPin` for the StickS3, so there is no ready-made
wake-up from deep sleep by button either; it would have to be set up by hand
with `esp_sleep_enable_ext0_wakeup`.

### If flashing fails

`A fatal error occurred: Failed to connect to ESP32-S3: No serial data received.`

The board shows up as `USB JTAG_serial debug unit` (VID 0x303A, PID 0x1001),
the built-in USB-Serial-JTAG rather than a CDC port (a consequence of
`ARDUINO_USB_MODE=1`). Auto-reset into download mode through it does not
always work, and neither `--before usb_reset` nor `--before no_reset` helps.
The only cure is manual: hold the side button until the green LED blinks.

## Tests

All of `lib/` is covered by unit tests: the FFT and the analyzer are checked
against synthesized sines and noise, and the expected values in the tests are
worked out by hand. Rendering and `AudioInput` are checked on the board. Do
not try to write tests for `Renderer`: it would need a mock of all of
LovyanGFX and would prove nothing useful.

End-to-end check without a phone: play a tone through the Mac speaker
(`afplay` with a 440 Hz WAV) and watch the `peak` line in Serial.

`~/.platformio/penv/bin/python tools/screenshot.py docs/screenshot.png` grabs
the current frame over Serial: the firmware answers an `s` with a
`SNAP <w> <h>` line and the raw sprite, and the script writes a PNG scaled up
3x. The README screenshot was taken this way, with a chord playing through the
Mac speaker. Take a new one whenever the screen changes. The 65 KB transfer
takes longer than the mic queue holds, so the audio has a short gap right after
each screenshot.

The test `main` returns the failure count from `UNITY_END()`, and PlatformIO
shows a non-zero exit code as a signal number. A line like
`Program received signal SIGALRM` when tests fail is an artifact of the
report, not a separate problem; it disappears once the tests pass.
