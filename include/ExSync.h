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

/**
After implementing more ExSync devices 
MUST be placed in own header  "ExSync.h"
*/
// BEGIN ExSync ExSync.h context
//! Is included here but should be included in "ExSync.h"
// #include "lmms_basics.h"


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

/**
 Functions, provided by Song.cpp, to control LMMS position/playing
 LMMS react only in if ExSync is on (button is green)
*/
struct ExSyncCallbacks
{
	//! @playing [true : to start; false : to pause] 
	void (* mode)(bool playing); 
	//! change position to @frames;
	void (* position)(uint32_t frames);
	//! to calculate frames from time (not used here - jack is working in frames)
	sample_rate_t (* processingSampleRate)(); //!< provided from Mixer.cpp
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
// struct ExSyncCallbacks *getExSync();
// END ExSync ExSync.h context

//! provides jackd ExSync driver API using frame based synchronization
struct ExSyncHandler * exSyncGetJackHandler();

void exSyncStoppedHack();

//For AudioJack:
void syncJackd(jack_client_t* client);

}

#else

// Some empty functions/macroses here

#endif



#endif
