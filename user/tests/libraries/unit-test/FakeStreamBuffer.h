#pragma once

#include "unit-test.h"

struct BufferNode {
  byte value;
  BufferNode *next;
};

/**
 * A fake stream which can be used in place of other streams to inject bytes to
 * be read and to record any bytes written.
 *
 * @author Matthew Paine
 **/
class FakeStreamBuffer : public FakeStream {
    public:
        /**
         * Creates a fake stream.
         **/
        FakeStreamBuffer();

        /**
         * Destroys this fake stream.
         **/
        virtual ~FakeStreamBuffer();

        /**
         * Reset the FakeStream so that it can be reused across tests.  When called,
         * 'bytesWritten' becomes empty  ("") and the buffer is reset.
         **/
        void reset();

        /**
         * Pushes an end of stream (-1) to the end of the buffer
         **/
        void setToEndOfStream();

        /**
         * Appends the current buffer of values to be read via read() or peek()
         *
         * @param b the byte value
         **/
        void nextByte(byte b);

        /**
         * Appends the current buffer of values to be read via read() or peek()
         *
         * @param b the byte value
         **/
        void nextBytes(const char *s);

        /**
         * The number of bytes available to be read.
         *
         * @return the number of available bytes
         **/
        int available();

        /**
         * Reads a byte, removing it from the stream.
         *
         * @return the next byte in the buffer, or -1 (end-of-stream) if the buffer
         *         is empty or setToEndOfStream() has been called
         **/
        int read();

        /**
         * Reads a byte without removing it from the stream.
         *
         * @return the next byte in the buffer, or -1 (end-of-stream) if the buffer
         *         is empty or setToEndOfStream() has been called
         **/
        int peek();

    private:
        BufferNode *_firstNode;
        BufferNode *_lastNode;

        BufferNode* _createNode (byte value);
        void _appendNode(BufferNode *node);
        void _pushByte(byte value);
        BufferNode* _popNode();
        byte _nextByte();
        int _getBufferSize();
        int _getBufferSize(byte stopAt);
        void _clearBuffer();
};

