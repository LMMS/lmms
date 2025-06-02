/*
 * OscilloscopeControlDialog.cpp - Example effect gui boilerplate code
 *
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
#include <QGroupBox>
#include <QDebug>

namespace lmms::gui
{

OscilloscopeControlDialog::OscilloscopeControlDialog(OscilloscopeControls* controls) :
	EffectControlDialog(controls)
{
	setAutoFillBackground(true);
	/* Uncomment this if you want to use a background image
	QPalette pal;
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);
	setFixedSize(100, 110);
	*/

	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	OscilloscopeGraph* graph = new OscilloscopeGraph(this, controls);
	mainLayout->addWidget(graph);

	QGroupBox* controlsWidget = new QGroupBox(this);
	mainLayout->addWidget(controlsWidget);
	QHBoxLayout* controlsLayout = new QHBoxLayout(controlsWidget);
	controlsLayout->setContentsMargins(5, 5, 5, 5);


	PixmapButton* pauseButton = new PixmapButton(this, tr("Pause"));
	pauseButton->setToolTip(tr("Pause"));
	pauseButton->setActiveGraphic(embed::getIconPixmap("play"));
	pauseButton->setInactiveGraphic(embed::getIconPixmap("pause"));
	pauseButton->setCheckable(true);
	pauseButton->setModel(&controls->m_pauseModel);
	controlsLayout->addWidget(pauseButton);

	Knob* lengthknob = new Knob(KnobType::Bright26, this);
	lengthknob->setModel(&controls->m_lengthModel);
	lengthknob->setLabel(tr("Length"));
	controlsLayout->addWidget(lengthknob);

	Knob* ampknob = new Knob(KnobType::Bright26, this);
	ampknob->setModel(&controls->m_ampModel);
	ampknob->setLabel(tr("Scale"));
	ampknob->setVolumeKnob(true);
	controlsLayout->addWidget(ampknob);
}



} // namespace lmms::gui
