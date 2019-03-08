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
#include "SaProcessor.h"

#include "Engine.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "Mixer.h"

SaSpectrumView::SaSpectrumView(SaControls *controls, SaProcessor *processor, QWidget *_parent) :
	QWidget(_parent),
	m_controls(controls),
	m_processor(processor),
	m_periodicalUpdate(false)
{
	setMinimumSize(300, 150);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	connect(gui->mainWindow(), SIGNAL(periodicUpdate()), this, SLOT(periodicalUpdate()));
	setAttribute(Qt::WA_TranslucentBackground, true);

	m_colorMono = QColor(255, 255, 255, 255);	// (set in SaControlsDialog)
	m_colorL = QColor(255, 255, 255, 255);
	m_colorR = QColor(255, 255, 255, 255);
	m_colorBG = QColor(7, 7, 7, 255);			// 20 % gray
	m_colorGrid = QColor(55, 55, 55, 255);		// 50 % gray
	m_colorLabels = QColor(202, 202, 202, 255);	// 90 % gray

	for (int i = 0; i < MAX_BANDS; i++) {
		m_bandHeight.append(0);				// FIXME: co ta promenna dela?
	}

	m_logTics = makeLogTics(LOWEST_FREQ, 20000);
	m_linearTics = makeLinearTics(0, 20000);
}


void SaSpectrumView::paintEvent(QPaintEvent *event)
{
	const float energyL = m_processor->getEnergyL();
	const float energyR = m_processor->getEnergyR();
	const bool stereo = m_controls->m_stereoModel.value();
	const int displayBottom = height() -20;
	const int HIGHEST_FREQ = m_processor->getSampleRate() / 2;
	const int LOWER_Y = -36;	// dB
	const float fallOff = 1.07;
	float pos = 0;
	std::vector<std::pair<int, std::string>> * freqTics = NULL;

	QPainter painter(this);

	painter.setRenderHint(QPainter::Antialiasing, true);

	// always draw the background
	painter.fillRect(0, 0, width(), displayBottom, m_colorBG);

	// select logarithmic or linear grid and draw it
	if (m_controls->m_logXModel.value()) {
		freqTics = &m_logTics;
	} else {
		freqTics = &m_linearTics;
	}

	painter.setPen(QPen(m_colorGrid, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	for (auto & line: *freqTics){
		painter.drawLine(freqToXPixel(line.first, width()), 0, freqToXPixel(line.first, width()), displayBottom);
	}

	painter.setPen(QPen(m_colorLabels, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	for (auto & line: *freqTics) {
		pos = freqToXPixel(line.first, width());
		// make an exception if the first or last label is too close to edge to prevent clipping
		if (line == freqTics->front() && pos < 10) {
			painter.drawText(0, displayBottom + 5, 20, 15, Qt::AlignLeft, QString(line.second.c_str()));
		} else if (line == freqTics->back() && pos + 10 > width()) {
			painter.drawText(width() - 20, displayBottom + 5, 20, 15, Qt::AlignRight, QString(line.second.c_str()));
		} else {
			painter.drawText(pos - 10, displayBottom + 5, 20, 15, Qt::AlignHCenter, QString(line.second.c_str()));
		}
	}

	// draw the graph only if there is any input or smooth decay residue
	if (energyL > 0 || energyR > 0 || m_decaySum > 0) {		

		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.setPen(QPen(m_colorMono, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	
		// update paths with new data if needed
		if (!m_processor->getInProgress() && m_periodicalUpdate == true) {
			m_periodicalUpdate = false;
			m_decaySum = 0;
		
			float peak;
			float *bands = m_processor->m_bandsL;
			float energy = energyL;
			QPainterPath m_path;
		
			for (int i = 0; i <= 1; i++){
				m_path = QPainterPath();
				m_path.moveTo(0, displayBottom);
		
				for (int x = 0; x < MAX_BANDS; x++, bands++) {
					peak = (displayBottom * 2.0 / 3.0 * (20 * (log10(*bands / energy)) - LOWER_Y) / (-LOWER_Y));
					if (peak < 0) {
						peak = 0;
					}
					else if (peak >= displayBottom) {
						continue;
					}
		
					if (peak > m_bandHeight[x] || false /*!m_mode_smooth*/) {	//FIXME make possible to turn off slow decay?
						m_bandHeight[x] = peak;
					} else {
						m_bandHeight[x] = m_bandHeight[x] / fallOff;
					}
		
					if (m_bandHeight[x] < 0) {
						m_bandHeight[x] = 0;
					}
		
					m_path.lineTo(freqToXPixel(bandToFreq(x), width()), displayBottom - m_bandHeight[x]);
					m_decaySum += m_bandHeight[x];
				}
		
				m_path.lineTo(width(), displayBottom);
				m_path.closeSubpath();
		
				// go for second iteration only if stereo processing is enabled
				if (i == 0) {
					m_pathL = m_path;
					if (stereo) {
						bands = m_processor->m_bandsR;
						energy = energyR;
					} else {
						break;
					}
				} else {
					m_pathR = m_path;
				}
			}
		}
	
		// draw computed paths
		if (stereo) {
			painter.fillPath(m_pathR, QBrush(m_colorR));
			painter.drawPath(m_pathR);
			painter.fillPath(m_pathL, QBrush(m_colorL));
			painter.drawPath(m_pathL);
		} else {
			painter.fillPath(m_pathL, QBrush(m_colorMono));
			painter.drawPath(m_pathL);
		}
	}

	// always draw the outline
	painter.setPen(QPen(m_colorGrid, 2, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin));
	painter.drawRoundedRect(0, 0, width(), displayBottom, 4.0, 4.0);

}


QColor SaSpectrumView::getColor() const
{
	return m_colorMono;
}


void SaSpectrumView::setColors(const QColor &mono, const QColor &left, const QColor &right)
{
	m_colorMono = mono;
	m_colorL = left;
	m_colorR = right;
}


float SaSpectrumView::bandToFreq(int index)
{
	return index * m_processor->getSampleRate() / (MAX_BANDS * 2);
}


float SaSpectrumView::freqToXPixel(float freq, int w)
{
	if (m_controls->m_logXModel.value()){
		float min = log10f(LOWEST_FREQ);
		float max = log10f(m_processor->getSampleRate() / 2);
		float range = max - min;
		return (log10f(freq) - min) / range * w;
	} else {
		float min = LOWEST_FREQ;
		float max = m_processor->getSampleRate() / 2;
		float range = max - min;
		return (freq - min) / range * w;
	}
}


float SaSpectrumView::gainToYPixel(float gain, int h, float pixelPerUnitHeight)	//FIXME not gain
{
	return h * 0.5 - gain * pixelPerUnitHeight;
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


void SaSpectrumView::periodicalUpdate()
{
	m_periodicalUpdate = true;
	m_processor->setActive(isVisible());
	update();
}
