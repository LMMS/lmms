/*
 * PlayHandle.h - base class PlayHandle - core of rendering engine
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

#ifndef PLAY_HANDLE_H
#define PLAY_HANDLE_H

#include <QtCore/QList>
#include <QtCore/QMutex>

#include "lmms_export.h"

#include "MemoryManager.h"

#include "ThreadableJob.h"
#include "lmms_basics.h"

class QThread;

class Track;
class AudioPort;

class LMMS_EXPORT PlayHandle : public ThreadableJob
{
public:
	enum Types
	{
		TypeNotePlayHandle = 0x01,
		TypeInstrumentPlayHandle = 0x02,
		TypeSamplePlayHandle = 0x04,
		TypePresetPreviewHandle = 0x08
	} ;
	typedef Types Type;

	enum
	{
		MaxNumber = 1024
	} ;

	PlayHandle( const Type type, f_cnt_t offset = 0 );

	PlayHandle & operator = ( PlayHandle & p )
	{
		m_type = p.m_type;
		m_offset = p.m_offset;
		m_affinity = p.m_affinity;
		m_usesBuffer = p.m_usesBuffer;
		m_audioPort = p.m_audioPort;
		return *this;
	}

	virtual ~PlayHandle();

	virtual bool affinityMatters() const
	{
		return false;
	}

	const QThread* affinity() const
	{
		return m_affinity;
	}

	Type type() const
	{
		return m_type;
	}

	// required for ThreadableJob
	void doProcessing() override;

	bool requiresProcessing() const override
	{
		return !isFinished();
	}

	void lock()
	{
		m_processingLock.lock();
	}
	void unlock()
	{
		m_processingLock.unlock();
	}
	bool tryLock()
	{
		return m_processingLock.tryLock();
	}
	virtual void play( sampleFrame* buffer ) = 0;
	virtual bool isFinished() const = 0;

	// returns the frameoffset at the start of the playhandle,
	// ie. how many empty frames should be inserted at the start of the first period
	f_cnt_t offset() const
	{
		return m_offset;
	}

	void setOffset( f_cnt_t _offset )
	{
		m_offset = _offset;
	}


	virtual bool isFromTrack( const Track * _track ) const = 0;

	bool usesBuffer() const
	{
		return m_usesBuffer;
	}
	
	void setUsesBuffer( const bool b )
	{
		m_usesBuffer = b;
	}
	
	AudioPort * audioPort()
	{
		return m_audioPort;
	}
	
	void setAudioPort( AudioPort * port )
	{
		m_audioPort = port;
	}
	
	void releaseBuffer();
	
	sampleFrame * buffer();

private:
	Type m_type;
	f_cnt_t m_offset;
	QThread* m_affinity;
	QMutex m_processingLock;
	sampleFrame* m_playHandleBuffer;
	bool m_bufferReleased;
	bool m_usesBuffer;
	AudioPort * m_audioPort;
} ;


typedef QList<PlayHandle *> PlayHandleList;
typedef QList<const PlayHandle *> ConstPlayHandleList;


#endif
