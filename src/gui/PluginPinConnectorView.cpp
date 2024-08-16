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

#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>

#include "gui_templates.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "PluginPinConnector.h"
#include "StringPairDrag.h"
#include "SubWindow.h"

namespace lmms::gui
{

namespace
{

constexpr auto GridMargin = 2;
constexpr auto CenterMargin = QSize{48, 0};
constexpr auto WindowMarginTop = QSize{0, 160};
constexpr auto WindowMarginBottom = QSize{0, 48};
constexpr auto WindowMarginSide = QSize{144, 0};
constexpr auto WindowMarginTotal = WindowMarginTop + WindowMarginBottom + WindowMarginSide + WindowMarginSide;
constexpr auto DefaultWindowSize = QSize{400, 256};

} // namespace

PluginPinConnectorView::PluginPinConnectorView(QWidget* parent)
	: QWidget{parent} // new QScrollArea{parent}
	//: QScrollArea{parent}
	, ModelView{nullptr, this}
{
	/*
	updateGeometry();

	// The initial window size should not exceed DefaultWindowSize but can be smaller
	const auto initialSize = QSize {
		std::min(m_minSize.width(), DefaultWindowSize.width()),
		std::min(m_minSize.height(), DefaultWindowSize.height())
	};
	*/

#if 0
	m_scrollArea = new QScrollArea{};
	m_scrollArea->setBackgroundRole(QPalette::Dark);
	m_scrollArea->setWidget(this);
	//m_scrollArea->setMinimumSize(initialSize);
#endif

	setWindowTitle(tr("Plugin Pin Connector"));
	m_subWindow = getGUI()->mainWindow()->addWindowedWidget(this);
	m_subWindow->setAttribute(Qt::WA_DeleteOnClose, false);
	setWindowIcon(embed::getIconPixmap("tool"));

	// No maximize button
	Qt::WindowFlags flags = m_subWindow->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	m_subWindow->setWindowFlags(flags);


	//auto layout = new QHBoxLayout{this};
	//layout->addWidget()


	//m_subWindow->setMinimumSize(initialSize);
	//setMinimumSize(initialSize);

	setMinimumSize(DefaultWindowSize);


	//m_subWindow->setMinimumSize(initialSize);

	//m_subWindow->show();
}

auto PluginPinConnectorView::sizeHint() const -> QSize
{
	//if (m_minSize.width() > DefaultWindowSize.width()) { return m_minSize; }
	//if (m_minSize.height() > DefaultWindowSize.height()) { return m_minSize; }

	//return DefaultWindowSize;


	return QSize {
		std::min(m_minSize.width(), DefaultWindowSize.width()),
		std::min(m_minSize.height(), DefaultWindowSize.height())
	};

}

void PluginPinConnectorView::toggleVisibility()
{
	if (m_subWindow->isVisible())
	{
		m_subWindow->hide();
		m_scrollArea->hide();
		hide();
	}
	else
	{
		updateGeometry();
		show();
		m_scrollArea->show();
		m_subWindow->show();
	}
}

void PluginPinConnectorView::update()
{
	updateGeometry();
	QWidget::update();
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
		const int xIdx = relMousePos.x() / (buttonW + GridMargin);
		const int yIdx = relMousePos.y() / (buttonH + GridMargin);

		// Check if within margin
		int relPos = relMousePos.x() - xIdx * buttonW;
		if (relPos >= buttonW || relPos <= 0) { return false; }
		relPos = relMousePos.y() - yIdx * buttonH;
		if (relPos >= buttonH || relPos <= 0) { return false; }
		
		BoolModel* model = pins.at(yIdx).at(xIdx);

		if (me->modifiers() & Qt::ControlModifier)
		{
			// Taken from AutomatableModelView::mousePressEvent
			new gui::StringPairDrag{"automatable_model", QString::number(model->id()),
				getIcon(*model, yIdx, xIdx), this};
		}
		else
		{
			model->setValue(!model->value());
		}

		me->accept();

		return true;
	};

	if (!handleClick(pinConnector->in().pinMap(), m_inRect))
	{
		if (!handleClick(pinConnector->out().pinMap(), m_outRect))
		{
			me->ignore();
		}
	}
}

void PluginPinConnectorView::paintEvent(QPaintEvent*)
{
	auto p = QPainter{this};
	p.setRenderHint(QPainter::Antialiasing);
	p.fillRect(rect(), p.background());

#if 0
#ifndef NDEBUG
	p.setPen(palette().text().color());
	const auto marginUpperLeft = QPoint {
		(WindowMarginTop + WindowMarginSide).width(),
		(WindowMarginTop + WindowMarginSide).height()
	};
	const auto marginSize = QSize {
		width() - WindowMarginSide.width() * 2,
		height() - WindowMarginTop.height() - WindowMarginBottom.height()
	};
	auto margins = QRect{marginUpperLeft, marginSize};
	p.drawRect(margins);
#endif
#endif

	const auto* pinConnector = castModel<PluginPinConnector>();
	if (!pinConnector) { return; }

	const auto& pinsIn = pinConnector->in().pinMap();
	const auto& pinsOut = pinConnector->out().pinMap();

	const auto buttonW = m_buttonOn.width();
	const auto buttonH = m_buttonOn.height();

	auto drawMatrix = [&](const PluginPinConnector::PinMap& pins, QRect rect) {
		if (pins.empty() || pins[0].empty()) { return; }
		if (pinConnector->trackChannelsUsed() == 0) { return; }

		auto drawXY = QPoint{};
		drawXY.ry() = rect.y();

		for (std::size_t tcIdx = 0; tcIdx < pins.size(); ++tcIdx)
		{
			drawXY.rx() = rect.x();

			auto& pluginChannels = pins[tcIdx];
			for (std::size_t pcIdx = 0; pcIdx < pluginChannels.size(); ++pcIdx)
			{
				BoolModel* pin = pluginChannels[pcIdx];
				p.drawPixmap(drawXY, getIcon(*pin, tcIdx, pcIdx));

				drawXY.rx() += buttonW + GridMargin;
			}

			drawXY.ry() += buttonH + GridMargin;
		}

#ifndef NDEBUG
		const auto expected = rect.bottomRight();
		const auto actual = drawXY - QPoint{GridMargin + 1, GridMargin + 1};
		//std::cout << "expected: {" << expected.x() << ", " << expected.y() << "}\n";
		//std::cout << "actual: {" << actual.x() << ", " << actual.y() << "}\n";
		assert(actual == expected);
#endif
	};

	drawMatrix(pinsIn, m_inRect);
	drawMatrix(pinsOut, m_outRect);

	QFont f = adjustedToPixelSize(font(), 16);
	f.setBold(false);
	p.setFont(f);
	p.setPen(palette().text().color());

	// Draw track channel text
	const auto xwIn = std::pair{0, m_inRect.left() - 4};
	const auto xwOut = std::pair{m_outRect.right() + 4, width() - m_outRect.right() - 4};
	int yPos = m_inRect.y();
	for (unsigned idx = 0; idx < pinConnector->trackChannelsUsed(); ++idx)
	{
		if (pinConnector->in().channelCount() != 0)
		{
			p.drawText(xwIn.first, yPos, xwIn.second, buttonH, Qt::AlignRight,
				QString::fromUtf16(u"%1 \U0001F82E").arg(idx + 1));
		}

		if (pinConnector->out().channelCount() != 0)
		{
			p.drawText(xwOut.first, yPos, xwOut.second, buttonH, Qt::AlignLeft,
				QString::fromUtf16(u"\U0001F82E %1").arg(idx + 1));
		}

		yPos += buttonH + GridMargin;
	}

	p.save();

	// Draw plugin channel text (in)
	yPos = m_inRect.top() - 4;
	int xPos = m_inRect.x() + GridMargin;
	for (int idx = 0; idx < pinConnector->in().channelCount(); ++idx)
	{
		const auto transform = QTransform{}.translate(xPos, yPos).rotate(-90);
		p.setTransform(transform);

		const auto name = pinConnector->in().channelName(idx);
		p.drawText(0, 0, buttonW, yPos, Qt::AlignLeft, name);

		xPos += buttonW + GridMargin;
	}

	// Draw plugin channel text (out)
	xPos = m_outRect.x() + GridMargin;
	for (int idx = 0; idx < pinConnector->out().channelCount(); ++idx)
	{
		const auto transform = QTransform{}.translate(xPos, yPos).rotate(-90);
		p.setTransform(transform);

		const auto name = pinConnector->out().channelName(idx);
		p.drawText(0, 0, buttonW, yPos, Qt::AlignLeft, name);

		xPos += buttonW + GridMargin;
	}

	p.restore();
}

void PluginPinConnectorView::updateGeometry()
{
	const auto inSize = calculateMatrixSize(true);
	const auto outSize = calculateMatrixSize(false);

	const auto centerMargin = (inSize.isEmpty() || outSize.isEmpty()) ? QSize{} : CenterMargin;

	m_minSize = WindowMarginTotal + centerMargin + inSize + outSize;

	

	//setMinimumSize(m_minSize);
	//m_subWindow->setMinimumSize(m_minSize);

	//std::cout << "m_minSize: {" << m_minSize.width() << ", " << m_minSize.height() << "}\n";

	const auto extra = QSize {
		std::max(DefaultWindowSize.width() - m_minSize.width(), 0) / 2,
		std::max(DefaultWindowSize.height() - m_minSize.height(), 0) / 2
	};

	//m_minSize.rwidth() = std::max(DefaultWindowSize.width(), m_minSize.width());
	//m_minSize.rheight() = std::max(DefaultWindowSize.height(), m_minSize.height());

	const auto inMatrixPos = WindowMarginTop + WindowMarginSide + extra;
	const auto outMatrixPos = inMatrixPos + centerMargin + QSize{inSize.width(), 0};

	m_inRect = QRect{inMatrixPos.width(), inMatrixPos.height(), inSize.width(), inSize.height()};
	m_outRect = QRect{outMatrixPos.width(), outMatrixPos.height(), outSize.width(), outSize.height()};

	//std::cout << "m_inRect: {" << m_inRect.x() << ", " << m_inRect.y() << "}, {" << m_inRect.width() << ", " << m_inRect.height() << "}\n";
	//std::cout << "m_outRect: {" << m_outRect.x() << ", " << m_outRect.y() << "}, {" << m_outRect.width() << ", " << m_outRect.height() << "}\n";
}

void updatePositions()
{

}

auto PluginPinConnectorView::calculateMatrixSize(bool inMatrix) const -> QSize
{
	const auto* pc = castModel<PluginPinConnector>();
	if (!pc) { return QSize{}; }

	const auto tcc = static_cast<int>(pc->trackChannelsUsed());
	if (tcc == 0) { return QSize{}; }

	const int pcc = inMatrix ? pc->in().channelCount() : pc->out().channelCount();
	if (pcc == 0) { return QSize{}; }

	const int pcSize = (pcc * m_buttonOn.width()) + (pcc - 1) * GridMargin;
	const int tcSize = (tcc * m_buttonOn.height()) + (tcc - 1) * GridMargin;

	return {pcSize, tcSize};
}

auto PluginPinConnectorView::getIcon(const BoolModel& model, int trackChannel, int pluginChannel) -> const QPixmap&
{
	// TODO: Alternate b/w black and gray icons?
	(void)trackChannel;
	(void)pluginChannel;
	const auto* offIcon = &m_buttonOffBlack;
	return model.value() ? m_buttonOn : *offIcon;
}

} // namespace lmms::gui
