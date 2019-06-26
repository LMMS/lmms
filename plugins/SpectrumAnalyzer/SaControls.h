/*
 * SaControls.h - declaration of SaControls class.
 *
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

#ifndef SACONTROLS_H
#define SACONTROLS_H

#include "ComboBoxModel.h"
#include "EffectControls.h"

//#define SA_DEBUG 1	// define SA_DEBUG to enable performance measurements

// Frequency ranges (in Hz).
// Full range is defined by LOWEST_LOG_FREQ and current sample rate.
const int LOWEST_LOG_FREQ = 10;		// arbitrary low limit for log. scale, >1

enum FREQUENCY_RANGES
{
	FRANGE_FULL = 0,
	FRANGE_AUDIBLE,
	FRANGE_BASS,
	FRANGE_MIDS,
	FRANGE_HIGH
};

const int FRANGE_AUDIBLE_START = 20;
const int FRANGE_AUDIBLE_END = 20000;
const int FRANGE_BASS_START = 20;
const int FRANGE_BASS_END = 300;
const int FRANGE_MIDS_START = 200;
const int FRANGE_MIDS_END = 5000;
const int FRANGE_HIGH_START = 4000;
const int FRANGE_HIGH_END = 20000;

// Amplitude ranges.
// Reference: sine wave from -1.0 to 1.0 = 0 dB.
// I.e. if master volume is 100 %, positive values signify clipping.
// Doubling or halving the amplitude produces 3 dB difference.
enum AMPLITUDE_RANGES
{
	ARANGE_EXTENDED = 0,
	ARANGE_DEFAULT,
	ARANGE_AUDIBLE,
	ARANGE_NOISE
};

const int ARANGE_EXTENDED_START = -80;
const int ARANGE_EXTENDED_END = 20;
const int ARANGE_DEFAULT_START = -30;
const int ARANGE_DEFAULT_END = 0;
const int ARANGE_AUDIBLE_START = -50;
const int ARANGE_AUDIBLE_END = 10;
const int ARANGE_NOISE_START = -60;
const int ARANGE_NOISE_END = -20;


class Analyzer;

// Holds all the configuration values
class SaControls : public EffectControls
{
	Q_OBJECT
public:
	explicit SaControls(Analyzer* effect);
	virtual ~SaControls() {}

	EffectControlDialog* createView() override;

	void saveSettings (QDomDocument& doc, QDomElement& parent) override;
	void loadSettings (const QDomElement &_this) override;

	QString nodeName() const override {return "Analyzer";}
	int controlCount() override {return 12;}

private:
	Analyzer *m_effect;

	BoolModel m_pauseModel;
	BoolModel m_refFreezeModel;

	BoolModel m_waterfallModel;
	BoolModel m_smoothModel;
	BoolModel m_stereoModel;
	BoolModel m_peakHoldModel;

	BoolModel m_logXModel;
	BoolModel m_logYModel;

	ComboBoxModel m_freqRangeModel;
	ComboBoxModel m_ampRangeModel;
	ComboBoxModel m_blockSizeModel;
	ComboBoxModel m_windowModel;

	QColor	m_colorL;
	QColor	m_colorR;
	QColor	m_colorMono;
	QColor	m_colorBG;
	QColor	m_colorGrid;
	QColor	m_colorLabels;

	friend class SaControlsDialog;
	friend class SaSpectrumView;
	friend class SaWaterfallView;
	friend class SaProcessor;
};
#endif // SACONTROLS_H
