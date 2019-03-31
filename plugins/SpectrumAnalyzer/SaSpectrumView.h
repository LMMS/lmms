/* SaSpectrumView.h - declaration of SaSpectrumView class.
 *
 * Copyright (c) 2019 Martin Pavelek <he29/dot/HS/at/gmail/dot/com>
 *
 * Based partially on Eq plugin code,
 * Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
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

#ifndef SASPECTRUMVIEW_H
#define SASPECTRUMVIEW_H

#include <algorithm>
#include <string>
#include <utility>
#include <QMouseEvent>
#include <QPainter>
#include <QString>
#include <QWidget>

#include "fft_helpers.h"
#include "lmms_basics.h"
#include "lmms_math.h"
#include "SaControls.h"
#include "SaProcessor.h"


class SaSpectrumView : public QWidget {
	Q_OBJECT
public:
	explicit SaSpectrumView(SaControls *controls, SaProcessor *processor, QWidget *_parent = 0);
	virtual ~SaSpectrumView(){}

	std::vector<std::pair<int, std::string>> makeLogTics(int low, int high);
	std::vector<std::pair<int, std::string>> makeLinearTics(int low, int high);
	std::vector<std::pair<float, std::string>> makeDBTics(int low, int high);
	std::vector<std::pair<float, std::string>> makeAmpTics(int low, int high);

protected:
	virtual void paintEvent(QPaintEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);

private slots:
	void periodicUpdate();

private:
	SaControls *m_controls;
	SaProcessor *m_processor;

	bool m_periodicUpdate;

	QColor m_colorL;
	QColor m_colorR;
	QColor m_colorMono;
	QColor m_colorBG;
	QColor m_colorGrid;
	QColor m_colorLabels;

	QPainterPath m_pathL;
	QPainterPath m_pathR;
	QPainterPath m_pathPeakL;
	QPainterPath m_pathPeakR;

	std::vector<std::pair<int, std::string>> m_logFreqTics;		// 10-20-50... Hz
	std::vector<std::pair<int, std::string>> m_linearFreqTics;	// 2k-4k-6k... Hz
	std::vector<std::pair<float, std::string>> m_logAmpTics;	// dB
	std::vector<std::pair<float, std::string>> m_linearAmpTics;	// 0..1

	std::vector<float> m_displayBufferL;
	std::vector<float> m_displayBufferR;
	std::vector<float> m_peakBufferL;
	std::vector<float> m_peakBufferR;

	float m_decaySum;
	bool m_freezeRequest;
	bool m_frozen;

	QPoint m_cursor;
	int m_freqRangeIndex;
	int m_ampRangeIndex;

	void refreshPaths();
	QPainterPath makePath(std::vector<float> &displayBuffer, float resolution);
	void drawGrid(QPainter &painter);
	void drawCursor(QPainter &painter);

	float freqToXPixel(float frequency, int width);
	float xPixelToFreq(float x, int width);
	float ampToYPixel(float amplitude, int height);
	float binToFreq(int index);

	const float m_smoothFactor = 0.15;
	const float m_peakFallFactor = 0.992;

	int m_displayTop;
	int m_displayBottom;
	int m_displayLeft;
	int m_displayRight;
	int m_displayWidth;
};
#endif // SASPECTRUMVIEW_H
