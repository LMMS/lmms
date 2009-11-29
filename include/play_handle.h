/*
 * play_handle.h - base-class playHandle - core of rendering engine
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _PLAY_HANDLE_H
#define _PLAY_HANDLE_H

#include <QtCore/QThread>

#include "ThreadableJob.h"
#include "Mixer.h"

class track;


class playHandle : public ThreadableJob
{
public:
	enum Types
	{
		NotePlayHandle,
		InstrumentPlayHandle,
		SamplePlayHandle,
		NumPlayHandleTypes
	} ;
	typedef Types Type;

	playHandle( const Type _type, f_cnt_t _offset = 0 ) :
		m_type( _type ),
		m_offset( _offset ),
		m_affinity( QThread::currentThread() )
	{
	}

	virtual inline ~playHandle()
	{
	}

	virtual inline bool affinityMatters() const
	{
		return false;
	}

	const QThread * affinity() const
	{
		return m_affinity;
	}

	inline Type type() const
	{
		return m_type;
	}

	// required for ThreadableJob
	virtual void doProcessing( sampleFrame * _working_buffer )
	{
		play( _working_buffer );
	}

	virtual bool requiresProcessing() const
	{
		return !done();
	}

	virtual void play( sampleFrame * _working_buffer ) = 0;
	virtual bool done() const = 0;

	// returns how many frames this play-handle is aligned ahead, i.e.
	// at which position it is inserted in the according buffer
	inline f_cnt_t offset() const
	{
		return m_offset;
	}

	inline void setOffset( f_cnt_t _offset )
	{
		m_offset = _offset;
	}


	virtual bool isFromTrack( const track * _track ) const = 0;


private:
	Type m_type;
	f_cnt_t m_offset;
	const QThread * m_affinity;

} ;


typedef QList<playHandle *> PlayHandleList;
typedef QList<const playHandle *> ConstPlayHandleList;


#endif
