/*
 * MixerWorkerThread.h - declaration of class MixerWorkerThread
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

#ifndef _MIXER_WORKER_THREAD_H
#define _MIXER_WORKER_THREAD_H

#include <QtCore/QAtomicPointer>
#include <QtCore/QThread>
#include <QtCore/QWaitCondition>

#include "mixer.h"


class MixerWorkerThread : public QThread
{
public:
	struct JobQueue
	{
#define JOB_QUEUE_SIZE 1024
		JobQueue() :
			items(),
			queueSize( 0 ),
			itemsDone( 0 )
		{
		}

		QAtomicPointer<ThreadableJob> items[JOB_QUEUE_SIZE];
		QAtomicInt queueSize;
		QAtomicInt itemsDone;
	} ;

	static JobQueue s_jobQueue;

	MixerWorkerThread( int _worker_num, mixer * _mixer );
	virtual ~MixerWorkerThread();

	virtual void quit();

	void processJobQueue();
	static void resetJobQueue();

	template<typename T>
	static void fillJobQueue( const T & _vec )
	{
		resetJobQueue();
		for( typename T::ConstIterator it = _vec.begin(); it != _vec.end(); ++it )
		{
			addJob( *it );
		}
	}

	static void addJob( ThreadableJob * _job );

	static void startJobs();
	static void waitForJobs();

	static void startAndWaitForJobs()
	{
		startJobs();
		waitForJobs();
	}


private:
	virtual void run();

	sampleFrame * m_workingBuf;
	int m_workerNum;
	volatile bool m_quit;
	mixer * m_mixer;
	QWaitCondition * m_queueReadyWaitCond;

} ;


#endif
