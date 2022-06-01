/*
 * TunerNote.cpp
 *
 * Copyright (c) 2022 sakertooth <sakertooth@gmail.com>
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

#include "TunerNote.h"

#include <cmath>
#include <string>

#include <iostream>

TunerNote::TunerNote(NoteName name, int octave, float frequency)
	: m_name(name)
	, m_octave(octave)
	, m_frequency(frequency)
{
}

TunerNote TunerNote::calculateNoteFromFrequency(float frequency)
{
	int approximateCentDistance = static_cast<int>(std::roundf(1200 * std::log2(frequency / m_frequency)));
	TunerNote note(*this);
	note.shiftByCents(approximateCentDistance);
	note.setFrequency(frequency);
	return note;
}

void TunerNote::shiftByCents(int cents)
{
	auto [semitonesToShift, remainingCents] = std::div(cents, 100);
	m_name = static_cast<NoteName>((((static_cast<int>(m_name) + semitonesToShift) % 12) + 12) % 12);
	m_octave += cents / 1200;
	m_cents += remainingCents;
}

void TunerNote::shiftBySemitones(int semitones)
{
	shiftByCents(semitones * 100);
}

void TunerNote::shiftByOctaves(int octaves)
{
	shiftByCents(octaves * 1200);
}

TunerNote::NoteName TunerNote::name() const
{
	return m_name;
}

int TunerNote::octave() const
{
	return m_octave;
}

int TunerNote::cents() const 
{
	return m_cents;
}

float TunerNote::frequency() const
{
	return m_frequency;
}

std::string TunerNote::nameToStr() const
{
	switch (m_name)
	{
	case NoteName::A:
		return "A";
	case NoteName::ASharp:
		return "A#";
	case NoteName::B:
		return "B";
	case NoteName::C:
		return "C";
	case NoteName::CSharp:
		return "C#";
	case NoteName::D:
		return "D";
	case NoteName::DSharp:
		return "D#";
	case NoteName::E:
		return "E";
	case NoteName::F:
		return "F";
	case NoteName::FSharp:
		return "F#";
	case NoteName::G:
		return "G";
	case NoteName::GSharp:
		return "G#";
	default:
		return "N/A";
	};
}

std::string TunerNote::fullNoteName() const
{
	return nameToStr() + std::to_string(m_octave);
}

void TunerNote::setName(NoteName name)
{
	m_name = name;
}

void TunerNote::setOctave(int octave)
{
	m_octave = octave;
}

void TunerNote::setCents(int cents)
{
	m_cents = cents;
}

void TunerNote::setFrequency(float frequency)
{
	m_frequency = frequency;
}