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

	float getBaseFreq() const {return m_baseFreq;}

	int map(int key, int baseNote) const {return m_map[key] - baseNote;}

private:
	QString m_description;

	float m_baseFreq;			//!< frequency of the base note (usually A4 @440 Hz)
	int m_map [NumKeys];		//!< the actual keymap
};

#endif
