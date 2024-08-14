/*
 * PluginPinConnectorView.h - Displays pin connectors
 *
 * Copyright (c) 2024 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#ifndef LMMS_GUI_PLUGIN_PIN_CONNECTOR_VIEW_H
#define LMMS_GUI_PLUGIN_PIN_CONNECTOR_VIEW_H

#include <QWidget>

#include "embed.h"
#include "ModelView.h"

namespace lmms
{

namespace gui
{

class PluginPinConnectorView
	: public QWidget
	, public ModelView
{
	Q_OBJECT

public:
	PluginPinConnectorView(QWidget* parent);

	auto sizeHint() const -> QSize override;

	auto getChannelCountText() const -> QString;

protected:
	void mousePressEvent(QMouseEvent* me) override;
	void paintEvent(QPaintEvent* pe) override;

private:
	QRect m_pinsInRect;
	QRect m_pinsOutRect;

	QPixmap m_buttonOffBlack = embed::getIconPixmap("step_btn_off");
	QPixmap m_buttonOffGray = embed::getIconPixmap("step_btn_off_light");
	//QPixmap m_buttonOn0 = embed::getIconPixmap("step_btn_on_0");
	QPixmap m_buttonOn = embed::getIconPixmap("step_btn_on_200");
};

} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_PLUGIN_PIN_CONNECTOR_VIEW_H
