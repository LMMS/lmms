/*
 * Microtuner.cpp - manage tuning and scale information of an instrument
 *
 * Copyright (c) 2020 Martin Pavelek <he29.HS/at/gmail.com>
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

#include "Engine.h"
#include "Keymap.h"
#include "Note.h"
#include "Scale.h"
#include "Song.h"

namespace lmms
{


Microtuner::Microtuner() :
	Model(nullptr, tr("Microtuner")),
	m_enabledModel(false, this, tr("Microtuner on / off")),
	m_scaleModel(this, tr("Selected scale")),
	m_keymapModel(this, tr("Selected keyboard mapping")),
	m_keyRangeImportModel(true)
{
	for (unsigned int i = 0; i < MaxScaleCount; i++)
	{
		m_scaleModel.addItem(QString::number(i) + ": " + Engine::getSong()->getScale(i)->getDescription());
	}

	for (unsigned int i = 0; i < MaxKeymapCount; i++)
	{
		m_keymapModel.addItem(QString::number(i) + ": " + Engine::getSong()->getKeymap(i)->getDescription());
	}
	connect(Engine::getSong(), SIGNAL(scaleListChanged(int)), this, SLOT(updateScaleList(int)));
	connect(Engine::getSong(), SIGNAL(keymapListChanged(int)), this, SLOT(updateKeymapList(int)));
}


/** \brief Return frequency for a given MIDI key, using the active mapping and scale.
 *  \param key A MIDI key number ranging from 0 to 127.
 *  \return Frequency in Hz; 0 if key is out of range or not mapped.
 */
float Microtuner::keyToFreq(int key, int userBaseNote) const
{
	if (key < 0 || key >= NumKeys) {return 0;}
	Song *song = Engine::getSong();
	if (!song) {return 0;}

	// Get keymap and scale selected at this moment
	std::shared_ptr<const Keymap> keymap = song->getKeymap(m_keymapModel.value());
	std::shared_ptr<const Scale> scale = song->getScale(m_scaleModel.value());
	const std::vector<Interval> &intervals = scale->getIntervals();

	// Convert MIDI key to scale degree + octave offset.
	// The octaves are primarily driven by the keymap wraparound: octave count is increased or decreased if the key
	// goes over or under keymap range. In case the keymap refers to a degree that does not exist in the scale, it is
	// assumed the keymap is non-repeating or just really big, so the octaves are also driven by the scale wraparound.
	const int keymapDegree = keymap->getDegree(key);		// which interval should be used according to the keymap
	if (keymapDegree == -1) {return 0;}						// key is not mapped, abort
	const int keymapOctave = keymap->getOctave(key);		// how many times did the keymap repeat
	const int octaveDegree = intervals.size() - 1;			// index of the interval with octave ratio
	if (octaveDegree == 0) {								// octave interval is 1/1, i.e. constant base frequency
		return keymap->getBaseFreq();						// → return the baseFreq directly
	}
	const int scaleOctave = keymapDegree / octaveDegree;

	// which interval should be used according to the scale and keymap together
	const int degree_rem = keymapDegree % octaveDegree;
	const int scaleDegree = degree_rem >= 0 ? degree_rem : degree_rem + octaveDegree;	// get true modulo

	// Compute base note (the "A4 reference") degree and octave
	const int baseNote = m_keyRangeImportModel.value() ? keymap->getBaseKey() : userBaseNote;
	const int baseKeymapDegree = keymap->getDegree(baseNote);
	if (baseKeymapDegree == -1) {return 0;}					// base key is not mapped, umm...
	const int baseKeymapOctave = keymap->getOctave(baseNote);
	const int baseScaleOctave = baseKeymapDegree / octaveDegree;

	const int baseDegree_rem = baseKeymapDegree % octaveDegree;
	const int baseScaleDegree = baseDegree_rem >= 0 ? baseDegree_rem : baseDegree_rem + octaveDegree;

	// Compute frequency of the middle note and return the final frequency
	const double octaveRatio = intervals[octaveDegree].getRatio();
	const float middleFreq = (keymap->getBaseFreq() / pow(octaveRatio, (baseScaleOctave + baseKeymapOctave)))
								/ intervals[baseScaleDegree].getRatio();

	return middleFreq * intervals[scaleDegree].getRatio() * pow(octaveRatio, keymapOctave + scaleOctave);
}

int Microtuner::octaveSize() const
{
	const int keymapSize = Engine::getSong()->getKeymap(currentKeymap())->getSize();
	if (keymapSize > 0)
	{
		return keymapSize;
	}

	// Determine octave size from the scale if the keymap isn't in use.
	return Engine::getSong()->getScale(currentScale())->getIntervals().size() - 1;
}

/**
 * \brief Update scale name displayed in the microtuner scale list.
 * \param index Index of the scale to update; update all scales if -1 or out of range.
 */
void Microtuner::updateScaleList(int index)
{
	if (index >= 0 && static_cast<std::size_t>(index) < MaxScaleCount)
	{
		m_scaleModel.replaceItem(index,
			QString::number(index) + ": " + Engine::getSong()->getScale(index)->getDescription());
	}
	else
	{
		for (auto i = std::size_t{0}; i < MaxScaleCount; i++)
		{
			m_scaleModel.replaceItem(i,
				QString::number(i) + ": " + Engine::getSong()->getScale(i)->getDescription());
		}
	}
}

/**
 * \brief Update keymap name displayed in the microtuner scale list.
 * \param index Index of the keymap to update; update all keymaps if -1 or out of range.
 */
void Microtuner::updateKeymapList(int index)
{
	if (index >= 0 && static_cast<std::size_t>(index) < MaxKeymapCount)
	{
		m_keymapModel.replaceItem(index,
			QString::number(index) + ": " + Engine::getSong()->getKeymap(index)->getDescription());
	}
	else
	{
		for (auto i = std::size_t{0}; i < MaxKeymapCount; i++)
		{
			m_keymapModel.replaceItem(i,
				QString::number(i) + ": " + Engine::getSong()->getKeymap(i)->getDescription());
		}
	}
}


void Microtuner::saveSettings(QDomDocument &document, QDomElement &element)
{
	m_enabledModel.saveSettings(document, element, "enabled");
	m_scaleModel.saveSettings(document, element, "scale");
	m_keymapModel.saveSettings(document, element, "keymap");
	m_keyRangeImportModel.saveSettings(document, element, "range_import");
}


void Microtuner::loadSettings(const QDomElement &element)
{
	m_enabledModel.loadSettings(element, "enabled");
	m_scaleModel.loadSettings(element, "scale");
	m_keymapModel.loadSettings(element, "keymap");
	m_keyRangeImportModel.loadSettings(element, "range_import");
}


} // namespace lmms
