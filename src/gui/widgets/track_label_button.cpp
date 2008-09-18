#ifndef SINGLE_SOURCE_COMPILE

/*
 * track_label_button.cpp - implementation of class trackLabelButton, a label
 *                          which is renamable by double-clicking it
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtGui/QFileDialog>
#include <QtGui/QHBoxLayout>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QToolButton>


#include "track_label_button.h"
#include "embed.h"
#include "rename_dialog.h"
#include "bb_track_container.h"
#include "bb_track.h"
#include "gui_templates.h"
#include "config_mgr.h"
#include "engine.h"



trackLabelButton::trackLabelButton( trackView * _tv, QWidget * _parent ) :
	QToolButton( _parent ),
	m_trackView( _tv )
{
	setAcceptDrops( TRUE );
	setCursor( QCursor( embed::getIconPixmap( "hand" ), 0, 0 ) );
	setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
	setFixedSize( 160, 29 );
	setIconSize( QSize( 32, 32 ) );
	setText( " " );

	connect( m_trackView->getTrack(), SIGNAL( dataChanged() ),
					this, SLOT( update() ) );
}




trackLabelButton::~trackLabelButton()
{
}




void trackLabelButton::rename( void )
{
	QString txt = m_trackView->getTrack()->name();
	renameDialog rename_dlg( txt );
	rename_dlg.exec();
	if( txt != text() )
	{
		m_trackView->getTrack()->setName( txt );
	}
}




void trackLabelButton::dragEnterEvent( QDragEnterEvent * _dee )
{
	m_trackView->dragEnterEvent( _dee );
}




void trackLabelButton::dropEvent( QDropEvent * _de )
{
	m_trackView->dropEvent( _de );
	setChecked( TRUE );
}




void trackLabelButton::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::RightButton )
	{
		rename();
	}
	else
	{
		QToolButton::mousePressEvent( _me );
	}
}




void trackLabelButton::mouseDoubleClickEvent( QMouseEvent * _me )
{
	rename();
}




void trackLabelButton::paintEvent( QPaintEvent * _pe )
{
	if( m_trackView->getTrack()->type() == track::InstrumentTrack )
	{
		QToolButton::paintEvent( _pe );
		QPainter p( this );
		const QString dn = m_trackView->getTrack()->displayName();
		const QString in = dn.split( ':' )[0];
		const QString tn = dn.split( ':' )[1];
		const int extra = ( isDown() || isChecked() ) ? -2 : -3;
		const int is = 40;
		p.setPen( QApplication::palette().buttonText().color().
								darker( 130 ) );
		p.drawText( is + extra, height() / 2 + extra, in );
		p.setPen( QApplication::palette().buttonText().color() );
		p.drawText( is + extra, height() / 2 +
				QFontMetrics( p.font() ).height() + extra - 2,
								tn );
	}
	else
	{
		setText( m_trackView->getTrack()->displayName() );
		QToolButton::paintEvent( _pe );
	}
}



#include "moc_track_label_button.cxx"


#endif
