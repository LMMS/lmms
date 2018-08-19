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

#include "StepRecorderWidget.h"

StepRecorderWidget::StepRecorderWidget( 
        QWidget * parent, 
        const int ppt,    
        const int margin_top,
        const int margin_bottom,
        const int margin_left,
        const int margin_right) : 
	QWidget(parent),
	m_margin_top(margin_top),
    m_margin_bottom(margin_bottom),
    m_margin_left(margin_left),
    m_margin_right(margin_right)
{
	const QColor baseColor =  QColor(255, 0, 0);// QColor(204, 163, 0); // Orange
	m_colorLineEnd   = baseColor.lighter(150);
	m_colorLineStart = baseColor.darker(120);

	setAttribute(Qt::WA_NoSystemBackground, true);
	setPixelsPerTact(ppt);

	m_top = m_margin_top;
	m_left = m_margin_left;
}

void StepRecorderWidget::setPixelsPerTact(int _ppt )
{
	m_ppt = _ppt;
}

void StepRecorderWidget::setCurrentPosition(MidiTime currentPosition)
{
	m_currentPosition = currentPosition;
}

void StepRecorderWidget::setBottomMargin(const int margin_bottom)
{
	m_margin_bottom = margin_bottom;
}

void StepRecorderWidget::setStartPosition(MidiTime pos)
{
	m_curStepStartPos = pos;	
}
void StepRecorderWidget::setEndPosition(MidiTime pos)
{
	m_curStepEndPos = pos;	
}

void StepRecorderWidget::setStepsLength(MidiTime stepsLength)
{
	m_stepsLength = stepsLength;
}

void StepRecorderWidget::paintEvent( QPaintEvent * pe )
{
	QPainter painter( this );

	updateBoundaries();

	move(0, 0);

	//draw steps ruler
	painter.setPen(m_colorLineEnd);

	MidiTime curPos = m_curStepEndPos;
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
	return m_margin_left + ((tick - m_currentPosition) * m_ppt / MidiTime::ticksPerTact() );
}


void StepRecorderWidget::drawVerLine(QPainter* painter, int x, const QColor& color, int top, int bottom)
{
	if(x >= m_margin_left && x <= (width() - m_margin_right))
	{
		painter->setPen(color);
		painter->drawLine( x, top, x, bottom );
	}
}

void StepRecorderWidget::drawVerLine(QPainter* painter, const MidiTime& pos, const QColor& color, int top, int bottom)
{
	drawVerLine(painter, xCoordOfTick(pos), color, top, bottom);
}

void StepRecorderWidget::updateBoundaries()
{
	setFixedSize(parentWidget()->size());

	m_bottom = height() - m_margin_bottom;
	m_right = width() - m_margin_top;

	//(no need to change top and left - they are static)
}

