/*
 * MixerWorkerThread.cpp - implementation of MixerWorkerThread
 *
 * Copyright (c) 2009-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "MixerWorkerThread.h"

#include <QDebug>
#include <QMutex>
#include <QWaitCondition>

#include "denormals.h"
#include "ThreadableJob.h"
#include "Mixer.h"

#if defined(LMMS_HOST_X86) || defined(LMMS_HOST_X86_64)
#include <xmmintrin.h>
#endif

MixerWorkerThread::JobQueue MixerWorkerThread::globalJobQueue;
QWaitCondition * MixerWorkerThread::queueReadyWaitCond = NULL;
QList<MixerWorkerThread *> MixerWorkerThread::workerThreads;

// implementation of internal JobQueue
void MixerWorkerThread::JobQueue::reset( OperationMode _opMode )
{
	m_writeIndex = 0;
	m_itemsDone = 0;
	m_opMode = _opMode;
}




void MixerWorkerThread::JobQueue::addJob( ThreadableJob * _job )
{
	if( _job->requiresProcessing() )
	{
		// update job state
		_job->queue();
		// actually queue the job via atomic operations
		auto index = m_writeIndex++;
		if (index < JOB_QUEUE_SIZE) {
			m_items[index] = _job;
		} else {
			qWarning() << "Job queue is full!";
			++m_itemsDone;
		}
	}
}



void MixerWorkerThread::JobQueue::run()
{
	bool processedJob = true;
	while (processedJob && m_itemsDone < m_writeIndex)
	{
		processedJob = false;
		for( int i = 0; i < m_writeIndex && i < JOB_QUEUE_SIZE; ++i )
		{
			ThreadableJob * job = m_items[i].exchange(nullptr);
			if( job )
			{
				job->process();
				processedJob = true;
				++m_itemsDone;
			}
		}
		// always exit loop if we're not in dynamic mode
		processedJob = processedJob && ( m_opMode == Dynamic );
	}
}




void MixerWorkerThread::JobQueue::wait()
{
	while (m_itemsDone < m_writeIndex)
	{
#if defined(LMMS_HOST_X86) || defined(LMMS_HOST_X86_64)
		_mm_pause();
#endif
	}
}





// implementation of worker threads

MixerWorkerThread::MixerWorkerThread( Mixer* mixer ) :
	QThread( mixer ),
	m_quit( false )
{
	// initialize global static data
	if( queueReadyWaitCond == NULL )
	{
		queueReadyWaitCond = new QWaitCondition;
	}

	// keep track of all instantiated worker threads - this is used for
	// processing the last worker thread "inline", see comments in
	// MixerWorkerThread::startAndWaitForJobs() for details
	workerThreads << this;

	resetJobQueue();
}




MixerWorkerThread::~MixerWorkerThread()
{
	workerThreads.removeAll( this );
}




void MixerWorkerThread::quit()
{
	m_quit = true;
	resetJobQueue();
}




void MixerWorkerThread::startAndWaitForJobs()
{
	queueReadyWaitCond->wakeAll();
	// The last worker-thread is never started. Instead it's processed "inline"
	// i.e. within the global Mixer thread. This way we can reduce latencies
	// that otherwise would be caused by synchronizing with another thread.
	globalJobQueue.run();
	globalJobQueue.wait();
}




void MixerWorkerThread::run()
{
	MemoryManager::ThreadGuard mmThreadGuard; Q_UNUSED(mmThreadGuard);
	disable_denormals();

	QMutex m;
	while( m_quit == false )
	{
		m.lock();
		queueReadyWaitCond->wait( &m );
		globalJobQueue.run();
		m.unlock();
	}
}


