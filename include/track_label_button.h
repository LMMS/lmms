/*
 * name_label.h - class trackLabelButton
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


#ifndef _NAME_LABEL_H
#define _NAME_LABEL_H

#include <QtGui/QToolButton>


class trackView;


class trackLabelButton : public QToolButton
{
	Q_OBJECT
public:
	trackLabelButton( trackView * _tv, QWidget * _parent );
	virtual ~trackLabelButton();

	const QPixmap & pixmap( void ) const
	{
		return( m_pixmap );
	}

	const QString & pixmapFile( void ) const
	{
		return( m_pixmapFile );
	}


public slots:
	void setPixmap( const QPixmap & _pixmap );
	void setPixmapFile( const QString & _file );
	void selectPixmap( void );
	void rename( void );
	void updateName( void );


signals:
	void nameChanged( void );
	void pixmapChanged( void );


protected:
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseDoubleClickEvent( QMouseEvent * _me );


private:
	trackView * m_trackView;
	QPixmap m_pixmap;
	QString m_pixmapFile;

} ;

#endif
