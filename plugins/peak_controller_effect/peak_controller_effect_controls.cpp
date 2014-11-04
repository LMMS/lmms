/*
 * peak_controller_effect_controls.cpp - controls for peakController effect
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
 * Copyright (c) 2009-2011 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "PeakController.h"
#include "peak_controller_effect_controls.h"
#include "peak_controller_effect.h"
#include "PresetPreviewPlayHandle.h"
#include "song.h"


PeakControllerEffectControls::
PeakControllerEffectControls( PeakControllerEffect * _eff ) :
	EffectControls( _eff ),
	m_effect( _eff ),
	m_baseModel( 0.5, 0.0, 1.0, 0.001, this, tr( "Base value" ) ),
	m_amountModel( 1.0, -1.0, 1.0, 0.005, this, tr( "Modulation amount" ) ),
	m_attackModel( 0, 0, 0.999, 0.001, this, tr( "Attack" ) ),
	m_decayModel( 0, 0, 0.999, 0.001, this, tr( "Release" ) ),
	m_muteModel( false, this, tr( "Mute output" ) ),
	m_absModel( true, this, tr("Abs Value") ),
	m_amountMultModel( 1.0, 0, 32, 0.2, this, tr("Amount Multiplicator") )
{
}




void PeakControllerEffectControls::loadSettings( const QDomElement & _this )
{
	m_baseModel.loadSettings( _this, "base" );
	m_amountModel.loadSettings( _this, "amount" );
	m_muteModel.loadSettings( _this, "mute" );

	m_attackModel.loadSettings( _this, "attack" );
	m_decayModel.loadSettings( _this, "decay" );

	m_absModel.loadSettings( _this, "abs" );
	m_amountMultModel.loadSettings( _this, "amountmult" );

	/*If the peak controller effect is NOT loaded from project,
	 * m_effectId stored is useless.
	 * Reason to assign a random number to it:
	 * If the user clone the instrument, and m_effectId is cloned, and
	 * m_effectId is copied, then there would be two instruments
	 * having the same id.
	 */
	if( engine::getSong()->isLoadingProject() == true )
	{
		m_effect->m_effectId = _this.attribute( "effectId" ).toInt();
	}
	else
	{
		m_effect->m_effectId = rand();
	}

	if( m_effect->m_autoController && PresetPreviewPlayHandle::isPreviewing() == true )
	{
		delete m_effect->m_autoController;
		m_effect->m_autoController = 0;
	}
}




void PeakControllerEffectControls::saveSettings( QDomDocument & _doc, 
							QDomElement & _this )
{
	_this.setAttribute( "effectId", m_effect->m_effectId );

	m_baseModel.saveSettings( _doc, _this, "base" );
	m_amountModel.saveSettings( _doc, _this, "amount" );
	m_muteModel.saveSettings( _doc, _this, "mute" );

	m_attackModel.saveSettings( _doc, _this, "attack" );
	m_decayModel.saveSettings( _doc, _this, "decay" );

	m_absModel.saveSettings( _doc,  _this, "abs" );
	m_amountMultModel.saveSettings( _doc, _this, "amountmult" );
}



#include "moc_peak_controller_effect_controls.cxx"

