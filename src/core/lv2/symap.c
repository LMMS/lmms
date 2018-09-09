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

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "symap.h"

/**
  @file symap.c Implementation of Symap, a basic symbol map (string interner).

  This implementation is primitive, but has some desirable qualities: good
  (O(lg(n)) lookup performance for already-mapped symbols, minimal space
  overhead, extremely fast (O(1)) reverse mapping (ID to string), simple code,
  no dependencies.

  The tradeoff is that mapping new symbols may be quite slow.  In other words,
  this implementation is ideal for use cases with a relatively limited set of
  symbols, or where most symbols are mapped early.  It will not fare so well
  with very dynamic sets of symbols.  For that, you're better off with a
  tree-based implementation (and the associated space cost, especially if you
  need reverse mapping).
*/

struct SymapImpl {
	/**
	   Unsorted array of strings, such that the symbol for ID i is found
	   at symbols[i - 1].
	*/
	char** symbols;

	/**
	   Array of IDs, sorted by corresponding string in `symbols`.
	*/
	uint32_t* index;

	/**
	   Number of symbols (number of items in `symbols` and `index`).
	*/
	uint32_t size;
};

Symap*
symap_new(void)
{
	Symap* map = (Symap*)malloc(sizeof(Symap));
	map->symbols = NULL;
	map->index   = NULL;
	map->size    = 0;
	return map;
}

void
symap_free(Symap* map)
{
	for (uint32_t i = 0; i < map->size; ++i) {
		free(map->symbols[i]);
	}

	free(map->symbols);
	free(map->index);
	free(map);
}

static char*
symap_strdup(const char* str)
{
	const size_t len  = strlen(str);
	char*        copy = (char*)malloc(len + 1);
	memcpy(copy, str, len + 1);
	return copy;
}

/**
   Return the index into map->index (not the ID) corresponding to `sym`,
   or the index where a new entry for `sym` should be inserted.
*/
static uint32_t
symap_search(const Symap* map, const char* sym, bool* exact)
{
	*exact = false;
	if (map->size == 0) {
		return 0;  // Empty map, insert at 0
	} else if (strcmp(map->symbols[map->index[map->size - 1] - 1], sym) < 0) {
		return map->size;  // Greater than last element, append
	}

	uint32_t lower = 0;
	uint32_t upper = map->size - 1;
	uint32_t i     = upper;
	int      cmp;

	while (upper >= lower) {
		i   = lower + ((upper - lower) / 2);
		cmp = strcmp(map->symbols[map->index[i] - 1], sym);

		if (cmp == 0) {
			*exact = true;
			return i;
		} else if (cmp > 0) {
			if (i == 0) {
				break;  // Avoid underflow
			}
			upper = i - 1;
		} else {
			lower = ++i;
		}
	}

	assert(!*exact || strcmp(map->symbols[map->index[i] - 1], sym) > 0);
	return i;
}

uint32_t
symap_try_map(Symap* map, const char* sym)
{
	bool           exact;
	const uint32_t index = symap_search(map, sym, &exact);
	if (exact) {
		assert(!strcmp(map->symbols[map->index[index]], sym));
		return map->index[index];
	}

	return 0;
}

uint32_t
symap_map(Symap* map, const char* sym)
{
	bool           exact;
	const uint32_t index = symap_search(map, sym, &exact);
	if (exact) {
		assert(!strcmp(map->symbols[map->index[index] - 1], sym));
		return map->index[index];
	}

	const uint32_t id  = ++map->size;
	char* const    str = symap_strdup(sym);

	/* Append new symbol to symbols array */
	map->symbols = (char**)realloc(map->symbols, map->size * sizeof(str));
	map->symbols[id - 1] = str;

	/* Insert new index element into sorted index */
	map->index = (uint32_t*)realloc(map->index, map->size * sizeof(uint32_t));
	if (index < map->size - 1) {
		memmove(map->index + index + 1,
		        map->index + index,
		        (map->size - index - 1) * sizeof(uint32_t));
	}

	map->index[index] = id;

	return id;
}

const char*
symap_unmap(Symap* map, uint32_t id)
{
	if (id == 0) {
		return NULL;
	} else if (id <= map->size) {
		return map->symbols[id - 1];
	}
	return NULL;
}

#ifdef STANDALONE

#include <stdio.h>

static void
symap_dump(Symap* map)
{
	fprintf(stderr, "{\n");
	for (uint32_t i = 0; i < map->size; ++i) {
		fprintf(stderr, "\t%u = %s\n",
		        map->index[i], map->symbols[map->index[i] - 1]);
	}
	fprintf(stderr, "}\n");
}

int
main()
{
	#define N_SYMS 5
	char* syms[N_SYMS] = {
		"hello", "bonjour", "goodbye", "aloha", "salut"
	};

	Symap* map = symap_new();
	for (int i = 0; i < N_SYMS; ++i) {
		if (symap_try_map(map, syms[i])) {
			fprintf(stderr, "error: Symbol already mapped\n");
			return 1;
		}

		const uint32_t id = symap_map(map, syms[i]);
		if (strcmp(map->symbols[id - 1], syms[i])) {
			fprintf(stderr, "error: Corrupt symbol table\n");
			return 1;
		}

		if (symap_map(map, syms[i]) != id) {
			fprintf(stderr, "error: Remapped symbol to a different ID\n");
			return 1;
		}

		symap_dump(map);
	}

	symap_free(map);
	return 0;
}

#endif /* STANDALONE */
