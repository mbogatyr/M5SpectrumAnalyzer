#pragma once

#include <stddef.h>
#include <stdint.h>

#include <vector>

// Прямое комплексное БПФ по основанию 2, на месте.
//
// Знак экспоненты отрицательный: X[k] = sum x[n] * exp(-2*pi*i*k*n/N),
// без нормировки. Косинус амплитуды A на бине k даёт A*N/2 в бинах k и N-k.
//
// Таблицы поворотных множителей и бит-реверса строятся один раз в
// конструкторе, так что transform() не вызывает ни sin, ни cos и не
// выделяет память.
class Fft {
  public:
    // n — степень двойки, не меньше 2.
    explicit Fft(size_t n);

    size_t size() const { return n_; }

    // re и im — по n элементов. На выходе в них спектр.
    void transform(float *re, float *im) const;

  private:
    size_t n_;
    std::vector<float> cos_; // n/2 значений cos(2*pi*k/n)
    std::vector<float> sin_; // n/2 значений sin(2*pi*k/n)
    std::vector<uint32_t> bitReversed_;
};
