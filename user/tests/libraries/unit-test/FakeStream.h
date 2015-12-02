/*
Copyright (c) 2009-2013 Matthew Murdoch

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#pragma once


/**
 * A fake stream which can be used in place of other streams
 * to inject bytes to be read and to record any bytes written.
 *
 * @author Matthew Murdoch
 */
class FakeStream : public Stream {
public:
    /**
     * Creates a fake stream. Until nextByte() is called all bytes
     * returned from read() or peek() will be -1 (end-of-stream).
     */
    FakeStream();

    /**
     * Destroys this fake stream.
     */
    virtual ~FakeStream();

    /**
     * Writes a byte to this stream. The byte is recorded by appending
     * it to the internal store and can be observed by calling bytesWritten().
     *
     * @param val the byte to write
     * @return the number of bytes written (always 1)
     */
    size_t write(uint8_t val);

    /**
     * Flushes this stream. Does nothing in this implementation to avoid conflicts (false negative tests).
     */
    void flush();

    /**
     * Reset the FakeStream so that it can be reused across tests.
     * When called, 'bytesWritten' becomes empty  ("").
     */
    void reset();

    /**
     * The bytes written by calling write(uint8_t).
     *
     * @return the bytes written
     */
    const String& bytesWritten();

    /**
     * Sets the next value to be read via read() or peek() to -1 (end-of-stream).
     */
    void setToEndOfStream();

    /**
     * Sets the value of the next byte to be read via read() or peek().
     *
     * @param b the byte value
     */
    void nextByte(byte b);

    /**
     * The number of bytes available to be read.
     *
     * @return the number of available bytes (always 1)
     */
    int available();

    /**
     * Reads a byte, removing it from the stream.
     *
     * @return the byte passed to nextByte() or -1 (end-of-stream) if
     *         nextByte() has not been called or setToEndOfStream() has been called
     */
    int read();

    /**
     * Reads a byte without removing it from the stream.
     *
     * @return the byte passed to nextByte() or -1 (end-of-stream) if
     *         nextByte() has not been called or setToEndOfStream() has been called
     */
    int peek();

private:
    String _bytesWritten;
    int _nextByte;
};
