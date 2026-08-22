/*
 * MidiPatch.h
 *
 * Copyright (c) 2026 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#ifndef LMMS_MIDI_PATCH_H
#define LMMS_MIDI_PATCH_H

#include <cstdint>

namespace lmms
{

struct MidiPatch
{
	//! 14-bit bank
	std::uint16_t bank = 0;

	//! 7-bit program
	std::uint8_t program = 0;

	constexpr auto bankMSB() const -> std::uint8_t { return (bank >> 7) & 0x7F; }
	constexpr auto bankLSB() const -> std::uint8_t { return bank & 0x7F; }
};

} // namespace lmms

#endif // LMMS_MIDI_PATCH_H
