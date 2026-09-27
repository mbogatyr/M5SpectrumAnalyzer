#include <unity.h>

#include <math.h>

#include "FrequencyReadout.h"

// Note frequencies are taken from the equal temperament table; cents are
// worked out by hand: 1200 * log2(f / f_note).

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

// Example from the mockup: 305 Hz is 34 cents below D#4 (311.13 Hz).
void test_flat_tone_reports_negative_cents_of_the_nearest_note(void) {
    assertNote("D#", 4, -34, 305.0f);
}

// The octave changes between B and C, not between G# and A.
void test_octave_number_changes_between_b_and_c(void) {
    assertNote("B", 3, 0, 246.94f);
    assertNote("C", 4, 0, 261.63f);
}

void test_low_notes_get_low_octaves(void) {
    assertNote("A", 0, 0, 27.5f);
    assertNote("E", 2, 0, 82.41f);
}

// 49 cents above A4 is still A4; 51 cents is already A#4, 49 cents below.
void test_nearest_note_switches_at_the_half_semitone(void) {
    assertNote("A", 4, 49, 452.63f);
    assertNote("A#", 4, -49, 453.15f);
}

// An invalid frequency must not yield an index outside the note table:
// instead of a note it gives an empty name, which is drawn as a dash.
void test_invalid_frequency_gives_no_note(void) {
    assertNote("", 0, 0, 0.0f);
    assertNote("", 0, 0, -440.0f);
    assertNote("", 0, 0, NAN);
}

// Below C-1 the MIDI number is negative: 7.717 Hz is MIDI -1, B-2.
// The remainder and the octave must round down, not toward zero.
void test_note_below_midi_zero_wraps_correctly(void) {
    assertNote("B", -2, 0, 7.717f);
}

void test_below_one_hundred_hz_shows_two_decimals(void) {
    assertFormat("82.41", 82.41f);
}

void test_hundreds_show_one_decimal(void) { assertFormat("440.3", 440.3f); }

void test_thousands_show_whole_hertz(void) { assertFormat("1234", 1234.4f); }

// Rounding must not produce a fifth digit: neither "100.00" nor "1000.0".
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
