#include "Fft.h"

#include <math.h>

#include <utility>

Fft::Fft(size_t n) : n_(n), cos_(n / 2), sin_(n / 2), bitReversed_(n) {
    // The tables are computed in double: at 2048 points, float error in the
    // argument noticeably degrades the twiddle factors at large k.
    const double kTwoPi = 6.283185307179586;
    for (size_t k = 0; k < n / 2; ++k) {
        cos_[k] = static_cast<float>(cos(kTwoPi * k / n));
        sin_[k] = static_cast<float>(sin(kTwoPi * k / n));
    }

    unsigned bits = 0;
    while ((static_cast<size_t>(1) << bits) < n) {
        ++bits;
    }
    for (size_t i = 0; i < n; ++i) {
        uint32_t reversed = 0;
        for (unsigned b = 0; b < bits; ++b) {
            if (i & (static_cast<size_t>(1) << b)) {
                reversed |= 1u << (bits - 1 - b);
            }
        }
        bitReversed_[i] = reversed;
    }
}

void Fft::transform(float *re, float *im) const {
    for (size_t i = 0; i < n_; ++i) {
        const size_t j = bitReversed_[i];
        if (j > i) {
            std::swap(re[i], re[j]);
            std::swap(im[i], im[j]);
        }
    }

    // Decimation-in-time butterflies: the block length doubles at each
    // stage, and the twiddle factor exp(-2*pi*i*k/len) is taken from the
    // shared table with a stride of n/len.
    for (size_t len = 2; len <= n_; len <<= 1) {
        const size_t half = len / 2;
        const size_t step = n_ / len;
        for (size_t start = 0; start < n_; start += len) {
            for (size_t k = 0; k < half; ++k) {
                const float wr = cos_[k * step];
                const float wi = -sin_[k * step];
                const size_t a = start + k;
                const size_t b = a + half;
                const float tr = re[b] * wr - im[b] * wi;
                const float ti = re[b] * wi + im[b] * wr;
                re[b] = re[a] - tr;
                im[b] = im[a] - ti;
                re[a] += tr;
                im[a] += ti;
            }
        }
    }
}
