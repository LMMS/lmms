/*
 * stereomatrix_controls.cpp - controls for stereoMatrix-effect
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
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

#include "stereomatrix_controls.h"
#include "stereo_matrix.h"


stereoMatrixControls::stereoMatrixControls( stereoMatrixEffect * _eff ) :
		EffectControls( _eff ),
		m_effect( _eff ),
		m_llModel( 1.0f, -1.0f, 1.0f, 0.01f, this, tr( "Left to Left" ) ),
		m_lrModel( 0.0f, -1.0f, 1.0f, 0.01f, this, tr( "Left to Right" ) ),
		m_rlModel( 0.0f, -1.0f, 1.0f, 0.01f, this, tr( "Right to Left" ) ),
		m_rrModel( 1.0f, -1.0f, 1.0f, 0.01f, this, tr( "Right to Right" ) )
{
	connect( &m_llModel, SIGNAL( dataChanged() ),
			this, SLOT( changeMatrix() ) );
	connect( &m_lrModel, SIGNAL( dataChanged() ),
			this, SLOT( changeMatrix() ) );
	connect( &m_rlModel, SIGNAL( dataChanged() ),
			this, SLOT( changeMatrix() ) );
	connect( &m_rrModel, SIGNAL( dataChanged() ),
			this, SLOT( changeMatrix() ) );

	changeMatrix();
}



void stereoMatrixControls::changeMatrix()
{
}



void stereoMatrixControls::loadSettings( const QDomElement & _this )
{
	m_llModel.loadSettings( _this, "l-l" );
	m_lrModel.loadSettings( _this, "l-r" );
	m_rlModel.loadSettings( _this, "r-l" );
	m_rrModel.loadSettings( _this, "r-r" );
}




void stereoMatrixControls::saveSettings( QDomDocument & _doc, 
							QDomElement & _this )
{
	m_llModel.saveSettings( _doc, _this, "l-l" );
	m_lrModel.saveSettings( _doc, _this, "l-r" );
	m_rlModel.saveSettings( _doc, _this, "r-l" );
	m_rrModel.saveSettings( _doc, _this, "r-r" );
}



#include "moc_stereomatrix_controls.cxx"

