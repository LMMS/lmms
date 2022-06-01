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
#include "Tuner.h"
#include "TunerControls.h"

TunerControlDialog::TunerControlDialog(TunerControls* controls)
	: EffectControlDialog(controls)
	, m_centsWidget(new LcdWidget(3, this, tr("Cents")))
	, m_freqWidget(new LcdWidget(4, this, tr("Frequency")))
	, m_noteLabel(new QLabel(this))
{
	setFixedSize(240, 240);

	m_centsWidget->setValue(0);
	m_centsWidget->setLabel(tr("Cents"));
	m_centsWidget->move(7, 210);

	m_freqWidget->setValue(0);
	m_freqWidget->setLabel(tr("Frequency"));
	m_freqWidget->move(100, 210);

	m_noteLabel->setFont(QFont("Arial", 32));
	m_noteLabel->setText("-");
	m_noteLabel->setFixedWidth(width());
	m_noteLabel->setAlignment(Qt::AlignCenter);
	m_noteLabel->move(0, height() / 2 - m_noteLabel->font().pointSize());

	LcdSpinBox* referenceFreqSpinBox = new LcdSpinBox(3, this, tr("Reference"));
	referenceFreqSpinBox->setModel(&controls->m_referenceFreqModel);
	referenceFreqSpinBox->setLabel(tr("Reference"));
	referenceFreqSpinBox->move(193, 210);

}