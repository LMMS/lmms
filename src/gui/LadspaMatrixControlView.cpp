/*
 * LadspaMatrixControlView.cpp - widget for controlling a LADSPA port
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2015 Michael Gregorius <michaelgregorius/at/web[dot]de>
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

#include "LadspaMatrixControlView.h"

#include "LadspaControl.h"

#include "LadspaBase.h"
#include "LedCheckBox.h"
#include "TempoSyncKnob.h"

#include <QLayout>


namespace lmms::gui
{

LadspaMatrixControlView::LadspaMatrixControlView( QWidget * parent,
						LadspaControl * ladspaControl ) :
	QWidget( parent ),
	ModelView( ladspaControl, this ),
	m_ladspaControl( ladspaControl )
{
	QHBoxLayout * layout = new QHBoxLayout( this );
	layout->setMargin( 0 );
	layout->setSpacing( 0 );

	Knob * knob = NULL;

	buffer_data_t dataType = m_ladspaControl->port()->data_type;
	switch( dataType )
	{
		case TOGGLED:
		{
			LedCheckBox * toggle = new LedCheckBox(
				"", this, QString::null, LedCheckBox::Green );
			toggle->setModel( m_ladspaControl->toggledModel() );
			layout->addWidget( toggle );
			setFixedSize( toggle->width(), toggle->height() );
			break;
		}

		case INTEGER:
		case FLOATING:
			knob = new Knob( knobBright_26, this, m_ladspaControl->port()->name );
			knob->setModel( m_ladspaControl->knobModel() );
			break;

		case TIME:
			knob = new TempoSyncKnob( knobBright_26, this, m_ladspaControl->port()->name );
			knob->setModel( m_ladspaControl->tempoSyncKnobModel() );
			break;

		default:
			break;
	}

	if( knob != NULL )
	{
		knob->setHintText( tr( "Value:" ), "" );
		knob->setWhatsThis( tr( "Sorry, no help available." ) );
		layout->addWidget( knob );
		setFixedSize( knob->width(), knob->height() );
	}
}




LadspaMatrixControlView::~LadspaMatrixControlView()
{
}

} // namespace lmms::gui
