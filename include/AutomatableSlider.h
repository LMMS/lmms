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

#ifndef LMMS_GUI_AUTOMATABLE_SLIDER_H
#define LMMS_GUI_AUTOMATABLE_SLIDER_H

#include <QSlider>

#include "AutomatableModelView.h"


namespace lmms::gui
{

class AutomatableSlider : public QSlider, public IntModelView
{
	Q_OBJECT
public:
	AutomatableSlider( QWidget * _parent, const QString & _name = QString() );
	~AutomatableSlider() override = default;

	bool showStatus()
	{
		return( m_showStatus );
	}


signals:
	void logicValueChanged( int _value );
	void logicSliderMoved( int _value );


protected:
	void contextMenuEvent( QContextMenuEvent * _me ) override;
	void mousePressEvent( QMouseEvent * _me ) override;
	void mouseReleaseEvent( QMouseEvent * _me ) override;
	void wheelEvent( QWheelEvent * _me ) override;

	void modelChanged() override;


private:
	bool m_showStatus;


private slots:
	void changeValue( int _value );
	void moveSlider( int _value );
	void updateSlider();

} ;


using sliderModel = IntModel;

} // namespace lmms::gui

#endif // LMMS_GUI_AUTOMATABLE_SLIDER_H
