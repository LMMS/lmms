/*
 * name_label.h - class nameLabel, a label which is renamable by
 *                double-clicking it
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "qt3support.h"

#ifdef QT4

#include <QtGui/QLabel>
#include <QtGui/QPixmap>

#else

#include <qlabel.h>
#include <qpixmap.h>

#endif

#include "engine.h"


class nameLabel : public QLabel, public engineObject
{
	Q_OBJECT
public:
	nameLabel( const QString & _initial_name, QWidget * _parent,
							engine * _engine );
	virtual ~nameLabel();

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
	void rename( void );
	void selectPixmap( void );


signals:
	void clicked( void );
	void nameChanged( void );
	void nameChanged( const QString & _new_name );
	void pixmapChanged( void );


protected:
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseDoubleClickEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );


private:
	QPixmap m_pixmap;
	QString m_pixmapFile;

} ;

#endif
