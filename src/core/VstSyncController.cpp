/*
 * VstSyncController.cpp - manage synchronization between LMMS and VST plugins
 *
 * Copyright (c) 2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2013 Mike Choi <rdavidian71/at/gmail/dot/com>
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

#include <QtCore/QDebug>

#include "config_mgr.h"
#include "engine.h"
#include "lmmsconfig.h"
#include "Mixer.h"
#include "VstSyncController.h"

#ifdef LMMS_BUILD_WIN32
#ifndef USE_QT_SHMEM
#define USE_QT_SHMEM
#endif
#endif

#ifndef USE_QT_SHMEM
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif


VstSyncController::VstSyncController() :
	m_syncData( NULL ),
	m_shmID( -1 ),
	m_shm( "/usr/bin/lmms" )
{
	if( configManager::inst()->value( "ui", "syncvstplugins" ).toInt() )
	{
		connect( engine::mixer(), SIGNAL( sampleRateChanged() ), this, SLOT( updateSampleRate() ) );

#ifdef USE_QT_SHMEM
		if ( m_shm.create( sizeof( VstSyncData ) ) )
		{
			m_syncData = (VstSyncData*) m_shm.data();
		}
		else
		{
			qWarning() << QString( "Failed to allocate shared memory for VST sync: %1" ).arg( m_shm.errorString() );
		}
#else
		key_t key; // make the key:
		if( ( key = ftok( VST_SNC_SHM_KEY_FILE, 'R' ) ) == -1 )
		{
				qWarning( "VstSyncController: ftok() failed" );
		}
		else
		{	// connect to shared memory segment
			if( ( m_shmID = shmget( key, sizeof( VstSyncData ), 0644 | IPC_CREAT ) ) == -1 )
			{
				qWarning( "VstSyncController: shmget() failed" );
			}
			else
			{		// attach segment
				m_syncData = (VstSyncData *)shmat( m_shmID, 0, 0 );
				if( m_syncData == (VstSyncData *)( -1 ) )
				{
					qWarning( "VstSyncController: shmat() failed" );
				}
			}
		}
#endif
	}
	else
	{
		qWarning( "VST sync support disabled in your configuration" );
	}

	if( m_syncData == NULL )
	{
		m_syncData = new VstSyncData;
		m_syncData->hasSHM = false;
	}
	else
	{
		m_syncData->hasSHM = true;
	}

	m_syncData->isPlaying = false;
	m_syncData->m_bufferSize = engine::mixer()->framesPerPeriod();
	m_syncData->timeSigNumer = 4;
	m_syncData->timeSigDenom = 4;

	updateSampleRate();
}



VstSyncController::~VstSyncController()
{
	if( m_syncData->hasSHM == false )
	{
		delete m_syncData;
	}
	else
	{
#ifdef USE_QT_SHMEM
		if( m_shm.data() )
		{
			// detach shared memory, delete it:
			m_shm.detach();
		}
#else
		if( shmdt( m_syncData ) != -1 )
		{
			shmctl( m_shmID, IPC_RMID, NULL );
		}
		else
		{
			qWarning( "VstSyncController: shmdt() failed" );
		}
#endif
	}
}



void VstSyncController::setAbsolutePosition( int ticks )
{
#ifdef VST_SNC_LATENCY
	m_syncData->ppqPos = ( ( ticks + 0 ) / (float)48 ) - m_syncData->m_latency;
#else
	m_syncData->ppqPos = ( ( ticks + 0 ) / (float)48 );
#endif
}



void VstSyncController::setTempo( int newTempo )
{
	m_syncData->m_bpm = newTempo;

#ifdef VST_SNC_LATENCY
	m_syncData->m_latency = m_syncData->m_bufferSize * newTempo / ( (float) m_syncData->m_sampleRate * 60 );
#endif

}



void VstSyncController::startCycle( int startTick, int endTick )
{
	m_syncData->isCycle = true;
	m_syncData->cycleStart = startTick / (float)48;
	m_syncData->cycleEnd = endTick / (float)48;
}



void VstSyncController::update()
{
	m_syncData->m_bufferSize = engine::mixer()->framesPerPeriod();

#ifdef VST_SNC_LATENCY
	m_syncData->m_latency = m_syncData->m_bufferSize * m_syncData->m_bpm / ( (float) m_syncData->m_sampleRate * 60 );
#endif
}



void VstSyncController::updateSampleRate()
{
	m_syncData->m_sampleRate = engine::mixer()->processingSampleRate();

#ifdef VST_SNC_LATENCY
	m_syncData->m_latency = m_syncData->m_bufferSize * m_syncData->m_bpm / ( (float) m_syncData->m_sampleRate * 60 );
#endif
}



#include "moc_VstSyncController.cxx"

