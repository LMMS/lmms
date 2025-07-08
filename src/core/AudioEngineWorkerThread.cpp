/*
 * AudioEngineWorkerThread.cpp - implementation of AudioEngineWorkerThread
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

#include "AudioEngineWorkerThread.h"

#include <QDebug>
#include <QMutex>
#include <QWaitCondition>

#include "denormals.h"
#include "AudioEngine.h"
#include "ThreadableJob.h"

#if __SSE__
#include <xmmintrin.h>
#endif

namespace lmms
{

AudioEngineWorkerThread::JobQueue AudioEngineWorkerThread::globalJobQueue;
QWaitCondition * AudioEngineWorkerThread::queueReadyWaitCond = nullptr;
QList<AudioEngineWorkerThread *> AudioEngineWorkerThread::workerThreads;

// implementation of internal JobQueue
void AudioEngineWorkerThread::JobQueue::reset( OperationMode _opMode )
{
	m_writeIndex = 0;
	m_itemsDone = 0;
	m_opMode = _opMode;
}




void AudioEngineWorkerThread::JobQueue::addJob( ThreadableJob * _job )
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



void AudioEngineWorkerThread::JobQueue::run()
{
	bool processedJob = true;
	while (processedJob && m_itemsDone < m_writeIndex)
	{
		processedJob = false;
		for (auto i = std::size_t{0}; i < m_writeIndex && i < JOB_QUEUE_SIZE; ++i)
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
		processedJob = processedJob && ( m_opMode == OperationMode::Dynamic );
	}
}




void AudioEngineWorkerThread::JobQueue::wait()
{
	while (m_itemsDone < m_writeIndex)
	{
#ifdef __SSE__
		_mm_pause();
#endif
	}
}





// implementation of worker threads

AudioEngineWorkerThread::AudioEngineWorkerThread( AudioEngine* audioEngine ) :
	QThread( audioEngine ),
	m_quit( false )
{
	// initialize global static data
	if( queueReadyWaitCond == nullptr )
	{
		queueReadyWaitCond = new QWaitCondition;
	}

	// keep track of all instantiated worker threads - this is used for
	// processing the last worker thread "inline", see comments in
	// AudioEngineWorkerThread::startAndWaitForJobs() for details
	workerThreads << this;

	resetJobQueue();
}




AudioEngineWorkerThread::~AudioEngineWorkerThread()
{
	workerThreads.removeAll( this );
}




void AudioEngineWorkerThread::quit()
{
	m_quit = true;
	resetJobQueue();
}




void AudioEngineWorkerThread::startAndWaitForJobs()
{
	queueReadyWaitCond->wakeAll();
	// The last worker-thread is never started. Instead it's processed "inline"
	// i.e. within the global AudioEngine thread. This way we can reduce latencies
	// that otherwise would be caused by synchronizing with another thread.
	globalJobQueue.run();
	globalJobQueue.wait();
}




void AudioEngineWorkerThread::run()
{
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

} // namespace lmms
