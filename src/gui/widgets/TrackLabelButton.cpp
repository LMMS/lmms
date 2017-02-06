/*
 * TrackLabelButton.cpp - implementation of class trackLabelButton, a label
 *                          which is renamable by double-clicking it
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "TrackLabelButton.h"

#include <QApplication>
#include <QMouseEvent>

#include "ConfigManager.h"
#include "embed.h"
#include "Engine.h"
#include "Instrument.h"
#include "InstrumentTrack.h"
#include "RenameDialog.h"
#include "Song.h"



TrackLabelButton::TrackLabelButton( TrackView * _tv, QWidget * _parent ) :
	QToolButton( _parent ),
	m_trackView( _tv ),
	m_iconName()
{
	setAttribute( Qt::WA_OpaquePaintEvent, true );
	setAcceptDrops( true );
	setCursor( QCursor( embed::getIconPixmap( "hand" ), 3, 3 ) );
	setToolButtonStyle( Qt::ToolButtonTextBesideIcon );

	if( ConfigManager::inst()->value( "ui", "compacttrackbuttons" ).toInt() )
	{
		setFixedSize( 32, 29 );
	}
	else
	{
		setFixedSize( 160, 29 );
		m_renameLineEdit = new QLineEdit( this );
		m_renameLineEdit->move( 30, ( height() / 2 ) - ( m_renameLineEdit->sizeHint().height() / 2 ) );
		m_renameLineEdit->setFixedWidth( width() - 33 );
		m_renameLineEdit->hide();
		connect( m_renameLineEdit, SIGNAL( editingFinished() ), this, SLOT( renameFinished() ) );
	}

	setIconSize( QSize( 24, 24 ) );
	setText( " " );
	connect( m_trackView->getTrack(), SIGNAL( dataChanged() ), this, SLOT( update() ) );
}




TrackLabelButton::~TrackLabelButton()
{
}




void TrackLabelButton::rename()
{
	if( ConfigManager::inst()->value( "ui", "compacttrackbuttons" ).toInt() )
	{
		QString txt = m_trackView->getTrack()->name();
		RenameDialog rename_dlg( txt );
		rename_dlg.exec();
		if( txt != text() )
		{
			m_trackView->getTrack()->setName( txt );
			Engine::getSong()->setModified();
		}
	}
	else
	{
		QString txt = m_trackView->getTrack()->name();
		m_renameLineEdit->show();
		m_renameLineEdit->setText( txt );
		m_renameLineEdit->selectAll();
		m_renameLineEdit->setFocus();
	}
}




void TrackLabelButton::renameFinished()
{
	if( !( ConfigManager::inst()->value( "ui", "compacttrackbuttons" ).toInt() ) )
	{
		m_renameLineEdit->hide();
		if( m_renameLineEdit->text() != "" && m_renameLineEdit->text() != m_trackView->getTrack()->name() )
		{
			m_trackView->getTrack()->setName( m_renameLineEdit->text() );
			Engine::getSong()->setModified();
		}
	}
}




void TrackLabelButton::dragEnterEvent( QDragEnterEvent * _dee )
{
	m_trackView->dragEnterEvent( _dee );
}




void TrackLabelButton::dropEvent( QDropEvent * _de )
{
	m_trackView->dropEvent( _de );
	setChecked( true );
}




void TrackLabelButton::mousePressEvent( QMouseEvent * _me )
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




void TrackLabelButton::mouseDoubleClickEvent( QMouseEvent * _me )
{
	rename();
}




void TrackLabelButton::paintEvent( QPaintEvent * _pe )
{
	if( m_trackView->getTrack()->type() == Track::InstrumentTrack )
	{
		InstrumentTrack * it =
			dynamic_cast<InstrumentTrack *>( m_trackView->getTrack() );
		const PixmapLoader * pl;
		if( it && it->instrument() &&
			it->instrument()->descriptor() &&
			( pl = it->instrument()->descriptor()->logo ) )
		{
			if( pl->pixmapName() != m_iconName )
			{
				m_iconName = pl->pixmapName();
				setIcon( pl->pixmap() );
			}
		}
	}
	if( ConfigManager::inst()->value( "ui", "compacttrackbuttons" ).toInt() )
	{
		setText( " " );
		setToolTip( m_trackView->getTrack()->displayName() );
	}
	else
	{
		setText( elideName( m_trackView->getTrack()->displayName() ) );
	}
	QToolButton::paintEvent( _pe );
}




QString TrackLabelButton::elideName( const QString &name )
{
	const int maxTextWidth = 120;
	QFontMetrics metrics( font() );
	QString elidedName = metrics.elidedText( name, Qt::ElideRight, maxTextWidth );
	return elidedName;
}
