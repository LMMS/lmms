/*
 * PluginPinConnectorView.cpp - Displays pin connectors
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

#include "PluginPinConnectorView.h"

#include <QMouseEvent>
#include <QPainter>

//#include "AutomatableModelView.h"
#include "gui_templates.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "PluginPinConnector.h"
#include "StringPairDrag.h"

namespace lmms
{

namespace gui
{

namespace
{

constexpr auto MarginSize = 2;

} // namespace

PluginPinConnectorView::PluginPinConnectorView(QWidget* parent)
	: QWidget{parent}
	, ModelView{nullptr, this}
{
	setWindowTitle(tr("Plugin Pin Connector"));
	getGUI()->mainWindow()->addWindowedWidget(this);
	setAttribute(Qt::WA_DeleteOnClose, false);
	setWindowIcon(embed::getIconPixmap("tool"));

	// No maximize button
	Qt::WindowFlags flags = parentWidget()->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	parentWidget()->setWindowFlags(flags);
}

auto PluginPinConnectorView::sizeHint() const -> QSize
{
	return QSize{512, 256};
}

auto PluginPinConnectorView::getChannelCountText() const -> QString
{
	const auto* pinConnector = castModel<PluginPinConnector>();
	if (!pinConnector) { return QString{}; }

	constexpr int totalTrackChannels = 2; // TODO: Move somewhere else

	const auto inText = pinConnector->channelCountIn() > totalTrackChannels
		? QString{"%1/%2"}.arg(totalTrackChannels).arg(pinConnector->channelCountIn())
		: QString{"%1"}.arg(pinConnector->channelCountIn());

	const auto outText = pinConnector->channelCountOut() > totalTrackChannels
		? QString{"%1/%2"}.arg(totalTrackChannels).arg(pinConnector->channelCountOut())
		: QString{"%1"}.arg(pinConnector->channelCountOut());

	return QString{tr("%1 in %2 out")}.arg(inText).arg(outText);
}

void PluginPinConnectorView::mousePressEvent(QMouseEvent* me)
{
	if (me->button() != Qt::LeftButton) { return; }

	const auto* pinConnector = castModel<PluginPinConnector>();
	if (!pinConnector) { return; }

	const int buttonW = m_buttonOn.width();
	const int buttonH = m_buttonOn.height();

	auto handleClick = [&](const PluginPinConnector::PinMap& pins, QRect rect) -> bool {
		if (!rect.contains(me->pos(), true)) { return false; }

		const auto relMousePos = me->pos() - rect.topLeft();
		const int xIdx = relMousePos.x() / (buttonW + MarginSize);
		const int yIdx = relMousePos.y() / (buttonH + MarginSize);

		// Check if within margin
		int relPos = relMousePos.x() - xIdx * buttonW;
		if (relPos >= buttonW || relPos <= 0) { return false; }
		relPos = relMousePos.y() - yIdx * buttonH;
		if (relPos >= buttonH || relPos <= 0) { return false; }
		
		BoolModel* model = pins.at(yIdx).at(xIdx);

		if (me->modifiers() & Qt::ControlModifier)
		{
			// Taken from AutomatableModelView::mousePressEvent
			new gui::StringPairDrag{"automatable_model", QString::number(model->id()), QPixmap{}, widget()};
		}
		else
		{
			model->setValue(!model->value());
			update();
		}

		me->accept();

		return true;
	};

	if (!handleClick(pinConnector->pinMapIn(), m_pinsInRect))
	{
		if (!handleClick(pinConnector->pinMapOut(), m_pinsOutRect))
		{
			me->ignore();
		}
	}
}

void PluginPinConnectorView::paintEvent(QPaintEvent*)
{
	

	auto p = QPainter{this};
	p.setRenderHint(QPainter::Antialiasing);



	// Draw the graph background
	//p.drawPixmap(rect(), m_lfoGraph);

	const auto* pinConnector = castModel<PluginPinConnector>();
	if (!pinConnector) { return; }

	const auto& pinsIn = pinConnector->pinMapIn();
	const auto& pinsOut = pinConnector->pinMapOut();

	//const auto w = width();
	//const auto h = height();
	const auto buttonW = m_buttonOn.width();
	const auto buttonH = m_buttonOn.height();

	p.fillRect(rect(), Qt::yellow);

	auto drawGrid = [&](const PluginPinConnector::PinMap& pins, QPoint xy) -> QRect {
		if (pins.empty() || pins[0].empty()) { return QRect{xy, xy}; }

		const auto* offIcon = &m_buttonOffBlack;

		auto drawXY = QPoint{};
		drawXY.ry() = xy.y();

		for (std::size_t tcIdx = 0; tcIdx < pins.size(); ++tcIdx)
		{
			drawXY.rx() = xy.x();

			auto& pluginChannels = pins[tcIdx];
			for (std::size_t pcIdx = 0; pcIdx < pluginChannels.size(); ++pcIdx)
			{
				const BoolModel* pin = pluginChannels[pcIdx];
				p.drawPixmap(drawXY, pin->value() ? m_buttonOn : *offIcon);

				// TODO: Alternate b/w black and gray icons

				drawXY.rx() += buttonW + MarginSize;
			}

			drawXY.ry() += buttonH + MarginSize;
		}

		return QRect{xy, drawXY - QPoint{MarginSize, MarginSize}};
	};

	m_pinsInRect = drawGrid(pinsIn, QPoint{5, 10});
	m_pinsOutRect = drawGrid(pinsOut, m_pinsInRect.topRight() + QPoint{5, 0});
}


} // namespace gui

} // namespace lmms
