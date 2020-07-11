/*
 * SimpleVisualizationWidget.h - widget for visualization of sound-data
 *
 * Copyright (c) 2019 Lathigos <lathigos/at/tutanota.com>
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of LMMS - https://lmms.io
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


#ifndef SIMPLE_VISUALIZATION_WIDGET
#define SIMPLE_VISUALIZATION_WIDGET

#include <QList>
#include <QPixmap>
#include <QWidget>
#include <QTime>

#include "lmms_basics.h"

class ProgressBar;

class SimpleVisualizationWidget : public QWidget
{
	Q_OBJECT
public:
	SimpleVisualizationWidget( QWidget * _parent );
	virtual ~SimpleVisualizationWidget();


protected slots:
	void updateVisualization();
	void updateAudioBuffer( const surroundSampleFrame * buffer );

private:
	ProgressBar * m_progressBarLeft;
	ProgressBar * m_progressBarRight;
	
	sampleFrame * m_buffer;
	
	float m_peakLeft = 0.0f;
	float m_peakRight = 0.0f;
	
	int frameCounter = 0;
} ;

#endif
