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
		return Engine::getSong()->getKeymap(m_instrumentTrack->keymapModel()->value())->getBaseFreq();
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
	Song *song = Engine::getSong();
	if (song == NULL) {return 0;}

	// Get keymap and scale selected at this moment
	const unsigned int keymap_id = m_instrumentTrack->keymapModel()->value();
	const unsigned int scale_id = m_instrumentTrack->scaleModel()->value();
	std::shared_ptr<const Keymap> keymap = song->getKeymap(keymap_id);
	std::shared_ptr<const Scale> scale = song->getScale(scale_id);
	const std::vector<Interval> &intervals = scale->getIntervals();

	// Convert MIDI key to scale degree + octave offset.
	// The octaves are primarily driven by the keymap wraparound: octave count is increased or decreased if the key
	// goes over or under keymap range. In case the keymap refers to a degree that does not exist in the scale, it is
	// assumed the keymap is non-repeating or just really big, so the octaves are also driven by the scale wraparound.
	const int keymapDegree = keymap->getDegree(key);		// which interval should be used according to the keymap
	if (keymapDegree == -1) {return 0;}						// key is not mapped
	const int keymapOctave = keymap->getOctave(key);		// how many times did the keymap repeat
	const int octaveDegree = intervals.size() - 1;
	const int scaleOctave = keymapDegree >= 0 ? keymapDegree / octaveDegree : keymapDegree / octaveDegree - 1;

	const int degree_rem = keymapDegree % octaveDegree;
	const int scaleDegree = degree_rem >= 0 ? degree_rem : degree_rem + octaveDegree;	// get true modulo

	// Compute base note (the "A4 reference") degree and octave
	const int baseNote = m_instrumentTrack->baseNoteModel()->value();
	const int baseKeymapDegree = keymap->getDegree(baseNote);
	const int baseKeymapOctave = keymap->getOctave(baseNote);
	const int baseScaleOctave = baseKeymapDegree >= 0 ?
		baseKeymapDegree / octaveDegree : baseKeymapDegree / octaveDegree - 1;

	const int baseDegree_rem = baseKeymapDegree % octaveDegree;
	const int baseScaleDegree = baseDegree_rem >= 0 ? baseDegree_rem : baseDegree_rem + octaveDegree;

	// Compute frequency of the middle note and the final frequency
	const double octaveRatio = intervals[octaveDegree].getRatio();
	const float middleFreq = (keymap->getBaseFreq() / pow(octaveRatio, (baseScaleOctave + baseKeymapOctave)))
								/ intervals[baseScaleDegree].getRatio();

	float frequency = middleFreq * intervals[scaleDegree].getRatio() * pow(octaveRatio, keymapOctave + scaleOctave);

	// Detune or pitch shift: adjust frequency by a given number of cents
	frequency *= powf(2.f, detune / (100 * 12.f));

	return frequency;
}


void Microtuner::saveSettings(QDomDocument &document, QDomElement &element)
{
}

void Microtuner::loadSettings(const QDomElement &element)
{
}
