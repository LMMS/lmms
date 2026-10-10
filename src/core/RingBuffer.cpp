/*
 * RingBuffer.cpp - an effective and flexible implementation of a ringbuffer for LMMS
 *
 * Copyright (c) 2014 Vesa Kivimäki
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2026 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include "RingBuffer.h"
#include "AudioEngine.h"
#include "Engine.h"
#include "MixHelpers.h"

namespace lmms {

RingBuffer::RingBuffer(f_cnt_t frames, ch_cnt_t channels)
	: m_fpp(Engine::audioEngine()->framesPerPeriod())
	, m_samplerate(Engine::audioEngine()->outputSampleRate())
	, m_buf(frames + m_fpp, channels)
{
	m_position = 0;
}

RingBuffer::RingBuffer(float milliseconds, ch_cnt_t channels)
	: m_fpp(Engine::audioEngine()->framesPerPeriod())
	, m_samplerate(Engine::audioEngine()->outputSampleRate())
	, m_buf(msToFrames(milliseconds) + m_fpp, channels)
{
	m_position = 0;
	setSamplerateAware(true);
	//qDebug( "m_size %d, m_position %d", m_size, m_position );
}

void RingBuffer::reset()
{
	m_buf.silenceAllChannels();
	m_position = 0;
}

void RingBuffer::changeSize(f_cnt_t frames)
{
	m_buf = AudioBuffer{frames + m_fpp, m_buf.totalChannels()};
	m_position = 0;
}

void RingBuffer::changeSize(float milliseconds)
{
	assert(milliseconds > 0);
	changeSize(static_cast<f_cnt_t>(msToFrames(milliseconds)));
}

void RingBuffer::setSamplerateAware(bool b)
{
	if (b)
	{
		connect(Engine::audioEngine(), &AudioEngine::sampleRateChanged,
			this, &RingBuffer::updateSamplerate, Qt::UniqueConnection);
	}
	else
	{
		disconnect(Engine::audioEngine(), &AudioEngine::sampleRateChanged,
			this, &RingBuffer::updateSamplerate);
	}
}

void RingBuffer::advance()
{
	m_position = (m_position + m_fpp) % m_buf.frames();
}

void RingBuffer::movePosition(int amount)
{
	m_position = (m_position + amount) % m_buf.frames();
}

void RingBuffer::movePosition(float milliseconds)
{
	movePosition(msToFrames(milliseconds));
}

void RingBuffer::pop(PlanarBufferSpan<float> dst)
{
	const auto readPosition = m_position;
	const auto readAmount = m_fpp;

	if (readPosition + readAmount <= m_buf.frames())
	{
		auto src = PlanarBufferSpan{m_buf.allBuffers(), readPosition, readAmount};
		MixHelpers::copy(dst, src);
		MixHelpers::zero(src);
	}
	else
	{
		const auto first = m_buf.frames() - readPosition;
		const auto second = readAmount - first;

		auto srcFirst = PlanarBufferSpan{m_buf.allBuffers(), readPosition, first};
		MixHelpers::copy(dst, srcFirst);
		MixHelpers::zero(srcFirst);

		auto srcSecond = PlanarBufferSpan{m_buf.allBuffers(), 0, second};
		MixHelpers::copy(dst.subspan(first), srcSecond);
		MixHelpers::zero(srcSecond);
	}

	m_position = (readPosition + readAmount) % m_buf.frames();
}

void RingBuffer::read(PlanarBufferSpan<float> dst, f_cnt_t frames) const
{
	const auto pos = (m_position + frames) % m_buf.frames();
	const auto readAmount = dst.frames();

	if (pos + readAmount <=  m_buf.frames())
	{
		MixHelpers::copy(dst, PlanarBufferSpan{m_buf.allBuffers(), pos, readAmount});
	}
	else
	{
		const auto first = m_buf.frames() - pos;
		const auto second = readAmount - first;

		MixHelpers::copy(dst, PlanarBufferSpan{m_buf.allBuffers(), pos, first});
		MixHelpers::copy(dst.subspan(first), PlanarBufferSpan{m_buf.allBuffers(), 0, second});
	}
}

void RingBuffer::read(PlanarBufferSpan<float> dst, float milliseconds) const
{
	assert(milliseconds > 0);
	read(dst, static_cast<f_cnt_t>(msToFrames(milliseconds)));
}

void RingBuffer::updateSamplerate()
{
	float newSize = static_cast<float>((m_buf.frames() - m_fpp)
		* Engine::audioEngine()->outputSampleRate()) / m_samplerate;
	const auto newFrames = static_cast<f_cnt_t>(std::ceil(newSize)) + m_fpp;
	m_samplerate = Engine::audioEngine()->outputSampleRate();
	m_buf = AudioBuffer{newFrames, m_buf.totalChannels()};
	m_position = 0;
}

} // namespace lmms
