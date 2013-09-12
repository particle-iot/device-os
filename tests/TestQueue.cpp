#include "UnitTest++.h"
#include "spark_protocol.h"

SUITE(Queue)
{
  TEST(QueueAvailableBeginsEqualToConst)
  {
    SparkProtocol spark_protocol;
    int size = spark_protocol.QUEUE_SIZE - 1;
    int available = spark_protocol.queue_bytes_available();
    CHECK_EQUAL(size, available);
  }

  TEST(QueueAvailableAfterPushIsSizeMinusOne)
  {
    SparkProtocol spark_protocol;
    char buf[1] = {'x'};
    spark_protocol.queue_push(buf, 1);
    int available = spark_protocol.queue_bytes_available();
    CHECK_EQUAL(spark_protocol.QUEUE_SIZE - 1 - 1, available);
  }

  TEST(QueueAvailableAfterPush5Pop2IsSizeMinusThree)
  {
    SparkProtocol spark_protocol;
    char buf[6] = "Spark";
    spark_protocol.queue_push(buf, 5);
    spark_protocol.queue_pop(buf, 2);
    int available = spark_protocol.queue_bytes_available();
    CHECK_EQUAL(spark_protocol.QUEUE_SIZE - 1 - 3, available);
  }

  TEST(QueueWrapsAsRingBuffer)
  {
    SparkProtocol spark_protocol;
    int too_big = spark_protocol.QUEUE_SIZE + 16;
    char buf[too_big];
    memset(buf, 42, too_big);
    spark_protocol.queue_push(buf, 500);
    spark_protocol.queue_pop(buf, 400);
    spark_protocol.queue_push(buf + 500, too_big - 500);
    int available = spark_protocol.queue_bytes_available();
    // we've pushed too_big bytes and popped 400, so too_big - 400 is used
    CHECK_EQUAL(spark_protocol.QUEUE_SIZE - 1 - too_big + 400, available);
  }

  TEST(InitialQueueCannotOverfill)
  {
    SparkProtocol spark_protocol;
    int too_big = spark_protocol.QUEUE_SIZE + 16;
    char buf[too_big];
    memset(buf, 42, too_big);
    int written = spark_protocol.queue_push(buf, too_big);
    CHECK_EQUAL(spark_protocol.QUEUE_SIZE - 1, written);
  }

  TEST(QueuePopsRequestedAmount)
  {
    SparkProtocol spark_protocol;
    char buf[300];
    memset(buf, '*', 300);
    spark_protocol.queue_push(buf, 300);
    int popped = spark_protocol.queue_pop(buf, 300);
    CHECK_EQUAL(300, popped);
  }
  
  TEST(QueueCanCopyOversizeInStages)
  {
    SparkProtocol spark_protocol;
    char src_buf[900];
    memset(src_buf,       'Z', 300);
    memset(src_buf + 300, 'a', 300);
    memset(src_buf + 600, 'c', 300);
    char dst_buf[900];
    memset(dst_buf, '*', 900);
    int written1 = spark_protocol.queue_push(src_buf, 900);
    spark_protocol.queue_pop(dst_buf, written1);
    int written2 = spark_protocol.queue_push(src_buf + written1, 900 - written1);
    spark_protocol.queue_pop(dst_buf + written1, written2);
    CHECK_ARRAY_EQUAL(src_buf, dst_buf, 900);
  }

  TEST(FullQueueShowsZeroAvailable)
  {
    SparkProtocol spark_protocol;
    int size = spark_protocol.QUEUE_SIZE - 1;
    char buf[size];
    memset(buf, '$', size);
    spark_protocol.queue_push(buf, size);
    int available = spark_protocol.queue_bytes_available();
    CHECK_EQUAL(0, available);
  }

  TEST(QueueCannotPopMoreThanFilled)
  {
    SparkProtocol spark_protocol;
    char buf[1];
    int popped = spark_protocol.queue_pop(buf, 1);
    CHECK_EQUAL(0, popped);
  }
}
