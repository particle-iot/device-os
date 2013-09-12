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
    char buf[600];
    memset(buf, 42, 600);
    spark_protocol.queue_push(buf, 500);
    spark_protocol.queue_pop(buf, 400);
    spark_protocol.queue_push(buf + 500, 100);
    int available = spark_protocol.queue_bytes_available();
    // we've pushed 600 bytes and popped 400, so only 200 is used
    CHECK_EQUAL(spark_protocol.QUEUE_SIZE - 1 - 200, available);
  }

  TEST(InitialQueueCannotOverfill)
  {
    SparkProtocol spark_protocol;
    char buf[600];
    memset(buf, 42, 600);
    int written = spark_protocol.queue_push(buf, 600);
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
    char src_buf[600];
    memset(src_buf,       'Z', 200);
    memset(src_buf + 200, 'a', 200);
    memset(src_buf + 400, 'c', 200);
    char dst_buf[600];
    memset(dst_buf, '*', 600);
    int written1 = spark_protocol.queue_push(src_buf, 600);
    spark_protocol.queue_pop(dst_buf, written1);
    int written2 = spark_protocol.queue_push(src_buf + written1, 600 - written1);
    spark_protocol.queue_pop(dst_buf + written1, written2);
    CHECK_ARRAY_EQUAL(src_buf, dst_buf, 600);
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
