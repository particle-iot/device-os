#include "UnitTest++.h"
#include "particle_protocol.h"

SUITE(Queue)
{
  TEST(QueueAvailableBeginsEqualToConst)
  {
    ParticleProtocol particle_protocol;
    int size = particle_protocol.QUEUE_SIZE - 1;
    int available = particle_protocol.queue_bytes_available();
    CHECK_EQUAL(size, available);
  }

  TEST(QueueAvailableAfterPushIsSizeMinusOne)
  {
    ParticleProtocol particle_protocol;
    char buf[1] = {'x'};
    particle_protocol.queue_push(buf, 1);
    int available = particle_protocol.queue_bytes_available();
    CHECK_EQUAL(particle_protocol.QUEUE_SIZE - 1 - 1, available);
  }

  TEST(QueueAvailableAfterPush5Pop2IsSizeMinusThree)
  {
    ParticleProtocol particle_protocol;
    char buf[6] = "Particle";
    particle_protocol.queue_push(buf, 5);
    particle_protocol.queue_pop(buf, 2);
    int available = particle_protocol.queue_bytes_available();
    CHECK_EQUAL(particle_protocol.QUEUE_SIZE - 1 - 3, available);
  }

  TEST(QueueWrapsAsRingBuffer)
  {
    ParticleProtocol particle_protocol;
    int too_big = particle_protocol.QUEUE_SIZE + 16;
    char buf[too_big];
    memset(buf, 42, too_big);
    particle_protocol.queue_push(buf, 500);
    particle_protocol.queue_pop(buf, 400);
    particle_protocol.queue_push(buf + 500, too_big - 500);
    int available = particle_protocol.queue_bytes_available();
    // we've pushed too_big bytes and popped 400, so too_big - 400 is used
    CHECK_EQUAL(particle_protocol.QUEUE_SIZE - 1 - too_big + 400, available);
  }

  TEST(InitialQueueCannotOverfill)
  {
    ParticleProtocol particle_protocol;
    int too_big = particle_protocol.QUEUE_SIZE + 16;
    char buf[too_big];
    memset(buf, 42, too_big);
    int written = particle_protocol.queue_push(buf, too_big);
    CHECK_EQUAL(particle_protocol.QUEUE_SIZE - 1, written);
  }

  TEST(QueuePopsRequestedAmount)
  {
    ParticleProtocol particle_protocol;
    char buf[300];
    memset(buf, '*', 300);
    particle_protocol.queue_push(buf, 300);
    int popped = particle_protocol.queue_pop(buf, 300);
    CHECK_EQUAL(300, popped);
  }

  TEST(QueueCanCopyOversizeInStages)
  {
    ParticleProtocol particle_protocol;
    char src_buf[900];
    memset(src_buf,       'Z', 300);
    memset(src_buf + 300, 'a', 300);
    memset(src_buf + 600, 'c', 300);
    char dst_buf[900];
    memset(dst_buf, '*', 900);
    int written1 = particle_protocol.queue_push(src_buf, 900);
    particle_protocol.queue_pop(dst_buf, written1);
    int written2 = particle_protocol.queue_push(src_buf + written1, 900 - written1);
    particle_protocol.queue_pop(dst_buf + written1, written2);
    CHECK_ARRAY_EQUAL(src_buf, dst_buf, 900);
  }

  TEST(FullQueueShowsZeroAvailable)
  {
    ParticleProtocol particle_protocol;
    int size = particle_protocol.QUEUE_SIZE - 1;
    char buf[size];
    memset(buf, '$', size);
    particle_protocol.queue_push(buf, size);
    int available = particle_protocol.queue_bytes_available();
    CHECK_EQUAL(0, available);
  }

  TEST(QueueCannotPopMoreThanFilled)
  {
    ParticleProtocol particle_protocol;
    char buf[1];
    int popped = particle_protocol.queue_pop(buf, 1);
    CHECK_EQUAL(0, popped);
  }
}
