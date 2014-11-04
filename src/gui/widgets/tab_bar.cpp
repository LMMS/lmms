/*
 * tab_bar.cpp - implementation of tab-bar
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "tab_bar.h"
#include "tab_button.h"
#include "gui_templates.h"
#include "tooltip.h"



tabBar::tabBar( QWidget * _parent, QBoxLayout::Direction _dir ) :
	QWidget( _parent ),
	m_layout( new QBoxLayout( _dir, this ) ),
	m_exclusive( false )
{
	m_layout->setMargin( 8 );
	m_layout->setSpacing( 0 );

	setLayout( m_layout );
}




tabBar::~tabBar()
{
}




tabButton * tabBar::addTab( QWidget * _w, const QString & _text, int _id,
				bool _add_stretch, bool _text_is_tooltip )
{
	// already tab with id?
	if( m_tabs.contains( _id ) )
	{
		// then remove it
		removeTab( _id );
	}
	QString caption = ( _text_is_tooltip ) ? QString( "" ) : _text;
	// create tab-button
	tabButton * b = new tabButton( caption, _id, this );
	connect( b, SIGNAL( clicked( int ) ), this, SLOT( tabClicked( int ) ) );
	b->setIconSize( QSize( 48, 48 ) );
	b->setFixedSize( 64, 64 );
	b->show();
	if( _text_is_tooltip )
	{
		toolTip::add( b, _text );
	}

	// small workaround, because QBoxLayout::addWidget(...) doesn't
	// work properly, so we first have to remove all tabs from the
	// layout and them add them in the correct order
	QMap<int, QPair<tabButton *, QWidget *> >::iterator it;
	for( it = m_tabs.begin(); it != m_tabs.end(); ++it )
	{
		m_layout->removeWidget( it.value().first );
	}
	m_tabs.insert( _id, qMakePair( b, _w ) );
	for( it = m_tabs.begin(); it != m_tabs.end(); ++it )
	{
		m_layout->addWidget( it.value().first );
	}

	if( _add_stretch )
	{
		m_layout->addStretch();
	}


	// we assume, parent-widget is a widget acting as widget-stack so all
	// widgets have the same size and only the one on the top is visible
	_w->setFixedSize( _w->parentWidget()->size() );

	b->setFont( pointSize<8>( b->font() ) );

	return( b );
}




void tabBar::removeTab( int _id )
{
	// find tab-button and delete it
	if( m_tabs.find( _id ) != m_tabs.end() )
	{
		delete m_tabs[_id].first;
		m_tabs.erase( m_tabs.find( _id ) );
	}
}




void tabBar::setActiveTab( int _id )
{
	setTabState( _id, true );
	hideAll( _id );
	if( allHidden() )
	{
		emit allWidgetsHidden();
	}
	else
	{
		emit widgetShown();
	}
}




int tabBar::activeTab()
{
	QMap<int, QPair<tabButton *, QWidget *> >::iterator it;
	for( it = m_tabs.begin(); it != m_tabs.end(); ++it )
	{
		if( tabState( it.key() ) == true )
		{
			return( it.key() );
		}
	}
	return( -1 );
}




bool tabBar::tabState( int _id )
{
	if( m_tabs.find( _id ) == m_tabs.end() )
	{
		return( false );
	}
	return( m_tabs[_id].first->isChecked() );
}




void tabBar::setTabState( int _id, bool _checked )
{
	if( m_tabs.find( _id ) != m_tabs.end() )
	{
		m_tabs[_id].first->setChecked( _checked );
	}
}




void tabBar::hideAll( int _exception )
{
	QMap<int, QPair<tabButton *, QWidget *> >::iterator it;
	for( it = m_tabs.begin(); it != m_tabs.end(); ++it )
	{
		if( it.key() != _exception )
		{
			setTabState( it.key(), false );
		}
		it.value().second->hide();
	}
	if( m_tabs.find( _exception ) != m_tabs.end() )
	{
		if( tabState( _exception ) )
		{
			m_tabs[_exception].second->show();
		}
		else
		{
			m_tabs[_exception].second->hide();
		}
	}
}




void tabBar::tabClicked( int _id )
{
	if( m_exclusive == true && activeTab() == -1 )
	{
		setActiveTab( _id );
	}
	else
	{
		bool all_hidden_before = allHidden();
		// disable tabbar-buttons except the one clicked
		hideAll( _id );
		bool now_hidden = allHidden();
		if( all_hidden_before == true && now_hidden == false )
		{
			emit widgetShown();
		}
		else if( all_hidden_before == false && now_hidden == true )
		{
			emit allWidgetsHidden();
		}
	}
}




bool tabBar::allHidden()
{
	QMap<int, QPair<tabButton *, QWidget *> >::iterator it;
	for( it = m_tabs.begin(); it != m_tabs.end(); ++it )
	{
		if( !it.value().second->isHidden() )
		{
			return( false );
		}
	}
	return( true );
}




#include "moc_tab_bar.cxx"
#include "moc_tab_button.cxx"


