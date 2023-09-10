/*
 * WaveForm.cpp - slice editor for SlicerT
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

#include "WaveForm.h"
#include "SlicerT.h"


namespace lmms
{


namespace gui
{
    WaveForm::WaveForm(int w, int h, SlicerT  * instrument, QWidget * parent) :
        QWidget(parent),
        m_sliceEditor(QPixmap(w, h*(1 - m_m_seekerRatio) - m_margin)),
        m_seeker(QPixmap(w, h*m_m_seekerRatio)),
        m_seekerWaveform(QPixmap(w, h*m_m_seekerRatio)),
        m_currentSample(instrument->m_originalSample.data(), instrument->m_originalSample.frames()),
        m_slicePoints(instrument->m_slicePoints)
        {
            m_width = w;
            m_height = h;
            m_slicerTParent = instrument;
            setFixedSize(m_width, m_height);
            setMouseTracking( true );

            m_sliceEditor.fill(m_waveformBgColor);
            m_seekerWaveform.fill(m_waveformBgColor);

            connect(m_slicerTParent,
                    SIGNAL(isPlaying(float, float, float)),
                    this,
                    SLOT(isPlaying(float, float, float)));

            connect(m_slicerTParent, SIGNAL(dataChanged()), this, SLOT(updateData()));

            updateUI();
        }

    void WaveForm::drawEditor() {
        m_sliceEditor.fill(m_waveformBgColor);
        QPainter brush(&m_sliceEditor);

        float startFrame = m_seekerStart * m_currentSample.frames();
        float endFrame = m_seekerEnd * m_currentSample.frames();

        brush.setPen(m_playHighlighColor);
        brush.drawLine(0, m_sliceEditor.height()/2, m_sliceEditor.width(), m_sliceEditor.height()/2);

        brush.setPen(m_waveformColor);
        m_currentSample.visualize(
		brush,
		QRect( 0, 0, m_sliceEditor.width(), m_sliceEditor.height() ),
		startFrame, endFrame);


        for (int i = 0;i<m_slicePoints.size();i++) {
            int sliceIndex = m_slicePoints[i];
            brush.setPen(QPen(m_sliceColor, 2));

            if (sliceIndex >= startFrame && sliceIndex <= endFrame)
            {
                float xPos = (float)(sliceIndex - startFrame) / (float)(endFrame - startFrame) * (float)m_width;
                if (i == m_sliceSelected)
                {
                    brush.setPen(QPen(m_selectedSliceColor, 2));
                }

                brush.drawLine(xPos, 0, xPos, m_height);
            }
        }
    }

    void WaveForm::drawSeekerWaveform() {
        QPainter brush(&m_seekerWaveform);
        brush.setPen(m_waveformColor);

        m_seekerWaveform.fill(m_waveformBgColor);
        m_currentSample.visualize(
		brush,
		QRect( 0, 0, m_seekerWaveform.width(), m_seekerWaveform.height() ),
		0, m_currentSample.frames());
    }

    void WaveForm::drawSeeker() {
        m_seeker.fill(QColor(0, 0, 0, 0));
        QPainter brush(&m_seeker);
        brush.setPen(m_waveformColor);

        // draw slice points
        brush.setPen(m_sliceColor);
        for (int i = 0;i<m_slicePoints.size();i++)
        {
            float xPos = (float)m_slicePoints[i] / (float)m_currentSample.frames() * (float)m_width;
            brush.drawLine(xPos, 0, xPos, m_height);
        }

        // draw current playBack
        brush.setPen(m_playColor);
        brush.drawLine(m_noteCurrent*m_width, 0, m_noteCurrent*m_width, m_height);
        brush.fillRect(m_noteStart*m_width, 0, (m_noteEnd-m_noteStart)*m_width, m_height, m_playHighlighColor);

        // draw m_seeker points
        brush.setPen(QPen(m_seekerColor, 3));
        brush.drawLine(m_seekerStart*m_width, 0, m_seekerStart*m_width, m_height);
        brush.drawLine(m_seekerEnd*m_width, 0, m_seekerEnd*m_width, m_height);

        // shadow on not selected area
        brush.fillRect(0, 0, m_seekerStart*m_width, m_height, m_seekerShadowColor);
        brush.fillRect(m_seekerEnd*m_width, 0, m_width, m_height, m_seekerShadowColor);
    }

    void WaveForm::updateUI() {
        drawSeekerWaveform();
        drawSeeker();
        drawEditor();
        update();
    }

    void WaveForm::updateData() {
        m_currentSample = SampleBuffer(m_slicerTParent->m_originalSample.data(), m_slicerTParent->m_originalSample.frames());
        updateUI();
    }

    void WaveForm::isPlaying(float current, float start, float end) {
        m_noteCurrent = current;
        m_noteStart = start;
        m_noteEnd = end;
        drawSeeker(); // only update seeker, else horrible performance
        update();
    }

    void WaveForm::mousePressEvent( QMouseEvent * me ) {
        float normalizedClick = (float)me->x() / m_width;

        if (me->button() == Qt::MouseButton::MiddleButton)
        {
            m_seekerStart = 0;
            m_seekerEnd = 1;
            return;
        }

        if (me->y() < m_height*m_m_seekerRatio)
        {
            if (abs(normalizedClick - m_seekerStart) < 0.03)
            {
                m_currentlyDragging = m_draggingTypes::m_seekerStart;

            } else if (abs(normalizedClick - m_seekerEnd) < 0.03)
            {
                m_currentlyDragging = m_draggingTypes::m_seekerEnd;

            } else if (normalizedClick > m_seekerStart && normalizedClick < m_seekerEnd)
            {
                m_currentlyDragging = m_draggingTypes::m_seekerMiddle;
                m_seekerMiddle = normalizedClick;
            }

        } else {
            m_sliceSelected = -1;
            float startFrame = m_seekerStart * m_currentSample.frames();
            float endFrame = m_seekerEnd * m_currentSample.frames();

            for (int i = 0;i<m_slicePoints.size();i++)
            {
                int sliceIndex = m_slicePoints[i];
                float xPos = (float)(sliceIndex - startFrame) / (float)(endFrame - startFrame);

                if (abs(xPos - normalizedClick) < 0.03)
                {
                    m_currentlyDragging = m_draggingTypes::slicePoint;
                    m_sliceSelected = i;
                }
            }
        }
        if (me->button() == Qt::MouseButton::LeftButton)
        {
            m_isDragging = true;

        } else if (me->button() == Qt::MouseButton::RightButton)
        {
            if (m_sliceSelected != -1 && m_slicePoints.size() > 2)
            {
                m_slicePoints.erase(m_slicePoints.begin() + m_sliceSelected);
                m_sliceSelected = -1;
            }
        }

    }

    void WaveForm::mouseReleaseEvent( QMouseEvent * me ) {
        m_isDragging = false;
        m_currentlyDragging = m_draggingTypes::nothing;
        updateUI();
    }

    void WaveForm::mouseMoveEvent( QMouseEvent * me ) {
        float normalizedClick = (float)me->x() / m_width;

        // handle dragging events
        if (m_isDragging) {
            if (m_currentlyDragging == m_draggingTypes::m_seekerStart)
            {
                m_seekerStart = std::clamp(normalizedClick, 0.0f, m_seekerEnd - 0.13f);

            } else if (m_currentlyDragging == m_draggingTypes::m_seekerEnd)
            {
                m_seekerEnd = std::clamp(normalizedClick, m_seekerStart + 0.13f, 1.0f);;

            } else if (m_currentlyDragging == m_draggingTypes::m_seekerMiddle)
            {
                float distStart = m_seekerStart - m_seekerMiddle;
                float distEnd = m_seekerEnd - m_seekerMiddle;

                m_seekerMiddle = normalizedClick;

                if (m_seekerMiddle + distStart > 0 && m_seekerMiddle + distEnd < 1)
                {
                    m_seekerStart = m_seekerMiddle + distStart;
                    m_seekerEnd = m_seekerMiddle + distEnd;
                }

            } else if (m_currentlyDragging == m_draggingTypes::slicePoint)
            {
                float startFrame = m_seekerStart * m_currentSample.frames();
                float endFrame = m_seekerEnd * m_currentSample.frames();

                m_slicePoints[m_sliceSelected] = startFrame + normalizedClick * (endFrame - startFrame);

                m_slicePoints[m_sliceSelected] = std::clamp(m_slicePoints[m_sliceSelected], 0, m_currentSample.frames());

                std::sort(m_slicePoints.begin(), m_slicePoints.end());
            }
            updateUI();
        }
    }

    void WaveForm::mouseDoubleClickEvent(QMouseEvent * me) {
        float normalizedClick = (float)me->x() / m_width;
        float startFrame = m_seekerStart * m_currentSample.frames();
        float endFrame = m_seekerEnd * m_currentSample.frames();

        float slicePosition = startFrame + normalizedClick * (endFrame - startFrame);

        for (int i = 0;i<m_slicePoints.size();i++)
        {
            if (m_slicePoints[i] < slicePosition)
            {
                m_slicePoints.insert(m_slicePoints.begin() + i, slicePosition);
                break;
            }
        }

        std::sort(m_slicePoints.begin(), m_slicePoints.end());
    }

    void WaveForm::paintEvent( QPaintEvent * pe) {
        QPainter p( this );
        p.drawPixmap(0, 0 ,m_seekerWaveform);
        p.drawPixmap(0, 0, m_seeker);
        p.drawPixmap(0, m_height*0.3f + m_margin, m_sliceEditor);

    }
} // namespace gui
} // namespace lmms