/*
 * LadspaControlView.cpp - widget for controlling a LADSPA port
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QHBoxLayout>

#include "LadspaControl.h"
#include "LadspaControlView.h"
#include "LadspaBase.h"
#include "LedCheckBox.h"
#include "TempoSyncKnob.h"


namespace lmms::gui
{

LadspaControlView::LadspaControlView( QWidget * _parent,
						LadspaControl * _ctl ) :
	QWidget( _parent ),
	ModelView( _ctl, this ),
	m_ctl( _ctl )
{
	auto layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing( 0 );

	LedCheckBox * link = nullptr;

	if( m_ctl->m_link )
	{
		link = new LedCheckBox( "", this );
		link->setModel( &m_ctl->m_linkEnabledModel );
		link->setToolTip(tr("Link channels"));
		layout->addWidget( link );
	}

	Knob * knb = nullptr;

	switch( m_ctl->port()->data_type )
	{
		case BufferDataType::Toggled:
		{
			auto toggle = new LedCheckBox(m_ctl->port()->name, this, QString(), LedCheckBox::LedColor::Green);
			toggle->setModel( m_ctl->toggledModel() );
			layout->addWidget( toggle );
			if( link != nullptr )
			{
				setFixedSize( link->width() + toggle->width(),
							toggle->height() );
			}
			else
			{
				setFixedSize( toggle->width(),
							toggle->height() );
			}
			break;
		}

		case BufferDataType::Integer:
		case BufferDataType::Enum:
		case BufferDataType::Floating:
			knb = new Knob(KnobType::Bright26, m_ctl->port()->name, this, Knob::LabelRendering::WidgetFont, m_ctl->port()->name);
			break;

		case BufferDataType::Time:
			knb = new TempoSyncKnob(KnobType::Bright26, m_ctl->port()->name, this, Knob::LabelRendering::WidgetFont, m_ctl->port()->name);
			break;

		default:
			break;
	}

	if( knb != nullptr )
	{
		if( m_ctl->port()->data_type != BufferDataType::Time )
		{
			knb->setModel( m_ctl->knobModel() );
		}
		else
		{
			knb->setModel( m_ctl->tempoSyncKnobModel() );
		}
		knb->setHintText( tr( "Value:" ), "" );
		layout->addWidget( knb );
		if( link != nullptr )
		{
			setFixedSize( link->width() + knb->width(),
						knb->height() );
		}
		else
		{
			setFixedSize( knb->width(), knb->height() );
		}
	}
}




} // namespace lmms::gui
