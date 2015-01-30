/*
 * VstSyncController.h - type declarations needed for VST to lmms host sync
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

#ifndef VST_SYNC_CONTROLLER_H
#define VST_SYNC_CONTROLLER_H

#include <QtCore/QObject>
#include <QtCore/QSharedMemory>

#include "VstSyncData.h"


class VstSyncController : public QObject
{
	Q_OBJECT
public:
	VstSyncController();
	~VstSyncController();

	void setAbsolutePosition( int ticks );

	void setPlaybackState( bool enabled )
	{
		m_syncData->isPlaying = enabled;
	}

	void setTempo( int newTempo );

	void setTimeSignature( int num, int denom )
	{
		m_syncData->timeSigNumer = num;
		m_syncData->timeSigDenom = denom;
	}

	void startCycle( int startTick, int endTick );

	void stopCycle()
	{
		m_syncData->isCycle = false;
	}

	void update();


private slots:
	void updateSampleRate();


private:
	struct VstSyncData
	{
		bool isPlaying;
		float ppqPos;
		int timeSigNumer;
		int timeSigDenom;
		bool isCycle;
		bool hasSHM;
		float cycleStart;
		float cycleEnd;
		int m_bufferSize;
		int m_sampleRate;
		int m_bpm;

#ifdef VST_SNC_LATENCY
		float m_latency;
#endif
	} ;

	VstSyncData* m_syncData;

	int m_shmID;

	QSharedMemory m_shm;

};

#endif
