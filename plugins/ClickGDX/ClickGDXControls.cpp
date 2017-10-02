/*
 * ClickGDXControls.cpp - controls for click remover effect
 *
 * Copyright (c) 2017
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

#include "ClickGDXControls.h"
#include "ClickGDX.h"
#include "Engine.h"
#include "Song.h"


ClickGDXControls::ClickGDXControls( ClickGDXEffect* effect ) :
	EffectControls( effect ),
	m_effect( effect ),
	m_attackTimeModel (    0.2f, 0.0f,    1.0f, 0.00001f, this, tr( "Attack Time" ) ),
	m_descentTimeModel(    0.2f, 0.0f,    1.0f, 0.00001f, this, tr( "Descent Time" ) ),
	m_panTimeModel(        4.0f, 0.0f,  128.0f, 0.00001f, this, tr( "Pan Time" ) ),
	m_attackTypeModel (    1.0f, 0.0f,    6.0f,     1.0f, this, tr( "Attack Type" ) ),
	m_descentTypeModel(    0.0f, 0.0f,    6.0f,     1.0f, this, tr( "Descent Type" ) ),
	m_panTypeModel (       0.0f, 0.0f,    8.0f,     1.0f, this, tr( "Pan Type" ) ),
	m_attackTempoModel ( 140.0f, 1.0f, 4800.0f,     1.0f, this, tr( "Attack Tempo" ) ),
	m_descentTempoModel( 140.0f, 1.0f, 4800.0f,     1.0f, this, tr( "Descent Tempo" ) ),
	m_panTempoModel (    140.0f, 1.0f, 4800.0f,     1.0f, this, tr( "Pan Tempo" ) )
{
/*	connect( &m_volumeModel, SIGNAL( dataChanged() ), this, SLOT( changeControl() ) );
	connect( &m_panModel, SIGNAL( dataChanged() ), this, SLOT( changeControl() ) );
	connect( &m_leftModel, SIGNAL( dataChanged() ), this, SLOT( changeControl() ) );
	connect( &m_rightModel, SIGNAL( dataChanged() ), this, SLOT( changeControl() ) );*/
}




void ClickGDXControls::changeControl()
{
//	engine::getSong()->setModified();
}




void ClickGDXControls::loadSettings( const QDomElement& _this )
{
	m_attackTimeModel.loadSettings( _this, "attack_time" );
	m_descentTimeModel.loadSettings( _this, "descent_time" );
	m_panTimeModel.loadSettings( _this, "pan_time" );
	m_attackTypeModel.loadSettings( _this, "attack_type" );
	m_descentTypeModel.loadSettings( _this, "descent_type" );
	m_panTypeModel.loadSettings( _this, "pan_type" );
	m_attackTempoModel.loadSettings( _this, "attack_tempo" );
	m_descentTempoModel.loadSettings( _this, "descent_tempo" );
	m_panTempoModel.loadSettings( _this, "pan_tempo" );
}




void ClickGDXControls::saveSettings( QDomDocument& doc, QDomElement& _this )
{
	m_attackTimeModel.saveSettings( doc, _this, "attack_time" );
	m_descentTimeModel.saveSettings( doc, _this, "descent_time" );
	m_panTimeModel.saveSettings( doc, _this, "pan_time" );
	m_attackTypeModel.saveSettings( doc, _this, "attack_type" );
	m_descentTypeModel.saveSettings( doc, _this, "descent_type" );
	m_panTypeModel.saveSettings( doc, _this, "pan_type" );
	m_attackTempoModel.saveSettings( doc, _this, "attack_tempo" );
	m_descentTempoModel.saveSettings( doc, _this, "descent_tempo" );
	m_panTempoModel.saveSettings( doc, _this, "pan_tempo" );
}
