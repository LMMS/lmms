/*
 * bassbooster_control_dialog.cpp - control-dialog for bassbooster-effect
 *
 * Copyright (c) 2006-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef QT3

#include <QtGui/QLayout>

#else

#include <qlayout.h>

#endif

#include "bass_booster.h"
#include "knob.h"



bassBoosterControlDialog::bassBoosterControlDialog( QWidget * _parent,
						bassBoosterEffect * _eff ) :
		effectControlDialog( _parent, _eff ),
		m_effect( _eff )
{
	QHBoxLayout * l = new QHBoxLayout( this );

	m_freqKnob = new knob( knobBright_26, this, tr( "Frequency" ), NULL );
	m_freqKnob->setRange( 50.0f, 200.0f, 1.0f );
	m_freqKnob->setInitValue( 100.0f );
	m_freqKnob->setLabel( tr( "FREQ" ) );
	m_freqKnob->setHintText( tr( "Frequency:" ) + " ", "Hz" );
	connect( m_freqKnob, SIGNAL( valueChanged( float ) ),
			this, SLOT( changeFrequency( void ) ) );

	m_gainKnob = new knob( knobBright_26, this, tr( "Gain" ), NULL );
	m_gainKnob->setRange( 0.1f, 5.0f, 0.1f );
	m_gainKnob->setInitValue( 1.0f );
	m_gainKnob->setLabel( tr( "GAIN" ) );
	m_gainKnob->setHintText( tr( "Gain:" ) + " ", "" );
	connect( m_gainKnob, SIGNAL( valueChanged( float ) ),
			this, SLOT( changeGain( void ) ) );

	m_ratioKnob = new knob( knobBright_26, this, tr( "Ratio" ), NULL );
	m_ratioKnob->setRange( 0.1f, 10.0f, 0.1f );
	m_ratioKnob->setInitValue( 2.0f );
	m_ratioKnob->setLabel( tr( "RATIO" ) );
	m_ratioKnob->setHintText( tr( "Ratio:" ) + " ", "" );
	connect( m_ratioKnob, SIGNAL( valueChanged( float ) ),
			this, SLOT( changeRatio( void ) ) );

	l->addWidget( m_freqKnob );
	l->addWidget( m_gainKnob );
	l->addWidget( m_ratioKnob );

	changeFrequency();
	changeGain();
	changeRatio();
}




void bassBoosterControlDialog::changeFrequency( void )
{
	m_effect->m_bbFX.leftFX().setFrequency( m_freqKnob->value() );
	m_effect->m_bbFX.rightFX().setFrequency( m_freqKnob->value() );
}




void bassBoosterControlDialog::changeGain( void )
{
	m_effect->m_bbFX.leftFX().setGain( m_gainKnob->value() );
	m_effect->m_bbFX.rightFX().setGain( m_gainKnob->value() );
}




void bassBoosterControlDialog::changeRatio( void )
{
	m_effect->m_bbFX.leftFX().setRatio( m_ratioKnob->value() );
	m_effect->m_bbFX.rightFX().setRatio( m_ratioKnob->value() );
}




/*
void bassBoosterControlDialog::updateEffect( void )
{
	//m_effect->m_bbFX = effectLib::bassBoost<>( m_freqKnob->value(),
	//			m_gainKnob->value(), m_ratioKnob->value() );
	m_effect->m_bbFX = effectLib::monoToStereoAdaptor<
			effectLib::bassBoost<> >(
		effectLib::bassBoost<>( m_freqKnob->value(),
				m_gainKnob->value(), m_ratioKnob->value(),
						m_effect->m_bbFX.leftFX() ),
		effectLib::bassBoost<>( m_freqKnob->value(),
				m_gainKnob->value(), m_ratioKnob->value(),
						m_effect->m_bbFX.rightFX() )
				);
	m_effect->m_bbFX.leftFX().setSelectivity( m_freqKnob->value() );
	m_effect->m_bbFX.rightFX().setSelectivity( m_freqKnob->value() );
	m_effect->m_bbFX.leftFX().setGain( m_gainKnob->value() );
	m_effect->m_bbFX.rightFX().setGain( m_gainKnob->value() );
	m_effect->m_bbFX.leftFX().setRatio( m_ratioKnob->value() );
	m_effect->m_bbFX.rightFX().setRatio( m_ratioKnob->value() );
}
*/



void FASTCALL bassBoosterControlDialog::loadSettings(
						const QDomElement & _this )
{
	m_freqKnob->setValue( _this.attribute( "freq" ).toFloat() );
	m_gainKnob->setValue( _this.attribute( "gain" ).toFloat() );
	m_ratioKnob->setValue( _this.attribute( "ratio" ).toFloat() );
}




void FASTCALL bassBoosterControlDialog::saveSettings( QDomDocument & _doc, 
							QDomElement & _this )
{
	_this.setAttribute( "freq", m_freqKnob->value() );
	_this.setAttribute( "gain", m_gainKnob->value() );
	_this.setAttribute( "ratio", m_ratioKnob->value() );
}



#include "bassbooster_control_dialog.moc"

