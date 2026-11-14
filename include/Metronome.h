/*
 * Metronome.h
 *
 * Copyright (c) 2024 saker
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

#ifndef LMMS_METRONOME_H
#define LMMS_METRONOME_H

#include <cstddef>
#include <mutex>

#include "Sample.h"

namespace lmms {
class LMMS_EXPORT Metronome
{
public:
	bool active() const { return m_active; }
	void setActive(bool active) { m_active = active; }
	void processTick(int currentTick, int ticksPerBar, int beatsPerBar, size_t bufferOffset);
	void processBeat(int currentBeat, int numBeats);

private:
	// We have to lazily initialize the buffers because QString cannot be used before the QApplication is created
	void init();
	inline static Sample s_strong;
	inline static Sample s_weak;
	inline static std::once_flag s_initFlag;
	bool m_active = false;
};
} // namespace lmms

#endif // LMMS_METRONOME_H
