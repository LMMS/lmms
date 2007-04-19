#ifndef SINGLE_SOURCE_COMPILE

/*
 * name_label.cpp - implementation of class nameLabel, a label which
 *                  is renamable by double-clicking it
 *
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtGui/QFileDialog>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>

#else

#include <qpainter.h>
#include <qfiledialog.h>
#include <qimage.h>

#endif


#include "name_label.h"
#include "rename_dialog.h"
#include "bb_editor.h"
#include "bb_track.h"
#include "gui_templates.h"
#include "config_mgr.h"
#include "engine.h"



nameLabel::nameLabel( const QString & _initial_name, QWidget * _parent ) :
	QLabel( _initial_name, _parent ),
	m_pixmap(),
	m_pixmapFile( "" )
{
#ifdef QT3
	setBackgroundMode( Qt::NoBackground );
#endif
}



nameLabel::~nameLabel()
{
}




void nameLabel::setPixmap( const QPixmap & _pixmap )
{
	m_pixmap = _pixmap;
}




void nameLabel::setPixmapFile( const QString & _file )
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
	m_pixmap = new_pixmap;
	m_pixmapFile = _file;
	emit( pixmapChanged() );
	update();
}




void nameLabel::selectPixmap( void )
{
#ifdef QT4
	QFileDialog ofd( NULL, tr( "Select icon" ) );
#else
	QFileDialog ofd( QString::null, QString::null, NULL, "", TRUE );
	ofd.setWindowTitle( tr( "Select icon" ) );
#endif

	QString dir;
	if( m_pixmapFile != "" )
	{
		QString f = m_pixmapFile;
		if( QFileInfo( f ).isRelative() )
		{
			f = configManager::inst()->trackIconsDir() + f;
		}
#ifdef QT4
		dir = QFileInfo( f ).absolutePath();
#else
		dir = QFileInfo( f ).dirPath( TRUE );
#endif
	}
	else
	{
		dir = configManager::inst()->trackIconsDir();
	}
	// change dir to position of previously opened file
	ofd.setDirectory( dir );
	// use default QFileDialog::ExistingFile

	// set filters
#ifdef QT4
	QStringList types;
	types << tr( "All images (*.png *.jpg *.jpeg *.gif *.bmp)" );
	ofd.setFilters( types );
#else
	ofd.addFilter( tr( "All images (*.png *.jpg *.jpeg *.gif *.bmp)" ) );
	ofd.setSelectedFilter( tr( "All images (*.png *.jpg *.jpeg *.gif "
								"*.bmp)" ) );
#endif
	if( m_pixmapFile != "" )
	{
		// select previously opened file
		ofd.selectFile( QFileInfo( m_pixmapFile ).fileName() );
	}

	if ( ofd.exec () == QDialog::Accepted
#ifndef QT3
		&& !ofd.selectedFiles().isEmpty()
#endif
	)
	{
		QString pf = ofd.
#ifdef QT3
				selectedFile();
#else
				selectedFiles()[0];
#endif
		if( !QFileInfo( pf ).isRelative() )
		{
#if QT_VERSION >= 0x030100
			pf = pf.replace( configManager::inst()->trackIconsDir(),
									"" );
#else
			pf = pf.replace( QRegExp(
				configManager::inst()->trackIconsDir() ), "" );
#endif
		}
		setPixmapFile( pf );
	}
}




void nameLabel::rename( void )
{
	QString txt = text();
	renameDialog rename_dlg( txt );
	rename_dlg.exec();
	if( txt != text() )
	{
		setText( txt );
		emit nameChanged( txt );
		emit nameChanged();
	}
}




void nameLabel::mousePressEvent( QMouseEvent * _me )
{

	if( _me->button() == Qt::RightButton )
	{
		QSize s( m_pixmap.width(), m_pixmap.height() );
#ifndef QT3
		s.scale( width(), height(), Qt::KeepAspectRatio );
#else
		s.scale( width(), height(), QSize::ScaleMin );
#endif
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
		emit clicked();
		QLabel::mousePressEvent( _me );
	}
}




void nameLabel::mouseDoubleClickEvent( QMouseEvent * _me )
{
	QSize s( m_pixmap.width(), m_pixmap.height() );
#ifndef QT3
	s.scale( width(), height(), Qt::KeepAspectRatio );
#else
	s.scale( width(), height(), QSize::ScaleMin );
#endif
	if( _me->x() > 4 + s.width() )
	{
		rename();
	}
	else
	{
		selectPixmap();
	}
}




void nameLabel::paintEvent( QPaintEvent * )
{
#ifdef QT4
	QPainter p( this );
	p.fillRect( rect(), palette().color( backgroundRole() ) );
#else
	QPixmap draw_pm( size() );
	draw_pm.fill( this, rect().topLeft() );

	QPainter p( &draw_pm, this );
#endif
	p.setFont( pointSize<8>( p.font() ) );

	int x = 4;
	if( m_pixmap.isNull() == FALSE )
	{
		QPixmap pm = m_pixmap;
		if( pm.height() > height() )
		{
#ifndef QT3
			pm = pm.scaledToHeight( height(),
						Qt::SmoothTransformation );
#else
			pm.convertFromImage( pm.convertToImage().smoothScale(
							pm.width(), height(),
							QImage::ScaleMin ) );
#endif
		}
		p.drawPixmap( x, ( height() - pm.height() ) / 2, pm );
		x += 4 + pm.width();
	}

	p.setPen( QColor( 16, 16, 16 ) );
	p.drawText( x+1, height() / 2 + p.fontMetrics().height() / 2 - 3,
								text() );

	p.setPen( QColor( 0, 224, 0 ) );
	bbTrack * bbt = bbTrack::findBBTrack(
					engine::getBBEditor()->currentBB() );
	if( bbt != NULL && bbt->getTrackSettingsWidget() ==
			dynamic_cast<trackSettingsWidget *>( parentWidget() ) )
	{
		p.setPen( QColor( 255, 255, 255 ) );
	}
	p.drawText( x, height() / 2 + p.fontMetrics().height() / 2 - 4,
								text() );

#ifndef QT4
	// and blit all the drawn stuff on the screen...
	bitBlt( this, rect().topLeft(), &draw_pm );
#endif
}




#include "name_label.moc"


#endif
