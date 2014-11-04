/*
 * LfoController.cpp - implementation of class controller which handles
 *                      remote-control of AutomatableModels
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail.com>
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

#include <math.h>
#include <QtXml/QDomElement>
#include <QtCore/QObject>
#include <QtCore/QVector>


#include "song.h"
#include "engine.h"
#include "Mixer.h"
#include "LfoController.h"
#include "ControllerDialog.h"

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
	m_phaseCorrection( 0 ),
	m_phaseOffset( 0 ),
	m_sampleFunction( &Oscillator::sinSample ),
	m_userDefSampleBuffer( new SampleBuffer )
{

	connect( &m_waveModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleFunction() ) );
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




// This code took forever to get right. It can
// definately be optimized.
// The code should probably be integrated with the oscillator class. But I
// don't know how to use oscillator because it is so confusing
float LfoController::value( int _offset )
{
	int frame = runningFrames() + _offset + m_phaseCorrection;

	//If the song is playing, sync the value with the time of the song.
	if( engine::getSong()->isPlaying() || engine::getSong()->isExporting() )
	{
		// The new duration in frames
		// (Samples/Second) / (periods/second) = (Samples/cycle)
		float newDurationF =
				engine::mixer()->processingSampleRate() *
				m_speedModel.value();

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

		m_phaseOffset = qRound(
			m_phaseModel.value() * newDurationF / 360.0 );


		int newDuration = static_cast<int>( newDurationF );
		m_phaseCorrection = static_cast<int>(engine::getSong()->getTicks()*engine::framesPerTick())%newDuration
								+ m_phaseOffset;

		// re-run the first calculation again
		frame = m_phaseCorrection + _offset;
	}

	// speedModel  0..1   fast..slow  0ms..20000ms
	// duration m_duration
	//

	//  frames / (20seconds of frames)
	float sampleFrame = float( frame+m_phaseOffset ) / 
		(engine::mixer()->processingSampleRate() *  m_speedModel.value() );

	switch(m_multiplierModel.value() )
	{
		case 1:
			sampleFrame *= 100.0;
			break;

		case 2:
			sampleFrame /= 100.0;
			break;

		default:
			break;
	}

	// 44100 frames/sec
	return m_baseModel.value() + ( m_amountModel.value() * 
				( m_sampleFunction != NULL ?
					m_sampleFunction(sampleFrame):
					m_userDefSampleBuffer->userWaveSample(sampleFrame) )
			/ 2.0f );
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


