/*
 * WaveForm.h - declaration of class WaveForm
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

#ifndef WAVEFORM_H
#define WAVEFORM_H

#include <QApplication>
#include <QFontMetrics>
#include <QInputDialog>
#include <QMouseEvent>
#include <QPainter>

#include "SampleBuffer.h"

namespace lmms {

class SlicerT;

namespace gui {

class WaveForm : public QWidget
{
	Q_OBJECT

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

	int m_seekerHorMargin = 5;
	int m_seekerHeight = 38; // used to calcualte all vertical sizes
	int m_seekerWidth;

	int m_middleMargin = 6;
	int m_editorHeight;
	int m_editorWidth;

	// colors
	static constexpr QColor s_waveformBgColor = QColor(255, 255, 255, 0);
	static constexpr QColor s_waveformColor = QColor(123, 49, 212);

	static constexpr QColor s_playColor = QColor(255, 255, 255, 200);
	static constexpr QColor s_playHighlighColor = QColor(255, 255, 255, 70);

	static constexpr QColor s_sliceColor = QColor(218, 193, 255);
	static constexpr QColor s_selectedSliceColor = QColor(178, 153, 215);

	static constexpr QColor s_seekerColor = QColor(178, 115, 255);
	static constexpr QColor s_seekerHighlightColor = QColor(178, 115, 255, 100);
	static constexpr QColor s_seekerShadowColor = QColor(0, 0, 0, 120);

	// interaction vars
	float m_distanceForClick = 0.03f;
	float m_minSeekerDistance = 0.13f;
	float m_zoomSensitivity = 0.5f;

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

	// editor vars
	float m_zoomLevel = 1.0f;

	// pixmaps
	QPixmap m_sliceArrow;
	QPixmap m_seeker;
	QPixmap m_seekerWaveform; // only stores waveform graphic
	QPixmap m_sliceEditor;

	SampleBuffer& m_currentSample;

	SlicerT* m_slicerTParent;
	std::vector<int>& m_slicePoints;

	void drawEditor();
	void drawSeekerWaveform();
	void drawSeeker();

public slots:
	void updateUI();
	void isPlaying(float current, float start, float end);

public:
	WaveForm(int w, int h, SlicerT* instrument, QWidget* parent);
};
} // namespace gui
} // namespace lmms
#endif // WAVEFORM_H