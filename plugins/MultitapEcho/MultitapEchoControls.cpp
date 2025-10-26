/*
 * MultitapEchoControls.cpp - a multitap echo delay plugin
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QDomElement>
#include <QVarLengthArray>

#include "MultitapEchoControls.h"
#include "MultitapEcho.h"
#include "lmms_math.h"
#include "base64.h"

namespace lmms
{


MultitapEchoControls::MultitapEchoControls( MultitapEchoEffect * eff ) :
	EffectControls( eff ),
	m_effect( eff ),
	m_steps( 16, 4, 32, this, "Steps" ),
	m_stepLength( 100.0f, 1.0f, 500.0f, 0.1f, 500.0f, this, "Step length" ),
	m_dryGain( 0.0f, -80.0f, 20.0f, 0.1f, this, "Dry gain" ),
	m_swapInputs( false, this, "Swap inputs" ),
	m_stages( 1.0f, 1.0f, 4.0f, 1.0f, this, "Lowpass stages" ),
	m_ampGraph( -60.0f, 0.0f, 16, this ),
	m_lpGraph( 0.0f, 3.0f, 16, this )
{
	m_stages.setStrictStepSize( true );
	connect( &m_ampGraph, SIGNAL( samplesChanged( int, int ) ), this, SLOT( ampSamplesChanged( int, int ) ) );
	connect( &m_lpGraph, SIGNAL( samplesChanged( int, int ) ), this, SLOT( lpSamplesChanged( int, int ) ) );

	connect( &m_steps, SIGNAL( dataChanged() ), this, SLOT( lengthChanged() ) );
	connect( Engine::audioEngine(), SIGNAL( sampleRateChanged() ), this, SLOT( sampleRateChanged() ) );

	setDefaultAmpShape();
	setDefaultLpShape();
}


void MultitapEchoControls::saveSettings( QDomDocument & doc, QDomElement & parent )
{
	m_steps.saveSettings( doc, parent, "steps" );
	m_stepLength.saveSettings( doc, parent, "steplength" );
	m_dryGain.saveSettings( doc, parent, "drygain" );
	m_swapInputs.saveSettings( doc, parent, "swapinputs" );
	m_stages.saveSettings( doc, parent, "stages" );
	
	QString ampString;
	base64::encode( (const char *) m_ampGraph.samples(), m_ampGraph.length() * sizeof(float), ampString );
	parent.setAttribute( "ampsteps", ampString );
	
	QString lpString;
	base64::encode( (const char *) m_lpGraph.samples(), m_lpGraph.length() * sizeof(float), lpString );
	parent.setAttribute( "lpsteps", lpString );
}


void MultitapEchoControls::loadSettings( const QDomElement & elem )
{
	m_steps.loadSettings( elem, "steps" );
	m_stepLength.loadSettings( elem, "steplength" );
	m_dryGain.loadSettings( elem, "drygain" );
	m_swapInputs.loadSettings( elem, "swapinputs" );
	m_stages.loadSettings( elem, "stages" );
	
	int size = 0;
	char * dst = 0;
	
	base64::decode( elem.attribute( "ampsteps"), &dst, &size );
	m_ampGraph.setSamples( (float*) dst );

	base64::decode( elem.attribute( "lpsteps"), &dst, &size );
	m_lpGraph.setSamples( (float*) dst );
	
	delete[] dst;
}


void MultitapEchoControls::setDefaultAmpShape()
{
	const int length = m_steps.value();
	
	QVarLengthArray<float> samples(length);
	for( int i = 0; i < length; ++i )
	{
		samples[i] = 0.0f;
	}
	
	m_ampGraph.setSamples( &samples[0] );
}


void MultitapEchoControls::setDefaultLpShape()
{
	const int length = m_steps.value();
	
	QVarLengthArray<float> samples(length);
	for( int i = 0; i < length; ++i )
	{
		samples[i] = 3.0f;
	}
	
	m_lpGraph.setSamples( &samples[0] );
}


void MultitapEchoControls::ampSamplesChanged( int begin, int end )
{
	const float * samples = m_ampGraph.samples();
	for( int i = begin; i <= end; ++i )
	{
		m_effect->m_amp[i] = dbfsToAmp( samples[i] );
	}
}


void MultitapEchoControls::ampResetClicked()
{
	setDefaultAmpShape();
}


void MultitapEchoControls::lpSamplesChanged( int begin, int end )
{
	//qDebug( "b/e %d - %d", begin, end );
	const float * samples = m_lpGraph.samples();
	for( int i = begin; i <= end; ++i )
	{
		m_effect->m_lpFreq[i] = 20.0f * fastPow10f(samples[i]);
	}
	m_effect->updateFilters( begin, end );
}


void MultitapEchoControls::lpResetClicked()
{
	setDefaultLpShape();
}


void MultitapEchoControls::lengthChanged()
{
	const int len = m_steps.value();
	m_ampGraph.setLength( len );
	ampSamplesChanged( 0, len - 1 );
	m_lpGraph.setLength( len );
	lpSamplesChanged( 0, len - 1 );
	m_effect->updateFilters( 0, len - 1 );
}


void MultitapEchoControls::sampleRateChanged()
{
	m_effect->m_sampleRate = Engine::audioEngine()->outputSampleRate();
	m_effect->m_sampleRatio = 1.0f / m_effect->m_sampleRate;
	m_effect->updateFilters( 0, 19 );
}


} // namespace lmms
