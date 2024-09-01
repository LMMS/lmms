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
#include "Song.h"

namespace lmms {

bool Metronome::active() const
{
	return m_active;
}

void Metronome::setActive(bool active)
{
	m_active = active;
}

void Metronome::process(size_t bufferSize)
{
	const auto song = Engine::getSong();
	const auto currentPlayMode = song->playMode();
	const auto supported = currentPlayMode == Song::PlayMode::MidiClip || currentPlayMode == Song::PlayMode::Song
		|| currentPlayMode == Song::PlayMode::Pattern;

	if (!supported || !m_active || song->isExporting() || song->countTracks() == 0) { return; }

	const auto ticksPerBar = TimePos::ticksPerBar();
	const auto beatsPerBar = song->getTimeSigModel().getNumerator();

	const auto framesPerTick = static_cast<std::size_t>(Engine::framesPerTick());
	const auto framesPerBar = framesPerTick * ticksPerBar;
	const auto framesPerBeat = framesPerBar / beatsPerBar;

	const auto currentTick = song->getPlayPos(song->playMode()).getTicks();
	const auto currentFrameOffset = song->getPlayPos(song->playMode()).currentFrame();
	auto currentFrame = static_cast<std::size_t>(currentTick * framesPerTick + currentFrameOffset);

	for (auto frame = std::size_t{0}; frame < bufferSize; ++frame, ++currentFrame)
	{
		if (currentFrame % framesPerBeat != 0) { continue; }

		const auto handle = currentFrame % framesPerBar == 0 ? new SamplePlayHandle("misc/metronome02.ogg")
															 : new SamplePlayHandle("misc/metronome01.ogg");
		handle->setOffset(frame);
		Engine::audioEngine()->addPlayHandle(handle);
	}
}
} // namespace lmms
