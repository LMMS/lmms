/*
 * WaveForm.h - declaration of class WaveForm
 *
 * Copyright (c) 2006-2008 Andreas Brandmaier <andy/at/brandmaier/dot/de>
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

#ifndef WAVEFORM_H
#define WAVEFORM_H

#include <QApplication>
#include <QFontMetrics>
#include <QInputDialog>
#include <QMouseEvent>
#include <QPainter>

#include "SampleBuffer.h"


namespace lmms
{

class SlicerT;

namespace gui
{


class WaveForm : public QWidget {
    	Q_OBJECT

    protected:
        virtual void mousePressEvent(QMouseEvent * me);
        virtual void mouseReleaseEvent(QMouseEvent * me);
        virtual void mouseMoveEvent(QMouseEvent * me);
        virtual void mouseDoubleClickEvent(QMouseEvent * me);

        virtual void paintEvent(QPaintEvent * pe);

    private:
        // vars used to control structure and colors
        int m_width;
        int m_height;

        int m_seekerHorMargin = 5;
        int m_seekerHeight = 38; // used to calcualte all hor sizes
        int m_seekerWidth;

        int m_middleMargin = 6;
        int m_editorHeight;
        int m_editorWidth;


        QColor m_waveformBgColor = QColor(255, 255, 255, 0);
        QColor m_waveformColor = QColor(123, 49, 212);
        // QColor m_waveformColorDark = QColor(39, 15, 67);

        // QColor m_waveformColor = QColor(255, 161, 247); // logo color
        QColor m_playColor = QColor(255, 255, 255, 200);
        QColor m_playHighlighColor = QColor(255, 255, 255, 70);
        QColor m_sliceColor = QColor(218, 193, 255);
        QColor m_selectedSliceColor = QColor(178, 153, 215);
        QColor m_seekerColor = QColor(178, 115, 255);
        QColor m_seekerHighlightColor = QColor(178, 115, 255, 100);
        QColor m_seekerShadowColor = QColor(0, 0, 0, 120);

        // interaction vars
        float distanceForClick = 0.03f;
        float minSeekerDistance = 0.13f;

        // dragging vars
        enum class m_draggingTypes
        {
            nothing,
            m_seekerStart,
            m_seekerEnd,
            m_seekerMiddle,
            m_slicePoint,
        };
        m_draggingTypes m_currentlyDragging;

        // seeker vars
        float m_seekerStart = 0;
        float m_seekerEnd = 1;
        float m_seekerMiddle = 0.5f;
        int m_sliceSelected = 0;

        // playback highlight vars
        float m_noteCurrent;
        float m_noteStart;
        float m_noteEnd;

        // pixmaps
        QPixmap m_sliceArrow;
        QPixmap m_seeker;
        QPixmap m_seekerWaveform; // only stores waveform graphic
        QPixmap m_sliceEditor;

        SampleBuffer & m_currentSample;

        void drawEditor();
        void drawSeekerWaveform();
        void drawSeeker();
    public slots:
        void updateUI();
        void isPlaying(float current, float start, float end);

    public:
        WaveForm(int w, int h, SlicerT * instrument, QWidget * parent);

    private:
        SlicerT * m_slicerTParent;
        std::vector<int> & m_slicePoints;
};
} // namespace gui
} // namespace lmms
#endif // WAVEFORM_H