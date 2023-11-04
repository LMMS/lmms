/*
 * SlicerTWaveform.cpp - slice editor for SlicerT
 *
 * Copyright (c) 2023 Daniel Kauss Serna <daniel.kauss.serna@gmail.com>
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

#include "SlicerTWaveform.h"

#include <QBitmap>

#include "SlicerT.h"
#include "SlicerTView.h"
#include "embed.h"

namespace lmms {

namespace gui {

SlicerTWaveform::SlicerTWaveform(int w, int h, SlicerT* instrument, QWidget* parent)
	: QWidget(parent)
	// calculate sizes
	, m_width(w)
	, m_height(h)
	, m_seekerWidth(w - m_seekerHorMargin * 2)
	, m_editorHeight(h - m_seekerHeight - m_middleMargin)
	, m_editorWidth(w)

	// create pixmaps
	, m_sliceArrow(PLUGIN_NAME::getIconPixmap("slide_indicator_arrow"))
	, m_seeker(QPixmap(m_seekerWidth, m_seekerHeight))
	, m_seekerWaveform(QPixmap(m_seekerWidth, m_seekerHeight))
	, m_editorWaveform(QPixmap(m_editorWidth, m_editorHeight))
	, m_sliceEditor(QPixmap(w, m_editorHeight))
	, m_emptySampleIcon(embed::getIconPixmap("sample_track.png"))

	// references to instrument vars
	, m_slicerTParent(instrument)
{
	// window config
	setFixedSize(m_width, m_height);
	setMouseTracking(true);

	// draw backgrounds
	m_seekerWaveform.fill(s_waveformBgColor);
	m_editorWaveform.fill(s_waveformBgColor);

	// connect to playback
	connect(instrument, &SlicerT::isPlaying, this, &SlicerTWaveform::isPlaying);
	connect(instrument, &SlicerT::dataChanged, this, &SlicerTWaveform::updateUI);

	// preprocess icons
	m_emptySampleIcon = m_emptySampleIcon.createMaskFromColor(QColor(255, 255, 255), Qt::MaskMode::MaskOutColor);

	updateUI();
}

void SlicerTWaveform::drawSeekerWaveform()
{
	m_seekerWaveform.fill(s_waveformBgColor);
	if (m_slicerTParent->m_originalSample.frames() < 2048) { return; }
	QPainter brush(&m_seekerWaveform);
	brush.setPen(s_waveformColor);

	m_slicerTParent->m_originalSample.visualize(brush, QRect(0, 0, m_seekerWaveform.width(), m_seekerWaveform.height()),
		0, m_slicerTParent->m_originalSample.frames());

	// increase brightness in inner color
	QBitmap innerMask = m_seekerWaveform.createMaskFromColor(s_waveformMaskColor, Qt::MaskMode::MaskOutColor);
	brush.setPen(s_waveformInnerColor);
	brush.drawPixmap(0, 0, innerMask);
}

void SlicerTWaveform::drawSeeker()
{
	m_seeker.fill(s_emptyColor);
	if (m_slicerTParent->m_originalSample.frames() < 2048) { return; }
	QPainter brush(&m_seeker);

	// draw slice points
	brush.setPen(s_sliceColor);
	for (int i = 0; i < m_slicerTParent->m_slicePoints.size(); i++)
	{
		float xPos = static_cast<float>(m_slicerTParent->m_slicePoints.at(i))
			 * m_seekerWidth;
		brush.drawLine(xPos, 0, xPos, m_seekerHeight);
	}

	// seeker vars
	float seekerStartPosX = m_seekerStart * m_seekerWidth;
	float seekerEndPosX = m_seekerEnd * m_seekerWidth;
	float seekerMiddleWidth = (m_seekerEnd - m_seekerStart) * m_seekerWidth;

	// note playback vars
	float noteCurrentPosX = m_noteCurrent * m_seekerWidth;
	float noteStartPosX = m_noteStart * m_seekerWidth;
	float noteEndPosX = (m_noteEnd - m_noteStart) * m_seekerWidth;

	// draw current playBack
	brush.setPen(s_playColor);
	brush.drawLine(noteCurrentPosX, 0, noteCurrentPosX, m_seekerHeight);
	brush.fillRect(noteStartPosX, 0, noteEndPosX, m_seekerHeight, s_playHighlightColor);

	// highlight on selected area
	brush.fillRect(seekerStartPosX, 0, seekerMiddleWidth - 1, m_seekerHeight, s_seekerHighlightColor);

	// shadow on not selected area
	brush.fillRect(0, 0, seekerStartPosX, m_seekerHeight, s_seekerShadowColor);
	brush.fillRect(seekerEndPosX - 1, 0, m_seekerWidth, m_seekerHeight, s_seekerShadowColor);

	// draw border around selection
	brush.setPen(QPen(s_seekerColor, 1));
	brush.drawRect(seekerStartPosX, 0, seekerMiddleWidth - 1, m_seekerHeight - 1); // -1 needed
}

void SlicerTWaveform::drawEditorWaveform()
{
	m_editorWaveform.fill(s_emptyColor);
	if (m_slicerTParent->m_originalSample.frames() < 2048) { return; }

	// draw SlicerTWaveform
	QPainter brush(&m_editorWaveform);
	float startFrame = m_seekerStart * m_slicerTParent->m_originalSample.frames();
	float endFrame = m_seekerEnd * m_slicerTParent->m_originalSample.frames();

	brush.setPen(s_waveformColor);
	float zoomOffset = (m_editorHeight - m_zoomLevel * m_editorHeight) / 2;
	m_slicerTParent->m_originalSample.visualize(
		brush, QRect(0, zoomOffset, m_editorWidth, m_zoomLevel * m_editorHeight), startFrame, endFrame);

	// increase brightness in inner color
	QBitmap innerMask = m_editorWaveform.createMaskFromColor(s_waveformMaskColor, Qt::MaskMode::MaskOutColor);
	brush.setPen(s_waveformInnerColor);
	brush.drawPixmap(0, 0, innerMask);
}

void SlicerTWaveform::drawEditor()
{
	m_sliceEditor.fill(s_waveformBgColor);
	QPainter brush(&m_sliceEditor);

	// draw text if no sample loaded
	if (m_slicerTParent->m_originalSample.frames() < 2048)
	{
		brush.setPen(s_playHighlightColor);
		brush.setFont(QFont(brush.font().family(), 9.0f, -1, false));
		brush.drawText(
			m_editorWidth / 2 - 100, m_editorHeight / 2 - 110, 200, 200, Qt::AlignCenter, tr("Drag sample to load"));
		int iconOffsetX = m_emptySampleIcon.width() / 2.0f;
		int iconOffsetY = m_emptySampleIcon.height() / 2.0f - 13;
		brush.drawPixmap(m_editorWidth / 2.0f - iconOffsetX, m_editorHeight / 2.0f - iconOffsetY, m_emptySampleIcon);
		return;
	}

	// editor boundaries
	float startFrame = m_seekerStart;
	float endFrame = m_seekerEnd;
	float numFramesToDraw = endFrame - startFrame;

	// playback state
	float noteCurrentPos = (m_noteCurrent - m_seekerStart) / (m_seekerEnd - m_seekerStart) * m_editorWidth;
	float noteStartPos = (m_noteStart - m_seekerStart) / (m_seekerEnd - m_seekerStart) * m_editorWidth;
	float noteLength = (m_noteEnd - m_noteStart) / (m_seekerEnd - m_seekerStart) * m_editorWidth;

	// 0 centered line
	brush.setPen(s_playHighlightColor);
	brush.drawLine(0, m_editorHeight / 2, m_editorWidth, m_editorHeight / 2);

	// draw waveform from pixmap
	brush.drawPixmap(0, 0, m_editorWaveform);

	// draw currently playing
	brush.setPen(s_playColor);
	brush.drawLine(noteCurrentPos, 0, noteCurrentPos, m_editorHeight);
	brush.fillRect(noteStartPos, 0, noteLength, m_editorHeight, s_playHighlightColor);

	// draw slicepoints
	brush.setPen(QPen(s_sliceColor, 2));

	for (int i = 0; i < m_slicerTParent->m_slicePoints.size(); i++)
	{
		float xPos = (m_slicerTParent->m_slicePoints.at(i) - startFrame) / numFramesToDraw * m_editorWidth;

		if (i == m_closestSlice)
		{
			brush.setPen(QPen(s_sliceHighlightColor, 2));
			brush.drawLine(xPos, 0, xPos, m_editorHeight);
			brush.drawPixmap(xPos - m_sliceArrow.width() / 2.0f, 0, m_sliceArrow);
			continue;
		}
		else
		{
			brush.setPen(QPen(s_sliceShadowColor, 1));
			brush.drawLine(xPos - 1, 0, xPos - 1, m_editorHeight);
			brush.setPen(QPen(s_sliceColor, 1));
			brush.drawLine(xPos, 0, xPos, m_editorHeight);
			brush.drawPixmap(xPos - m_sliceArrow.width() / 2.0f, 0, m_sliceArrow);
		}
	}
}

void SlicerTWaveform::isPlaying(float current, float start, float end)
{
	m_noteCurrent = current;
	m_noteStart = start;
	m_noteEnd = end;
	drawSeeker();
	drawEditor();
	update();
}

void SlicerTWaveform::updateUI()
{
	drawSeekerWaveform();
	drawEditorWaveform();
	drawSeeker();
	drawEditor();
	update();
}

// updates the closest object and changes the cursor respectivly
void SlicerTWaveform::updateClosest(QMouseEvent* me)
{
	float normalizedClickSeeker = static_cast<float>(me->x() - m_seekerHorMargin) / m_seekerWidth;
	float normalizedClickEditor = static_cast<float>(me->x()) / m_editorWidth;

	m_closestObject = UIObjects::Nothing;
	m_closestSlice = -1;

	if (me->y() < m_seekerHeight) // seeker click
	{
		if (std::abs(normalizedClickSeeker - m_seekerStart) < s_distanceForClick) // dragging start
		{
			m_closestObject = UIObjects::SeekerStart;
		}
		else if (std::abs(normalizedClickSeeker - m_seekerEnd) < s_distanceForClick) // dragging end
		{
			m_closestObject = UIObjects::SeekerEnd;
		}
		else if (normalizedClickSeeker > m_seekerStart && normalizedClickSeeker < m_seekerEnd) // dragging middle
		{
			m_closestObject = UIObjects::SeekerMiddle;
		}
	}
	else // editor click
	{
		m_closestSlice = -1;
		float startFrame = m_seekerStart;
		float endFrame = m_seekerEnd;
		// select slice
		for (int i = 0; i < m_slicerTParent->m_slicePoints.size(); i++)
		{
			float sliceIndex = m_slicerTParent->m_slicePoints.at(i);
			float xPos = (sliceIndex - startFrame) / (endFrame - startFrame);

			if (std::abs(xPos - normalizedClickEditor) < s_distanceForClick)
			{
				m_closestObject = UIObjects::SlicePoint;
				m_closestSlice = i;
			}
		}
	}
	updateCursor();
	updateUI();
}

void SlicerTWaveform::updateCursor()
{
	if (m_closestObject == UIObjects::SlicePoint || m_closestObject == UIObjects::SeekerStart
		|| m_closestObject == UIObjects::SeekerEnd)
	{
		setCursor(Qt::SizeHorCursor);
	}
	else if (m_closestObject == UIObjects::SeekerMiddle && m_seekerEnd - m_seekerStart != 1.0f)
	{
		setCursor(Qt::SizeAllCursor);
	}
	else { setCursor(Qt::ArrowCursor); }
}

// handles deletion, reset and middles seeker
void SlicerTWaveform::mousePressEvent(QMouseEvent* me)
{
	/* updateClosest(me); */

	// reset seeker on middle click
	if (me->button() == Qt::MouseButton::MiddleButton)
	{
		m_seekerStart = 0;
		m_seekerEnd = 1;
		m_zoomLevel = 1;
		return;
	}

	if (me->button() == Qt::MouseButton::LeftButton)
	{
		// update seeker middle for correct movement
		m_seekerMiddle = static_cast<float>(me->x() - m_seekerHorMargin) / m_seekerWidth;
	}

	// delete closesd slice to mouse
	if (me->button() == Qt::MouseButton::RightButton && m_slicerTParent->m_slicePoints.size() > 2
		&& m_closestObject == UIObjects::SlicePoint)
	{
		m_slicerTParent->m_slicePoints.erase(m_slicerTParent->m_slicePoints.begin() + m_closestSlice);
	}
	updateClosest(me);
}

// sort slices after moving and remove draggable object
void SlicerTWaveform::mouseReleaseEvent(QMouseEvent* me)
{
	std::sort(m_slicerTParent->m_slicePoints.begin(), m_slicerTParent->m_slicePoints.end());
	updateClosest(me);
}

// this handles dragging and mouse cursor changes
// what is being dragged is determined in mousePressEvent
void SlicerTWaveform::mouseMoveEvent(QMouseEvent* me)
{
	// if no button pressed, update closest and cursor
	if (me->buttons() == Qt::MouseButton::NoButton)
	{
		updateClosest(me);
		return;
	}

	float normalizedClickSeeker = static_cast<float>(me->x() - m_seekerHorMargin) / m_seekerWidth;
	float normalizedClickEditor = static_cast<float>(me->x()) / m_editorWidth;

	float distStart = m_seekerStart - m_seekerMiddle;
	float distEnd = m_seekerEnd - m_seekerMiddle;
	float startFrame = m_seekerStart;
	float endFrame = m_seekerEnd;

	// handle dragging events
	switch (m_closestObject)
	{
	case UIObjects::SeekerStart:
		m_seekerStart = std::clamp(normalizedClickSeeker, 0.0f, m_seekerEnd - s_minSeekerDistance);
		break;

	case UIObjects::SeekerEnd:
		m_seekerEnd = std::clamp(normalizedClickSeeker, m_seekerStart + s_minSeekerDistance, 1.0f);
		break;

	case UIObjects::SeekerMiddle:
		m_seekerMiddle = normalizedClickSeeker;

		if (m_seekerMiddle + distStart >= 0 && m_seekerMiddle + distEnd <= 1)
		{
			m_seekerStart = m_seekerMiddle + distStart;
			m_seekerEnd = m_seekerMiddle + distEnd;
		}
		break;

	case UIObjects::SlicePoint:
		if (m_closestSlice == -1) { break; }
		m_slicerTParent->m_slicePoints.at(m_closestSlice)
			= startFrame + normalizedClickEditor * (endFrame - startFrame);
		m_slicerTParent->m_slicePoints.at(m_closestSlice) = std::clamp(
			m_slicerTParent->m_slicePoints.at(m_closestSlice), 0.0f, 1.0f);
		break;
	case UIObjects::Nothing:
		break;
	}
	// dont update closest, and update seeker waveform
	drawEditorWaveform();
	updateUI();
}

void SlicerTWaveform::mouseDoubleClickEvent(QMouseEvent* me)
{
	if (me->button() != Qt::MouseButton::LeftButton) { return; }

	float normalizedClickEditor = static_cast<float>(me->x()) / m_editorWidth;
	float startFrame = m_seekerStart;
	float endFrame = m_seekerEnd;
	float slicePosition = startFrame + normalizedClickEditor * (endFrame - startFrame);

	m_slicerTParent->m_slicePoints.insert(m_slicerTParent->m_slicePoints.begin(), slicePosition);
	std::sort(m_slicerTParent->m_slicePoints.begin(), m_slicerTParent->m_slicePoints.end());
}

void SlicerTWaveform::wheelEvent(QWheelEvent* _we)
{
	m_zoomLevel += _we->angleDelta().y() / 360.0f * s_zoomSensitivity;
	m_zoomLevel = std::max(0.0f, m_zoomLevel);

	drawEditor();
	update();
}

void SlicerTWaveform::paintEvent(QPaintEvent* pe)
{
	QPainter p(this);
	p.drawPixmap(m_seekerHorMargin, 0, m_seekerWaveform);
	p.drawPixmap(m_seekerHorMargin, 0, m_seeker);
	p.drawPixmap(0, m_seekerHeight + m_middleMargin, m_sliceEditor);
}
} // namespace gui
} // namespace lmms
