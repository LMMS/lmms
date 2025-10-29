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

#ifndef LMMS_GUI_SLICERT_WAVEFORM_H
#define LMMS_GUI_SLICERT_WAVEFORM_H

#include <QElapsedTimer>
#include <QWidget>

#include "SampleThumbnail.h"

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
	SlicerTWaveform(int totalWidth, int totalHeight, SlicerT* instrument, QWidget* parent);

	// predefined sizes
	static constexpr int s_seekerHorMargin = 5;
	static constexpr int s_seekerVerMargin = 6;
	static constexpr int s_middleMargin = 6;
	static constexpr int s_arrowHeight = 5;

	// interaction behavior values
	static constexpr float s_distanceForClick = 0.02f;
	static constexpr float s_minSeekerDistance = 0.13f;
	static constexpr float s_zoomSensitivity = 0.5f;
	static constexpr int s_minMilisPassed = 10;

	enum class UIObjects
	{
		Nothing,
		SeekerStart,
		SeekerEnd,
		SeekerMiddle,
		SlicePoint,
	};

protected:
	void mousePressEvent(QMouseEvent* me) override;
	void mouseReleaseEvent(QMouseEvent* me) override;
	void mouseMoveEvent(QMouseEvent* me) override;
	void mouseDoubleClickEvent(QMouseEvent* me) override;
	void wheelEvent(QWheelEvent* we) override;

	void paintEvent(QPaintEvent* pe) override;
	void resizeEvent(QResizeEvent* event) override;

private:
	int m_width;
	int m_height;

	int m_seekerHeight; // used to calcualte all vertical sizes
	int m_seekerWidth;
	int m_editorHeight;
	int m_editorWidth;

	UIObjects m_closestObject;
	int m_closestSlice = -1;

	float m_seekerStart = 0;
	float m_seekerEnd = 1;
	float m_seekerMiddle = 0.5f;

	float m_noteCurrent;
	float m_noteStart;
	float m_noteEnd;

	float m_zoomLevel = 1.0f;

	QPixmap m_sliceArrow;
	QPixmap m_seeker;
	QPixmap m_seekerWaveform;
	QPixmap m_editorWaveform;
	QPixmap m_sliceEditor;
	QPixmap m_emptySampleIcon;
	
	SampleThumbnail m_sampleThumbnail;

	SlicerT* m_slicerTParent;

	QElapsedTimer m_updateTimer;
	void drawSeekerWaveform();
	void drawSeeker();
	void drawEditorWaveform();
	void drawEditor();

	void updateClosest(QMouseEvent* me);
	void updateCursor();
};
} // namespace gui
} // namespace lmms
#endif // LMMS_GUI_SLICERT_WAVEFORM_H
