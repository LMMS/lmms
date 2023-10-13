/*
 * SlicerTWaveform.h - declaration of class SlicerTWaveform
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

#ifndef LMMS_SlicerT_Waveform_H
#define LMMS_SlicerT_Waveform_H

#include <QApplication>
#include <QFontMetrics>
#include <QInputDialog>
#include <QMouseEvent>
#include <QPainter>

#include "SampleBuffer.h"

namespace lmms {

class SlicerT;

namespace gui {

class SlicerTWaveform : public QWidget
{
	Q_OBJECT

public slots:
	void updateUI();
	void isPlaying(float current, float start, float end);

public:
	SlicerTWaveform(int w, int h, SlicerT* instrument, QWidget* parent);

	// predefined sizes
	static constexpr int m_seekerHorMargin = 5;
	static constexpr int m_seekerHeight = 38; // used to calcualte all vertical sizes
	static constexpr int m_middleMargin = 6;

	// colors
	static constexpr QColor s_SlicerTWaveformBgColor = QColor(255, 255, 255, 0);
	static constexpr QColor s_SlicerTWaveformColor = QColor(123, 49, 212);

	static constexpr QColor s_playColor = QColor(255, 255, 255, 200);
	static constexpr QColor s_playHighlighColor = QColor(255, 255, 255, 70);

	static constexpr QColor s_sliceColor = QColor(218, 193, 255);
	static constexpr QColor s_selectedSliceColor = QColor(178, 153, 215);

	static constexpr QColor s_seekerColor = QColor(178, 115, 255);
	static constexpr QColor s_seekerHighlightColor = QColor(178, 115, 255, 100);
	static constexpr QColor s_seekerShadowColor = QColor(0, 0, 0, 120);

	// interaction vars
	static constexpr float m_distanceForClick = 0.03f;
	static constexpr float m_minSeekerDistance = 0.13f;
	static constexpr float m_zoomSensitivity = 0.5f;

	enum class DraggingTypes
	{
		Nothing,
		SeekerStart,
		SeekerEnd,
		SeekerMiddle,
		SlicePoint,
	};

protected:
	virtual void mousePressEvent(QMouseEvent* me);
	virtual void mouseReleaseEvent(QMouseEvent* me);
	virtual void mouseMoveEvent(QMouseEvent* me);
	virtual void mouseDoubleClickEvent(QMouseEvent* me);
	virtual void wheelEvent(QWheelEvent* _we);

	virtual void paintEvent(QPaintEvent* pe);

private:
	// sizes
	int m_width;
	int m_height;

	// later calculated
	int m_seekerWidth;
	int m_editorHeight;
	int m_editorWidth;

	// dragging vars
	DraggingTypes m_currentlyDragging;

	// seeker vars
	float m_seekerStart = 0;
	float m_seekerEnd = 1;
	float m_seekerMiddle = 0.5f;
	int m_sliceSelected = 0;

	// playback highlight vars
	float m_noteCurrent;
	float m_noteStart;
	float m_noteEnd;

	// editor vars
	float m_zoomLevel = 1.0f;

	// pixmaps
	QPixmap m_sliceArrow;
	QPixmap m_seeker;
	QPixmap m_seekerSlicerTWaveform; // only stores SlicerTWaveform graphic
	QPixmap m_sliceEditor;

	SampleBuffer& m_currentSample;

	SlicerT* m_slicerTParent;
	std::vector<int>& m_slicePoints;

	void drawEditor();
	void drawSeekerSlicerTWaveform();
	void drawSeeker();
};
} // namespace gui
} // namespace lmms
#endif // LMMS_SlicerT_Waveform_H