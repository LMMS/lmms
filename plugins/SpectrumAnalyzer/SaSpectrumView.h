/* SaSpectrumView.h - declaration of SaSpectrumView class.
*
* Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
* Copyright (c) 2019 Martin Pavelek <he29/dot/HS/at/gmail/dot/com>
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
#include <QPainter>
#include <QWidget>
#include <QString>

#include "SaControls.h"
#include "SaProcessor.h"

#include "fft_helpers.h"
#include "lmms_basics.h"
#include "lmms_math.h"


class SaSpectrumView : public QWidget
{
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

private slots:
	void periodicUpdate();

private:
	SaControls *m_controls;
	SaProcessor *m_processor;

	QColor m_colorL;
	QColor m_colorR;
	QColor m_colorMono;
	QColor m_colorBG;
	QColor m_colorGrid;
	QColor m_colorLabels;

	QPainterPath m_pathL;
	QPainterPath m_pathR;

	std::vector<std::pair<int, std::string>> m_logFreqTics;		// 10-20-50... Hz
	std::vector<std::pair<int, std::string>> m_linearFreqTics;	// 2k-4k-6k... Hz
	std::vector<std::pair<float, std::string>> m_logAmpTics;	// dB
	std::vector<std::pair<float, std::string>> m_linearAmpTics;	// 0..1

	std::vector<float> m_bandHeightL;
	std::vector<float> m_bandHeightR;
	std::vector<float> m_bandPeakL;
	std::vector<float> m_bandPeakR;

	float m_decaySum;
	bool m_periodicUpdate;
	bool m_freezeRequest;

	float freqToXPixel(float frequency, int width);
	float ampToYPixel(float amplitude, int height);
	float bandToFreq(int index);
};
#endif // SASPECTRUMVIEW_H
