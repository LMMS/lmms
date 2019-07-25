/*
 * VecControls.cpp - definition of VecControls class.
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

#include "VecControls.h"

#include <QtXml/QDomElement>

#include "Vectorscope.h"
#include "VecControlsDialog.h"


VecControls::VecControls(Vectorscope *effect) :
	EffectControls(effect),
	m_effect(effect),

	// initialize models and set default values
	m_persistenceModel(0.5f, 0.0f, 1.0f, 0.05f, this, tr("Display persistence amount")),
	m_logarithmicModel(false, this, tr("Logarithmic scale"))
{
	// Colors
	m_colorFG = QColor(51, 148, 204, 204);
	m_colorBG = QColor(7, 7, 7, 255);			// ~20 % gray (after gamma correction)
	m_colorGrid = QColor(30, 34, 38, 255);		// ~40 % gray (slightly cold / blue)
	m_colorLabels = QColor(192, 202, 212, 255);	// ~90 % gray (slightly cold / blue)
}


// Create the VecControlDialog widget which handles display of GUI elements.
EffectControlDialog* VecControls::createView()
{
	return new VecControlsDialog(this);
}


void VecControls::loadSettings(const QDomElement &_this)
{
	m_persistenceModel.loadSettings(_this, "Persistence");
	m_logarithmicModel.loadSettings(_this, "Logarithmic");
}


void VecControls::saveSettings(QDomDocument &doc, QDomElement &parent)
{
	m_persistenceModel.saveSettings(doc, parent, "Persistence");
	m_logarithmicModel.saveSettings(doc, parent, "Logarithmic");
}
