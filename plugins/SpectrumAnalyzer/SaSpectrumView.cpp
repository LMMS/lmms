/* SaSpectrumView.cpp - implementation of SaSpectrumView class.
 *
 * Copyright (c) 2019 Martin Pavelek <he29/dot/HS/at/gmail/dot/com>
 *
 * Based partially on Eq plugin code,
 * Copyright (c) 2014-2017, David French <dave/dot/french3/at/googlemail/dot/com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include "SaSpectrumView.h"

#include <cmath>
#include <QMouseEvent>
#include <QMutexLocker>
#include <QPainter>
#include <QPainterPath>
#include <QString>

#include "fft_helpers.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "SaControls.h"
#include "SaProcessor.h"
#include "lmms_math.h"

#ifdef SA_DEBUG
	#include <chrono>
#endif

namespace lmms::gui
{


SaSpectrumView::SaSpectrumView(SaControls *controls, SaProcessor *processor, QWidget *_parent) :
	QWidget(_parent),
	m_controls(controls),
	m_processor(processor),
	m_freezeRequest(false),
	m_frozen(false),
	m_cachedRangeMin(-1),
	m_cachedRangeMax(-1),
	m_cachedLogX(true),
	m_cachedDisplayWidth(0),
	m_cachedBinCount(0),
	m_cachedSampleRate(0)
{
	setMinimumSize(360, 170);
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

	connect(getGUI()->mainWindow(), SIGNAL(periodicUpdate()), this, SLOT(periodicUpdate()));

	m_displayBufferL.resize(m_processor->binCount(), 0);
	m_displayBufferR.resize(m_processor->binCount(), 0);
	m_peakBufferL.resize(m_processor->binCount(), 0);
	m_peakBufferR.resize(m_processor->binCount(), 0);

	m_freqRangeIndex = m_controls->m_freqRangeModel.value();
	m_ampRangeIndex = m_controls->m_ampRangeModel.value();

	m_logFreqTics = makeLogFreqTics(m_processor->getFreqRangeMin(), m_processor->getFreqRangeMax());
	m_linearFreqTics = makeLinearFreqTics(m_processor->getFreqRangeMin(), m_processor->getFreqRangeMax());
	m_logAmpTics = makeLogAmpTics(m_processor->getAmpRangeMin(), m_processor->getAmpRangeMax());
	m_linearAmpTics = makeLinearAmpTics(m_processor->getAmpRangeMin(), m_processor->getAmpRangeMax());

	m_cursor = QPointF(0, 0);

	// Initialize the size of bin → pixel X position LUT to the maximum allowed number of bins + 1.
	m_cachedBinToX.resize(FFT_BLOCK_SIZES.back() / 2 + 2);

	#ifdef SA_DEBUG
		m_execution_avg = m_path_avg = m_draw_avg = 0;
	#endif
}


// Compose and draw all the content; periodically called by Qt.
// NOTE: Performance sensitive! If the drawing takes too long, it will drag
// the FPS down for the entire program! Use SA_DEBUG to display timings.
void SaSpectrumView::paintEvent(QPaintEvent *event)
{
	#ifdef SA_DEBUG
		int total_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	#endif

	// 0) Constants and init
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);

	// drawing and path-making are split into multiple methods for clarity;
	// display boundaries are updated here and shared as member variables
	m_displayTop = 1;
	m_displayBottom = height() -20;
	m_displayLeft = 26;
	m_displayRight = width() -26;
	m_displayWidth = m_displayRight - m_displayLeft;

	// recompute range labels if needed
	if (m_freqRangeIndex != m_controls->m_freqRangeModel.value())
	{
		m_logFreqTics = makeLogFreqTics(m_processor->getFreqRangeMin(), m_processor->getFreqRangeMax());
		m_linearFreqTics = makeLinearFreqTics(m_processor->getFreqRangeMin(true), m_processor->getFreqRangeMax());
		m_freqRangeIndex = m_controls->m_freqRangeModel.value();
	}
	if (m_ampRangeIndex != m_controls->m_ampRangeModel.value())
	{
		m_logAmpTics = makeLogAmpTics(m_processor->getAmpRangeMin(), m_processor->getAmpRangeMax());
		m_linearAmpTics = makeLinearAmpTics(m_processor->getAmpRangeMin(true), m_processor->getAmpRangeMax());
		m_ampRangeIndex = m_controls->m_ampRangeModel.value();
	}

	// generate freeze request or clear "frozen" status based on freeze button
	if (!m_frozen && m_controls->m_refFreezeModel.value())
	{
		m_freezeRequest = true;
	}
	else if (!m_controls->m_refFreezeModel.value())
	{
		m_frozen = false;
	}

	// 1) Background, grid and labels
	drawGrid(painter);

	// 2) Spectrum display
	drawSpectrum(painter);

	// 3) Overlays
	// draw cursor (if it is within bounds)
	drawCursor(painter);

	// always draw the display outline
	painter.setPen(QPen(m_controls->m_colorGrid, 2, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	painter.drawRoundedRect(m_displayLeft, 1,
							m_displayWidth, m_displayBottom,
							2.0, 2.0);

	#ifdef SA_DEBUG
		// display performance measurements if enabled
		total_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() - total_time;
		m_execution_avg = 0.95 * m_execution_avg + 0.05 * total_time / 1000000.0;
		painter.setPen(QPen(m_controls->m_colorLabels, 1,
							Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
		painter.drawText(m_displayRight -150, 10, 130, 16, Qt::AlignLeft,
						 QString("Exec avg.: ").append(std::to_string(m_execution_avg).substr(0, 5).c_str()).append(" ms"));
		painter.drawText(m_displayRight -150, 30, 130, 16, Qt::AlignLeft,
						 QString("Buff. upd. avg: ").append(std::to_string(m_refresh_avg).substr(0, 5).c_str()).append(" ms"));
		painter.drawText(m_displayRight -150, 50, 130, 16, Qt::AlignLeft,
						 QString("Path build avg: ").append(std::to_string(m_path_avg).substr(0, 5).c_str()).append(" ms"));
		painter.drawText(m_displayRight -150, 70, 130, 16, Qt::AlignLeft,
						 QString("Path draw avg: ").append(std::to_string(m_draw_avg).substr(0, 5).c_str()).append(" ms"));

	#endif
}


// Refresh data and draw the spectrum.
void SaSpectrumView::drawSpectrum(QPainter &painter)
{
	#ifdef SA_DEBUG
		int draw_time = 0;
	#endif

	// draw the graph only if there is any input, averaging residue or peaks
	if (m_decaySum > 0 || m_processor->spectrumNotEmpty())
	{
		// update data buffers and reconstruct paths
		refreshPaths();

		// draw stored paths
		#ifdef SA_DEBUG
			draw_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
		#endif
		// in case stereo is disabled, mono data are stored in left channel structures
		if (m_controls->m_stereoModel.value())
		{
			painter.fillPath(m_pathR, QBrush(m_controls->m_colorR));
			painter.fillPath(m_pathL, QBrush(m_controls->m_colorL));
		}
		else
		{
			painter.fillPath(m_pathL, QBrush(m_controls->m_colorMono));
		}
		// draw the peakBuffer only if peak hold or reference freeze is active
		if (m_controls->m_peakHoldModel.value() || m_controls->m_refFreezeModel.value())
		{
			if (m_controls->m_stereoModel.value())
			{
				painter.setPen(QPen(m_controls->m_colorR, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
				painter.drawPath(m_pathPeakR);
				painter.setPen(QPen(m_controls->m_colorL, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
				painter.drawPath(m_pathPeakL);
			}
			else
			{
				painter.setPen(QPen(m_controls->m_colorL, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
				painter.drawPath(m_pathPeakL);
			}
		}
		#ifdef SA_DEBUG
			draw_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() - draw_time;
		#endif
	}

	#ifdef SA_DEBUG
		// save performance measurement result
		m_draw_avg = 0.95 * m_draw_avg + 0.05 * draw_time / 1000000.0;
	#endif
}


// Read newest FFT results from SaProcessor, update local display buffers
// and build QPainter paths.
void SaSpectrumView::refreshPaths()
{
	// Reallocation lock is required for the entire function, to keep display
	// buffer size consistent with block size.
	QMutexLocker reloc_lock(&m_processor->m_reallocationAccess);

	// check if bin count changed and reallocate display buffers accordingly
	if (m_processor->binCount() != m_displayBufferL.size())
	{
		m_displayBufferL.clear();
		m_displayBufferR.clear();
		m_peakBufferL.clear();
		m_peakBufferR.clear();
		m_displayBufferL.resize(m_processor->binCount(), 0);
		m_displayBufferR.resize(m_processor->binCount(), 0);
		m_peakBufferL.resize(m_processor->binCount(), 0);
		m_peakBufferR.resize(m_processor->binCount(), 0);
	}

	// update display buffers for left and right channel
	#ifdef SA_DEBUG
		int refresh_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	#endif
	m_decaySum = 0;
	updateBuffers(m_processor->getSpectrumL(), m_displayBufferL.data(), m_peakBufferL.data());
	updateBuffers(m_processor->getSpectrumR(), m_displayBufferR.data(), m_peakBufferR.data());
	#ifdef SA_DEBUG
		refresh_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() - refresh_time;
	#endif

	// if there was a freeze request, it was taken care of during the update
	if (m_controls->m_refFreezeModel.value() && m_freezeRequest)
	{
		m_freezeRequest = false;
		m_frozen = true;
	}

	#ifdef SA_DEBUG
		int path_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	#endif
	// Use updated display buffers to prepare new paths for QPainter.
	// This is the second slowest action (first is the subsequent drawing); use
	// the resolution parameter to balance display quality and performance.
	m_pathL = makePath(m_displayBufferL, m_controls->m_spectrumResolutionModel.value());
	if (m_controls->m_stereoModel.value())
	{
		m_pathR = makePath(m_displayBufferR, m_controls->m_spectrumResolutionModel.value());
	}
	if (m_controls->m_peakHoldModel.value() || m_controls->m_refFreezeModel.value())
	{
		m_pathPeakL = makePath(m_peakBufferL, m_controls->m_envelopeResolutionModel.value());
		if (m_controls->m_stereoModel.value())
		{
			m_pathPeakR = makePath(m_peakBufferR, m_controls->m_envelopeResolutionModel.value());
		}
	}
	#ifdef SA_DEBUG
		path_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() - path_time;
	#endif

	#ifdef SA_DEBUG
		// save performance measurement results
		m_refresh_avg = 0.95 * m_refresh_avg + 0.05 * refresh_time / 1000000.0;
		m_path_avg = .95f * m_path_avg + .05f * path_time / 1000000.f;
	#endif
}


// Update display buffers: add new data, update average and peaks / reference.
// Output the sum of all displayed values -- draw only if it is non-zero.
// NOTE: The calling function is responsible for acquiring SaProcessor
// reallocation access lock! Data access lock is not needed: the final result
// buffer is updated very quickly and the worst case is that one frame will be
// part new, part old. At reasonable frame rate, such difference is invisible..
void SaSpectrumView::updateBuffers(const float *spectrum, float *displayBuffer, float *peakBuffer)
{
	for (auto n = std::size_t{0}; n < m_processor->binCount(); n++)
	{
		// Update the exponential average if enabled, or simply copy the value.
		if (!m_controls->m_pauseModel.value())
		{
			if (m_controls->m_smoothModel.value())
			{
				const float smoothFactor = m_controls->m_averagingWeightModel.value();
				displayBuffer[n] = spectrum[n] * smoothFactor + displayBuffer[n] * (1 - smoothFactor);
			}
			else
			{
				displayBuffer[n] = spectrum[n];
			}
		}
		// Update peak-hold and reference freeze data (using a shared curve).
		// Peak hold and freeze can be combined: decay only if not frozen.
		// Ref. freeze operates on the (possibly averaged) display buffer.
		if (m_controls->m_refFreezeModel.value() && m_freezeRequest)
		{
			peakBuffer[n] = displayBuffer[n];
		}
		else if (m_controls->m_peakHoldModel.value() && !m_controls->m_pauseModel.value())
		{
			if (spectrum[n] > peakBuffer[n])
			{
				peakBuffer[n] = spectrum[n];
			}
			else if (!m_controls->m_refFreezeModel.value())
			{
				peakBuffer[n] = peakBuffer[n] * m_controls->m_peakDecayFactorModel.value();
			}
		}
		else if (!m_controls->m_refFreezeModel.value() && !m_controls->m_peakHoldModel.value())
		{
			peakBuffer[n] = 0;
		}
		// take note if there was actually anything to display
		m_decaySum += displayBuffer[n] + peakBuffer[n];
	}
}


// Use display buffer to build a path that can be drawn or filled by QPainter.
// Resolution controls the performance / quality tradeoff; the value specifies
// number of points in x axis per device pixel. Values over 1.0 still
// contribute to quality and accuracy thanks to anti-aliasing.
QPainterPath SaSpectrumView::makePath(std::vector<float> &displayBuffer, float resolution = 1.0)
{
	// convert resolution to number of path points per logical pixel
	float pixel_limit = resolution * window()->devicePixelRatio();

	QPainterPath path;
	path.moveTo(m_displayLeft, m_displayBottom);

	// Translate frequency bins to path points.
	// Display is flipped: y values grow towards zero, initial max is bottom.
	// Bins falling to interval [x_start, x_next) contribute to a single point.
	float max = m_displayBottom;
	float x_start = -1;		// lower bound of currently constructed point

	// Speed up bin → x position translation by building a LUT cache.
	// Update the cache only when range or display width are changed.
	float rangeMin = m_processor->getFreqRangeMin(m_controls->m_logXModel.value());
	float rangeMax = m_processor->getFreqRangeMax();
	if (rangeMin != m_cachedRangeMin || rangeMax != m_cachedRangeMax || m_displayWidth != m_cachedDisplayWidth ||
		m_controls->m_logXModel.value() != m_cachedLogX || m_processor->binCount() + 1 != m_cachedBinCount ||
		m_processor->getSampleRate() != m_cachedSampleRate)
	{
		m_cachedRangeMin = rangeMin;
		m_cachedRangeMax = rangeMax;
		m_cachedDisplayWidth = m_displayWidth;
		m_cachedLogX = m_controls->m_logXModel.value();
		m_cachedBinCount = m_processor->binCount() + 1;
		m_cachedSampleRate = m_processor->getSampleRate();
		for (unsigned int n = 0; n < m_cachedBinCount; n++)
		{
			m_cachedBinToX[n] = freqToXPixel(binToFreq(n), m_displayWidth);
		}
	}

	for (unsigned int n = 0; n < m_processor->binCount(); n++)
	{
		float x = m_cachedBinToX[n];
		float x_next = m_cachedBinToX[n + 1];
		float y = ampToYPixel(displayBuffer[n], m_displayBottom);

		// consider making a point only if x falls within display bounds
		if (0 < x && x < m_displayWidth)
		{
			if (x_start == -1)
			{
				x_start = x;
				// the first displayed bin is stretched to the left edge to prevent
				// creating a misleading slope leading to zero (at log. scale)
				path.lineTo(m_displayLeft, y + m_displayTop);
			}
			// Opt.: QPainter is very slow -- draw at most [pixel_limit] points
			// per logical pixel. As opposed to limiting the bin count, this
			// allows high resolution display if user resizes the analyzer.
			// Look at bins that share the pixel and use the highest value:
			max = y < max ? y : max;
			// And make the final point in the middle of current interval.
			if ((int)(x * pixel_limit) != (int)(x_next * pixel_limit))
			{
				x = (x + x_start) / 2;
				path.lineTo(x + m_displayLeft, max + m_displayTop);
				max = m_displayBottom;
				x_start = x_next;
			}
		}
		else
		{
			// stop processing after a bin falls outside right edge
			// and align it to the edge to prevent a gap
			if (n > 0 && x > 0)
			{
				path.lineTo(m_displayRight,	y + m_displayTop);
				break;
			}
		}
	}
	path.lineTo(m_displayRight, m_displayBottom);
	path.closeSubpath();
	return path;
}


// Draw background, grid and associated frequency and amplitude labels.
void SaSpectrumView::drawGrid(QPainter &painter)
{
	std::vector<std::pair<int, std::string>> *freqTics = nullptr;
	std::vector<std::pair<float, std::string>> *ampTics = nullptr;
	float pos = 0;
	float label_width = 24;
	float label_height = 15;
	float margin = 5;

	// always draw the background
	painter.fillRect(m_displayLeft, m_displayTop,
					 m_displayWidth, m_displayBottom,
					 m_controls->m_colorBG);

	// select logarithmic or linear frequency grid and draw it
	if (m_controls->m_logXModel.value())
	{
		freqTics = &m_logFreqTics;
	}
	else
	{
		freqTics = &m_linearFreqTics;
	}
	// draw frequency grid (line.first is display position)
	painter.setPen(QPen(m_controls->m_colorGrid, 1,	Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	for (auto &line: *freqTics)
	{
		painter.drawLine(m_displayLeft + freqToXPixel(line.first, m_displayWidth),
						 2,
						 m_displayLeft + freqToXPixel(line.first, m_displayWidth),
						 m_displayBottom);
	}
	// print frequency labels (line.second is label)
	painter.setPen(QPen(m_controls->m_colorLabels, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	for (auto & line: *freqTics)
	{
		pos = m_displayLeft + freqToXPixel(line.first, m_displayWidth);
		// align first and last label to the edge if needed, otherwise center them
		if (line == freqTics->front() && pos - label_width / 2 < m_displayLeft)
		{
			painter.drawText(m_displayLeft, m_displayBottom + margin,
							 label_width, label_height, Qt::AlignLeft | Qt::TextDontClip,
							 QString(line.second.c_str()));
		}
		else if (line == freqTics->back() && pos + label_width / 2 > m_displayRight)
		{
			painter.drawText(m_displayRight - label_width, m_displayBottom + margin,
							 label_width, label_height, Qt::AlignRight | Qt::TextDontClip,
							 QString(line.second.c_str()));
		}
		else
		{
			painter.drawText(pos - label_width / 2, m_displayBottom + margin,
							 label_width, label_height, Qt::AlignHCenter | Qt::TextDontClip,
							 QString(line.second.c_str()));
		}
	}

	margin = 2;
	// select logarithmic or linear amplitude grid and draw it
	if (m_controls->m_logYModel.value())
	{
		ampTics = &m_logAmpTics;
	}
	else
	{
		ampTics = &m_linearAmpTics;
	}
	// draw amplitude grid
	painter.setPen(QPen(m_controls->m_colorGrid, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	for (auto & line: *ampTics)
	{
		painter.drawLine(m_displayLeft + 1,
						 ampToYPixel(line.first, m_displayBottom),
						 m_displayRight - 1,
						 ampToYPixel(line.first, m_displayBottom));
	}
	// print amplitude labels
	painter.setPen(QPen(m_controls->m_colorLabels, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	bool stereo = m_controls->m_stereoModel.value();
	for (auto & line: *ampTics)
	{
		pos = ampToYPixel(line.first, m_displayBottom);
		// align first and last labels to edge if needed, otherwise center them
		if (line == ampTics->back() && pos < 8)
		{
			if (stereo)
			{
				painter.setPen(QPen(m_controls->m_colorL.lighter(), 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
			}
			painter.drawText(m_displayLeft - label_width - margin, m_displayTop - 2,
							 label_width, label_height, Qt::AlignRight | Qt::AlignTop | Qt::TextDontClip,
							 QString(line.second.c_str()));
			if (stereo)
			{
				painter.setPen(QPen(m_controls->m_colorR.lighter(), 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
			}
			painter.drawText(m_displayRight + margin, m_displayTop - 2,
							 label_width, label_height, Qt::AlignLeft | Qt::AlignTop | Qt::TextDontClip,
							 QString(line.second.c_str()));
		}
		else if (line == ampTics->front() && pos > m_displayBottom - label_height)
		{
			if (stereo)
			{
				painter.setPen(QPen(m_controls->m_colorL.lighter(), 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
			}
			painter.drawText(m_displayLeft - label_width - margin, m_displayBottom - label_height + 2,
							 label_width, label_height, Qt::AlignRight | Qt::AlignBottom | Qt::TextDontClip,
							 QString(line.second.c_str()));
			if (stereo)
			{
				painter.setPen(QPen(m_controls->m_colorR.lighter(), 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
			}
			painter.drawText(m_displayRight + margin, m_displayBottom - label_height + 2,
							 label_width, label_height, Qt::AlignLeft | Qt::AlignBottom | Qt::TextDontClip,
							 QString(line.second.c_str()));
		}
		else
		{
			if (stereo)
			{
				painter.setPen(QPen(m_controls->m_colorL.lighter(), 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
			}
			painter.drawText(m_displayLeft - label_width - margin, pos - label_height / 2,
							 label_width, label_height, Qt::AlignRight | Qt::AlignVCenter | Qt::TextDontClip,
							 QString(line.second.c_str()));
			if (stereo)
			{
				painter.setPen(QPen(m_controls->m_colorR.lighter(), 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
			}
			painter.drawText(m_displayRight + margin, pos - label_height / 2,
							 label_width, label_height, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextDontClip,
							 QString(line.second.c_str()));
		}
	}
}


// Draw cursor and its coordinates if it is within display bounds.
void SaSpectrumView::drawCursor(QPainter &painter)
{
	if (	m_cursor.x() >= m_displayLeft
		&&	m_cursor.x() <= m_displayRight
		&&	m_cursor.y() >= m_displayTop
		&&	m_cursor.y() <= m_displayBottom)
	{
		// cursor lines
		painter.setPen(QPen(m_controls->m_colorGrid.lighter(), 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
		painter.drawLine(QPointF(m_cursor.x(), m_displayTop), QPointF(m_cursor.x(), m_displayBottom));
		painter.drawLine(QPointF(m_displayLeft, m_cursor.y()), QPointF(m_displayRight, m_cursor.y()));

		// coordinates: background box
		QFontMetrics fontMetrics = painter.fontMetrics();
		unsigned int const box_left = 5;
		unsigned int const box_top = 5;
		unsigned int const box_margin = 3;
		unsigned int const box_height = 2*(fontMetrics.size(Qt::TextSingleLine, "0 HzdBFS").height() + box_margin);
		unsigned int const box_width = fontMetrics.size(Qt::TextSingleLine, "-99.9 dBFS").width() + 2*box_margin;
		painter.setPen(QPen(m_controls->m_colorLabels.darker(), 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
		painter.fillRect(m_displayLeft + box_left, m_displayTop + box_top,
						 box_width, box_height, QColor(0, 0, 0, 64));

		// coordinates: text
		painter.setPen(QPen(m_controls->m_colorLabels, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
		QString tmps;

		// frequency
		int xFreq = (int)m_processor->xPixelToFreq(m_cursor.x() - m_displayLeft, m_displayWidth);
		tmps = QString("%1 Hz").arg(xFreq);
		painter.drawText(m_displayLeft + box_left + box_margin,
						 m_displayTop + box_top + box_margin,
						 box_width, box_height / 2, Qt::AlignLeft, tmps);

		// amplitude
		float yAmp = m_processor->yPixelToAmp(m_cursor.y(), m_displayBottom);
		if (m_controls->m_logYModel.value())
		{
			tmps = QString(std::to_string(yAmp).substr(0, 5).c_str()).append(" dBFS");
		}
		else
		{
			// add 0.0005 to get proper rounding to 3 decimal places
			tmps = QString(std::to_string(0.0005f + yAmp).substr(0, 5).c_str());
		}
		painter.drawText(m_displayLeft + box_left + box_margin,
						 m_displayTop + box_top + box_height / 2,
						 box_width, box_height / 2, Qt::AlignLeft, tmps);
	}
}


// Wrappers for most used SaProcessor helpers (to make local code more compact).
float SaSpectrumView::binToFreq(unsigned int bin_index)
{
	return m_processor->binToFreq(bin_index);
}


float SaSpectrumView::freqToXPixel(float frequency, unsigned int width)
{
	return m_processor->freqToXPixel(frequency, width);
}


float SaSpectrumView::ampToYPixel(float amplitude, unsigned int height)
{
	return m_processor->ampToYPixel(amplitude, height);
}


// Generate labels suitable for logarithmic frequency scale.
// Low / high limits are in Hz. Lowest possible label is 10 Hz.
std::vector<std::pair<int, std::string>> SaSpectrumView::makeLogFreqTics(int low, int high)
{
	std::vector<std::pair<int, std::string>> result;
	auto a = std::array{10, 20, 50};		// sparse series multipliers
	auto b = std::array{14, 30, 70};		// additional (denser) series

	// generate main steps (powers of 10); use the series to specify smaller steps
	for (int i = 1; i <= high; i *= 10)
	{
		for (int j = 0; j < 3; j++)
		{
			// insert a label from sparse series if it falls within bounds
			if (i * a[j] >= low && i * a[j] <= high)
			{
				if (i * a[j] < 1000)
				{
					result.emplace_back(i * a[j], std::to_string(i * a[j]));
				}
				else
				{
					result.emplace_back(i * a[j], std::to_string(i * a[j] / 1000) + "k");
				}
			}
			// also insert denser series if high and low values are close
			if ((std::log10(high) - std::log10(low) < 2) && (i * b[j] >= low && i * b[j] <= high))
			{
				if (i * b[j] < 1500)
				{
					result.emplace_back(i * b[j], std::to_string(i * b[j]));
				}
				else
				{
					result.emplace_back(i * b[j], std::to_string(i * b[j] / 1000) + "k");
				}
			}
		}
	}
	return result;
}


// Generate labels suitable for linear frequency scale.
// Low / high limits are in Hz.
std::vector<std::pair<int, std::string>> SaSpectrumView::makeLinearFreqTics(int low, int high)
{
	std::vector<std::pair<int, std::string>> result;
	int increment;

	// select a suitable increment based on zoom level
	if (high - low < 500) {increment = 50;}
	else if (high - low < 1000) {increment = 100;}
	else if (high - low < 5000) {increment = 1000;}
	else {increment = 2000;}

	// generate steps based on increment, starting at 0
	for (int i = 0; i <= high; i += increment)
	{
		if (i >= low)
		{
			if (i < 1000)
			{
				result.emplace_back(i, std::to_string(i));
			}
			else
			{
				result.emplace_back(i, std::to_string(i/1000) + "k");
			}
		}
	}
	return result;
}


// Generate labels suitable for logarithmic (dB) amplitude scale.
// Low / high limits are in dB; 0 dB amplitude = 1.0 linear.
// Treating results as power ratio, i.e., 3 dB should be about twice as loud.
std::vector<std::pair<float, std::string>> SaSpectrumView::makeLogAmpTics(int low, int high)
{
	std::vector<std::pair<float, std::string>> result;
	double increment;

	// Base zoom level on selected range and how close is the current height
	// to the sizeHint() (denser scale for bigger window).
	if ((high - low) < 20 * ((float)height() / sizeHint().height()))
	{
		increment = fastPow10f(0.3f); // 3 dB steps when really zoomed in
	}
	else if (high - low < 45 * ((float)height() / sizeHint().height()))
	{
		increment = fastPow10f(0.6f); // 6 dB steps when sufficiently zoomed in
	}
	else
	{
		increment = 10;				// 10 dB steps otherwise
	}

	// Generate n dB increments, start checking at -90 dB. Limits are tweaked
	// just a little bit to make sure float comparisons do not miss edges.
	for (float i = 0.000000001f; 10 * std::log10(i) <= (high + 0.001); i *= increment)
	{
		if (10 * std::log10(i) >= (low - 0.001))
		{
			result.emplace_back(i, std::to_string((int)std::round(10 * std::log10(i))));
		}
	}
	return result;
}


// Generate labels suitable for linear amplitude scale.
// Low / high limits are in dB; 0 dB amplitude = 1.0 linear.
// Smallest possible label is 0.001, largest is 999. This includes the majority
// of useful labels; going lower or higher would require increasing margin size
// so that the text can fit. That would be a waste of space -- the linear scale
// would only make the experience worse for the main, logarithmic (dB) scale.
std::vector<std::pair<float, std::string>> SaSpectrumView::makeLinearAmpTics(int low, int high)
{
	std::vector<std::pair<float, std::string>> result;
	// make about 5 labels when window is small, 10 if it is big
	float split = (float)height() / sizeHint().height() >= 1.5 ? 10.0 : 5.0;

	// convert limits to linear scale
	float lin_low = fastPow10f(low / 10.0);
	float lin_high = fastPow10f(high / 10.0);

	// Linear scale will vary widely, so instead of trying to craft extra nice
	// multiples, just generate a few evenly spaced increments across the range,
	// paying attention only to the decimal places to keep labels short.
	// Limits are shifted a bit so that float comparisons do not miss edges.
	for (double i = 0; i <= (lin_high + 0.0001); i += (lin_high - lin_low) / split)
	{
		if (i >= (lin_low - 0.0001))
		{
			if (i >= 9.99 && i < 99.9)
			{
				double nearest = std::round(i);
				result.emplace_back(nearest, std::to_string(nearest).substr(0, 2));
			}
			else if (i >= 0.099)
			{	// also covers numbers above 100
				double nearest = std::round(i * 10) / 10;
				result.emplace_back(nearest, std::to_string(nearest).substr(0, 3));
			}
			else if (i >= 0.0099)
			{
				double nearest = std::round(i * 1000) / 1000;
				result.emplace_back(nearest, std::to_string(nearest).substr(0, 4));
			}
			else if	(i >= 0.00099)
			{
				double nearest = std::round(i * 10000) / 10000;
				result.emplace_back(nearest, std::to_string(nearest).substr(1, 4));
			}
			else if (i > -0.01 && i < 0.01)
			{
				result.emplace_back(i, "0");	// an exception, zero is short..
			}
		}
	}
	return result;
}


// Periodic update is called by LMMS.
void SaSpectrumView::periodicUpdate()
{
	// check if the widget is visible; if it is not, processing can be paused
	m_processor->setSpectrumActive(isVisible());
	// tell Qt it is time for repaint
	update();
}


// Handle mouse input: set new cursor position.
// For some reason (a bug?), localPos() only returns integers. As a workaround
// the fractional part is taken from windowPos() (which works correctly).
void SaSpectrumView::mouseMoveEvent(QMouseEvent *event)
{
	m_cursor = QPointF(	event->localPos().x() - (event->windowPos().x() - (long)event->windowPos().x()),
						event->localPos().y() - (event->windowPos().y() - (long)event->windowPos().y()));
}

void SaSpectrumView::mousePressEvent(QMouseEvent *event)
{
	m_cursor = QPointF(	event->localPos().x() - (event->windowPos().x() - (long)event->windowPos().x()),
						event->localPos().y() - (event->windowPos().y() - (long)event->windowPos().y()));
}


// Handle resize event: rebuild grid and labels
void SaSpectrumView::resizeEvent(QResizeEvent *event)
{
	// frequency does not change density with size
	// amplitude does: rebuild labels
	m_logAmpTics = makeLogAmpTics(m_processor->getAmpRangeMin(), m_processor->getAmpRangeMax());
	m_linearAmpTics = makeLinearAmpTics(m_processor->getAmpRangeMin(), m_processor->getAmpRangeMax());
}


} // namespace lmms::gui
