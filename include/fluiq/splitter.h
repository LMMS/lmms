/*
 * splitter.h - header file for FLUIQ::Splitter
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _FLUIQ_SPLITTER_H
#define _FLUIQ_SPLITTER_H

#include <QtGui/QWidget>

class QBoxLayout;


namespace FLUIQ
{


class Splitter : public QWidget
{
	Q_OBJECT
public:
	Splitter( Qt::Orientation _o, QWidget * _parent = NULL );
	virtual ~Splitter();

	void addWidget( QWidget * _widget );

	int indexOf( QWidget * _widget ) const;
	QWidget * widget( int _idx );

	int count() const
	{
		return m_children.count();
	}

	Qt::Orientation orientation() const
	{
		return m_orientation;
	}


private:
	Qt::Orientation m_orientation;
	QList<QWidget *> m_children;
	QBoxLayout * m_mainLayout;

} ;


}

#endif
