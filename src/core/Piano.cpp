/*
 * Piano.cpp - implementation of piano-widget used in instrument-track-window
 *             for testing + according model class
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

/** \file Piano.cpp
 *  \brief A piano keyboard to play notes on in the instrument plugin window.
 */

/*
 * \mainpage Instrument plugin keyboard display classes
 *
 * \section introduction Introduction
 *
 * \todo fill this out
 * \todo write isWhite inline function and replace throughout
 */

#include "Piano.h"

#include "InstrumentTrack.h"


namespace lmms
{


//! The black/white order of keys as they appear on the keyboard.
static const auto KEY_ORDER = std::array
{
//	C                      CIS                    D                      DIS
	Piano::KeyType::White, Piano::KeyType::Black, Piano::KeyType::White, Piano::KeyType::Black,
//	E                      F                      FIS                    G
	Piano::KeyType::White, Piano::KeyType::White, Piano::KeyType::Black, Piano::KeyType::White,
//	GIS                    A                      AIS                    B
	Piano::KeyType::Black, Piano::KeyType::White, Piano::KeyType::Black, Piano::KeyType::White
} ;


Piano::Piano(InstrumentTrack* track)
	: Model(nullptr)
	, m_instrumentTrack(track)
	, m_midiEvProc(track)
{}


void Piano::setKeyState(int key, bool state)
{
	if (isValidKey(key))
	{
		m_pressedKeys[key] = state;

		emit dataChanged();
	}
}


void Piano::handleKeyPress(int key, int midiVelocity)
{
	if (midiVelocity == -1)
	{
		midiVelocity = m_instrumentTrack->midiPort()->baseVelocity();
	}
	if (isValidKey(key))
	{
		m_midiEvProc->processInEvent(MidiEvent(MidiNoteOn, -1, key, midiVelocity));
		m_pressedKeys[key] = true;
	}
}


void Piano::handleKeyRelease(int key)
{
	if (isValidKey(key))
	{
		m_midiEvProc->processInEvent(MidiEvent(MidiNoteOff, -1, key, 0));
		m_pressedKeys[key] = false;
	}
}



bool Piano::isBlackKey(int key)
{
	int keyCode = key % KeysPerOctave;

	return KEY_ORDER[keyCode] == Piano::KeyType::Black;
}


bool Piano::isWhiteKey(int key)
{
	return !isBlackKey(key);
}


} // namespace lmms
