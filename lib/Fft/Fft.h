#pragma once

#include <stddef.h>
#include <stdint.h>

#include <vector>

// Forward complex radix-2 FFT, in place.
//
// Negative exponent: X[k] = sum x[n] * exp(-2*pi*i*k*n/N), unnormalized.
// A cosine of amplitude A at bin k gives A*N/2 in bins k and N-k.
//
// The twiddle-factor and bit-reversal tables are built once in the
// constructor, so transform() calls neither sin nor cos and does not
// allocate memory.
class Fft {
  public:
    // n is a power of two, at least 2.
    explicit Fft(size_t n);

    size_t size() const { return n_; }

    // re and im hold n elements each. On return they contain the spectrum.
    void transform(float *re, float *im) const;

  private:
    size_t n_;
    std::vector<float> cos_; // n/2 values of cos(2*pi*k/n)
    std::vector<float> sin_; // n/2 values of sin(2*pi*k/n)
    std::vector<uint32_t> bitReversed_;
};
