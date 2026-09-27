#include <unity.h>

#include <math.h>

#include "FrequencyReadout.h"

// Частоты нот взяты из таблицы равномерно темперированного строя,
// центы посчитаны вручную: 1200 * log2(f / f_ноты).

static void assertNote(const char *name, int octave, int cents, float hz) {
    const Note note = noteFor(hz);
    TEST_ASSERT_EQUAL_STRING(name, note.name);
    TEST_ASSERT_EQUAL_INT(octave, note.octave);
    TEST_ASSERT_EQUAL_INT(cents, note.cents);
}

static void assertFormat(const char *want, float hz) {
    char buf[12];
    formatHz(hz, buf, sizeof buf);
    TEST_ASSERT_EQUAL_STRING(want, buf);
}

void setUp(void) {}
void tearDown(void) {}

void test_concert_a_is_a4_in_tune(void) { assertNote("A", 4, 0, 440.0f); }

void test_middle_c_is_c4(void) { assertNote("C", 4, 0, 261.63f); }

void test_sharp_is_spelled_with_a_hash(void) {
    assertNote("A#", 4, 0, 466.16f);
}

void test_slightly_sharp_a_reports_positive_cents(void) {
    assertNote("A", 4, 20, 445.0f);
}

// Пример с макета: 305 Гц ниже D#4 (311.13 Гц) на 34 цента.
void test_flat_tone_reports_negative_cents_of_the_nearest_note(void) {
    assertNote("D#", 4, -34, 305.0f);
}

// Октава меняется между B и C, а не между G# и A.
void test_octave_number_changes_between_b_and_c(void) {
    assertNote("B", 3, 0, 246.94f);
    assertNote("C", 4, 0, 261.63f);
}

void test_low_notes_get_low_octaves(void) {
    assertNote("A", 0, 0, 27.5f);
    assertNote("E", 2, 0, 82.41f);
}

// 49 центов выше A4 — ещё A4, 51 цент — уже A#4 на 49 центов ниже.
void test_nearest_note_switches_at_the_half_semitone(void) {
    assertNote("A", 4, 49, 452.63f);
    assertNote("A#", 4, -49, 453.15f);
}

// Недопустимая частота не должна давать индекс за пределами таблицы нот:
// вместо ноты — пустое имя, которое рисуется как прочерк.
void test_invalid_frequency_gives_no_note(void) {
    assertNote("", 0, 0, 0.0f);
    assertNote("", 0, 0, -440.0f);
    assertNote("", 0, 0, NAN);
}

// Ниже C-1 номер MIDI отрицательный: 7.717 Гц — это MIDI -1, B-2.
// Остаток и октава должны считаться с округлением вниз, а не к нулю.
void test_note_below_midi_zero_wraps_correctly(void) {
    assertNote("B", -2, 0, 7.717f);
}

void test_below_one_hundred_hz_shows_two_decimals(void) {
    assertFormat("82.41", 82.41f);
}

void test_hundreds_show_one_decimal(void) { assertFormat("440.3", 440.3f); }

void test_thousands_show_whole_hertz(void) { assertFormat("1234", 1234.4f); }

// Округление не должно давать пятую цифру: не "100.00" и не "1000.0".
void test_rounding_up_moves_to_the_next_format(void) {
    assertFormat("100.0", 99.996f);
    assertFormat("1000", 999.96f);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_concert_a_is_a4_in_tune);
    RUN_TEST(test_middle_c_is_c4);
    RUN_TEST(test_sharp_is_spelled_with_a_hash);
    RUN_TEST(test_slightly_sharp_a_reports_positive_cents);
    RUN_TEST(test_flat_tone_reports_negative_cents_of_the_nearest_note);
    RUN_TEST(test_octave_number_changes_between_b_and_c);
    RUN_TEST(test_low_notes_get_low_octaves);
    RUN_TEST(test_nearest_note_switches_at_the_half_semitone);
    RUN_TEST(test_invalid_frequency_gives_no_note);
    RUN_TEST(test_note_below_midi_zero_wraps_correctly);

    RUN_TEST(test_below_one_hundred_hz_shows_two_decimals);
    RUN_TEST(test_hundreds_show_one_decimal);
    RUN_TEST(test_thousands_show_whole_hertz);
    RUN_TEST(test_rounding_up_moves_to_the_next_format);

    return UNITY_END();
}
