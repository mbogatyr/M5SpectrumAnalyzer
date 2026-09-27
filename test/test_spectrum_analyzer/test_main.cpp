#include <unity.h>

#include <math.h>
#include <stdio.h>

#include "SpectrumAnalyzer.h"

// Настройки по умолчанию: 16 кГц, 2048 точек, шаг бина 7.8125 Гц,
// 48 полос от 50 Гц до 8 кГц. Номера полос и бинов в тестах посчитаны
// вручную по формулам из комментариев.

static const int kN = 2048;
static const float kRate = 16000.0f;
static const float kPi = 3.14159265358979f;

static int16_t samples[kN];

static void fillSilence() {
    for (int i = 0; i < kN; ++i) {
        samples[i] = 0;
    }
}

static void addSine(float hz, float amplitude) {
    for (int i = 0; i < kN; ++i) {
        const float value =
            samples[i] + amplitude * sinf(2.0f * kPi * hz * i / kRate);
        samples[i] = static_cast<int16_t>(lroundf(value));
    }
}

// Детерминированный белый шум: линейный конгруэнтный генератор,
// равномерно от -amplitude до +amplitude.
static void addNoise(float amplitude) {
    uint32_t state = 12345;
    for (int i = 0; i < kN; ++i) {
        state = state * 1103515245u + 12345u;
        const float unit = ((state >> 8) & 0xFFFF) / 32767.5f - 1.0f;
        samples[i] = static_cast<int16_t>(lroundf(samples[i] + amplitude * unit));
    }
}

static size_t loudestBand(const Spectrum &s) {
    size_t best = 0;
    for (size_t i = 1; i < s.bandDb.size(); ++i) {
        if (s.bandDb[i] > s.bandDb[best]) {
            best = i;
        }
    }
    return best;
}

void setUp(void) { fillSilence(); }
void tearDown(void) {}

// 500 Гц — ровно бин 64, без растекания между бинами.
void test_tone_on_a_bin_is_found_exactly(void) {
    SpectrumAnalyzer analyzer;
    addSine(500.0f, 16384.0f);

    const Spectrum &s = analyzer.analyze(samples);

    TEST_ASSERT_TRUE(s.hasPeak);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 500.0f, s.peakHz);
}

// Полшкалы: 20*log10(0.5) = -6.02 дБFS.
void test_half_scale_tone_reads_minus_six_db(void) {
    SpectrumAnalyzer analyzer;
    addSine(500.0f, 16384.0f);

    const Spectrum &s = analyzer.analyze(samples);

    TEST_ASSERT_FLOAT_WITHIN(0.2f, -6.02f, s.peakDb);
}

// 440 Гц — это 56.32 бина: без интерполяции ответ был бы 437.5 Гц.
void test_concert_a_is_interpolated_between_bins(void) {
    SpectrumAnalyzer analyzer;
    addSine(440.0f, 8000.0f);

    const Spectrum &s = analyzer.analyze(samples);

    TEST_ASSERT_FLOAT_WITHIN(0.5f, 440.0f, s.peakHz);
}

// 1234.5 Гц — это 158.02 бина, ещё одна точка вне сетки.
void test_high_tone_off_the_grid_is_within_one_hz(void) {
    SpectrumAnalyzer analyzer;
    addSine(1234.5f, 8000.0f);

    const Spectrum &s = analyzer.analyze(samples);

    TEST_ASSERT_FLOAT_WITHIN(1.0f, 1234.5f, s.peakHz);
}

void test_the_louder_of_two_tones_wins(void) {
    SpectrumAnalyzer analyzer;
    addSine(300.0f, 4000.0f);
    addSine(2000.0f, 12000.0f);

    const Spectrum &s = analyzer.analyze(samples);

    TEST_ASSERT_FLOAT_WITHIN(1.0f, 2000.0f, s.peakHz);
}

void test_silence_has_no_peak(void) {
    SpectrumAnalyzer analyzer;

    const Spectrum &s = analyzer.analyze(samples);

    TEST_ASSERT_FALSE(s.hasPeak);
}

// Амплитуда 3 — это 20*log10(3/32768) = -80.8 дБFS, ниже порога -70.
void test_tone_below_the_silence_threshold_has_no_peak(void) {
    SpectrumAnalyzer analyzer;
    addSine(1000.0f, 3.0f);

    const Spectrum &s = analyzer.analyze(samples);

    TEST_ASSERT_FALSE(s.hasPeak);
}

// Постоянная составляющая лежит в бине 0, а поиск начинается с 50 Гц.
void test_dc_offset_is_not_a_peak(void) {
    SpectrumAnalyzer analyzer;
    for (int i = 0; i < kN; ++i) {
        samples[i] = 5000;
    }

    const Spectrum &s = analyzer.analyze(samples);

    TEST_ASSERT_FALSE(s.hasPeak);
}

// 440 Гц лежит на 0.32 бина от сетки: без поправки высоты по параболе
// окно Ханна занизило бы уровень на 0.58 дБ.
void test_level_of_an_off_grid_tone_is_corrected(void) {
    SpectrumAnalyzer analyzer;
    addSine(440.0f, 16384.0f);

    const Spectrum &s = analyzer.analyze(samples);

    TEST_ASSERT_FLOAT_WITHIN(0.25f, -6.02f, s.peakDb);
}

// Громкий тон ниже 50 Гц в диапазон не попадает, но его склон
// заходит в нижние бины. Склон — не вершина: пика быть не должно, и уж
// тем более не должно быть выдуманной частоты. Раньше здесь получалось
// 15.6 Гц при 43 Гц на входе и даже отрицательные частоты около 42.5 Гц.
void test_loud_tone_just_below_the_range_is_not_a_peak(void) {
    SpectrumAnalyzer analyzer;
    for (float hz = 30.0f; hz <= 49.5f; hz += 0.05f) {
        fillSilence();
        addSine(hz, 8231.0f); // -12 дБFS

        const Spectrum &s = analyzer.analyze(samples);

        if (s.hasPeak) {
            char message[48];
            snprintf(message, sizeof message, "tone %.2f Hz gave %.2f Hz", hz,
                     s.peakHz);
            TEST_FAIL_MESSAGE(message);
        }
    }
}

// 50.5 Гц — это 6.46 бина: вершина в бине 6, то есть на 46.9 Гц, ниже
// диапазона, но уточнённая частота уже внутри него.
void test_tone_just_above_the_lower_edge_is_found(void) {
    SpectrumAnalyzer analyzer;
    addSine(50.5f, 8231.0f);

    const Spectrum &s = analyzer.analyze(samples);

    TEST_ASSERT_TRUE(s.hasPeak);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 50.5f, s.peakHz);
}

// Шум заметно громче порога тишины (-70 дБ), но тона в нём нет: самый
// громкий бин случаен, и показывать его частоту бессмысленно. Так на
// плате выглядел шум 5-8 кГц, который прыгал между 7.1 и 7.4 кГц.
void test_broadband_noise_is_not_a_peak(void) {
    SpectrumAnalyzer analyzer;
    addNoise(1000.0f);

    const Spectrum &s = analyzer.analyze(samples);

    TEST_ASSERT_FALSE(s.hasPeak);
}

void test_tone_rising_out_of_noise_is_still_found(void) {
    SpectrumAnalyzer analyzer;
    addNoise(1000.0f);
    addSine(1000.0f, 8000.0f);

    const Spectrum &s = analyzer.analyze(samples);

    TEST_ASSERT_TRUE(s.hasPeak);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 1000.0f, s.peakHz);
}

// Гласная низкого голоса: основной тон 110 Гц, самая громкая — вторая
// гармоника. Соседние гармоники в 14 бинах от пика не должны
// засчитываться в его «фон».
void test_harmonics_of_a_low_voice_are_a_peak(void) {
    SpectrumAnalyzer analyzer;
    addNoise(200.0f);
    addSine(110.0f, 3000.0f);
    addSine(220.0f, 6000.0f);
    addSine(330.0f, 4000.0f);
    addSine(440.0f, 2500.0f);
    addSine(550.0f, 1500.0f);

    const Spectrum &s = analyzer.analyze(samples);

    TEST_ASSERT_TRUE(s.hasPeak);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 220.0f, s.peakHz);
}

// Частота вне диапазона 50 Гц - 8 кГц не должна становиться пиком.
void test_hum_below_the_range_does_not_steal_the_peak(void) {
    SpectrumAnalyzer analyzer;
    addSine(25.0f, 16000.0f);
    addSine(700.0f, 2000.0f);

    const Spectrum &s = analyzer.analyze(samples);

    TEST_ASSERT_FLOAT_WITHIN(1.0f, 700.0f, s.peakHz);
}

// Полоса i: floor(48 * ln(f/50) / ln(160)). Для 1 кГц это 28.33 -> 28.
void test_peak_band_is_where_the_frequency_sits_on_the_log_axis(void) {
    SpectrumAnalyzer analyzer;
    addSine(1000.0f, 8000.0f);

    const Spectrum &s = analyzer.analyze(samples);

    TEST_ASSERT_EQUAL_UINT32(28, s.peakBand);
}

// Позиция 30.70 на шкале полос: номер полосы — целая часть, а не
// округление (иначе была бы 31).
void test_peak_band_takes_the_whole_part_of_the_position(void) {
    SpectrumAnalyzer analyzer;
    addSine(1284.4f, 8000.0f);

    const Spectrum &s = analyzer.analyze(samples);

    TEST_ASSERT_EQUAL_UINT32(30, s.peakBand);
}

// В белом шуме каждой полосе есть что показать: ни одна не остаётся
// на полу, в том числе узкие полосы на низах.
void test_noise_lights_every_band(void) {
    SpectrumAnalyzer analyzer;
    addNoise(1000.0f);

    const Spectrum &s = analyzer.analyze(samples);

    for (size_t i = 0; i < s.bandDb.size(); ++i) {
        TEST_ASSERT_TRUE(s.bandDb[i] > SpectrumAnalyzer::kFloorDb + 20.0f);
    }
}

void test_tone_lights_its_own_band_brightest(void) {
    SpectrumAnalyzer analyzer;
    addSine(1000.0f, 8000.0f);

    const Spectrum &s = analyzer.analyze(samples);

    TEST_ASSERT_EQUAL_UINT32(48, s.bandDb.size());
    TEST_ASSERT_EQUAL_UINT32(28, loudestBand(s));
    TEST_ASSERT_TRUE(s.bandDb[28] - s.bandDb[5] > 40.0f);
    TEST_ASSERT_TRUE(s.bandDb[28] - s.bandDb[45] > 40.0f);
}

// Полоса 1 — от 55.58 до 61.77 Гц, уже шага бина: собственного бина в
// ней нет. Тон 58.6 Гц должен всё равно её зажечь, а не оставить
// провалом в столбиках.
void test_band_narrower_than_a_bin_still_shows_its_tone(void) {
    SpectrumAnalyzer analyzer;
    addSine(58.6f, 8000.0f);

    const Spectrum &s = analyzer.analyze(samples);

    TEST_ASSERT_TRUE(s.bandDb[1] > s.peakDb - 6.0f);
}

void test_silence_leaves_every_band_at_the_floor(void) {
    SpectrumAnalyzer analyzer;
    addSine(1000.0f, 8000.0f);
    analyzer.analyze(samples);

    fillSilence();
    const Spectrum &s = analyzer.analyze(samples);

    for (size_t i = 0; i < s.bandDb.size(); ++i) {
        TEST_ASSERT_FLOAT_WITHIN(0.01f, SpectrumAnalyzer::kFloorDb,
                                 s.bandDb[i]);
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_tone_on_a_bin_is_found_exactly);
    RUN_TEST(test_half_scale_tone_reads_minus_six_db);
    RUN_TEST(test_concert_a_is_interpolated_between_bins);
    RUN_TEST(test_high_tone_off_the_grid_is_within_one_hz);
    RUN_TEST(test_level_of_an_off_grid_tone_is_corrected);
    RUN_TEST(test_the_louder_of_two_tones_wins);

    RUN_TEST(test_silence_has_no_peak);
    RUN_TEST(test_tone_below_the_silence_threshold_has_no_peak);
    RUN_TEST(test_dc_offset_is_not_a_peak);
    RUN_TEST(test_broadband_noise_is_not_a_peak);
    RUN_TEST(test_tone_rising_out_of_noise_is_still_found);
    RUN_TEST(test_harmonics_of_a_low_voice_are_a_peak);
    RUN_TEST(test_hum_below_the_range_does_not_steal_the_peak);
    RUN_TEST(test_loud_tone_just_below_the_range_is_not_a_peak);
    RUN_TEST(test_tone_just_above_the_lower_edge_is_found);

    RUN_TEST(test_peak_band_is_where_the_frequency_sits_on_the_log_axis);
    RUN_TEST(test_peak_band_takes_the_whole_part_of_the_position);
    RUN_TEST(test_tone_lights_its_own_band_brightest);
    RUN_TEST(test_noise_lights_every_band);
    RUN_TEST(test_band_narrower_than_a_bin_still_shows_its_tone);
    RUN_TEST(test_silence_leaves_every_band_at_the_floor);

    return UNITY_END();
}
