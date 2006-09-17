#ifndef SINGLE_SOURCE_COMPILE

/*
 * rack_view.cpp - provides the display for the rackInsert instances
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


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
	QWidget( _parent ),
	journallingObject( _engine ),
	m_track( _track ),
	m_port( _port )
{
	setFixedSize( 230, 184 );
	
	m_mainLayout = new QVBoxLayout( this );
	m_mainLayout->setMargin( 0 );
	m_scrollArea = new QScrollArea( this );
	m_scrollArea->setFixedSize( 230, 184 );
#ifdef QT4
	m_scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
#else
	m_scrollArea->setVScrollBarMode( QScrollArea::AlwaysOn );
#endif
	m_mainLayout->addWidget( m_scrollArea );
	
	m_lastY = 0;
}



rackView::~rackView()
{
}




void rackView::addEffect( effect * _e )
{
#ifdef QT4
	if( !m_scrollArea->widget() )
	{
		QWidget * w = new QWidget( m_scrollArea->viewport() );
		QVBoxLayout * vb = new QVBoxLayout( w );
		w->show();
		m_scrollArea->setWidget( w );
	}
	QWidget * w = m_scrollArea->widget();
#else
	QWidget * w = m_scrollArea->viewport();
#endif
	rackPlugin * plugin = new rackPlugin( w, _e, m_track, m_port );
	connect( plugin, SIGNAL( moveUp( rackPlugin * ) ), 
				this, SLOT( moveUp( rackPlugin * ) ) );
	connect( plugin, SIGNAL( moveDown( rackPlugin * ) ),
				this, SLOT( moveDown( rackPlugin * ) ) );
	connect( plugin, SIGNAL( deletePlugin( rackPlugin * ) ),
				this, SLOT( deletePlugin( rackPlugin * ) ) );
#ifndef QT3
	plugin->move( 0, m_lastY );
#else
	m_scrollArea->addChild( plugin );
	m_scrollArea->moveChild( plugin, 0, m_lastY );
#endif
	plugin->show();
	m_lastY += plugin->height();
#ifdef QT4
	m_scrollArea->widget()->setFixedSize( 210, m_lastY );
#else
	m_scrollArea->resizeContents( 210, m_lastY );
#endif
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
			if( *it == _plugin )
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
			if( *it == _plugin )
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

#ifdef QT3
	m_scrollArea->removeChild( _plugin );
#endif
	
	vvector<rackPlugin *>::iterator loc = NULL;
	for( vvector<rackPlugin *>::iterator it = m_rackInserts.begin(); 
					it != m_rackInserts.end(); it++ )
	{
		if( *it == _plugin )
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
#ifdef QT4
		( *it )->move( 0, m_lastY );
#else
		m_scrollArea->moveChild( *it, 0, m_lastY );
#endif
		m_lastY += ( *it )->height();
	}
#ifdef QT4
	m_scrollArea->widget()->setFixedSize( 210, m_lastY );
#else
	m_scrollArea->resizeContents( 210, m_lastY );
#endif
}	




void FASTCALL rackView::saveSettings( QDomDocument & _doc, 
							QDomElement & _this )
{
	_this.setAttribute( "numofeffects", m_rackInserts.count() );
	for( vvector<rackPlugin *>::iterator it = m_rackInserts.begin(); 
					it != m_rackInserts.end(); it++ )
	{
		QDomElement ef = ( *it )->saveState( _doc, _this );
		ef.setAttribute( "name", 
				( *it )->getEffect()->getDescriptor()->name );
		ef.setAttribute( "key", 
				( *it )->getEffect()->getKey().dumpBase64() );
	}
}




void FASTCALL rackView::loadSettings( const QDomElement & _this )
{
	const int plugin_cnt = _this.attribute( "numofeffects" ).toInt();

	QDomNode node = _this.firstChild();
	for( int i = 0; i < plugin_cnt; i++ )
	{
		if( node.isElement() && node.nodeName() == "effect" )
		{
			QDomElement cn = node.toElement();
			const QString name = cn.attribute( "name" );
			effect::constructionData cd =
			{
				eng(),
				// we have this really convenient key-ctor
				// which takes a QString and decodes the
				// base64-data inside :-)
				effectKey( cn.attribute( "key" ) )
			} ;
			addEffect(effect::instantiate( name, cd ) );
			// TODO: somehow detect if effect is sub-plugin-capable
			// but couldn't load sub-plugin with requsted key
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
		node = node.nextSibling();
	}
	
}



#include "rack_view.moc"

#endif
