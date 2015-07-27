/*
 * Copyright (c) 2012 Mark McCurry
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef RTOSC_THREAD_LINK
#define RTOSC_THREAD_LINK

#include <cstring>
#include <cassert>
#include <cstdio>
#include <rtosc/rtosc.h>

namespace rtosc {
typedef const char *msg_t;

/**
 * ThreadLink - A simple wrapper around jack's ringbuffers desinged to make
 * sending messages via rt-osc trivial.
 * This class provides the basics of reading and writing events via fixed sized
 * buffers, which can be specified at compile time.
 */
class ThreadLink
{
    public:
        ThreadLink(size_t max_message_length, size_t max_messages);
        ~ThreadLink(void);

        /**
         * Write message to ringbuffer
         * @see rtosc_message()
         */
        void write(const char *dest, const char *args, ...);

        /**
         * Write an arary of arguments to ringbuffer
         * @see rtosc_amessage()
         */
        void writeArray(const char *dest, const char *args, const rtosc_arg_t *aargs);

        /**
         * Directly write message to ringbuffer
         */
        void raw_write(const char *msg);

        /**
         * @returns true iff there is another message to be read in the buffer
         */
        bool hasNext(void) const;

        /**
         * Read a new message from the ringbuffer
         */
        msg_t read(void);

        /**
         * Peak at last message read without reading another
         */
        msg_t peak(void) const;

        /**
         * Raw write buffer access for more complicated task
         */
        char *buffer(void);
        /**
         * Access to write buffer length
         */
        size_t buffer_size(void) const;
    private:
        const size_t MaxMsg;
        const size_t BufferSize;
        char *write_buffer;
        char *read_buffer;

        struct internal_ringbuffer_t *ring;
};
};
#endif
