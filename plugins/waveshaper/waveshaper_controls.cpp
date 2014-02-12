/*
 * waveshaper_controls.cpp - controls for waveshaper-effect
 *
 * Copyright  * (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtXml/QDomElement>

#include "waveshaper_controls.h"
#include "waveshaper.h"
#include "graph.h"
#include "engine.h"
#include "song.h"


waveShaperControls::waveShaperControls( waveShaperEffect * _eff ) :
	EffectControls( _eff ),
	m_effect( _eff ),
	m_inputModel( 1.0f, 0.0f, 5.0f, 0.01f, this, tr( "Input gain" ) ),
	m_outputModel( 1.0f, 0.0f, 5.0f, 0.01f, this, tr( "Output gain" ) ),
	m_wavegraphModel( 0.0f, 1.0f, 200, this )
{
	connect( &m_inputModel, SIGNAL( dataChanged() ),
			this, SLOT( changeInput() ) );

	connect( &m_outputModel, SIGNAL( dataChanged() ),
			this, SLOT( changeOutput() ) );

	connect( &m_wavegraphModel, SIGNAL( samplesChanged( int, int ) ),
			this, SLOT( samplesChanged( int, int ) ) );

	changeInput();
	changeOutput();
	setDefaultShape();
	
}




void waveShaperControls::changeInput()
{
}




void waveShaperControls::changeOutput()
{
}




void waveShaperControls::samplesChanged( int _begin, int _end)
{	
}




void waveShaperControls::loadSettings( const QDomElement & _this )
{
//load input, output knobs
	m_inputModel.setValue( _this.attribute( "inputGain" ).toFloat() );
	m_outputModel.setValue( _this.attribute( "outputGain" ).toFloat() );
	
//load waveshape
	int size = 0;
	char * dst = 0;
	base64::decode( _this.attribute( "waveShape"), &dst, &size );
	
	m_wavegraphModel.setSamples( (float*) dst );
	delete[] dst;

}




void waveShaperControls::saveSettings( QDomDocument & _doc, 
							QDomElement & _this )
{
//save input, output knobs
	_this.setAttribute( "inputGain", m_inputModel.value() );
	_this.setAttribute( "outputGain", m_outputModel.value() );

//save waveshape
	QString sampleString;
	base64::encode( (const char *)m_wavegraphModel.samples(),
		m_wavegraphModel.length() * sizeof(float), sampleString );
	_this.setAttribute( "waveShape", sampleString );

}


void waveShaperControls::setDefaultShape()
{
	float shp [200] = { };
	for ( int i = 0; i<200; i++) 
	{
		shp[i] = ((float)i + 1.0f) / 200.0f;
	}

	m_wavegraphModel.setLength( 200 );
	m_wavegraphModel.setSamples( (float*)&shp );
}

void waveShaperControls::resetClicked()
{
	setDefaultShape();
	engine::getSong()->setModified();
}

void waveShaperControls::smoothClicked()
{
	m_wavegraphModel.smoothNonCyclic();
	engine::getSong()->setModified();
}



#include "moc_waveshaper_controls.cxx"

