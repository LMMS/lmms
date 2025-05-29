/*
 * AmplifierControlDialog.cpp - control dialog for amplifier effect
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

#include "AmplifierControlDialog.h"
#include "AmplifierControls.h"
#include "embed.h"
#include "Knob.h"

#include <QGridLayout>


namespace lmms::gui
{

AmplifierControlDialog::AmplifierControlDialog(AmplifierControls* controls) :
	EffectControlDialog(controls)
{
	setAutoFillBackground(true);
	QPalette pal;
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);

	QGridLayout* gridLayout = new QGridLayout(this);
	
	auto makeKnob = [this](const QString& label, const QString& hintText, const QString& unit, FloatModel* model, bool isVolume)
	{
		Knob* newKnob = new Knob(KnobType::Bright26, label, this);
		newKnob->setModel(model);
		newKnob->setHintText(hintText, unit);
		newKnob->setVolumeKnob(isVolume);
		return newKnob;
	};

	gridLayout->addWidget(makeKnob(tr("VOL"), tr("Volume:"), "%", &controls->m_volumeModel, true), 0, 0, Qt::AlignHCenter);
	gridLayout->addWidget(makeKnob(tr("PAN"), tr("Panning:"), "%", &controls->m_panModel, false), 0, 1, Qt::AlignHCenter);
	gridLayout->addWidget(makeKnob(tr("LEFT"), tr("Left gain:"), "%", &controls->m_leftModel, true), 1, 0, Qt::AlignHCenter);
	gridLayout->addWidget(makeKnob(tr("RIGHT"), tr("Right gain:"), "%", &controls->m_rightModel, true), 1, 1, Qt::AlignHCenter);
}

} // namespace lmms::gui
