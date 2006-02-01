/*
 * rubberband.h - rubberband - either own implementation for Qt3 or wrapper for
 *                             Qt4
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


#ifndef _RUBBERBAND_H
#define _RUBBERBAND_H

#include "qt3support.h"

#ifndef QT3

#include <QRubberBand>
#include <QVector>

#else

#include <qwidget.h>
#include <qvaluevector.h>

#endif


class selectableObject : public QWidget
{
	Q_OBJECT
public:
	selectableObject( QWidget * _parent
#ifdef QT3
				, WFlags _f
#endif
						) :
		QWidget( _parent
#ifdef QT3
				, NULL, _f
#endif
				),
		m_selected( FALSE )
	{
	}

	virtual ~selectableObject()
	{
	}

	inline void setSelected( bool _selected )
	{
		m_selected = _selected;
		update();
	}

	inline bool isSelected( void ) const
	{
		return( m_selected );
	}


public slots:
	virtual void update( void )
	{
		QWidget::update();
	}


private:
	bool m_selected;

} ;



typedef
#ifndef QT3
	QRubberBand
#else
	QWidget
#endif
			rubberBandBase;


class rubberBand : public rubberBandBase
{
public:
	rubberBand( QWidget * _parent );
	virtual ~rubberBand();

	vvector<selectableObject *> selectedObjects( void ) const;


protected:
	virtual void resizeEvent( QResizeEvent * _re );
#ifdef QT3
	virtual bool event( QEvent * _e );
	void updateMask( void );
#endif

private:
	vvector<selectableObject *> selectableObjects( void ) const;

};


#endif

