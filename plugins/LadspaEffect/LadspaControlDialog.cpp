/*
 * LadspaControlDialog.cpp - dialog for displaying and editing control port
 *                             values for LADSPA plugins
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtGui/QGroupBox>
#include <QtGui/QLayout>

#include "LadspaEffect.h"
#include "LadspaControlDialog.h"
#include "LadspaControlView.h"
#include "led_checkbox.h"



LadspaControlDialog::LadspaControlDialog( LadspaControls * _ctl ) :
	EffectControlDialog( _ctl ),
	m_effectLayout( NULL ),
	m_stereoLink( NULL )
{
	QVBoxLayout * mainLay = new QVBoxLayout( this );

	m_effectLayout = new QHBoxLayout();
	mainLay->addLayout( m_effectLayout );

	updateEffectView( _ctl );

	if( _ctl->m_processors > 1 )
	{
		mainLay->addSpacing( 3 );
		QHBoxLayout * center = new QHBoxLayout();
		mainLay->addLayout( center );
		m_stereoLink = new ledCheckBox( tr( "Link Channels" ), this );
		m_stereoLink->setModel( &_ctl->m_stereoLinkModel );
		center->addWidget( m_stereoLink );
	}
}




LadspaControlDialog::~LadspaControlDialog()
{
}




void LadspaControlDialog::updateEffectView( LadspaControls * _ctl )
{
	QList<QGroupBox *> list = findChildren<QGroupBox *>();
	for( QList<QGroupBox *>::iterator it = list.begin(); it != list.end();
									++it )
	{
		delete *it;
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
		buffer_data_t last_port = NONE;

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

		QGridLayout * gl = new QGridLayout( grouper );
		grouper->setLayout( gl );
		grouper->setAlignment( Qt::Vertical );

		for( control_list_t::iterator it = controls.begin(); 
						it != controls.end(); it++ )
		{
			if( (*it)->port()->proc == proc )
			{
				if( last_port != NONE &&
					(*it)->port()->data_type == TOGGLED &&
					!( (*it)->port()->data_type == TOGGLED && 
							last_port == TOGGLED ) )
				{
					++row;
					col = 0;
				}
				gl->addWidget( new LadspaControlView( grouper, *it ), row, col );
				if( ++col == cols )
				{
					++row;
					col = 0;
				}
				last_port = (*it)->port()->data_type;
			}
		}

		m_effectLayout->addWidget( grouper );
	}

	if( _ctl->m_processors > 1 && m_stereoLink != NULL )
	{
		m_stereoLink->setModel( &_ctl->m_stereoLinkModel );
	}

	connect( _ctl, SIGNAL( effectModelChanged( LadspaControls * ) ),
				this, SLOT( updateEffectView( LadspaControls * ) ),
							Qt::DirectConnection );
}


#include "moc_LadspaControlDialog.cxx"

