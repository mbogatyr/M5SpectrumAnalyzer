#include <unity.h>

#include "BlockRing.h"

// Блоки по 3 отсчёта в кольце из 7. Тест играет роль микрофона: пишет в
// каждый блок его порядковый номер, помноженный на 10, плюс номер
// отсчёта — так по любому числу видно, откуда оно взялось.

static const size_t kLen = 3;
static const size_t kCount = 7;

static void queueBlocks(BlockRing &ring, int from, int to) {
    for (int seq = from; seq < to; ++seq) {
        int16_t *block = ring.nextWriteBlock();
        for (size_t i = 0; i < kLen; ++i) {
            block[i] = static_cast<int16_t>(seq * 10 + i);
        }
        ring.markQueued();
    }
}

void setUp(void) {}
void tearDown(void) {}

void test_write_blocks_walk_the_ring_and_wrap(void) {
    BlockRing ring(kLen, kCount);
    int16_t *first = ring.nextWriteBlock();

    ring.markQueued();
    TEST_ASSERT_EQUAL_PTR(first + kLen, ring.nextWriteBlock());

    for (size_t i = 1; i < kCount; ++i) {
        ring.markQueued();
    }
    TEST_ASSERT_EQUAL_PTR(first, ring.nextWriteBlock());
}

// Два последних поставленных блока ещё пишутся.
void test_blocks_in_the_queue_are_not_complete(void) {
    BlockRing ring(kLen, kCount);

    queueBlocks(ring, 0, 2);
    TEST_ASSERT_EQUAL_UINT32(0, ring.completedBlocks());

    queueBlocks(ring, 2, 3);
    TEST_ASSERT_EQUAL_UINT32(1, ring.completedBlocks());
}

void test_window_is_refused_until_enough_blocks_are_complete(void) {
    BlockRing ring(kLen, kCount);
    int16_t window[4 * kLen];

    queueBlocks(ring, 0, 5); // готовы блоки 0..2

    TEST_ASSERT_FALSE(ring.copyLatest(window, 4));
}

// Поставлено 0..5, пишутся 4 и 5, готовы 0..3.
void test_window_holds_the_latest_complete_blocks_in_order(void) {
    BlockRing ring(kLen, kCount);
    int16_t window[4 * kLen];
    queueBlocks(ring, 0, 6);

    TEST_ASSERT_TRUE(ring.copyLatest(window, 4));

    const int16_t want[4 * kLen] = {0,  1,  2,  10, 11, 12,
                                    20, 21, 22, 30, 31, 32};
    TEST_ASSERT_EQUAL_INT16_ARRAY(want, window, 4 * kLen);
}

// Поставлено 0..9, готовы 0..7. Блоки 4, 5, 6 лежат в хвосте кольца,
// блок 7 — снова в первом слоте. Окно должно идти по времени, а не по
// адресам.
void test_window_stays_chronological_across_the_ring_end(void) {
    BlockRing ring(kLen, kCount);
    int16_t window[4 * kLen];
    queueBlocks(ring, 0, 10);

    TEST_ASSERT_TRUE(ring.copyLatest(window, 4));

    const int16_t want[4 * kLen] = {40, 41, 42, 50, 51, 52,
                                    60, 61, 62, 70, 71, 72};
    TEST_ASSERT_EQUAL_INT16_ARRAY(want, window, 4 * kLen);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_write_blocks_walk_the_ring_and_wrap);
    RUN_TEST(test_blocks_in_the_queue_are_not_complete);
    RUN_TEST(test_window_is_refused_until_enough_blocks_are_complete);
    RUN_TEST(test_window_holds_the_latest_complete_blocks_in_order);
    RUN_TEST(test_window_stays_chronological_across_the_ring_end);

    return UNITY_END();
}
