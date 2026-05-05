/*
 * MidiImport.h - support for importing MIDI-files
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _MIDI_IMPORT_H
#define _MIDI_IMPORT_H

#include <cstdint>
#include <QString>

#include "MidiEvent.h"
#include "ImportFilter.h"

namespace lmms
{


class MidiImport : public ImportFilter
{
	Q_OBJECT
public:
	MidiImport(const QString& file);
	~MidiImport() override = default;
	gui::PluginView* instantiateView(QWidget*) override { return nullptr; }

private:
	bool tryImport(TrackContainer* tc) override;
	bool readSMF(TrackContainer* tc);
	bool readRIFF(TrackContainer* tc);
	bool readTrack(int track_end, QString& track_name);
	void error();

	inline std::int32_t read32LE()
	{
		std::int32_t value = readByte();
		value |= readByte() << 8;
		value |= readByte() << 16;
		value |= readByte() << 24;
		return value;
	}

	inline std::int32_t readVar()
	{
		std::int32_t c = readByte();
		std::int32_t value = c & 0x7f;
		if (c & 0x80)
		{
			c = readByte();
			value = (value << 7) | (c & 0x7f);
			if (c & 0x80)
			{
				c = readByte();
				value = (value << 7) | (c & 0x7f);
				if (c & 0x80)
				{
					c = readByte();
					value = (value << 7) | c;
					if (c & 0x80) { return -1; }
				}
			}
		}
		return !file().atEnd() ? value : -1;
	}

	inline void skip(unsigned num_bytes) { while (num_bytes--) { readByte(); } }

	int m_timingDivision = 0;
};


} // namespace lmms

#endif
