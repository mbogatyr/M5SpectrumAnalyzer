#pragma once

#include <stdint.h>

// Сегменты семисегментного знакоместа:
//
//    aaa
//   f   b
//    ggg
//   e   c
//    ddd
enum Segment : uint8_t {
    SegA = 1 << 0,
    SegB = 1 << 1,
    SegC = 1 << 2,
    SegD = 1 << 3,
    SegE = 1 << 4,
    SegF = 1 << 5,
    SegG = 1 << 6,
};

// Маска сегментов для цифры 0-9. Для всего остального — 0.
uint8_t segmentsForDigit(uint8_t digit);
