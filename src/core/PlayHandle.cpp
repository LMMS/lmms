/*
 * PlayHandle.cpp - base class PlayHandle - core of rendering engine
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 
#include "PlayHandle.h"
#include "AudioEngine.h"
#include "BufferManager.h"
#include "Engine.h"

#include <QThread>


namespace lmms
{

PlayHandle::PlayHandle(const Type type, f_cnt_t offset) :
		m_type(type),
		m_offset(offset),
		m_affinity(QThread::currentThread()),
		m_playHandleBuffer(BufferManager::acquire()),
		m_bufferReleased(true),
		m_usesBuffer(true)
{
}


PlayHandle::~PlayHandle()
{
	BufferManager::release(m_playHandleBuffer);
}


void PlayHandle::doProcessing()
{
	if( m_usesBuffer )
	{
		m_bufferReleased = false;
		zeroSampleFrames(m_playHandleBuffer, Engine::audioEngine()->framesPerPeriod());
		play( buffer() );
	}
	else
	{
		play( nullptr );
	}
}


void PlayHandle::releaseBuffer()
{
	m_bufferReleased = true;
}

SampleFrame* PlayHandle::buffer()
{
	return m_bufferReleased ? nullptr : m_playHandleBuffer;
};

} // namespace lmms