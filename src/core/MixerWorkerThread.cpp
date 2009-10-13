/*
 * MixerWorkerThread.cpp - implementation of MixerWorkerThread
 *
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "MixerWorkerThread.h"
#include "Cpu.h"
#include "engine.h"
#include "mixer.h"


MixerWorkerThread::JobQueue MixerWorkerThread::s_jobQueue;


MixerWorkerThread::MixerWorkerThread( int _worker_num, mixer * _mixer ) :
	QThread( _mixer ),
	m_workingBuf( CPU::allocFrames( _mixer->framesPerPeriod() ) ),
	m_workerNum( _worker_num ),
	m_quit( false ),
	m_mixer( _mixer ),
	m_queueReadyWaitCond( &m_mixer->m_queueReadyWaitCond )
{
	resetJobQueue();
}




MixerWorkerThread::~MixerWorkerThread()
{
	CPU::freeFrames( m_workingBuf );
}




void MixerWorkerThread::quit()
{
	m_quit = true;
}




void MixerWorkerThread::processJobQueue()
{
	while( s_jobQueue.itemsDone != s_jobQueue.queueSize )
	{
		for( int i = 0; i < s_jobQueue.queueSize; ++i )
		{
			ThreadableJob * job =
							s_jobQueue.items[i].fetchAndStoreOrdered( NULL );
			if( job )
			{
				job->process( m_workingBuf );
				s_jobQueue.itemsDone.fetchAndAddOrdered( 1 );
			}
		}
	}
}




void MixerWorkerThread::resetJobQueue()
{
	s_jobQueue.queueSize = 0;
	s_jobQueue.itemsDone = 0;
}




void MixerWorkerThread::addJob( ThreadableJob * _job )
{
	if( _job->requiresProcessing() )
	{
		// update job state
		_job->queue();
		// actually queue the job via atomic operations
		s_jobQueue.items[s_jobQueue.queueSize.fetchAndAddOrdered(1)] = _job;
	}
}




void MixerWorkerThread::startJobs()
{
	// TODO: this is dirty!
	engine::getMixer()->m_queueReadyWaitCond.wakeAll();
}




void MixerWorkerThread::waitForJobs()
{
	// TODO: this is dirty!
	mixer * m = engine::getMixer();
	m->m_workers[m->m_numWorkers]->processJobQueue();
}




void MixerWorkerThread::run()
{
	QMutex m;
	while( m_quit == false )
	{
		m.lock();
		m_queueReadyWaitCond->wait( &m );
		processJobQueue();
		m.unlock();
	}
}


