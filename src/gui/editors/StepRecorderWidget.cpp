/*
 * StepRecoderWidget.cpp - widget that provide gui markers for step recording
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

#include <QPainter>

#include "StepRecorderWidget.h"
#include "TextFloat.h"
#include "embed.h"

namespace lmms::gui
{

StepRecorderWidget::StepRecorderWidget(
		QWidget * parent,
		const int ppb,
		const int marginTop,
		const int marginBottom,
		const int marginLeft,
		const int marginRight) :
	QWidget(parent),
	m_marginTop(marginTop),
	m_marginBottom(marginBottom),
	m_marginLeft(marginLeft),
	m_marginRight(marginRight)
{
	const QColor baseColor =  QColor(255, 0, 0);// QColor(204, 163, 0); // Orange
	m_colorLineEnd   = baseColor.lighter(150);
	m_colorLineStart = baseColor.darker(120);

	setAttribute(Qt::WA_NoSystemBackground, true);
	setPixelsPerBar(ppb);

	m_top = m_marginTop;
	m_left = m_marginLeft;
}

void StepRecorderWidget::setPixelsPerBar(int ppb)
{
	m_ppb = ppb;
}

void StepRecorderWidget::setCurrentPosition(TimePos currentPosition)
{
	m_currentPosition = currentPosition;
}

void StepRecorderWidget::setMargins(const QMargins &qm)
{
	m_left = qm.left();
	m_right = qm.right();
	m_top = qm.top();
	m_bottom = qm.bottom();
}

QMargins StepRecorderWidget::margins()
{
	return QMargins(m_left, m_top, m_right, m_bottom);
}

void StepRecorderWidget::setBottomMargin(const int marginBottom)
{
	m_marginBottom = marginBottom;
}

void StepRecorderWidget::setStartPosition(TimePos pos)
{
	m_curStepStartPos = pos;
}

void StepRecorderWidget::setEndPosition(TimePos pos)
{
	m_curStepEndPos = pos;
	emit positionChanged(m_curStepEndPos);
}

void StepRecorderWidget::showHint()
{
	TextFloat::displayMessage(tr( "Hint" ), tr("Move recording cursor using <Left/Right> arrows"),
		embed::getIconPixmap("hint"));
}

void StepRecorderWidget::setStepsLength(TimePos stepsLength)
{
	m_stepsLength = stepsLength;
}

void StepRecorderWidget::paintEvent(QPaintEvent * pe)
{
	QPainter painter(this);

	updateBoundaries();

	move(0, 0);

	//draw steps ruler
	painter.setPen(m_colorLineEnd);

	TimePos curPos = m_curStepEndPos;
	int x = xCoordOfTick(curPos);
	while(x <= m_right)
	{
		const int w = 2;
		const int h = 4;
		painter.drawRect(x - 1, m_top, w, h);
		curPos += m_stepsLength;
		x = xCoordOfTick(curPos);
	}

	//draw current step start/end position lines
	if(m_curStepStartPos != m_curStepEndPos)
	{
		drawVerLine(&painter, m_curStepStartPos, m_colorLineStart, m_top, m_bottom);
	}

	drawVerLine(&painter, m_curStepEndPos, m_colorLineEnd, m_top, m_bottom);

	//if the line is adjacent to the keyboard at the left - it cannot be seen.
	//add another line to make it clearer
	if(m_curStepEndPos == 0)
	{
		drawVerLine(&painter, xCoordOfTick(m_curStepEndPos) + 1, m_colorLineEnd, m_top, m_bottom);
	}
}

int StepRecorderWidget::xCoordOfTick(int tick)
{
	return m_marginLeft + ((tick - m_currentPosition) * m_ppb / TimePos::ticksPerBar());
}


void StepRecorderWidget::drawVerLine(QPainter* painter, int x, const QColor& color, int top, int bottom)
{
	if(x >= m_marginLeft && x <= (width() - m_marginRight))
	{
		painter->setPen(color);
		painter->drawLine( x, top, x, bottom );
	}
}

void StepRecorderWidget::drawVerLine(QPainter* painter, const TimePos& pos, const QColor& color, int top, int bottom)
{
	drawVerLine(painter, xCoordOfTick(pos), color, top, bottom);
}

void StepRecorderWidget::updateBoundaries()
{
	setFixedSize(parentWidget()->size());

	m_bottom = height() - m_marginBottom;
	m_right = width() - m_marginTop;

	//(no need to change top and left as they are static)
}


} // namespace lmms::gui