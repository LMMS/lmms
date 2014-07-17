/*
 * LfoController.cpp - implementation of class controller which handles
 *                      remote-control of AutomatableModels
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail.com>
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

#include <math.h>
#include <QtXml/QDomElement>
#include <QtCore/QObject>
#include <QtCore/QVector>


#include "song.h"
#include "engine.h"
#include "Mixer.h"
#include "LfoController.h"
#include "ControllerDialog.h"
#include "lmms_math.h"

//const float TWO_PI = 6.28318531f;

LfoController::LfoController( Model * _parent ) :
	Controller( Controller::LfoController, _parent, tr( "LFO Controller" ) ),
	m_baseModel( 0.5, 0.0, 1.0, 0.001, this, tr( "Base value" ) ),
	m_speedModel( 2.0, 0.01, 20.0, 0.0001, 20000.0, this, tr( "Oscillator speed" ) ),
	m_amountModel( 1.0, -1.0, 1.0, 0.005, this, tr( "Oscillator amount" ) ),
	m_phaseModel( 0.0, 0.0, 360.0, 4.0, this, tr( "Oscillator phase" ) ),
	m_waveModel( Oscillator::SineWave, 0, Oscillator::NumWaveShapes,
			this, tr( "Oscillator waveform" ) ),
	m_multiplierModel( 0, 0, 2, this, tr( "Frequency Multiplier" ) ),
	m_duration( 1000 ),
	m_phaseOffset( 0 ),	
	m_currentPhase( 0 ),
	m_sampleFunction( &Oscillator::sinSample ),
	m_userDefSampleBuffer( new SampleBuffer )
{
	setSampleExact( true );
	connect( &m_waveModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleFunction() ) );
	
	connect( &m_speedModel, SIGNAL( dataChanged() ),
			this, SLOT( updateDuration() ) );
	connect( &m_multiplierModel, SIGNAL( dataChanged() ),
			this, SLOT( updateDuration() ) );
	connect( engine::mixer(), SIGNAL( sampleRateChanged() ), 
			this, SLOT( updateDuration() ) );
	
	connect( engine::getSong(), SIGNAL( playbackStateChanged() ),
			this, SLOT( updatePhase() ) );
	connect( engine::getSong(), SIGNAL( playbackPositionChanged() ),
			this, SLOT( updatePhase() ) );
			
	updateDuration();
}




LfoController::~LfoController()
{
	sharedObject::unref( m_userDefSampleBuffer );
	m_baseModel.disconnect( this );
	m_speedModel.disconnect( this );
	m_amountModel.disconnect( this );
	m_phaseModel.disconnect( this );
	m_waveModel.disconnect( this );
	m_multiplierModel.disconnect( this );
}


void LfoController::updateValueBuffer()
{
	m_phaseOffset = m_phaseModel.value() / 360.0;	
	float * values = m_valueBuffer.values();	
	float phase = m_currentPhase + m_phaseOffset;

	// roll phase up until we're in sync with period counter
	m_bufferLastUpdated++; 
	if( m_bufferLastUpdated < s_periods )
	{
		int diff = s_periods - m_bufferLastUpdated;
		phase += static_cast<float>( engine::framesPerTick() * diff ) / m_duration;
		m_bufferLastUpdated += diff;
	}


	for( int i = 0; i < m_valueBuffer.length(); i++ )
	{		
		const float currentSample = m_sampleFunction != NULL 
			? m_sampleFunction( phase )
			: m_userDefSampleBuffer->userWaveSample( phase );
			
		values[i] = qBound( 0.0f, m_baseModel.value() + ( m_amountModel.value() * currentSample / 2.0f ), 1.0f );

		phase += 1.0 / m_duration;
	}
	
	m_currentPhase = absFraction( phase - m_phaseOffset );
}


void LfoController::updatePhase()
{
	m_currentPhase = ( engine::getSong()->getFrames() ) / m_duration;
	m_bufferLastUpdated = s_periods - 1;
}


void LfoController::updateDuration()
{
	float newDurationF = engine::mixer()->processingSampleRate() *	m_speedModel.value();

	switch(m_multiplierModel.value() )
	{
		case 1:
			newDurationF /= 100.0;
			break;

		case 2:
			newDurationF *= 100.0;
			break;

		default:
			break;
	}
	
	m_duration = newDurationF;
}

void LfoController::updateSampleFunction()
{
	switch( m_waveModel.value() )
	{
		case Oscillator::SineWave:
			m_sampleFunction = &Oscillator::sinSample;
			break;
		case Oscillator::TriangleWave:
			m_sampleFunction = &Oscillator::triangleSample;
			break;
		case Oscillator::SawWave:
			m_sampleFunction = &Oscillator::sawSample;
			break;
		case Oscillator::SquareWave:
			m_sampleFunction = &Oscillator::squareSample;
			break;
		case Oscillator::MoogSawWave:
			m_sampleFunction = &Oscillator::moogSawSample;
			break;
		case Oscillator::ExponentialWave:
			m_sampleFunction = &Oscillator::expSample;
			break;
		case Oscillator::WhiteNoise:
			m_sampleFunction = &Oscillator::noiseSample;
			break;
		case Oscillator::UserDefinedWave:
			m_sampleFunction = NULL;
			/*TODO: If C++11 is allowed, should change the type of
			 m_sampleFunction be std::function<sample_t(const float)>
			 and use the line below:
			*/
			//m_sampleFunction = &(m_userDefSampleBuffer->userWaveSample)
			break;
	}
}



void LfoController::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	Controller::saveSettings( _doc, _this );

	m_baseModel.saveSettings( _doc, _this, "base" );
	m_speedModel.saveSettings( _doc, _this, "speed" );
	m_amountModel.saveSettings( _doc, _this, "amount" );
	m_phaseModel.saveSettings( _doc, _this, "phase" );
	m_waveModel.saveSettings( _doc, _this, "wave" );
	m_multiplierModel.saveSettings( _doc, _this, "multiplier" );
	_this.setAttribute( "userwavefile" , m_userDefSampleBuffer->audioFile() );
}



void LfoController::loadSettings( const QDomElement & _this )
{
	Controller::loadSettings( _this );

	m_baseModel.loadSettings( _this, "base" );
	m_speedModel.loadSettings( _this, "speed" );
	m_amountModel.loadSettings( _this, "amount" );
	m_phaseModel.loadSettings( _this, "phase" );
	m_waveModel.loadSettings( _this, "wave" );
	m_multiplierModel.loadSettings( _this, "multiplier" );
	m_userDefSampleBuffer->setAudioFile( _this.attribute("userwavefile" ) );

	updateSampleFunction();
}



QString LfoController::nodeName() const
{
	return( "lfocontroller" );
}



ControllerDialog * LfoController::createDialog( QWidget * _parent )
{
	return new LfoControllerDialog( this, _parent );
}


#include "moc_LfoController.cxx"


