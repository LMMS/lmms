/*
 * plugin_browser.cpp - implementation of the plugin-browser
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "qt3support.h"

#ifdef QT4

#include <QLabel>
#include <QPainter>
/*#include <QScrollArea>*/
#include <QCursor>

#else

#include <qlabel.h>
#include <qpainter.h>
/*#include <qscrollview.h>*/
#include <qcursor.h>

#endif


#include "plugin_browser.h"
#include "embed.h"
#include "debug.h"
#include "gui_templates.h"
#include "string_pair_drag.h"



pluginBrowser::pluginBrowser( QWidget * _parent ) :
	sideBarWidget( tr( "Instrument plugins" ),
				embed::getIconPixmap( "plugins" ), _parent )
{
	setWindowTitle( tr( "Plugin browser" ) );
	m_view = new QWidget( contentParent() );
	//m_view->setFrameShape( QFrame::NoFrame );

	addContentWidget( m_view );

	QVBoxLayout * view_layout = new QVBoxLayout( m_view, 5, 10 );


	QLabel * hint = new QLabel( tr( "You can drag an instrument-plugin "
					"into either the Song-Editor, the "
					"Beat+Bassline Editor or just into a "
					"channel-window or on the "
					"corresponding channel-button." ),
								m_view );
	hint->setFont( pointSize<8>( hint->font() ) );
	hint->setAlignment( hint->alignment() | WordBreak );
	view_layout->addWidget( hint );

	plugin::getDescriptorsOfAvailPlugins( m_pluginDescriptors );

	for( vvector<plugin::descriptor>::iterator it =
						m_pluginDescriptors.begin();
					it != m_pluginDescriptors.end(); ++it )
	{
		pluginDescWidget * p = new pluginDescWidget( *it, m_view );
		p->show();
		view_layout->addWidget( p );
	}
	view_layout->addStretch();
	show();
}




pluginBrowser::~pluginBrowser()
{
}







pluginDescWidget::pluginDescWidget( const plugin::descriptor & _pd,
							QWidget * _parent ) :
	QWidget( _parent ),
	m_pluginDescriptor( _pd ),
	m_logo(),
	m_mouseOver( FALSE )
{
	m_logo.loadFromData( _pd.logo.data, _pd.logo.size );
	setFixedHeight( 60 );
	setMouseTracking( TRUE );
#ifndef QT4
	setBackgroundMode( Qt::NoBackground );
#endif
	setCursor( PointingHandCursor );
}




pluginDescWidget::~pluginDescWidget()
{
}




void pluginDescWidget::paintEvent( QPaintEvent * )
{
	QColor fill_color( 192, 192, 192 );
	if( m_mouseOver )
	{
		fill_color = QColor( 224, 224, 224 );
	}

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
	p.setPen( QColor( 64, 64, 64 ) );
	p.drawRect( rect() );
	p.drawPixmap( 4, 4, m_logo );

	QFont f = pointSize<8>( p.font() );
	f.setBold( TRUE );
	p.setFont( f );
	p.drawText( 58, 14, m_pluginDescriptor.public_name );

	f.setBold( FALSE );
	p.setFont( pointSize<7>( f ) );
	QStringList words = QStringList::split( ' ',
			pluginBrowser::tr( m_pluginDescriptor.description ) );
	for( QStringList::iterator it = words.begin(); it != words.end(); ++it )
	{
		if( ( *it ).contains( '-' ) )
		{
			QStringList splitted_word = QStringList::split( '-',
									*it );
			QStringList::iterator orig_it = it;
			for( QStringList::iterator it2 = splitted_word.begin();
					it2 != splitted_word.end(); ++it2 )
			{
				if( it2 == splitted_word.fromLast() )
				{
					words.insert( it, *it2 );
				}
				else
				{
					words.insert( it, *it2 + "-" );
					++it;
				}
			}
			words.erase( orig_it );
			--it;
		}
	}

	int y = 26;
	int avail_width = width() - 58 - 5;
	QString s;
	for( QStringList::iterator it = words.begin(); it != words.end(); ++it )
	{
		if( p.fontMetrics().width( s + *it + " " ) >= avail_width )
		{
			p.drawText( 58, y, s );
			y += 10;
			s = "";
		}
		s += *it;
		if( ( *it ).right( 1 ) != "-" )
		{
			s += " ";
		}
	}
	p.drawText( 58, y, s );

#ifndef QT4
	// blit drawn pixmap to actual widget
	bitBlt( this, rect().topLeft(), &pm );
#endif
}




void pluginDescWidget::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton )
	{
		new stringPairDrag( "plugin", m_pluginDescriptor.name, m_logo,
									this );
		m_mouseOver = FALSE;
		update();
	}
}




void pluginDescWidget::mouseMoveEvent( QMouseEvent * _me )
{
	bool new_mouse_over = rect().contains( _me->pos() );
	if( new_mouse_over != m_mouseOver )
	{
		m_mouseOver = new_mouse_over;
		update();
	}
}




void pluginDescWidget::mouseReleaseEvent( QMouseEvent * _me )
{
	mouseMoveEvent( _me );
}



#include "plugin_browser.moc"

