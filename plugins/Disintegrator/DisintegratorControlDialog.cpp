/*
 * DisintegratorControlDialog.cpp
 *
 * Copyright (c) 2019 Lost Robot <r94231@gmail.com>
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

#include <QLayout>

#include "DisintegratorControlDialog.h"
#include "DisintegratorControls.h"

#include "embed.h"
#include "gui_templates.h"



DisintegratorControlDialog::DisintegratorControlDialog(DisintegratorControls* controls) :
	EffectControlDialog(controls),
	m_controls(controls)
{
	setAutoFillBackground(true);
	QPalette pal;
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);
	setFixedSize(179, 97);

	m_lowCutKnob = new Knob(knobBright_26, this);
	m_lowCutKnob -> move(70, 10);
	m_lowCutKnob->setModel(&m_controls->m_lowCutModel);
	m_lowCutKnob->setLabel(tr("LOW CUT"));
	m_lowCutKnob->setHintText(tr("Low Cut:"), " Hz");

	m_highCutKnob = new Knob(knobBright_26, this);
	m_highCutKnob -> move(125, 10);
	m_highCutKnob->setModel(&m_controls->m_highCutModel);
	m_highCutKnob->setLabel(tr("HIGH CUT"));
	m_highCutKnob->setHintText(tr("High Cut:"), " Hz");

	m_amountKnob = new Knob(knobBright_26, this);
	m_amountKnob -> move(15, 10);
	m_amountKnob -> setVolumeKnob(true);
	m_amountKnob->setModel(&m_controls->m_amountModel);
	m_amountKnob->setLabel(tr("AMOUNT"));
	m_amountKnob->setHintText(tr("Amount:"), " ms");

	m_typeBox = new ComboBox(this);
	m_typeBox->setGeometry(1000, 5, 149, 22);
	m_typeBox->setFont(pointSize<8>(m_typeBox->font()));
	m_typeBox->move(14, 65);
	m_typeBox->setModel(&m_controls->m_typeModel);

	m_freqKnob = new Knob(knobBright_26, this);
	m_freqKnob -> move(132, 10);
	m_freqKnob->setModel(&m_controls->m_freqModel);
	m_freqKnob->setLabel(tr("FREQ"));
	m_freqKnob->setHintText(tr("Frequency:"), " Hz");

	connect(&m_controls->m_typeModel, SIGNAL(dataChanged()), this, SLOT(updateKnobVisibility()));
	emit m_controls->m_typeModel.dataChanged();// Needed to update knobs when view is opened
}


/* Switches between the lowcut/highcut and
frequency knobs depending on the modulation type. */
void DisintegratorControlDialog::updateKnobVisibility()
{
	if (m_controls->m_typeModel.value() == 2)// Sine Mode
	{
		m_lowCutKnob->hide();
		m_highCutKnob->hide();
		m_freqKnob->show();
	}
	else
	{
		m_lowCutKnob->show();
		m_highCutKnob->show();
		m_freqKnob->hide();
	}
}
