/*
 * EnvelopeAndLfoParameters.cpp - class EnvelopeAndLfoParameters
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#include <QtXml/QDomElement>

#include "EnvelopeAndLfoParameters.h"
#include "debug.h"
#include "engine.h"
#include "Mixer.h"
#include "Oscillator.h"


// how long should be each envelope-segment maximal (e.g. attack)?
extern const float SECS_PER_ENV_SEGMENT = 5.0f;
// how long should be one LFO-oscillation maximal?
extern const float SECS_PER_LFO_OSCILLATION = 20.0f;


EnvelopeAndLfoParameters::LfoInstances * EnvelopeAndLfoParameters::s_lfoInstances = NULL;


void EnvelopeAndLfoParameters::LfoInstances::trigger()
{
	QMutexLocker m( &m_lfoListMutex );
	for( LfoList::Iterator it = m_lfos.begin();
							it != m_lfos.end(); ++it )
	{
		( *it )->m_lfoFrame +=
				engine::mixer()->framesPerPeriod();
		( *it )->m_bad_lfoShapeData = true;
	}
}




void EnvelopeAndLfoParameters::LfoInstances::reset()
{
	QMutexLocker m( &m_lfoListMutex );
	for( LfoList::Iterator it = m_lfos.begin();
							it != m_lfos.end(); ++it )
	{
		( *it )->m_lfoFrame = 0;
		( *it )->m_bad_lfoShapeData = true;
	}
}




void EnvelopeAndLfoParameters::LfoInstances::add( EnvelopeAndLfoParameters * lfo )
{
	QMutexLocker m( &m_lfoListMutex );
	m_lfos.append( lfo );
}




void EnvelopeAndLfoParameters::LfoInstances::remove( EnvelopeAndLfoParameters * lfo )
{
	QMutexLocker m( &m_lfoListMutex );
	m_lfos.removeAll( lfo );
}




EnvelopeAndLfoParameters::EnvelopeAndLfoParameters(
					float _value_for_zero_amount,
							Model * _parent ) :
	Model( _parent ),
	m_used( false ),
	m_predelayModel( 0.0, 0.0, 2.0, 0.001, this, tr( "Predelay" ) ),
	m_attackModel( 0.0, 0.0, 2.0, 0.001, this, tr( "Attack" ) ),
	m_holdModel( 0.5, 0.0, 2.0, 0.001, this, tr( "Hold" ) ),
	m_decayModel( 0.5, 0.0, 2.0, 0.001, this, tr( "Decay" ) ),
	m_sustainModel( 0.5, 0.0, 1.0, 0.001, this, tr( "Sustain" ) ),
	m_releaseModel( 0.1, 0.0, 2.0, 0.001, this, tr( "Release" ) ),
	m_amountModel( 0.0, -1.0, 1.0, 0.005, this, tr( "Modulation" ) ),
	m_valueForZeroAmount( _value_for_zero_amount ),
	m_pahdEnv( NULL ),
	m_rEnv( NULL ),
	m_lfoPredelayModel( 0.0, 0.0, 1.0, 0.001, this, tr( "LFO Predelay" ) ),
	m_lfoAttackModel( 0.0, 0.0, 1.0, 0.001, this, tr( "LFO Attack" ) ),
	m_lfoSpeedModel( 0.1, 0.001, 1.0, 0.0001,
				SECS_PER_LFO_OSCILLATION * 1000.0, this,
							tr( "LFO speed" ) ),
	m_lfoAmountModel( 0.0, -1.0, 1.0, 0.005, this, tr( "LFO Modulation" ) ),
	m_lfoWaveModel( SineWave, 0, NumLfoShapes, this, tr( "LFO Wave Shape" ) ),
	m_x100Model( false, this, tr( "Freq x 100" ) ),
	m_controlEnvAmountModel( false, this, tr( "Modulate Env-Amount" ) ),
	m_lfoFrame( 0 ),
	m_lfoAmountIsZero( false ),
	m_lfoShapeData( NULL )
{
	m_amountModel.setCenterValue( 0 );
	m_lfoAmountModel.setCenterValue( 0 );

	if( s_lfoInstances == NULL )
	{
		s_lfoInstances = new LfoInstances();
	}

	instances()->add( this );

	connect( &m_predelayModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleVars() ) );
	connect( &m_attackModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleVars() ) );
	connect( &m_holdModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleVars() ) );
	connect( &m_decayModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleVars() ) );
	connect( &m_sustainModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleVars() ) );
	connect( &m_releaseModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleVars() ) );
	connect( &m_amountModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleVars() ) );

	connect( &m_lfoPredelayModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleVars() ) );
	connect( &m_lfoAttackModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleVars() ) );
	connect( &m_lfoSpeedModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleVars() ) );
	connect( &m_lfoAmountModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleVars() ) );
	connect( &m_lfoWaveModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleVars() ) );
	connect( &m_x100Model, SIGNAL( dataChanged() ),
				this, SLOT( updateSampleVars() ) );

	connect( engine::mixer(), SIGNAL( sampleRateChanged() ),
				this, SLOT( updateSampleVars() ) );


	m_lfoShapeData =
		new sample_t[engine::mixer()->framesPerPeriod()];

	updateSampleVars();
}




EnvelopeAndLfoParameters::~EnvelopeAndLfoParameters()
{
	m_predelayModel.disconnect( this );
	m_attackModel.disconnect( this );
	m_holdModel.disconnect( this );
	m_decayModel.disconnect( this );
	m_sustainModel.disconnect( this );
	m_releaseModel.disconnect( this );
	m_amountModel.disconnect( this );
	m_lfoPredelayModel.disconnect( this );
	m_lfoAttackModel.disconnect( this );
	m_lfoSpeedModel.disconnect( this );
	m_lfoAmountModel.disconnect( this );
	m_lfoWaveModel.disconnect( this );
	m_x100Model.disconnect( this );

	delete[] m_pahdEnv;
	delete[] m_rEnv;
	delete[] m_lfoShapeData;

	instances()->remove( this );

	if( instances()->isEmpty() )
	{
		delete instances();
		s_lfoInstances = NULL;
	}
}




inline sample_t EnvelopeAndLfoParameters::lfoShapeSample( fpp_t _frame_offset )
{
	f_cnt_t frame = ( m_lfoFrame + _frame_offset ) % m_lfoOscillationFrames;
	const float phase = frame / static_cast<float>(
						m_lfoOscillationFrames );
	sample_t shape_sample;
	switch( m_lfoWaveModel.value()  )
	{
		case TriangleWave:
			shape_sample = Oscillator::triangleSample( phase );
			break;
		case SquareWave:
			shape_sample = Oscillator::squareSample( phase );
			break;
		case SawWave:
			shape_sample = Oscillator::sawSample( phase );
			break;
		case UserDefinedWave:
			shape_sample = m_userWave.userWaveSample( phase );
			break;
		case RandomWave:
			if( frame == 0 )
			{
				m_random = Oscillator::noiseSample( 0.0f );
			}
			shape_sample = m_random;
			break;
		case SineWave:
		default:
			shape_sample = Oscillator::sinSample( phase );
			break;
	}
	return shape_sample * m_lfoAmount;
}




void EnvelopeAndLfoParameters::updateLfoShapeData()
{
	const fpp_t frames = engine::mixer()->framesPerPeriod();
	for( fpp_t offset = 0; offset < frames; ++offset )
	{
		m_lfoShapeData[offset] = lfoShapeSample( offset );
	}
	m_bad_lfoShapeData = false;
}




inline void EnvelopeAndLfoParameters::fillLfoLevel( float * _buf,
							f_cnt_t _frame,
							const fpp_t _frames )
{
	if( m_lfoAmountIsZero || _frame <= m_lfoPredelayFrames )
	{
		for( fpp_t offset = 0; offset < _frames; ++offset )
		{
			*_buf++ = 0.0f;
		}
		return;
	}
	_frame -= m_lfoPredelayFrames;

	if( m_bad_lfoShapeData )
	{
		updateLfoShapeData();
	}

	fpp_t offset = 0;
	const float lafI = 1.0f / m_lfoAttackFrames;
	for( ; offset < _frames && _frame < m_lfoAttackFrames; ++offset,
								++_frame )
	{
		*_buf++ = m_lfoShapeData[offset] * _frame * lafI;
	}
	for( ; offset < _frames; ++offset )
	{
		*_buf++ = m_lfoShapeData[offset];
	}
}




void EnvelopeAndLfoParameters::fillLevel( float * _buf, f_cnt_t _frame,
						const f_cnt_t _release_begin,
						const fpp_t _frames )
{
	if( _frame < 0 || _release_begin < 0 )
	{
		return;
	}

	fillLfoLevel( _buf, _frame, _frames );

	for( fpp_t offset = 0; offset < _frames; ++offset, ++_buf, ++_frame )
	{
		float env_level;
		if( _frame < _release_begin )
		{
			if( _frame < m_pahdFrames )
			{
				env_level = m_pahdEnv[_frame];
			}
			else
			{
				env_level = m_sustainLevel;
			}
		}
		else if( ( _frame - _release_begin ) < m_rFrames )
		{
			env_level = m_rEnv[_frame - _release_begin] *
				( ( _release_begin < m_pahdFrames ) ?
				m_pahdEnv[_release_begin] : m_sustainLevel );
		}
		else
		{
			env_level = 0.0f;
		}

		// at this point, *_buf is LFO level
		*_buf = m_controlEnvAmountModel.value() ?
			env_level * ( 0.5f + *_buf ) :
			env_level + *_buf;
	}
}




void EnvelopeAndLfoParameters::saveSettings( QDomDocument & _doc,
							QDomElement & _parent )
{
	m_predelayModel.saveSettings( _doc, _parent, "pdel" );
	m_attackModel.saveSettings( _doc, _parent, "att" );
	m_holdModel.saveSettings( _doc, _parent, "hold" );
	m_decayModel.saveSettings( _doc, _parent, "dec" );
	m_sustainModel.saveSettings( _doc, _parent, "sustain" );
	m_releaseModel.saveSettings( _doc, _parent, "rel" );
	m_amountModel.saveSettings( _doc, _parent, "amt" );
	m_lfoWaveModel.saveSettings( _doc, _parent, "lshp" );
	m_lfoPredelayModel.saveSettings( _doc, _parent, "lpdel" );
	m_lfoAttackModel.saveSettings( _doc, _parent, "latt" );
	m_lfoSpeedModel.saveSettings( _doc, _parent, "lspd" );
	m_lfoAmountModel.saveSettings( _doc, _parent, "lamt" );
	m_x100Model.saveSettings( _doc, _parent, "x100" );
	m_controlEnvAmountModel.saveSettings( _doc, _parent, "ctlenvamt" );
	_parent.setAttribute( "userwavefile", m_userWave.audioFile() );
}




void EnvelopeAndLfoParameters::loadSettings( const QDomElement & _this )
{
	m_predelayModel.loadSettings( _this, "pdel" );
	m_attackModel.loadSettings( _this, "att" );
	m_holdModel.loadSettings( _this, "hold" );
	m_decayModel.loadSettings( _this, "dec" );
	m_sustainModel.loadSettings( _this, "sustain" );
	m_releaseModel.loadSettings( _this, "rel" );
	m_amountModel.loadSettings( _this, "amt" );
	m_lfoWaveModel.loadSettings( _this, "lshp" );
	m_lfoPredelayModel.loadSettings( _this, "lpdel" );
	m_lfoAttackModel.loadSettings( _this, "latt" );
	m_lfoSpeedModel.loadSettings( _this, "lspd" );
	m_lfoAmountModel.loadSettings( _this, "lamt" );
	m_x100Model.loadSettings( _this, "x100" );
	m_controlEnvAmountModel.loadSettings( _this, "ctlenvamt" );

/*	 ### TODO:
	Old reversed sustain kept for backward compatibility
	with 4.15 file format*/

	if( _this.hasAttribute( "sus" ) )
	{	
		m_sustainModel.loadSettings( _this, "sus" );
		m_sustainModel.setValue( 1.0 - m_sustainModel.value() );
	}

	// ### TODO:
/*	// Keep compatibility with version 2.1 file format
	if( _this.hasAttribute( "lfosyncmode" ) )
	{
		m_lfoSpeedKnob->setSyncMode(
		( TempoSyncKnob::TtempoSyncMode ) _this.attribute(
						"lfosyncmode" ).toInt() );
	}*/
	
	m_userWave.setAudioFile( _this.attribute( "userwavefile" ) );

	updateSampleVars();
}




void EnvelopeAndLfoParameters::updateSampleVars()
{
	engine::mixer()->lock();

	const float frames_per_env_seg = SECS_PER_ENV_SEGMENT *
				engine::mixer()->processingSampleRate();
	// TODO: Remove the expKnobVals, time should be linear
	const f_cnt_t predelay_frames = static_cast<f_cnt_t>(
							frames_per_env_seg *
					expKnobVal( m_predelayModel.value() ) );

	const f_cnt_t attack_frames = static_cast<f_cnt_t>( frames_per_env_seg *
					expKnobVal( m_attackModel.value() ) );

	const f_cnt_t hold_frames = static_cast<f_cnt_t>( frames_per_env_seg *
					expKnobVal( m_holdModel.value() ) );

	const f_cnt_t decay_frames = static_cast<f_cnt_t>( frames_per_env_seg *
					expKnobVal( m_decayModel.value() *
						( 1 - m_sustainModel.value() ) ) );

	m_sustainLevel = m_sustainModel.value();
	m_amount = m_amountModel.value();
	if( m_amount >= 0 )
	{
		m_amountAdd = ( 1.0f - m_amount ) * m_valueForZeroAmount;
	}
	else
	{
		m_amountAdd = m_valueForZeroAmount;
	}

	m_pahdFrames = predelay_frames + attack_frames + hold_frames +
								decay_frames;
	m_rFrames = static_cast<f_cnt_t>( frames_per_env_seg *
					expKnobVal( m_releaseModel.value() ) );

	if( static_cast<int>( floorf( m_amount * 1000.0f ) ) == 0 )
	{
		//m_pahdFrames = 0;
		m_rFrames = 0;
	}

	delete[] m_pahdEnv;
	delete[] m_rEnv;

	m_pahdEnv = new sample_t[m_pahdFrames];
	m_rEnv = new sample_t[m_rFrames];

	const float aa = m_amountAdd;
	for( f_cnt_t i = 0; i < predelay_frames; ++i )
	{
		m_pahdEnv[i] = aa;
	}

	f_cnt_t add = predelay_frames;

	const float afI = ( 1.0f / attack_frames ) * m_amount;
	for( f_cnt_t i = 0; i < attack_frames; ++i )
	{
		m_pahdEnv[add+i] = i * afI + aa;
	}

	add += attack_frames;
	const float amsum = m_amount + m_amountAdd;
	for( f_cnt_t i = 0; i < hold_frames; ++i )
	{
		m_pahdEnv[add + i] = amsum;
	}

	add += hold_frames;
	const float dfI = ( 1.0 / decay_frames ) * ( m_sustainLevel -1 ) * m_amount;
	for( f_cnt_t i = 0; i < decay_frames; ++i )
	{
/*
		m_pahdEnv[add + i] = ( m_sustainLevel + ( 1.0f -
						(float)i / decay_frames ) *
						( 1.0f - m_sustainLevel ) ) *
							m_amount + m_amountAdd;
*/
		m_pahdEnv[add + i] = amsum + i*dfI;
	}

	const float rfI = ( 1.0f / m_rFrames ) * m_amount;
	for( f_cnt_t i = 0; i < m_rFrames; ++i )
	{
		m_rEnv[i] = (float)( m_rFrames - i ) * rfI;
	}

	// save this calculation in real-time-part
	m_sustainLevel = m_sustainLevel * m_amount + m_amountAdd;


	const float frames_per_lfo_oscillation = SECS_PER_LFO_OSCILLATION *
				engine::mixer()->processingSampleRate();
	m_lfoPredelayFrames = static_cast<f_cnt_t>( frames_per_lfo_oscillation *
				expKnobVal( m_lfoPredelayModel.value() ) );
	m_lfoAttackFrames = static_cast<f_cnt_t>( frames_per_lfo_oscillation *
				expKnobVal( m_lfoAttackModel.value() ) );
	m_lfoOscillationFrames = static_cast<f_cnt_t>(
						frames_per_lfo_oscillation *
						m_lfoSpeedModel.value() );
	if( m_x100Model.value() )
	{
		m_lfoOscillationFrames /= 100;
	}
	m_lfoAmount = m_lfoAmountModel.value() * 0.5f;

	m_used = true;
	if( static_cast<int>( floorf( m_lfoAmount * 1000.0f ) ) == 0 )
	{
		m_lfoAmountIsZero = true;
		if( static_cast<int>( floorf( m_amount * 1000.0f ) ) == 0 )
		{
			m_used = false;
		}
	}
	else
	{
		m_lfoAmountIsZero = false;
	}

	m_bad_lfoShapeData = true;

	emit dataChanged();

	engine::mixer()->unlock();
}





#include "moc_EnvelopeAndLfoParameters.cxx"


