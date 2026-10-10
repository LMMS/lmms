/*
 * Instrument.cpp - base-class for all instrument-plugins (synths, samplers etc)
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "Instrument.h"

#include <cmath>
#include <numbers>

#include "DummyInstrument.h"
#include "InstrumentTrack.h"
#include "LmmsTypes.h"

namespace lmms
{


Instrument::Instrument(InstrumentTrack * _instrument_track,
			const Descriptor * _descriptor,
			const Descriptor::SubPluginFeatures::Key *key,
			Flags flags) :
	Plugin(_descriptor, nullptr/* _instrument_track*/, key),
	m_instrumentTrack( _instrument_track ),
	m_flags(flags)
{
}

void Instrument::play(std::optional<PlanarBufferView<float>>)
{
}




void Instrument::deleteNotePluginData( NotePlayHandle * )
{
}




f_cnt_t Instrument::beatLen( NotePlayHandle * ) const
{
	return( 0 );
}




Instrument *Instrument::instantiate(const QString &_plugin_name,
	InstrumentTrack *_instrument_track, const Descriptor::SubPluginFeatures::Key *key, bool keyFromDnd)
{
	if(keyFromDnd)
		Q_ASSERT(!key);
	// copy from above // TODO! common cleaner func
	Plugin * p = Plugin::instantiateWithKey(_plugin_name, _instrument_track, key, keyFromDnd);
	if(dynamic_cast<Instrument *>(p))
		return dynamic_cast<Instrument *>(p);
	delete p;
	return( new DummyInstrument( _instrument_track ) );
}




bool Instrument::isFromTrack( const Track * _track ) const
{
	return( m_instrumentTrack == _track );
}

// helper function for Instrument::applyFadeIn
static std::uint16_t countZeroCrossings(PlanarBufferSpan<const float> buf)
{
	// zero point crossing counts of all channels
	auto zeroCrossings = std::array<std::uint16_t, MaxChannelsPerAudioBuffer>{};
	const auto channels = buf.channels();
	const auto frames = buf.frames();
	assert(channels <= zeroCrossings.size());

	// maximum zero point crossing of all channels
	std::uint16_t maxZeroCrossings = 0;

	// determine the zero point crossing counts
	for (ch_cnt_t ch = 0; ch < channels; ++ch)
	{
		// We need to skip the first sample because it almost always
		// produces a zero crossing; it's not helpful while
		// determining the fade in length. Hence 1
		for (f_cnt_t f = 1; f < frames; ++f)
		{
			// we don't want to count [-1, 0, 1] as two crossings
			if ((buf[ch][f - 1] <= 0.0 && buf[ch][f] > 0.0)
				|| (buf[ch][f - 1] >= 0.0 && buf[ch][f] < 0.0))
			{
				++zeroCrossings[ch];
				if (zeroCrossings[ch] > maxZeroCrossings)
				{
					maxZeroCrossings = zeroCrossings[ch];
				}
			}
		}
	}

	return maxZeroCrossings;
}

// helper function for Instrument::applyFadeIn
static f_cnt_t getFadeInLength(float maxLength, f_cnt_t frames, int zeroCrossings)
{
	// calculate the length of the fade in
	// Length is inversely proportional to the max of zeroCrossings,
	// because for low frequencies, we need a longer fade in to
	// prevent clicking.
	return (f_cnt_t) (maxLength  / ((float) zeroCrossings / ((float) frames / 128.0f) + 1.0f));
}

void Instrument::applyFadeIn(PlanarBufferView<float> inOut, NotePlayHandle* nph)
{
	const static float MAX_FADE_IN_LENGTH = 85.0;
	const auto channels = inOut.channels();
	f_cnt_t total = nph->totalFramesPlayed();
	if (total == 0)
	{
		const f_cnt_t frames = nph->framesLeftForCurrentPeriod();
		const f_cnt_t offset = nph->offset();

		int maxZeroCrossings = countZeroCrossings({inOut, offset, frames});

		f_cnt_t length = getFadeInLength(MAX_FADE_IN_LENGTH, frames, maxZeroCrossings);
		nph->m_fadeInLength = length;

		// apply fade in
		length = length < frames ? length : frames;
		for (ch_cnt_t ch = 0; ch < channels; ++ch)
		{
			for (f_cnt_t f = 0; f < length; ++f)
			{
				inOut[ch][offset + f] *= 0.5 - 0.5 * std::cos(
					std::numbers::pi_v<float> * static_cast<float>(f) / static_cast<float>(nph->m_fadeInLength));
			}
		}
	}
	else if (total < nph->m_fadeInLength)
	{
		const f_cnt_t frames = nph->framesLeftForCurrentPeriod();

		int new_zc = countZeroCrossings(inOut.first(frames));
		f_cnt_t new_length = getFadeInLength(MAX_FADE_IN_LENGTH, frames, new_zc);

		for (ch_cnt_t ch = 0; ch < channels; ++ch)
		{
			for (f_cnt_t f = 0; f < frames; ++f)
			{
				float currentLength = nph->m_fadeInLength * (1.0f - (float) f / frames)
					+ new_length * ((float) f / frames);
				inOut[ch][f] *= 0.5 - 0.5 * std::cos(
					std::numbers::pi_v<float> * static_cast<float>(total + f) / currentLength);
				if (total + f >= currentLength)
				{
					nph->m_fadeInLength = currentLength;
					return;
				}
			}
		}
		nph->m_fadeInLength = new_length;
	}
}

void Instrument::applyRelease(PlanarBufferView<float> out, const NotePlayHandle* nph)
{
	const auto releaseFrames = desiredReleaseFrames();

	const auto endFrame = nph->framesLeft();
	const auto startFrame = endFrame - std::min(endFrame, releaseFrames);

	const auto frames = std::min(endFrame, out.frames());
	const auto channels = out.channels();
	for (ch_cnt_t ch = 0; ch < channels; ++ch)
	{
		float* const outPtr = out.bufferPtr(ch);
		for (auto frame = startFrame; frame < frames; ++frame)
		{
			const float fac = static_cast<float>(endFrame - frame) / releaseFrames;
			outPtr[frame] *= fac;
		}
	}
}

float Instrument::computeReleaseTimeMsByFrameCount(f_cnt_t frames) const
{
	return frames / getSampleRate() * 1000.;
}


sample_rate_t Instrument::getSampleRate() const
{
	return Engine::audioEngine()->outputSampleRate();
}


QString Instrument::fullDisplayName() const
{
	return instrumentTrack()->displayName();
}


} // namespace lmms
