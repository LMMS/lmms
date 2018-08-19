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
#ifndef _STEP_RECOREDER_WIDGET
#define _STEP_RECOREDER_WIDGET

#include "lmms_basics.h"
#include "Note.h"

#include <QWidget>
#include <QColor>
#include <QPainter>

class StepRecorderWidget : public QWidget
{
public:
	StepRecorderWidget(
        QWidget * parent, 
        const int _ppt,    
        const int margin_top,
        const int margin_bottom,
        const int margin_left,
        const int margin_right);

	//API used by PianoRoll 
	void setPixelsPerTact(int _ppt);
	void setCurrentPosition(MidiTime currentPosition);
    void setBottomMargin(const int margin_bottom);

	//API used by StepRecorder
    void setStepsLength(MidiTime stepsLength);
	void setStartPosition(MidiTime pos);
    void setEndPosition(MidiTime pos);
private:
	virtual void paintEvent(QPaintEvent * pe);
    
    int xCoordOfTick(int tick);
    
    void drawVerLine(QPainter* painter, int x, const QColor& color, int top, int bottom);
    void drawVerLine(QPainter* painter, const MidiTime& pos, const QColor& color, int top, int bottom);
    
    void updateBoundaries();

    MidiTime m_stepsLength;
	MidiTime m_curStepStartPos;
    MidiTime m_curStepEndPos;

	int m_ppt; // pixels per tact	
	MidiTime m_currentPosition; // current position showed by on PianoRoll 

	QColor m_colorLineStart;
    QColor m_colorLineEnd;

    // boundaries within piano roll window
    int m_top;
    int m_bottom;
    int m_left;
    int m_right;

    const int m_margin_top;
    int m_margin_bottom; // not const since can change on resize of edit-note area
    const int m_margin_left;
    const int m_margin_right;
} ;

#endif //_STEP_RECOREDER_WIDGET
