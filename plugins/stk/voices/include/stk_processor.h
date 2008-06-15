/*
 * stk_processor.h - base class for stk sound generators
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
#ifndef _STK_PROCESSOR_H
#define _STK_PROCESSOR_H

#include "Instrmnt.h"

#include "config_mgr.h"
#include "note_play_handle.h"
#include "types.h"


template <class MODEL, class STKVOICE>
class stkProcessor
{
public:
	stkProcessor( sample_rate_t _sample_rate );
	stkProcessor( sample_rate_t _sample_rate,
			StkFloat _lowest_frequency );
	virtual ~stkProcessor( void );
	
	StkFloat nextSampleLeft( void );
	StkFloat nextSampleRight( void );

	Instrmnt * voice( void );
	bool active( void );
	float lastFrequency( void );
	notePlayHandle * note( void );
	fpp_t attackFrame( void );
	float deltaVelocity( void );
	float deltaFrequency( void );
	basicFilters<> * velocitySensitiveLPF( void );
	float velocity( void );
	void clearBuffer( void );
	
	void setActive( bool _state );
	void setLastFrequency( float _frequency );
	void setNote( notePlayHandle * _note );
	void setAttackFrame( fpp_t _frame );
	void setDeltaVelocity( float _velocity );
	void setDeltaFrequency( float _frequency );
	void setVelocity( float _velocity );
	void noteOn( StkFloat _pitch, StkFloat _velocity );
	
protected:
	Uint8 m_delayRead;
	Uint8 m_delayWrite;

private:
	Instrmnt * m_voice;

	StkFloat m_delay[256];
	bool m_active;
	StkFloat m_sample;
	Uint16 m_bufferIndex;
	float m_lastFrequency;
	float m_velocity;
	notePlayHandle * m_note;
	fpp_t m_attackFrame;
	float m_deltaVelocity;
	float m_deltaFrequency;
	basicFilters<> * m_velocitySensitiveLPF;
};




template <class MODEL, class STKVOICE>
		inline stkProcessor<MODEL, STKVOICE>::stkProcessor( 
						sample_rate_t _sample_rate ):
				m_delayWrite( 0 ),
					      m_lastFrequency( 440.0f )
{
	try
	{
		Stk::setSampleRate( _sample_rate );
		Stk::setRawwavePath( 
				    configManager::inst()->stkDir().toAscii().constData() );
	
		m_voice = new STKVOICE();
	}
	catch( ... )
	{
		m_voice = NULL;
	}
		
	setActive( FALSE );
		
	m_velocitySensitiveLPF = 
		new basicFilters<>( engine::getMixer()->processingSampleRate() );
	
	m_velocitySensitiveLPF->setFilterType( basicFilters<>::LOWPASS );
}	




template <class MODEL, class STKVOICE>
		inline stkProcessor<MODEL, STKVOICE>::stkProcessor( 
						sample_rate_t _sample_rate,
						StkFloat _lowest_frequency ):
	m_delayWrite( 0 ),
	m_lastFrequency( 440.0f )
{
	try
	{
		Stk::setSampleRate( _sample_rate );
		Stk::setRawwavePath( 
				    configManager::inst()->stkDir().toAscii().constData() );
	
		m_voice = new STKVOICE( _lowest_frequency );
	}
	catch( ... )
	{
		m_voice = NULL;
	}
		
	setActive( FALSE );
		
	m_velocitySensitiveLPF = 
		new basicFilters<>( engine::getMixer()->processingSampleRate() );
	
	m_velocitySensitiveLPF->setFilterType( basicFilters<>::LOWPASS );
}	




template <class MODEL, class STKVOICE>
inline stkProcessor<MODEL, STKVOICE>::~stkProcessor( void )
{
	m_voice->noteOff( 0.0 );
	delete m_voice;
	delete m_velocitySensitiveLPF;
}	




template <class MODEL, class STKVOICE>
inline StkFloat stkProcessor<MODEL, STKVOICE>::nextSampleLeft( void )
{
	if( m_voice == NULL )
	{
		return( 0.0f );
	}
	else
	{
		m_sample = m_voice->tick();
		m_delay[m_delayWrite] = m_sample;
		m_delayWrite++;
		return( m_sample );
	}
}




template <class MODEL, class STKVOICE>
inline StkFloat stkProcessor<MODEL, STKVOICE>::nextSampleRight( void )
{
	m_sample = m_delay[m_delayRead];
	m_delayRead++;
	return( m_sample );
}




template <class MODEL, class STKVOICE>
inline Instrmnt * stkProcessor<MODEL, STKVOICE>::voice( void )
{
	 return( m_voice );
}




template <class MODEL, class STKVOICE>
inline bool stkProcessor<MODEL, STKVOICE>::active( void )
{
	 return( m_active );
}




template <class MODEL, class STKVOICE>
inline float stkProcessor<MODEL, STKVOICE>::lastFrequency( void )
{
	 return( m_lastFrequency );
}




template <class MODEL, class STKVOICE>
inline notePlayHandle * stkProcessor<MODEL, STKVOICE>::note( void )
{
	 return( m_note );
}




template <class MODEL, class STKVOICE>
inline fpp_t stkProcessor<MODEL, STKVOICE>::attackFrame( void )
{
	 return( m_attackFrame );
}




template <class MODEL, class STKVOICE>
inline float stkProcessor<MODEL, STKVOICE>::deltaVelocity( void )
{
	 return( m_deltaVelocity );
}




template <class MODEL, class STKVOICE>
inline float stkProcessor<MODEL, STKVOICE>::deltaFrequency( void )
{
	 return( m_deltaFrequency );
}




template <class MODEL, class STKVOICE>
inline basicFilters<> * stkProcessor<MODEL, STKVOICE>::velocitySensitiveLPF( void )
{
	 return( m_velocitySensitiveLPF );
}




template <class MODEL, class STKVOICE>
inline float stkProcessor<MODEL, STKVOICE>::velocity( void )
{
	return( m_velocity );
}




template <class MODEL, class STKVOICE>
void stkProcessor<MODEL, STKVOICE>::setActive( bool _state )
{ 
	m_active = _state;
		
	if( !m_active && m_voice != NULL )
	{
		clearBuffer();
		m_voice->noteOff( 0.0f );
		m_note = NULL;
	}
}



template <class MODEL, class STKVOICE>
void stkProcessor<MODEL, STKVOICE>::setLastFrequency( float _frequency )
{
	m_lastFrequency = _frequency;
}




template <class MODEL, class STKVOICE>
void stkProcessor<MODEL, STKVOICE>::setNote( notePlayHandle * _note )
{
	m_note = _note;
}




template <class MODEL, class STKVOICE>
void stkProcessor<MODEL, STKVOICE>::setAttackFrame( fpp_t _frame )
{
	m_attackFrame = _frame;
}




template <class MODEL, class STKVOICE>
void stkProcessor<MODEL, STKVOICE>::setDeltaVelocity( float _velocity )
{
	m_deltaVelocity = _velocity;
}




template <class MODEL, class STKVOICE>
void stkProcessor<MODEL, STKVOICE>::setDeltaFrequency( float _frequency )
{
	m_deltaFrequency = _frequency;
}




template <class MODEL, class STKVOICE>
void stkProcessor<MODEL, STKVOICE>::clearBuffer( void )
{
	for( m_bufferIndex = 0; m_bufferIndex < 256; m_bufferIndex++ )
	{
		m_delay[m_bufferIndex] = 0.0;
	}
}




template <class MODEL, class STKVOICE>
void stkProcessor<MODEL, STKVOICE>::noteOn( StkFloat _pitch, 
						StkFloat _velocity )
{
	m_voice->noteOn( _pitch, _velocity );
}




template <class MODEL, class STKVOICE>
void stkProcessor<MODEL, STKVOICE>::setVelocity( float _velocity )
{ 
	m_velocity = _velocity; 
}



#endif
