/*
 * stk_voice.h - base class for stk voices
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
#ifndef _STK_VOICE_H
#define _STK_VOICE_H

#include "note_play_handle.h"


template <class PROCESSOR, class MODEL>
class stkVoice
{
public:
	stkVoice( Uint8 _polyphony = 64 );
	virtual ~stkVoice();
	
	sampleFrame * FASTCALL playNote( notePlayHandle * _n,
						MODEL * _model,
						sampleFrame * _buffer,
						const fpp_t _frames );

	void setMissingFile( bool _state );

private:
	bool m_filesMissing;
	
	float m_targetFrequency;
	float m_currentFrequency;
	float m_lastFrequency;
	PROCESSOR * m_processor;
	fpp_t m_frame;
	sample_t m_leftSample;
	sample_t m_rightSample;
	Uint8 m_channel;
	float m_leftRMS;
	float m_rightRMS;
	float m_pan;
	fpp_t m_totalAttackFrames;
	fpp_t m_attackFrame;
	float m_deltaVelocity;
	float m_deltaFrequency;
	float m_attackVelocity;
	float m_attackFrequency;
		
	QVector<PROCESSOR *> m_voices;
	Uint8 m_voiceIndex;
	Uint8 m_polyphony;
	
	sample_rate_t m_sampleRate;
	int m_portamentoFrames;
};


template <class PROCESSOR, class MODEL>
stkVoice<PROCESSOR, MODEL>::stkVoice( Uint8 _polyphony ):
	m_lastFrequency( 440.0f ),
	m_polyphony( _polyphony ),
	m_sampleRate( engine::getMixer()->sampleRate() ),
	m_portamentoFrames( 0 )
{
	for( m_voiceIndex = 0; m_voiceIndex < m_polyphony; m_voiceIndex++ )
	{
		m_voices.append( new PROCESSOR( m_sampleRate ) );
	}
}




template <class PROCESSOR, class MODEL>
stkVoice<PROCESSOR, MODEL>::~stkVoice()
{
	for( m_voiceIndex = 0; m_voiceIndex < m_polyphony; m_voiceIndex++ )
	{
		delete m_voices[m_voiceIndex];
	}
}



template <class PROCESSOR, class MODEL>
sampleFrame * FASTCALL stkVoice<PROCESSOR, MODEL>::playNote( 
							notePlayHandle * _n,
							MODEL * _model,
							sampleFrame * _buffer,
							const fpp_t _frames )
{
	// Don't do anything if STK isn't installed properly.
	if( m_filesMissing )
	{
		return _buffer;
	}
	
	// Don't do anything if the instrument is release triggered and we 
	// haven't released yet.
	if( _model->releaseTriggered()->value() && !_n->released() )
	{
		return _buffer;
	}
	
	// If it's a new note, we need to get a voice.
	if ( _n->totalFramesPlayed() == 0 || _n->m_pluginData == NULL )
	{
		// Monophonic instruments only use the first defined voice.
		if( _model->monophonic()->value() )
		{
			if( m_voices[0]->active() && 
						m_voices[0]->note() != NULL )
			{
				m_voices[0]->note()->noteOff( 0 );
			}
			m_processor = m_voices[0];
		}
		// Otherwise, search for a voice that isn't in use.
		else
		{
			m_processor = NULL;
			for( m_voiceIndex = 0; m_voiceIndex < m_polyphony;
							m_voiceIndex++ )
			{
				if( !m_voices[m_voiceIndex]->active() )
				{
					m_processor = m_voices[m_voiceIndex];
					break;
				}
			}
			
			// Don't do anything if no voices are available.
			if( m_processor == NULL )
			{
				return( _buffer );
			}
		}
		
		// Assign the voice and label it as active.
		_n->m_pluginData = m_processor;
		m_processor->setActive( TRUE );
		
		// Initialize the velocity and the number of frames played
		// so far.
		m_processor->setVelocity( 
				static_cast<float>( _n->getVolume() ) /
					100.0f );
		m_processor->setAttackFrame( 0 );
		
		// Initialize the number of frames for the portamento
		// transition.
		m_portamentoFrames = static_cast<int>( 
				engine::getMixer()->sampleRate() * 
				_model->portamento()->value() / 
					1000.0f );
		
		// If the attack is randomized, calculate the number of frames
		// over which the modifications should take place and
		// generate a pair of offsets.
		if( _model->randomizeAttack()->value() )
		{
			m_totalAttackFrames = static_cast<int>( 
					engine::getMixer()->sampleRate() *
					_model->randomizeLength()->value() /
						1000.0f );
			
			m_attackVelocity = 
				_model->randomizeVelocityAmount()->value() * 
					_model->offset();
			if( m_processor->velocity() + m_attackVelocity < 0.0f )
			{ 
				m_attackVelocity = -m_processor->velocity();
			}
			if( m_processor->velocity() + 
					m_attackVelocity > 1.0f )
			{
				m_attackVelocity = 1.0f - m_processor->velocity();
			}
			
			m_processor->setDeltaVelocity( m_attackVelocity );
			
			m_attackFrequency = 
				_model->randomizeFrequencyAmount()->value() *
					_model->offset();
			
			m_processor->setDeltaFrequency( m_attackFrequency );
		}
		else
		{
			m_totalAttackFrames = 0;
			m_processor->setDeltaVelocity( 0.0f );
			m_processor->setDeltaFrequency( 0.0f );
		}
		
		// If the velocity sensitive filter is turned on, calculate the
		// coefficients and clear the taps.  The filter cutoff is set
		// as:
		//    cutoff = max_freq * ( 10^velocity ) / 10
		if( _model->velocitySensitiveLPF()->value() )
		{
			m_processor->velocitySensitiveLPF()->calcFilterCoeffs( 
					pow( 10.0f, m_processor->velocity() +
						m_processor->deltaVelocity() ) *
					engine::getMixer()->sampleRate() / 20.0f,
					_model->velocitySensitiveQ()->value() );
			m_processor->velocitySensitiveLPF()->clearHistory();
		}
	}

	// Used to keep track of the level across the buffer.
	m_leftRMS = 0.0f;
	m_rightRMS = 0.0f;
	
	// Get the processing unit assigned to the note.
	m_processor = static_cast<PROCESSOR *>( _n->m_pluginData );

	// Initialize the pitch settings.
	m_targetFrequency = _n->frequency();
	m_lastFrequency = m_processor->lastFrequency();
	m_currentFrequency = m_lastFrequency;
	
	m_processor->setControls( _model );
	
	// Initialize the attack offset calculation variables.
	m_attackVelocity = 0.0f;
	m_attackFrequency = 1.0f;
	m_attackFrame = m_processor->attackFrame();
	m_deltaVelocity = m_processor->deltaVelocity();
	m_deltaFrequency = m_processor->deltaFrequency();
	
	// Generate the buffer.
	for( m_frame = 0; m_frame < _frames; ++m_frame )
	{
		// Slide across the randomized attack offsets.
		// Will be ignored if randomization is turned off because
		// both m_attackFrame and m_totalAttackFrames will be 
		// zero.  Just a linear iterpolation.
		if( m_attackFrame < m_totalAttackFrames )
		{
			m_attackVelocity = m_deltaVelocity - m_deltaVelocity *
							m_attackFrame / 
							m_totalAttackFrames;
			
			if( m_processor->velocity() + m_attackVelocity < 0.0f )
			{ 
				m_attackVelocity = -m_processor->velocity();
			}
			else if( m_processor->velocity() + m_attackVelocity > 1.0f )
			{
				m_attackVelocity = 1.0f - m_processor->velocity();
			}
			

			m_attackFrequency = m_deltaFrequency - 
						m_deltaFrequency *
							m_attackFrame / 
							m_totalAttackFrames;
			
			// Convert m_attackFrequency to a half step based 
			// scaling factor for the pitch.
			m_attackFrequency = pow( 2.0f, m_attackFrequency / 
								12.0f );
			
			m_attackFrame++;
		}
		
		// If there are any portamento frames to be processed, 
		// calculate the pitch slide based on whatever the most
		// recently used frequency was.
		if( m_portamentoFrames > 0 )
		{
			m_currentFrequency += ( m_targetFrequency - 
					m_lastFrequency ) / 
				static_cast<float>( m_portamentoFrames ); 
			
			// Sliding polyphonic instruments can end up with
			// negative frequencies, so stop the slid as long
			// as the situation persists.
			if( m_currentFrequency < 0.0f )
			{
				m_currentFrequency -= 
					( m_targetFrequency - m_lastFrequency ) / 
					static_cast<float>( m_portamentoFrames );
			}
			
			m_processor->noteOn( m_currentFrequency *
						_model->bendScaler(),
						m_processor->velocity() +
						m_attackVelocity );
		}
		else
		{
			m_processor->noteOn( m_targetFrequency * 
						_model->bendScaler() * 
						m_attackFrequency, 
						m_processor->velocity() + 
						m_attackVelocity );
		}
		
		// Calculate the pan level for the right channel.
		m_pan = ( 1.0f + _model->pan()->value() ) / 2.0f;
		
		// Generate the samples.
		m_leftSample = m_processor->nextSampleLeft() * 
				_model->volume()->value() * ( 1.0f - m_pan );
		m_rightSample = m_processor->nextSampleRight() * 
				_model->volume()->value() * m_pan;
		
		// Process the velocity sensitive filter.
		if( _model->velocitySensitiveLPF()->value() )
		{
			m_leftSample = m_processor->velocitySensitiveLPF()->update( m_leftSample, 0 );
			m_rightSample = m_processor->velocitySensitiveLPF()->update( m_rightSample, 1 );
		}
		
		// Keep track of the levels.
		m_leftRMS += m_leftSample * m_leftSample;
		m_rightRMS += m_rightSample * m_rightSample;
		
		// Stuff the samples into the channels.
		for( m_channel = 0; m_channel < DEFAULT_CHANNELS / 2; 
								++m_channel )
		{
			_buffer[m_frame][m_channel * DEFAULT_CHANNELS / 2] = m_leftSample;
			_buffer[m_frame][( m_channel + 1 ) * DEFAULT_CHANNELS / 2] = m_rightSample;
		}
	}
	
	// Remember the frequency and the number of frames processed.
	m_processor->setLastFrequency( m_currentFrequency );
	m_processor->setAttackFrame( m_attackFrame );
	
	// Keep track of the portamento frames.
	if( m_portamentoFrames > 0 )
	{
		m_portamentoFrames -= _frames;
	}
	
	return( _buffer );
}




template <class PROCESSOR, class MODEL>
		void stkVoice<PROCESSOR, MODEL>::setMissingFile( bool _state )
{
	m_filesMissing = _state;
}


#endif
