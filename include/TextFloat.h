/*
 * TextFloat.h - class textFloat, a floating text-label
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of LMMS - https://lmms.io
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

#include <QWidget>
#include <QPixmap>

#include "lmms_export.h"


class LMMS_EXPORT TextFloat : public QWidget
{
	Q_OBJECT
public:
	TextFloat();
	virtual ~TextFloat()
	{
	}

	void setTitle( const QString & _title );
	void setText( const QString & _text );
	void setPixmap( const QPixmap & _pixmap );

	void setVisibilityTimeOut( int _msecs );


	static TextFloat * displayMessage( const QString & _msg,
						int _timeout = 2000,
						QWidget * _parent = NULL,
						int _add_y_margin = 0 );
	static TextFloat * displayMessage( const QString & _title,
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
	void paintEvent( QPaintEvent * _me ) override;
	void mousePressEvent( QMouseEvent * _me ) override;


private:
	void updateSize();

	QString m_title;
	QString m_text;
	QPixmap m_pixmap;

};

#endif
