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

#ifdef LMMS_HAVE_JACK


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


static jack_client_t * cs_syncJackd = nullptr; //!< Set by Jack audio 
static jack_transport_state_t cs_lastJackState = JackTransportStopped;


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
	if (cs_syncJackd) { return true; } else { return false; }
}


static void jackPlay(bool playing)
{
	if (cs_syncJackd)
	{
		if (playing) {
			jack_transport_start(cs_syncJackd);
		} else {
			jack_transport_stop(cs_syncJackd);
		}
	}
}


static void jackPosition(const SongExtendedPos *pos)
{
	if (cs_syncJackd)
	{
		jack_transport_locate(cs_syncJackd, pos->frame);
	}
}


static bool jackStopped()
{
	bool justStopped = false;

	if (cs_syncJackd)
	{ 
		jack_transport_state_t state = jack_transport_query(cs_syncJackd, nullptr);
		if ((JackTransportStopped == state) && (state != cs_lastJackState))
		{
			justStopped = true;
		}
		cs_lastJackState = state;
	} else {
		cs_lastJackState = JackTransportStopped;
	}

	return justStopped;
}


static void jackSlave(bool set)
{
	if (cs_syncJackd)
	{
		if (set)
		{
			jack_set_sync_callback(cs_syncJackd, &syncCallBack, nullptr);
		} else {
			jack_set_sync_callback(cs_syncJackd, nullptr, nullptr);
		}
	}
}



/* (END) [Jack Transport target implementation ] */




/* NEW target implementation private code here */




// In future will be array ExSyncHandler cs_handler[] 
static struct ExSyncHandler cs_handler = {
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

static struct ExSyncCallbacks *cs_slaveCallBacks = nullptr;

static struct ExSyncCallbacks * getSlaveCallbacks() {return cs_slaveCallBacks; }

static bool cs_exSyncSlaveOn = false; //!< (Receave)
static bool cs_exSyncMasterOn = true; //!< (Send)
static bool cs_exSyncOn = false; //!< (React and Send)
static unsigned cs_exSyncMode = 0; //!< (for ModeButton state)


static void exSyncMode(bool playing)
{
	auto _ = Engine::getSong();

	if ((exSyncReact()) && (_->isPlaying() != playing)) 
	{
		if ( _->isStopped() ) { _->playSong(); } else {	_->togglePause(); }
	}
}


static void exSyncPosition(uint32_t frames)
{
	auto _ = Engine::getSong();

	if ((exSyncReact()) && (_->playMode()  == Song::PlayMode::Song))
	{
		_->setToTime(TimePos::fromFrames(frames , Engine::framesPerTick()));
	}
}


static sample_rate_t exSyncSampleRate()
{
	return Engine::audioEngine()->outputSampleRate();
}


// In future will be array ExSyncHandler cs_exSyncCallbacks[]
// now available only one target: Jack Transport
static struct ExSyncCallbacks cs_exSyncCallbacks = {
	&exSyncMode,
	&exSyncPosition,
	&exSyncSampleRate
};

enum {
	ESM_MASTER = 0, ESM_SLAVE, ESM_DUPLEX, ESM_LAST
};


static const char * cs_exSyncModeStrings[ESM_LAST] = {
	"Master", "Slave", "Duplex"
};




/* -----------------------ExSync public ----------------------------- */


/* Jack Transport target implementation (public part): */


void syncJackd(jack_client_t* client)
{
	cs_syncJackd = client;
}




/* NEW target implementation public code here */



/* Target independent part: */


struct ExSyncHandler * exSyncGetHandler()
{
	return &cs_handler;
}


void exSyncStopped()
{
	struct ExSyncCallbacks *slaveCallBacks  = getSlaveCallbacks();
	struct ExSyncHandler *sync = exSyncGetHandler();
	// Now slaveCallBacks  and sync are local copy - never be changed by other thread ...
	if (sync && slaveCallBacks && sync->Stopped()) { slaveCallBacks->mode(false); }
}


void exSyncSendPosition() 
{
	struct SongExtendedPos pos;
	auto _ = Engine::getSong();
	
	if (cs_exSyncMasterOn && cs_exSyncOn)
	{
		pos.bar = _->getBars();
		pos.beat = _->getBeat();
		pos.tick = _->getBeatTicks();
		pos.barStartTick = _->getTicks();
		pos.beatsPerBar = _->getTimeSigModel().numeratorModel().value();
		pos.beatType = _->getTimeSigModel().denominatorModel().value();
		pos.ticksPerBeat = _->getPlayPos().ticksPerBeat( _->getTimeSigModel() );
		pos.tempo = _->getTempo();
		pos.frame = _->getFrames();
		
		ExSyncHandler * sync =  exSyncGetHandler();
		sync->sendPosition(&pos);
	}
}


void exSyncSendPositioniIfMaster()
{
	if( ESM_MASTER == cs_exSyncMode) { exSyncSendPosition(); }
}


const char * exSyncToggleMode()
{
	ExSyncHandler * sync =  exSyncGetHandler();
	if ( !sync->availableNow() ) 
	{
		// If driver is not available nothing to do ... 
		return cs_exSyncModeStrings[cs_exSyncMode];
	}
	//! Make state change (Master -> Slave -> Duplex -> Master -> ...)
	cs_exSyncMode += 1; 
	if (cs_exSyncMode >= ESM_LAST) { cs_exSyncMode = ESM_MASTER; }
	switch(cs_exSyncMode)
	{
	case ESM_MASTER: // Master
		cs_exSyncSlaveOn = false;
		cs_exSyncMasterOn = true;
		sync->setSlave(false); // ExSync more calls after ExSync.h
		cs_slaveCallBacks = nullptr;
		break;
	case ESM_SLAVE: // Slave
		cs_exSyncSlaveOn = true;
		cs_exSyncMasterOn = false;
		sync->setSlave(true); // ExSync more calls after ExSync.h
		cs_slaveCallBacks = &cs_exSyncCallbacks; // in future = getSlaveCallbacks();
		break;
	case ESM_DUPLEX: // Duplex
		cs_exSyncMasterOn = true;
	}
	return cs_exSyncModeStrings[cs_exSyncMode];
}


const char * exSyncGetModeString()
{
	return cs_exSyncModeStrings[cs_exSyncMode];
}


bool exSyncToggle()
{
	ExSyncHandler * sync =  exSyncGetHandler();

	if ( sync->availableNow() )
	{
		if (cs_exSyncOn) {	cs_exSyncOn = false; } else { cs_exSyncOn = true; }
	} else {
		cs_exSyncOn = false;
	}
	return cs_exSyncOn;
}


bool exSyncReact() { return cs_exSyncOn; }


bool exSyncAvailable()
{
	ExSyncHandler * sync =  exSyncGetHandler();
	if ( sync->availableNow() ) { return true; }
	return false;
}


bool exSyncMasterAndSync() { return cs_exSyncMasterOn && cs_exSyncOn; }


} // namespace lmms 

#endif // LMMS_HAVE_JACK

