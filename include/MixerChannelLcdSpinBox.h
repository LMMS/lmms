/*
 * MixerChannelLcdSpinBox.h - a specialization of LcdSpnBox for setting mixer channels
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

#ifndef LMMS_GUI_MIXER_CHANNEL_LCD_SPIN_BOX_H
#define LMMS_GUI_MIXER_CHANNEL_LCD_SPIN_BOX_H

#include "LcdSpinBox.h"

namespace lmms::gui
{


class TrackView;


class MixerChannelLcdSpinBox : public LcdSpinBox
{
	Q_OBJECT
public:
	MixerChannelLcdSpinBox(int numDigits, QWidget * parent, const QString& name, TrackView * tv = nullptr) :
		LcdSpinBox(numDigits, parent, name), m_tv(tv)
	{}
	~MixerChannelLcdSpinBox() override = default;

	void setTrackView(TrackView * tv);

protected:
	void mouseDoubleClickEvent(QMouseEvent* event) override;
	void contextMenuEvent(QContextMenuEvent* event) override;

private:
	void enterValue();

	TrackView * m_tv;
};


} // namespace lmms::gui

#endif // LMMS_GUI_MIXER_CHANNEL_LCD_SPIN_BOX_H
