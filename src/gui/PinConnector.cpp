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
#include <QSpacerItem>

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

constexpr auto CenterMargin = QSize{48, 0};
constexpr auto WindowMarginTop = QSize{0, 96};
constexpr auto WindowMarginBottom = QSize{0, 32};
constexpr auto WindowMarginSide = QSize{48, 0};
constexpr auto WindowMarginTotal = WindowMarginTop + WindowMarginBottom + WindowMarginSide + WindowMarginSide;
constexpr auto DefaultMaxWindowSize = QSize{400, 256};

} // namespace


PinConnector::PinConnector(AudioPortsModel* model)
	: QWidget{}
	, ModelView{model, this}
	, m_inView{new MatrixView{this, model->in(), true}}
	, m_outView{new MatrixView{this, model->out(), false}}
{
	assert(model != nullptr);
	connect(model, &AudioPortsModel::propertiesChanged, this, &PinConnector::updateGeometry);

	const Model* parentModel = model->parentModel();
	assert(parentModel != nullptr);

	setWindowTitle(parentModel->fullDisplayName());
	m_subWindow = getGUI()->mainWindow()->addWindowedWidget(this);
	m_subWindow->setAttribute(Qt::WA_DeleteOnClose, false);
	setWindowIcon(embed::getIconPixmap("tool"));

	// No maximize button
	Qt::WindowFlags flags = m_subWindow->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	m_subWindow->setWindowFlags(flags);

	auto hLayout = new QHBoxLayout{};
	hLayout->addSpacing(WindowMarginSide.width());
	hLayout->addWidget(m_inView, 0, Qt::AlignmentFlag::AlignRight);

	auto getSpacerWidth = [&]() {
		const bool singleMatrix = model->trackChannelCount() == 0
			|| model->in().channelCount() == 0
			|| model->out().channelCount() == 0;

		return singleMatrix	? 0 : CenterMargin.width();
	};

	auto spacer = new QSpacerItem{getSpacerWidth(), 1, QSizePolicy::Fixed};
	connect(model, &AudioPortsModel::propertiesChanged, this, [&]() {
		spacer->changeSize(getSpacerWidth(), 1, QSizePolicy::Fixed);
	});
	hLayout->addSpacerItem(spacer);

	hLayout->addWidget(m_outView, 0, Qt::AlignmentFlag::AlignLeft);
	hLayout->addSpacing(WindowMarginSide.width());


	auto vLayout = new QVBoxLayout{this};
	vLayout->addSpacing(WindowMarginTop.height());
	vLayout->addLayout(hLayout);
	vLayout->addSpacing(WindowMarginBottom.height());

	{
		auto sp = m_subWindow->sizePolicy();
		sp.setRetainSizeWhenHidden(true);
		m_subWindow->setSizePolicy(sp);
	}

	updateGeometry();
	show();
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

	return WindowMarginTotal + centerMargin + inSize + outSize;
}

auto PinConnector::minimumSizeHint() const -> QSize
{
	const auto minSize = sizeHint();
	return QSize {
		std::min(minSize.width(), DefaultMaxWindowSize.width()),
		std::min(minSize.height(), DefaultMaxWindowSize.height())
	};
}

void PinConnector::toggleVisibility()
{
	if (m_subWindow->isVisible())
	{
		m_subWindow->hide();
		//m_scrollArea->hide();
		hide();
	}
	else
	{
		updateGeometry();
		show();
		//m_scrollArea->show();
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

	const auto* model = castModel<AudioPortsModel>();
	if (!model) { return; }

	constexpr int cellSize = MatrixView::cellSize();

	auto f = adjustedToPixelSize(font(), static_cast<int>(cellSize * 0.9));
	f.setBold(false);
	p.setFont(f);
	p.setPen(palette().text().color());

	// Get matrix postions/sizes
	auto inMatrixRect = m_inView->rect();
	inMatrixRect.moveTo(m_inView->pos());
	auto outMatrixRect = m_outView->rect();
	outMatrixRect.moveTo(m_outView->pos());

	// Draw track channel text (in)
	if (model->in().channelCount() != 0)
	{
		const auto width = inMatrixRect.left() - 4;
		int yPos = inMatrixRect.y() + MatrixView::BorderWidth;
		for (unsigned idx = 0; idx < model->trackChannelCount(); ++idx)
		{
			p.drawText(0, yPos, width, cellSize, Qt::AlignRight,
				QString::fromUtf16(u"%1 \U0001F82E").arg(idx + 1));

			yPos += cellSize;
		}
	}

	// Draw track channel text (out)
	if (model->out().channelCount() != 0)
	{
		const int xPos = outMatrixRect.right() + 4;
		int yPos = outMatrixRect.y() + MatrixView::BorderWidth;
		const int width = this->width() - outMatrixRect.right() - 4;
		for (unsigned idx = 0; idx < model->trackChannelCount(); ++idx)
		{
			p.drawText(xPos, yPos, width, cellSize, Qt::AlignLeft,
				QString::fromUtf16(u"\U0001F82E %1").arg(idx + 1));

			yPos += cellSize;
		}
	}

	p.save();

	// TODO: Draw instruments with sidechain inputs differently?

	// Draw processor channel text (in)
	int yPos = inMatrixRect.top() - 4;
	int xPos = inMatrixRect.x() + MatrixView::BorderWidth;
	for (int idx = 0; idx < model->in().channelCount(); ++idx)
	{
		const auto transform = QTransform{}.translate(xPos, yPos).rotate(-90);
		p.setTransform(transform);

		const auto name = model->in().channelName(idx);
		p.drawText(0, 0, yPos, cellSize, Qt::AlignLeft,
			QString::fromUtf16(u"\U0001F82E %1").arg(name));

		xPos += cellSize;
	}

	// Draw processor channel text (out)
	yPos = outMatrixRect.top() - 4;
	xPos = outMatrixRect.x() + MatrixView::BorderWidth;
	for (int idx = 0; idx < model->out().channelCount(); ++idx)
	{
		const auto transform = QTransform{}.translate(xPos, yPos).rotate(-90);
		p.setTransform(transform);

		const auto name = model->out().channelName(idx);
		p.drawText(0, 0, yPos, cellSize, Qt::AlignLeft,
			QString::fromUtf16(u"\U0001F82C %1").arg(name));

		xPos += cellSize;
	}

	p.restore();
}

void PinConnector::updateGeometry()
{
	m_inView->updateSize();
	m_outView->updateSize();

	m_subWindow->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	m_subWindow->setMinimumSize(minimumSizeHint());

	update();
}

//////////////////////////////////////////////

PinConnector::MatrixView::MatrixView(PinConnector* view,
	const AudioPortsModel::Matrix& matrix, bool isIn)
	: QWidget{nullptr}
	, m_model{view->castModel<AudioPortsModel>()}
	, m_matrix{&matrix}
{
	assert(m_model != nullptr);
	connect(m_model, &AudioPortsModel::dataChanged, this, static_cast<void(QWidget::*)()>(&QWidget::update));

	const Model* parentModel = m_model->parentModel();
	assert(parentModel != nullptr);

	const char* formatText = isIn ? "LMMS to %1 input(s)" : "%1 output(s) to LMMS";
	setToolTip(QString{formatText}.arg(parentModel->fullDisplayName()));

	setMouseTracking(true);

	updateSize();
}

auto PinConnector::MatrixView::sizeHint() const -> QSize
{
	return calculateSize();
}

auto PinConnector::MatrixView::minimumSizeHint() const -> QSize
{
	return calculateSize();
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

	int xIdx = 0;
	int yIdx = 0;
	if (!getCell(me->pos(), xIdx, yIdx))
	{
		unsetCursor();
		return;
	}

	setCursor(Qt::PointingHandCursor);
}

void PinConnector::MatrixView::mousePressEvent(QMouseEvent* me)
{
	int xIdx = 0;
	int yIdx = 0;
	if (!getCell(me->pos(), xIdx, yIdx))
	{
		me->ignore();
		return;
	}

	BoolModel* model = m_matrix->pins().at(yIdx).at(xIdx);

	if (me->modifiers() & Qt::ControlModifier)
	{
		// Taken from AutomatableModelView::mousePressEvent
		new gui::StringPairDrag{"automatable_model", QString::number(model->id()), QPixmap{}, this};
	}
	else
	{
		model->setValue(!model->value());
	}

	me->accept();
}

void PinConnector::MatrixView::updateSize()
{
	const auto newSize = calculateSize();

	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	setFixedSize(newSize);

	if (newSize.width() == 0 || newSize.height() == 0)
	{
		hide();
	}
	else
	{
		show();
	}
}

auto PinConnector::MatrixView::getCell(const QPoint& mousePos, int& xIdx, int& yIdx) -> bool
{
	if (!rect().adjusted(BorderWidth, BorderWidth, -BorderWidth, -BorderWidth)
		.contains(mousePos, true)) { return false; }

	const auto mousePosAdj = mousePos - QPoint{BorderWidth + s_lineThickness, BorderWidth + s_lineThickness};

	xIdx = mousePosAdj.x() / cellSize();
	yIdx = mousePosAdj.y() / cellSize();

	// Check if within margin
	int relPos = mousePosAdj.x() - xIdx * cellSize();
	if (relPos > (cellSize() - s_lineThickness) || relPos < 0) { return false; }

	relPos = mousePosAdj.y() - yIdx * cellSize();
	if (relPos > (cellSize() - s_lineThickness) || relPos < 0) { return false; } // NOLINT

	return true;
}

auto PinConnector::MatrixView::getColor(track_ch_t trackChannel, proc_ch_t processorChannel) -> QColor
{
	return m_matrix->enabled(trackChannel, processorChannel)
		? m_enabledColor
		: m_disabledColor;
}

auto PinConnector::MatrixView::calculateSize() const -> QSize
{
	const auto tcc = static_cast<int>(m_matrix->trackChannelCount());
	if (tcc == 0) { return {0, 0}; }

	const int pcc = static_cast<int>(m_matrix->channelCount());
	if (pcc == 0) { return {0, 0}; }

	const int pcSize = pcc * cellSize() + s_lineThickness + BorderWidth * 2;
	const int tcSize = tcc * cellSize() + s_lineThickness + BorderWidth * 2;

	return {pcSize, tcSize};
}

} // namespace lmms::gui
