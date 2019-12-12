/*
 * SaControls.cpp - definition of SaControls class.
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

#include "SaControls.h"

#include <QtXml/QDomElement>

#include "Analyzer.h"
#include "SaControlsDialog.h"


SaControls::SaControls(Analyzer *effect) :
	EffectControls(effect),
	m_effect(effect),

	// initialize bool models and set default values
	m_pauseModel(false, this, tr("Pause")),
	m_refFreezeModel(false, this, tr("Reference freeze")),

	m_waterfallModel(false, this, tr("Waterfall")),
	m_smoothModel(false, this, tr("Averaging")),
	m_stereoModel(false, this, tr("Stereo")),
	m_peakHoldModel(false, this, tr("Peak hold")),

	m_logXModel(true, this, tr("Logarithmic frequency")),
	m_logYModel(true, this, tr("Logarithmic amplitude")),

	// default values of combo boxes are set after they are populated
	m_freqRangeModel(this, tr("Frequency range")),
	m_ampRangeModel(this, tr("Amplitude range")),
	m_blockSizeModel(this, tr("FFT block size")),
	m_windowModel(this, tr("FFT window type"))
{
	// Frequency and amplitude ranges; order must match
	// FREQUENCY_RANGES and AMPLITUDE_RANGES defined in SaControls.h
	m_freqRangeModel.addItem(tr("Full (auto)"));
	m_freqRangeModel.addItem(tr("Audible"));
	m_freqRangeModel.addItem(tr("Bass"));
	m_freqRangeModel.addItem(tr("Mids"));
	m_freqRangeModel.addItem(tr("High"));
	m_freqRangeModel.setValue(m_freqRangeModel.findText(tr("Full (auto)")));

	m_ampRangeModel.addItem(tr("Extended"));
	m_ampRangeModel.addItem(tr("Default"));
	m_ampRangeModel.addItem(tr("Audible"));
	m_ampRangeModel.addItem(tr("Noise"));
	m_ampRangeModel.setValue(m_ampRangeModel.findText(tr("Default")));

	// FFT block size labels are generated automatically, based on
	// FFT_BLOCK_SIZES vector defined in fft_helpers.h
	for (unsigned int i = 0; i < FFT_BLOCK_SIZES.size(); i++)
	{
		if (i == 0)
		{
			m_blockSizeModel.addItem((std::to_string(FFT_BLOCK_SIZES[i]) + " ").c_str() + tr("(High time res.)"));
		}
		else if (i == FFT_BLOCK_SIZES.size() - 1)
		{
			m_blockSizeModel.addItem((std::to_string(FFT_BLOCK_SIZES[i]) + " ").c_str() + tr("(High freq. res.)"));
		}
		else
		{
			m_blockSizeModel.addItem(std::to_string(FFT_BLOCK_SIZES[i]).c_str());
		}
	}
	m_blockSizeModel.setValue(m_blockSizeModel.findText("2048"));

	// Window type order must match FFT_WINDOWS defined in fft_helpers.h
	m_windowModel.addItem(tr("Rectangular (Off)"));
	m_windowModel.addItem(tr("Blackman-Harris (Default)"));
	m_windowModel.addItem(tr("Hamming"));
	m_windowModel.addItem(tr("Hanning"));
	m_windowModel.setValue(m_windowModel.findText(tr("Blackman-Harris (Default)")));

	// Colors
	// Background color is defined by Qt / theme.
	// Make sure the sum of colors for L and R channel stays lower or equal
	// to 255. Otherwise the Waterfall pixels may overflow back to 0 even when
	// the input signal isn't clipping (over 1.0).
	m_colorL = QColor(51, 148, 204, 135);
	m_colorR = QColor(204, 107, 51, 135);
	m_colorMono = QColor(51, 148, 204, 204);
	m_colorBG = QColor(7, 7, 7, 255);			// ~20 % gray (after gamma correction)
	m_colorGrid = QColor(30, 34, 38, 255);		// ~40 % gray (slightly cold / blue)
	m_colorLabels = QColor(192, 202, 212, 255);	// ~90 % gray (slightly cold / blue)
}


// Create the SaControlDialog widget which handles display of GUI elements.
EffectControlDialog* SaControls::createView()
{
	return new SaControlsDialog(this, m_effect->getProcessor());
}


void SaControls::loadSettings(const QDomElement &_this)
{
	m_waterfallModel.loadSettings(_this, "Waterfall");
	m_smoothModel.loadSettings(_this, "Smooth");
	m_stereoModel.loadSettings(_this, "Stereo");
	m_peakHoldModel.loadSettings(_this, "PeakHold");
	m_logXModel.loadSettings(_this, "LogX");
	m_logYModel.loadSettings(_this, "LogY");
	m_freqRangeModel.loadSettings(_this, "RangeX");
	m_ampRangeModel.loadSettings(_this, "RangeY");
	m_blockSizeModel.loadSettings(_this, "BlockSize");
	m_windowModel.loadSettings(_this, "WindowType");
}


void SaControls::saveSettings(QDomDocument &doc, QDomElement &parent)
{
	m_waterfallModel.saveSettings(doc, parent, "Waterfall");
	m_smoothModel.saveSettings(doc, parent, "Smooth");
	m_stereoModel.saveSettings(doc, parent, "Stereo");
	m_peakHoldModel.saveSettings(doc, parent, "PeakHold");
	m_logXModel.saveSettings(doc, parent, "LogX");
	m_logYModel.saveSettings(doc, parent, "LogY");
	m_freqRangeModel.saveSettings(doc, parent, "RangeX");
	m_ampRangeModel.saveSettings(doc, parent, "RangeY");
	m_blockSizeModel.saveSettings(doc, parent, "BlockSize");
	m_windowModel.saveSettings(doc, parent, "WindowType");
}
