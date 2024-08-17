/*
 * ExSync.h - support for external synchronization
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

// Jack Transport target implementation:
void syncJackd(jack_client_t* client); //!< called from src/core/audio/AudioJack.cpp

// Common target independent part:
//! ExSync sending code provide all fields (for future), but here used only @frame
struct SongExtendedPos
{
	bar_t bar;
	int beat;
	int tick;
	int barStartTick;
	int beatsPerBar;
	int beatType;
	tick_t ticksPerBeat;
	bpm_t tempo;
	f_cnt_t frame;
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
	//! driver check if target plaing just stopped (after last call) 
	bool (* Stopped)();
	//! driver MUST start/stop remote LMMS controling @set true/false
	void (* setSlave)(bool set);
};

struct ExSyncHandler * exSyncGetHandler();


/* ExSync implementation (not ExSync API part): 
 * semi private - do not use (except bug-fix or refactoring context) 
 */

/** 
	Catch events,needed to sent.
	Events:
	* jump - when song position changed not in "natural" way
	(most challenging event to catch, so even @pulse() is needed);
	* start - when song starts playing ;
	* stop - when song stops playing . 
	Implementation details see in ExSync.cpp
 */ 
class ExSyncHook
{
public:
	static void pulse(); //!< called periodically to catch jump when stopped
	static void jump(); //!< placed where jump introduced by user or by LMMS
	static void start();
	static void stop();
};

/**
	Used to control ExSync by GUI (all calls are in SongEditorWindow) 
*/
class ExSyncCtl
{
public:
	//! ExSync modes named from LMMS point of view, toggled in round robin way 
	enum ExSyncMode 
	{
		Master = 0, //!< LMMS send commands, but not react 
		Slave, //!< LMMS react but not send
		Duplex, //!< LMMS send and react, position followed to external application
		Last //!< used for array element count 
	};
	static ExSyncMode toggleMode(); //!< @return mode after call
	static ExSyncMode getMode(); //!< @return current mode
	static bool toggleOnOff(); //!< @return true if ExSync became active
	static bool have(); //!< @return true if available
};


} // namespace lmms 


#define 	LMMS_HAVE_EXSYNC


#endif // LMMS_HAVE_JACK


#endif // LMMS_EXSYNC_H
