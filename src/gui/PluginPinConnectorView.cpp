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

constexpr auto GridMargin = 2; // TODO: Remove?
constexpr auto CenterMargin = QSize{48, 0};
constexpr auto WindowMarginTop = QSize{0, 144};
constexpr auto WindowMarginBottom = QSize{0, 48};
constexpr auto WindowMarginSide = QSize{64, 0};
constexpr auto WindowMarginTotal = WindowMarginTop + WindowMarginBottom + WindowMarginSide + WindowMarginSide;
constexpr auto DefaultWindowSize = QSize{400, 256};

} // namespace


class PluginPinConnectorView::MatrixView : public QWidget
{
public:
	MatrixView(PluginPinConnectorView* parent, const PluginPinConnector::Matrix& matrix);
	~MatrixView() override = default;
	auto sizeHint() const -> QSize override;
	void paintEvent(QPaintEvent*) override;
	void mousePressEvent(QMouseEvent* me) override;
	void updateSize();
	auto cellSize() const -> QSize;

private:
	auto calculateSize() const -> QSize;
	auto getIcon(const BoolModel& model, int trackChannel, int pluginChannel) -> const QPixmap&;

	//PluginPinConnectorView* m_view;
	PluginPinConnector* m_model;
	const PluginPinConnector::Matrix* m_matrix;
	//QString m_name;

	QPixmap m_buttonOffBlack = embed::getIconPixmap("step_btn_off");
	QPixmap m_buttonOffGray = embed::getIconPixmap("step_btn_off_light");
	//QPixmap m_buttonOn0 = embed::getIconPixmap("step_btn_on_0");
	QPixmap m_buttonOn = embed::getIconPixmap("step_btn_on_200");

	static constexpr int s_gridMargin = 2;
};


PluginPinConnectorView::PluginPinConnectorView(PluginPinConnector* model, QWidget* parent)
	: QWidget{parent} // new QScrollArea{parent}
	//: QScrollArea{parent}
	, ModelView{model, this}
	, m_inView{new MatrixView{this, model->in()}}
	, m_outView{new MatrixView{this, model->out()}}
{


	//connect(model, &PluginPinConnector::dataChanged, this, static_cast<void(QWidget::*)()>(&QWidget::update));

	// Update geometry then regular update
	//connect(model, &PluginPinConnector::propertiesChanged, this, &PluginPinConnectorView::updateGeometry);

	

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

	//setMinimumSize(DefaultWindowSize);
	//setSizePolicy(QSizePolicy::MinimumExpanding);
	//setMinimumSize(m_minSize);


	//m_subWindow->setMinimumSize(initialSize);

	//m_subWindow->show();

	updateGeometry();


	// TEMPORARY!
	m_inView->move(m_inRect.topLeft());
	m_outView->move(m_outRect.topLeft());


	show();
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

void PluginPinConnectorView::update()
{
/*
	updateGeometry();


	setMinimumSize({
		std::min(m_minSize.width(), DefaultWindowSize.width()),
		std::min(m_minSize.height(), DefaultWindowSize.height())
	});

	setMaximumSize(m_minSize);
*/
	QWidget::update();
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

	const auto cellSize = m_inView->cellSize();
	const auto textSize = QSize {
		static_cast<int>(cellSize.width() * 0.9),
		static_cast<int>(cellSize.height() * 0.9)
	};

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
			p.drawText(xwIn.first, yPos, xwIn.second, textSize.height(), Qt::AlignRight,
				QString::fromUtf16(u"%1 \U0001F82E").arg(idx + 1));
		}

		if (pinConnector->out().channelCount() != 0)
		{
			p.drawText(xwOut.first, yPos, xwOut.second, textSize.height(), Qt::AlignLeft,
				QString::fromUtf16(u"\U0001F82E %1").arg(idx + 1));
		}

		yPos += cellSize.height();
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
		p.drawText(0, 0, textSize.width(), yPos, Qt::AlignLeft, name);

		xPos += cellSize.width();
	}

	// Draw plugin channel text (out)
	xPos = m_outRect.x() + GridMargin;
	for (int idx = 0; idx < pinConnector->out().channelCount(); ++idx)
	{
		const auto transform = QTransform{}.translate(xPos, yPos).rotate(-90);
		p.setTransform(transform);

		const auto name = pinConnector->out().channelName(idx);
		p.drawText(0, 0, textSize.width(), yPos, Qt::AlignLeft, name);

		xPos += cellSize.width();
	}

	p.restore();
}

void PluginPinConnectorView::updateGeometry()
{
	const auto inSize = m_inView->size();
	const auto outSize = m_outView->size();

	const auto centerMargin = (inSize.isEmpty() || outSize.isEmpty()) ? QSize{} : CenterMargin;

	m_minSize = WindowMarginTotal + centerMargin + inSize + outSize;

	

	//setMinimumSize(m_minSize);
	//m_subWindow->setMinimumSize(m_minSize);

	//std::cout << "m_minSize: {" << m_minSize.width() << ", " << m_minSize.height() << "}\n";


	setMinimumSize({
		std::min(m_minSize.width(), DefaultWindowSize.width()),
		std::min(m_minSize.height(), DefaultWindowSize.height())
	});

	setMaximumSize(m_minSize);



	const auto extra = QSize {
		std::max(width() - m_minSize.width(), 0) / 2,
		std::max(height() - m_minSize.height(), 0) / 2
	};

	//m_minSize.rwidth() = std::max(DefaultWindowSize.width(), m_minSize.width());
	//m_minSize.rheight() = std::max(DefaultWindowSize.height(), m_minSize.height());

	const auto inMatrixPos = WindowMarginTop + WindowMarginSide + extra;
	const auto outMatrixPos = inMatrixPos + centerMargin + QSize{inSize.width(), 0};

	m_inRect = QRect{inMatrixPos.width(), inMatrixPos.height(), inSize.width(), inSize.height()};
	m_outRect = QRect{outMatrixPos.width(), outMatrixPos.height(), outSize.width(), outSize.height()};

	//std::cout << "m_inRect: {" << m_inRect.x() << ", " << m_inRect.y() << "}, {" << m_inRect.width() << ", " << m_inRect.height() << "}\n";
	//std::cout << "m_outRect: {" << m_outRect.x() << ", " << m_outRect.y() << "}, {" << m_outRect.width() << ", " << m_outRect.height() << "}\n";


	

	update();
}

void updatePositions()
{

}

//////////////////////////////////////////////

PluginPinConnectorView::MatrixView::MatrixView(PluginPinConnectorView* parent,
	const PluginPinConnector::Matrix& matrix)
	: QWidget{parent}
	, m_model{parent->castModel<PluginPinConnector>()}
	, m_matrix{&matrix}
{
	assert(m_model != nullptr);

	connect(m_model, &PluginPinConnector::dataChanged, this, static_cast<void(QWidget::*)()>(&QWidget::update));
	connect(m_model, &PluginPinConnector::propertiesChanged, this, &MatrixView::updateSize);

	updateSize();
}

auto PluginPinConnectorView::MatrixView::sizeHint() const -> QSize
{
	return calculateSize();
}

void PluginPinConnectorView::MatrixView::paintEvent(QPaintEvent*)
{
	auto p = QPainter{this};
	p.setRenderHint(QPainter::Antialiasing);
	p.fillRect(rect(), p.background());

	const auto& pins = m_matrix->pinMap();
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

#ifndef NDEBUG
	const auto expected = QPoint{width(), height()};
	const auto actual = drawXY - QPoint{s_gridMargin, s_gridMargin};
	//std::cout << "expected: {" << expected.x() << ", " << expected.y() << "}\n";
	//std::cout << "actual: {" << actual.x() << ", " << actual.y() << "}\n";
	assert(actual == expected);
#endif
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

	const auto relMousePos = me->pos();
	const int xIdx = relMousePos.x() / (buttonW + s_gridMargin);
	const int yIdx = relMousePos.y() / (buttonH + s_gridMargin);

	// Check if within margin
	int relPos = relMousePos.x() - xIdx * buttonW;
	if (relPos >= buttonW || relPos <= 0)
	{
		me->ignore();
		return;
	}

	relPos = relMousePos.y() - yIdx * buttonH;
	if (relPos >= buttonH || relPos <= 0)
	{
		me->ignore();
		return;
	}

	BoolModel* model = m_matrix->pinMap().at(yIdx).at(xIdx);

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
	setFixedSize(calculateSize());
}

auto PluginPinConnectorView::MatrixView::cellSize() const -> QSize
{
	return {m_buttonOn.width() + s_gridMargin, m_buttonOn.height() + s_gridMargin};
}

auto PluginPinConnectorView::MatrixView::calculateSize() const -> QSize
{
	const auto tcc = static_cast<int>(m_model->trackChannelsUsed());
	if (tcc == 0) { return QSize{}; }

	const int pcc = m_matrix->channelCount();
	if (pcc == 0) { return QSize{}; }

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
