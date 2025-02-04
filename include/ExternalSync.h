/*
 * ExternalSync.h - support for external synchronization
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

#ifndef LMMS_EXTERNALSYNC_H
#define LMMS_EXTERNALSYNC_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_JACK

#include "lmms_basics.h"

namespace lmms
{

// Common target independent part:
//! LMMS and Jack ExternalSync provide all fields, but here used only @frame
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


//! Functions, MUST be provided by ExternalSync driver
struct SyncDriverHandler
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
	void (* setFollow)(bool set);
};


/* ExternalSync Extention handler: */

class SyncExtentionHanlder
{
public:
	static void add(struct SyncHandler *syncH);
	static bool remove(struct SyncHandler *syncH);
};


/* ExternalSync API main part: 
 * - this structure is provided by LMMS to implement Follower mode;
 * - must be provided by target (plugin, external device, ...)
 *   so LMMS can control it it Leader mode; 
 */


struct SyncHandler
{
	enum GetPositionFlags
	{
		On = 1,
		Frame = 1 << 1,
		Tick = 1 << 2,
		Beat = 1 << 3,
		Bar = 1 << 4,
		Tempo = 1 << 5,
		BarStartTick = 1 << 6,
		TicksPerBeat = 1 << 7,
		BeatsPerBar = 1 << 8, 
		BeatType = 1 << 9,
		All = 0xFFFF,
		No = 0
	};
	void (* start)();
	void (* stop)();
	void (* jump)(f_cnt_t frame);
	unsigned short (* getPosition)(struct SongExtendedPos *position);
};


/* ExternalSync LMMS handler for target be able to control LMMS
 *  
 */ 


struct SyncHandler *getLMMSSyncHandler();


/* ExternalSync implementation (not ExternalSync API part): 
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
class SyncHook
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
class SyncCtl
{
public:
	//! ExSync modes named from LMMS point of view, toggled in round robin way 
	enum SyncMode 
	{
		Leader = 0, //!< LMMS send commands, but not react 
		Follower, //!< LMMS react but not send
		Duplex, //!< LMMS send and react, position followed to external application
		Last //!< used for array element count 
	};
	static SyncMode toggleMode(); //!< @return mode after call
	static void setMode(SyncMode mode); //!< directly set mode, or set Off, if "Last" is used
	static SyncMode getMode(); //!< @return current mode
	static bool toggleOnOff(); //!< @return true if ExternalSync became active
	static bool have(); //!< @return true if available
};


} // namespace lmms 


#define 	LMMS_HAVE_EXTERNALSYNC


#endif // LMMS_HAVE_JACK


#endif // LMMS_EXTERNALSYNC_H
