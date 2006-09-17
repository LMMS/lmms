#ifndef SINGLE_SOURCE_COMPILE

/*
 * ladspa_control_dialog.cpp - dialog for displaying and editing control port
 *                             values for LADSPA plugins
 *
 * Copyright (c) 2006 Danny McRae <khjklujn/at/users.sourceforge.net>
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


#ifdef QT4

#include <QtGui/QMessageBox>

#else

#include <qmessagebox.h>

#endif

#include "ladspa_effect.h"


ladspaControlDialog::ladspaControlDialog( QWidget * _parent, 
						ladspaEffect * _eff, 
						track * _track ) :
	effectControlDialog( _parent, _eff ),
	m_effect( _eff ),
	m_processors( _eff->getProcessorCount() ),
	m_track( _track ),
	m_noLink( FALSE )
{
	m_mainLay = new QVBoxLayout( this );
#ifdef QT3
	m_effectLay = new QHBoxLayout( m_mainLay );
#else
	m_effectLay = new QHBoxLayout();
	m_mainLay->addLayout( m_effectLay );
#endif
	
	multi_proc_t controls = m_effect->getControls();
	m_controlCount = controls.count();
	
	int rows = static_cast<int>( sqrt( 
		static_cast<double>( m_controlCount ) ) );
	
	for( ch_cnt_t proc = 0; proc < m_processors; proc++ )
	{
		control_list_t p;
		
		bool linked_control = FALSE;
		int row_cnt = 0;
		buffer_data_t last_port = NONE;
		
		QGroupBox * grouper;
		if( m_processors > 1 )
		{
#ifdef QT3
			grouper = new QGroupBox( rows, Qt::Vertical, 
				tr( "Channel " ) + QString::number( proc + 1 ),
				this );
#else
			grouper = new QGroupBox( tr( "Channel " ) +
						QString::number( proc + 1 ),
									this );
			grouper->setAlignment( Qt::Vertical );
#endif
			if( proc == 0 )
			{
				linked_control = TRUE;
			}
		}
		else
		{
#ifdef QT3
			grouper = new QGroupBox( rows, Qt::Vertical, 
								"", this );
#else
			grouper = new QGroupBox( this );
			grouper->setAlignment( Qt::Vertical );
#endif
		}
		
		for( multi_proc_t::iterator it = controls.begin(); 
						it != controls.end(); it++ )
		{
			if( (*it)->proc == proc )
			{
				if( last_port == NONE || 
					(*it)->data_type != TOGGLED || 
					( (*it)->data_type == TOGGLED && 
					last_port == TOGGLED ) )
				{
					(*it)->control = 
						new ladspaControl( 
							grouper, (*it), 
							eng(), m_track,
							linked_control );
				}
				else
				{
					while( row_cnt < rows )
					{
						m_blanks.append( 
						new QWidget( grouper ) );
						row_cnt++;
					}
					(*it)->control = new ladspaControl( 
								grouper, (*it),
								eng(), 
								m_track,
							linked_control );
					row_cnt = 0;
				}
				
				row_cnt++;
				if( row_cnt == ( rows - 1 ) )
				{
					row_cnt = 0;
				}
				last_port = (*it)->data_type;
				
				p.append( (*it)->control );
				
				if( linked_control )
				{
					connect( (*it)->control, 
					SIGNAL( linkChanged( Uint16, bool ) ),
						this,
					SLOT( linkPort( Uint16, bool ) ) );
				}
			}
		}
		
		m_controls.append( p );

		m_effectLay->addWidget( grouper );
	}
	if( m_processors > 1 )
	{
		m_mainLay->addSpacing( 3 );
#ifdef QT3
		QHBoxLayout * center = new QHBoxLayout( m_mainLay );
#else
		QHBoxLayout * center = new QHBoxLayout();
		m_mainLay->addLayout( center );
#endif
		m_stereoLink = new ledCheckBox( tr( "Link Channels" ), 
					this, "", eng(), m_track );
		connect( m_stereoLink, SIGNAL( toggled( bool ) ), 
					this, SLOT( link( bool ) ) );
		m_stereoLink->setChecked( TRUE );
		center->addWidget( m_stereoLink );
	}
}




ladspaControlDialog::~ladspaControlDialog()
{
	for( ch_cnt_t proc = 0; proc < m_processors; proc++ )
	{
		m_controls[proc].clear();
	}
	m_controls.clear();
}




void FASTCALL ladspaControlDialog::saveSettings( QDomDocument & _doc, 
							QDomElement & _this )
{
	if( m_processors > 1 )
	{
		_this.setAttribute( "link", m_stereoLink->isChecked() );
	}
	
	multi_proc_t controls = m_effect->getControls();
	_this.setAttribute( "ports", controls.count() );
	for( multi_proc_t::iterator it = controls.begin(); 
						it != controls.end(); it++ )
	{
		QString n = "port" + QString::number( (*it)->proc ) + 
					QString::number( (*it)->port_id );
		(*it)->control->saveSettings( _doc, _this, n );
	}
}




void FASTCALL ladspaControlDialog::loadSettings( const QDomElement & _this )
{
	if( m_processors > 1 )
	{
		m_stereoLink->setChecked( _this.attribute( "link" ).toInt() );
	}
	
	multi_proc_t controls = m_effect->getControls();
	for( multi_proc_t::iterator it = controls.begin(); 
						it != controls.end(); it++ )
	{
		QString n = "port" + QString::number( (*it)->proc ) + 
					QString::number( (*it)->port_id );
		(*it)->control->loadSettings( _this, n );
	}
}




void ladspaControlDialog::linkPort( Uint16 _port, bool _state )
{
	ladspaControl * first = m_controls[0][_port];
	if( _state )
	{
		for( ch_cnt_t proc = 1; proc < m_processors; proc++ )
		{
			first->linkControls( m_controls[proc][_port] );
		}
	}
	else
	{
		for( ch_cnt_t proc = 1; proc < m_processors; proc++ )
		{
			first->unlinkControls( m_controls[proc][_port] );
		}
		m_noLink = TRUE;
		m_stereoLink->setChecked( FALSE );
	}
}



void ladspaControlDialog::link( bool _state )
{
	if( _state )
	{
		for( Uint16 port = 0; 
			port < m_controlCount / m_processors;
			port++ )
		{
			m_controls[0][port]->setLink( TRUE );
		}
	}
	else if( !m_noLink )
	{
		for( Uint16 port = 0; 
			port < m_controlCount / m_processors;
			port++ )
		{
			m_controls[0][port]->setLink( FALSE );
		}
	}
	else
	{
		m_noLink = FALSE;
	}
}


#include "ladspa_control_dialog.moc"

#endif

