"""Grabs the analyzer's screen over USB and saves it as a PNG.

The firmware answers an 's' on Serial with a "SNAP <width> <height>" line
followed by the raw frame: RGB565 pixels, high byte first. This script sends
the request, reads the frame, and writes a PNG scaled up by an integer
factor with nearest-neighbor sampling, so the display's pixels stay crisp.

Needs only pyserial, which ships with PlatformIO:

    ~/.platformio/penv/bin/python tools/screenshot.py docs/screenshot.png
"""
import argparse
import glob
import struct
import sys
import time
import zlib

import serial


def find_port() -> str:
    ports = sorted(glob.glob("/dev/cu.usbmodem*"))
    if not ports:
        sys.exit("No /dev/cu.usbmodem* port found; is the board connected?")
    return ports[0]


def read_frame(port: str, timeout_s: float = 5.0):
    with serial.Serial(port, 115200, timeout=0.2) as link:
        link.reset_input_buffer()
        link.write(b"s")
        deadline = time.time() + timeout_s
        buf = b""
        # Skip the once-a-second status lines until the header shows up.
        while b"SNAP " not in buf or b"\n" not in buf[buf.index(b"SNAP ") :]:
            if time.time() > deadline:
                sys.exit("No SNAP header from the board.")
            buf += link.read(4096)
        start = buf.index(b"SNAP ")
        end = buf.index(b"\n", start)
        width, height = map(int, buf[start + 5 : end].split())
        pixels = buf[end + 1 :]
        size = width * height * 2
        while len(pixels) < size:
            if time.time() > deadline:
                sys.exit(f"Frame cut short: {len(pixels)} of {size} bytes.")
            pixels += link.read(size - len(pixels))
        return width, height, pixels[:size]


def rgb_rows(width, height, pixels, scale):
    for y in range(height):
        row = bytearray()
        for x in range(width):
            i = 2 * (y * width + x)
            v = (pixels[i] << 8) | pixels[i + 1]
            # Expand 5/6/5 bits to 8 by repeating the top bits.
            r = (v >> 11) & 0x1F
            g = (v >> 5) & 0x3F
            b = v & 0x1F
            row += bytes(((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2))) * scale
        for _ in range(scale):
            yield bytes(row)


def write_png(path, width, height, rows):
    def chunk(kind, data):
        body = kind + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body))

    raw = b"".join(b"\x00" + row for row in rows)
    header = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", header))
        f.write(chunk(b"IDAT", zlib.compress(raw, 9)))
        f.write(chunk(b"IEND", b""))


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("output", help="PNG file to write")
    parser.add_argument("--port", help="serial port (default: first /dev/cu.usbmodem*)")
    parser.add_argument("--scale", type=int, default=3, help="integer upscale factor")
    args = parser.parse_args()

    width, height, pixels = read_frame(args.port or find_port())
    rows = rgb_rows(width, height, pixels, args.scale)
    write_png(args.output, width * args.scale, height * args.scale, rows)
    print(f"{args.output}: {width}x{height} frame, saved at {args.scale}x")


if __name__ == "__main__":
    main()
