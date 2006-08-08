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

#include "rack_view.h"


rackView::rackView( QWidget * _parent, engine * _engine, instrumentTrack * _track ):
	QWidget( _parent, "rackView" ),
	engineObject( _engine ),
	m_instrumentTrack( _track )
{
	setFixedSize( 230, 180 );
	
	m_mainLayout = new QVBoxLayout( this );
	m_scrollView = new QScrollView( this );
	m_scrollView->setFixedSize( 230, 180 );
	m_scrollView->setVScrollBarMode( QScrollView::AlwaysOn );
	m_mainLayout->addWidget( m_scrollView );
	
	m_rack = new QVBox( m_scrollView->viewport() );
	m_scrollView->addChild( m_rack );
	
	m_rackInserts.setAutoDelete( TRUE );
}



rackView::~rackView()
{
	m_rackInserts.setAutoDelete( TRUE );
	while( ! m_rackInserts.isEmpty() )
	{
		m_rackInserts.removeFirst();
	}	
	
	delete m_rack;
	delete m_scrollView;
	delete m_mainLayout;
}




void rackView::addPlugin( ladspa_key_t _key )
{
	rackPlugin * plugin = new rackPlugin( m_rack, _key, m_instrumentTrack, eng() );
	m_rackInserts.append( plugin );
	m_rackInserts.current()->repaint();
	m_rackInserts.current()->show();	
}


#include "rack_view.moc"

#endif

#endif
