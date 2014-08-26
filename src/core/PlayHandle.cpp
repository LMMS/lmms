/*
 * PlayHandle.cpp - base class PlayHandle - core of rendering engine
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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
#include "BufferManager.h"


PlayHandle::PlayHandle( const Type type, f_cnt_t offset ) :
		m_type( type ),
		m_offset( offset ),
		m_affinity( QThread::currentThread() ),
		m_playHandleBuffer( NULL ),
		m_usesBuffer( true )
{
}


PlayHandle::~PlayHandle()
{
}


void PlayHandle::doProcessing()
{
	if( m_usesBuffer )
	{
		if( ! m_playHandleBuffer ) m_playHandleBuffer = BufferManager::acquire();
		play( m_playHandleBuffer );
	}
	else
	{
		play( m_playHandleBuffer );
	}
}


void PlayHandle::releaseBuffer()
{
	BufferManager::release( m_playHandleBuffer );
	m_playHandleBuffer = NULL;
}
