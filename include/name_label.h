/*
 * name_label.h - class nameLabel, a label which is renamable by
 *                double-clicking it
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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


#ifndef _NAME_LABEL_H
#define _NAME_LABEL_H

#include "qt3support.h"

#ifdef QT4

#include <QLabel>
#include <QPixmap>

#else

#include <qlabel.h>
#include <qpixmap.h>

#endif


class nameLabel : public QLabel
{
	Q_OBJECT
public:
	nameLabel( const QString & _initial_name, QWidget * _parent,
					const QPixmap & _pm = QPixmap() );
	~nameLabel();
	const QPixmap * pixmap( void ) const;


public slots:
	void setPixmap( const QPixmap & _pm );
	void rename( void );


signals:
	void nameChanged( const QString & _new_name );
	void clicked( void );


protected:
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseDoubleClickEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );


private:
	QPixmap m_pm;

} ;

#endif
