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

	m_bandHeightL.resize(m_processor->binCount(), 0);
	m_bandHeightR.resize(m_processor->binCount(), 0);
	m_bandPeakL.resize(m_processor->binCount(), 0);
	m_bandPeakR.resize(m_processor->binCount(), 0);

	m_logFreqTics = makeLogTics(LOWEST_FREQ, 20000);
	m_linearFreqTics = makeLinearTics(0, 20000);
	m_logAmpTics = makeDBTics(-50, 0);
	m_linearAmpTics = makeAmpTics(0, 1);
}


void SaSpectrumView::paintEvent(QPaintEvent *event)
{
	#ifdef DEBUG
		int start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
		int line_time, draw_time;
	#endif

	const float smoothFactor = 0.15;

	const bool stereo = m_controls->m_stereoModel.value();
	const bool freeze = m_freezeRequest;

	const int displayBottom = height() -20;
	const int displayLeft = 20;
	const int displayRight = width() -20;
	const int displayWidth = displayRight - displayLeft;

	float pos = 0;
	std::vector<std::pair<int, std::string>> * freqTics = NULL;
	std::vector<std::pair<float, std::string>> * ampTics = NULL;

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);

	// always draw the background
	painter.fillRect(displayLeft, 1, displayWidth, displayBottom, m_controls->m_colorBG);

	// select logarithmic or linear frequency grid and draw it
	if (m_controls->m_logXModel.value()) {
		freqTics = &m_logFreqTics;
	} else {
		freqTics = &m_linearFreqTics;
	}

	painter.setPen(QPen(m_controls->m_colorGrid, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	for (auto & line: *freqTics){
		painter.drawLine(	displayLeft + freqToXPixel(line.first, displayWidth), 2,
							displayLeft + freqToXPixel(line.first, displayWidth), displayBottom);
	}

	painter.setPen(QPen(m_controls->m_colorLabels, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	for (auto & line: *freqTics) {
		pos = displayLeft + freqToXPixel(line.first, displayWidth);
		// make an exception if the first or last label is too close to edge
		if (line == freqTics->front() && pos - 10 < displayLeft) {
			painter.drawText(displayLeft, displayBottom + 5, 20, 15, Qt::AlignLeft, QString(line.second.c_str()));
		} else if (line == freqTics->back() && pos + 10 > displayRight) {
			painter.drawText(displayRight - 20, displayBottom + 5, 20, 15, Qt::AlignRight, QString(line.second.c_str()));
		} else {
			painter.drawText(pos - 10, displayBottom + 5, 20, 15, Qt::AlignHCenter, QString(line.second.c_str()));
		}
	}

	// select logarithmic or linear amplitude grid and draw it
	if (m_controls->m_logYModel.value()) {
		ampTics = &m_logAmpTics;
	} else {
		ampTics = &m_linearAmpTics;
	}

	painter.setPen(QPen(m_controls->m_colorGrid, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	for (auto & line: *ampTics){
		painter.drawLine(	displayLeft + 1, ampToYPixel(line.first, displayBottom),
							displayRight - 1, ampToYPixel(line.first, displayBottom));
	}

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
			painter.drawText(2, displayBottom - 16, 16, 16, Qt::AlignRight | Qt::AlignBottom, QString(line.second.c_str()));
			if (stereo) {painter.setPen(QPen(m_controls->m_colorR.lighter(), 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));}
			painter.drawText(displayRight + 2, displayBottom - 16, 16, 16, Qt::AlignLeft | Qt::AlignBottom, QString(line.second.c_str()));
		} else {
			if (stereo) {painter.setPen(QPen(m_controls->m_colorL.lighter(), 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));}
			painter.drawText(2, pos - 8, 16, 16, Qt::AlignRight | Qt::AlignVCenter, QString(line.second.c_str()));
			if (stereo) {painter.setPen(QPen(m_controls->m_colorR.lighter(), 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));}
			painter.drawText(displayRight + 2, pos - 8, 16, 16, Qt::AlignLeft | Qt::AlignVCenter, QString(line.second.c_str()));
		}
	}

	// draw the graph only if there is any input or smooth decay / averaging residue
	m_processor->m_dataAccess.lock();
	if (m_decaySum > 0 || notEmpty(m_processor->m_normSpectrumL) || notEmpty(m_processor->m_normSpectrumR)) {

		// update paths with new data if needed
		if (!m_processor->getInProgress() && m_periodicUpdate == true && !m_controls->m_pauseModel.value()) {

			if (m_processor->binCount() != m_bandHeightL.size()) {
				m_bandHeightL.clear();
				m_bandHeightR.clear();
				m_bandPeakL.clear();
				m_bandPeakR.clear();
				m_bandHeightL.resize(m_processor->binCount(), 0);
				m_bandHeightR.resize(m_processor->binCount(), 0);
				m_bandPeakL.resize(m_processor->binCount(), 0);
				m_bandPeakR.resize(m_processor->binCount(), 0);
			}

			m_periodicUpdate = false;
			m_decaySum = 0;
		
			float *bands = m_processor->m_normSpectrumL.data();
			std::vector<float> *m_bandHeight = &m_bandHeightL;
			std::vector<float> *m_bandPeak = &m_bandPeakL;
			QPainterPath m_path;

			#ifdef DEBUG
				line_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
			#endif

			for (int i = 0; i <= 1; i++){
				m_path = QPainterPath();
				m_path.moveTo(displayLeft, displayBottom);

				// the first band is stretched to the left edge to prevent
				// creating a misleading slope leading to zero (at log. scale)
				m_path.lineTo(	displayLeft,
								ampToYPixel((*m_bandHeight)[0], displayBottom));

				for (int x = 0; x < m_processor->binCount(); x++) {
					// direct display
					if (m_controls->m_smoothModel.value()) {
						(*m_bandHeight)[x] = bands[x] * smoothFactor + (*m_bandHeight)[x] * (1 - smoothFactor);
//						(*m_bandHeight)[x] = (*m_bandHeight)[x] / 1.2;
					} else {
						(*m_bandHeight)[x] = bands[x];
					}

					if (freqToXPixel(bandToFreq(x), displayWidth) >= 0) {
						m_path.lineTo(	freqToXPixel(bandToFreq(x), displayWidth) + displayLeft,
										ampToYPixel((*m_bandHeight)[x], displayBottom));
						m_decaySum += (*m_bandHeight)[x];
					}

					// peak-hold and reference freeze (using the same curve
					// to save resources and keep screen clean and readable)
					if (m_controls->m_refFreezeModel.value() && freeze) {
						(*m_bandPeak)[x] = bands[x];
					} else if (m_controls->m_peakHoldModel.value()) {
						if (bands[x] > (*m_bandPeak)[x]) {
							(*m_bandPeak)[x] = bands[x];
						}
					}
				}

				m_path.lineTo(displayRight, displayBottom);
				m_path.closeSubpath();

				// go for second iteration only if stereo processing is enabled
				if (i == 0) {
					m_pathL = m_path;
					if (stereo) {
						bands = m_processor->m_normSpectrumR.data();
						m_bandHeight = &m_bandHeightR;
						m_bandPeak = &m_bandPeakR;
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

	// always draw the outline
	painter.setPen(QPen(m_controls->m_colorGrid, 2, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	painter.drawRoundedRect(displayLeft, 1, displayWidth, displayBottom, 2.0, 2.0);

	#ifdef DEBUG
		start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() - start_time;
		painter.setPen(QPen(m_controls->m_colorLabels, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
		painter.drawText(displayRight -100, 10, 100, 16, Qt::AlignLeft, QString(std::string("Max FPS: " + std::to_string(1000000000.0 / start_time)).c_str()));
		painter.drawText(displayRight -100, 30, 100, 16, Qt::AlignLeft, QString(std::string("Lines ms: " + std::to_string(line_time / 1000000.0)).c_str()));
		painter.drawText(displayRight -100, 50, 100, 16, Qt::AlignLeft, QString(std::string("Draw ms: " + std::to_string(draw_time / 1000000.0)).c_str()));
	#endif
}


float SaSpectrumView::bandToFreq(int index)
{
	return index * m_processor->getSampleRate() / (m_processor->binCount() * 2);
}


float SaSpectrumView::freqToXPixel(float freq, int width)
{
	if (m_controls->m_logXModel.value()){
		float min = log10f(LOWEST_FREQ);
		float max = log10f(m_processor->getSampleRate() / 2);
		float range = max - min;
		return (log10f(freq) - min) / range * width;
	} else {
		float min = LOWEST_FREQ;
		float max = m_processor->getSampleRate() / 2;
		float range = max - min;
		return (freq - min) / range * width;
	}
}


float SaSpectrumView::ampToYPixel(float amplitude, int height)
{
	if (m_controls->m_logYModel.value()){
		if (log10f(amplitude) < LOWEST_AMP){
			return height;
		} else {
			return height * log10f(amplitude) / LOWEST_AMP;
		}
	} else {
		return height - height * amplitude;
	}
}


std::vector<std::pair<int, std::string>> SaSpectrumView::makeLogTics(int low, int high)
{
	std::vector<std::pair<int, std::string>> result;
	int i;

	// generate 1-2-5 series
	for (i = 1; i <= high; i *= 10){
		if (i*1 >= low){
			if (i < 1000){
				result.push_back(std::pair<int, std::string>(i*1, std::to_string(i*1)));
			} else {
				result.push_back(std::pair<int, std::string>(i*1, std::to_string(i*1/1000) + "k"));
			}
		}
		if (i*2 >= low){
			if (i*2 > high) break;
			if (i < 1000){
				result.push_back(std::pair<int, std::string>(i*2, std::to_string(i*2)));
			} else {
				result.push_back(std::pair<int, std::string>(i*2, std::to_string(i*2/1000) + "k"));
			}
		}
		if (i*5 >= low){
			if (i*5 > high) break;
			if (i < 1000){
				result.push_back(std::pair<int, std::string>(i*5, std::to_string(i*5)));
			} else {
				result.push_back(std::pair<int, std::string>(i*5, std::to_string(i*5/1000) + "k"));
			}
		}
	}

	return result;
}


std::vector<std::pair<int, std::string>> SaSpectrumView::makeLinearTics(int low, int high)
{
	std::vector<std::pair<int, std::string>> result;
	int i;

	// generate 2k-4k-6k series
	for (i = 0; i <= high; i += 2000){
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


std::vector<std::pair<float, std::string>> SaSpectrumView::makeDBTics(int low, int high)
{
	std::vector<std::pair<float, std::string>> result;
	float i;

	// generate 10 dB increments
	for (i = 0.00000001; 10 * log10f(i) <= high; i *= 10){
		if (10 * log10f(i) >= low){
			result.push_back(std::pair<float, std::string>(i, std::to_string((int)(10*log10f(i)))));
		}
	}

	return result;
}


std::vector<std::pair<float, std::string>> SaSpectrumView::makeAmpTics(int low, int high)
{
	std::vector<std::pair<float, std::string>> result;
	float i;

	// generate 0.2 linear amplitude increments
	for (i = 0; i <= high; i += 0.2){
		if (i >= low){
			result.push_back(std::pair<float, std::string>(i, std::to_string(i).substr(0, 3)));
		}
	}

	return result;
}

void SaSpectrumView::periodicUpdate()
{
	m_periodicUpdate = true;				//FIXME: visibilitu by mel processor hodnotit i pro waterfall
	m_processor->setActive(isVisible());
	update();
}
