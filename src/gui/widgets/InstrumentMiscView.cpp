/*
 * InstrumentMiscView.cpp - Miscellaneous instrument settings
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2019 Martin Pavelek <he29.HS/at/gmail.com>
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

#include "InstrumentMiscView.h"

#include <QLabel>
#include <QLayout>

#include "ComboBox.h"
#include "GroupBox.h"
#include "InstrumentTrack.h"
#include "LcdSpinBox.h"


InstrumentMiscView::InstrumentMiscView(InstrumentTrack *it, QWidget *parent) :
	QWidget(parent)
{
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setMargin(5);

	// Master pitch toggle
	m_pitchGroupBox = new GroupBox(tr("MASTER PITCH"));
	m_pitchGroupBox->setModel(&it->m_useMasterPitchModel);
	layout->addWidget(m_pitchGroupBox);

	QHBoxLayout *masterPitchLayout = new QHBoxLayout(m_pitchGroupBox);
	masterPitchLayout->setContentsMargins(8, 18, 8, 8);

	QLabel *tlabel = new QLabel(tr("Enables the use of master pitch"));
	masterPitchLayout->addWidget(tlabel);

	// Microtuner settings
	m_microtunerGroupBox = new GroupBox(tr("MICROTUNER"));
//	m_microtunerGroupBox->setModel(&it->m_useMicrotunerModel);
	layout->addWidget(m_microtunerGroupBox);

	QHBoxLayout *microtunerLayout = new QHBoxLayout(m_microtunerGroupBox);
	microtunerLayout->setContentsMargins(8, 18, 8, 8);

	ComboBox *scaleCombo = new ComboBox(this, tr("Scale"));
	scaleCombo->setToolTip(tr("Scale"));
	microtunerLayout->addWidget(scaleCombo);

	LcdSpinBox *baseFreqSpinBox = new LcdSpinBox(3, m_microtunerGroupBox);
	baseFreqSpinBox->setLabel(tr("BASE FREQ"));
	baseFreqSpinBox->setToolTip(tr("Base note frequency"));
	microtunerLayout->addWidget(baseFreqSpinBox);

	// fill remaining space
	layout->addStretch();
}

InstrumentMiscView::~InstrumentMiscView()
{

}
