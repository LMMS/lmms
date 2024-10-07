/*
 * lv2_evbuf.cpp - Lv2 event buffer implementation
 *
 * Copyright (c) 2019-2020 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

/*
 * The original code was written by David Robillard <http://drobilla.net>
 */

#include "Lv2Evbuf.h"

#ifdef LMMS_HAVE_LV2

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <lv2/atom/atom.h>

namespace lmms
{


struct LV2_Evbuf_Impl {
	uint32_t capacity;
	uint32_t atom_Chunk;
	uint32_t atom_Sequence;
	LV2_Atom_Sequence buf;
};

static inline uint32_t
lv2_evbuf_pad_size(uint32_t size)
{
	return (size + 7) & (~7);
}

LV2_Evbuf*
lv2_evbuf_new(uint32_t capacity, uint32_t atom_Chunk, uint32_t atom_Sequence)
{
	// FIXME: memory must be 64-bit aligned
	auto evbuf = (LV2_Evbuf*)malloc(sizeof(LV2_Evbuf) + sizeof(LV2_Atom_Sequence) + capacity);
	evbuf->capacity = capacity;
	evbuf->atom_Chunk = atom_Chunk;
	evbuf->atom_Sequence = atom_Sequence;
	lv2_evbuf_reset(evbuf, true);
	return evbuf;
}

void
lv2_evbuf_free(LV2_Evbuf* evbuf)
{
	free(evbuf);
}

void
lv2_evbuf_reset(LV2_Evbuf* evbuf, bool input)
{
	if (input) {
		evbuf->buf.atom.size = sizeof(LV2_Atom_Sequence_Body);
		evbuf->buf.atom.type = evbuf->atom_Sequence;
	} else {
		evbuf->buf.atom.size = evbuf->capacity;
		evbuf->buf.atom.type = evbuf->atom_Chunk;
	}
}

uint32_t
lv2_evbuf_get_size(LV2_Evbuf* evbuf)
{
	assert(evbuf->buf.atom.type != evbuf->atom_Sequence
	|| evbuf->buf.atom.size >= sizeof(LV2_Atom_Sequence_Body));
	return evbuf->buf.atom.type == evbuf->atom_Sequence
		? evbuf->buf.atom.size - sizeof(LV2_Atom_Sequence_Body)
		: 0;
}

void*
lv2_evbuf_get_buffer(LV2_Evbuf* evbuf)
{
	return &evbuf->buf;
}

LV2_Evbuf_Iterator
lv2_evbuf_begin(LV2_Evbuf* evbuf)
{
	LV2_Evbuf_Iterator iter = { evbuf, 0 };
	return iter;
}

LV2_Evbuf_Iterator
lv2_evbuf_end(LV2_Evbuf* evbuf)
{
	const uint32_t size = lv2_evbuf_get_size(evbuf);
	const LV2_Evbuf_Iterator iter = { evbuf, lv2_evbuf_pad_size(size) };
	return iter;
}

bool
lv2_evbuf_is_valid(LV2_Evbuf_Iterator iter)
{
	return iter.offset < lv2_evbuf_get_size(iter.evbuf);
}

LV2_Evbuf_Iterator
lv2_evbuf_next(LV2_Evbuf_Iterator iter)
{
	if (!lv2_evbuf_is_valid(iter)) {
		return iter;
	}

	LV2_Evbuf* evbuf  = iter.evbuf;
	uint32_t   offset = iter.offset;

	const auto contents = static_cast<char*>(LV2_ATOM_CONTENTS(LV2_Atom_Sequence, &evbuf->buf.atom)) + offset;
	const uint32_t size = reinterpret_cast<LV2_Atom_Event*>(contents)->body.size;

	offset += lv2_evbuf_pad_size(sizeof(LV2_Atom_Event) + size);
	LV2_Evbuf_Iterator next = { evbuf, offset };
	return next;
}

bool
lv2_evbuf_get(LV2_Evbuf_Iterator iter,
	uint32_t* frames,
	uint32_t* type,
	uint32_t* size,
	uint8_t** data)
{
	*frames = *type = *size = 0;
	*data = nullptr;

	if (!lv2_evbuf_is_valid(iter)) {
		return false;
	}

	LV2_Atom_Sequence* aseq = &iter.evbuf->buf;
	auto aev = (LV2_Atom_Event*)((char*)LV2_ATOM_CONTENTS(LV2_Atom_Sequence, aseq) + iter.offset);

	*frames = aev->time.frames;
	*type = aev->body.type;
	*size = aev->body.size;
	*data = (uint8_t*)LV2_ATOM_BODY(&aev->body);

	return true;
}

bool
lv2_evbuf_write(LV2_Evbuf_Iterator* iter,
	uint32_t frames,
	uint32_t type,
	uint32_t size,
	const uint8_t* data)
{
	LV2_Atom_Sequence* aseq = &iter->evbuf->buf;
	if (iter->evbuf->capacity - sizeof(LV2_Atom) - aseq->atom.size <
		sizeof(LV2_Atom_Event) + size) {
		return false;
	}

	auto aev = (LV2_Atom_Event*)((char*)LV2_ATOM_CONTENTS(LV2_Atom_Sequence, aseq) + iter->offset);

	aev->time.frames = frames;
	aev->body.type = type;
	aev->body.size = size;
	memcpy(LV2_ATOM_BODY(&aev->body), data, size);

	size = lv2_evbuf_pad_size(sizeof(LV2_Atom_Event) + size);
	aseq->atom.size += size;
	iter->offset += size;

	return true;
}


} // namespace lmms

#endif // LMMS_HAVE_LV2
