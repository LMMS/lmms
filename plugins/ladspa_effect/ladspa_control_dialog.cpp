/*
 * ladspa_control_dialog.cpp - dialog for displaying and editing control port
 *                             values for LADSPA plugins
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
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


#include <QtGui/QGroupBox>
#include <QtGui/QLayout>

#include "ladspa_effect.h"
#include "ladspa_control_dialog.h"
#include "ladspa_control_view.h"
#include "led_checkbox.h"



ladspaControlDialog::ladspaControlDialog( ladspaControls * _ctl ) :
	effectControlDialog( _ctl )
{
	QVBoxLayout * mainLay = new QVBoxLayout( this );
	QHBoxLayout * effectLay = new QHBoxLayout();
	mainLay->addLayout( effectLay );

	int rows = static_cast<int>( sqrt( 
		static_cast<double>( _ctl->m_controlCount ) ) );
	
	for( ch_cnt_t proc = 0; proc < _ctl->m_processors; proc++ )
	{
		control_list_t & controls = _ctl->m_controls[proc];
		int row_cnt = 0;
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
		grouper->setAlignment( Qt::Vertical );

		for( control_list_t::iterator it = controls.begin(); 
						it != controls.end(); it++ )
		{
			if( (*it)->getPort()->proc == proc )
			{
				if( last_port == NONE || 
					(*it)->getPort()->data_type != TOGGLED || 
					( (*it)->getPort()->data_type == TOGGLED && 
					last_port == TOGGLED ) )
				{
					new ladspaControlView( grouper, *it );
				}
				else
				{
					while( row_cnt < rows )
					{
						new QWidget( grouper );
						row_cnt++;
					}
					new ladspaControlView( grouper, *it );
					row_cnt = 0;
				}

				row_cnt++;
				if( row_cnt == ( rows - 1 ) )
				{
					row_cnt = 0;
				}
				last_port = (*it)->getPort()->data_type;
			}
		}

		effectLay->addWidget( grouper );
	}

	if( _ctl->m_processors > 1 )
	{
		mainLay->addSpacing( 3 );
		QHBoxLayout * center = new QHBoxLayout();
		mainLay->addLayout( center );
		ledCheckBox * stereoLink = new ledCheckBox(
						tr( "Link Channels" ), this );
		stereoLink->setModel( &_ctl->m_stereoLinkModel );
		center->addWidget( stereoLink );
	}
}




ladspaControlDialog::~ladspaControlDialog()
{
}


