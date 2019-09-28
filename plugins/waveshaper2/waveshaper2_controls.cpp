/*
 * waveshaper_controls.cpp - controls for waveshaper-effect
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "waveshaper2_controls.h"
#include "waveshaper2.h"
#include "base64.h"
#include "Graph.h"
#include "Engine.h"
#include "Song.h"


#define onedB 1.1220184543019633f

waveShaper2Controls::waveShaper2Controls( waveShaper2Effect * _eff ) :
	EffectControls( _eff ),
	m_effect( _eff ),
	m_inputModel( 1.0f, 0.0f, 5.0f, 0.01f, this, tr( "Input gain" ) ),
	m_outputModel( 1.0f, 0.0f, 5.0f, 0.01f, this, tr( "Output gain" ) ),
	m_vectorGraphModel( this, true ),
	m_clipModel( false, this )
{
}




void waveShaper2Controls::loadSettings( const QDomElement & _this )
{
//load input, output knobs
	m_inputModel.loadSettings( _this, "inputGain" );
	m_outputModel.loadSettings( _this, "outputGain" );

	m_clipModel.loadSettings( _this, "clipInput" );
}




void waveShaper2Controls::saveSettings( QDomDocument & _doc,
							QDomElement & _this )
{
//save input, output knobs
	m_inputModel.saveSettings( _doc, _this, "inputGain" );
	m_outputModel.saveSettings( _doc, _this, "outputGain" );

	m_clipModel.saveSettings( _doc, _this, "clipInput" );

}


void waveShaper2Controls::setDefaultShape()
{
	//
}
