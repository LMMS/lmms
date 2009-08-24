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
#include "mixer.h"
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
	m_sampleFunction( &Oscillator::sinSample )
{

	connect( &m_waveModel, SIGNAL( dataChanged() ),
			this, SLOT( updateSampleFunction() ) );
}




LfoController::~LfoController()
{
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

	// Recalculate speed each period
	// Actually, _offset != only if HQ, and we may want to recalc in that case,
	// so this statement may be unrequired
	if (_offset == 0) {

		// The new duration in frames 
		// (Samples/Second) / (periods/second) = (Samples/cycle)
		float newDurationF =
				engine::getMixer()->processingSampleRate() *
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

		if (newDuration != m_duration) {
			// frame offset
			// (Samples - Samples) = Samples
			float oldFramePhase = float(frame % m_duration);

			// Phase between 0 and 1
			// (Samples/Samples) = factor
			float phase = oldFramePhase / m_duration;

			// where we SHOULD be according to new frequency
			// (factor*Samples) = Samples
			int newFrameOffset = static_cast<int>( phase * newDuration );

			// recalc
			// (Samples - (Samples-Samples)) = Samples
			
			// Go back to beginning of last natural period
			m_phaseCorrection = -(runningFrames()%newDuration);

			// newFrameOffset has old phaseCorrection built-in
			m_phaseCorrection += newFrameOffset;

			// re-run the first calculation again
			frame = runningFrames() + m_phaseCorrection;

			m_duration = newDuration;
			
		}
	}

	// speedModel  0..1   fast..slow  0ms..20000ms
	// duration m_duration
	//

	//  frames / (20seconds of frames)
	float sampleFrame = float( frame+m_phaseOffset ) / 
		(engine::getMixer()->processingSampleRate() *  m_speedModel.value() );

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
			m_sampleFunction(sampleFrame) 
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


