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

#include "Knob.h"
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
	master_layout->setContentsMargins(0, 2, 0, 0);
	setLayout(master_layout);

	// Visualizer widget
	// The size of 768 pixels seems to offer a good balance of speed, accuracy and trace thickness.
	auto display = new VectorView(controls, m_controls->m_effect->getBuffer(), 768, this);
	master_layout->addWidget(display);

	// Config area located inside visualizer
	auto internal_layout = new QVBoxLayout(display);
	auto config_layout = new QHBoxLayout();
	auto switch_layout = new QVBoxLayout();
	internal_layout->addStretch();
	internal_layout->addLayout(config_layout);
	config_layout->addLayout(switch_layout);

	// High-quality switch
	m_highQualityButton = new LedCheckBox(tr("HQ"), this);
	m_highQualityButton->setToolTip(tr("Double the resolution and simulate continuous analog-like trace."));
	m_highQualityButton->setCheckable(true);
	m_highQualityButton->setModel(&controls->m_highQualityModel);
	switch_layout->addWidget(m_highQualityButton);

	// Log. scale switch
	auto logarithmicButton = new LedCheckBox(tr("Log. scale"), this);
	logarithmicButton->setToolTip(tr("Display amplitude on logarithmic scale to better see small values."));
	logarithmicButton->setCheckable(true);
	logarithmicButton->setModel(&controls->m_logarithmicModel);
	switch_layout->addWidget(logarithmicButton);

	// Switch between lines mode and point mode
	auto linesMode = new LedCheckBox(tr("Lines"), this);
	linesMode->setToolTip(tr("Render with lines."));
	linesMode->setCheckable(true);
	linesMode->setModel(&controls->m_linesModeModel);
	switch_layout->addWidget(linesMode);

	// Switch between legacy mode and non-legacy mode
	auto legacyMode = new LedCheckBox(tr("Legacy"), this);
	legacyMode->setToolTip(tr("Render in legacy mode."));
	legacyMode->setCheckable(true);
	legacyMode->setModel(&controls->m_legacyModeModel);
	switch_layout->addWidget(legacyMode);

	config_layout->addStretch();

	// Persistence knob
	m_persistenceKnob = new Knob(KnobType::Small17, this);
	m_persistenceKnob->setModel(&controls->m_persistenceModel);
	m_persistenceKnob->setLabel(tr("Persist."));
	m_persistenceKnob->setToolTip(tr("Trace persistence: higher amount means the trace will stay bright for longer time."));
	m_persistenceKnob->setHintText(tr("Trace persistence"), "");
	config_layout->addWidget(m_persistenceKnob);

	connect(&m_controls->m_legacyModeModel, &BoolModel::dataChanged, [this] { onLegacyModelChanged(); });

	onLegacyModelChanged();
}


// Suggest the best widget size.
QSize VecControlsDialog::sizeHint() const
{
	return QSize(275, 300);
}

void VecControlsDialog::onLegacyModelChanged()
{
	const auto isPointMode = m_controls->m_legacyModeModel.value();
	m_highQualityButton->setVisible(isPointMode);
	m_persistenceKnob->setVisible(isPointMode);
}

} // namespace lmms::gui