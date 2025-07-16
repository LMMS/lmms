/*
 * OscilloscopeControlDialog.cpp - Oscilloscope effect gui window
 *
 * Copyright (c) 2025 Keratin
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "OscilloscopeControlDialog.h"
#include "OscilloscopeControls.h"
#include "OscilloscopeGraph.h"
#include "embed.h"
#include "Knob.h"
#include "LedCheckBox.h"
#include "PixmapButton.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

namespace lmms::gui
{

OscilloscopeControlDialog::OscilloscopeControlDialog(OscilloscopeControls* controls) :
	EffectControlDialog(controls)
{
	setAutoFillBackground(true);

	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(5);
	OscilloscopeGraph* graph = new OscilloscopeGraph(this, controls);
	mainLayout->addWidget(graph);

	QWidget* controlsWidget = new QWidget(this);
	mainLayout->addWidget(controlsWidget);
	QHBoxLayout* controlsLayout = new QHBoxLayout(controlsWidget);
	controlsLayout->setContentsMargins(5, 0, 5, 0);
	controlsLayout->setSpacing(10);


	PixmapButton* pauseButton = new PixmapButton(this, tr("Pause"));
	pauseButton->setToolTip(tr("Pause"));
	pauseButton->setActiveGraphic(embed::getIconPixmap("play"));
	pauseButton->setInactiveGraphic(embed::getIconPixmap("pause"));
	pauseButton->setCheckable(true);
	pauseButton->setModel(&controls->m_pauseModel);
	controlsLayout->addWidget(pauseButton);

	LedCheckBox* stereoCheck = new LedCheckBox("Stereo", this, tr("Stereo"), LedCheckBox::LedColor::Green);
	stereoCheck->setModel(&controls->m_stereoModel);
	controlsLayout->addWidget(stereoCheck);

	Knob* lengthKnob = new Knob(KnobType::Bright26, tr("Window Size"), this);
	lengthKnob->setModel(&controls->m_lengthModel);
	controlsLayout->addWidget(lengthKnob);

	Knob* phaseKnob = new Knob(KnobType::Bright26, tr("Offset"), this);
	phaseKnob->setModel(&controls->m_phaseModel);
	controlsLayout->addWidget(phaseKnob);

	Knob* ampknob = new Knob(KnobType::Bright26, tr("Scale"), this);
	ampknob->setModel(&controls->m_ampModel);
	ampknob->setVolumeKnob(true);
	controlsLayout->addWidget(ampknob);
}



} // namespace lmms::gui
