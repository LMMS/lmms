/*
 * PinConnector.cpp - View for AudioPortsModel
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

#include "PinConnector.h"

#include <QBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QSizePolicy>
#include <QStyle>

#include "FontHelper.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "AudioPortsModel.h"
#include "StringPairDrag.h"
#include "SubWindow.h"

namespace lmms::gui
{

namespace
{

constexpr auto CenterMargin = QSize{32, 0};
constexpr auto WindowMarginTop = QSize{0, 112};
constexpr auto WindowMarginBottom = QSize{0, 16};
constexpr auto WindowMarginSide = QSize{32, 0};
constexpr auto WindowMarginTotal = WindowMarginTop + WindowMarginBottom + WindowMarginSide + WindowMarginSide;
constexpr auto MinimumAllowedWindowSize = QSize{112, 112};

} // namespace


PinConnector::PinConnector(AudioPortsModel* model)
	: QWidget{}
	, ModelView{model, this}
	, m_inView{new MatrixView{this, model->in()}}
	, m_outView{new MatrixView{this, model->out()}}
{
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	assert(model != nullptr);
	connect(model, &AudioPortsModel::propertiesChanged, this, &PinConnector::updateProperties);

	m_scrollArea = new QScrollArea{};
	m_scrollArea->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

	if (const Model* parentModel = model->parentModel())
	{
		m_scrollArea->setWindowTitle(tr("%1 Pin Connector").arg(parentModel->fullDisplayName()));
	}
	else
	{
		m_scrollArea->setWindowTitle(tr("Pin Connector"));
	}

	m_subWindow = getGUI()->mainWindow()->addWindowedWidget(m_scrollArea);
	m_subWindow->setAttribute(Qt::WA_DeleteOnClose, false);
	m_subWindow->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	m_subWindow->setWindowIcon(embed::getIconPixmap("tool"));

	// No maximize button
	auto flags = m_subWindow->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	m_subWindow->setWindowFlags(flags);

	// Fixed-width space between the input matrix and the output matrix.
	//   When there's only one matrix, the spacer's size is zero.
	m_spacer = new QWidget{};
	m_spacer->setFixedWidth(getSpacerWidth());
	m_spacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

	// Horizontal layout
	auto hLayout = new QHBoxLayout{};
	hLayout->addStretch(1);
	hLayout->addWidget(m_inView);
	hLayout->addWidget(m_spacer);
	hLayout->addWidget(m_outView);
	hLayout->addStretch(1);

	// Vertical layout
	auto vLayout = new QVBoxLayout{this};
	vLayout->addStretch(1);
	vLayout->addLayout(hLayout);
	vLayout->addSpacing(WindowMarginBottom.height());

	// Add widget to the scroll area
	m_scrollArea->setWidget(this);
	m_scrollArea->setWidgetResizable(true);
	m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	// Remember size when pin connector is hidden
	auto sp = m_subWindow->sizePolicy();
	sp.setRetainSizeWhenHidden(true);
	m_subWindow->setSizePolicy(sp);
	sp = m_scrollArea->sizePolicy();
	sp.setRetainSizeWhenHidden(true);
	m_scrollArea->setSizePolicy(sp);

	updateProperties();

	m_subWindow->setMinimumSize(MinimumAllowedWindowSize);
	m_subWindow->resize(m_subWindow->maximumSize());

	m_scrollArea->show();
}

PinConnector::~PinConnector()
{
	closeWindow();
}

auto PinConnector::sizeHint() const -> QSize
{
	const auto inSize = m_inView->size();
	const auto outSize = m_outView->size();

	const auto centerMargin = (inSize.isEmpty() || outSize.isEmpty()) ? QSize{0, 0} : CenterMargin;

	const auto combinedMatrixSize = QSize {
		inSize.width() + outSize.width(),
		std::max(inSize.height(), outSize.height())
	};

	return WindowMarginTotal + centerMargin + combinedMatrixSize;
}

auto PinConnector::minimumSizeHint() const -> QSize
{
	const auto minSize = sizeHint();
	return QSize {
		std::min(minSize.width(), MinimumAllowedWindowSize.width()),
		std::min(minSize.height(), MinimumAllowedWindowSize.height())
	};
}

void PinConnector::toggleVisibility()
{
	if (m_subWindow->isVisible())
	{
		m_subWindow->hide();
		m_scrollArea->hide();
	}
	else
	{
		updateProperties();
		m_scrollArea->show();
		m_subWindow->show();
	}
}

void PinConnector::closeWindow()
{
	m_subWindow->setAttribute(Qt::WA_DeleteOnClose);
	m_subWindow->close();
}

void PinConnector::paintEvent(QPaintEvent*)
{
	auto p = QPainter{this};
	p.setRenderHint(QPainter::Antialiasing);
	p.fillRect(rect(), p.background());

	auto* model = castModel<AudioPortsModel>();
	if (!model) { return; }

	constexpr int cellSize = MatrixView::cellSize();

	auto f = adjustedToPixelSize(font(), static_cast<int>(cellSize * 0.9));
	f.setBold(false);
	p.setFont(f);
	p.setPen(palette().text().color());

	// Get matrix postions/sizes
	auto inMatrixRect = m_inView->rect();
	inMatrixRect.moveTo(m_inView->mapTo(this, QPoint{0, 0}));
	auto outMatrixRect = m_outView->rect();
	outMatrixRect.moveTo(m_outView->mapTo(this, QPoint{0, 0}));

	// Draw track channel text (in)
	if (model->in().channelCount() != 0)
	{
		const auto width = inMatrixRect.left() - 4;
		int yPos = inMatrixRect.y() + MatrixView::BorderWidth - 2;
		for (unsigned idx = 0; idx < model->trackChannelCount(); ++idx)
		{
			const auto name = trackChannelName(*model, idx);
			p.drawText(0, yPos, width, cellSize, Qt::AlignRight,
				QString::fromUtf16(u"%1 \U0001F82E").arg(name));

			yPos += cellSize;
		}
	}

	// Draw track channel text (out)
	if (model->out().channelCount() != 0)
	{
		const int xPos = outMatrixRect.right() + 4;
		int yPos = outMatrixRect.y() + MatrixView::BorderWidth - 2;
		const int width = this->width() - outMatrixRect.right() - 4;
		for (unsigned idx = 0; idx < model->trackChannelCount(); ++idx)
		{
			const auto name = trackChannelName(*model, idx);
			p.drawText(xPos, yPos, width, cellSize, Qt::AlignLeft,
				QString::fromUtf16(u"\U0001F82E %1").arg(name));

			yPos += cellSize;
		}
	}

	p.save();

	// TODO: Draw instruments with sidechain inputs differently?

	// Draw processor channel text (in)
	int yPos = inMatrixRect.top() - 4;
	int xPos = inMatrixRect.x() + MatrixView::BorderWidth - 2;
	for (int idx = 0; idx < model->in().channelCount(); ++idx)
	{
		const auto transform = QTransform{}.translate(xPos, yPos).rotate(-90);
		p.setTransform(transform);

		const auto name = model->in().channelName(idx);
		p.drawText(0, 0, yPos, cellSize * 2, Qt::AlignLeft,
			QString::fromUtf16(u"\U0001F82E %1").arg(name));

		xPos += cellSize;
	}

	// Draw processor channel text (out)
	yPos = outMatrixRect.top() - 4;
	xPos = outMatrixRect.x() + MatrixView::BorderWidth - 2;
	for (int idx = 0; idx < model->out().channelCount(); ++idx)
	{
		const auto transform = QTransform{}.translate(xPos, yPos).rotate(-90);
		p.setTransform(transform);

		const auto name = model->out().channelName(idx);
		p.drawText(0, 0, yPos, cellSize * 2, Qt::AlignLeft,
			QString::fromUtf16(u"\U0001F82C %1").arg(name));

		xPos += cellSize;
	}

	p.restore();
}

auto PinConnector::trackChannelName(const AudioPortsModel& model, track_ch_t channel) const -> QString
{
	// TODO: Custom track channel names?
	return QString{"%1"}.arg(channel + 1);
}

auto PinConnector::getSpacerWidth() const -> int
{
	auto model = castModel<AudioPortsModel>();
	assert(model != nullptr);

	const bool singleMatrix = model->trackChannelCount() == 0
			|| model->in().channelCount() == 0
			|| model->out().channelCount() == 0;

	return singleMatrix ? 0 : CenterMargin.width();
}

auto PinConnector::getMaximumWindowSize() const -> QSize
{
	// Content size
	QSize size = sizeHint();

	// Account for scrollbars
	const int scrollbarExtent = m_scrollArea->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
	size += QSize{scrollbarExtent, scrollbarExtent};

	// Account for scroll area frame (borders)
	size += m_scrollArea->size() - m_scrollArea->viewport()->size();

	// A little extra space just to make sure the scrollbars aren't shown
	//   when the window is at its maximum size
	size += QSize{16, 16};

	return size;
}

void PinConnector::updateProperties()
{
	m_inView->updateProperties(this);
	m_outView->updateProperties(this);

	m_spacer->setFixedWidth(getSpacerWidth());

	// Update maximum window size
	const auto windowSize = getMaximumWindowSize();
	m_scrollArea->resize(windowSize);
	m_subWindow->setMaximumSize(windowSize);

	update();
}

PinConnector::MatrixView::MatrixView(const PinConnector* view, AudioPortsModel::Matrix& matrix)
	: QWidget{}
	, m_matrix{&matrix}
{
	auto model = view->castModel<AudioPortsModel>();
	assert(model != nullptr);
	connect(model, &AudioPortsModel::dataChanged, this, static_cast<void(QWidget::*)()>(&QWidget::update));

	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	setMouseTracking(true);
	updateProperties(view);
}

auto PinConnector::MatrixView::sizeHint() const -> QSize
{
	const auto tcc = static_cast<int>(m_matrix->trackChannelCount());
	if (tcc == 0) { return {0, 0}; }

	const int pcc = static_cast<int>(m_matrix->channelCount());
	if (pcc == 0) { return {0, 0}; }

	const int pcSize = pcc * cellSize() + s_lineThickness + BorderWidth * 2;
	const int tcSize = tcc * cellSize() + s_lineThickness + BorderWidth * 2;

	return {pcSize, tcSize};
}

auto PinConnector::MatrixView::minimumSizeHint() const -> QSize
{
	return sizeHint();
}

void PinConnector::MatrixView::paintEvent(QPaintEvent*)
{
	auto p = QPainter{this};
	p.setRenderHint(QPainter::Antialiasing);
	p.fillRect(rect(), p.background());

	const auto tcCount = m_matrix->trackChannelCount();
	const auto pcCount = m_matrix->channelCount();
	if (tcCount == 0 || pcCount == 0) { return; }

	// Draw border
	p.fillRect(rect(), m_lineColor);
	p.fillRect(rect().adjusted(BorderWidth, BorderWidth, -BorderWidth, -BorderWidth), p.background());

	// Draw vertical lines of grid
	for (int drawX = BorderWidth; drawX < width(); drawX += cellSize())
	{
		p.fillRect(drawX, BorderWidth, s_lineThickness, height() - BorderWidth, m_lineColor);
	}

	// Draw horizontal lines of grid
	for (int drawY = BorderWidth; drawY < height(); drawY += cellSize())
	{
		p.fillRect(BorderWidth, drawY, width() - BorderWidth, s_lineThickness, m_lineColor);
	}

	// Draw pin connector pins
	int drawX;
	int drawY = BorderWidth + s_lineThickness;
	for (track_ch_t tc = 0; tc < tcCount; ++tc)
	{
		drawX = BorderWidth + s_lineThickness;
		for (proc_ch_t pc = 0; pc < pcCount; ++pc)
		{
			p.fillRect(drawX + s_pinInnerMargin, drawY + s_pinInnerMargin,
				s_pinSize, s_pinSize, getColor(tc, pc));

			drawX += cellSize();
		}

		drawY += cellSize();
	}
}

void PinConnector::MatrixView::mouseMoveEvent(QMouseEvent* me)
{
	me->ignore();

	if (me->button() != Qt::MouseButton::NoButton)
	{
		unsetCursor();
		return;
	}

	if (!getPin(me->pos()))
	{
		unsetCursor();
		return;
	}

	setCursor(Qt::PointingHandCursor);
}

void PinConnector::MatrixView::mousePressEvent(QMouseEvent* me)
{
	auto pin = getPin(me->pos());
	if (!pin)
	{
		me->ignore();
		return;
	}

	const auto [tc, pc] = *pin;
	m_matrix->setPin(tc, pc, !m_matrix->enabled(tc, pc));

	me->accept();
}

void PinConnector::MatrixView::updateProperties(const PinConnector* view)
{
	// Update size
	const auto newSize = sizeHint();
	setFixedSize(newSize);

	if (newSize.width() == 0 || newSize.height() == 0)
	{
		hide();
	}
	else
	{
		show();
	}

	// Update tooltip
	const Model* parentModel = view->model() ? view->model()->parentModel() : nullptr;
	const auto formatText = m_matrix->isOutput() ? tr("%1 output(s) to LMMS") : tr("LMMS to %1 input(s)");
	const auto processorName = parentModel ? parentModel->fullDisplayName() : tr("processor");
	setToolTip(formatText.arg(processorName));
}

auto PinConnector::MatrixView::getPin(const QPoint& mousePos) -> std::optional<std::pair<track_ch_t, proc_ch_t>>
{
	if (!rect().adjusted(BorderWidth, BorderWidth, -BorderWidth, -BorderWidth)
		.contains(mousePos, true)) { return std::nullopt; }

	const auto mousePosAdj = mousePos - QPoint{BorderWidth + s_lineThickness, BorderWidth + s_lineThickness};

	int xIdx = mousePosAdj.x() / cellSize();
	int yIdx = mousePosAdj.y() / cellSize();

	// Check if within margin
	int relPos = mousePosAdj.x() - xIdx * cellSize();
	if (relPos > (cellSize() - s_lineThickness) || relPos < 0) { return std::nullopt; }

	relPos = mousePosAdj.y() - yIdx * cellSize();
	if (relPos > (cellSize() - s_lineThickness) || relPos < 0) { return std::nullopt; } // NOLINT

	assert(xIdx >= 0);
	assert(yIdx >= 0);
	return std::pair{static_cast<track_ch_t>(yIdx), static_cast<proc_ch_t>(xIdx)};
}

auto PinConnector::MatrixView::getColor(track_ch_t trackChannel, proc_ch_t processorChannel) -> QColor
{
	return m_matrix->enabled(trackChannel, processorChannel)
		? m_enabledColor
		: m_disabledColor;
}

} // namespace lmms::gui
