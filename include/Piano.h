/*
 * Piano.h - declaration of class Piano
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_PIANO_H
#define LMMS_PIANO_H

#include <array>

#include "Model.h"
#include "Note.h"

namespace lmms
{


class InstrumentTrack;
class MidiEventProcessor;

class Piano final : public Model
{
public:
	enum class KeyType
	{
		White,
		Black
	} ;

	Piano(InstrumentTrack* track);

	void setKeyState(int key, bool state);

	bool isKeyPressed(int key) const
	{
		return m_pressedKeys[key];
	}

	void handleKeyPress(int key, int midiVelocity = -1);
	void handleKeyRelease(int key);

	InstrumentTrack* instrumentTrack() const
	{
		return m_instrumentTrack;
	}

	MidiEventProcessor* midiEventProcessor() const
	{
		return m_midiEvProc;
	}

	static bool isWhiteKey(int key);
	static bool isBlackKey(int key);

	static const unsigned int WhiteKeysPerOctave = 7;
	static const unsigned int BlackKeysPerOctave = 5;
	static const unsigned int NumWhiteKeys = 75;
	static const unsigned int NumBlackKeys = 53;

private:
	static bool isValidKey(int key)
	{
		return key >= 0 && key < NumKeys;
	}

	InstrumentTrack* m_instrumentTrack;
	MidiEventProcessor* m_midiEvProc;
	std::array<bool, NumKeys> m_pressedKeys = {};

} ;


} // namespace lmms

#endif // LMMS_PIANO_H
