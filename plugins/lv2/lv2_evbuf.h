/*
  Copyright 2008-2014 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef LV2_EVBUF_H
#define LV2_EVBUF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

/**
   Format of actual buffer.
*/
typedef enum {
	/**
	   An (old) ev:EventBuffer (LV2_Event_Buffer).
	*/
	LV2_EVBUF_EVENT,

	/**
	   A (new) atom:Sequence (LV2_Atom_Sequence).
	*/
	LV2_EVBUF_ATOM
} LV2_Evbuf_Type;

/**
   An abstract/opaque LV2 event buffer.
*/
typedef struct LV2_Evbuf_Impl LV2_Evbuf;

/**
   An iterator over an LV2_Evbuf.
*/
typedef struct {
	LV2_Evbuf* evbuf;
	uint32_t   offset;
} LV2_Evbuf_Iterator;

/**
   Allocate a new, empty event buffer.
   URIDs for atom:Chunk and atom:Sequence must be passed for LV2_EVBUF_ATOM.
*/
LV2_Evbuf*
lv2_evbuf_new(uint32_t       capacity,
              LV2_Evbuf_Type type,
              uint32_t       atom_Chunk,
              uint32_t       atom_Sequence);

/**
   Free an event buffer allocated with lv2_evbuf_new.
*/
void
lv2_evbuf_free(LV2_Evbuf* evbuf);

/**
   Reset and change the type of an existing event buffer.
   URIDs for atom:Chunk and atom:Sequence must be passed for LV2_EVBUF_ATOM.
*/
void
lv2_evbuf_set_type(LV2_Evbuf* evbuf, LV2_Evbuf_Type type);

/**
   Clear and initialize an existing event buffer.
   The contents of buf are ignored entirely and overwritten, except capacity
   which is unmodified.
   If input is false and this is an atom buffer, the buffer will be prepared
   for writing by the plugin.  This MUST be called before every run cycle.
*/
void
lv2_evbuf_reset(LV2_Evbuf* evbuf, bool input);

/**
   Return the total padded size of the events stored in the buffer.
*/
uint32_t
lv2_evbuf_get_size(LV2_Evbuf* evbuf);

/**
   Return the actual buffer implementation.
   The format of the buffer returned depends on the buffer type.
*/
void*
lv2_evbuf_get_buffer(LV2_Evbuf* evbuf);

/**
   Return an iterator to the start of `evbuf`.
*/
LV2_Evbuf_Iterator
lv2_evbuf_begin(LV2_Evbuf* evbuf);

/**
   Return an iterator to the end of `evbuf`.
*/
LV2_Evbuf_Iterator
lv2_evbuf_end(LV2_Evbuf* evbuf);

/**
   Check if `iter` is valid.
   @return True if `iter` is valid, otherwise false (past end of buffer)
*/
bool
lv2_evbuf_is_valid(LV2_Evbuf_Iterator iter);

/**
   Advance `iter` forward one event.
   `iter` must be valid.
   @return True if `iter` is valid, otherwise false (reached end of buffer)
*/
LV2_Evbuf_Iterator
lv2_evbuf_next(LV2_Evbuf_Iterator iter);

/**
   Dereference an event iterator (i.e. get the event currently pointed to).
   `iter` must be valid.
   `type` Set to the type of the event.
   `size` Set to the size of the event.
   `data` Set to the contents of the event.
   @return True on success.
*/
bool
lv2_evbuf_get(LV2_Evbuf_Iterator iter,
              uint32_t*          frames,
              uint32_t*          subframes,
              uint32_t*          type,
              uint32_t*          size,
              uint8_t**          data);

/**
   Write an event at `iter`.
   The event (if any) pointed to by `iter` will be overwritten, and `iter`
   incremented to point to the following event (i.e. several calls to this
   function can be done in sequence without twiddling iter in-between).
   @return True if event was written, otherwise false (buffer is full).
*/
bool
lv2_evbuf_write(LV2_Evbuf_Iterator* iter,
                uint32_t            frames,
                uint32_t            subframes,
                uint32_t            type,
                uint32_t            size,
                const uint8_t*      data);

#ifdef __cplusplus
}
#endif

#endif /* LV2_EVBUF_H */
