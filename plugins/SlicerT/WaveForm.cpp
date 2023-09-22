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

#include "embed.h"

namespace lmms
{


namespace gui
{
WaveForm::WaveForm(int w, int h, SlicerT  * instrument, QWidget * parent) :
    QWidget(parent),
    // calculate sizes
    m_width(w),
    m_height(h),
    m_seekerWidth(w - m_seekerHorMargin * 2),
    m_editorHeight(h - m_seekerHeight - m_middleMargin),
    m_editorWidth(w),

    // create pixmaps
    m_sliceArrow(PLUGIN_NAME::getIconPixmap( "slide_indicator_arrow" )),
    m_seeker(QPixmap(m_seekerWidth, m_seekerHeight)),
    m_seekerWaveform(QPixmap(m_seekerWidth, m_seekerHeight)),
    m_sliceEditor(QPixmap(w, m_editorHeight)),

    // references to instrument vars
    m_slicerTParent(instrument),
    m_currentSample(instrument->m_originalSample),
    m_slicePoints(instrument->m_slicePoints)
    {
        setFixedSize(m_width, m_height);
        setMouseTracking( true );

        m_sliceEditor.fill(m_waveformBgColor);
        m_seekerWaveform.fill(m_waveformBgColor);

        connect(m_slicerTParent,
                SIGNAL(isPlaying(float, float, float)),
                this,
                SLOT(isPlaying(float, float, float)));
        connect(m_slicerTParent, SIGNAL(dataChanged()), this, SLOT(updateUI()));

        updateUI();
    }

// graphics
void WaveForm::drawSeekerWaveform()
{
    m_seekerWaveform.fill(m_waveformBgColor);
    QPainter brush(&m_seekerWaveform);
    brush.setPen(m_waveformColor);

    m_currentSample.visualize(
    brush,
    QRect( 0, 0, m_seekerWaveform.width(), m_seekerWaveform.height() ),
    0, m_currentSample.frames());
}

void WaveForm::drawSeeker()
{
    m_seeker.fill(m_waveformBgColor);
    QPainter brush(&m_seeker);
    // draw slice points
    brush.setPen(m_sliceColor);
    for (int i = 0;i<m_slicePoints.size();i++)
    {
        float xPos = (float)m_slicePoints[i] / m_currentSample.frames() * m_seekerWidth;
        brush.drawLine(xPos, 0, xPos, m_seekerHeight);
    }

    // this is a complete mess
    // draw current playBack
    brush.setPen(m_playColor);
    brush.drawLine(m_noteCurrent*m_seekerWidth, 0, m_noteCurrent*m_seekerWidth, m_seekerHeight);
    brush.fillRect(m_noteStart*m_seekerWidth, 0, (m_noteEnd-m_noteStart)*m_seekerWidth, m_seekerHeight, m_playHighlighColor);

    // highlight on selected area
    brush.fillRect(m_seekerStart*m_seekerWidth, 0, (m_seekerEnd-m_seekerStart)*m_seekerWidth, m_seekerHeight, m_seekerHighlightColor);

    // shadow on not selected area
    brush.fillRect(0, 0, m_seekerStart*m_seekerWidth, m_seekerHeight, m_seekerShadowColor);
    brush.fillRect(m_seekerEnd*m_seekerWidth+1, 0, m_seekerWidth+1, m_seekerHeight, m_seekerShadowColor);

    // draw border around selection
    brush.setPen(QPen(m_seekerColor, 1));
    brush.drawRoundedRect(m_seekerStart*m_seekerWidth, 0, (m_seekerEnd-m_seekerStart)*m_seekerWidth-1, m_seekerHeight-1, 4, 4); // -1 needed
}

void WaveForm::drawEditor()
{
    m_sliceEditor.fill(m_waveformBgColor);
    QPainter brush(&m_sliceEditor);

    float startFrame = m_seekerStart * m_currentSample.frames();
    float endFrame = m_seekerEnd * m_currentSample.frames();
    float numFramesToDraw = endFrame - startFrame;

    // 0 centered line
    brush.setPen(m_playHighlighColor);
    brush.drawLine(0, m_editorHeight/2, m_editorWidth, m_editorHeight/2);

    // draw waveform
    brush.setPen(m_waveformColor);
    m_currentSample.visualize(
    brush,
    QRect( 0, 0, m_editorWidth, m_editorHeight ),
    startFrame, endFrame);

    // draw slicepoints
    brush.setPen(QPen(m_sliceColor, 2));
    for (int i = 0;i<m_slicePoints.size();i++)
    {
        float xPos = (float)(m_slicePoints[i] - startFrame) / numFramesToDraw * m_editorWidth;

        if (i == m_sliceSelected)
        {
            brush.setPen(QPen(m_selectedSliceColor, 2));
        } else
        {
            brush.setPen(QPen(m_sliceColor, 2));
        }

        brush.drawLine(xPos, 0, xPos, m_editorHeight);
        brush.drawPixmap(xPos - (float)m_sliceArrow.width()/2, 0, m_sliceArrow);
    }
}

// slots
void WaveForm::updateUI()
{
    drawSeekerWaveform();
    drawSeeker();
    drawEditor();
    update();
}

void WaveForm::isPlaying(float current, float start, float end)
{
    m_noteCurrent = current;
    m_noteStart = start;
    m_noteEnd = end;
    drawSeeker(); // only update seeker, else horrible performance
    update();
}

// events
void WaveForm::mousePressEvent( QMouseEvent * me )
{
    float normalizedClickSeeker = (float)(me->x() - m_seekerHorMargin) / m_seekerWidth;
    float normalizedClickEditor = (float)(me->x()) / m_editorWidth;
    // reset seeker on middle click
    if (me->button() == Qt::MouseButton::MiddleButton)
    {
        m_seekerStart = 0;
        m_seekerEnd = 1;
        return;
    }

    if (me->y() < m_seekerHeight) // seeker click
    {
        if (abs(normalizedClickSeeker - m_seekerStart) < distanceForClick) // dragging start
        {
            m_currentlyDragging = m_draggingTypes::m_seekerStart;
        } else if (abs(normalizedClickSeeker - m_seekerEnd) < distanceForClick) // dragging end
        {
            m_currentlyDragging = m_draggingTypes::m_seekerEnd;

        } else if (normalizedClickSeeker > m_seekerStart && normalizedClickSeeker < m_seekerEnd) // dragging middle
        {
            m_currentlyDragging = m_draggingTypes::m_seekerMiddle;
            m_seekerMiddle = normalizedClickSeeker;
        }
    } else { // editor click
        m_sliceSelected = -1;
        float startFrame = m_seekerStart * m_currentSample.frames();
        float endFrame = m_seekerEnd * m_currentSample.frames();
        // select slice
        for (int i = 0;i<m_slicePoints.size();i++)
        {
            int sliceIndex = m_slicePoints[i];
            float xPos = (float)(sliceIndex - startFrame) / (float)(endFrame - startFrame);

            if (abs(xPos - normalizedClickEditor) < distanceForClick)
            {
                m_currentlyDragging = m_draggingTypes::m_slicePoint;
                m_sliceSelected = i;
            }
        }
    }

    if (me->button() == Qt::MouseButton::RightButton) // erase selected slice
    {
        if (m_sliceSelected != -1 && m_slicePoints.size() > 2)
        {
            m_slicePoints.erase(m_slicePoints.begin() + m_sliceSelected);
            m_sliceSelected = -1;
        }
    }
    updateUI();
}

void WaveForm::mouseReleaseEvent( QMouseEvent * me )
{
    m_currentlyDragging = m_draggingTypes::nothing;
    updateUI();
}

void WaveForm::mouseMoveEvent( QMouseEvent * me )
{
    float normalizedClickSeeker = (float)(me->x() - m_seekerHorMargin) / m_seekerWidth;
    float normalizedClickEditor = (float)(me->x()) / m_editorWidth;

    float distStart = m_seekerStart - m_seekerMiddle;
    float distEnd = m_seekerEnd - m_seekerMiddle;
    float startFrame = m_seekerStart * m_currentSample.frames();
    float endFrame = m_seekerEnd * m_currentSample.frames();

    // handle dragging events
    switch (m_currentlyDragging)
    {
    case m_draggingTypes::m_seekerStart:
        m_seekerStart = std::clamp(normalizedClickSeeker, 0.0f, m_seekerEnd - minSeekerDistance);
        break;

    case m_draggingTypes::m_seekerEnd:
        m_seekerEnd = std::clamp(normalizedClickSeeker, m_seekerStart + minSeekerDistance, 1.0f);
        break;

    case m_draggingTypes::m_seekerMiddle:
        m_seekerMiddle = normalizedClickSeeker;

        if (m_seekerMiddle + distStart >= 0 && m_seekerMiddle + distEnd <= 1)
        {
            m_seekerStart = m_seekerMiddle + distStart;
            m_seekerEnd = m_seekerMiddle + distEnd;
        }
        break;

    case m_draggingTypes::m_slicePoint: // TODO: fix this
        m_slicePoints[m_sliceSelected] = startFrame + normalizedClickEditor * (endFrame - startFrame);

        m_slicePoints[m_sliceSelected] = std::clamp(m_slicePoints[m_sliceSelected], 0, m_currentSample.frames());

        std::sort(m_slicePoints.begin(), m_slicePoints.end());
        break;
    case m_draggingTypes::nothing:
        break;
    }
    updateUI();
}

void WaveForm::mouseDoubleClickEvent(QMouseEvent * me)
{
    float normalizedClickEditor = (float)(me->x()) / m_editorWidth;
    float startFrame = m_seekerStart * m_currentSample.frames();
    float endFrame = m_seekerEnd * m_currentSample.frames();

    float slicePosition = startFrame + normalizedClickEditor * (endFrame - startFrame);

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

void WaveForm::paintEvent( QPaintEvent * pe)
{
    QPainter p( this );
    p.drawPixmap(m_seekerHorMargin, 0 ,m_seekerWaveform);
    p.drawPixmap(m_seekerHorMargin, 0, m_seeker);
    p.drawPixmap(0, m_seekerHeight + m_middleMargin, m_sliceEditor);
}
} // namespace gui
} // namespace lmms