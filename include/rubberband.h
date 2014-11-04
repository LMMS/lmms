/*
 * rubberband.h - rubberband - either own implementation for Qt3 or wrapper for
 *                             Qt4
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _RUBBERBAND_H
#define _RUBBERBAND_H

#include <QtGui/QRubberBand>
#include <QtCore/QVector>


class selectableObject : public QWidget
{
	Q_OBJECT
public:
	selectableObject( QWidget * _parent ) :
		QWidget( _parent ),
		m_selected( false )
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

	inline bool isSelected() const
	{
		return( m_selected );
	}


public slots:
	virtual void update()
	{
		QWidget::update();
	}


private:
	bool m_selected;

} ;




class rubberBand : public QRubberBand
{
public:
	rubberBand( QWidget * _parent );
	virtual ~rubberBand();

	QVector<selectableObject *> selectedObjects() const;


protected:
	virtual void resizeEvent( QResizeEvent * _re );

private:
	QVector<selectableObject *> selectableObjects() const;

};


#endif

