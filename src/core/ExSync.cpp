/*
 * ExSync.cpp - support for external synchronization
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

// ExSync driver API frame based implementation
// BEGIN
static jack_client_t * cs_syncJackd = nullptr;


static bool cs_exSyncAvailable()
{
	if (cs_syncJackd) { return true; }
	return false;
}


static struct lmms::ExSyncCallbacks *cs_slaveCallBacks = nullptr;


static int cs_syncCallBack(jack_transport_state_t state, jack_position_t *pos, void *arg)
{
	struct lmms::ExSyncCallbacks *slaveCallBacks  = cs_slaveCallBacks;
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


static jack_transport_state_t cs_lastState = JackTransportStopped;



static void cs_play( bool playing )
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


static void cs_position(const lmms::SongExtendedPos *pos)
{
	if (cs_syncJackd)
	{
		jack_transport_locate(cs_syncJackd, pos->frame);
	}
}


static void cs_slave(struct lmms::ExSyncCallbacks *cb)
{
	cs_slaveCallBacks = cb;
	if (cs_syncJackd)
	{
		if (cb)
		{
			jack_set_sync_callback (cs_syncJackd, 
					&cs_syncCallBack, nullptr);
		} else {
			jack_set_sync_callback (cs_syncJackd, nullptr, nullptr);
		}
	}
}


static struct lmms::ExSyncHandler cs_handler = {
	&cs_exSyncAvailable,
	&cs_play,
	&cs_position,
	&cs_slave
};


static bool m_exSyncSlaveOn = false; //(Receave)
static bool m_exSyncMasterOn = true; //(Send)
static bool m_exSyncOn = false; //(React and Send)
static unsigned m_exSyncMode = 0; //(for ModeButton state)



static void cs_exSyncMode(bool playing)
{
	lmms::Song * l_song = lmms::Engine::getSong();
	if ((lmms::exSyncReact()) && (l_song->isPlaying() != playing)) 
	{
		if ( l_song->isStopped() )
		{
			l_song->playSong();
		} else {
			l_song->togglePause();
		}
	}
}




static void cs_exSyncPosition(uint32_t frames)
{
	lmms::Song * l_song = lmms::Engine::getSong();
	if ((lmms::exSyncReact()) && (l_song->playMode()  == lmms::Song::PlayMode::Song))
	{
		lmms::TimePos timePos = 
			lmms::TimePos::fromFrames(frames , lmms::Engine::framesPerTick());
		l_song->setToTime(timePos);
	}
}




static lmms::sample_rate_t cs_exSyncSampleRate()
{
	return lmms::Engine::audioEngine()->outputSampleRate();
}




static struct lmms::ExSyncCallbacks cs_exSyncCallbacks = {
	&cs_exSyncMode,
	&cs_exSyncPosition,
	&cs_exSyncSampleRate
};



#define 	EXSYNC_MAX_MODES 	(3)
static const char * cs_exSyncModeStrings[EXSYNC_MAX_MODES] = {
	"Master", "Slave", "Duplex"
};



namespace lmms 
{




struct lmms::ExSyncHandler * exSyncGetJackHandler()
{
	return &cs_handler;
}



// Called from SongEditor.cpp line ~827 (updatePosition implementation)
void exSyncStoppedHack()
{
	//Now static inner interface available!!! TODO:
	struct ExSyncCallbacks *slaveCallBacks  = cs_slaveCallBacks;
	// Now slaveCallBacks is local copy - never be changed by other thread ...
	if (cs_syncJackd && slaveCallBacks)
	{ 
		jack_transport_state_t state = jack_transport_query(cs_syncJackd, nullptr);
		if ( ( JackTransportStopped == state) && (state != cs_lastState) )
		{
			slaveCallBacks->mode(false);
		}
		cs_lastState = state ;
	} else {
		cs_lastState = JackTransportStopped;
	}
}

void syncJackd(jack_client_t* client)
{
	cs_syncJackd = client;
}



//From Song.cpp:
void exSyncSendPosition() 
{
	struct SongExtendedPos pos;
	if (m_exSyncMasterOn && m_exSyncOn)
	{
		//pos.bar = Engine::getSong()->currentBar(); //TODO private
		//pos.beat = Engine::getSong()->getBeat();
		//pos.tick = Engine::getSong()->getBeatTicks();
		//pos.barStartTick = Engine::getSong()->getTicks();
		//pos.beatsPerBar = Engine::getSong()->getTimeSigModel().numeratorModel().value();
		//pos.beatType = Engine::getSong()->getTimeSigModel().denominatorModel().value();
		//pos.ticksPerBeat = Engine::getSong()->getPlayPos().ticksPerBeat( Engine::getSong()->getTimeSigModel() );
		//pos.tempo = Engine::getSong()->getTempo();
		pos.frame = Engine::getSong()->getFrames(); //currentFrame(); //TODO private
		
		//Now static inner interface available!!! TODO:
		ExSyncHandler * sync =  exSyncGetJackHandler();
		sync->sendPosition(&pos);

	}
}



const char * exSyncToggleMode()
{
	//Now static inner interface available!!! TODO:
	ExSyncHandler * sync =  exSyncGetJackHandler();
	if ( !sync->availableNow() ) 
	{
		// If driver is not available nothing to do ... 
		return cs_exSyncModeStrings[m_exSyncMode] ;
	}
	m_exSyncMode += 1; 
	if (m_exSyncMode >= EXSYNC_MAX_MODES) { m_exSyncMode = 0; }
	switch(m_exSyncMode)
	{
	case 0: // Master
		m_exSyncSlaveOn = false;
		m_exSyncMasterOn = true;
		sync->setSlave(nullptr); // ExSync more calls after ExSync.h
		break;
	case 1: // Slave
		m_exSyncSlaveOn = true;
		m_exSyncMasterOn = false;
		sync->setSlave(&cs_exSyncCallbacks); // ExSync more calls after ExSync.h
		break;
	case 2: // Duplex
		m_exSyncMasterOn = true;
	}
	return cs_exSyncModeStrings[m_exSyncMode] ;
}




const char * exSyncGetModeString()
{
	return cs_exSyncModeStrings[m_exSyncMode] ;
}




bool exSyncToggle()
{
	//Now static inner interface available!!! TODO:
	ExSyncHandler * sync =  exSyncGetJackHandler();
	if ( sync->availableNow() )
	{
		if  (m_exSyncOn)
		{
			m_exSyncOn = false;
		} else {
			m_exSyncOn = true;
		}
	} else {
		m_exSyncOn = false;
	}
	return m_exSyncOn;
}




bool exSyncReact() { return m_exSyncOn; }



bool exSyncAvailable()
{
	//Now static inner interface available!!! TODO:
	ExSyncHandler * sync =  exSyncGetJackHandler();
	if ( sync->availableNow() ) { return true; }
	return false;
}



bool exSyncMasterAndSync() { return m_exSyncMasterOn && m_exSyncOn; }





}

#endif

