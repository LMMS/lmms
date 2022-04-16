/*
 * MixerLineLcdSpinBox.h - a specialization of LcdSpnBox for setting mixer channels
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef MIXER_LINE_LCD_SPIN_BOX_H
#define MIXER_LINE_LCD_SPIN_BOX_H

#include "LcdSpinBox.h"

namespace lmms::gui
{


class TrackView;


class MixerLineLcdSpinBox : public LcdSpinBox
{
	Q_OBJECT
public:
	MixerLineLcdSpinBox(int numDigits, QWidget * parent, const QString& name, TrackView * tv = nullptr) :
		LcdSpinBox(numDigits, parent, name), m_tv(tv)
	{}
	virtual ~MixerLineLcdSpinBox() {}

	void setTrackView(TrackView * tv);

protected:
	void mouseDoubleClickEvent(QMouseEvent* event) override;
	void contextMenuEvent(QContextMenuEvent* event) override;

private:
	TrackView * m_tv;
};


} // namespace lmms::gui

#endif
