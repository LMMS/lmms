#ifndef SINGLE_SOURCE_COMPILE

/*
 * lfo_controller.cpp - implementation of class controller which handles remote-control
 *                  of automatableModels
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
#include <Qt/QtXml>
#include <QtCore/QObject>
#include <QtCore/QVector>


#include "song.h"
#include "engine.h"
#include "mixer.h"
#include "lfo_controller.h"
#include "controller_dialog.h"

const float TWO_PI = 6.28318531f;

lfoController::lfoController( model * _parent ) :
	controller( _parent ),
	m_lfoAttackModel( 0.0, 0.0, 1.0, 0.001, this ),
	m_lfoSpeedModel( 0.1, 0.01, 10.0, 0.0001, 20000.0, this ),
	m_lfoAmountModel( 1.0, -1.0, 1.0, 0.005, this ),
	m_lfoWaveModel( SineWave, 0, NumLfoShapes, 1, this ),
	m_duration( 0 ),
	m_phaseCorrection( 0 )
{
}


lfoController::~lfoController()
{
	m_lfoAttackModel.disconnect( this );
	m_lfoSpeedModel.disconnect( this );
	m_lfoAmountModel.disconnect( this );
	m_lfoWaveModel.disconnect( this );
}

// This code took forever to get right. It can
// definately be optimized a bit.
float lfoController::value( int _offset )
{
	int frame = runningFrames() + _offset + m_phaseCorrection;

	// Recalculate speed each period
	if (_offset == 0) {

		// The new duration in frames 
		// (Samples/Second) / (periods/second) = (Samples/cycle)
		int newDuration = engine::getMixer()->sampleRate() / m_lfoSpeedModel.value();

		if (newDuration != m_duration) {
			// frame offset
			// (Samples - Samples) = Samples
			float oldFramePhase = (frame % m_duration);

			// Phase between 0 and 1
			// (Samples/Samples) = factor
			float phase = oldFramePhase / m_duration;

			// where we SHOULD be according to new frequency
			// (factor*Samples) = Samples
			int newFrameOffset = phase * newDuration;

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


	// 44100 frames/sec
	return 0.5 + (m_lfoAmountModel.value() * 
			sinf( TWO_PI * float(frame * m_lfoSpeedModel.value()) / engine::getMixer()->sampleRate() ) / 2.0f);
}


controllerDialog * lfoController::createDialog( QWidget * _parent )
{
	controllerDialog * d = new lfoControllerDialog( this, _parent );

	

	return d;
}


#include "lfo_controller.moc"


#endif
