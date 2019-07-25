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

#include <QLabel>
#include <QSizePolicy>
#include <QWidget>

#include "embed.h"
#include "LedCheckbox.h"
#include "VecControls.h"


// The entire GUI layout is built here.
VecControlsDialog::VecControlsDialog(VecControls *controls) :
	EffectControlDialog(controls),
	m_controls(controls)
{
	LedCheckBox *logarithmicButton = new LedCheckBox(tr("Logarithmic scale"), this);
	logarithmicButton->setToolTip(tr("Display amplitude on logarithmic scale to better see small values"));
	logarithmicButton->setCheckable(true);
	logarithmicButton->setMinimumSize(70, 12);
	logarithmicButton->setModel(&controls->m_waterfallModel);

// anchor to left / top corner
	config_layout->addWidget(logarithmicButton, 0, 1);

// persistence knob

	VectorView *display = new VectorView(controls, this);

}


// Suggest the best current widget size.
QSize VecControlsDialog::sizeHint() const
{
	return QSize(300, 200);
}
