/*
 * TunerControlsDialog.cpp
 *
 * Copyright (c) 2022 saker <sakertooth@gmail.com>
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

#include "TunerControlDialog.h"

#include <QLabel>

#include "LcdFloatSpinBox.h"
#include "LcdSpinBox.h"
#include "TunerControls.h"

TunerControlDialog::TunerControlDialog(TunerControls* controls)
	: EffectControlDialog(controls)
{
	setFixedSize(320, 320);

	LcdSpinBox* referenceFreqSpinBox = new LcdSpinBox(3, this, tr("Reference"));
	referenceFreqSpinBox->setModel(&controls->m_referenceFreqModel);
	referenceFreqSpinBox->setValue(controls->m_referenceFreqModel.value());
	referenceFreqSpinBox->setLabel(tr("Reference"));
	referenceFreqSpinBox->move(270, 280);

	QLabel* playedNoteReadout = new QLabel(this);
	playedNoteReadout->setFont(QFont("Arial", 32));
	playedNoteReadout->setText("-");
	playedNoteReadout->setFixedWidth(width());
	playedNoteReadout->setAlignment(Qt::AlignCenter);
	playedNoteReadout->move(0, height()/2 - playedNoteReadout->font().pointSize());

	LcdWidget* freqReadout = new LcdWidget(2, this, tr("Cents"));
	freqReadout->setValue(0);
	freqReadout->setLabel(tr("Cents"));
	freqReadout->move(10, 280);

	//TODO: Add cent LED lights and alternate displays
}