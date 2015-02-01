/*
 * plugin_browser.cpp - implementation of the plugin-browser
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtGui/QLabel>
#include <QtGui/QPainter>
#include <QtGui/QCursor>
#include <QtGui/QMouseEvent>
#include <QtGui/QScrollArea>

#include <algorithm> // for std::sort

#include "plugin_browser.h"
#include "embed.h"
#include "debug.h"
#include "templates.h"
#include "gui_templates.h"
#include "string_pair_drag.h"


bool pluginBefore( const Plugin::Descriptor& d1, const Plugin::Descriptor& d2 )
{
	return qstricmp( d1.displayName, d2.displayName ) < 0 ? true : false;
}




PluginBrowser::PluginBrowser( QWidget * _parent ) :
	SideBarWidget( tr( "Instrument plugins" ),
				embed::getIconPixmap( "plugins" ).transformed( QTransform().rotate( 90 ) ), _parent )
{
	setWindowTitle( tr( "Instrument browser" ) );
	m_view = new QWidget( contentParent() );
	//m_view->setFrameShape( QFrame::NoFrame );

	addContentWidget( m_view );

	QVBoxLayout * view_layout = new QVBoxLayout( m_view );
	view_layout->setMargin( 5 );
	view_layout->setSpacing( 5 );


	QLabel * hint = new QLabel( tr( "Drag an instrument "
					"into either the Song-Editor, the "
					"Beat+Bassline Editor or into an "
					"existing instrument track." ),
								m_view );
	hint->setFont( pointSize<8>( hint->font() ) );
	hint->setWordWrap( true );

	QScrollArea* scrollarea = new QScrollArea( m_view );
	PluginDescList* descList = new PluginDescList( m_view );
	scrollarea->setWidget(descList);
	scrollarea->setWidgetResizable(true);

	view_layout->addWidget(hint);
	view_layout->addWidget(scrollarea);
}




PluginBrowser::~PluginBrowser()
{
}






PluginDescList::PluginDescList(QWidget *parent) :
	QWidget(parent)
{
	QVBoxLayout* layout = new QVBoxLayout(this);

	Plugin::getDescriptorsOfAvailPlugins( m_pluginDescriptors );
	std::sort(m_pluginDescriptors.begin(), m_pluginDescriptors.end(), pluginBefore);


	for(Plugin::DescriptorList::const_iterator it = m_pluginDescriptors.constBegin();
		it != m_pluginDescriptors.constEnd(); it++)
	{
		if( it->type == Plugin::Instrument )
		{
			PluginDescWidget* p = new PluginDescWidget( *it, this );
			p->show();
			layout->addWidget(p);
		}
	}

	setLayout(layout);
	layout->addStretch();
}






PluginDescWidget::PluginDescWidget( const Plugin::Descriptor & _pd,
							QWidget * _parent ) :
	QWidget( _parent ),
	m_updateTimer( this ),
	m_pluginDescriptor( _pd ),
	m_logo( _pd.logo->pixmap() ),
	m_mouseOver( false ),
	m_targetHeight( 24 )
{
	connect( &m_updateTimer, SIGNAL( timeout() ), SLOT( updateHeight() ) );
	setFixedHeight( m_targetHeight );
	setMouseTracking( true );
	setCursor( Qt::PointingHandCursor );
}




PluginDescWidget::~PluginDescWidget()
{
}




void PluginDescWidget::paintEvent( QPaintEvent * )
{
	const QColor fill_color = m_mouseOver ? QColor( 224, 224, 224 ) :
						QColor( 192, 192, 192 );

	QPainter p( this );
	p.fillRect( rect(), fill_color );

	const int s = 16 + ( 32 * ( tLimit( height(), 24, 60 ) - 24 ) ) /
								( 60 - 24 );
	const QSize logo_size( s, s );
	QPixmap logo = m_logo.scaled( logo_size, Qt::KeepAspectRatio,
						Qt::SmoothTransformation );
	p.setPen( QColor( 64, 64, 64 ) );
	p.drawRect( 0, 0, rect().right(), rect().bottom() );
	p.drawPixmap( 4, 4, logo );

	QFont f = pointSize<8>( p.font() );
	f.setBold( true );
	p.setFont( f );
	p.drawText( 10 + logo_size.width(), 15,
					m_pluginDescriptor.displayName );

	if( height() > 24 || m_mouseOver )
	{
		f.setBold( false );
		p.setFont( pointSize<8>( f ) );
		QRect br;
		p.drawText( 10 + logo_size.width(), 20, width() - 58 - 5, 999,
							Qt::TextWordWrap,
			PluginBrowser::tr( m_pluginDescriptor.description ),
								&br );
		if( m_mouseOver )
		{
			m_targetHeight = qMax( 60, 25 + br.height() );
		}
	}

}




void PluginDescWidget::enterEvent( QEvent * _e )
{
	m_mouseOver = true;
	m_targetHeight = height() + 1;
	updateHeight();
	QWidget::enterEvent( _e );
}




void PluginDescWidget::leaveEvent( QEvent * _e )
{
	m_mouseOver = false;
	m_targetHeight = 24;
	updateHeight();
	QWidget::leaveEvent( _e );
}




void PluginDescWidget::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton )
	{
		new stringPairDrag( "instrument", m_pluginDescriptor.name,
								m_logo, this );
		leaveEvent( _me );
	}
}




void PluginDescWidget::updateHeight()
{
	if( m_targetHeight > height() )
	{
		setFixedHeight( height() + 1 );
	}
	else if( m_targetHeight < height() )
	{
		setFixedHeight( height() - 1 );
	}
	else
	{
		m_updateTimer.stop();
		return;
	}
	if( !m_updateTimer.isActive() )
	{
		m_updateTimer.start( 15 );
	}
}




#include "moc_plugin_browser.cxx"
