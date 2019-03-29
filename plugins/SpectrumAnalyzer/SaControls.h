/*
 * SaControls.h - declaration of SaControls class.
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

#ifndef SACONTROLS_H
#define SACONTROLS_H

#include <QPainter>

#include "ComboBoxModel.h"
#include "EffectControls.h"

#define DEBUG 1


class Analyzer;

// FIXME: move this somewhere appropriate
const int LOWEST_LOG_FREQ = 10;	// arbitrary low frequency limit for log. scale (Hz, >1)
const int LOWEST_LOG_AMP = -5;	// arbitrary low amplitude limit for log. scale (10*dB)

const int WATERFALL_HEIGHT = 256;

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

enum AMPLITUDE_RANGES
{
	ARANGE_EXTENDED = 0,
	ARANGE_STANDARD,
	ARANGE_LOUD,
	ARANGE_SILENT
};

const int ARANGE_EXTENDED_START = -80;
const int ARANGE_EXTENDED_END = 10;
const int ARANGE_STANDARD_START = -50;
const int ARANGE_STANDARD_END = 0;
const int ARANGE_LOUD_START = -30;
const int ARANGE_LOUD_END = 5;
const int ARANGE_SILENT_START = -60;
const int ARANGE_SILENT_END = -20;

class SaControls : public EffectControls
{
	Q_OBJECT
public:
	explicit SaControls(Analyzer* effect);
	virtual ~SaControls() {}

	virtual void saveSettings (QDomDocument& doc, QDomElement& parent);
	virtual void loadSettings (const QDomElement &_this);

	inline virtual QString nodeName() const	{return "Analyzer";}
	virtual int controlCount() {return 7;}

	virtual EffectControlDialog* createView();

private:

	Analyzer *m_effect;

	BoolModel m_stereoModel;
	BoolModel m_smoothModel;
	BoolModel m_waterfallModel;

	BoolModel m_logXModel;
	BoolModel m_logYModel;

	BoolModel m_peakHoldModel;
	BoolModel m_pauseModel;
	BoolModel m_refFreezeModel;

	ComboBoxModel m_blockSizeModel;
	ComboBoxModel m_windowModel;
	ComboBoxModel m_freqRangeModel;
	ComboBoxModel m_ampRangeModel;

	QColor	m_colorL;
	QColor	m_colorR;
	QColor	m_colorMono;
	QColor	m_colorBG;
	QColor	m_colorGrid;
	QColor	m_colorLabels;

	bool m_inProgress;

	friend class SaControlsDialog;
	friend class SaSpectrumView;
	friend class SaWaterfallView;
	friend class SaProcessor;
};
#endif // SACONTROLS_H
