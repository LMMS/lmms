/* SaSpectrumView.cpp - implementation of SaSpectrumView class.
*
* Copyright (c) 2014-2017, David French <dave/dot/french3/at/googlemail/dot/com>
* Copyright (c) 2019 Martin Pavelek <he29/dot/HS/at/gmail/dot/com>
*
* This file is part of LMMS - https://lmms.io
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
#include <iostream>
#include <mutex>

#include "Engine.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "Mixer.h"
#include "SaProcessor.h"

SaSpectrumView::SaSpectrumView(SaControls *controls, SaProcessor *processor, QWidget *_parent) :
	QWidget(_parent),
	m_controls(controls),
	m_processor(processor),
	m_periodicUpdate(false),
	m_freezeRequest(false)
{
	setMinimumSize(400, 200);

	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	connect(gui->mainWindow(), SIGNAL(periodicUpdate()), this, SLOT(periodicUpdate()));

	m_binHeightL.resize(m_processor->binCount(), 0);
	m_binHeightR.resize(m_processor->binCount(), 0);
	m_binPeakL.resize(m_processor->binCount(), 0);
	m_binPeakR.resize(m_processor->binCount(), 0);

	m_freqRangeIndex = m_controls->m_freqRangeModel.value();
	m_ampRangeIndex = m_controls->m_ampRangeModel.value();
	m_logFreqTics = makeLogTics(m_processor->getFreqRangeMin(), m_processor->getFreqRangeMax());
	m_linearFreqTics = makeLinearTics(m_processor->getFreqRangeMin(), m_processor->getFreqRangeMax());
	m_logAmpTics = makeDBTics(m_processor->getAmpRangeMin(), m_processor->getAmpRangeMax());
	m_linearAmpTics = makeAmpTics(m_processor->getAmpRangeMin(), m_processor->getAmpRangeMax());

	m_cursor = QPoint(0, 0);
}


void SaSpectrumView::paintEvent(QPaintEvent *event)
{
	#ifdef DEBUG
		int start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
		int line_time = 0, draw_time = 0;
	#endif

	// 0) constants and init stuff
	const float smoothFactor = 0.15;

	const bool stereo = m_controls->m_stereoModel.value();
	const bool freeze = m_freezeRequest;

	const int displayTop = 1;
	const int displayBottom = height() -20;
	const int displayLeft = 20;
	const int displayRight = width() -20;
	const int displayWidth = displayRight - displayLeft;

	float pos = 0;
	std::vector<std::pair<int, std::string>> * freqTics = NULL;
	std::vector<std::pair<float, std::string>> * ampTics = NULL;

	// update ranges if needed
	if (m_freqRangeIndex != m_controls->m_freqRangeModel.value()) {
		m_freqRangeIndex = m_controls->m_freqRangeModel.value();
		m_logFreqTics = makeLogTics(m_processor->getFreqRangeMin(), m_processor->getFreqRangeMax());
		m_linearFreqTics = makeLinearTics(m_processor->getFreqRangeMin(true), m_processor->getFreqRangeMax());
	}

	if (m_ampRangeIndex != m_controls->m_ampRangeModel.value()) {
		m_ampRangeIndex = m_controls->m_ampRangeModel.value();
		m_logAmpTics = makeDBTics(m_processor->getAmpRangeMin(), m_processor->getAmpRangeMax());
		m_linearAmpTics = makeAmpTics(m_processor->getAmpRangeMin(), m_processor->getAmpRangeMax());
	}

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);


	// 1) background, grid and labels
	// always draw the background
	painter.fillRect(displayLeft, 1, displayWidth, displayBottom, m_controls->m_colorBG);

	// select logarithmic or linear frequency grid and draw it
	if (m_controls->m_logXModel.value()) {
		freqTics = &m_logFreqTics;
	} else {
		freqTics = &m_linearFreqTics;
	}
	// frequency grid
	painter.setPen(QPen(m_controls->m_colorGrid, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	for (auto & line: *freqTics){
		painter.drawLine(	displayLeft + freqToXPixel(line.first, displayWidth), 2,
							displayLeft + freqToXPixel(line.first, displayWidth), displayBottom);
	}
	// frequency labels
	painter.setPen(QPen(m_controls->m_colorLabels, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	for (auto & line: *freqTics) {
		pos = displayLeft + freqToXPixel(line.first, displayWidth);
		// make an exception if the first or last label is too close to edge
		if (line == freqTics->front() && pos - 12 < displayLeft) {
			painter.drawText(displayLeft, displayBottom + 5, 24, 15, Qt::AlignLeft, QString(line.second.c_str()));
		} else if (line == freqTics->back() && pos + 10 > displayRight) {
			painter.drawText(displayRight - 24, displayBottom + 5, 24, 15, Qt::AlignRight, QString(line.second.c_str()));
		} else {
			painter.drawText(pos - 12, displayBottom + 5, 24, 15, Qt::AlignHCenter, QString(line.second.c_str()));
		}
	}

	// select logarithmic or linear amplitude grid and draw it
	if (m_controls->m_logYModel.value()) {
		ampTics = &m_logAmpTics;
	} else {
		ampTics = &m_linearAmpTics;
	}
	// amplitude grid
	painter.setPen(QPen(m_controls->m_colorGrid, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	for (auto & line: *ampTics){
		painter.drawLine(	displayLeft + 1, ampToYPixel(line.first, displayBottom),
							displayRight - 1, ampToYPixel(line.first, displayBottom));
	}
	// amplitude labels
	painter.setPen(QPen(m_controls->m_colorLabels, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	for (auto & line: *ampTics) {
		pos = ampToYPixel(line.first, displayBottom);
		// make an exception if the top or bottom label is too close to edge
		if (line == ampTics->back() && pos < 8) {
			if (stereo) {painter.setPen(QPen(m_controls->m_colorL.lighter(), 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));}
			painter.drawText(2, 0, 16, 16, Qt::AlignRight | Qt::AlignTop, QString(line.second.c_str()));
			if (stereo) {painter.setPen(QPen(m_controls->m_colorR.lighter(), 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));}
			painter.drawText(displayRight + 2, 0, 16, 16, Qt::AlignLeft | Qt::AlignTop, QString(line.second.c_str()));
		} else if (line == ampTics->front() && pos > displayBottom - 16) {
			if (stereo) {painter.setPen(QPen(m_controls->m_colorL.lighter(), 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));}
			painter.drawText(2, displayBottom - 14, 16, 16, Qt::AlignRight | Qt::AlignBottom, QString(line.second.c_str()));
			if (stereo) {painter.setPen(QPen(m_controls->m_colorR.lighter(), 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));}
			painter.drawText(displayRight + 2, displayBottom - 14, 16, 16, Qt::AlignLeft | Qt::AlignBottom, QString(line.second.c_str()));
		} else {
			if (stereo) {painter.setPen(QPen(m_controls->m_colorL.lighter(), 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));}
			painter.drawText(2, pos - 8, 16, 16, Qt::AlignRight | Qt::AlignVCenter, QString(line.second.c_str()));
			if (stereo) {painter.setPen(QPen(m_controls->m_colorR.lighter(), 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));}
			painter.drawText(displayRight + 2, pos - 8, 16, 16, Qt::AlignLeft | Qt::AlignVCenter, QString(line.second.c_str()));
		}
	}


	// 2) Spectrum display
	// draw the graph only if there is any input or smooth decay / averaging residue
	m_processor->m_dataAccess.lock();
	if (m_decaySum > 0 || notEmpty(m_processor->m_normSpectrumL) || notEmpty(m_processor->m_normSpectrumR)) {
		// update paths with new data if needed
		if (!m_processor->getInProgress() && m_periodicUpdate == true && !m_controls->m_pauseModel.value()) {
			if (m_processor->binCount() != m_binHeightL.size()) {
				m_binHeightL.clear();
				m_binHeightR.clear();
				m_binPeakL.clear();
				m_binPeakR.clear();
				m_binHeightL.resize(m_processor->binCount(), 0);
				m_binHeightR.resize(m_processor->binCount(), 0);
				m_binPeakL.resize(m_processor->binCount(), 0);
				m_binPeakR.resize(m_processor->binCount(), 0);
			}

			m_periodicUpdate = false;
			m_decaySum = 0;
		
			float *bins = m_processor->m_normSpectrumL.data();
			std::vector<float> *m_binHeight = &m_binHeightL;
			std::vector<float> *m_binPeak = &m_binPeakL;
			QPainterPath m_path;

			#ifdef DEBUG
				line_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
			#endif

			// draw left and right channel
			for (int i = 0; i <= 1; i++) {
				m_path = QPainterPath();
				m_path.moveTo(displayLeft, displayBottom);

				// the first bin is stretched to the left edge to prevent
				// creating a misleading slope leading to zero (at log. scale)
				m_path.lineTo(	displayLeft,
								ampToYPixel((*m_binHeight)[0], displayBottom));

				for (int n = 0; n < m_processor->binCount(); n++) {
					// direct or average spectrum display
					if (m_controls->m_smoothModel.value()) {
						(*m_binHeight)[n] = bins[n] * smoothFactor + (*m_binHeight)[n] * (1 - smoothFactor);
					} else {
						(*m_binHeight)[n] = bins[n];
					}

					// peak-hold and reference freeze (using the same curve
					// to save resources and keep screen clean and readable)
					if (m_controls->m_refFreezeModel.value() && freeze) {
						(*m_binPeak)[n] = bins[n];
					} else if (m_controls->m_peakHoldModel.value()) {
						if (bins[n] > (*m_binPeak)[n]) {
							(*m_binPeak)[n] = bins[n];
						}
					}

					float x = freqToXPixel(binToFreq(n), displayWidth);
					if (0 <= x && x <= displayWidth) {
						m_path.lineTo(	x + displayLeft,
										ampToYPixel((*m_binHeight)[n], displayBottom));
						m_decaySum += (*m_binHeight)[n];
					} else {
						// first bin outside right edge gets aligned to the right edge to prevent a gap
						if (n > 0 && x > 0 && freqToXPixel(binToFreq(n-1), displayWidth) <= displayWidth) {
							m_path.lineTo(	displayRight,
											ampToYPixel((*m_binHeight)[n], displayBottom));
						}
					}
				}

				m_path.lineTo(displayRight, displayBottom);
				m_path.closeSubpath();

				// go for second iteration only if stereo processing is enabled
				if (i == 0) {
					m_pathL = m_path;
					if (stereo) {
						bins = m_processor->m_normSpectrumR.data();
						m_binHeight = &m_binHeightR;
						m_binPeak = &m_binPeakR;
					} else {
						break;
					}
				} else {
					m_pathR = m_path;
				}
			}

			#ifdef DEBUG
				line_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() - line_time;
			#endif

			if (freeze) {m_freezeRequest = false;}
		}

		m_processor->m_dataAccess.unlock();

		// draw stored paths
		#ifdef DEBUG
			draw_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
		#endif
		if (stereo) {
			painter.fillPath(m_pathR, QBrush(m_controls->m_colorR));
			painter.fillPath(m_pathL, QBrush(m_controls->m_colorL));
		} else {
			painter.fillPath(m_pathL, QBrush(m_controls->m_colorMono));
		}
		#ifdef DEBUG
			draw_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() - draw_time;
		#endif
	} else {
		m_processor->m_dataAccess.unlock();
	}

	// draw cursor if it is within bounds
	if (m_cursor.x() > 0 && m_cursor.y() > 0 && m_cursor.x() < width() && m_cursor.y() < height()) {
		painter.setPen(QPen(m_controls->m_colorGrid.lighter(), 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
		painter.drawLine(m_cursor.x(), displayTop, m_cursor.x(), displayBottom);
		painter.drawLine(displayLeft, m_cursor.y(), displayRight, m_cursor.y());
		painter.setPen(QPen(m_controls->m_colorLabels.darker(), 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
		painter.drawText(displayRight -60, 5, 100, 16, Qt::AlignLeft, "Cursor");
		QString tmps;
		tmps = QString(std::string(std::to_string((int)m_processor->xPixelToFreq(m_cursor.x() - displayLeft, displayWidth)) + " Hz").c_str());
		painter.drawText(displayRight -60, 18, 100, 16, Qt::AlignLeft, tmps);
		if (m_controls->m_logYModel.value()) {
			tmps = QString(std::string(std::to_string(m_processor->yPixelToAmp(m_cursor.y(), displayBottom)).substr(0, 5) + " dB").c_str());
		} else {
			tmps = QString(std::string(std::to_string(m_processor->yPixelToAmp(m_cursor.y(), displayBottom))).substr(0, 5).c_str());
		}
		painter.drawText(displayRight -60, 30, 100, 16, Qt::AlignLeft, tmps);
	}

	// always draw the outline
	painter.setPen(QPen(m_controls->m_colorGrid, 2, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	painter.drawRoundedRect(displayLeft, 1, displayWidth, displayBottom, 2.0, 2.0);

	#ifdef DEBUG
		start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() - start_time;
		painter.setPen(QPen(m_controls->m_colorLabels, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
		painter.drawText(displayRight -100, 70, 100, 16, Qt::AlignLeft, QString(std::string("Max FPS: " + std::to_string(1000000000.0 / start_time)).c_str()));
		painter.drawText(displayRight -100, 90, 100, 16, Qt::AlignLeft, QString(std::string("Lines ms: " + std::to_string(line_time / 1000000.0)).c_str()));
		painter.drawText(displayRight -100, 110, 100, 16, Qt::AlignLeft, QString(std::string("Draw ms: " + std::to_string(draw_time / 1000000.0)).c_str()));
	#endif
}


float SaSpectrumView::binToFreq(int index) {
	return m_processor->binToFreq(index);
}


float SaSpectrumView::freqToXPixel(float freq, int width) {
	return m_processor->freqToXPixel(freq, width);
}


float SaSpectrumView::xPixelToFreq(float x, int width) {
	return m_processor->xPixelToFreq(x, width);
}


float SaSpectrumView::ampToYPixel(float amplitude, int height) {
	return m_processor->ampToYPixel(amplitude, height);
}


std::vector<std::pair<int, std::string>> SaSpectrumView::makeLogTics(int low, int high) {
	std::vector<std::pair<int, std::string>> result;
	int i, j;
	int a[] = {10, 20, 50};
	int b[] = {14, 30, 70};

	// generate 1-2-5 (+ optional 3-7-14) series
	for (i = 1; i <= high; i *= 10){
		for (j = 0; j < 3; j++) {
			if (i * a[j] >= low && i * a[j] <= high){
				if (i * a[j] < 1000){
					result.push_back(std::pair<int, std::string>(i * a[j], std::to_string(i * a[j])));
				} else {
					result.push_back(std::pair<int, std::string>(i * a[j], std::to_string(i * a[j] / 1000) + "k"));
				}
			}
			if ((log10(high) - log10(low) < 2) && (i * b[j] >= low && i * b[j] <= high)){
				if (i * b[j] < 1500){
					result.push_back(std::pair<int, std::string>(i * b[j], std::to_string(i * b[j])));
				} else {
					result.push_back(std::pair<int, std::string>(i * b[j], std::to_string(i * b[j] / 1000) + "k"));
				}
			}
		}
	}

	return result;
}


std::vector<std::pair<int, std::string>> SaSpectrumView::makeLinearTics(int low, int high) {
	std::vector<std::pair<int, std::string>> result;
	int i, increment;

	if (high - low < 500) {increment = 50;}
	else if (high - low < 1000) {increment = 100;}
	else if (high - low < 5000) {increment = 1000;}
	else {increment = 2000;}

	// generate 2k-4k-6k series
	for (i = 0; i <= high; i += increment){
		if (i >= low){
			if (i < 1000){
				result.push_back(std::pair<int, std::string>(i, std::to_string(i)));
			} else {
				result.push_back(std::pair<int, std::string>(i, std::to_string(i/1000) + "k"));
			}
		}
	}

	return result;
}


std::vector<std::pair<float, std::string>> SaSpectrumView::makeDBTics(int low, int high) {
	std::vector<std::pair<float, std::string>> result;
	float i;

	// generate 10 dB increments
	for (i = 0.00000001; 10 * log10(i) <= high; i *= 10){
		if (10 * log10(i) >= low){
			result.push_back(std::pair<float, std::string>(i, std::to_string((int)std::round(10 * log10(i)))));
		}
	}

	return result;
}


std::vector<std::pair<float, std::string>> SaSpectrumView::makeAmpTics(int low, int high) {
	std::vector<std::pair<float, std::string>> result;
	float i, nearest;

	float lin_low = pow(10, low / 10.0);
	float lin_high = pow(10, high / 10.0);

	// generate linear amplitude increments
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


void SaSpectrumView::periodicUpdate() {
	m_periodicUpdate = true;				//FIXME: visibilitu by mel processor hodnotit i pro waterfall
	m_processor->setActive(isVisible());
	update();
}


void SaSpectrumView::mouseMoveEvent(QMouseEvent *event) {
	m_cursor = event->pos();
}


void SaSpectrumView::mousePressEvent(QMouseEvent *event) {
	m_cursor = event->pos();
}
