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

#include <QGridLayout>
#include <QLabel>
#include <QSizePolicy>
#include <QWidget>

#include "embed.h"
#include "LedCheckbox.h"
#include "VecControls.h"
#include "Vectorscope.h"
#include "VectorView.h"


// The entire GUI layout is built here.
VecControlsDialog::VecControlsDialog(VecControls *controls) :
	EffectControlDialog(controls),
	m_controls(controls)
{
	QVBoxLayout *master_layout = new QVBoxLayout;
	setLayout(master_layout);

	// Visualizer widget
	VectorView *display = new VectorView(controls, m_controls->m_effect->getBuffer(), 512, this);
	master_layout->addWidget(display);

	// Config area located inside visualizer
	QVBoxLayout *internal_layout = new QVBoxLayout(display);
	QHBoxLayout *config_layout = new QHBoxLayout();
	QVBoxLayout *switch_layout = new QVBoxLayout();
	internal_layout->addStretch();
	internal_layout->addLayout(config_layout);
	config_layout->addLayout(switch_layout);

	// Log. scale switch
	LedCheckBox *logarithmicButton = new LedCheckBox(tr("Log. scale"), this);
	logarithmicButton->setToolTip(tr("Display amplitude on logarithmic scale to better see small values"));
	logarithmicButton->setCheckable(true);
	logarithmicButton->setMinimumSize(70, 12);
	logarithmicButton->setModel(&controls->m_logarithmicModel);
	switch_layout->addWidget(logarithmicButton);

	// Full-view switch
	LedCheckBox *fullViewButton = new LedCheckBox(tr("Full view"), this);
	fullViewButton->setToolTip(tr("Display amplitude on logarithmic scale to better see small values"));
	fullViewButton->setCheckable(true);
	fullViewButton->setMinimumSize(70, 12);
	fullViewButton->setModel(&controls->m_fullViewModel);
	switch_layout->addWidget(fullViewButton);

	config_layout->addStretch();

	// Persistence knob
	Knob *persistenceKnob = new Knob(knobSmall_17, this);
	persistenceKnob->setModel(&controls->m_persistenceModel);
	persistenceKnob->setLabel(tr("Persist."));
	persistenceKnob->setToolTip(tr("Trace persistence: higher amount means the trace will stay bright for longer time."));
	persistenceKnob->setHintText(tr("Trace persistence"), "");
	config_layout->addWidget(persistenceKnob);
}


// Suggest the best current widget size.
QSize VecControlsDialog::sizeHint() const
{
	return QSize(300, 300);
}
