/*
 * name_label.cpp - implementation of class nameLabel, a label which
 *                  is renamable by double-clicking it
 *
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "qt3support.h"

#ifdef QT4

#include <QPainter>
#include <QMouseEvent>

#else

#include <qpainter.h>

#endif


#include "name_label.h"
#include "rename_dialog.h"
#include "bb_editor.h"
#include "bb_track.h"
#include "gui_templates.h"



nameLabel::nameLabel( const QString & _initial_name, QWidget * _parent,
							const QPixmap & _pm ) :
	QLabel( _initial_name, _parent ),
	m_pm( _pm )
{
#ifndef QT4
	setBackgroundMode( Qt::NoBackground );
#endif
}



nameLabel::~nameLabel()
{
}




const QPixmap * nameLabel::pixmap( void ) const
{
	return( &m_pm );
}




void nameLabel::setPixmap( const QPixmap & _pm )
{
	m_pm = _pm;
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
	}
}




void nameLabel::paintEvent( QPaintEvent * )
{
#ifdef QT4
	QPainter p( this );
#else
	QPixmap draw_pm( rect().size() );
	draw_pm.fill( this, rect().topLeft() );

	QPainter p( &draw_pm, this );
#endif
	p.setFont( pointSize<8>( p.font() ) );

	QFontMetrics fm( font() );

	p.drawPixmap( 4, ( height() - m_pm.height() ) / 2, m_pm );
	p.setPen( QColor( 0, 224, 0 ) );
	bbTrack * bbt = bbTrack::findBBTrack( bbEditor::inst()->currentBB() );
	if( bbt != NULL && bbt->getTrackSettingsWidget() ==
			dynamic_cast<trackSettingsWidget *>( parentWidget() ) )
	{
		p.setPen( QColor( 255, 255, 255 ) );
	}
	p.drawText( m_pm.width() + 8, height() / 2 + fm.height() / 2 - 4,
								text() );

#ifndef QT4
	// and blit all the drawn stuff on the screen...
	bitBlt( this, rect().topLeft(), &draw_pm );
#endif
}




void nameLabel::mousePressEvent( QMouseEvent * _me )
{

	if( _me->button() == Qt::RightButton )
	{
		rename();
	}
	else
	{
		emit clicked();
		QLabel::mousePressEvent( _me );
	}
}




void nameLabel::mouseDoubleClickEvent( QMouseEvent * _me )
{
	rename();
}




#include "name_label.moc"

