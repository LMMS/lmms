/*
 * MixerWorkerThread.h - declaration of class MixerWorkerThread
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

#ifndef MIXER_WORKER_THREAD_H
#define MIXER_WORKER_THREAD_H

#include <QtCore/QAtomicPointer>
#include <QtCore/QThread>

#include "ThreadableJob.h"
#include "Mixer.h"

#ifdef __SSE__
#include <xmmintrin.h>
#endif
#ifdef __SSE3__
#include <pmmintrin.h>
#endif

class MixerWorkerThread : public QThread
{
public:
	// internal representation of the job queue - all functions are thread-safe
	class JobQueue
	{
	public:
		enum OperationMode
		{
			Static,	// no jobs added while processing queue
			Dynamic	// jobs can be added while processing queue
		} ;

		JobQueue() :
			m_items(),
			m_queueSize( 0 ),
			m_itemsDone( 0 ),
			m_opMode( Static )
		{
		}

		void reset( OperationMode _opMode );

		void addJob( ThreadableJob * _job );

		void run( sampleFrame * _buffer );
		void wait();

	private:
#define JOB_QUEUE_SIZE 1024
		QAtomicPointer<ThreadableJob> m_items[JOB_QUEUE_SIZE];
		QAtomicInt m_queueSize;
		QAtomicInt m_itemsDone;
		OperationMode m_opMode;

	} ;


	MixerWorkerThread( Mixer* mixer );
	virtual ~MixerWorkerThread();

	virtual void quit();

	static void resetJobQueue( JobQueue::OperationMode _opMode =
													JobQueue::Static )
	{
		globalJobQueue.reset( _opMode );
	}

	static void addJob( ThreadableJob * _job )
	{
		globalJobQueue.addJob( _job );
	}

	// a convenient helper function allowing to pass a container with pointers
	// to ThreadableJob objects
	template<typename T>
	static void fillJobQueue( const T & _vec,
							JobQueue::OperationMode _opMode = JobQueue::Static )
	{
		resetJobQueue( _opMode );
		for( typename T::ConstIterator it = _vec.begin(); it != _vec.end(); ++it )
		{
			addJob( *it );
		}
	}

	static void startAndWaitForJobs();


private:
	virtual void run();

	static JobQueue globalJobQueue;
	static QWaitCondition * queueReadyWaitCond;
	static QList<MixerWorkerThread *> workerThreads;

	sampleFrame * m_workingBuf;
	volatile bool m_quit;

} ;


#endif
