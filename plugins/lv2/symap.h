/*
  Copyright 2011-2014 David Robillard <http://drobilla.net>

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

/**
   @file symap.h API for Symap, a basic symbol map (string interner).

   Particularly useful for implementing LV2 URI mapping.

   @see <a href="http://lv2plug.in/ns/ext/urid">LV2 URID</a>
   @see <a href="http://lv2plug.in/ns/ext/uri-map">LV2 URI Map</a>
*/

#ifndef SYMAP_H
#define SYMAP_H

#include <stdint.h>

struct SymapImpl;

typedef struct SymapImpl Symap;

/**
   Create a new symbol map.
*/
Symap*
symap_new(void);

/**
   Free a symbol map.
*/
void
symap_free(Symap* map);

/**
   Map a string to a symbol ID if it is already mapped, otherwise return 0.
*/
uint32_t
symap_try_map(Symap* map, const char* sym);

/**
   Map a string to a symbol ID.

   Note that 0 is never a valid symbol ID.
*/
uint32_t
symap_map(Symap* map, const char* sym);

/**
   Unmap a symbol ID back to a symbol, or NULL if no such ID exists.

   Note that 0 is never a valid symbol ID.
*/
const char*
symap_unmap(Symap* map, uint32_t id);

#endif /* SYMAP_H */
