#include <unity.h>

#include "TrafficLight.h"

// Длительности по умолчанию: red 8000, redYellow 2000, green 8000,
// greenBlink 3000, yellow 3000 => полный цикл 24000 мс.
static TrafficLight defaultLight() { return TrafficLight(); }

void setUp(void) {}
void tearDown(void) {}

// --- порядок и границы фаз ---

void test_cycle_starts_with_red(void) {
    TrafficLight light = defaultLight();
    TEST_ASSERT_EQUAL(static_cast<int>(Phase::Red),
                      static_cast<int>(light.phaseAt(0)));
}

void test_red_holds_until_its_last_millisecond(void) {
    TrafficLight light = defaultLight();
    TEST_ASSERT_EQUAL(static_cast<int>(Phase::Red),
                      static_cast<int>(light.phaseAt(7999)));
}

void test_red_yellow_begins_exactly_when_red_ends(void) {
    TrafficLight light = defaultLight();
    TEST_ASSERT_EQUAL(static_cast<int>(Phase::RedYellow),
                      static_cast<int>(light.phaseAt(8000)));
}

void test_phases_follow_the_documented_order(void) {
    TrafficLight light = defaultLight();
    TEST_ASSERT_EQUAL(static_cast<int>(Phase::RedYellow),
                      static_cast<int>(light.phaseAt(9000)));
    TEST_ASSERT_EQUAL(static_cast<int>(Phase::Green),
                      static_cast<int>(light.phaseAt(10000)));
    TEST_ASSERT_EQUAL(static_cast<int>(Phase::GreenBlink),
                      static_cast<int>(light.phaseAt(18000)));
    TEST_ASSERT_EQUAL(static_cast<int>(Phase::Yellow),
                      static_cast<int>(light.phaseAt(21000)));
}

void test_cycle_wraps_back_to_red(void) {
    TrafficLight light = defaultLight();
    TEST_ASSERT_EQUAL(static_cast<int>(Phase::Red),
                      static_cast<int>(light.phaseAt(24000)));
    TEST_ASSERT_EQUAL(static_cast<int>(light.phaseAt(5000)),
                      static_cast<int>(light.phaseAt(24000 + 5000)));
}

void test_cycle_length_is_the_sum_of_all_phases(void) {
    TrafficLight light = defaultLight();
    TEST_ASSERT_EQUAL_UINT32(24000, light.cycleMs());
}

// --- обратный отсчёт ---

void test_countdown_shows_full_phase_length_at_its_start(void) {
    TrafficLight light = defaultLight();
    TEST_ASSERT_EQUAL_UINT8(8, light.secondsRemainingAt(0));
}

void test_countdown_ticks_down_once_per_second(void) {
    TrafficLight light = defaultLight();
    TEST_ASSERT_EQUAL_UINT8(8, light.secondsRemainingAt(999));
    TEST_ASSERT_EQUAL_UINT8(7, light.secondsRemainingAt(1000));
}

void test_countdown_shows_one_through_the_final_second(void) {
    TrafficLight light = defaultLight();
    TEST_ASSERT_EQUAL_UINT8(1, light.secondsRemainingAt(7000));
    TEST_ASSERT_EQUAL_UINT8(1, light.secondsRemainingAt(7999));
}

void test_countdown_restarts_on_the_next_phase(void) {
    TrafficLight light = defaultLight();
    // redYellow длится 2000 мс
    TEST_ASSERT_EQUAL_UINT8(2, light.secondsRemainingAt(8000));
    TEST_ASSERT_EQUAL_UINT8(1, light.secondsRemainingAt(9000));
}

// --- лампы ---

void test_red_phase_lights_only_red(void) {
    Lamps lamps = defaultLight().lampsAt(0);
    TEST_ASSERT_TRUE(lamps.red);
    TEST_ASSERT_FALSE(lamps.yellow);
    TEST_ASSERT_FALSE(lamps.green);
}

void test_red_yellow_phase_lights_both(void) {
    Lamps lamps = defaultLight().lampsAt(8500);
    TEST_ASSERT_TRUE(lamps.red);
    TEST_ASSERT_TRUE(lamps.yellow);
    TEST_ASSERT_FALSE(lamps.green);
}

void test_green_phase_lights_only_green(void) {
    Lamps lamps = defaultLight().lampsAt(12000);
    TEST_ASSERT_FALSE(lamps.red);
    TEST_ASSERT_FALSE(lamps.yellow);
    TEST_ASSERT_TRUE(lamps.green);
}

void test_yellow_phase_lights_only_yellow(void) {
    Lamps lamps = defaultLight().lampsAt(21500);
    TEST_ASSERT_FALSE(lamps.red);
    TEST_ASSERT_TRUE(lamps.yellow);
    TEST_ASSERT_FALSE(lamps.green);
}

// --- мигание зелёного ---

void test_blinking_green_is_lit_at_the_start_of_the_phase(void) {
    // greenBlink начинается на 18000 мс
    TEST_ASSERT_TRUE(defaultLight().lampsAt(18000).green);
}

void test_blinking_green_goes_dark_for_the_second_half_of_the_period(void) {
    // период мигания 1000 мс: горит 500, погашен 500
    TEST_ASSERT_FALSE(defaultLight().lampsAt(18500).green);
}

void test_blinking_green_lights_again_on_the_next_period(void) {
    TEST_ASSERT_TRUE(defaultLight().lampsAt(19000).green);
}

void test_blinking_green_never_lights_other_lamps(void) {
    Lamps dark = defaultLight().lampsAt(18500);
    TEST_ASSERT_FALSE(dark.red);
    TEST_ASSERT_FALSE(dark.yellow);
}

// --- настраиваемые длительности ---

void test_custom_durations_shift_the_phase_boundaries(void) {
    PhaseDurations fast;
    fast.redMs = 1000;
    fast.redYellowMs = 1000;
    fast.greenMs = 1000;
    fast.greenBlinkMs = 1000;
    fast.yellowMs = 1000;

    TrafficLight light(fast);
    TEST_ASSERT_EQUAL_UINT32(5000, light.cycleMs());
    TEST_ASSERT_EQUAL(static_cast<int>(Phase::Red),
                      static_cast<int>(light.phaseAt(999)));
    TEST_ASSERT_EQUAL(static_cast<int>(Phase::RedYellow),
                      static_cast<int>(light.phaseAt(1000)));
    TEST_ASSERT_EQUAL(static_cast<int>(Phase::Yellow),
                      static_cast<int>(light.phaseAt(4999)));
}

int main(int, char **) {
    UNITY_BEGIN();

    RUN_TEST(test_cycle_starts_with_red);
    RUN_TEST(test_red_holds_until_its_last_millisecond);
    RUN_TEST(test_red_yellow_begins_exactly_when_red_ends);
    RUN_TEST(test_phases_follow_the_documented_order);
    RUN_TEST(test_cycle_wraps_back_to_red);
    RUN_TEST(test_cycle_length_is_the_sum_of_all_phases);

    RUN_TEST(test_countdown_shows_full_phase_length_at_its_start);
    RUN_TEST(test_countdown_ticks_down_once_per_second);
    RUN_TEST(test_countdown_shows_one_through_the_final_second);
    RUN_TEST(test_countdown_restarts_on_the_next_phase);

    RUN_TEST(test_red_phase_lights_only_red);
    RUN_TEST(test_red_yellow_phase_lights_both);
    RUN_TEST(test_green_phase_lights_only_green);
    RUN_TEST(test_yellow_phase_lights_only_yellow);

    RUN_TEST(test_blinking_green_is_lit_at_the_start_of_the_phase);
    RUN_TEST(test_blinking_green_goes_dark_for_the_second_half_of_the_period);
    RUN_TEST(test_blinking_green_lights_again_on_the_next_period);
    RUN_TEST(test_blinking_green_never_lights_other_lamps);

    RUN_TEST(test_custom_durations_shift_the_phase_boundaries);

    return UNITY_END();
}
