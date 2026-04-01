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


#include "Analyzer.h"
#include "fft_helpers.h"
#include "SaControlsDialog.h"

namespace lmms
{


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
	m_windowModel(this, tr("FFT window type")),

	// Advanced settings knobs
	m_envelopeResolutionModel(0.25f, 0.1f, 3.0f, 0.05f, this, tr("Peak envelope resolution")),
	m_spectrumResolutionModel(1.5f, 0.1f, 3.0f, 0.05f, this, tr("Spectrum display resolution")),
	m_peakDecayFactorModel(0.992f, 0.95f, 0.999f, 0.001f, this, tr("Peak decay multiplier")),
	m_averagingWeightModel(0.15f, 0.01f, 0.5f, 0.01f, this, tr("Averaging weight")),
	m_waterfallHeightModel(300.0f, 50.0f, 1000.0f, 50.0f, this, tr("Waterfall history size")),
	m_waterfallGammaModel(0.30f, 0.10f, 1.00f, 0.05f, this, tr("Waterfall gamma correction")),
	m_windowOverlapModel(2.0f, 1.0f, 4.0f, 1.0f, this, tr("FFT window overlap")),
	m_zeroPaddingModel(2.0f, 0.0f, 4.0f, 1.0f, this, tr("FFT zero padding"))
{
	// Frequency and amplitude ranges; order must match
	// FrequencyRange and AmplitudeRange defined in SaControls.h
	m_freqRangeModel.addItem(tr("Full (auto)"));
	m_freqRangeModel.addItem(tr("Audible"));
	m_freqRangeModel.addItem(tr("Bass"));
	m_freqRangeModel.addItem(tr("Mids"));
	m_freqRangeModel.addItem(tr("High"));
	m_freqRangeModel.setValue(m_freqRangeModel.findText(tr("Full (auto)")));

	m_ampRangeModel.addItem(tr("Extended"));
	m_ampRangeModel.addItem(tr("Audible"));
	m_ampRangeModel.addItem(tr("Loud"));
	m_ampRangeModel.addItem(tr("Silent"));
	m_ampRangeModel.setValue(m_ampRangeModel.findText(tr("Audible")));

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

	// Window type order must match FFTWindow defined in fft_helpers.h
	m_windowModel.addItem(tr("Rectangular (Off)"));
	m_windowModel.addItem(tr("Blackman-Harris (Default)"));
	m_windowModel.addItem(tr("Hamming"));
	m_windowModel.addItem(tr("Hanning"));
	m_windowModel.setValue(m_windowModel.findText(tr("Blackman-Harris (Default)")));

	// Colors
	// Background color is defined by Qt / theme.
	// Make sure the sum of colors for L and R channel results into a neutral
	// color that has at least one component equal to 255 (i.e. ideally white).
	// This means the color overflows to zero exactly when signal reaches
	// clipping threshold, indicating the problematic frequency to user.
	// Mono waterfall color should have similarly at least one component at 255.
	m_colorL = QColor(51, 148, 204, 135);
	m_colorR = QColor(204, 107, 51, 135);
	m_colorMono = QColor(51, 148, 204, 204);
	m_colorMonoW = QColor(64, 185, 255, 255);
	m_colorBG = QColor(7, 7, 7, 255);			// ~20 % gray (after gamma correction)
	m_colorGrid = QColor(30, 34, 38, 255);		// ~40 % gray (slightly cold / blue)
	m_colorLabels = QColor(192, 202, 212, 255);	// ~90 % gray (slightly cold / blue)
}


// Create the SaControlDialog widget which handles display of GUI elements.
gui::EffectControlDialog* SaControls::createView()
{
	return new gui::SaControlsDialog(this, m_effect->getProcessor());
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

	m_envelopeResolutionModel.loadSettings(_this, "EnvelopeRes");
	m_spectrumResolutionModel.loadSettings(_this, "SpectrumRes");
	m_peakDecayFactorModel.loadSettings(_this, "PeakDecayFactor");
	m_averagingWeightModel.loadSettings(_this, "AverageWeight");
	m_waterfallHeightModel.loadSettings(_this, "WaterfallHeight");
	m_waterfallGammaModel.loadSettings(_this, "WaterfallGamma");
	m_windowOverlapModel.loadSettings(_this, "WindowOverlap");
	m_zeroPaddingModel.loadSettings(_this, "ZeroPadding");
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

	m_envelopeResolutionModel.saveSettings(doc, parent, "EnvelopeRes");
	m_spectrumResolutionModel.saveSettings(doc, parent, "SpectrumRes");
	m_peakDecayFactorModel.saveSettings(doc, parent, "PeakDecayFactor");
	m_averagingWeightModel.saveSettings(doc, parent, "AverageWeight");
	m_waterfallHeightModel.saveSettings(doc, parent, "WaterfallHeight");
	m_waterfallGammaModel.saveSettings(doc, parent, "WaterfallGamma");
	m_windowOverlapModel.saveSettings(doc, parent, "WindowOverlap");
	m_zeroPaddingModel.saveSettings(doc, parent, "ZeroPadding");

}


} // namespace lmms
