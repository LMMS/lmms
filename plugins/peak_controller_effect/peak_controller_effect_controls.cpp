/*
 * stereomatrix_controls.cpp - controls for stereoMatrix-effect
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
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

#include "peak_controller.h"
#include "peak_controller_effect_controls.h"
#include "peak_controller_effect.h"


peakControllerEffectControls::
peakControllerEffectControls( peakControllerEffect * _eff ) :
	effectControls( _eff ),
	m_effect( _eff ),
	m_baseModel( 0.5, 0.0, 1.0, 0.001, this, tr( "Base value" ) ),
	m_amountModel( 1.0, -1.0, 1.0, 0.005, this, tr( "Modulation amount" ) ),
	m_decayModel( 0.1, 0.01, 5.0, 0.0001, 20000.0, this, tr( "Release decay" ) ),
	m_muteModel( FALSE, this, tr( "Mute output" ) )
{
}



void peakControllerEffectControls::loadSettings( const QDomElement & _this )
{
	printf("peakControllerEffect loadSettings\n");
	m_baseModel.setValue( _this.attribute( "base" ).toFloat() );
	m_amountModel.setValue( _this.attribute( "amount" ).toFloat() );
	m_muteModel.setValue( _this.attribute( "mute" ).toFloat() );
	
	int effectId = _this.attribute( "effectId" ).toInt();
	if( effectId > peakController::s_lastEffectId )
	{
		peakController::s_lastEffectId = effectId;
	}
	m_effect->m_effectId = effectId;

	if( m_effect->m_autoController )
	{
		delete m_effect->m_autoController;
		m_effect->m_autoController = 0;
	}
}




void peakControllerEffectControls::saveSettings( QDomDocument & _doc, 
							QDomElement & _this )
{
	_this.setAttribute( "base", m_baseModel.value() );
	_this.setAttribute( "amount", m_amountModel.value() );
	_this.setAttribute( "mute", m_muteModel.value() );
	_this.setAttribute( "effectId", m_effect->m_effectId );
}



#include "peak_controller_effect_controls.moc"

