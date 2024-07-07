/*
 * Keymap.h - holds information about a key mapping
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

#ifndef LMMS_KEYMAP_H
#define LMMS_KEYMAP_H

#include <vector>
#include <QObject>
#include <QString>

#include "SerializingObject.h"

namespace lmms
{

class Keymap : public QObject, public SerializingObject
{
	Q_OBJECT
public:
	Keymap();
	Keymap(
		QString description,
		std::vector<int> newMap,
		int newFirst,
		int newLast,
		int newMiddle,
		int newBaseKey,
		float newBaseFreq
	);

	QString getDescription() const;
	void setDescription(QString description);

	int getMiddleKey() const {return m_middleKey;}
	int getFirstKey() const {return m_firstKey;}
	int getLastKey() const {return m_lastKey;}
	int getBaseKey() const {return m_baseKey;}
	float getBaseFreq() const {return m_baseFreq;}

	std::size_t getSize() const {return m_map.size();}
	int getDegree(int key) const;
	int getOctave(int key) const;
	const std::vector<int> &getMap() const {return m_map;}

	void saveSettings(QDomDocument &doc, QDomElement &element) override;
	void loadSettings(const QDomElement &element) override;
	inline QString nodeName() const override {return "keymap";}

private:
	QString m_description;          //!< name or description of the keymap

	std::vector<int> m_map;         //!< key to scale degree mapping
	int m_firstKey;                 //!< first key that will be mapped
	int m_lastKey;                  //!< last key that will be mapped
	int m_middleKey;                //!< first line of the map refers to this key
	int m_baseKey;                  //!< key which is assigned the reference "base note"
	float m_baseFreq;               //!< frequency of the base note (usually A4 @440 Hz)
};

} // namespace lmms

#endif // LMMS_KEYMAP_H
