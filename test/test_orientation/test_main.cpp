#include <unity.h>

#include "Orientation.h"

// Ускорение в g в осях платы. Стоит на длинной грани: тяжесть вдоль X.
static const float kNormal[] = {1.0f, 0.0f, 0.0f};
static const float kFlipped[] = {-1.0f, 0.0f, 0.0f};
static const float kFlat[] = {0.0f, 0.0f, 1.0f};
static const float kPortrait[] = {0.0f, 1.0f, 0.0f};

static constexpr uint32_t kSettle = Orientation::kSettleMs;

static bool feed(Orientation &o, uint32_t now, const float *a) {
    return o.update(now, a[0], a[1], a[2]);
}

// Уже определилась: стоит как обычно.
static Orientation settledNormal() {
    Orientation o;
    feed(o, 0, kNormal);
    return o;
}

void setUp(void) {}
void tearDown(void) {}

void test_is_not_flipped_before_any_reading(void) {
    Orientation o;
    TEST_ASSERT_FALSE(o.flipped());
}

void test_first_clear_reading_applies_at_once(void) {
    Orientation o;
    TEST_ASSERT_TRUE(feed(o, 0, kFlipped));
}

void test_flat_reading_does_not_decide_anything(void) {
    Orientation o;
    TEST_ASSERT_FALSE(feed(o, 0, kFlat));
    TEST_ASSERT_TRUE(feed(o, 20, kFlipped)); // первое ясное показание
}

void test_turning_over_flips_after_it_settles(void) {
    Orientation o = settledNormal();
    TEST_ASSERT_FALSE(feed(o, 1000, kFlipped));
    TEST_ASSERT_FALSE(feed(o, 1000 + kSettle - 1, kFlipped));
    TEST_ASSERT_TRUE(feed(o, 1000 + kSettle, kFlipped));
}

void test_turning_back_unflips(void) {
    Orientation o;
    feed(o, 0, kFlipped);
    feed(o, 1000, kNormal);
    TEST_ASSERT_FALSE(feed(o, 1000 + kSettle, kNormal));
}

void test_short_bump_does_not_flip(void) {
    Orientation o = settledNormal();
    feed(o, 1000, kFlipped);
    feed(o, 1200, kNormal); // вернули раньше, чем показание устоялось
    TEST_ASSERT_FALSE(feed(o, 1000 + kSettle, kFlipped));
    TEST_ASSERT_FALSE(feed(o, 1000 + kSettle + 100, kFlipped));
}

void test_lying_flat_keeps_the_orientation(void) {
    Orientation o;
    feed(o, 0, kFlipped);
    feed(o, 1000, kFlat);
    TEST_ASSERT_TRUE(feed(o, 1000 + kSettle * 10, kFlat));
}

void test_standing_upright_keeps_the_orientation(void) {
    Orientation o;
    feed(o, 0, kFlipped);
    feed(o, 1000, kPortrait);
    TEST_ASSERT_TRUE(feed(o, 1000 + kSettle * 10, kPortrait));
}

void test_weak_tilt_is_not_enough(void) {
    Orientation o = settledNormal();
    const float tilted[] = {-0.5f, 0.0f, 0.866f}; // на 60° от вертикали
    feed(o, 1000, tilted);
    TEST_ASSERT_FALSE(feed(o, 1000 + kSettle, tilted));
}

void test_shaking_is_ignored_and_restarts_the_wait(void) {
    Orientation o = settledNormal();
    const float shake[] = {-2.0f, 0.0f, 0.0f};
    feed(o, 1000, kFlipped);
    feed(o, 1200, shake);
    TEST_ASSERT_FALSE(feed(o, 1000 + kSettle, kFlipped));
    TEST_ASSERT_TRUE(feed(o, 1000 + kSettle + kSettle, kFlipped));
}

void test_reset_lets_the_next_clear_reading_apply_at_once(void) {
    Orientation o = settledNormal();
    o.reset();
    TEST_ASSERT_FALSE(o.flipped()); // до нового показания — прежняя
    TEST_ASSERT_TRUE(feed(o, 5000, kFlipped));
}

void test_settling_survives_the_millis_rollover(void) {
    Orientation o = settledNormal();
    const uint32_t start = 0xFFFFFF00u; // до переполнения 256 мс
    feed(o, start, kFlipped);
    TEST_ASSERT_FALSE(feed(o, start + kSettle - 1, kFlipped));
    TEST_ASSERT_TRUE(feed(o, start + kSettle, kFlipped));
}

int main(int, char **) {
    UNITY_BEGIN();

    RUN_TEST(test_is_not_flipped_before_any_reading);
    RUN_TEST(test_first_clear_reading_applies_at_once);
    RUN_TEST(test_flat_reading_does_not_decide_anything);
    RUN_TEST(test_turning_over_flips_after_it_settles);
    RUN_TEST(test_turning_back_unflips);
    RUN_TEST(test_short_bump_does_not_flip);
    RUN_TEST(test_lying_flat_keeps_the_orientation);
    RUN_TEST(test_standing_upright_keeps_the_orientation);
    RUN_TEST(test_weak_tilt_is_not_enough);
    RUN_TEST(test_shaking_is_ignored_and_restarts_the_wait);
    RUN_TEST(test_reset_lets_the_next_clear_reading_apply_at_once);
    RUN_TEST(test_settling_survives_the_millis_rollover);

    return UNITY_END();
}
