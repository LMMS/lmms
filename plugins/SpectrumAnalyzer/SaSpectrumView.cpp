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
#include <mutex>

#include "GuiApplication.h"
#include "MainWindow.h"
#include "SaProcessor.h"

#ifdef SA_DEBUG
	#include <chrono>
	#include <iostream>
#endif


SaSpectrumView::SaSpectrumView(SaControls *controls, SaProcessor *processor, QWidget *_parent) :
	QWidget(_parent),
	m_controls(controls),
	m_processor(processor),
	m_periodicUpdate(false),
	m_freezeRequest(false),
	m_frozen(false)
{
	setMinimumSize(360, 170);
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

	connect(gui->mainWindow(), SIGNAL(periodicUpdate()),
			this, SLOT(periodicUpdate()));

	m_displayBufferL.resize(m_processor->binCount(), 0);
	m_displayBufferR.resize(m_processor->binCount(), 0);
	m_peakBufferL.resize(m_processor->binCount(), 0);
	m_peakBufferR.resize(m_processor->binCount(), 0);

	m_freqRangeIndex = m_controls->m_freqRangeModel.value();
	m_ampRangeIndex = m_controls->m_ampRangeModel.value();

	m_logFreqTics		= makeLogFreqTics(m_processor->getFreqRangeMin(),
										  m_processor->getFreqRangeMax());
	m_linearFreqTics	= makeLinearFreqTics(m_processor->getFreqRangeMin(),
											 m_processor->getFreqRangeMax());
	m_logAmpTics		= makeLogAmpTics(m_processor->getAmpRangeMin(),
										 m_processor->getAmpRangeMax());
	m_linearAmpTics		= makeLinearAmpTics(m_processor->getAmpRangeMin(),
											m_processor->getAmpRangeMax());

	m_cursor = QPoint(0, 0);
}


// Compose and draw all the content; periodically called by Qt.
// NOTE: Performance sensitive! If the drawing takes too long, it will drag
// the FPS down for the entire program! Use SA_DEBUG to display timings.
void SaSpectrumView::paintEvent(QPaintEvent *event) {
	#ifdef SA_DEBUG
		int total_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	#endif

	// 0) Constants and init
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);

	m_displayTop = 1;
	m_displayBottom = height() -20;
	m_displayLeft = 20;
	m_displayRight = width() -20;
	m_displayWidth = m_displayRight - m_displayLeft;

	// recompute range labels if needed
	if (m_freqRangeIndex != m_controls->m_freqRangeModel.value()) {
		m_logFreqTics = makeLogFreqTics(m_processor->getFreqRangeMin(),
										m_processor->getFreqRangeMax());
		m_linearFreqTics = makeLinearFreqTics(m_processor->getFreqRangeMin(true),
											  m_processor->getFreqRangeMax());
		m_freqRangeIndex = m_controls->m_freqRangeModel.value();
	}
	if (m_ampRangeIndex != m_controls->m_ampRangeModel.value()) {
		m_logAmpTics = makeLogAmpTics(m_processor->getAmpRangeMin(),
									  m_processor->getAmpRangeMax());
		m_linearAmpTics = makeLinearAmpTics(m_processor->getAmpRangeMin(),
											m_processor->getAmpRangeMax());
		m_ampRangeIndex = m_controls->m_ampRangeModel.value();
	}

	// generate freeze request or clear "frozen" status based on freeze button
	if (!m_frozen && m_controls->m_refFreezeModel.value()) {
		m_freezeRequest = true;
	} else if (!m_controls->m_refFreezeModel.value()) {
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
	painter.setPen(QPen(m_controls->m_colorGrid, 2,
						Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	painter.drawRoundedRect(m_displayLeft, 1,
							m_displayWidth, m_displayBottom,
							2.0, 2.0);

	// display measurement results
	#ifdef SA_DEBUG
		total_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() - total_time;
		painter.setPen(QPen(m_controls->m_colorLabels, 1,
							Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
		painter.drawText(m_displayRight -100, 70, 100, 16, Qt::AlignLeft,
						 QString(std::string("Max FPS: " + std::to_string(1000000000.0 / total_time)).c_str()));
	#endif
}


// Refresh data if needed and draw the spectrum.
void SaSpectrumView::drawSpectrum(QPainter &painter) {
	#ifdef SA_DEBUG
		int line_time = 0, draw_time = 0;
	#endif

	// draw the graph only if there is any input, averaging residue or peaks
	m_processor->m_dataAccess.lock();
	if (m_decaySum > 0 || notEmpty(m_processor->m_normSpectrumL) ||
		notEmpty(m_processor->m_normSpectrumR)) {
		m_processor->m_dataAccess.unlock();
		// update paths with new data if update was requested by Qt
		if (m_periodicUpdate == true) {
			#ifdef SA_DEBUG
				line_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
			#endif
			refreshPaths();
			#ifdef SA_DEBUG
				line_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() - line_time;
			#endif

			m_periodicUpdate = false;
		}

		// draw stored paths
		#ifdef SA_DEBUG
			draw_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
		#endif
		// in case stereo is disabled, mono data are stored in left channel structures
		if (m_controls->m_stereoModel.value()) {
			painter.fillPath(m_pathR, QBrush(m_controls->m_colorR));
			painter.fillPath(m_pathL, QBrush(m_controls->m_colorL));
		} else {
			painter.fillPath(m_pathL, QBrush(m_controls->m_colorMono));
		}
		// draw the peakBuffer only if peak hold or reference freeze is active
		if (m_controls->m_peakHoldModel.value() || m_controls->m_refFreezeModel.value()) {
			if (m_controls->m_stereoModel.value()) {
				painter.setPen(QPen(m_controls->m_colorR, 1,
									Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
				painter.drawPath(m_pathPeakR);
				painter.setPen(QPen(m_controls->m_colorL, 1,
									Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
				painter.drawPath(m_pathPeakL);
			} else {
				painter.setPen(QPen(m_controls->m_colorL, 1,
									Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
				painter.drawPath(m_pathPeakL);
			}
		}
		#ifdef SA_DEBUG
			draw_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() - draw_time;
		#endif
	} else {
		m_processor->m_dataAccess.unlock();
	}

	// display measurement results
	#ifdef SA_DEBUG
		painter.drawText(m_displayRight -100, 90, 100, 16, Qt::AlignLeft,
						 QString(std::string("Lines ms: " + std::to_string(line_time / 1000000.0)).c_str()));
		painter.drawText(m_displayRight -100, 110, 100, 16, Qt::AlignLeft,
						 QString(std::string("Draw ms: " + std::to_string(draw_time / 1000000.0)).c_str()));
	#endif
}


// Read newest FFT results from SaProcessor, update local display buffers
// and build QPainter paths.
void SaSpectrumView::refreshPaths() {
	// Lock is required for the entire function, mainly to prevent block size
	// changes from causing reallocation of data structures mid-way.
	m_processor->m_dataAccess.lock();

	// check if bin count changed and reallocate display buffers accordingly
	if (m_processor->binCount() != m_displayBufferL.size()) {
		m_displayBufferL.clear();
		m_displayBufferR.clear();
		m_peakBufferL.clear();
		m_peakBufferR.clear();
		m_displayBufferL.resize(m_processor->binCount(), 0);
		m_displayBufferR.resize(m_processor->binCount(), 0);
		m_peakBufferL.resize(m_processor->binCount(), 0);
		m_peakBufferR.resize(m_processor->binCount(), 0);
	}

	m_decaySum = 0;
	float *bins = m_processor->m_normSpectrumL.data();
	float *displayBuffer = m_displayBufferL.data();
	float *peakBuffer = m_peakBufferL.data();

	// refresh display buffers for left and right channel
	#ifdef SA_DEBUG
		int refresh_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	#endif
	for (int channel = 0; channel <= 1; channel++) {
		for (int n = 0; n < m_processor->binCount(); n++) {
			// update direct or average spectrum display
			if (!m_controls->m_pauseModel.value()) {
				if (m_controls->m_smoothModel.value()) {
					displayBuffer[n] = bins[n] * m_smoothFactor +
									   displayBuffer[n] * (1 - m_smoothFactor);
				} else {
					displayBuffer[n] = bins[n];
				}
			}
			// Update peak-hold and reference freeze data (using a shared curve
			// to save draw time and to keep screen clean and readable).
			// Peak hold and freeze can be combined: decay only if not frozen.
			if (m_controls->m_refFreezeModel.value() && m_freezeRequest) {
				peakBuffer[n] = displayBuffer[n];
			} else if (m_controls->m_peakHoldModel.value() && !m_controls->m_pauseModel.value()) {
				if (bins[n] > peakBuffer[n]) {
					peakBuffer[n] = bins[n];
				} else if (!m_controls->m_refFreezeModel.value()) {
					peakBuffer[n] = peakBuffer[n] * m_peakFallFactor;
				}
			} else if (!m_controls->m_refFreezeModel.value() && !m_controls->m_peakHoldModel.value()) {
				peakBuffer[n] = 0;
			}
			// take note if there was actually anything to display
			m_decaySum += displayBuffer[n] + peakBuffer[n];
		}
		// go for second (right channel) iteration if stereo processing enabled
		if (channel == 0 && m_controls->m_stereoModel.value()) {
			bins = m_processor->m_normSpectrumR.data();
			displayBuffer = m_displayBufferR.data();
			peakBuffer = m_peakBufferR.data();
		} else {
			break;
		}
	}
	#ifdef SA_DEBUG
		refresh_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() - refresh_time;
	#endif

	if (m_controls->m_refFreezeModel.value() && m_freezeRequest) {
		m_freezeRequest = false;
		m_frozen = true;
	}

	#ifdef SA_DEBUG
		int make_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	#endif
	// Use updated display buffers to prepare new paths for QPainter.
	// This is the second slowest action (first is drawing); use the resolution
	// parameter to balance display quality and performance.
	m_pathL = makePath(m_displayBufferL, 1.5);
	if (m_controls->m_stereoModel.value()) {
		m_pathR = makePath(m_displayBufferR, 1.5);
	}
	if (m_controls->m_peakHoldModel.value() || m_controls->m_refFreezeModel.value()) {
		m_pathPeakL = makePath(m_peakBufferL, 0.2);
		if (m_controls->m_stereoModel.value()) {
			m_pathPeakR = makePath(m_peakBufferR, 0.2);
		}
	}
	#ifdef SA_DEBUG
		make_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() - make_time;
	#endif

	m_processor->m_dataAccess.unlock();

	// display measurement results
	#ifdef SA_DEBUG
		std::cout << "Refresh ms: " << std::to_string(refresh_time / 1000000.0) << " ";
		std::cout << "Path-make ms: " << std::to_string(make_time / 1000000.0) << std::endl;
	#endif
}


// Use display buffer to build a path that can be drawn or filled by QPainter.
// Resolutions controls the performance / quality tradeoff; its value specifies
// the number of points in x axis per device pixel. Values over 1.0 still
// contribute to quality and accuracy thanks to anti-aliasing.
QPainterPath SaSpectrumView::makePath(std::vector<float> &displayBuffer, float resolution = 1.0) {
	// convert resolution to number of path points per logical pixel
	float pixel_limit = resolution * window()->devicePixelRatio();

	QPainterPath path;
	path.moveTo(m_displayLeft, m_displayBottom);

	// translate frequency bins to path points
	// display is flipped: y values grow towards zero, initial max is bottom
	float max = m_displayBottom;
	float first_x = -1;		// lower bound of currently constructed point
	for (int n = 0; n < m_processor->binCount(); n++) {
		float x = freqToXPixel(binToFreq(n), m_displayWidth);
		float x1 = freqToXPixel(binToFreq(n + 1), m_displayWidth);
		float y = ampToYPixel(displayBuffer[n], m_displayBottom);

		if (0 < x && x < m_displayWidth) {
			if (first_x == -1) {
				first_x = x;
				// the first displayed bin is stretched to the left edge to prevent
				// creating a misleading slope leading to zero (at log. scale)
				path.lineTo(m_displayLeft, y + m_displayTop);
			}
			// Opt.: QPainter is very slow -- draw at most [pixel_limit] points
			// per logical pixel. As opposed to limiting the bin count, this
			// allows high resolution display if user resizes the analyzer.
			// Accumulate bins that share the pixel and use the highest:
			max = y < max ? y : max;
			// And make the final point in the middle of accumulated segment.
			if ((int)(x * pixel_limit) != (int)(x1 * pixel_limit)) {
				x = (x + first_x) / 2;
				path.lineTo(x + m_displayLeft, max + m_displayTop);
				max = m_displayBottom;
				first_x = x1;
			}
		} else {
			// stop processing after a bin falls outside right edge
			// and align it to the edge to prevent a gap
			if (n > 0 && x > 0) {
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
void SaSpectrumView::drawGrid(QPainter &painter) {
	std::vector<std::pair<int, std::string>> *freqTics = NULL;
	std::vector<std::pair<float, std::string>> *ampTics = NULL;
	float pos = 0;

	// always draw the background
	painter.fillRect(m_displayLeft, 1,
					 m_displayWidth, m_displayBottom,
					 m_controls->m_colorBG);

	// select logarithmic or linear frequency grid and draw it
	if (m_controls->m_logXModel.value()) {
		freqTics = &m_logFreqTics;
	} else {
		freqTics = &m_linearFreqTics;
	}
	// frequency grid
	painter.setPen(QPen(m_controls->m_colorGrid, 1,
						Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	for (auto &line: *freqTics){
		painter.drawLine(m_displayLeft + freqToXPixel(line.first, m_displayWidth),
						 2,
						 m_displayLeft + freqToXPixel(line.first, m_displayWidth),
						 m_displayBottom);
	}
	// frequency labels
	painter.setPen(QPen(m_controls->m_colorLabels, 1,
						Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	for (auto & line: *freqTics) {
		pos = m_displayLeft + freqToXPixel(line.first, m_displayWidth);
		// make an exception if the first or last label is too close to edge
		if (line == freqTics->front() && pos - 12 < m_displayLeft) {
			painter.drawText(m_displayLeft, m_displayBottom + 5,
							 24, 15,
							 Qt::AlignLeft,
							 QString(line.second.c_str()));
		} else if (line == freqTics->back() && pos + 10 > m_displayRight) {
			painter.drawText(m_displayRight - 24, m_displayBottom + 5,
							 24, 15,
							 Qt::AlignRight,
							 QString(line.second.c_str()));
		} else {
			painter.drawText(pos - 12, m_displayBottom + 5,
							 24, 15,
							 Qt::AlignHCenter,
							 QString(line.second.c_str()));
		}
	}

	// select logarithmic or linear amplitude grid and draw it
	if (m_controls->m_logYModel.value()) {
		ampTics = &m_logAmpTics;
	} else {
		ampTics = &m_linearAmpTics;
	}
	// amplitude grid
	painter.setPen(QPen(m_controls->m_colorGrid, 1,
						Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	for (auto & line: *ampTics){
		painter.drawLine(m_displayLeft + 1,
						 ampToYPixel(line.first, m_displayBottom),
						 m_displayRight - 1,
						 ampToYPixel(line.first, m_displayBottom));
	}
	// amplitude labels
	painter.setPen(QPen(m_controls->m_colorLabels, 1,
						Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	bool stereo = m_controls->m_stereoModel.value();
	for (auto & line: *ampTics) {
		pos = ampToYPixel(line.first, m_displayBottom);
		// make an exception if the top or bottom label is too close to edge
		if (line == ampTics->back() && pos < 8) {
			if (stereo) {
				painter.setPen(QPen(m_controls->m_colorL.lighter(),
									1, Qt::SolidLine,
									Qt::RoundCap, Qt::BevelJoin));
			}
			painter.drawText(2, 0, 16, 16,
							 Qt::AlignRight | Qt::AlignTop,
							 QString(line.second.c_str()));
			if (stereo) {
				painter.setPen(QPen(m_controls->m_colorR.lighter(),
									1, Qt::SolidLine,
									Qt::RoundCap, Qt::BevelJoin));
			}
			painter.drawText(m_displayRight + 2, 0, 16, 16,
							 Qt::AlignLeft | Qt::AlignTop,
							 QString(line.second.c_str()));
		} else if (line == ampTics->front() && pos > m_displayBottom - 16) {
			if (stereo) {
				painter.setPen(QPen(m_controls->m_colorL.lighter(),
									1, Qt::SolidLine,
									Qt::RoundCap, Qt::BevelJoin));
			}
			painter.drawText(2, m_displayBottom - 14, 16, 16,
							 Qt::AlignRight | Qt::AlignBottom,
							 QString(line.second.c_str()));
			if (stereo) {
				painter.setPen(QPen(m_controls->m_colorR.lighter(),
									1, Qt::SolidLine,
									Qt::RoundCap, Qt::BevelJoin));
			}
			painter.drawText(m_displayRight + 2, m_displayBottom - 14, 16, 16,
							 Qt::AlignLeft | Qt::AlignBottom,
							 QString(line.second.c_str()));
		} else {
			if (stereo) {
				painter.setPen(QPen(m_controls->m_colorL.lighter(),
									1, Qt::SolidLine,
									Qt::RoundCap, Qt::BevelJoin));
			}
			painter.drawText(2, pos - 8, 16, 16,
							 Qt::AlignRight | Qt::AlignVCenter,
							 QString(line.second.c_str()));
			if (stereo) {
				painter.setPen(QPen(m_controls->m_colorR.lighter(),
									1, Qt::SolidLine,
									Qt::RoundCap, Qt::BevelJoin));
			}
			painter.drawText(m_displayRight + 2, pos - 8, 16, 16,
							 Qt::AlignLeft | Qt::AlignVCenter,
							 QString(line.second.c_str()));
		}
	}
}


// Draw cursor its coordinates if it is within display bounds.
void SaSpectrumView::drawCursor(QPainter &painter) {
	if (m_cursor.x() > 0 && m_cursor.y() > 0
		&& m_cursor.x() < width() && m_cursor.y() < height()) {
		// cursor
		painter.setPen(QPen(m_controls->m_colorGrid.lighter(), 1,
							Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
		painter.drawLine(m_cursor.x(), m_displayTop,
						 m_cursor.x(), m_displayBottom);
		painter.drawLine(m_displayLeft, m_cursor.y(),
						 m_displayRight, m_cursor.y());
		// coordinates
		painter.setPen(QPen(m_controls->m_colorLabels.darker(), 1,
							Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
		painter.drawText(m_displayRight -60, 5, 100, 16,
						 Qt::AlignLeft,
						 "Cursor");
		QString tmps;
		tmps = QString(std::string(std::to_string((int)m_processor->xPixelToFreq(m_cursor.x() - m_displayLeft, m_displayWidth)) + " Hz").c_str());
		painter.drawText(m_displayRight -60, 18, 100, 16, Qt::AlignLeft, tmps);
		if (m_controls->m_logYModel.value()) {
			tmps = QString(std::string(std::to_string(m_processor->yPixelToAmp(m_cursor.y(), m_displayBottom)).substr(0, 5) + " dB").c_str());
		} else {
			tmps = QString(std::string(std::to_string(m_processor->yPixelToAmp(m_cursor.y(), m_displayBottom))).substr(0, 5).c_str());
		}
		painter.drawText(m_displayRight -60, 30, 100, 16, Qt::AlignLeft, tmps);
	}
}


// Wrappers for SaProcessor helpers (to make local code more compact).
float SaSpectrumView::binToFreq(int bin_index) {
	return m_processor->binToFreq(bin_index);
}


float SaSpectrumView::freqToXPixel(float frequency, int width) {
	return m_processor->freqToXPixel(frequency, width);
}


float SaSpectrumView::ampToYPixel(float amplitude, int height) {
	return m_processor->ampToYPixel(amplitude, height);
}


// Generate labels suitable for logarithmic frequency scale.
// Low / high limits are in Hz.
std::vector<std::pair<int, std::string>> SaSpectrumView::makeLogFreqTics(int low, int high) {
	std::vector<std::pair<int, std::string>> result;
	int i, j;
	int a[] = {10, 20, 50};		// sparse series
	int b[] = {14, 30, 70};		// additional (denser) series

	// generate large 10x steps, starting from lowest sensible frequency
	for (i = 1; i <= high; i *= 10) {
		for (j = 0; j < 3; j++) {
			// insert a label from sparse series if it falls within bounds
			if (i * a[j] >= low && i * a[j] <= high) {
				if (i * a[j] < 1000) {
					result.push_back(std::pair<int, std::string>(i * a[j], std::to_string(i * a[j])));
				} else {
					result.push_back(std::pair<int, std::string>(i * a[j], std::to_string(i * a[j] / 1000) + "k"));
				}
			}
			// also insert denser series if high and low values are close
			if ((log10(high) - log10(low) < 2) && (i * b[j] >= low && i * b[j] <= high)) {
				if (i * b[j] < 1500) {
					result.push_back(std::pair<int, std::string>(i * b[j], std::to_string(i * b[j])));
				} else {
					result.push_back(std::pair<int, std::string>(i * b[j], std::to_string(i * b[j] / 1000) + "k"));
				}
			}
		}
	}
	return result;
}


// Generate labels suitable for linear frequency scale.
// Low / high limits are in Hz.
std::vector<std::pair<int, std::string>> SaSpectrumView::makeLinearFreqTics(int low, int high) {
	std::vector<std::pair<int, std::string>> result;
	int i, increment;

	// select a suitable increment based on zoom level
	if (high - low < 500) {increment = 50;}
	else if (high - low < 1000) {increment = 100;}
	else if (high - low < 5000) {increment = 1000;}
	else {increment = 2000;}

	// generate steps based on increment
	for (i = 0; i <= high; i += increment) {
		if (i >= low){
			if (i < 1000) {
				result.push_back(std::pair<int, std::string>(i, std::to_string(i)));
			} else {
				result.push_back(std::pair<int, std::string>(i, std::to_string(i/1000) + "k"));
			}
		}
	}
	return result;
}


// Generate labels suitable for logarithmic (dB) amplitude scale.
// Low / high limits are in dB; 0 dB amplitude = 1.0 linear.
std::vector<std::pair<float, std::string>> SaSpectrumView::makeLogAmpTics(int low, int high) {
	std::vector<std::pair<float, std::string>> result;
	float i;
	float increment;

	if (high - low < 45) {
		increment = pow(10, 0.6);	// 6 dB steps when sufficiently zoomed in
	} else {
		increment = 10;				// 10 dB steps otherwise
	}
	// generate n dB increments, start checking at -90 dB
	for (i = 0.000000001; 10 * log10(i) <= high; i *= increment){
		if (10 * log10(i) >= low) {
			result.push_back(std::pair<float, std::string>(i, std::to_string((int)std::round(10 * log10(i)))));
		}
	}
	return result;
}


// Generate labels suitable for linear amplitude scale.
// Low / high limits are in dB; 0 dB amplitude = 1.0 linear.
std::vector<std::pair<float, std::string>> SaSpectrumView::makeLinearAmpTics(int low, int high) {
	std::vector<std::pair<float, std::string>> result;
	float i, nearest;

	// convert limits to linear scale
	float lin_low = pow(10, low / 10.0);
	float lin_high = pow(10, high / 10.0);

	// generate 5 linear amplitude increments
	for (i = 0; i <= lin_high; i += (lin_high - lin_low) / 5.0){
		if (i >= lin_low){
			if (i < 0.01) {
				nearest = std::round(i * 1000) / 1000;
				result.push_back(std::pair<float, std::string>(nearest, std::to_string(nearest).substr(1, 3)));
			} else if (i >= 10) {
				nearest = std::round(i);
				result.push_back(std::pair<float, std::string>(nearest, std::to_string(nearest).substr(0, 2)));
			} else {
				nearest = std::round(i * 10) / 10;
				result.push_back(std::pair<float, std::string>(nearest, std::to_string(nearest).substr(0, 3)));
			}
		}
	}
	return result;
}


// When Qt asks for update, also check if the widget is visible.
// If it is not, the processor can stop processing.
void SaSpectrumView::periodicUpdate() {
	m_periodicUpdate = true;
	m_processor->setActive(isVisible());
	update();
}


void SaSpectrumView::mouseMoveEvent(QMouseEvent *event) {
	m_cursor = event->pos();
}


void SaSpectrumView::mousePressEvent(QMouseEvent *event) {
	m_cursor = event->pos();
}
