/*
 * LadspaControlDialog.cpp - dialog for displaying and editing control port
 *                             values for LADSPA plugins
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

#include <cmath>

#include <QHBoxLayout>
#include <QGroupBox>
#include <QVBoxLayout>

#include "LadspaBase.h"
#include "LadspaControl.h"
#include "LadspaControls.h"
#include "LadspaControlDialog.h"
#include "LadspaControlView.h"
#include "LedCheckBox.h"

namespace lmms::gui
{


LadspaControlDialog::LadspaControlDialog( LadspaControls * _ctl ) :
	EffectControlDialog( _ctl ),
	m_effectLayout( nullptr ),
	m_stereoLink( nullptr )
{
	auto mainLay = new QVBoxLayout(this);

	m_effectLayout = new QHBoxLayout();
	mainLay->addLayout( m_effectLayout );

	updateEffectView( _ctl );

	if( _ctl->m_processors > 1 )
	{
		mainLay->addSpacing( 3 );
		auto center = new QHBoxLayout();
		mainLay->addLayout( center );
		m_stereoLink = new LedCheckBox( tr( "Link Channels" ), this );
		m_stereoLink->setModel( &_ctl->m_stereoLinkModel );
		center->addWidget( m_stereoLink );
	}
}




void LadspaControlDialog::updateEffectView( LadspaControls * _ctl )
{
	QList<QGroupBox *> groupBoxes = findChildren<QGroupBox *>();
	for (const auto& groupBox : groupBoxes)
	{
		delete groupBox;
	}

	m_effectControls = _ctl;


	const int cols = static_cast<int>( sqrt( 
		static_cast<double>( _ctl->m_controlCount /
						_ctl->m_processors ) ) );
	for( ch_cnt_t proc = 0; proc < _ctl->m_processors; proc++ )
	{
		control_list_t & controls = _ctl->m_controls[proc];
		int row = 0;
		int col = 0;
		BufferDataType last_port = BufferDataType::None;

		QGroupBox * grouper;
		if( _ctl->m_processors > 1 )
		{
			grouper = new QGroupBox( tr( "Channel " ) +
						QString::number( proc + 1 ),
								this );
		}
		else
		{
			grouper = new QGroupBox( this );
		}

		auto gl = new QGridLayout(grouper);
		grouper->setLayout( gl );
		grouper->setAlignment( Qt::Vertical );

		for (const auto& control : controls)
		{
			if (control->port()->proc == proc)
			{
				BufferDataType this_port = control->port()->data_type;
				if( last_port != BufferDataType::None &&
					( this_port == BufferDataType::Toggled || this_port == BufferDataType::Enum ) &&
					( last_port != BufferDataType::Toggled && last_port != BufferDataType::Enum ) )
				{
					++row;
					col = 0;
				}
				gl->addWidget(new LadspaControlView(grouper, control), row, col);
				if( ++col == cols )
				{
					++row;
					col = 0;
				}
				last_port = control->port()->data_type;
			}
		}

		m_effectLayout->addWidget( grouper );
	}

	if( _ctl->m_processors > 1 && m_stereoLink != nullptr )
	{
		m_stereoLink->setModel( &_ctl->m_stereoLinkModel );
	}

	connect( _ctl, SIGNAL( effectModelChanged( lmms::LadspaControls * ) ),
				this, SLOT( updateEffectView( lmms::LadspaControls * ) ),
							Qt::DirectConnection );
}


} // namespace lmms::gui
