/*
 * DisintegratorControls.cpp
 *
 * Copyright (c) 2019 Lost Robot <r94231@gmail.com>
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

#include "DisintegratorControls.h"
#include "Disintegrator.h"
#include "Engine.h"
#include "Song.h"


DisintegratorControls::DisintegratorControls( DisintegratorEffect* effect ) :
	EffectControls( effect ),
	m_effect( effect ),
	// ==TAG== INITIALIZEMODELS ==TAG== //
	m_volumeModel( 20.0f, 20.0f, 21050.0f, 0.001f, this, tr( "Volume" ) ),
	m_panModel( 20.0f, 20.0f, 21050.0f, 0.001f, this, tr( "Panning" ) ),
	m_leftModel( 50.0f, 0.0f, 100.0f, 0.001f, this, tr( "Left gain" ) ),
	m_rightModel( 100.0f, 0.0f, 200.0f, 0.1f, this, tr( "Right gain" ) )
	// ==TAG== ENDINITIALIZEMODELS ==TAG== //
{
}


void DisintegratorControls::saveSettings( QDomDocument& doc, QDomElement& _this )
{
	// ==TAG== SAVESETTINGS ==TAG== //
	m_volumeModel.saveSettings( doc, _this, "volume" ); 
	m_panModel.saveSettings( doc, _this, "pan" );
	m_leftModel.saveSettings( doc, _this, "left" );
	m_rightModel.saveSettings( doc, _this, "right" );
	// ==TAG== ENDSAVESETTINGS ==TAG== //
}



void DisintegratorControls::loadSettings( const QDomElement& _this )
{
	// ==TAG== LOADSETTINGS ==TAG== //
	m_volumeModel.loadSettings( _this, "volume" );
	m_panModel.loadSettings( _this, "pan" );
	m_leftModel.loadSettings( _this, "left" );
	m_rightModel.loadSettings( _this, "right" );
	// ==TAG== ENDLOADSETTINGS ==TAG== //
}


