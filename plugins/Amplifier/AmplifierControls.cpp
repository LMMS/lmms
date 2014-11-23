/*
 * AmplifierControls.cpp - controls for amplifier effect
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "AmplifierControls.h"
#include "Amplifier.h"
#include "engine.h"
#include "song.h"


AmplifierControls::AmplifierControls( AmplifierEffect* effect ) :
	EffectControls( effect ),
	m_effect( effect ),
	m_volumeModel( 100.0f, 0.0f, 200.0f, 0.1f, this, tr( "Volume" ) ),
	m_panModel( 0.0f, -100.0f, 100.0f, 0.1f, this, tr( "Panning" ) ),
	m_leftModel( 100.0f, 0.0f, 200.0f, 0.1f, this, tr( "Left gain" ) ),
	m_rightModel( 100.0f, 0.0f, 200.0f, 0.1f, this, tr( "Right gain" ) )
{
/*	connect( &m_volumeModel, SIGNAL( dataChanged() ), this, SLOT( changeControl() ) );
	connect( &m_panModel, SIGNAL( dataChanged() ), this, SLOT( changeControl() ) );
	connect( &m_leftModel, SIGNAL( dataChanged() ), this, SLOT( changeControl() ) );
	connect( &m_rightModel, SIGNAL( dataChanged() ), this, SLOT( changeControl() ) );*/
}




void AmplifierControls::changeControl()
{
//	engine::getSong()->setModified();
}




void AmplifierControls::loadSettings( const QDomElement& _this )
{
	m_volumeModel.loadSettings( _this, "volume" );
	m_panModel.loadSettings( _this, "pan" );
	m_leftModel.loadSettings( _this, "left" );
	m_rightModel.loadSettings( _this, "right" );
}




void AmplifierControls::saveSettings( QDomDocument& doc, QDomElement& _this )
{
	m_volumeModel.saveSettings( doc, _this, "volume" ); 
	m_panModel.saveSettings( doc, _this, "pan" );
	m_leftModel.saveSettings( doc, _this, "left" );
	m_rightModel.saveSettings( doc, _this, "right" );
}



#include "moc_AmplifierControls.cxx"

