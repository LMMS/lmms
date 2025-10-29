/*
 * VstSyncData.h - type declarations needed for VST to lmms host sync
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

#ifndef LMMS_VST_SYNC_DATA_H
#define LMMS_VST_SYNC_DATA_H

namespace lmms
{


// VST sync frequency (in ms), how often will be VST plugin synced
// keep it power of two if possible (not used by now)
//#define VST_SNC_TIMER 1

// When defined, latency should be subtracted from song PPQ position
//#define VST_SNC_LATENCY



struct VstSyncData
{
	double ppqPos;
	int timeSigNumer;
	int timeSigDenom;
	bool isPlaying;
	bool isCycle;
	float cycleStart;
	float cycleEnd;
	bool playbackJumped;
	int bufferSize;
	int sampleRate;
	int bpm;

#ifdef VST_SNC_LATENCY
	float latency;
#endif
} ;


} // namespace lmms

#endif // LMMS_VST_SYNC_DATA_H
