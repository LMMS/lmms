/*
 * LfoController.cpp - implementation of class controller which handles
 *                      remote-control of AutomatableModels
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail.com>
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

#include "LfoController.h"

#include <QDomElement>
#include <QFileInfo>

#include "AudioEngine.h"
#include "Oscillator.h"
#include "PathUtil.h"
#include "SampleLoader.h"
#include "Song.h"

namespace lmms
{


LfoController::LfoController( Model * _parent ) :
	Controller( ControllerType::Lfo, _parent, tr( "LFO Controller" ) ),
	m_baseModel(0.5f, 0.f, 1.f, 0.001f, this, tr("Base value")),
	m_speedModel(2.f, 0.01f, 20.f, 0.0001f, 20000.f, this, tr("Oscillator speed")),
	m_amountModel(1.f, -1.f, 1.f, 0.005f, this, tr("Oscillator amount")),
	m_phaseModel( 0.0, 0.0, 360.0, 4.0, this, tr( "Oscillator phase" ) ),
	m_waveModel( static_cast<int>(Oscillator::WaveShape::Sine), 0, Oscillator::NumWaveShapes,
			this, tr( "Oscillator waveform" ) ),
	m_multiplierModel( 0, 0, 2, this, tr( "Frequency Multiplier" ) ),
	m_duration( 1000 ),
	m_phaseOffset( 0 ),
	m_currentPhase( 0 ),
	m_sampleFunction( &Oscillator::sinSample ),
	m_userDefSampleBuffer(std::make_shared<SampleBuffer>())
{
	setSampleExact( true );
	connect( &m_waveModel, SIGNAL(dataChanged()),
			this, SLOT(updateSampleFunction()), Qt::DirectConnection );

	connect( &m_speedModel, SIGNAL(dataChanged()),
			this, SLOT(updateDuration()), Qt::DirectConnection );
	connect( &m_multiplierModel, SIGNAL(dataChanged()),
			this, SLOT(updateDuration()), Qt::DirectConnection );
	connect( Engine::audioEngine(), SIGNAL(sampleRateChanged()),
			this, SLOT(updateDuration()));

	connect( Engine::getSong(), SIGNAL(playbackStateChanged()),
			this, SLOT(updatePhase()));
	connect( Engine::getSong(), SIGNAL(playbackPositionChanged()),
			this, SLOT(updatePhase()));

	updateDuration();
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


void LfoController::updateValueBuffer()
{
	m_phaseOffset = m_phaseModel.value() / 360.0;
	float phase = m_currentPhase + m_phaseOffset;
	float phasePrev = 0.0f;

	// roll phase up until we're in sync with period counter
	m_bufferLastUpdated++;
	if( m_bufferLastUpdated < s_periods )
	{
		int diff = s_periods - m_bufferLastUpdated;
		phase += static_cast<float>( Engine::audioEngine()->framesPerPeriod() * diff ) / m_duration;
		m_bufferLastUpdated += diff;
	}

	float amount = m_amountModel.value();
	ValueBuffer *amountBuffer = m_amountModel.valueBuffer();
	int amountInc = amountBuffer ? 1 : 0;
	float *amountPtr = amountBuffer ? &(amountBuffer->values()[ 0 ] ) : &amount;
	Oscillator::WaveShape waveshape = static_cast<Oscillator::WaveShape>(m_waveModel.value());

	for( float& f : m_valueBuffer )
	{
		float currentSample = 0;
		switch (waveshape)
		{
		case Oscillator::WaveShape::WhiteNoise:
		{
			if (absFraction(phase) < absFraction(phasePrev))
			{
				// Resample when phase period has completed
				m_heldSample = m_sampleFunction(phase);
			}
			currentSample = m_heldSample;
			break;
		}
		case Oscillator::WaveShape::UserDefined:
		{
			currentSample = Oscillator::userWaveSample(m_userDefSampleBuffer.get(), phase);
			break;
		}
		default:
		{
			if (m_sampleFunction != nullptr)
			{
				currentSample = m_sampleFunction(phase);
			}
		}
	}

		f = std::clamp(m_baseModel.value() + (*amountPtr * currentSample / 2.0f), 0.0f, 1.0f);

		phasePrev = phase;
		phase += 1.0 / m_duration;
		amountPtr += amountInc;
	}

	m_currentPhase = absFraction(phase - m_phaseOffset);
	m_bufferLastUpdated = s_periods;
}

void LfoController::updatePhase()
{
	m_currentPhase = ( Engine::getSong()->getFrames() ) / m_duration;
	m_bufferLastUpdated = s_periods - 1;
}


void LfoController::updateDuration()
{
	float newDurationF = Engine::audioEngine()->outputSampleRate() * m_speedModel.value();

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
	switch( static_cast<Oscillator::WaveShape>(m_waveModel.value()) )
	{
		case Oscillator::WaveShape::Sine:
		default:
			m_sampleFunction = &Oscillator::sinSample;
			break;
		case Oscillator::WaveShape::Triangle:
			m_sampleFunction = &Oscillator::triangleSample;
			break;
		case Oscillator::WaveShape::Saw:
			m_sampleFunction = &Oscillator::sawSample;
			break;
		case Oscillator::WaveShape::Square:
			m_sampleFunction = &Oscillator::squareSample;
			break;
		case Oscillator::WaveShape::MoogSaw:
			m_sampleFunction = &Oscillator::moogSawSample;
			break;
		case Oscillator::WaveShape::Exponential:
			m_sampleFunction = &Oscillator::expSample;
			break;
		case Oscillator::WaveShape::WhiteNoise:
			m_sampleFunction = &Oscillator::noiseSample;
			break;
		case Oscillator::WaveShape::UserDefined:
			m_sampleFunction = nullptr;
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
	_this.setAttribute("userwavefile", m_userDefSampleBuffer->audioFile());
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

	if (const auto userWaveFile = _this.attribute("userwavefile"); !userWaveFile.isEmpty())
	{
		if (QFileInfo(PathUtil::toAbsolute(userWaveFile)).exists())
		{
			m_userDefSampleBuffer = gui::SampleLoader::createBufferFromFile(_this.attribute("userwavefile"));
		}
		else { Engine::getSong()->collectError(QString("%1: %2").arg(tr("Sample not found"), userWaveFile)); }
	}

	updateSampleFunction();
}



QString LfoController::nodeName() const
{
	return( "lfocontroller" );
}



gui::ControllerDialog * LfoController::createDialog( QWidget * _parent )
{
	return new gui::LfoControllerDialog( this, _parent );
}


} // namespace lmms
