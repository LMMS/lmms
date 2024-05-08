/*
 * MidiEventToByteSeq.h - writeToByteSeq declaration
 *
 * Copyright (c) 2020-2020 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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

#ifndef LMMS_MIDIEVENTTOBYTESEQ_H
#define LMMS_MIDIEVENTTOBYTESEQ_H

#include <cstddef>
#include <cstdint>

#include "lmms_export.h"


namespace lmms
{

/**
	Write MIDI event into byte sequence.

	Conforming to http://lv2plug.in/ns/ext/midi#MidiEvent

	@param data Pointer to the target buffer for the byte sequence. Must
		point to existing memory with at least 3 bytes size.
	@param bufsize Available size of the target buffer.
	@return Used size of the target buffer, or 0 if the MidiEvent could not
		be converted.
*/
std::size_t LMMS_EXPORT writeToByteSeq( const class MidiEvent& ev,
										uint8_t* data, std::size_t bufsize );


} // namespace lmms

#endif // LMMS_MIDIEVENTTOBYTESEQ_H
