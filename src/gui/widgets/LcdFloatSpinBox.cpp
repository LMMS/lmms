/*
 * LcdFloatSpinBox.cpp - class LcdFloatSpinBox (LcdSpinBox for floats)
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2008 Paul Giblock <pgllama/at/gmail.com>
 * Copyright (c) 2020 Martin Pavelek <he29.HS/at/gmail.com>
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

#include "LcdFloatSpinBox.h"

#include <cmath>

#include <QApplication>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QStyleOptionFrameV2>
#include <QVBoxLayout>

#include "CaptionMenu.h"
#include "embed.h"
#include "GuiApplication.h"
#include "gui_templates.h"
#include "MainWindow.h"


LcdFloatSpinBox::LcdFloatSpinBox(int numWhole, int numFrac, const QString& name, QWidget* parent) :
	FloatModelView(new FloatModel(0, 0, 0, 0, nullptr, name, true), this),
	m_wholeDisplay(numWhole, parent, name, false),
	m_fractionDisplay(numFrac, parent, name, true),
	m_mouseMoving(false),
	m_intStep(false),
	m_origMousePos(),
	m_displayOffset(0)
{
	layoutSetup();
}


LcdFloatSpinBox::LcdFloatSpinBox(int numWhole, int numFrac, const QString& style, const QString& name, QWidget* parent) :
	FloatModelView(new FloatModel(0, 0, 0, 0, nullptr, name, true), this),
	m_wholeDisplay(numWhole, style, parent, name, false),
	m_fractionDisplay(numFrac, style, parent, name, true),
	m_mouseMoving(false),
	m_intStep(false),
	m_origMousePos(),
	m_displayOffset(0)
{
	layoutSetup(style);
}


void LcdFloatSpinBox::layoutSetup(const QString &style)
{
	// Assemble the LCD parts
	QHBoxLayout *lcdLayout = new QHBoxLayout();

	m_wholeDisplay.setSeamless(false, true);
	m_fractionDisplay.setSeamless(true, false);

	lcdLayout->addWidget(&m_wholeDisplay);

	QLabel *dotLabel = new QLabel("", this);
	QPixmap dotPixmap(embed::getIconPixmap(QString("lcd_" + style + "_dot").toUtf8().constData()));
	dotLabel->setPixmap(dotPixmap.copy(0, 0, dotPixmap.size().width(), dotPixmap.size().height() / 2));
	lcdLayout->addWidget(dotLabel);

	lcdLayout->addWidget(&m_fractionDisplay);

	lcdLayout->setContentsMargins(0, 0, 0, 0);
	lcdLayout->setSpacing(0);

	// Add space for label
	QVBoxLayout *outerLayout = new QVBoxLayout();
	outerLayout->addLayout(lcdLayout);
	outerLayout->addSpacing(9);
	outerLayout->setContentsMargins(0, 0, 0, 0);
	outerLayout->setSizeConstraint(QLayout::SetFixedSize);
	this->setLayout(outerLayout);
}


void LcdFloatSpinBox::update()
{
	const int whole = static_cast<int>(model()->value());
	const float fraction = model()->value() - whole;
	const int intFraction = fraction * std::pow(10.f, m_fractionDisplay.numDigits());
	m_wholeDisplay.setValue(whole);
	m_fractionDisplay.setValue(intFraction);

	QWidget::update();
}


void LcdFloatSpinBox::contextMenuEvent(QContextMenuEvent* event)
{
	CaptionMenu contextMenu(model()->displayName());
	addDefaultActions(&contextMenu);
	contextMenu.exec(QCursor::pos());
}


void LcdFloatSpinBox::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton &&
		!(event->modifiers() & Qt::ControlModifier) &&
		event->y() < m_wholeDisplay.cellHeight() + 2)
	{
		m_mouseMoving = true;
		m_origMousePos = event->globalPos();

		AutomatableModel *thisModel = model();
		if (thisModel)
		{
			thisModel->addJournalCheckPoint();
			thisModel->saveJournallingState(false);
		}
	}
	else
	{
		FloatModelView::mousePressEvent(event);
	}
}


void LcdFloatSpinBox::mouseMoveEvent(QMouseEvent* event)
{
	// switch between integer and fractional step based on cursor position
	if (event->x() < m_wholeDisplay.width()) { m_intStep = true; }
	else { m_intStep = false; }

	if (m_mouseMoving)
	{
		int dy = event->globalY() - m_origMousePos.y();
		if (gui->mainWindow()->isShiftPressed()) { dy = qBound(-4, dy/4, 4); }
		if (dy > 1 || dy < -1)
		{
			model()->setValue(model()->value() - dy / 2 * getStep());
			emit manualChange();
			m_origMousePos = event->globalPos();
		}
	}
}


void LcdFloatSpinBox::mouseReleaseEvent(QMouseEvent*)
{
	if (m_mouseMoving)
	{
		model()->restoreJournallingState();
		m_mouseMoving = false;
	}
}


void LcdFloatSpinBox::wheelEvent(QWheelEvent *event)
{
	// switch between integer and fractional step based on cursor position
	if (event->x() < m_wholeDisplay.width()) { m_intStep = true; }
	else { m_intStep = false; }

	event->accept();
	model()->setValue(model()->value() + ((event->angleDelta().y() > 0) ? 1 : -1) * getStep());
	emit manualChange();
}


void LcdFloatSpinBox::mouseDoubleClickEvent(QMouseEvent *)
{
	enterValue();
}


void LcdFloatSpinBox::enterValue()
{
	bool ok;
	float newVal;

	newVal = QInputDialog::getDouble(
			this, tr("Set value"),
			tr("Please enter a new value between %1 and %2:").
				arg(model()->minValue()).
				arg(model()->maxValue()),
			model()->value(),
			model()->minValue(),
			model()->maxValue(),
			m_fractionDisplay.numDigits(), &ok);

	if (ok)
	{
		model()->setValue(newVal);
	}
}


float LcdFloatSpinBox::getStep() const
{
	if (m_intStep) { return 1; }
	else { return model()->step<float>(); }
}


void LcdFloatSpinBox::paintEvent(QPaintEvent*)
{
	QPainter p(this);

	// Border
	QStyleOptionFrame opt;
	opt.initFrom(this);
	opt.state = QStyle::State_Sunken;
	opt.rect = QRect(0, 0, width() - 1, m_wholeDisplay.height());
	style()->drawPrimitive(QStyle::PE_Frame, &opt, &p, this);

	// Label
	if (!m_label.isEmpty())
	{
		p.setFont(pointSizeF(p.font(), 6.5));
		p.setPen(m_wholeDisplay.textShadowColor());
		p.drawText(width() / 2 - p.fontMetrics().width(m_label) / 2 + 1, height(), m_label);
		p.setPen(m_wholeDisplay.textColor());
		p.drawText(width() / 2 - p.fontMetrics().width(m_label) / 2, height() - 1, m_label);
	}
}
