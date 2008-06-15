/*
 * stk_instrument.h - base class for stk interfaces to lmms
 *
 * Copyright (c) 2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * 
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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
#ifndef _STK_INSTRUMENT_H
#define _STK_INSTRUMENT_H

#include "instrument.h"

#include "stk_voice.h"


template <class PROCESSOR, class MODEL>
class stkInstrument : public instrument
{
public:
	stkInstrument( instrumentTrack * _channel_track, 
			const descriptor * _descriptor, 
			Uint8 _polyphony = 64 );
	virtual ~stkInstrument();
	
	void playNote( notePlayHandle * _n, bool _try_parallelizing, sampleFrame * _buf );

	void deleteNotePluginData( notePlayHandle * _n );

	void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	
	void loadSettings( const QDomElement & _this );
	
	MODEL * model( void ) const;

	void setMissingFile( bool _state );

private:
	MODEL * m_model;

	stkVoice<PROCESSOR, MODEL> * m_voice;

	sampleFrame * m_buffer;
	fpp_t m_frames;
};




template <class PROCESSOR, class MODEL>
stkInstrument<PROCESSOR, MODEL>::stkInstrument( 
					instrumentTrack * _channel_track,
					const descriptor * _descriptor, 
					Uint8 _polyphony ):
	instrument( _channel_track, _descriptor ),
	m_model( new MODEL() ),
	m_voice( new stkVoice<PROCESSOR, MODEL>( _polyphony ) ),
	m_buffer( new sampleFrame[engine::getMixer()->framesPerPeriod()] )
{
	m_model->monophonic()->setTrack( _channel_track );
	m_model->portamento()->setTrack( _channel_track );
	m_model->bend()->setTrack( _channel_track );
	m_model->bendRange()->setTrack( _channel_track );
	m_model->velocitySensitiveLPF()->setTrack( _channel_track );
	m_model->velocitySensitiveQ()->setTrack( _channel_track );
	m_model->volume()->setTrack( _channel_track );
	m_model->pan()->setTrack( _channel_track );
	m_model->releaseTriggered()->setTrack( _channel_track );
	m_model->randomizeAttack()->setTrack( _channel_track );
	m_model->randomizeLength()->setTrack( _channel_track );
	m_model->randomizeVelocityAmount()->setTrack( _channel_track );
	m_model->randomizeFrequencyAmount()->setTrack( _channel_track );
	m_model->spread()->setTrack( _channel_track );
}




template <class PROCESSOR, class MODEL>
stkInstrument<PROCESSOR, MODEL>::~stkInstrument()
{
	delete m_voice;
	delete m_model;
}




template <class PROCESSOR, class MODEL>
inline void stkInstrument<PROCESSOR, MODEL>::playNote( 
						notePlayHandle * _n,
						bool _try_parallelizing,
						sampleFrame * _buf )
{
	m_frames = _n->framesLeftForCurrentPeriod();
		
	m_voice->playNote( _n, m_model, _buf, m_frames );
	getInstrumentTrack()->processAudioBuffer( _buf, m_frames, _n );
}




template <class PROCESSOR, class MODEL>
inline void stkInstrument<PROCESSOR, MODEL>::deleteNotePluginData( 
							notePlayHandle * _n )
{
	static_cast<PROCESSOR *>( _n->m_pluginData )->setActive( FALSE );
	_n->m_pluginData = NULL;
}




template <class PROCESSOR, class MODEL>
void stkInstrument<PROCESSOR, MODEL>::saveSettings( 
						QDomDocument & _doc, 
						QDomElement & _parent )
{
	m_model->saveSettings( _doc, _parent );
}




template <class PROCESSOR, class MODEL>
inline void stkInstrument<PROCESSOR, MODEL>::loadSettings( 
						const QDomElement & _this )
{
	m_model->loadSettings( _this );
}




template <class PROCESSOR, class MODEL>
inline MODEL * stkInstrument<PROCESSOR, MODEL>::model( void ) const
{ 
	return( m_model ); 
}




template <class PROCESSOR, class MODEL>
inline void stkInstrument<PROCESSOR, MODEL>::setMissingFile( bool _state )
{
	m_voice->setMissingFile( _state );
}


#endif
