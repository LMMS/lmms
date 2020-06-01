/*
 * MixerWorkerThread.h - declaration of class MixerWorkerThread
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

#ifndef MIXER_WORKER_THREAD_H
#define MIXER_WORKER_THREAD_H

#include <QtCore/QThread>

#include <atomic>

class QWaitCondition;
class Mixer;
class ThreadableJob;

class MixerWorkerThread : public QThread
{
	Q_OBJECT
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

#define JOB_QUEUE_SIZE 8192
		JobQueue() :
			m_items(),
			m_writeIndex( 0 ),
			m_itemsDone( 0 ),
			m_opMode( Static )
		{
			std::fill(m_items, m_items + JOB_QUEUE_SIZE, nullptr);
		}

		void reset( OperationMode _opMode );

		void addJob( ThreadableJob * _job );

		void run();
		void wait();

	private:
		std::atomic<ThreadableJob*> m_items[JOB_QUEUE_SIZE];
		std::atomic_int m_writeIndex;
		std::atomic_int m_itemsDone;
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
	void run() override;

	static JobQueue globalJobQueue;
	static QWaitCondition * queueReadyWaitCond;
	static QList<MixerWorkerThread *> workerThreads;

	volatile bool m_quit;

} ;


#endif
