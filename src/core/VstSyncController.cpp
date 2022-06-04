/*
 * VstSyncController.cpp - manage synchronization between LMMS and VST plugins
 *
 * Copyright (c) 2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2013 Mike Choi <rdavidian71/at/gmail/dot/com>
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

#include "VstSyncController.h"

#include <stdexcept>

#include <QDebug>

#include "AudioEngine.h"
#include "ConfigManager.h"
#include "Engine.h"
#include "RemotePlugin.h"


namespace lmms
{


VstSyncController::VstSyncController() :
	m_syncData( nullptr )
{
	if( ConfigManager::inst()->value( "ui", "syncvstplugins" ).toInt() )
	{
		connect( Engine::audioEngine(), SIGNAL( sampleRateChanged() ), this, SLOT( updateSampleRate() ) );

		try
		{
			m_shm.create("usr_bin_lmms");
			m_syncData = m_shm.get();
		}
		catch (const std::runtime_error& error)
		{
			qWarning() << "Failed to allocate shared memory for VST sync:" << error.what();
		}
	}
	else
	{
		qWarning( "VST sync support disabled in your configuration" );
	}

	if( m_syncData == nullptr )
	{
		m_syncData = new VstSyncData;
		m_syncData->hasSHM = false;
	}
	else
	{
		m_syncData->hasSHM = true;
	}

	m_syncData->isPlaying = false;
	m_syncData->m_bufferSize = Engine::audioEngine()->framesPerPeriod();
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
}



void VstSyncController::setAbsolutePosition( double ticks )
{
#ifdef VST_SNC_LATENCY
	m_syncData->ppqPos = ( ( ticks + 0 ) / 48.0 ) - m_syncData->m_latency;
#else
	m_syncData->ppqPos = ( ( ticks + 0 ) / 48.0 );
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
	m_syncData->m_bufferSize = Engine::audioEngine()->framesPerPeriod();

#ifdef VST_SNC_LATENCY
	m_syncData->m_latency = m_syncData->m_bufferSize * m_syncData->m_bpm / ( (float) m_syncData->m_sampleRate * 60 );
#endif
}



void VstSyncController::updateSampleRate()
{
	m_syncData->m_sampleRate = Engine::audioEngine()->processingSampleRate();

#ifdef VST_SNC_LATENCY
	m_syncData->m_latency = m_syncData->m_bufferSize * m_syncData->m_bpm / ( (float) m_syncData->m_sampleRate * 60 );
#endif
}


} // namespace lmms
