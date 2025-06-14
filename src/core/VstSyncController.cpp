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


VstSyncController::VstSyncController()
{
	try
	{
		m_syncData.create();
	}
	catch (const std::runtime_error& error)
	{
		qCritical() << "Failed to allocate shared memory for VST sync:" << error.what();
		return;
	}

	m_syncData->isPlaying = false;
	m_syncData->bufferSize = Engine::audioEngine()->framesPerPeriod();
	m_syncData->timeSigNumer = 4;
	m_syncData->timeSigDenom = 4;

	connect(Engine::audioEngine(), &AudioEngine::sampleRateChanged, this, &VstSyncController::updateSampleRate);
	updateSampleRate();
}



void VstSyncController::setAbsolutePosition(double ticks)
{
	if (!m_syncData) { return; }

#ifdef VST_SNC_LATENCY
	m_syncData->ppqPos = ((ticks + 0) / 48.0) - m_syncData->latency;
#else
	m_syncData->ppqPos = ((ticks + 0) / 48.0);
#endif
}



void VstSyncController::setPlaybackState(bool enabled)
{
	if (!m_syncData) { return; }

	m_syncData->isPlaying = enabled;
}



void VstSyncController::setTempo(int newTempo)
{
	if (!m_syncData) { return; }

	m_syncData->bpm = newTempo;

#ifdef VST_SNC_LATENCY
	m_syncData->latency = m_syncData->bufferSize * newTempo / (static_cast<float>(m_syncData->sampleRate) * 60);
#endif

}



void VstSyncController::setTimeSignature(int num, int denom)
{
	if (!m_syncData) { return; }

	m_syncData->timeSigNumer = num;
	m_syncData->timeSigDenom = denom;
}



void VstSyncController::startCycle(int startTick, int endTick)
{
	if (!m_syncData) { return; }

	m_syncData->isCycle = true;
	m_syncData->cycleStart = startTick / (float)48;
	m_syncData->cycleEnd = endTick / (float)48;
}



void VstSyncController::stopCycle()
{
	if (!m_syncData) { return; }

	m_syncData->isCycle = false;
}



void VstSyncController::setPlaybackJumped(bool jumped)
{
	if (!m_syncData) { return; }

	m_syncData->playbackJumped = jumped;
}



void VstSyncController::update()
{
	if (!m_syncData) { return; }

	m_syncData->bufferSize = Engine::audioEngine()->framesPerPeriod();

#ifdef VST_SNC_LATENCY
	m_syncData->latency = m_syncData->bufferSize * m_syncData->bpm / (static_cast<float>(m_syncData->sampleRate) * 60);
#endif
}



void VstSyncController::updateSampleRate()
{
	if (!m_syncData) { return; }

	m_syncData->sampleRate = Engine::audioEngine()->outputSampleRate();

#ifdef VST_SNC_LATENCY
	m_syncData->latency = m_syncData->bufferSize * m_syncData->bpm / (static_cast<float>(m_syncData->sampleRate) * 60);
#endif
}


} // namespace lmms
