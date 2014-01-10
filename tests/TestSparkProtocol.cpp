#include "UnitTest++.h"
#include "spark_protocol.h"
#include "ConstructorFixture.h"

SUITE(SparkProtocolConstruction)
{
  TEST_FIXTURE(ConstructorFixture, NoErrorReturnedFromHandshake)
  {
    int err = spark_protocol.handshake();
    CHECK_EQUAL(0, err);
  }

  TEST_FIXTURE(ConstructorFixture, HandshakeReceives40Bytes)
  {
    spark_protocol.handshake();
    CHECK_EQUAL(40, bytes_received[0]);
  }

  TEST_FIXTURE(ConstructorFixture, HandshakeSends256Bytes)
  {
    spark_protocol.handshake();
    CHECK_EQUAL(256, bytes_sent[0]);
  }

  TEST_FIXTURE(ConstructorFixture, HandshakeLaterReceives384Bytes)
  {
    spark_protocol.handshake();
    CHECK_EQUAL(384, bytes_received[1]);
  }

  TEST_FIXTURE(ConstructorFixture, HandshakeLaterSends18Bytes)
  {
    spark_protocol.handshake();
    CHECK_EQUAL(18, bytes_sent[1]);
  }

  TEST_FIXTURE(ConstructorFixture, HandshakeSendsExpectedHello)
  {
    const uint8_t expected[18] = {
      0x00, 0x10,
      0xde, 0x89, 0x60, 0x27, 0xba, 0x11, 0x1a, 0x95,
      0xbe, 0x39, 0x6f, 0x4d, 0xfc, 0x30, 0x9b, 0x6b };
    spark_protocol.handshake();
    CHECK_ARRAY_EQUAL(expected, sent_buf_1, 18);
  }


  /********* event loop *********/

  TEST_FIXTURE(ConstructorFixture, EventLoopReceives2Bytes)
  {
    spark_protocol.handshake();
    bytes_received[0] = 0;
    spark_protocol.event_loop();
    CHECK_EQUAL(2, bytes_received[0]);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopLaterReceives16Bytes)
  {
    uint8_t describe[18] = {
      0x00, 0x10,
      0xd5, 0xb7, 0xf7, 0xfe, 0x9f, 0x2d, 0xca, 0xac,
      0xda, 0x15, 0x10, 0xa3, 0x27, 0x8b, 0xa7, 0xa9 };
    memcpy(message_to_receive, describe, 18);
    spark_protocol.handshake();
    bytes_received[0] = bytes_received[1] = 0;
    spark_protocol.event_loop();
    CHECK_EQUAL(16, bytes_received[1]);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopRespondsToDescribeWith50Bytes)
  {
    uint8_t describe[18] = {
      0x00, 0x10,
      0x4d, 0x2b, 0x01, 0x6f, 0x13, 0xee, 0xde, 0xdc,
      0xaf, 0x79, 0x23, 0xfb, 0x76, 0x81, 0xb3, 0x7a };
    memcpy(message_to_receive, describe, 18);
    spark_protocol.handshake();
    bytes_received[0] = bytes_sent[0] = 0;
    spark_protocol.event_loop();
    CHECK_EQUAL(50, bytes_sent[0]);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopRespondsToDescribeWithDescription)
  {
    uint8_t describe[18] = {
      0x00, 0x10,
      0x4d, 0x2b, 0x01, 0x6f, 0x13, 0xee, 0xde, 0xdc,
      0xaf, 0x79, 0x23, 0xfb, 0x76, 0x81, 0xb3, 0x7a };
    memcpy(message_to_receive, describe, 18);
    const uint8_t expected[50] = {
      0x00, 0x30,
      0x8c, 0x75, 0x7e, 0x24, 0xf7, 0x56, 0xde, 0x78,
      0xd8, 0x4f, 0x19, 0x80, 0xa8, 0xe0, 0xd1, 0x84,
      0xb0, 0x96, 0x00, 0x94, 0x76, 0x92, 0x11, 0xc8,
      0x84, 0xf5, 0x95, 0x6a, 0xe4, 0x16, 0x21, 0x61,
      0x10, 0xf5, 0xe5, 0x1a, 0xf1, 0x78, 0x0b, 0x75,
      0x6a, 0xed, 0x83, 0xc5, 0xe0, 0x5d, 0x5c, 0x5a };
    spark_protocol.handshake();
    bytes_received[0] = bytes_sent[0] = 0;
    spark_protocol.event_loop();
    CHECK_ARRAY_EQUAL(expected, sent_buf_0, 50);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopRespondsToFunctionCallWithACK)
  {
    uint8_t function_call[34] = {
      0x00, 0x20,
      0xc6, 0xf4, 0xcb, 0x28, 0x85, 0xd8, 0x1e, 0xdd,
      0x23, 0x1c, 0x56, 0xb8, 0xd8, 0x03, 0x28, 0x94,
      0xce, 0x89, 0xab, 0x42, 0x48, 0x86, 0x78, 0x76,
      0xa0, 0x14, 0x07, 0x23, 0xb5, 0xa9, 0xfe, 0x75 };
    memcpy(message_to_receive, function_call, 34);
    const uint8_t expected[18] = {
      0x00, 0x10,
      0x80, 0xef, 0x08, 0x49, 0xd1, 0xbe, 0xe3, 0x20,
      0xf5, 0x06, 0xf9, 0x7d, 0xf1, 0xce, 0x25, 0x36 };
    spark_protocol.handshake();
    bytes_received[0] = bytes_sent[0] = 0;
    spark_protocol.event_loop();
    CHECK_ARRAY_EQUAL(expected, sent_buf_0, 18);
  }

  TEST_FIXTURE(ConstructorFixture,
               EventLoopLaterRespondsToFunctionCallWithFunctionReturn)
  {
    uint8_t function_call[34] = {
      0x00, 0x20,
      0xc6, 0xf4, 0xcb, 0x28, 0x85, 0xd8, 0x1e, 0xdd,
      0x23, 0x1c, 0x56, 0xb8, 0xd8, 0x03, 0x28, 0x94,
      0xce, 0x89, 0xab, 0x42, 0x48, 0x86, 0x78, 0x76,
      0xa0, 0x14, 0x07, 0x23, 0xb5, 0xa9, 0xfe, 0x75 };
    memcpy(message_to_receive, function_call, 34);
    const uint8_t expected[18] = {
      0x00, 0x10,
      0xa5, 0x71, 0x32, 0x5f, 0x7a, 0xd0, 0xf1, 0x3a,
      0xb9, 0x1c, 0x09, 0x2d, 0xe2, 0x8b, 0x4f, 0xe0 };
    spark_protocol.handshake();
    bytes_received[0] = bytes_sent[0] = 0;
    spark_protocol.event_loop();
    CHECK_ARRAY_EQUAL(expected, sent_buf_1, 18);
  }

  TEST_FIXTURE(ConstructorFixture,
               EventLoopRespondsToVariableRequestWithVariableValue)
  {
    uint8_t variable_request[34] = {
      0x00, 0x20,
      0xbc, 0x99, 0x43, 0x1b, 0x9a, 0x74, 0x44, 0x93,
      0x8f, 0x91, 0xa8, 0x4b, 0xc2, 0x09, 0x2b, 0xb5,
      0x75, 0x28, 0x57, 0xa6, 0xc6, 0xf4, 0x44, 0x07,
      0xdd, 0xd2, 0x0a, 0x72, 0x32, 0xb0, 0x1c, 0xbf };
    memcpy(message_to_receive, variable_request, 34);
    const uint8_t expected[18] = {
      0x00, 0x10,
      0x59, 0x28, 0xfa, 0x3d, 0x00, 0xea, 0xb2, 0x85,
      0x15, 0xb5, 0x06, 0x6d, 0x44, 0x65, 0x83, 0xa8 };
    spark_protocol.handshake();
    bytes_received[0] = bytes_sent[0] = 0;
    spark_protocol.event_loop();
    CHECK_ARRAY_EQUAL(expected, sent_buf_0, 18);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopCallsFunctionViaDescriptor)
  {
    uint8_t function_call[34] = {
      0x00, 0x20,
      0xc6, 0xf4, 0xcb, 0x28, 0x85, 0xd8, 0x1e, 0xdd,
      0x23, 0x1c, 0x56, 0xb8, 0xd8, 0x03, 0x28, 0x94,
      0xce, 0x89, 0xab, 0x42, 0x48, 0x86, 0x78, 0x76,
      0xa0, 0x14, 0x07, 0x23, 0xb5, 0xa9, 0xfe, 0x75 };
    memcpy(message_to_receive, function_call, 34);
    spark_protocol.handshake();
    bytes_received[0] = bytes_sent[0] = 0;
    spark_protocol.event_loop();
    CHECK(function_called);
  }

  TEST_FIXTURE(ConstructorFixture, BlockingSendAccumulatesOverMultipleCalls)
  {
    uint8_t expected[20] = {
      1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
    spark_protocol.blocking_send(expected, 20);
    CHECK_ARRAY_EQUAL(expected, sent_buf_0, 20);
  }

  TEST_FIXTURE(ConstructorFixture, BlockingReceiveAccumulatesOverMultipleCalls)
  {
    uint8_t expected[20] = {
      1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
    uint8_t receive_buf[20];
    memcpy(message_to_receive, expected, 20);
    spark_protocol.blocking_receive(receive_buf, 20);
    CHECK_ARRAY_EQUAL(expected, receive_buf, 20);
  }



  /*********************************
   * Over-the-air Firmware Updates *
   *********************************/

  TEST_FIXTURE(ConstructorFixture, EventLoopRespondsToUpdateBeginWithACK)
  {
    CHECK(false);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopPreparesForUpdateUponUpdateBegin)
  {
    // callbacks.prepare_for_firmware_update
    CHECK(false);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopRespondsToUpdateBeginWithUpdateReady)
  {
    CHECK(false);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopRespondsToChunkWithACK)
  {
    CHECK(false);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopRespondsToChunkWithChunkReceivedOKIfCRCMatches)
  {
    CHECK(false);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopRespondsToChunkWithChunkReceivedBADOnCRCMismatch)
  {
    CHECK(false);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopSavesReceivedChunk)
  {
    // callbacks.save_firmware_chunk
    CHECK(false);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopRespondsToUpdateDoneWithACK)
  {
    CHECK(false);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopFinishesFirmwareUpdateOnUpdateDone)
  {
    // callbacks.finish_firmware_update
    CHECK(false);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopSendsChunkMissedOnTimeout)
  {
    CHECK(false);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopCallsSignalWithTrueOnSignalStart)
  {
    uint8_t signal_start[18] = {
      0x00, 0x10,
      0x0B, 0xD9, 0x31, 0x11, 0xF2, 0x1D, 0xD2, 0x48,
      0x2B, 0x78, 0x26, 0x43, 0x38, 0x05, 0x9B, 0xB2 };
    memcpy(message_to_receive, signal_start, 18);
    spark_protocol.handshake();
    bytes_received[0] = bytes_sent[0] = 0;
    spark_protocol.event_loop();
    CHECK_EQUAL(true, signal_called_with);
  }

  TEST_FIXTURE(ConstructorFixture, EventLoopCallsSignalWithFalseOnSignalStop)
  {
    uint8_t signal_stop[18] = {
      0x00, 0x10,
      0xc0, 0x2f, 0x1e, 0x98, 0x92, 0x53, 0xc6, 0x12,
      0x67, 0x2a, 0xe0, 0x10, 0xeb, 0x20, 0xcc, 0x9f };
    memcpy(message_to_receive, signal_stop, 18);
    spark_protocol.handshake();
    bytes_received[0] = bytes_sent[0] = 0;
    spark_protocol.event_loop();
    CHECK_EQUAL(false, signal_called_with);
  }

  TEST(IsInitializedIsFalse)
  {
    SparkProtocol spark_protocol;
    CHECK_EQUAL(false, spark_protocol.is_initialized());
  }

  TEST_FIXTURE(ConstructorFixture, AfterInitIsInitializedIsTrue)
  {
    CHECK_EQUAL(true, spark_protocol.is_initialized());
  }
}
