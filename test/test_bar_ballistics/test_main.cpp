#include <unity.h>

#include "BarBallistics.h"

// Скорости в тестах свои, круглые, чтобы ожидания считались в уме:
// столбик падает на 100 дБ/с, колпачок висит 500 мс и падает на 50 дБ/с.

static const float kFloor = -120.0f;
static const float kTolerance = 0.01f;

static BarBallistics makeBars() {
    BallisticsConfig config;
    config.releaseDbPerSec = 100.0f;
    config.capHoldMs = 500;
    config.capReleaseDbPerSec = 50.0f;
    return BarBallistics(1, kFloor, config);
}

static void feed(BarBallistics &bars, float db, uint32_t nowMs) {
    bars.update(&db, nowMs);
}

void setUp(void) {}
void tearDown(void) {}

void test_bar_jumps_up_to_a_louder_level_at_once(void) {
    BarBallistics bars = makeBars();

    feed(bars, -20.0f, 1000);

    TEST_ASSERT_FLOAT_WITHIN(kTolerance, -20.0f, bars.level(0));
}

// 100 мс при 100 дБ/с — минус 10 дБ.
void test_bar_falls_at_the_release_rate(void) {
    BarBallistics bars = makeBars();
    feed(bars, -20.0f, 1000);

    feed(bars, kFloor, 1100);

    TEST_ASSERT_FLOAT_WITHIN(kTolerance, -30.0f, bars.level(0));
}

void test_louder_input_interrupts_the_fall(void) {
    BarBallistics bars = makeBars();
    feed(bars, -20.0f, 1000);
    feed(bars, kFloor, 1100);

    feed(bars, -10.0f, 1200);

    TEST_ASSERT_FLOAT_WITHIN(kTolerance, -10.0f, bars.level(0));
}

// Через 400 мс столбик уже на -60, а колпачок ещё держит -20.
void test_cap_holds_the_peak_while_the_bar_falls(void) {
    BarBallistics bars = makeBars();
    feed(bars, -20.0f, 0);

    feed(bars, kFloor, 400);

    TEST_ASSERT_FLOAT_WITHIN(kTolerance, -60.0f, bars.level(0));
    TEST_ASSERT_FLOAT_WITHIN(kTolerance, -20.0f, bars.cap(0));
}

// Удержание кончилось на 500 мс, ещё 500 мс по 50 дБ/с — минус 25 дБ.
void test_cap_falls_after_the_hold_time(void) {
    BarBallistics bars = makeBars();
    feed(bars, -20.0f, 0);

    feed(bars, kFloor, 1000);

    TEST_ASSERT_FLOAT_WITHIN(kTolerance, -45.0f, bars.cap(0));
}

void test_rising_bar_pushes_the_cap_up(void) {
    BarBallistics bars = makeBars();
    feed(bars, -20.0f, 0);

    feed(bars, -10.0f, 100);

    TEST_ASSERT_FLOAT_WITHIN(kTolerance, -10.0f, bars.cap(0));
}

// Новый пик заново запускает удержание: через 400 мс после него
// колпачок ещё не падает, хотя от первого пика прошло 700 мс.
void test_new_peak_restarts_the_hold(void) {
    BarBallistics bars = makeBars();
    feed(bars, -20.0f, 0);
    feed(bars, -15.0f, 300);

    feed(bars, kFloor, 700);

    TEST_ASSERT_FLOAT_WITHIN(kTolerance, -15.0f, bars.cap(0));
}

void test_long_silence_stops_at_the_floor(void) {
    BarBallistics bars = makeBars();
    feed(bars, -20.0f, 0);

    feed(bars, kFloor, 10000);

    TEST_ASSERT_FLOAT_WITHIN(kTolerance, kFloor, bars.level(0));
    TEST_ASSERT_FLOAT_WITHIN(kTolerance, kFloor, bars.cap(0));
}

// millis() переполняется примерно через 49 суток: с 2^32 - 50 до 50
// прошло 100 мс, а не минус четыре миллиарда.
void test_fall_survives_millis_overflow(void) {
    BarBallistics bars = makeBars();
    feed(bars, -20.0f, 0xFFFFFFCEu);

    feed(bars, kFloor, 50);

    TEST_ASSERT_FLOAT_WITHIN(kTolerance, -30.0f, bars.level(0));
}

void test_bands_move_independently(void) {
    BallisticsConfig config;
    config.releaseDbPerSec = 100.0f;
    BarBallistics bars(2, kFloor, config);
    const float first[2] = {-20.0f, -50.0f};
    const float second[2] = {kFloor, -5.0f};

    bars.update(first, 0);
    bars.update(second, 100);

    TEST_ASSERT_FLOAT_WITHIN(kTolerance, -30.0f, bars.level(0));
    TEST_ASSERT_FLOAT_WITHIN(kTolerance, -5.0f, bars.level(1));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_bar_jumps_up_to_a_louder_level_at_once);
    RUN_TEST(test_bar_falls_at_the_release_rate);
    RUN_TEST(test_louder_input_interrupts_the_fall);

    RUN_TEST(test_cap_holds_the_peak_while_the_bar_falls);
    RUN_TEST(test_cap_falls_after_the_hold_time);
    RUN_TEST(test_rising_bar_pushes_the_cap_up);
    RUN_TEST(test_new_peak_restarts_the_hold);

    RUN_TEST(test_long_silence_stops_at_the_floor);
    RUN_TEST(test_fall_survives_millis_overflow);
    RUN_TEST(test_bands_move_independently);

    return UNITY_END();
}
