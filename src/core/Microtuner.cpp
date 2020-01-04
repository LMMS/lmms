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


/** \brief Return frequency for a given MIDI key, using the active mapping and scale.
 *  \param key A MIDI key number ranging from 0 to 127.
 *  \param detune Note detuning amount (+- cents).
 *  \return Frequency in Hz; 0 if key is out of range or not mapped.
 */
float Microtuner::keyToFreq(int key, float detune) const
{
	if (key < 0 || key >= NumKeys) {return 0;}
	const int note = m_keymap[key] - m_instrumentTrack->baseNoteModel()->value();

	if (note < 0 || note >= NumNotes) {return 0;}
	float frequency = m_notemap[note];

	// detune or pitch shift: adjust frequency by a given number of cents
	frequency *= powf(2.f, detune / (100 * 12.f));

	return frequency;
}


void Microtuner::saveSettings(QDomDocument &document, QDomElement &element)
{
}

void Microtuner::loadSettings(const QDomElement &element)
{
}
