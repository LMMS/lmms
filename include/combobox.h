/*
 * combobox.h - class comboBox, a very cool combo-box
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _COMBOBOX_H
#define _COMBOBOX_H

#include "qt3support.h"

#ifdef QT4

#include <QtGui/QWidget>
#include <QtCore/QVector>
#include <QtGui/QMenu>
#include <QtGui/QPixmap>
#include <QtCore/QPair>

#else

#include <qwidget.h>
#include <qvaluevector.h>
#include <qpopupmenu.h>
#include <qpixmap.h>
#include <qpair.h>

#endif

#include "automatable_object.h"


class QAction;


class comboBox : public QWidget, public automatableObject<int>
{
	Q_OBJECT
public:
	comboBox( QWidget * _parent, engine * _engine );
	virtual ~comboBox();

	void addItem( const QString & _item, const QPixmap & _pixmap =
								QPixmap() );

	inline void clear( void )
	{
		setRange( 0, 0 );
		m_items.clear();
		update();
	}

	int findText( const QString & _txt ) const;

	QString currentText( void ) const
	{
		return( m_items[value()].first );
	}

	void setValue( const int _idx );


protected:
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void wheelEvent( QWheelEvent * _we );


private:
	static QPixmap * s_background;
	static QPixmap * s_arrow;

	QMenu m_menu;

	typedef QPair<QString, QPixmap> item;

	vvector<item> m_items;

	bool m_pressed;


private slots:
	void setItem( QAction * _item );
	void setItem( int _item );


signals:
	void activated( const QString & );
	void valueChanged( int );

} ;

#endif
