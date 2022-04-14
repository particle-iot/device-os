#include "UnitTest++.h"
#include "core_protocol.h"

SUITE(Queue)
{
  TEST(QueueAvailableBeginsEqualToConst)
  {
    CoreProtocol core_protocol;
    int size = core_protocol.QUEUE_SIZE - 1;
    int available = core_protocol.queue_bytes_available();
    CHECK_EQUAL(size, available);
  }

  TEST(QueueAvailableAfterPushIsSizeMinusOne)
  {
    CoreProtocol core_protocol;
    char buf[1] = {'x'};
    core_protocol.queue_push(buf, 1);
    int available = core_protocol.queue_bytes_available();
    CHECK_EQUAL(core_protocol.QUEUE_SIZE - 1 - 1, available);
  }

  TEST(QueueAvailableAfterPush5Pop2IsSizeMinusThree)
  {
    CoreProtocol core_protocol;
    char buf[6] = "Spark";
    core_protocol.queue_push(buf, 5);
    core_protocol.queue_pop(buf, 2);
    int available = core_protocol.queue_bytes_available();
    CHECK_EQUAL(core_protocol.QUEUE_SIZE - 1 - 3, available);
  }

  TEST(QueueWrapsAsRingBuffer)
  {
    CoreProtocol core_protocol;
    int too_big = core_protocol.QUEUE_SIZE + 16;
    char buf[too_big];
    memset(buf, 42, too_big);
    core_protocol.queue_push(buf, 500);
    core_protocol.queue_pop(buf, 400);
    core_protocol.queue_push(buf + 500, too_big - 500);
    int available = core_protocol.queue_bytes_available();
    // we've pushed too_big bytes and popped 400, so too_big - 400 is used
    CHECK_EQUAL(core_protocol.QUEUE_SIZE - 1 - too_big + 400, available);
  }

  TEST(InitialQueueCannotOverfill)
  {
    CoreProtocol core_protocol;
    int too_big = core_protocol.QUEUE_SIZE + 16;
    char buf[too_big];
    memset(buf, 42, too_big);
    int written = core_protocol.queue_push(buf, too_big);
    CHECK_EQUAL(core_protocol.QUEUE_SIZE - 1, written);
  }

  TEST(QueuePopsRequestedAmount)
  {
    CoreProtocol core_protocol;
    char buf[300];
    memset(buf, '*', 300);
    core_protocol.queue_push(buf, 300);
    int popped = core_protocol.queue_pop(buf, 300);
    CHECK_EQUAL(300, popped);
  }

  TEST(QueueCanCopyOversizeInStages)
  {
    CoreProtocol core_protocol;
    char src_buf[900];
    memset(src_buf,       'Z', 300);
    memset(src_buf + 300, 'a', 300);
    memset(src_buf + 600, 'c', 300);
    char dst_buf[900];
    memset(dst_buf, '*', 900);
    int written1 = core_protocol.queue_push(src_buf, 900);
    core_protocol.queue_pop(dst_buf, written1);
    int written2 = core_protocol.queue_push(src_buf + written1, 900 - written1);
    core_protocol.queue_pop(dst_buf + written1, written2);
    CHECK_ARRAY_EQUAL(src_buf, dst_buf, 900);
  }

  TEST(FullQueueShowsZeroAvailable)
  {
    CoreProtocol core_protocol;
    int size = core_protocol.QUEUE_SIZE - 1;
    char buf[size];
    memset(buf, '$', size);
    core_protocol.queue_push(buf, size);
    int available = core_protocol.queue_bytes_available();
    CHECK_EQUAL(0, available);
  }

  TEST(QueueCannotPopMoreThanFilled)
  {
    CoreProtocol core_protocol;
    char buf[1];
    int popped = core_protocol.queue_pop(buf, 1);
    CHECK_EQUAL(0, popped);
  }
}
