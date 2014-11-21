/*
 * stereoenhancer_control_dialog.cpp - control-dialog for stereoenhancer-effect
 *
 * Copyright (c) 2006-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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



#include <QtGui/QLayout>

#include "stereoenhancer_control_dialog.h"
#include "stereoenhancer_controls.h"



stereoEnhancerControlDialog::stereoEnhancerControlDialog(
	stereoEnhancerControls * _controls ) :
	EffectControlDialog( _controls )
{
	QHBoxLayout * l = new QHBoxLayout( this );

	knob * widthKnob = new knob( knobBright_26, this );
	widthKnob->setModel( &_controls->m_widthModel );
	widthKnob->setLabel( tr( "WIDE" ) );
	widthKnob->setHintText( tr( "Width:" ) + " ", "samples" );

	l->addWidget( widthKnob );

	this->setLayout(l);
}

#include "moc_stereoenhancer_control_dialog.cxx"
