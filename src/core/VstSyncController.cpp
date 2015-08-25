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

#include <QDebug>

#include "ConfigManager.h"
#include "Engine.h"
#include "lmmsconfig.h"
#include "Mixer.h"
#include "VstSyncController.h"
#include "RemotePlugin.h"


VstSyncController::VstSyncController() :
	m_syncData( NULL ),
	m_shmID( -1 ),
	m_shm( "/usr/bin/lmms" )
{
	if( ConfigManager::inst()->value( "ui", "syncvstplugins" ).toInt() )
	{
		connect( Engine::mixer(), SIGNAL( sampleRateChanged() ), this, SLOT( updateSampleRate() ) );

		if ( m_shm.create( sizeof( VstSyncData ) ) )
		{
			m_syncData = (VstSyncData*) m_shm.data();
		}
		else
		{
			qWarning() << QString( "Failed to allocate shared memory for VST sync: %1" ).arg( m_shm.errorString() );
		}
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
	m_syncData->m_bufferSize = Engine::mixer()->framesPerPeriod();
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
		if( m_shm.data() )
		{
			// detach shared memory, delete it:
			m_shm.detach();
		}
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
	m_syncData->m_bufferSize = Engine::mixer()->framesPerPeriod();

#ifdef VST_SNC_LATENCY
	m_syncData->m_latency = m_syncData->m_bufferSize * m_syncData->m_bpm / ( (float) m_syncData->m_sampleRate * 60 );
#endif
}



void VstSyncController::updateSampleRate()
{
	m_syncData->m_sampleRate = Engine::mixer()->processingSampleRate();

#ifdef VST_SNC_LATENCY
	m_syncData->m_latency = m_syncData->m_bufferSize * m_syncData->m_bpm / ( (float) m_syncData->m_sampleRate * 60 );
#endif
}





