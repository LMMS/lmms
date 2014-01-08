/*
 * bassbooster_controls.cpp - controls for bassbooster-effect
 *
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

#include "bassbooster_controls.h"
#include "bass_booster.h"



bassBoosterControls::bassBoosterControls( bassBoosterEffect * _eff ) :
	EffectControls( _eff ),
	m_effect( _eff ),
	m_freqModel( 100.0f, 50.0f, 200.0f, 1.0f, this, tr( "Frequency" ) ),
	m_gainModel( 1.0f, 0.1f, 5.0f, 0.05f, this, tr( "Gain" ) ),
	m_ratioModel( 2.0f, 0.1f, 10.0f, 0.1f, this, tr( "Ratio" ) )
{
	connect( &m_freqModel, SIGNAL( dataChanged() ),
			this, SLOT( changeFrequency() ) );

	connect( &m_gainModel, SIGNAL( dataChanged() ),
			this, SLOT( changeGain() ) );

	connect( &m_ratioModel, SIGNAL( dataChanged() ),
			this, SLOT( changeRatio() ) );

	connect( engine::mixer(), SIGNAL( sampleRateChanged() ),
			this, SLOT( changeFrequency() ) );
	changeFrequency();
	changeGain();
	changeRatio();
}




void bassBoosterControls::changeFrequency()
{
	const sample_t fac = engine::mixer()->processingSampleRate() /
								44100.0f;
	m_effect->m_bbFX.leftFX().setFrequency( m_freqModel.value() * fac );
	m_effect->m_bbFX.rightFX().setFrequency( m_freqModel.value() * fac );
}




void bassBoosterControls::changeGain()
{
	m_effect->m_bbFX.leftFX().setGain( m_gainModel.value() );
	m_effect->m_bbFX.rightFX().setGain( m_gainModel.value() );
}




void bassBoosterControls::changeRatio()
{
	m_effect->m_bbFX.leftFX().setRatio( m_ratioModel.value() );
	m_effect->m_bbFX.rightFX().setRatio( m_ratioModel.value() );
}




void bassBoosterControls::loadSettings( const QDomElement & _this )
{
	m_freqModel.setValue( _this.attribute( "freq" ).toFloat() );
	m_gainModel.setValue( _this.attribute( "gain" ).toFloat() );
	m_ratioModel.setValue( _this.attribute( "ratio" ).toFloat() );
}




void bassBoosterControls::saveSettings( QDomDocument & _doc, 
							QDomElement & _this )
{
	_this.setAttribute( "freq", m_freqModel.value() );
	_this.setAttribute( "gain", m_gainModel.value() );
	_this.setAttribute( "ratio", m_ratioModel.value() );
}



#include "moc_bassbooster_controls.cxx"

