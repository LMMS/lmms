/*
 * PlayHandle.h - base class PlayHandle - core of rendering engine
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#include <QtCore/QThread>
#include <QtCore/QVector>

#include "ThreadableJob.h"
#include "lmms_basics.h"

class track;


class PlayHandle : public ThreadableJob
{
public:
	enum Types
	{
		TypeNotePlayHandle,
		TypeInstrumentPlayHandle,
		TypeSamplePlayHandle,
		TypePresetPreviewHandle,
		TypeCount
	} ;
	typedef Types Type;

	PlayHandle( const Type type, f_cnt_t offset = 0 ) :
		m_type( type ),
		m_offset( offset ),
		m_affinity( QThread::currentThread() )
	{
	}

	virtual ~PlayHandle()
	{
	}

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
	virtual void doProcessing( sampleFrame* buffer )
	{
		play( buffer );
	}

	virtual bool requiresProcessing() const
	{
		return !isFinished();
	}


	virtual void play( sampleFrame* buffer ) = 0;
	virtual bool isFinished( void ) const = 0;

	// returns how many frames this play-handle is aligned ahead, i.e.
	// at which position it is inserted in the according buffer
	f_cnt_t offset() const
	{
		return m_offset;
	}

	void setOffset( f_cnt_t _offset )
	{
		m_offset = _offset;
	}


	virtual bool isFromTrack( const track * _track ) const = 0;


private:
	Type m_type;
	f_cnt_t m_offset;
	const QThread* m_affinity;

} ;


typedef QList<PlayHandle *> PlayHandleList;
typedef QList<const PlayHandle *> ConstPlayHandleList;


#endif
