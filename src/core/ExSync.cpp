/*
 * ExSync.cpp - support for external synchronization
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

#include "ExSync.h"

#ifdef LMMS_HAVE_EXSYNC


#include "Engine.h"
#include "Song.h"

namespace lmms 
{


/* -----------------------ExSync private --------------------------- */




/**
	 Functions to control LMMS position/playing in Slave || Duplex
	 LMMS react only in if ExSync is on (button is green)
	 MUST be provided by ExSync driver
	 
	 External code MUST NOT use this functions: this is adapter
	 from external device events to LMMS (so not included in ExSync.h)
*/
struct ExSyncCallbacks
{
	//! @playing [true : to start; false : to pause] 
	void (* mode)(bool playing); 
	//! change position to @frames;
	void (* position)(uint32_t frames);
	//! to calculate frames from time (not used here - jack is working in frames)
	sample_rate_t (* processingSampleRate)();
};

static struct ExSyncCallbacks * getSlaveCallbacks();



/* Jack Transport target implementation private part (BEGIN): */


static jack_client_t * s_syncJackd = nullptr; //!< Set by Jack audio 
static jack_transport_state_t s_lastJackState = JackTransportStopped;


/*! Function adapt events from Jack Transport to LMMS  */
static int syncCallBack(jack_transport_state_t state, jack_position_t *pos, void *arg)
{
	struct ExSyncCallbacks *slaveCallBacks  = getSlaveCallbacks();
	// Now slaveCallBacks is local copy - never be changed by other thread ...
	if (slaveCallBacks)
	{
		switch(state)
		{
		case JackTransportStopped:
			slaveCallBacks->mode(false);
			slaveCallBacks->position(pos->frame);
			break;
		case JackTransportStarting:
			slaveCallBacks->mode(true);
			slaveCallBacks->position(pos->frame);
			break;
		case JackTransportRolling: //!< mostly not called with this state
			slaveCallBacks->mode(true);
			slaveCallBacks->position(pos->frame);
			break;
		default:
			; // not use JackTransportLooping  and JackTransportNetStarting enum
		}
	}
	return 1; 
}


/* Functions needed  to control Jack Transport (adapt events from LMMS) */


static bool jackAvailable()
{
	if (s_syncJackd) { return true; } else { return false; }
}


static void jackPlay(bool playing)
{
	if (s_syncJackd)
	{
		if (playing) {
			jack_transport_start(s_syncJackd);
		} else {
			jack_transport_stop(s_syncJackd);
		}
	}
}


static void jackPosition(const SongExtendedPos *pos)
{
	if (s_syncJackd)
	{
		jack_transport_locate(s_syncJackd, pos->frame);
	}
}


static bool jackStopped()
{
	bool justStopped = false;

	if (s_syncJackd)
	{ 
		jack_transport_state_t state = jack_transport_query(s_syncJackd, nullptr);
		if ((JackTransportStopped == state) && (state != s_lastJackState))
		{
			justStopped = true;
		}
		s_lastJackState = state;
	} else {
		s_lastJackState = JackTransportStopped;
	}

	return justStopped;
}


static void jackSlave(bool set)
{
	if (s_syncJackd)
	{
		if (set)
		{
			jack_set_sync_callback(s_syncJackd, &syncCallBack, nullptr);
		} else {
			jack_set_sync_callback(s_syncJackd, nullptr, nullptr);
		}
	}
}



/* (END) [Jack Transport target implementation ] */




/* NEW target implementation private code here */




static struct ExSyncHandler s_handler = {
	&jackAvailable,
	&jackPlay,
	&jackPosition,
	&jackStopped,
	&jackSlave
};


/**
	Model controled by user interface 
	using View/Controller in SongEditor
	(include/SongEditor.h, src/gui/editors/SongEditor.cpp)
 */ 

static struct ExSyncCallbacks *s_slaveCallBacks = nullptr;

static struct ExSyncCallbacks * getSlaveCallbacks() {return s_slaveCallBacks; }


/* class ExSyncHook && class ExSyncCtl: private part */


static bool s_exSyncSlaveOn = false; //!< (Receave)
static bool s_exSyncMasterOn = true; //!< (Send)
static bool s_exSyncOn = false; //!< (React and Send)
static ExSyncCtl::ExSyncMode s_exSyncMode = ExSyncCtl::Master; //!< (for ModeButton state)


static void exSyncMode(bool playing)
{
	auto lSong = Engine::getSong();

	if ((! lSong->isExporting()) && s_exSyncOn && (lSong->isPlaying() != playing)) 
	{
		if ( lSong->isStopped() ) { lSong->playSong(); } else {	lSong->togglePause(); }
	}
}


static void exSyncPosition(uint32_t frames)
{
	auto lSong = Engine::getSong();

	if ((! lSong->isExporting()) && s_exSyncOn && (lSong->playMode()  == Song::PlayMode::Song))
	{
		lSong->setToTime(TimePos::fromFrames(frames , Engine::framesPerTick()));
	}
}


static sample_rate_t exSyncSampleRate()
{
	return Engine::audioEngine()->outputSampleRate();
}


//! Function used by internal code to send messages to LMMS::Song from
//! external device (in Slave , Duplex modes)
static struct ExSyncCallbacks s_exSyncCallbacks = {
	&exSyncMode,
	&exSyncPosition,
	&exSyncSampleRate
};


/* -----------------------ExSync public ----------------------------- */


/* Jack Transport target implementation (public part): */


void syncJackd(jack_client_t* client)
{
	s_syncJackd = client;
}


/* NEW target implementation public code here */


/* Target independent part: */


struct ExSyncHandler * exSyncGetHandler()
{
	return &s_handler;
}


/* class ExSyncHook: public part */


static f_cnt_t s_lastFrame = 0; // Save last frame position to catch change
void ExSyncHook::pulse()
{
	struct ExSyncCallbacks *slaveCallBacks  = getSlaveCallbacks();
	struct ExSyncHandler *sync = exSyncGetHandler();
	auto lSong = Engine::getSong();
	f_cnt_t lFrame = 0;
	if (sync && slaveCallBacks && sync->Stopped()) 
	{ 
		slaveCallBacks->mode(false); 
	}
	if (sync &&  lSong->isStopped())
	{
		lFrame = lSong->getFrames();
		if (s_exSyncMasterOn && s_exSyncOn && (lFrame != s_lastFrame) )
		{
			s_lastFrame = lFrame;
			jump();
		}
	}
}


void ExSyncHook::jump()
{
	struct SongExtendedPos pos;
	auto lSong = Engine::getSong();
	if ((! lSong->isExporting()) && s_exSyncMasterOn && s_exSyncOn)
	{
		pos.bar = lSong->getBars();
		pos.beat = lSong->getBeat();
		pos.tick = lSong->getBeatTicks();
		pos.barStartTick = lSong->getTicks();
		pos.beatsPerBar = lSong->getTimeSigModel().numeratorModel().value();
		pos.beatType = lSong->getTimeSigModel().denominatorModel().value();
		pos.ticksPerBeat = lSong->getPlayPos().ticksPerBeat( lSong->getTimeSigModel() );
		pos.tempo = lSong->getTempo();
		pos.frame = lSong->getFrames();
		ExSyncHandler * sync =  exSyncGetHandler();
		if (sync) { sync->sendPosition(&pos); }
	}
}


void ExSyncHook::start()
{
	struct ExSyncHandler *sync = exSyncGetHandler();
	if (sync && s_exSyncOn)
	{
		sync->sendPlay(true);
		if( ExSyncCtl::Master == s_exSyncMode) { jump(); }
	}
}


void ExSyncHook::stop()
{
	struct ExSyncHandler *sync = exSyncGetHandler();
	if (sync && s_exSyncOn)
	{
		sync->sendPlay(false);
		if( ExSyncCtl::Master == s_exSyncMode) { jump(); }
	}
}


/* class ExSyncCtl: public part */


ExSyncCtl::ExSyncMode ExSyncCtl::toggleMode()
{
	ExSyncHandler * sync =  exSyncGetHandler();
	if ( !sync->availableNow() ) 
	{
		return s_exSyncMode;
	}
	// Make state change (Master -> Slave -> Duplex -> Master -> ...)
	switch(s_exSyncMode)
	{
	case Duplex: // Duplex -> Master
		s_exSyncMode = Master;
		s_exSyncSlaveOn = false;
		s_exSyncMasterOn = true;
		sync->setSlave(false);
		s_slaveCallBacks = nullptr;
		break;
	case Master: // Master -> Slave
		s_exSyncMode = Slave;
		s_exSyncSlaveOn = true;
		s_exSyncMasterOn = false;
		sync->setSlave(true);
		s_slaveCallBacks = &s_exSyncCallbacks;
		break;
	case Slave: // Slave -> Duplex
		s_exSyncMode = Duplex;
		s_exSyncSlaveOn = true; // already set, but ... to be simple
		s_exSyncMasterOn = true;
		sync->setSlave(true); // already set, but ... to be simple 
		s_slaveCallBacks = &s_exSyncCallbacks; // already set, but ... to be simple 
		break;
	default: // never happens, but our compiler want this
		s_exSyncMode = Master;
	}
	return s_exSyncMode;
}


ExSyncCtl::ExSyncMode ExSyncCtl::getMode()
{
	return s_exSyncMode;
}


bool ExSyncCtl::toggleOnOff()
{
	ExSyncHandler * sync =  exSyncGetHandler();

	if ( sync->availableNow() )
	{
		if (s_exSyncOn) {	s_exSyncOn = false; } else { s_exSyncOn = true; }
	} else {
		s_exSyncOn = false;
	}
	return s_exSyncOn;
}


bool ExSyncCtl::have()
{
	ExSyncHandler * sync =  exSyncGetHandler();
	if ( sync->availableNow() ) { return true; }
	return false;
}


} // namespace lmms 

#endif // LMMS_HAVE_EXSYNC

