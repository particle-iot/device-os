/*
 ******************************************************************************
 *  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#pragma once

#include "pipe_hal.h"

#ifndef EOF
    #define EOF (-1)
#endif

/** Buffered serial interface (rtos capable/interrupt driven)
*/
class ElectronSerialPipe
{
public:
    /** Constructor
        \param rxSize the size of the receiving buffer
        \param txSize the size of the transmitting buffer
    */
    ElectronSerialPipe(int rxSize = 256, int txSize = 256);

    /** Destructor
    */
    virtual ~ElectronSerialPipe(void);

    /** Initialization member function
    */
    void begin(unsigned baud, bool hwFlowCtrl);

    void end();

    // tx channel
    //----------------------------------------------------

    /** check if writable
        return the number of free bytes
    */
    int writeable(void);

    /** send a character (blocking)
        \param c the character to send
        \return c
    */
    int putc(int c);

    /** send a buffer
        \param buffer the buffer to send
        \param length the size of the buffer to send
        \param blocking, if true this function will block
               until all bytes placed in the buffer.
        \return the number of bytes written
    */
    int put(const void* buffer, int length, bool blocking);

    // rx channel
    //----------------------------------------------------

    /** check if readable
        \return the size available in the buffer.
    */
    int readable(void);

    /** receive one character from the serial port (blocking)
        \param the character received
    */
    int getc(void);

    /** read a buffer from the serial port
        \param pointer to the buffer to read.
        \param length number of bytes to read
        \param blocking true if all bytes shall be read. false if only the available bytes.
        \return the number of bytes read.
    */
    int get(void* buffer, int length, bool blocking);

    /** receive interrupt routine
    */
    void rxIrqBuf(void);

    /** transmit interrupt routine
    */
    void txIrqBuf(void);

    /** resumes paused receiver (hardware flow control)
    */
    void rxResume();

    /** pauses receiver (deasserts RTS line - hardware flow control)
    */
    void rxPause();

    /** returns size of TX pipe
    */
    int txSize();

    /** returns size of RX pipe
    */
    int rxSize();

    /** returns size of TX pipe
    */
    void txDump();

    /** returns size of RX pipe
    */
    void rxDump();

protected:
    //! start transmission helper
    void txStart(void);
    //! move bytes to hardware
    void txCopy(void);

    Pipe<char> _pipeRx; //!< receive pipe
    Pipe<char> _pipeTx; //!< transmit pipe
    int baud_;
    bool pause_, hwFlowCtrl_;
};
