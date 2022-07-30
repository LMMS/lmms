/*
 * MidiEventToByteSeq.cpp - writeToByteSeq implementation
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

#include "MidiEventToByteSeq.h"

#include <QtGlobal>

#include "MidiEvent.h"


namespace lmms
{


std::size_t writeToByteSeq(
	const MidiEvent& ev, uint8_t *data, std::size_t bufsize)
{
	Q_ASSERT(bufsize >= 3);

	std::size_t size = 0;
	data[0] = ev.type() | (ev.channel() & 0x0F);

	switch (ev.type())
	{
		case MidiNoteOn:
			if (ev.velocity() > 0)
			{
				if (ev.key() < 0 || ev.key() > MidiMaxKey)
					break;

				data[1] = ev.key();
				data[2] = ev.velocity();
				size    = 3;
				break;
			}
			else
			{
				// Lv2 MIDI specs:
				// "Note On messages with velocity 0 are not allowed.
				// These messages are equivalent to Note Off in standard
				// MIDI streams, but here only proper Note Off messages
				// are allowed."
				data[0] = MidiNoteOff | (ev.channel() & 0x0F);
				// nobreak
			}

		case MidiNoteOff:
			if (ev.key() < 0 || ev.key() > MidiMaxKey)
				break;
			data[1] = ev.key();
			data[2] = ev.velocity(); // release time
			size    = 3;
			break;

		case MidiKeyPressure:
			data[1] = ev.key();
			data[2] = ev.velocity();
			size    = 3;
			break;

		case MidiControlChange:
			data[1] = ev.controllerNumber();
			data[2] = ev.controllerValue();
			size    = 3;
			break;

		case MidiProgramChange:
			data[1] = ev.program();
			size    = 2;
			break;

		case MidiChannelPressure:
			data[1] = ev.channelPressure();
			size    = 2;
			break;

		case MidiPitchBend:
			data[1] = ev.pitchBend() & 0x7f;
			data[2] = ev.pitchBend() >> 7;
			size    = 3;
			break;

		default:
			// unhandled
			break;
	}

	return size;
}


} // namespace lmms
