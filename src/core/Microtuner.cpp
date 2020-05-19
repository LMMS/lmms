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

#include <vector>
#include <cmath>

#include "ConfigManager.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "Keymap.h"
#include "Scale.h"
#include "Song.h"


Microtuner::Microtuner(InstrumentTrack *parent) :
	Model(parent, tr("Microtuner")),
	m_instrumentTrack(parent),
	m_enabledModel(false, this, tr("Microtuner on / off"))
{
}


Microtuner::~Microtuner()
{
}


/** \brief Return base frequency, based on currently selected keymap.
 *  \return Frequency in Hz
 */
float Microtuner::baseFreq() const
{
	if (enabled())
	{
		return Engine::getSong()->getKeymap(m_instrumentTrack->keymapModel()->value()).getBaseFreq();
	}
	else
	{
		return DefaultBaseFreq;
	}
}


/** \brief Return frequency for a given MIDI key, using the active mapping and scale.
 *  \param key A MIDI key number ranging from 0 to 127.
 *  \param detune Note detuning amount (+- cents).
 *  \return Frequency in Hz; 0 if key is out of range or not mapped.
 */
float Microtuner::keyToFreq(int key, float detune) const
{
	if (key < 0 || key >= NumKeys) {return 0;}

	// Get keymap and scale selected at this moment
	const unsigned int keymap_id = m_instrumentTrack->keymapModel()->value();
	const unsigned int scale_id = m_instrumentTrack->scaleModel()->value();

	const Keymap &keymap = Engine::getSong()->getKeymap(keymap_id);
	const Scale &scale = Engine::getSong()->getScale(scale_id);

	// Convert MIDI key to LMMS note number (affected by key mapping and base note)
	const int note = keymap.map(key, m_instrumentTrack->baseNoteModel()->value());
	if (note < 0 || note >= NumNotes) {return 0;}

	// Convert LMMS note to frequency
	const std::vector<Interval> &intervals = scale.getIntervals();	//TODO: intervals may be empty for fraction of a second during definition update; also, the size could change before subsequent reads.. but copy would be nasty, it may be quite large..
	const int octaveDegree = intervals.size() - 1;
	const double octaveRatio = intervals[octaveDegree].getRatio();

	const int scalePos = note - (NumKeys - 1);	//FIXME wtf, why does this work; should be minus the middle note?
	const int degree_rem = scalePos % octaveDegree;
	const int degree = degree_rem >= 0 ? degree_rem : degree_rem + octaveDegree;	// get true modulo

	float frequency = keymap.getBaseFreq() * intervals[degree].getRatio() *
					pow(octaveRatio, floor((float)scalePos / octaveDegree));

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
