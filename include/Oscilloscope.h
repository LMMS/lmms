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

#ifndef LMMS_GUI_OSCILLOSCOPE_H
#define LMMS_GUI_OSCILLOSCOPE_H

#include <QWidget>
#include <QPixmap>

#include "LmmsTypes.h"

namespace lmms
{

class SampleFrame;

}

namespace lmms::gui
{


class Oscilloscope : public QWidget
{
	Q_OBJECT
public:
	Q_PROPERTY( QColor leftChannelColor READ leftChannelColor WRITE setLeftChannelColor )
	Q_PROPERTY( QColor rightChannelColor READ rightChannelColor WRITE setRightChannelColor )
	Q_PROPERTY( QColor otherChannelsColor READ otherChannelsColor WRITE setOtherChannelsColor )
	Q_PROPERTY( QColor clippingColor READ clippingColor WRITE setClippingColor )

	Oscilloscope( QWidget * _parent );
	~Oscilloscope() override;

	void setActive( bool _active );

	QColor const & leftChannelColor() const;
	void setLeftChannelColor(QColor const & leftChannelColor);

	QColor const & rightChannelColor() const;
	void setRightChannelColor(QColor const & rightChannelColor);

	QColor const & otherChannelsColor() const;
	void setOtherChannelsColor(QColor const & otherChannelsColor);

	QColor const & clippingColor() const;
	void setClippingColor(QColor const & clippingColor);


protected:
	void paintEvent( QPaintEvent * _pe ) override;
	void mousePressEvent( QMouseEvent * _me ) override;


protected slots:
	void updateAudioBuffer(const lmms::SampleFrame* buffer);

private:
	bool clips(float level) const;

private:
	QPixmap m_background;
	QPointF * m_points;

	SampleFrame* m_buffer;
	bool m_active;

	QColor m_leftChannelColor;
	QColor m_rightChannelColor;
	QColor m_otherChannelsColor;
	QColor m_clippingColor;
} ;


} // namespace lmms::gui

#endif // LMMS_GUI_OSCILLOSCOPE_H
