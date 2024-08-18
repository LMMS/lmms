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

#include <QBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>
#include <QSpacerItem>

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

constexpr auto CenterMargin = QSize{48, 0};
constexpr auto WindowMarginTop = QSize{0, 96};
constexpr auto WindowMarginBottom = QSize{0, 32};
constexpr auto WindowMarginSide = QSize{48, 0};
constexpr auto WindowMarginTotal = WindowMarginTop + WindowMarginBottom + WindowMarginSide + WindowMarginSide;
constexpr auto DefaultMaxWindowSize = QSize{400, 256};

} // namespace


class PluginPinConnectorView::MatrixView : public QWidget
{
public:
	MatrixView(PluginPinConnectorView* view, const PluginPinConnector::Matrix& matrix, bool isIn);
	~MatrixView() override = default;
	auto sizeHint() const -> QSize override;
	auto minimumSizeHint() const -> QSize override;
	void paintEvent(QPaintEvent*) override;
	void mousePressEvent(QMouseEvent* me) override;
	void updateSize();
	auto cellSize() const -> QSize;

private:
	auto calculateSize() const -> QSize;
	auto getIcon(const BoolModel& model, int trackChannel, int pluginChannel) -> const QPixmap&;

	PluginPinConnector* m_model;
	const PluginPinConnector::Matrix* m_matrix;

	QPixmap m_buttonOffBlack = embed::getIconPixmap("step_btn_off");
	QPixmap m_buttonOffGray = embed::getIconPixmap("step_btn_off_light");
	//QPixmap m_buttonOn0 = embed::getIconPixmap("step_btn_on_0");
	QPixmap m_buttonOn = embed::getIconPixmap("step_btn_on_200");

	static constexpr int s_gridMargin = 2;
};


PluginPinConnectorView::PluginPinConnectorView(PluginPinConnector* model, QWidget* parent)
	: QWidget{parent}
	, ModelView{model, this}
	, m_inView{new MatrixView{this, model->in(), true}}
	, m_outView{new MatrixView{this, model->out(), false}}
{
	connect(model, &PluginPinConnector::propertiesChanged, this, &PluginPinConnectorView::updateGeometry);

	setWindowTitle(tr("Plugin Pin Connector"));
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
		const bool singleMatrix = model->trackChannelsUsed() == 0
			|| model->in().channelCount == 0
			|| model->out().channelCount == 0;
		
		return singleMatrix	? 0 : CenterMargin.width();
	};

	auto spacer = new QSpacerItem{getSpacerWidth(), 1, QSizePolicy::Fixed};
	connect(model, &PluginPinConnector::propertiesChanged, this, [&]() {
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

auto PluginPinConnectorView::sizeHint() const -> QSize
{
	const auto inSize = m_inView->size();
	const auto outSize = m_outView->size();

	const auto centerMargin = (inSize.isEmpty() || outSize.isEmpty()) ? QSize{0, 0} : CenterMargin;

	return WindowMarginTotal + centerMargin + inSize + outSize;
}

auto PluginPinConnectorView::minimumSizeHint() const -> QSize
{
	const auto minSize = sizeHint();
	return QSize {
		std::min(minSize.width(), DefaultMaxWindowSize.width()),
		std::min(minSize.height(), DefaultMaxWindowSize.height())
	};
}

void PluginPinConnectorView::toggleVisibility()
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

void PluginPinConnectorView::closeWindow()
{
	m_subWindow->setAttribute(Qt::WA_DeleteOnClose);
	m_subWindow->close();
}

void PluginPinConnectorView::paintEvent(QPaintEvent*)
{
	auto p = QPainter{this};
	p.setRenderHint(QPainter::Antialiasing);
	p.fillRect(rect(), p.background());

	const auto* model = castModel<PluginPinConnector>();
	if (!model) { return; }

	const auto cellSize = m_inView->cellSize();
	const auto textSize = QSize {
		static_cast<int>(cellSize.width() * 0.9),
		static_cast<int>(cellSize.height() * 0.9)
	};

	QFont f = adjustedToPixelSize(font(), 16);
	f.setBold(false);
	p.setFont(f);
	p.setPen(palette().text().color());

	// Get matrix postions/sizes
	auto inMatrixRect = m_inView->rect();
	inMatrixRect.moveTo(m_inView->pos());
	auto outMatrixRect = m_outView->rect();
	outMatrixRect.moveTo(m_outView->pos());

	// Draw track channel text (in)
	if (model->in().channelCount != 0)
	{
		const auto xwIn = std::pair{0, inMatrixRect.left() - 4};
		int yPos = inMatrixRect.y();
		for (unsigned idx = 0; idx < model->trackChannelsUsed(); ++idx)
		{
			p.drawText(xwIn.first, yPos, xwIn.second, textSize.height(), Qt::AlignRight,
				QString::fromUtf16(u"%1 \U0001F82E").arg(idx + 1));

			yPos += cellSize.height();
		}
	}

	// Draw track channel text (out)
	if (model->out().channelCount != 0)
	{
		const auto xwOut = std::pair{outMatrixRect.right() + 4, width() - outMatrixRect.right() - 4};
		int yPos = outMatrixRect.y();
		for (unsigned idx = 0; idx < model->trackChannelsUsed(); ++idx)
		{
			p.drawText(xwOut.first, yPos, xwOut.second, textSize.height(), Qt::AlignLeft,
				QString::fromUtf16(u"\U0001F82E %1").arg(idx + 1));

			yPos += cellSize.height();
		}
	}

	p.save();

	// Draw plugin channel text (in)
	int yPos = inMatrixRect.top() - 4;
	int xPos = inMatrixRect.x() + 2;
	for (int idx = 0; idx < model->in().channelCount; ++idx)
	{
		const auto transform = QTransform{}.translate(xPos, yPos).rotate(-90);
		p.setTransform(transform);

		const auto name = model->in().channelName(idx);
		p.drawText(0, 0, textSize.width(), yPos, Qt::AlignLeft, name);

		xPos += cellSize.width();
	}

	// Draw plugin channel text (out)
	yPos = outMatrixRect.top() - 4;
	xPos = outMatrixRect.x() + 2;
	for (int idx = 0; idx < model->out().channelCount; ++idx)
	{
		const auto transform = QTransform{}.translate(xPos, yPos).rotate(-90);
		p.setTransform(transform);

		const auto name = model->out().channelName(idx);
		p.drawText(0, 0, textSize.width(), yPos, Qt::AlignLeft, name);

		xPos += cellSize.width();
	}

	p.restore();
}

void PluginPinConnectorView::updateGeometry()
{
	m_inView->updateSize();
	m_outView->updateSize();

	m_subWindow->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	m_subWindow->setMinimumSize(minimumSizeHint());

	update();
}

//////////////////////////////////////////////

PluginPinConnectorView::MatrixView::MatrixView(PluginPinConnectorView* view,
	const PluginPinConnector::Matrix& matrix, bool isIn)
	: QWidget{nullptr}
	, m_model{view->castModel<PluginPinConnector>()}
	, m_matrix{&matrix}
{
	assert(m_model != nullptr);
	connect(m_model, &PluginPinConnector::dataChanged, this, static_cast<void(QWidget::*)()>(&QWidget::update));

	const Model* parentModel = m_model->parentModel();
	assert(parentModel != nullptr);

	const char* formatText = isIn ? "LMMS to %1 input(s)" : "%1 output(s) to LMMS";
	setToolTip(QString{formatText}.arg(parentModel->fullDisplayName()));

	updateSize();
}

auto PluginPinConnectorView::MatrixView::sizeHint() const -> QSize
{
	return calculateSize();
}

auto PluginPinConnectorView::MatrixView::minimumSizeHint() const -> QSize
{
	return calculateSize();
}

void PluginPinConnectorView::MatrixView::paintEvent(QPaintEvent*)
{
	auto p = QPainter{this};
	p.setRenderHint(QPainter::Antialiasing);
	p.fillRect(rect(), p.background());

	const auto& pins = m_matrix->pins;
	if (pins.empty() || pins[0].empty()) { return; }
	if (m_model->trackChannelsUsed() == 0) { return; }

	assert(m_model->trackChannelsUsed() <= pins.size());
	const auto cellSize = this->cellSize();

	auto drawXY = QPoint{0, 0};
	for (std::size_t tcIdx = 0; tcIdx < pins.size(); ++tcIdx)
	{
		drawXY.rx() = 0;
		auto& pluginChannels = pins[tcIdx];
		for (std::size_t pcIdx = 0; pcIdx < pluginChannels.size(); ++pcIdx)
		{
			BoolModel* pin = pluginChannels[pcIdx];
			p.drawPixmap(drawXY, getIcon(*pin, tcIdx, pcIdx));

			drawXY.rx() += cellSize.width();
		}

		drawXY.ry() += cellSize.height();
	}
}

void PluginPinConnectorView::MatrixView::mousePressEvent(QMouseEvent* me)
{
	if (!rect().contains(me->pos(), true))
	{
		me->ignore();
		return;
	}

	const int buttonW = m_buttonOn.width();
	const int buttonH = m_buttonOn.height();
	const auto cellSize = this->cellSize();

	const auto relMousePos = me->pos();
	const int xIdx = relMousePos.x() / cellSize.width();
	const int yIdx = relMousePos.y() / cellSize.height();

	// Check if within margin
	int relPos = relMousePos.x() - xIdx * cellSize.width();
	if (relPos >= buttonW || relPos <= 0)
	{
		me->ignore();
		return;
	}

	relPos = relMousePos.y() - yIdx * cellSize.height();
	if (relPos >= buttonH || relPos <= 0)
	{
		me->ignore();
		return;
	}

	BoolModel* model = m_matrix->pins.at(yIdx).at(xIdx);

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
}

void PluginPinConnectorView::MatrixView::updateSize()
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

auto PluginPinConnectorView::MatrixView::cellSize() const -> QSize
{
	return {m_buttonOn.width() + s_gridMargin, m_buttonOn.height() + s_gridMargin};
}

auto PluginPinConnectorView::MatrixView::calculateSize() const -> QSize
{
	const auto tcc = static_cast<int>(m_model->trackChannelsUsed());
	if (tcc == 0) { return QSize{0, 0}; }

	const int pcc = m_matrix->channelCount;
	if (pcc == 0) { return QSize{0, 0}; }

	const int pcSize = pcc * (m_buttonOn.width() + s_gridMargin) - s_gridMargin;
	const int tcSize = tcc * (m_buttonOn.height() + s_gridMargin) - s_gridMargin;

	return {pcSize, tcSize};
}

auto PluginPinConnectorView::MatrixView::getIcon(const BoolModel& model, int trackChannel, int pluginChannel)
	-> const QPixmap&
{
	// TODO: Alternate b/w black and gray icons?
	(void)trackChannel;
	(void)pluginChannel;
	const auto* offIcon = &m_buttonOffBlack;
	return model.value() ? m_buttonOn : *offIcon;
}

} // namespace lmms::gui
