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


#include "VecControlsDialog.h"
#include "Vectorscope.h"

namespace lmms
{


VecControls::VecControls(Vectorscope *effect) :
	EffectControls(effect),
	m_effect(effect),

	// initialize models and set default values
	m_logarithmicModel(false, this, tr("Logarithmic scale")),
	m_linesModeModel(true, this, tr("Lines rendering"))
{
}


// Create the VecControlDialog widget which handles display of GUI elements.
gui::EffectControlDialog* VecControls::createView()
{
	return new gui::VecControlsDialog(this);
}


void VecControls::loadSettings(const QDomElement &element)
{
	m_logarithmicModel.loadSettings(element, "Logarithmic");
	m_linesModeModel.loadSettings(element, "LinesMode");
}


void VecControls::saveSettings(QDomDocument &document, QDomElement &element)
{
	m_logarithmicModel.saveSettings(document, element, "Logarithmic");
	m_linesModeModel.saveSettings(document, element, "LinesMode");
}


} // namespace lmms
