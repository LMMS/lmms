/*
  Copyright (C) 2000 Paul Davis
  Copyright (C) 2003 Rohan Drape
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.
  
  You should have received a copy of the GNU Lesser General Public License
  along with this program; if not, write to the Free Software 
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#ifndef _RINGBUFFER_H
#define _RINGBUFFER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/types.h>

/** @file ringbuffer.h
 *
 * A set of library functions to make lock-free ringbuffers available
 * to JACK clients.  The `capture_client.c' (in the example_clients
 * directory) is a fully functioning user of this API.
 *
 * The key attribute of a ringbuffer is that it can be safely accessed
 * by two threads simultaneously -- one reading from the buffer and
 * the other writing to it -- without using any synchronization or
 * mutual exclusion primitives.  For this to work correctly, there can
 * only be a single reader and a single writer thread.  Their
 * identities cannot be interchanged.
 */

typedef struct {
    char *buf;
    size_t len;
}
jack_ringbuffer_data_t ;

typedef struct {
    char	*buf;
    volatile size_t write_ptr;
    volatile size_t read_ptr;
    size_t	size;
    size_t	size_mask;
    int	mlocked;
}
jack_ringbuffer_t ;

/**
 * Allocates a ringbuffer data structure of a specified size. The
 * caller must arrange for a call to jack_ringbuffer_free() to release
 * the memory associated with the ringbuffer.
 *
 * @param sz the ringbuffer size in bytes.
 *
 * @return a pointer to a new jack_ringbuffer_t, if successful; NULL
 * otherwise.
 */
jack_ringbuffer_t *jack_ringbuffer_create(size_t sz);

/**
 * Frees the ringbuffer data structure allocated by an earlier call to
 * jack_ringbuffer_create().
 *
 * @param rb a pointer to the ringbuffer structure.
 */
void jack_ringbuffer_free(jack_ringbuffer_t *rb);

/**
 * Fill a data structure with a description of the current readable
 * data held in the ringbuffer.  This description is returned in a two
 * element array of jack_ringbuffer_data_t.  Two elements are needed
 * because the data to be read may be split across the end of the
 * ringbuffer.
 *
 * The first element will always contain a valid @a len field, which
 * may be zero or greater.  If the @a len field is non-zero, then data
 * can be read in a contiguous fashion using the address given in the
 * corresponding @a buf field.
 *
 * If the second element has a non-zero @a len field, then a second
 * contiguous stretch of data can be read from the address given in
 * its corresponding @a buf field.
 *
 * @param rb a pointer to the ringbuffer structure.
 * @param vec a pointer to a 2 element array of jack_ringbuffer_data_t.
 *
 */
void jack_ringbuffer_get_read_vector(const jack_ringbuffer_t *rb,
                                     jack_ringbuffer_data_t *vec);

/**
 * Fill a data structure with a description of the current writable
 * space in the ringbuffer.  The description is returned in a two
 * element array of jack_ringbuffer_data_t.  Two elements are needed
 * because the space available for writing may be split across the end
 * of the ringbuffer.
 *
 * The first element will always contain a valid @a len field, which
 * may be zero or greater.  If the @a len field is non-zero, then data
 * can be written in a contiguous fashion using the address given in
 * the corresponding @a buf field.
 *
 * If the second element has a non-zero @a len field, then a second
 * contiguous stretch of data can be written to the address given in
 * the corresponding @a buf field.
 *
 * @param rb a pointer to the ringbuffer structure.
 * @param vec a pointer to a 2 element array of jack_ringbuffer_data_t.
 */
void jack_ringbuffer_get_write_vector(const jack_ringbuffer_t *rb,
                                      jack_ringbuffer_data_t *vec);

/**
 * Read data from the ringbuffer.
 *
 * @param rb a pointer to the ringbuffer structure.
 * @param dest a pointer to a buffer where data read from the
 * ringbuffer will go.
 * @param cnt the number of bytes to read.
 *
 * @return the number of bytes read, which may range from 0 to cnt.
 */
size_t jack_ringbuffer_read(jack_ringbuffer_t *rb, char *dest, size_t cnt);

/**
 * Read data from the ringbuffer. Opposed to jack_ringbuffer_read()
 * this function does not move the read pointer. Thus it's
 * a convenient way to inspect data in the ringbuffer in a
 * continous fashion. The price is that the data is copied
 * into a user provided buffer. For "raw" non-copy inspection
 * of the data in the ringbuffer use jack_ringbuffer_get_read_vector().
 *
 * @param rb a pointer to the ringbuffer structure.
 * @param dest a pointer to a buffer where data read from the
 * ringbuffer will go.
 * @param cnt the number of bytes to read.
 *
 * @return the number of bytes read, which may range from 0 to cnt.
 */
size_t jack_ringbuffer_peek(jack_ringbuffer_t *rb, char *dest, size_t cnt);

/**
 * Advance the read pointer.
 *
 * After data have been read from the ringbuffer using the pointers
 * returned by jack_ringbuffer_get_read_vector(), use this function to
 * advance the buffer pointers, making that space available for future
 * write operations.
 *
 * @param rb a pointer to the ringbuffer structure.
 * @param cnt the number of bytes read.
 */
void jack_ringbuffer_read_advance(jack_ringbuffer_t *rb, size_t cnt);

/**
 * Return the number of bytes available for reading.
 *
 * @param rb a pointer to the ringbuffer structure.
 *
 * @return the number of bytes available to read.
 */
size_t jack_ringbuffer_read_space(const jack_ringbuffer_t *rb);

/**
 * Lock a ringbuffer data block into memory.
 *
 * Uses the mlock() system call.  This is not a realtime operation.
 *
 * @param rb a pointer to the ringbuffer structure.
 */
int jack_ringbuffer_mlock(jack_ringbuffer_t *rb);

/**
 * Reset the read and write pointers, making an empty buffer.
 *
 * This is not thread safe.
 *
 * @param rb a pointer to the ringbuffer structure.
 */
void jack_ringbuffer_reset(jack_ringbuffer_t *rb);

/**
 * Reset the internal "available" size, and read and write pointers, making an empty buffer.
 *
 * This is not thread safe.
 *
 * @param rb a pointer to the ringbuffer structure.
 * @param sz the new size, that must be less than allocated size.
 */
void jack_ringbuffer_reset_size (jack_ringbuffer_t * rb, size_t sz);

/**
 * Write data into the ringbuffer.
 *
 * @param rb a pointer to the ringbuffer structure.
 * @param src a pointer to the data to be written to the ringbuffer.
 * @param cnt the number of bytes to write.
 *
 * @return the number of bytes write, which may range from 0 to cnt
 */
size_t jack_ringbuffer_write(jack_ringbuffer_t *rb, const char *src,
                             size_t cnt);

/**
 * Advance the write pointer.
 *
 * After data have been written the ringbuffer using the pointers
 * returned by jack_ringbuffer_get_write_vector(), use this function
 * to advance the buffer pointer, making the data available for future
 * read operations.
 *
 * @param rb a pointer to the ringbuffer structure.
 * @param cnt the number of bytes written.
 */
void jack_ringbuffer_write_advance(jack_ringbuffer_t *rb, size_t cnt);

/**
 * Return the number of bytes available for writing.
 *
 * @param rb a pointer to the ringbuffer structure.
 *
 * @return the amount of free space (in bytes) available for writing.
 */
size_t jack_ringbuffer_write_space(const jack_ringbuffer_t *rb);

#ifdef __cplusplus
}
#endif

#endif
