#include <unity.h>

#include "SevenSegment.h"

// Сегменты по стандартной раскладке:
//   aaa
//  f   b
//   ggg
//  e   c
//   ddd

static bool lit(uint8_t digit, uint8_t segment) {
    return (segmentsForDigit(digit) & segment) != 0;
}

void setUp(void) {}
void tearDown(void) {}

void test_zero_lights_the_ring_but_not_the_middle(void) {
    TEST_ASSERT_EQUAL_UINT8(SegA | SegB | SegC | SegD | SegE | SegF,
                            segmentsForDigit(0));
    TEST_ASSERT_FALSE(lit(0, SegG));
}

void test_one_lights_only_the_two_right_segments(void) {
    TEST_ASSERT_EQUAL_UINT8(SegB | SegC, segmentsForDigit(1));
}

void test_two_lights_the_zigzag(void) {
    TEST_ASSERT_EQUAL_UINT8(SegA | SegB | SegG | SegE | SegD,
                            segmentsForDigit(2));
}

void test_three_differs_from_two_by_the_lower_right_side(void) {
    TEST_ASSERT_EQUAL_UINT8(SegA | SegB | SegG | SegC | SegD,
                            segmentsForDigit(3));
}

void test_four_has_no_top_and_no_bottom(void) {
    TEST_ASSERT_EQUAL_UINT8(SegF | SegG | SegB | SegC, segmentsForDigit(4));
    TEST_ASSERT_FALSE(lit(4, SegA));
    TEST_ASSERT_FALSE(lit(4, SegD));
}

void test_five_is_two_mirrored(void) {
    TEST_ASSERT_EQUAL_UINT8(SegA | SegF | SegG | SegC | SegD,
                            segmentsForDigit(5));
}

void test_six_is_five_plus_the_lower_left_side(void) {
    TEST_ASSERT_EQUAL_UINT8(segmentsForDigit(5) | SegE, segmentsForDigit(6));
}

void test_seven_is_the_top_and_the_right_side(void) {
    TEST_ASSERT_EQUAL_UINT8(SegA | SegB | SegC, segmentsForDigit(7));
}

void test_eight_lights_every_segment(void) {
    TEST_ASSERT_EQUAL_UINT8(SegA | SegB | SegC | SegD | SegE | SegF | SegG,
                            segmentsForDigit(8));
}

void test_nine_is_eight_without_the_lower_left_side(void) {
    TEST_ASSERT_EQUAL_UINT8(segmentsForDigit(8) & ~SegE, segmentsForDigit(9));
}

// --- инварианты, ловящие опечатку в таблице ---

void test_every_digit_lights_at_least_two_segments(void) {
    for (uint8_t d = 0; d <= 9; ++d) {
        uint8_t count = 0;
        for (uint8_t bit = 1; bit != 0; bit = static_cast<uint8_t>(bit << 1)) {
            if (segmentsForDigit(d) & bit) {
                ++count;
            }
        }
        TEST_ASSERT_GREATER_OR_EQUAL_UINT8(2, count);
    }
}

void test_all_digits_have_distinct_patterns(void) {
    for (uint8_t a = 0; a <= 9; ++a) {
        for (uint8_t b = static_cast<uint8_t>(a + 1); b <= 9; ++b) {
            TEST_ASSERT_NOT_EQUAL_UINT8(segmentsForDigit(a),
                                        segmentsForDigit(b));
        }
    }
}

void test_an_out_of_range_value_lights_nothing(void) {
    TEST_ASSERT_EQUAL_UINT8(0, segmentsForDigit(10));
    TEST_ASSERT_EQUAL_UINT8(0, segmentsForDigit(255));
}

int main(int, char **) {
    UNITY_BEGIN();

    RUN_TEST(test_zero_lights_the_ring_but_not_the_middle);
    RUN_TEST(test_one_lights_only_the_two_right_segments);
    RUN_TEST(test_two_lights_the_zigzag);
    RUN_TEST(test_three_differs_from_two_by_the_lower_right_side);
    RUN_TEST(test_four_has_no_top_and_no_bottom);
    RUN_TEST(test_five_is_two_mirrored);
    RUN_TEST(test_six_is_five_plus_the_lower_left_side);
    RUN_TEST(test_seven_is_the_top_and_the_right_side);
    RUN_TEST(test_eight_lights_every_segment);
    RUN_TEST(test_nine_is_eight_without_the_lower_left_side);

    RUN_TEST(test_every_digit_lights_at_least_two_segments);
    RUN_TEST(test_all_digits_have_distinct_patterns);
    RUN_TEST(test_an_out_of_range_value_lights_nothing);

    return UNITY_END();
}
