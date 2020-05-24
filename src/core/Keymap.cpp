/*
 * Keymap.cpp - implementation of scale class
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

#include "Keymap.h"


Keymap::Keymap() :
	m_description(tr("default (440 Hz, all keys)")),
	m_middleKey(0),
	m_firstKey(0),
	m_lastKey(NumKeys - 1),
	m_baseFreq(440.f)
{
	// default 1:1 keyboard mapping
	for (int i = 0; i < NumKeys; i++)
	{
		m_map.push_back(i);
	}
}


Keymap::Keymap(QString description) :
	m_description(description)
{
}


/**
 * \brief Return scale degree for a given key, based on current map and first/middle/last notes
 * \param MIDI key to be mapped
 * \return Scale degree defined by the mapping on success, -1 if key isn't mapped
 */
int Keymap::getDegree(int key) const
{
	if (key < m_firstKey || key > m_lastKey) {return -1;}

	const int keyOffset = key - m_middleKey;								// -127..127
	const int key_rem = keyOffset % (int)m_map.size();							// remainder
	const int key_mod = key_rem >= 0 ? key_rem : key_rem + m_map.size();	// true modulo
	return m_map[key_mod];
}


/**
 * \brief Return octave offset for a given key, based on current map and the middle note
 * \param MIDI key to be mapped
 * \return Octave offset defined by the mapping on success, 0 if key isn't mapped
 */
int Keymap::getOctave(int key) const
{
	if (key < m_firstKey || key > m_lastKey) {return 0;}

	const int keyOffset = key - m_middleKey;
	return keyOffset >= 0 ? keyOffset / (int)m_map.size() : (keyOffset + 1) / (int)m_map.size() - 1;
}


QString Keymap::getDescription() const
{
	return m_description;
}


void Keymap::setDescription(QString description)
{
	m_description = description;
}

