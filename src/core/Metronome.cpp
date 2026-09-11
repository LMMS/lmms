/*
 * Metronome.cpp
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

#include "Metronome.h"

#include "Engine.h"
#include "SamplePlayHandle.h"

namespace lmms {
Metronome::Metronome(bool active)
	: m_active(active)
{
}

void Metronome::processTick(int currentTick, int ticksPerBar, int beatsPerBar, size_t bufferOffset)
{
	// Normally, these would be allocated outside of the processTick entirely, but because of some Qt shenanigans likely
	// related to PathUtil and the error handling in fromFile (error in console was
	// "QCoreApplication::applicationDirPath: Please instantiate the QApplication object first"), we have do it in here
	// for now
	static auto s_metronomeStrong = SampleBuffer::fromFile("misc/metronome02.ogg");
	static auto s_metronomeWeak = SampleBuffer::fromFile("misc/metronome01.ogg");

	const auto ticksPerBeat = ticksPerBar / beatsPerBar;
	if (currentTick % ticksPerBeat != 0 || !m_active) { return; }

	const auto handle = currentTick % ticksPerBar == 0 ? new SamplePlayHandle{new Sample{s_metronomeStrong}}
													   : new SamplePlayHandle{new Sample{s_metronomeWeak}};
	handle->setOffset(bufferOffset);
	Engine::audioEngine()->addPlayHandle(handle);
}
} // namespace lmms
