/*
 * TunerNote.h
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

#include <string>

#ifndef TUNER_NOTE_H
#define TUNER_NOTE_H

class TunerNote
{
public:
	enum class NoteName
	{
		A,
		ASharp,
		B,
		C,
		CSharp,
		D,
		DSharp,
		E,
		F,
		FSharp,
		G,
		GSharp,
		Count
	};

	TunerNote(NoteName name, int octave, float frequency);

	TunerNote calculateNoteFromFrequency(float frequency);

	void shiftByCents(int cents);
	void shiftBySemitones(int semitones);
	void shiftByOctaves(int octaves);

	NoteName name() const;
	int octave() const;
	int cents() const;
	float frequency() const;

	std::string nameToStr() const;
	std::string fullNoteName() const;

	void setName(NoteName name);
	void setOctave(int octave);
	void setCents(int cents);
	void setFrequency(float frequency);

private:
	NoteName m_name = NoteName::A;
	int m_octave = 0;
	int m_cents = 0;
	float m_frequency = 0.0f;
};

#endif