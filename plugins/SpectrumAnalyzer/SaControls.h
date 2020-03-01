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
#include "lmms_constants.h"

//#define SA_DEBUG 1	// define SA_DEBUG to enable performance measurements


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
	int controlCount() override {return 20;}

private:
	Analyzer *m_effect;

	// basic settings
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

	// advanced settings
	FloatModel m_envelopeResolutionModel;
	FloatModel m_spectrumResolutionModel;
	FloatModel m_peakDecayFactorModel;
	FloatModel m_averagingWeightModel;
	FloatModel m_waterfallHeightModel;
	FloatModel m_waterfallGammaModel;
	FloatModel m_windowOverlapModel;
	FloatModel m_zeroPaddingModel;

	// colors (hard-coded, values must add up to specific numbers)
	QColor m_colorL;		//!< color of the left channel
	QColor m_colorR;		//!< color of the right channel
	QColor m_colorMono;		//!< mono color for spectrum display
	QColor m_colorMonoW;	//!< mono color for waterfall display
	QColor m_colorBG;		//!< spectrum display background color
	QColor m_colorGrid;		//!< color of grid lines
	QColor m_colorLabels;	//!< color of axis labels

	friend class SaControlsDialog;
	friend class SaSpectrumView;
	friend class SaWaterfallView;
	friend class SaProcessor;
};
#endif // SACONTROLS_H
