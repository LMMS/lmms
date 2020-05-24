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
#include "LedCheckbox.h"


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
	int comboHeight = 22;	// using the same constant as zoom combo in SongEditor
	m_microtunerGroupBox = new GroupBox(tr("MICROTUNER"));
	m_microtunerGroupBox->setModel(it->m_microtuner.enabledModel());
	layout->addWidget(m_microtunerGroupBox);

	QVBoxLayout *microtunerLayout = new QVBoxLayout(m_microtunerGroupBox);
	microtunerLayout->setContentsMargins(8, 18, 8, 8);

	QLabel *scaleLabel = new QLabel(tr("Active scale:"));
	microtunerLayout->addWidget(scaleLabel);

	ComboBox *scaleCombo = new ComboBox();
	scaleCombo->setModel(it->scaleModel());
	scaleCombo->setFixedHeight(comboHeight);
	microtunerLayout->addWidget(scaleCombo);

	QLabel *keymapLabel = new QLabel(tr("Active keymap:"));
	microtunerLayout->addWidget(keymapLabel);

	ComboBox *keymapCombo = new ComboBox();
	keymapCombo->setModel(it->keymapModel());
	keymapCombo->setFixedHeight(comboHeight);
	microtunerLayout->addWidget(keymapCombo);

	LedCheckBox *importCheckbox = new LedCheckBox(tr("Import first and last notes from keymap"), this);
	importCheckbox->setToolTip(tr("When enabled, the first and last note of this instrument will be overwritten with values specified by the active keymap."));
	importCheckbox->setCheckable(true);
	microtunerLayout->addWidget(importCheckbox);

	// Fill remaining space
	layout->addStretch();
}

InstrumentMiscView::~InstrumentMiscView()
{

}
