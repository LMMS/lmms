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


class Analyzer;

// FIXME: move this somewhere appropriate
const int LOWEST_FREQ = 10;		// arbitrary low frequency limit for log. scale (Hz, >0)
const int LOWEST_AMP = -5;		// arbitrary low amplitude limit for log. scale (10*dB)
const int RANGE_AUDIBLE_START = 20;
const int RANGE_AUDIBLE_END = 20000;
const int RANGE_BASS_START = 20;
const int RANGE_BASS_END = 300;
const int RANGE_MID_START = 200;
const int RANGE_MID_SEND = 5000;
const int RANGE_HIGH_START = 4000;
const int RANGE_HIGH_END = 20000;
const int WATERFALL_HEIGHT = 256;

#define DEBUG 1

class SaControls : public EffectControls
{
	Q_OBJECT
public:
	explicit SaControls(Analyzer* effect);
	virtual ~SaControls()
	{
	}

	virtual void saveSettings (QDomDocument& doc, QDomElement& parent);
	virtual void loadSettings (const QDomElement &_this);

	inline virtual QString nodeName() const
	{
		return "Analyzer";
	}

	virtual int controlCount()
	{
		return 7;
	}

	virtual EffectControlDialog* createView();

	bool m_inProgress;

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
