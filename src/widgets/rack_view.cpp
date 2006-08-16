#ifndef SINGLE_SOURCE_COMPILE

/*
 * right_frame.cpp - provides the display for the rackInsert instances
 *
 * Copyright (c) 2006 Danny McRae <khjklujn@netscape.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "ladspa_manager.h"
#ifdef LADSPA_SUPPORT

#ifdef QT4

#include <QtGui/QMessageBox>

#else

#include <qmessagebox.h>

#endif


#include "rack_view.h"


rackView::rackView( QWidget * _parent, 
			engine * _engine, 
			track * _track, 
			audioPort * _port ):
	QWidget( _parent, "rackView" ),
	journallingObject( _engine ),
	m_track( _track ),
	m_port( _port ),
	m_ladspa( eng()->getLADSPAManager() )
{
	setFixedSize( 230, 184 );
	
	m_mainLayout = new QVBoxLayout( this );
	m_scrollView = new QScrollView( this );
	m_scrollView->setFixedSize( 230, 184 );
	m_scrollView->setVScrollBarMode( QScrollView::AlwaysOn );
	m_mainLayout->addWidget( m_scrollView );
	
	m_lastY = 0;
}



rackView::~rackView()
{
}




void rackView::addPlugin( ladspa_key_t _key )
{
	rackPlugin * plugin = new rackPlugin( m_scrollView->viewport(),
						_key, m_track, eng(), m_port );
	connect( plugin, SIGNAL( moveUp( rackPlugin * ) ), 
				this, SLOT( moveUp( rackPlugin * ) ) );
	connect( plugin, SIGNAL( moveDown( rackPlugin * ) ),
				this, SLOT( moveDown( rackPlugin * ) ) );
	connect( plugin, SIGNAL( deletePlugin( rackPlugin * ) ),
				this, SLOT( deletePlugin( rackPlugin * ) ) );
	m_scrollView->addChild( plugin );
	m_scrollView->moveChild( plugin, 0, m_lastY );
	plugin->show();
	m_lastY += plugin->height();
	m_scrollView->resizeContents( 210, m_lastY );
	m_rackInserts.append( plugin );
}




void rackView::moveUp( rackPlugin * _plugin )
{
	if( _plugin != m_rackInserts.first() )
	{
		int i = 0;
		for( vvector<rackPlugin *>::iterator it = 
						m_rackInserts.begin(); 
					it != m_rackInserts.end(); it++, i++ )
		{
			if( (*it) == _plugin )
			{
				break;
			}
		}
		
		rackPlugin * temp = m_rackInserts[ i - 1 ];
		
		m_rackInserts[i - 1] = _plugin;
		m_rackInserts[i] = temp;
		
		redraw();
	}
}




void rackView::moveDown( rackPlugin * _plugin )
{
	m_port->getEffects()->moveDown( _plugin->getEffect() );
	if( _plugin != m_rackInserts.last() )
	{
		int i = 0;
		for( vvector<rackPlugin *>::iterator it = 
						m_rackInserts.begin(); 
					it != m_rackInserts.end(); it++, i++ )
		{
			if( (*it) == _plugin )
			{
				break;
			}
		}
		
		rackPlugin * temp = m_rackInserts.at( i + 1 );
		
		m_rackInserts[i + 1] = _plugin;
		m_rackInserts[i] = temp;
		
		redraw();
	}
}




void rackView::deletePlugin( rackPlugin * _plugin )
{
	m_port->getEffects()->deleteEffect( _plugin->getEffect() );
	
	m_scrollView->removeChild( _plugin );
	
	vvector<rackPlugin *>::iterator loc = NULL;
	for( vvector<rackPlugin *>::iterator it = m_rackInserts.begin(); 
					it != m_rackInserts.end(); it++ )
	{
		if( (*it) == _plugin )
		{
			loc = it;
			break;
		}
	}
	
	if( loc != NULL )
	{
		delete _plugin;
		m_rackInserts.erase( loc );
		redraw();	
	}
}




void rackView::redraw()
{
	m_lastY = 0;
	for( vvector<rackPlugin *>::iterator it = m_rackInserts.begin(); 
					it != m_rackInserts.end(); it++ )
	{
		m_scrollView->moveChild( (*it), 0, m_lastY );
		m_lastY += (*it)->height();
	}
	m_scrollView->resizeContents( 210, m_lastY );
}	




void FASTCALL rackView::saveSettings( QDomDocument & _doc, 
							QDomElement & _this )
{
	int num = 0;
	_this.setAttribute( "plugins", m_rackInserts.count() );
	for( vvector<rackPlugin *>::iterator it = m_rackInserts.begin(); 
					it != m_rackInserts.end(); it++ )
	{
		ladspa_key_t key = (*it)->getKey();
		_this.setAttribute( "label" + QString::number(num), 
								key.first );
		_this.setAttribute( "lib" + QString::number(num), key.second );
		_this.setAttribute( "name" + QString::number(num), 
						m_ladspa->getName( key ) );
		_this.setAttribute( "maker" + QString::number(num), 
						m_ladspa->getMaker( key ) );
		(*it)->saveState( _doc, _this );
		num++;
	}
}




void FASTCALL rackView::loadSettings( const QDomElement & _this )
{
	int plugin_cnt = _this.attribute( "plugins" ).toInt();
	
	QDomNode node = _this.firstChild();
	for( int i = 0; i < plugin_cnt; i++ )
	{
		QString lib = _this.attribute( "lib" + QString::number( i ) );
		QString label = _this.attribute( "label" + 
							QString::number( i ) );
		ladspa_key_t key( label, lib );
		if( m_ladspa->getDescriptor( key ) != NULL )
		{
			addPlugin( key );
			
			if( node.isElement() )
			{
				if( m_rackInserts.last()->nodeName() == 
							node.nodeName() )
				{
					m_rackInserts.last()->restoreState( 
							node.toElement() );
				}
			}
		}
		else
		{
			QString name = _this.attribute( "name" + 
						QString::number( i ) );
			QString maker = _this.attribute( "maker" + 
						QString::number( i ) );
			QString message = "Couldn't find " + name + 
						" from:\n\n";
			message += "Library: " + lib + "\n";
			message += "Label: " + label + "\n";
			message += "Maker: " + maker;
			
			QMessageBox::information( 0, tr( "Uknown plugin" ), 
						message, QMessageBox::Ok );
		}
		node = node.nextSibling();
	}
	
}



#include "rack_view.moc"

#endif

#endif
