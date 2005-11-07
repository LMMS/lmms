/*
 * text_float.h - class textFloat, a floating text-label
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _TEXT_FLOAT
#define _TEXT_FLOAT

#include "qt3support.h"

#ifdef QT4

#include <QWidget>

#else

#include <qwidget.h>

#endif


class textFloat : public QWidget
{
public:
	textFloat( QWidget * _parent );
	virtual ~textFloat()
	{
	}

	void setText( const QString & _text );

	void reparent( QWidget * _new_parent );

	void setVisibilityTimeOut( int _msecs );


protected:
	virtual void paintEvent( QPaintEvent * _me );


private:
	QString m_text;

} ;


#endif
