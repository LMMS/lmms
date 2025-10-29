/*
 * VecControlsDialog.cpp - definition of VecControlsDialog class.
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

#include "VecControlsDialog.h"


#include <QHBoxLayout>
#include <QVBoxLayout>

#include "LedCheckBox.h"
#include "VecControls.h"
#include "Vectorscope.h"
#include "VectorView.h"

namespace lmms::gui
{


// The entire GUI layout is built here.
VecControlsDialog::VecControlsDialog(VecControls *controls) :
	EffectControlDialog(controls),
	m_controls(controls)
{
	auto master_layout = new QVBoxLayout;
	master_layout->setContentsMargins(0, 0, 0, 0);
	setLayout(master_layout);

	// Visualizer widget
	auto display = new VectorView(controls, m_controls->m_effect->getBuffer(), this);
	master_layout->addWidget(display);

	auto controlLayout = new QHBoxLayout();
	master_layout->addLayout(controlLayout);

	// Log. scale switch
	auto logarithmicButton = new LedCheckBox(tr("Log. scale"), this);
	logarithmicButton->setToolTip(tr("Display amplitude on logarithmic scale to better see small values."));
	logarithmicButton->setCheckable(true);
	logarithmicButton->setModel(&controls->m_logarithmicModel);
	controlLayout->addWidget(logarithmicButton);

	controlLayout->addStretch();

	// Switch between lines mode and point mode
	auto linesMode = new LedCheckBox(tr("Lines"), this);
	linesMode->setToolTip(tr("Render with lines."));
	linesMode->setCheckable(true);
	linesMode->setModel(&controls->m_linesModeModel);
	controlLayout->addWidget(linesMode);
}


// Suggest the best widget size.
QSize VecControlsDialog::sizeHint() const
{
	return QSize(275, 300);
}

} // namespace lmms::gui