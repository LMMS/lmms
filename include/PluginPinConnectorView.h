/*
 * PluginPinConnectorView.h - Displays pin connectors
 *
 * Copyright (c) 2025 Dalton Messmer <messmer.dalton/at/gmail.com>
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
#include "lmms_export.h"
#include "ModelView.h"

class QPixmap;
//class QScrollArea;

namespace lmms
{

class BoolModel;
class PluginPinConnector;

namespace gui
{

class SubWindow;

class LMMS_EXPORT PluginPinConnectorView
	: public QWidget
	, public ModelView
{
	Q_OBJECT

public:
	PluginPinConnectorView(PluginPinConnector* model, QWidget* parent);

	auto sizeHint() const -> QSize override;
	auto minimumSizeHint() const -> QSize override;

	void toggleVisibility();
	void closeWindow();

protected:
	void paintEvent(QPaintEvent* pe) override;

private:
	class MatrixView;

	void updateGeometry();

	SubWindow* m_subWindow = nullptr;
	//QScrollArea* m_scrollArea = nullptr;

	MatrixView* m_inView = nullptr;
	MatrixView* m_outView = nullptr;
};

} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_PLUGIN_PIN_CONNECTOR_VIEW_H
