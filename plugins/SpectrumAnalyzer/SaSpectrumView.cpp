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

#include "Engine.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "Mixer.h"
#include "SaProcessor.h"

SaSpectrumView::SaSpectrumView(SaControls *controls, SaProcessor *processor, QWidget *_parent) :
	QWidget(_parent),
	m_controls(controls),
	m_processor(processor),
	m_periodicalUpdate(false),
	m_freezeRequest(false)
{
	setMinimumSize(400, 200);

	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	connect(gui->mainWindow(), SIGNAL(periodicUpdate()), this, SLOT(periodicalUpdate()));

	for (int i = 0; i < MAX_BANDS; i++) {
		m_bandHeightL.append(0);
		m_bandHeightR.append(0);
		m_bandPeakL.append(0);
		m_bandPeakR.append(0);
	}

	m_logFreqTics = makeLogTics(LOWEST_FREQ, 20000);
	m_linearFreqTics = makeLinearTics(0, 20000);
	m_logAmpTics = makeDBTics(-50, 0);
	m_linearAmpTics = makeAmpTics(0, 1);
}


void SaSpectrumView::paintEvent(QPaintEvent *event)
{
	const int HIGHEST_FREQ = m_processor->getSampleRate() / 2;
	const float smoothFactor = 0.15;

	const float energyL = isnan(m_processor->getEnergyL()) ? 0 : m_processor->getEnergyL();
	const float energyR = isnan(m_processor->getEnergyR()) ? 0 : m_processor->getEnergyR();
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
	painter.setRenderHint(QPainter::Antialiasing, false);

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

	// draw the graph only if there is any input or smooth decay residue
	if (energyL > 0 || energyR > 0 || m_decaySum > 0) {

		// update paths with new data if needed
		if (!m_processor->getInProgress() && m_periodicalUpdate == true && !m_controls->m_pauseModel.value()) {
			m_periodicalUpdate = false;
			m_decaySum = 0;
		
			float *band = m_processor->m_normBandsL;
			QList<float> *m_bandHeight = &m_bandHeightL;
			QList<float> *m_bandPeak = &m_bandPeakL;
			float energy = energyL;
			QPainterPath m_path;
		
			for (int i = 0; i <= 1; i++){
				m_path = QPainterPath();
				m_path.moveTo(displayLeft, displayBottom);
		
				for (int x = 0; x < MAX_BANDS; x++) {
					// direct display
					if (m_controls->m_smoothModel.value() && (*m_bandHeight)[x] > band[x]) {
						(*m_bandHeight)[x] = band[x] * smoothFactor + (*m_bandHeight)[x] * (1 - smoothFactor);
//						(*m_bandHeight)[x] = (*m_bandHeight)[x] / 1.2;
					} else {
						(*m_bandHeight)[x] = band[x];
					}
		
					m_path.lineTo(	freqToXPixel(bandToFreq(x), displayWidth) + displayLeft,
									ampToYPixel((*m_bandHeight)[x], displayBottom));
					m_decaySum += (*m_bandHeight)[x];

					// peak-hold and reference freeze (using the same curve
					// to save resources and keep screen clean and readable)
					if (m_controls->m_refFreezeModel.value() && freeze) {
						(*m_bandPeak)[x] = band[x]; 
					} else if (m_controls->m_peakHoldModel.value()) {
						if (band[x] > (*m_bandPeak)[x]) {
							(*m_bandPeak)[x] = band[x];
						}
					}
				}

				m_path.lineTo(displayRight, displayBottom);
				m_path.closeSubpath();
		
				// go for second iteration only if stereo processing is enabled
				if (i == 0) {
					m_pathL = m_path;
					if (stereo) {
						band = m_processor->m_normBandsR;
						m_bandHeight = &m_bandHeightR;
						m_bandPeak = &m_bandPeakR;
						energy = energyR;
					} else {
						break;
					}
				} else {
					m_pathR = m_path;
				}
			}
			if (freeze) {m_freezeRequest = false;}
		}
	
		// draw stored paths
		if (stereo) {
			painter.fillPath(m_pathR, QBrush(m_controls->m_colorR));
			painter.fillPath(m_pathL, QBrush(m_controls->m_colorL));
		} else {
			painter.fillPath(m_pathL, QBrush(m_controls->m_colorMono));
		}
	}

	// always draw the outline
	painter.setPen(QPen(m_controls->m_colorGrid, 2, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	painter.drawRoundedRect(displayLeft, 1, displayWidth, displayBottom, 2.0, 2.0);

}


float SaSpectrumView::bandToFreq(int index)
{
	return index * m_processor->getSampleRate() / (MAX_BANDS * 2);
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

void SaSpectrumView::periodicalUpdate()
{
	m_periodicalUpdate = true;
	m_processor->setActive(isVisible());
	update();
}
