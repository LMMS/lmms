/*
 * stereoenhancer_control_dialog.cpp - control-dialog for stereoenhancer-effect
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


#include <qlayout.h>

#include "stereo_enhancer.h"
#include "knob.h"



stereoEnhancerControlDialog::stereoEnhancerControlDialog( QWidget * _parent,
						stereoEnhancerEffect * _eff ) :
		effectControlDialog( _parent, _eff ),
		m_effect( _eff )
{
	QHBoxLayout * l = new QHBoxLayout( this );

	m_widthKnob = new knob( knobBright_26, this, tr( "Width" ), NULL );
	m_widthKnob->setRange( 0.0f, 180.0f, 1.0f );
	m_widthKnob->setInitValue( 0.0f );
	m_widthKnob->setLabel( tr( "WIDE" ) );
	m_widthKnob->setHintText( tr( "Width:" ) + " ", "samples" );
	connect( m_widthKnob, SIGNAL( valueChanged( float ) ),
			this, SLOT( changeWideCoeff( void ) ) );

	l->addWidget( m_widthKnob );

	changeWideCoeff();
}




void stereoEnhancerControlDialog::changeWideCoeff( void )
{
	m_effect->m_seFX.setWideCoeff( m_widthKnob->value() );
}



void FASTCALL stereoEnhancerControlDialog::loadSettings(
						const QDomElement & _this )
{
	m_widthKnob->setValue( _this.attribute( "width" ).toFloat() );
}




void FASTCALL stereoEnhancerControlDialog::saveSettings( QDomDocument & _doc, 
							QDomElement & _this )
{
	_this.setAttribute( "width", m_widthKnob->value() );
}



#include "stereoenhancer_control_dialog.moc"

