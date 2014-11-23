/*
 * text_float.h - class textFloat, a floating text-label
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef TEXT_FLOAT_H
#define TEXT_FLOAT_H

#include <QtGui/QWidget>
#include <QtGui/QPixmap>

#include "export.h"


class EXPORT textFloat : public QWidget
{
	Q_OBJECT
public:
	textFloat();
	virtual ~textFloat()
	{
	}

	void setTitle( const QString & _title );
	void setText( const QString & _text );
	void setPixmap( const QPixmap & _pixmap );

	void setVisibilityTimeOut( int _msecs );


	static textFloat * displayMessage( const QString & _msg,
						int _timeout = 2000,
						QWidget * _parent = NULL,
						int _add_y_margin = 0 );
	static textFloat * displayMessage( const QString & _title,
						const QString & _msg,
						const QPixmap & _pixmap =
								QPixmap(),
						int _timeout = 2000,
						QWidget * _parent = NULL );

	void moveGlobal( QWidget * _w, const QPoint & _offset )
	{
		move( _w->mapToGlobal( QPoint( 0, 0 ) )+_offset );
	}


protected:
	virtual void paintEvent( QPaintEvent * _me );
	virtual void mousePressEvent( QMouseEvent * _me );


private:
	void updateSize();

	QString m_title;
	QString m_text;
	QPixmap m_pixmap;

};

#endif
