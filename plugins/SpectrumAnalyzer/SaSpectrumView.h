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


#include <string>
#include <utility>
#include <vector>
#include <QPainterPath>
#include <QWidget>


namespace lmms
{


class SaControls;
class SaProcessor;

namespace gui
{


//! Widget that displays a spectrum curve and frequency / amplitude grid
class SaSpectrumView : public QWidget
{
	Q_OBJECT
public:
	explicit SaSpectrumView(SaControls *controls, SaProcessor *processor, QWidget *_parent = 0);
	~SaSpectrumView() override = default;

	QSize sizeHint() const override {return QSize(400, 200);}

protected:
	void paintEvent(QPaintEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;

private slots:
	void periodicUpdate();

private:
	const SaControls *m_controls;
	SaProcessor *m_processor;

	// grid labels (position, label) and methods to generate them
	std::vector<std::pair<int, std::string>> m_logFreqTics;		// 10-20-50... Hz
	std::vector<std::pair<int, std::string>> m_linearFreqTics;	// 2k-4k-6k... Hz
	std::vector<std::pair<float, std::string>> m_logAmpTics;	// dB
	std::vector<std::pair<float, std::string>> m_linearAmpTics;	// 0..1

	std::vector<std::pair<int, std::string>> makeLogFreqTics(int low, int high);
	std::vector<std::pair<int, std::string>> makeLinearFreqTics(int low, int high);
	std::vector<std::pair<float, std::string>> makeLogAmpTics(int low, int high);
	std::vector<std::pair<float, std::string>> makeLinearAmpTics(int low, int high);

	// currently selected ranges (see SaControls.h for enum definitions)
	int m_freqRangeIndex;
	int m_ampRangeIndex;

	// draw the grid and all labels based on selected ranges
	void drawGrid(QPainter &painter);

	// local buffers for frequency bin values and a method to update them
	// (mainly needed for averaging and to keep track of peak values)
	std::vector<float> m_displayBufferL;
	std::vector<float> m_displayBufferR;
	std::vector<float> m_peakBufferL;
	std::vector<float> m_peakBufferR;
	void updateBuffers(const float *spectrum, float *displayBuffer, float *peakBuffer);

	// final paths to be drawn by QPainter and methods to build them
	QPainterPath m_pathL;
	QPainterPath m_pathR;
	QPainterPath m_pathPeakL;
	QPainterPath m_pathPeakR;
	void refreshPaths();
	QPainterPath makePath(std::vector<float> &displayBuffer, float resolution);

	// helper variables for path drawing
	float m_decaySum;		// indicates if there is anything left to draw
	bool m_freezeRequest;	// new reference should be acquired
	bool m_frozen;			// a reference is currently stored in the peakBuffer

	// top level: refresh buffers, make paths and draw the spectrum
	void drawSpectrum(QPainter &painter);

	// current cursor location and a method to draw it
	QPointF m_cursor;
	void drawCursor(QPainter &painter);

	// wrappers for most used SaProcessor conversion helpers
	// (to make local code more readable)
	float binToFreq(unsigned int bin_index);
	float freqToXPixel(float frequency, unsigned int width);
	float ampToYPixel(float amplitude, unsigned int height);

	// current boundaries for drawing
	unsigned int m_displayTop;
	unsigned int m_displayBottom;
	unsigned int m_displayLeft;
	unsigned int m_displayRight;
	unsigned int m_displayWidth;

	// cached frequency bin → x position conversion for better performance
	std::vector<float> m_cachedBinToX;
	float m_cachedRangeMin;
	float m_cachedRangeMax;
	bool m_cachedLogX;
	unsigned int m_cachedDisplayWidth;
	unsigned int m_cachedBinCount;
	unsigned int m_cachedSampleRate;

	#ifdef SA_DEBUG
		float m_execution_avg;
		float m_refresh_avg;
		float m_path_avg;
		float m_draw_avg;
	#endif
};


} // namespace gui

} // namespace lmms

#endif // SASPECTRUMVIEW_H

