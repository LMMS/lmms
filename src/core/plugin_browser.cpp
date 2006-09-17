#ifndef SINGLE_SOURCE_COMPILE

/*
 * plugin_browser.cpp - implementation of the plugin-browser
 *
 * Copyright (c) 2005-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "qt3support.h"

#ifdef QT4

#include <QtGui/QLabel>
#include <QtGui/QPainter>
#include <QtGui/QCursor>
#include <QtGui/QMouseEvent>

#else

#include <qlabel.h>
#include <qpainter.h>
#include <qcursor.h>

#endif


#include "plugin_browser.h"
#include "embed.h"
#include "debug.h"
#include "templates.h"
#include "gui_templates.h"
#include "string_pair_drag.h"



pluginBrowser::pluginBrowser( QWidget * _parent, engine * _engine ) :
	sideBarWidget( tr( "Instrument plugins" ),
				embed::getIconPixmap( "plugins" ), _parent ),
	engineObject( _engine )
{
	setWindowTitle( tr( "Plugin browser" ) );
	m_view = new QWidget( contentParent() );
	//m_view->setFrameShape( QFrame::NoFrame );

	addContentWidget( m_view );

	QVBoxLayout * view_layout = new QVBoxLayout( m_view );
	view_layout->setMargin( 5 );
	view_layout->setSpacing( 10 );


	QLabel * hint = new QLabel( tr( "You can drag an instrument-plugin "
					"into either the Song-Editor, the "
					"Beat+Baseline Editor or just into a "
					"channel-window or on the "
					"corresponding channel-button." ),
								m_view );
	hint->setFont( pointSize<8>( hint->font() ) );
#ifdef QT4
	hint->setWordWrap( TRUE );
#else
	hint->setAlignment( hint->alignment() | Qt::WordBreak );
#endif
	view_layout->addWidget( hint );

	plugin::getDescriptorsOfAvailPlugins( m_pluginDescriptors );

	for( vvector<plugin::descriptor>::iterator it =
						m_pluginDescriptors.begin();
					it != m_pluginDescriptors.end(); ++it )
	{
		if( it->type == plugin::Instrument )
		{
			pluginDescWidget * p = new pluginDescWidget( *it,
								m_view, eng() );
			p->show();
			view_layout->addWidget( p );
		}
	}
	view_layout->addStretch();
	show();
}




pluginBrowser::~pluginBrowser()
{
}







pluginDescWidget::pluginDescWidget( const plugin::descriptor & _pd,
					QWidget * _parent, engine * _engine ) :
	QWidget( _parent ),
	engineObject( _engine ),
	m_updateTimer( this ),
	m_pluginDescriptor( _pd ),
	m_logo( *_pd.logo ),
	m_mouseOver( FALSE ),
	m_targetHeight( 24 )
{
	connect( &m_updateTimer, SIGNAL( timeout() ), SLOT( updateHeight() ) );
	setFixedHeight( m_targetHeight );
	setMouseTracking( TRUE );
#ifdef QT3
	setBackgroundMode( Qt::NoBackground );
#endif
	setCursor( Qt::PointingHandCursor );
}




pluginDescWidget::~pluginDescWidget()
{
}




void pluginDescWidget::paintEvent( QPaintEvent * )
{
	const QColor fill_color = m_mouseOver ? QColor( 224, 224, 224 ) :
						QColor( 192, 192, 192 );

#ifdef QT4
	QPainter p( this );
	p.fillRect( rect(), fill_color );
#else
	// create pixmap for whole widget
	QPixmap pm( rect().size() );
	pm.fill( fill_color );

	// and a painter for it
	QPainter p( &pm );
#endif

	const int s = 16 + ( 32 * ( tLimit( height(), 24, 60 ) - 24 ) ) /
								( 60 - 24 );
	const QSize logo_size( s, s );
#ifndef QT3
	QPixmap logo = m_logo.scaled( logo_size, Qt::KeepAspectRatio,
						Qt::SmoothTransformation );
#else
	QPixmap logo;
	logo.convertFromImage( m_logo.convertToImage().smoothScale( logo_size,
							QImage::ScaleMin ) );
#endif
	p.setPen( QColor( 64, 64, 64 ) );
#ifndef QT3
	p.drawRect( 0, 0, rect().right(), rect().bottom() );
#else
	p.drawRect( rect() );
#endif
	p.drawPixmap( 4, 4, logo );

	QFont f = pointSize<8>( p.font() );
	f.setBold( TRUE );
	p.setFont( f );
	p.drawText( 10 + logo_size.width(), 15,
					m_pluginDescriptor.public_name );

	if( height() > 24 || m_mouseOver )
	{
		f.setBold( FALSE );
		p.setFont( pointSize<7>( f ) );
		QRect br;
		p.drawText( 10 + logo_size.width(), 20, width() - 58 - 5, 999,
#ifndef QT3
							Qt::TextWordWrap,
#else
							Qt::WordBreak,
#endif
			pluginBrowser::tr( m_pluginDescriptor.description ),
#ifdef QT3
								-1,
#endif
								&br );
		if( m_mouseOver )
		{
			m_targetHeight = tMax( 60, 25 + br.height() );
		}
	}

#ifndef QT4
	// blit drawn pixmap to actual widget
	bitBlt( this, rect().topLeft(), &pm );
#endif
}




void pluginDescWidget::enterEvent( QEvent * _e )
{
	m_mouseOver = TRUE;
	m_targetHeight = height() + 1;
	updateHeight();
	QWidget::enterEvent( _e );
}




void pluginDescWidget::leaveEvent( QEvent * _e )
{
	m_mouseOver = FALSE;
	m_targetHeight = 24;
	updateHeight();
	QWidget::leaveEvent( _e );
}




void pluginDescWidget::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton )
	{
		new stringPairDrag( "instrument", m_pluginDescriptor.name,
							m_logo, this, eng() );
		leaveEvent( _me );
	}
}




void pluginDescWidget::updateHeight( void )
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




#include "plugin_browser.moc"


#endif
