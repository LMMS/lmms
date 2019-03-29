/*
 * SaControls.cpp - definition of SaControls class.
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

#include "SaControls.h"

#include <QtXml/QDomElement>

#include "SaControlsDialog.h"
#include "Analyzer.h"


SaControls::SaControls(Analyzer *effect) :
	EffectControls(effect),
	m_effect(effect),

	m_stereoModel(true, this, tr("Display stereo channels separately")),
	m_smoothModel(false, this, tr("Smooth graph decay")),
	m_waterfallModel(false, this, tr("Display waterfall diagram")),
	m_logXModel(true, this, tr("Logarithmic X axis scale")),
	m_logYModel(true, this, tr("Logarithmic Y axis scale")),
	m_peakHoldModel(false, this, tr("Keep the peak values displayed")),
	m_refFreezeModel(false, this, tr("Freeze current input as a reference"))
{
	m_colorL = QColor(51, 148, 204, 135);		// Make sure the sum of L and R
	m_colorR = QColor(204, 107, 51, 135);		// stays lower or equal to 255.
	m_colorMono = QColor(51, 148, 204, 204);
	m_colorBG = QColor(7, 7, 7, 255);			// 20 % gray
	m_colorGrid = QColor(30, 34, 38, 255);		// 40 % gray (slightly cold / blue)
	m_colorLabels = QColor(192, 202, 212, 255);	// 90 % gray (slightly cold / blue)
}


void SaControls::loadSettings(const QDomElement &_this)
{
	m_stereoModel.loadSettings(_this, "Stereo");
	m_smoothModel.loadSettings(_this, "Smooth");
	m_waterfallModel.loadSettings(_this, "Waterfall");
	m_logXModel.loadSettings(_this, "LogX");
	m_logYModel.loadSettings(_this, "LogY");
	m_peakHoldModel.loadSettings(_this, "PeakHold");
}

EffectControlDialog* SaControls::createView()
{
	return new SaControlsDialog(this, m_effect->getProcessor());
}


void SaControls::saveSettings(QDomDocument &doc, QDomElement &parent)
{
	m_stereoModel.saveSettings(doc, parent, "Stereo");
	m_smoothModel.saveSettings(doc, parent, "Smooth");
	m_waterfallModel.saveSettings(doc, parent, "Waterfall");
	m_logXModel.saveSettings(doc, parent, "LogX");
	m_logYModel.saveSettings(doc, parent, "LogY");
	m_peakHoldModel.saveSettings(doc, parent, "PeakHold");
}

