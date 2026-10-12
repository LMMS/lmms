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
void Metronome::processTick(int currentTick, int ticksPerBar, int beatsPerBar, size_t bufferOffset)
{
	std::call_once(s_initFlag, &Metronome::init, this);

	const auto ticksPerBeat = ticksPerBar / beatsPerBar;
	if (currentTick % ticksPerBeat != 0 || !m_active) { return; }

	const auto handle = currentTick % ticksPerBar == 0 ? new SamplePlayHandle{&s_strong}
													   : new SamplePlayHandle{&s_weak};
	handle->setOffset(bufferOffset);
	Engine::audioEngine()->addPlayHandle(handle);
}

void Metronome::processBeat(int currentBeat, int numBeats)
{
	std::call_once(s_initFlag, &Metronome::init, this);

	const auto handle = currentBeat % numBeats == 0 ? new SamplePlayHandle{&s_strong}
													: new SamplePlayHandle{&s_weak};
	Engine::audioEngine()->addPlayHandle(handle);
}

void Metronome::init()
{
	// The metronome sounds should always load successfully, otherwise something is wrong with the installation
	// We opt to throw an exception instead of handling the error in this case
	s_strong = Sample{SampleBuffer::fromFile("misc/metronome02.ogg").value()};
	s_weak = Sample{SampleBuffer::fromFile("misc/metronome01.ogg").value()};
}

} // namespace lmms
