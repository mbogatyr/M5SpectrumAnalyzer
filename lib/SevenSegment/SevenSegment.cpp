#include "SevenSegment.h"

uint8_t segmentsForDigit(uint8_t digit) {
    switch (digit) {
    case 0:
        return SegA | SegB | SegC | SegD | SegE | SegF;
    case 1:
        return SegB | SegC;
    case 2:
        return SegA | SegB | SegG | SegE | SegD;
    case 3:
        return SegA | SegB | SegG | SegC | SegD;
    case 4:
        return SegF | SegG | SegB | SegC;
    case 5:
        return SegA | SegF | SegG | SegC | SegD;
    case 6:
        return SegA | SegF | SegG | SegE | SegD | SegC;
    case 7:
        return SegA | SegB | SegC;
    case 8:
        return SegA | SegB | SegC | SegD | SegE | SegF | SegG;
    case 9:
        return SegA | SegB | SegC | SegD | SegF | SegG;
    default:
        return 0;
    }
}
