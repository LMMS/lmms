/*
 * Oscilloscope.h
 *
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


#ifndef _OSCILLOSCOPE
#define _OSCILLOSCOPE

#include <QWidget>
#include <QPixmap>

#include "lmms_basics.h"


class Oscilloscope : public QWidget
{
	Q_OBJECT
public:
	Q_PROPERTY( QColor normalColor READ normalColor WRITE setNormalColor )
	Q_PROPERTY( QColor clippingColor READ clippingColor WRITE setClippingColor )

	Oscilloscope( QWidget * _parent );
	virtual ~Oscilloscope();

	void setActive( bool _active );

	QColor const & normalColor() const;
	void setNormalColor(QColor const & normalColor);

	QColor const & clippingColor() const;
	void setClippingColor(QColor const & clippingColor);


protected:
	void paintEvent( QPaintEvent * _pe ) override;
	void mousePressEvent( QMouseEvent * _me ) override;


protected slots:
	void updateAudioBuffer( const surroundSampleFrame * buffer );

private:
	QColor const & determineLineColor(float level) const;

private:
	QPixmap m_background;
	QPointF * m_points;

	sampleFrame * m_buffer;
	bool m_active;

	QColor m_normalColor;
	QColor m_clippingColor;
} ;

#endif
