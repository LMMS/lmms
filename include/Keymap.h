/*
 * Keymap.h - holds information about a scale and its intervals
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

#ifndef KEYMAP_H
#define KEYMAP_H

#include <vector>
#include <QObject>
#include <QString>

#include "Note.h"

class Keymap : public QObject
{
	Q_OBJECT
public:
	Keymap();
	Keymap(QString description);

	QString getDescription() const;
	void setDescription(QString description);

	int getMiddleKey() const {return m_middleKey;}
	int getFirstKey() const {return m_firstKey;}
	int getLastKey() const {return m_lastKey;}
	float getBaseFreq() const {return m_baseFreq;}

	int getSize() const {return m_map.size();}
	int getDegree(int key) const;
	int getOctave(int key) const;
	const std::vector<int> &getMap() const {return m_map;}

private:
	QString m_description;				//!< name or description of the keymap

	std::vector<int> m_map;				//!< key to scale degree mapping
	int m_middleKey;					//!< first line of the map refers to this key
	int m_firstKey;						//!< first key that will be mapped
	int m_lastKey;						//!< last key that will be mapped
	float m_baseFreq;					//!< frequency of the base note (usually A4 @440 Hz)
};

#endif
