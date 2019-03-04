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

	m_stereo(true, this, tr("Display stereo channels separately")),
	m_smooth(false, this, tr("Smooth graph decay")),
	m_waterfall(false, this, tr("Display waterfall diagram")),
	m_log_x(true, this, tr("Logarithmic X axis scale")),
	m_log_y(false, this, tr("Logarithmic Y axis scale" ))
{
	m_inProgress = false;
}


void SaControls::loadSettings(const QDomElement &_this)
{
	m_stereo.loadSettings(_this, "Stereo");
	m_smooth.loadSettings(_this, "Smooth");
	m_waterfall.loadSettings(_this, "Waterfall");
	m_log_x.loadSettings(_this, "LogX");
	m_log_y.loadSettings(_this, "LogY");
}

EffectControlDialog* SaControls::createView()
{
	return new SaControlsDialog(this);
}


void SaControls::saveSettings(QDomDocument &doc, QDomElement &parent)
{
	m_stereo.saveSettings(doc, parent, "Stereo");
	m_smooth.saveSettings(doc, parent, "Smooth");
	m_waterfall.saveSettings(doc, parent, "Waterfall");
	m_log_x.saveSettings(doc, parent, "LogX");
	m_log_y.saveSettings(doc, parent, "LogY");
}
