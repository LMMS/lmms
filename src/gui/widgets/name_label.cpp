#ifndef SINGLE_SOURCE_COMPILE

/*
 * name_label.cpp - implementation of class trackLabelButton, a label which
 *                  is renamable by double-clicking it
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


#include "name_label.h"
#include "embed.h"
#include "rename_dialog.h"
#include "bb_track_container.h"
#include "bb_track.h"
#include "gui_templates.h"
#include "config_mgr.h"
#include "engine.h"



trackLabelButton::trackLabelButton( trackView * _tv, QWidget * _parent ) :
	QToolButton( _parent ),
	m_trackView( _tv ),
	m_pixmap(),
	m_pixmapFile( "" )
{
	setAcceptDrops( TRUE );
	setCursor( QCursor( embed::getIconPixmap( "hand" ), 0, 0 ) );
	setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
	setFixedSize( 160, 29 );
	updateName();

	connect( m_trackView->getTrack(), SIGNAL( dataChanged() ),
					this, SLOT( updateName() ) );
}




trackLabelButton::~trackLabelButton()
{
}




void trackLabelButton::setPixmap( const QPixmap & _pixmap )
{
	m_pixmap = _pixmap;
	setIcon( m_pixmap );
}




void trackLabelButton::setPixmapFile( const QString & _file )
{
	QPixmap new_pixmap;
	if( QFileInfo( _file ).isRelative() )
	{
		new_pixmap = QPixmap( configManager::inst()->trackIconsDir() +
									_file );
	}
	else
	{
		new_pixmap = QPixmap( _file );
	}
	if( new_pixmap.isNull() )
	{
		return;
	}
	setPixmap( new_pixmap );
	m_pixmapFile = _file;
	emit( pixmapChanged() );
	update();
}




void trackLabelButton::selectPixmap( void )
{
	QFileDialog ofd( NULL, tr( "Select icon" ) );

	QString dir;
	if( m_pixmapFile != "" )
	{
		QString f = m_pixmapFile;
		if( QFileInfo( f ).isRelative() )
		{
			f = configManager::inst()->trackIconsDir() + f;
		}
		dir = QFileInfo( f ).absolutePath();
	}
	else
	{
		dir = configManager::inst()->trackIconsDir();
	}
	// change dir to position of previously opened file
	ofd.setDirectory( dir );
	// use default QFileDialog::ExistingFile

	// set filters
	QStringList types;
	types << tr( "All images (*.png *.jpg *.jpeg *.gif *.bmp)" );
	ofd.setFilters( types );
	if( m_pixmapFile != "" )
	{
		// select previously opened file
		ofd.selectFile( QFileInfo( m_pixmapFile ).fileName() );
	}

	if ( ofd.exec () == QDialog::Accepted
		&& !ofd.selectedFiles().isEmpty()
	)
	{
		QString pf = ofd.selectedFiles()[0];
		if( !QFileInfo( pf ).isRelative() )
		{
			pf = pf.replace( configManager::inst()->trackIconsDir(),
									"" );
		}
		setPixmapFile( pf );
	}
}




void trackLabelButton::rename( void )
{
	QString txt = m_trackView->getTrack()->name();
	renameDialog rename_dlg( txt );
	rename_dlg.exec();
	if( txt != text() )
	{
		m_trackView->getTrack()->setName( txt );
		updateName();
	}
}




void trackLabelButton::updateName( void )
{
	setText( m_trackView->getTrack()->name() );
}




void trackLabelButton::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::RightButton )
	{
		QSize s( m_pixmap.width(), m_pixmap.height() );
		s.scale( width(), height(), Qt::KeepAspectRatio );
		if( _me->x() > 4 + s.width() )
		{
			rename();
		}
		else
		{
			selectPixmap();
		}
	}
	else
	{
		QToolButton::mousePressEvent( _me );
	}
}




void trackLabelButton::mouseDoubleClickEvent( QMouseEvent * _me )
{
	QSize s( m_pixmap.width(), m_pixmap.height() );
	s.scale( width(), height(), Qt::KeepAspectRatio );
	if( _me->x() > 4 + s.width() )
	{
		rename();
	}
	else
	{
		selectPixmap();
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



#include "name_label.moc"


#endif
