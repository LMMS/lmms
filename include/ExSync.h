/*
 * ExSync.h - support for external synchronization
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_EXSYNC_H
#define LMMS_EXSYNC_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_JACK
#ifndef LMMS_HAVE_WEAKJACK
#include <jack/jack.h>
#else
#include <weak_libjack.h>
#endif

#include "lmms_basics.h"

namespace lmms
{
//! ExSync sending code provide all fields, but here used only @frame
struct SongExtendedPos
{
	bar_t bar; 	//!< the bar position, 0-based "Song.h"
	int beat; 		//!< the beat position inside the bar, 0-based "Song.h"
	int tick; 		//!< the remainder after bar and beat are removed , 0-based "Song.h"
	int barStartTick; //!< tick_t currentTick()  "Song.h"
	int beatsPerBar; //!<  getTimeSigModel().numeratorModel().value(); "MeterModel.h"
	int beatType; 	//!<  getTimeSigModel().denominatorModel().value(); "MeterModel.h"
	tick_t ticksPerBeat; //!< ticksPerBeat(...) "TimePos.h"
	bpm_t tempo; //!< beats per minute getTempo(); "Song.h"
	f_cnt_t frame; //!< currentFrame(); "Song.h"
};

//! Functions, MUST be provided by ExSync driver
struct ExSyncHandler
{
	//! true if synchronisation is available (driver is on)
	bool (* availableNow)(); 
	//! driver MUST send start/pause message if @playing is true/false 
	void (* sendPlay)(bool playing); 
	//! driver MUST send new position message
	void (* sendPosition)(const SongExtendedPos *pos);
	//! driver MUST start/stop remote LMMS controling @cb !nullptr/nullptr
	void (* setSlave)(struct ExSyncCallbacks *cb); 
};


//! provides jackd ExSync driver API using frame based synchronization
struct ExSyncHandler * exSyncGetHandler();

void exSyncStopped();

void syncJackd(jack_client_t* client);

void exSyncSendPosition();
const char * exSyncToggleMode();
const char * exSyncGetModeString();
bool exSyncToggle();
bool exSyncReact();
bool exSyncAvailable();
bool exSyncMasterAndSync();

}

#define 	LMMS_HAVE_EXSYNC

#else

// Some empty functions/macroses here

namespace lmms 
{

inline void exSyncStopped(){}
inline void exSyncSendPosition(){}

}

#endif



#endif
