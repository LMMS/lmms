/*
 * AutomatableSlider.h - class AutomatableSlider, a QSlider with automation
 *
 * Copyright (c) 2006-2008 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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


#ifndef AUTOMATABLE_SLIDER_H
#define AUTOMATABLE_SLIDER_H

#include <QSlider>

#include "AutomatableModelView.h"



class AutomatableSlider : public QSlider, public IntModelView
{
	Q_OBJECT
public:
	AutomatableSlider( QWidget * _parent, const QString & _name = QString::null );
	virtual ~AutomatableSlider();

	bool showStatus()
	{
		return( m_showStatus );
	}


signals:
	void logicValueChanged( int _value );
	void logicSliderMoved( int _value );


protected:
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );
	virtual void wheelEvent( QWheelEvent * _me );

	virtual void modelChanged();


private:
	bool m_showStatus;


private slots:
	void changeValue( int _value );
	void moveSlider( int _value );
	void updateSlider();

} ;


typedef IntModel sliderModel;


#endif
