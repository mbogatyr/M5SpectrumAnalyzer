#include <unity.h>

#include <math.h>

#include "Fft.h"

// Ожидаемые значения выведены руками из определения ДПФ:
// X[k] = sum x[n] * exp(-2*pi*i*k*n/N).

static const float kPi = 3.14159265358979f;
static const float kTolerance = 1e-3f;

void setUp(void) {}
void tearDown(void) {}

void test_impulse_at_zero_gives_a_flat_real_spectrum(void) {
    Fft fft(8);
    float re[8] = {1, 0, 0, 0, 0, 0, 0, 0};
    float im[8] = {0};

    fft.transform(re, im);

    for (int k = 0; k < 8; ++k) {
        TEST_ASSERT_FLOAT_WITHIN(kTolerance, 1.0f, re[k]);
        TEST_ASSERT_FLOAT_WITHIN(kTolerance, 0.0f, im[k]);
    }
}

void test_constant_lands_entirely_in_bin_zero(void) {
    Fft fft(8);
    float re[8] = {1, 1, 1, 1, 1, 1, 1, 1};
    float im[8] = {0};

    fft.transform(re, im);

    TEST_ASSERT_FLOAT_WITHIN(kTolerance, 8.0f, re[0]);
    for (int k = 1; k < 8; ++k) {
        TEST_ASSERT_FLOAT_WITHIN(kTolerance, 0.0f, re[k]);
        TEST_ASSERT_FLOAT_WITHIN(kTolerance, 0.0f, im[k]);
    }
}

// Импульс, сдвинутый на один отсчёт, при N=4 даёт exp(-i*pi*k/2):
// 1, -i, -1, i. Ловит ошибки перестановки и знака поворота.
void test_impulse_at_one_rotates_clockwise(void) {
    Fft fft(4);
    float re[4] = {0, 1, 0, 0};
    float im[4] = {0};

    fft.transform(re, im);

    const float wantRe[4] = {1, 0, -1, 0};
    const float wantIm[4] = {0, -1, 0, 1};
    for (int k = 0; k < 4; ++k) {
        TEST_ASSERT_FLOAT_WITHIN(kTolerance, wantRe[k], re[k]);
        TEST_ASSERT_FLOAT_WITHIN(kTolerance, wantIm[k], im[k]);
    }
}

void test_cosine_on_a_bin_splits_into_two_real_peaks(void) {
    Fft fft(16);
    float re[16];
    float im[16] = {0};
    for (int n = 0; n < 16; ++n) {
        re[n] = cosf(2.0f * kPi * 3.0f * n / 16.0f);
    }

    fft.transform(re, im);

    for (int k = 0; k < 16; ++k) {
        const float want = (k == 3 || k == 13) ? 8.0f : 0.0f;
        TEST_ASSERT_FLOAT_WITHIN(kTolerance, want, re[k]);
        TEST_ASSERT_FLOAT_WITHIN(kTolerance, 0.0f, im[k]);
    }
}

// sin = (e^{ix} - e^{-ix}) / 2i, поэтому в бине k лежит -i*N/2,
// а в зеркальном +i*N/2. Обратное преобразование дало бы знаки наоборот.
void test_sine_on_a_bin_puts_negative_imaginary_part_in_bin_k(void) {
    Fft fft(16);
    float re[16];
    float im[16] = {0};
    for (int n = 0; n < 16; ++n) {
        re[n] = sinf(2.0f * kPi * 5.0f * n / 16.0f);
    }

    fft.transform(re, im);

    TEST_ASSERT_FLOAT_WITHIN(kTolerance, -8.0f, im[5]);
    TEST_ASSERT_FLOAT_WITHIN(kTolerance, 8.0f, im[11]);
    TEST_ASSERT_FLOAT_WITHIN(kTolerance, 0.0f, re[5]);
    TEST_ASSERT_FLOAT_WITHIN(kTolerance, 0.0f, im[4]);
}

// Размер, который пойдёт в прошивку: одиннадцать стадий бабочек.
void test_full_size_transform_finds_a_cosine_on_bin_100(void) {
    const int n = 2048;
    Fft fft(n);
    static float re[n];
    static float im[n];
    for (int i = 0; i < n; ++i) {
        re[i] = cosf(2.0f * kPi * 100.0f * i / n);
        im[i] = 0.0f;
    }

    fft.transform(re, im);

    TEST_ASSERT_FLOAT_WITHIN(0.5f, 1024.0f, re[100]);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 1024.0f, re[n - 100]);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, re[99]);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, re[101]);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, re[0]);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_impulse_at_zero_gives_a_flat_real_spectrum);
    RUN_TEST(test_constant_lands_entirely_in_bin_zero);
    RUN_TEST(test_impulse_at_one_rotates_clockwise);
    RUN_TEST(test_cosine_on_a_bin_splits_into_two_real_peaks);
    RUN_TEST(test_sine_on_a_bin_puts_negative_imaginary_part_in_bin_k);
    RUN_TEST(test_full_size_transform_finds_a_cosine_on_bin_100);

    return UNITY_END();
}
