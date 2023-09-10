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
        int m_width;
        int m_height;
        float m_m_seekerRatio = 0.3f;
        int m_margin = 5;
        QColor m_waveformBgColor = QColor(11, 11, 11);
        QColor m_waveformColor = QColor(124, 49, 214);
        QColor m_playColor = QColor(255, 255, 255, 200);
        QColor m_playHighlighColor = QColor(255, 255, 255, 70);
        QColor m_sliceColor = QColor(49, 214, 124);
        QColor m_selectedSliceColor = QColor(172, 236, 190);
        QColor m_seekerColor = QColor(214, 124, 49);
        QColor m_seekerShadowColor = QColor(0, 0, 0, 175);

        enum class m_draggingTypes {
            nothing,
            m_seekerStart,
            m_seekerEnd,
            m_seekerMiddle,
            slicePoint,
        };
        m_draggingTypes m_currentlyDragging;
        bool m_isDragging = false;

        float m_seekerStart = 0;
        float m_seekerEnd = 1;
        float m_seekerMiddle = 0.5f;
        int m_sliceSelected = 0;

        float m_noteCurrent;
        float m_noteStart;
        float m_noteEnd;

        QPixmap m_sliceEditor;
        QPixmap m_seeker;

        SampleBuffer m_currentSample;

        void drawEditor();
        void drawSeeker();
        void updateUI();

    public slots:
        void updateData();
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