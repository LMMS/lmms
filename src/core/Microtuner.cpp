/*
 * Microtuner.cpp - manage tuning and scale information of an instrument
 *
 * Copyright (c) 2019 Martin Pavelek <he29.HS/at/gmail.com>
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


#include "Microtuner.h"

#include "ConfigManager.h"
#include "InstrumentTrack.h"

#include <iostream>

Microtuner::Microtuner(InstrumentTrack *parent) :
	Model(parent, tr("Microtuner")),
	m_instrumentTrack(parent),
	m_enabledModel(false, this, tr("Microtuner on / off"))
{
	// TEMP: default 1:1 keyboard mapping (to be acquired from microtuner storage class)
	m_keymap = new int[NumKeys];
	for (int i = 0; i < NumKeys; i++)
	{
		m_keymap[i] = i + (NumKeys - 1);
	}
	// TEMP: default 12-TET scale mapping
	m_notemap = new float[NumNotes];
	for (int i = 0; i < NumNotes; i++)
	{
		int scalePos = i - (NumKeys - 1);
		m_notemap[i] = DefaultBaseFreq * powf(2.0f, scalePos / 12.f);
	}
}


Microtuner::~Microtuner()
{
	delete[] m_keymap;	// TEMP: remove locally generated default keymap
	delete[] m_notemap;	// TEMP: remove locally generated default notemap
}


/** \brief Map MIDI instrument key to note number.
 *  \param key A MIDI key number ranging from 0 to 127.
 *  \return -1 when key is out of range or not mapped, otherwise note number from 0 to 255.
 */
int Microtuner::keyToNote(int key) const
{
	return (key >= 0 && key < NumKeys) ? m_keymap[key] - m_instrumentTrack->baseNoteModel()->value() : -1;
}


/** \brief Find the nearest mapped and valid key in a given direction.
 *  \param key Starting MIDI key (range 0 to 127).
 *  \param direction Search upwards if positive, downwards if negative.
 *  \return Nearest key with valid note mapping; -1 if not possible.
 */
int Microtuner::nearestKey(int key, int direction) const
{
	if (key < 0) {key = 0;}
	if (key >= NumKeys) {key = NumKeys -1;}

	// search for keys mapped to a note in the given direction
	direction = direction > 0 ? 1 : -1;
	for (int i = key; i > 0 && i < KeyNum ; i += direction)
	{
		if (m_keymap[i] != -1) {return i;}
	}

	// no valid key found; try to find one in the other direction
	direction *= -1;
	for (int i = key + direction; i > 0 && i < KeyNum ; i += direction)
	{
		if (m_keymap[i] != -1) {return i;}
	}

	// all keys are disabled / unmapped
	return -1;
}


/** \brief Return frequency for a given MIDI key, using the active mapping and scale.
 *  \param key A MIDI key number ranging from 0 to 127.
 *  \param detune Note detuning (+- number of keys).
 *  \param pitchShift Global detune / pitch shift (+- cents).
 *  \return Frequency in Hz; 0 if key is out of range or not mapped.
 */
float Microtuner::keyToFreq(int key, float detune, float pitchShift) const
{
	float frequency;

	// note detuning: smoothly transition between neighbouring keys
	if (detune != 0)
	{
//1.25	1.75	-1.25	-1.75
+4 target
*
+2.25--detune
+2
*
0 key
*
-2
*

// .. OK, problem: one assumption I made is not valid: notes increase in frequency - X
	// - noty klidne muzou byt na preskacku, jako v Rali JI scale
	// - to znamena ze pokud budu interpolovat dve base a mezi nima je shadow s mnohem vyssi frekvenci...
	// - on such scale, note detune would be completely unusable..

		// find the two keys closest to the detuned position and translate them to notes
		const float detunedKey = key + detune;
		const int startKey = detune > 0 ? floor(detunedKey) : ceil(detunedKey);
		const int endKey = detune > 0 ? ceil(detunedKey) : floor(detunedKey);
		// 
		const int startNote = keyToNote(nearestKey(startKey, -1 * detune));
		const int endNote = keyToNote(nearestKey(endKey, detune));
		// check if sensible notes were found
		if (startNote == -1 || endNote == -1) {return 0;}

		frequency *= powf(2.f, 
	}
	else
	{
		// no note detuning, map the key directly to note
		const int note = keyToNote(key);
		if (note < 0 || note >= NumNotes) {return 0;}

		frequency = m_notemap[note];
		std::cout<<"freq raw"<<frequency<<", ";
	}

	// pitch shift: adjust frequency by a given number of cents
	frequency *= powf(2.f, pitchShift / (100 * 12.f));

	std::cout<<"freq fin "<<frequency<<std::endl;

	return frequency;
}


void Microtuner::saveSettings(QDomDocument &document, QDomElement &element)
{
}

void Microtuner::loadSettings(const QDomElement &element)
{
}
