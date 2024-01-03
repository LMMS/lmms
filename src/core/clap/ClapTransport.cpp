/*
 * ClapTransport.cpp - CLAP transport events
 *
 * Copyright (c) 2024 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include "ClapTransport.h"

#ifdef LMMS_HAVE_CLAP

#include "Engine.h"
#include "Song.h"

namespace lmms
{

namespace
{

template<std::size_t mask, typename T>
inline void setBit(T& number, bool value) noexcept
{
	static_assert(mask && !(mask & (mask - 1)), "mask must have single bit set");
	constexpr auto bitPos = [=] {
		// constexpr log2
		unsigned pos = 0;
		auto x = mask;
		while (x != 0) {
			x >>= 1;
			++pos;
		}
		return pos - 1;
	}();
	static_assert(bitPos < sizeof(T) * 8, "mask is too big for T");
	number = (number & ~mask) | (static_cast<T>(value) << bitPos);
}

} // namespace

clap_event_transport ClapTransport::s_transport = {};

void ClapTransport::update()
{
	s_transport = {};
	s_transport.header.size = sizeof(clap_event_transport);
	s_transport.header.type = CLAP_EVENT_TRANSPORT;

	const Song* song = Engine::getSong();
	if (!song) { return; }

	s_transport.flags = 0;

	setPlaying(song->isPlaying());
	setRecording(song->isRecording());
	//setLooping(song->isLooping());

	// TODO: Pre-roll, isLooping, tempo_inc

	setBeatPosition();
	setTimePosition(song->getMilliseconds());

	setTempo(song->getTempo());
	setTimeSignature(song->getTimeSigModel().getNumerator(), song->getTimeSigModel().getDenominator());
}

void ClapTransport::setPlaying(bool isPlaying)
{
	setBit<CLAP_TRANSPORT_IS_PLAYING>(s_transport.flags, isPlaying);
}

void ClapTransport::setRecording(bool isRecording)
{
	setBit<CLAP_TRANSPORT_IS_RECORDING>(s_transport.flags, isRecording);
}

void ClapTransport::setLooping(bool isLooping)
{
	setBit<CLAP_TRANSPORT_IS_LOOP_ACTIVE>(s_transport.flags, isLooping);
	// TODO: loop_start_* and loop_end_*
}

void ClapTransport::setBeatPosition()
{
	const Song* song = Engine::getSong();
	if (!song) { return; }

	s_transport.flags |= static_cast<std::uint32_t>(CLAP_TRANSPORT_HAS_BEATS_TIMELINE);

	// Logic taken from TimeDisplayWidget.cpp
	// NOTE: If the time signature changes during the song, this info may be misleading
	const auto tick = song->getPlayPos().getTicks();
	const auto ticksPerBar = song->ticksPerBar();
	const auto timeSigNum = song->getTimeSigModel().getNumerator();
	const auto barsElapsed = static_cast<int>(tick / ticksPerBar); // zero-based

	s_transport.bar_number = barsElapsed;

	const auto barsElapsedInBeats = (barsElapsed * timeSigNum) + 1; // one-based

	s_transport.bar_start = barsElapsedInBeats << 31; // same as multiplication by CLAP_BEATTIME_FACTOR

	const auto beatWithinBar = 1.0 * (tick % ticksPerBar) / (ticksPerBar / timeSigNum); // zero-based
	const auto beatsElapsed = barsElapsedInBeats + beatWithinBar; // one-based

	s_transport.song_pos_beats = std::lround(CLAP_BEATTIME_FACTOR * beatsElapsed);
}

void ClapTransport::setTimePosition(int elapsedMilliseconds)
{
	s_transport.flags |= static_cast<std::uint32_t>(CLAP_TRANSPORT_HAS_SECONDS_TIMELINE);
	s_transport.song_pos_seconds = std::lround(CLAP_SECTIME_FACTOR * (elapsedMilliseconds / 1000.0));
}

void ClapTransport::setTempo(bpm_t tempo)
{
	s_transport.flags |= static_cast<std::uint32_t>(CLAP_TRANSPORT_HAS_TEMPO);
	s_transport.tempo  = static_cast<double>(tempo);
	// TODO: tempo_inc
}

void ClapTransport::setTimeSignature(int num, int denom)
{
	s_transport.flags |= static_cast<std::uint32_t>(CLAP_TRANSPORT_HAS_TIME_SIGNATURE);
	s_transport.tsig_num = static_cast<std::uint16_t>(num);
	s_transport.tsig_denom = static_cast<std::uint16_t>(denom);
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
