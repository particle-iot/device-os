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

#include <string.h>
#include <stddef.h>

#include "service_debug.h"

#ifdef putc
#undef putc
#undef getc
#endif


/** Pipe: this class implements a buffered pipe that can be safely
    written and read between two context. I.e., Written from a task
    and read from a interrupt.
*/
template <class T>
class Pipe
{
public:
    /* Constructor
        \param n size of the pipe/buffer
        \param b optional buffer that should be used.
                 if NULL the constructor will allocate a buffer of size n.
    */
    Pipe(int n, T* b = NULL)
    {
        _a = b ? NULL : n ? new T[n] : NULL;
        _r = 0;
        _w = 0;
        _b = b ? b : _a;
        _s = n;
    }
    /** Destructor
        frees a allocated buffer.
    */
    ~Pipe(void)
    {
        if (_a)
            delete [] _a;
    }

    /* This function can be used during debugging to hexdump the
       content of a buffer to the stdout.
    */
    void dump(void)
    {
        int o = _r;
        int MAX = size();
        char temp1[MAX*3];
        char temp2[4];
        sprintf(temp1,"pipe: %d/%d ", size(), _s);
        while (o != _w) {
            T t = _b[o];
            sprintf(temp2, "%0*X", sizeof(T)*2, t);
            strcat(temp1, temp2);
            o = _inc(o);
        }
        strcat(temp1,"\n");
        DEBUG_D(temp1);
    }

    // writing thread/context API
    //-------------------------------------------------------------

    /** Check if buffer is writeable (=not full)
        \return true if writeable
    */
    bool writeable(void)
    {
        return free() > 0;
    }

    /** Return the number of free elements in the buffer
        \return the number of free elements
    */
    int free(void)
    {
        int s = _r - _w;
        if (s <= 0)
            s += _s;
        return s - 1;
    }

    /* Add a single element to the buffer. (blocking)
        \param c the element to add.
        \return c
    */
    T putc(T c)
    {
        int i = _w;
        int j = i;
        i = _inc(i);
        while (i == _r) // = !writeable()
            /* nothing / just wait */;
        _b[j] = c;
        _w = i;
        return c;
    }

    /* Add a buffer of elements to the buffer.
        \param p the elements to add
        \param n the number elements to add from p
        \param t set to true if blocking, false otherwise
        \return number elements added
    */
    int put(const T* p, int n, bool t = false)
    {
        int c = n;
        while (c)
        {
            int f;
            for (;;) // wait for space
            {
                f = free();
                if (f > 0) break;     // data avail
                if (!t) return n - c; // no more space and not blocking
                /* nothing / just wait */;
            }
            // check free space
            if (c < f) f = c;
            int w = _w;
            int m = _s - w;
            // check wrap
            if (f > m) f = m;
            memcpy(&_b[w], p, f);
            _w = _inc(w, f);
            c -= f;
            p += f;
        }
        return n - c;
    }

    // reading thread/context API
    // --------------------------------------------------------

    /** Check if there are any emelemnt available (readble / not empty)
        \return true if readable/not empty
    */
    bool readable(void)
    {
        return (_r != _w);
    }

    /** Get the number of values available in the buffer
        return the number of element available
    */
    int size(void)
    {
        int s = _w - _r;
        if (s < 0)
            s += _s;
        return s;
    }

    /** get a single value from buffered pipe (this function will block if no values available)
        \return the element extracted
    */
    T getc(void)
    {
        int r = _r;
        while (r == _w) // = !readable()
            /* nothing / just wait */;
        T t = _b[r];
        _r = _inc(r);
        return t;
    }

    /*! get elements from the buffered pipe
        \param p the elements extracted
        \param n the maximum number elements to extract
        \param t set to true if blocking, false otherwise
        \return number elements extracted
    */
    int get(T* p, int n, bool t = false)
    {
        int c = n;
        while (c)
        {
            int f;
            for (;;) // wait for data
            {
                f = size();
                if (f)  break;        // free space
                if (!t) return n - c; // no space and not blocking
                /* nothing / just wait */;
            }
            // check available data
            if (c < f) f = c;
            int r = _r;
            int m = _s - r;
            // check wrap
            if (f > m) f = m;
            memcpy(p, &_b[r], f);
            _r = _inc(r, f);
            c -= f;
            p += f;
        }
        return n - c;
    }

    // the following functions are useful if you like to inspect
    // or parse the buffer in the reading thread/context
    // --------------------------------------------------------

    /** set the parsing index and return the number of available
        elments starting this position.
        \param ix the index to set.
        \return the number of elements starting at this position
    */
    int set(int ix)
    {
        int sz = size();
        ix = (ix > sz) ? sz : ix;
        _o = _inc(_r, ix);
        return sz - ix;
    }

    /** get the next element from parsing position and increment parsing index
        \return the extracted element.
    */
    T next(void)
    {
        int o = _o;
        T t = _b[o];
        _o = _inc(o);
        return t;
    }

    /** commit the index, mark the current parsing index as consumed data.
    */
    void done(void)
    {
        _r = _o;
    }

private:
    /** increment the index
        \param i index to increment
        \param n the step to increment
        \return the incremented index.
    */
    inline int _inc(int i, int n = 1)
    {
        i += n;
        if (i >= _s)
            i -= _s;
        return i;
    }

    T*            _b; //!< buffer
    T*            _a; //!< allocated buffer
    int           _s; //!< size of buffer (s - 1) elements can be stored
    volatile int  _w; //!< write index
    volatile int  _r; //!< read index
    int           _o; //!< offest index used by parsing functions
};
