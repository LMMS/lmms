/*
 * StepRecorderWidget.h - widget that provide gui markers for step recording
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of"the GNU General Public
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

#ifndef LMMS_GUI_STEP_RECOREDER_WIDGET_H
#define LMMS_GUI_STEP_RECOREDER_WIDGET_H

#include <QWidget>
#include <QColor>

#include "TimePos.h"

namespace lmms::gui
{

class StepRecorderWidget : public QWidget
{
	Q_OBJECT

public:
	StepRecorderWidget(
		QWidget * parent,
		const int ppb,
		const int marginTop,
		const int marginBottom,
		const int marginLeft,
		const int marginRight);

	//API used by PianoRoll
	void setPixelsPerBar(int ppb);
	void setCurrentPosition(TimePos currentPosition);
	void setMargins(const QMargins &qm);
	void setBottomMargin(const int marginBottom);
	QMargins margins();

	//API used by StepRecorder
	void setStepsLength(TimePos stepsLength);
	void setStartPosition(TimePos pos);
	void setEndPosition(TimePos pos);

	void showHint();

private:
	void paintEvent(QPaintEvent * pe) override;

	int xCoordOfTick(int tick);

	void drawVerLine(QPainter* painter, int x, const QColor& color, int top, int bottom);
	void drawVerLine(QPainter* painter, const TimePos& pos, const QColor& color, int top, int bottom);

	void updateBoundaries();

	TimePos m_stepsLength;
	TimePos m_curStepStartPos;
	TimePos m_curStepEndPos;

	int m_ppb; // pixels per bar
	TimePos m_currentPosition; // current position showed by on PianoRoll

	QColor m_colorLineStart;
	QColor m_colorLineEnd;

	// boundaries within piano roll window
	int m_top;
	int m_bottom;
	int m_left;
	int m_right;

	const int m_marginTop;
	int m_marginBottom; // not const since can change on resize of edit-note area
	const int m_marginLeft;
	const int m_marginRight;

signals:
	void positionChanged(const lmms::TimePos & t);
} ;

} // namespace lmms::gui

#endif // LMMS_GUI_STEP_RECOREDER_WIDGET_H
