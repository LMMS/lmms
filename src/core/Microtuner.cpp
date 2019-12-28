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


Microtuner::Microtuner(InstrumentTrack *parent) :
	Model(parent, tr("Microtuner")),
	m_instrumentTrack(parent),
	m_enabledModel(false, this, tr("Microtuner on / off")),
	m_baseFreqModel(DefaultBaseFreq, 1.f, 1000.f, 0.01f, this, tr("Base note frequency [Hz]"))
{
	// TEMP: default 1:1 keyboard mapping (to be acquired from microtuner storage class)
	m_keymap = new int[MidiNoteCount];
	for (int i = 0; i < MidiNoteCount; i++) {m_keymap[i] = i;}
}


Microtuner::~Microtuner()
{
	delete[] m_keymap;	// TEMP: remove locally generated default keymap
}


/** \brief Map MIDI instrument key to note number.
 *  \param key A MIDI key number ranging from 0 to 127.
 *  \return -1 when key is out of range or not mapped, otherwise note number from 0 to 127.
 */
// TODO: maybe notes shouldn't be limited to 0..127? Base note shift should allow ultra/sub-sonic..
// Everything points to on-the-fly computation; lookup table is too limiting.
// Although, here I just have a note number, basenote does not affect it..
int Microtuner::keyToNote(int key) const
{
	return (key >= 0 && key < MidiNoteCount) ? m_keymap[key] : -1;
}


/** \brief Return frequency for a given note of the active scale.
 *  \param note A note number ranging from 0 to 127.
 *  \return Frequency in Hz; 0 if note is out of range or has invalid (-1) mapping.
 */
float Microtuner::noteToFreq(int note) const
{
	if (note < 0 || note >= MidiNoteCount) {return 0;}

	// get positive or negative note position on scale, relative to base note
	int scalePos = note - m_instrumentTrack->baseNote();

	// temporary 12-TET conversion
	return baseFreq() * powf(2.0f, scalePos / 12.f);
}


/** \brief Return frequency for a given MIDI key, using the active mapping and scale.
 *  \param note A MIDI key number ranging from 0 to 127.
 *  \return Frequency in Hz; 0 if key is out of range or not mapped.
 */
float Microtuner::keyToFreq(int key) const
{
	return noteToFreq(keyToNote(key));
}


void Microtuner::saveSettings(QDomDocument &document, QDomElement &element)
{
}

void Microtuner::loadSettings(const QDomElement &element)
{
}
