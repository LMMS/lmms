/*
 * ThreadableJob.h - declaration of class ThreadableJob
 *
 * Copyright (c) 2009-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _THREADABLE_JOB_H
#define _THREADABLE_JOB_H

#include <QtCore/QAtomicInt>

#include "lmms_basics.h"


class ThreadableJob
{
public:

	enum ProcessingState
	{
		Unstarted,
		Queued,
		InProgress,
		Done
	};

	ThreadableJob() :
		m_state( ThreadableJob::Unstarted )
	{
	}

	inline ProcessingState state() const
	{
		return static_cast<ProcessingState>( (int) m_state );
	}

	inline void reset()
	{
		m_state = Unstarted;
	}

	inline void queue()
	{
		m_state = Queued;
	}
	
	inline void done()
	{
		m_state = Done;
	}

	void process( sampleFrame* workingBuffer = NULL )
	{
		if( m_state.testAndSetOrdered( Queued, InProgress ) )
		{
			doProcessing( workingBuffer );
			m_state = Done;
		}
	}

	virtual bool requiresProcessing() const = 0;


protected:
	virtual void doProcessing( sampleFrame* workingBuffer) = 0;

	QAtomicInt m_state;

} ;

#endif
