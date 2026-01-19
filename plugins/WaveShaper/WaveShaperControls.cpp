/*
 * WaveShaperControls.cpp - controls for WaveShaper effect
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

#include "WaveShaperControls.h"
#include "WaveShaper.h"
#include "base64.h"
#include "Engine.h"
#include "Song.h"

namespace lmms
{


#define onedB 1.1220184543019633f

WaveShaperControls::WaveShaperControls( WaveShaperEffect * _eff ) :
	EffectControls( _eff ),
	m_effect( _eff ),
	m_inputModel( 1.0f, 0.0f, 5.0f, 0.01f, this, tr( "Input gain" ) ),
	m_outputModel( 1.0f, 0.0f, 5.0f, 0.01f, this, tr( "Output gain" ) ),
	m_clipModel( false, this )
	, m_graphModel(20, 20, GRID_MAX_STEPS, GRID_MAX_STEPS, 200, this, tr("waveshape"))
{
	setDefaultShape();
}








void WaveShaperControls::loadSettings( const QDomElement & _this )
{
//load input, output knobs
	m_inputModel.loadSettings( _this, "inputGain" );
	m_outputModel.loadSettings( _this, "outputGain" );

	m_clipModel.loadSettings( _this, "clipInput" );
	m_graphModel.loadSettings(_this, "waveShapeGraph");
}




void WaveShaperControls::saveSettings( QDomDocument & _doc,
							QDomElement & _this )
{
//save input, output knobs
	m_inputModel.saveSettings( _doc, _this, "inputGain" );
	m_outputModel.saveSettings( _doc, _this, "outputGain" );

	m_clipModel.saveSettings( _doc, _this, "clipInput" );
	m_graphModel.saveSettings(_doc, _this, "waveShapeGraph");
}


void WaveShaperControls::setDefaultShape()
{
	m_graphModel.clear();
	m_graphModel.addItem(VGPoint{VGPoint::Type::bezier}, GridModel::ItemInfo{0.0f, 0.0f});
	m_graphModel.addItem(VGPoint{VGPoint::Type::bezier},
		GridModel::ItemInfo(m_graphModel.getLength(), m_graphModel.getLength()));
}

void WaveShaperControls::resetClicked()
{
	setDefaultShape();
	Engine::getSong()->setModified();
}

} // namespace lmms
