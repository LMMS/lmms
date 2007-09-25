/*
 * visualization_widget.h - widget for visualization of sound-data
 *
 * Copyright (c) 2005-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of"the GNU General Public
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


#ifndef _VISUALIZATION_WIDGET
#define _VISUALIZATION_WIDGET

#include <QtGui/QWidget>
#include <QtGui/QPixmap>

#include "mixer.h"


class QTimer;


class visualizationWidget : public QWidget
{
	Q_OBJECT
public:
	enum visualizationTypes
	{
		SIMPLE		// add more here
	} ;
	visualizationWidget( const QPixmap & _bg, QWidget * _parent,
					visualizationTypes _vtype = SIMPLE );
	virtual ~visualizationWidget();


protected:	
	void paintEvent( QPaintEvent * _pe );
	void mousePressEvent( QMouseEvent * _me );


protected slots:
	void updateAudioBuffer( void );


private:
	QPixmap s_background;

	bool m_enabled;
	surroundSampleFrame * m_buffer;

	QTimer * m_updateTimer;

} ;

#endif
